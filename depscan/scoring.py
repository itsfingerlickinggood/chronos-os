"""Turn collected facts into explainable risk signals and a 0-100 score.

Every point of the score comes from a named signal, so a report can always
answer "why is this package risky?".
"""
from __future__ import annotations

from datetime import datetime, timezone

from . import typosquat, versions
from .models import Dependency, PackageInfo, Signal

# score -> level thresholds
THRESHOLDS = [(75, "critical"), (50, "high"), (25, "medium"), (0, "low")]

VULN_WEIGHT = {"CRITICAL": 40, "HIGH": 30, "MEDIUM": 15, "UNKNOWN": 10, "LOW": 5}
VULN_CAP = 60

# Weak heuristics that are common among healthy, widely used packages. Their weight is
# halved once a package has this many weekly downloads.
POPULAR_DOWNLOADS = 1_000_000
DAMPENED = {"NEW_PUBLISHER", "INSTALL_SCRIPT", "SINGLE_MAINTAINER", "STALE", "NO_REPO",
            "SINGLE_RELEASE", "LOW_SCORECARD"}
# A lookalike name that is itself this widely used is a real project, not a typosquat.
ESTABLISHED_DOWNLOADS = 50_000


def level_for(score: int) -> str:
    return next(level for threshold, level in THRESHOLDS if score >= threshold)


def _age_days(when: datetime | None, now: datetime) -> int | None:
    return (now - when).days if when else None


def _years(days: int) -> str:
    return f"{days / 365:.1f} years"


def evaluate(dep: Dependency, info: PackageInfo, now: datetime | None = None) -> list[Signal]:
    now = now or datetime.now(timezone.utc)
    out: list[Signal] = []

    def add(code: str, weight: int, title: str, detail: str = "") -> None:
        out.append(Signal(code, weight, title, detail))

    if dep.direct_url:
        add("DIRECT_URL", 20, "Installed from a URL instead of the package registry",
            f"'{dep.direct_url}' bypasses registry checks; its contents can change without a new version.")
        return out

    if not info.exists:
        if info.errors:
            return out
        add("NOT_FOUND", 70, "Package does not exist on the public registry",
            "If this is a private/internal package, anyone can register this name publicly and "
            "your installer may fetch theirs instead (dependency confusion). If it is a typo, fix it.")
        _lookalike(dep, info, now, add)
        return out

    if info.errors and info.latest_version is None:
        return out

    downloads = info.downloads_weekly
    age = _age_days(info.first_release, now)

    # --- known bad -------------------------------------------------------
    malware = [v for v in info.vulnerabilities if v.severity == "MALWARE"]
    others = [v for v in info.vulnerabilities if v.severity != "MALWARE"]
    if malware:
        add("MALWARE", 100, "Reported as malicious",
            f"{malware[0].id}: {malware[0].summary or 'malicious package advisory'}")
    if others:
        total = min(sum(VULN_WEIGHT[v.severity] for v in others), VULN_CAP)
        counts: dict[str, int] = {}
        for v in others:
            counts[v.severity] = counts.get(v.severity, 0) + 1
        breakdown = ", ".join(f"{n} {sev.lower()}" for sev, n in counts.items())
        fixes = [v.fixed_in for v in others if v.fixed_in]
        fix = versions.highest(fixes)
        detail = breakdown + (f". Upgrade to {fix} or later." if fix else ". No fixed version recorded.")
        plural = "y" if len(others) == 1 else "ies"
        add("VULNERABLE", total,
            f"{len(others)} known vulnerabilit{plural} in {info.evaluated_version}", detail)

    if info.version_exists is False:
        add("VERSION_NOT_FOUND", 30, f"Version {dep.version} is not published on the registry",
            f"The install will fail or resolve unexpectedly. Latest is {info.latest_version}.")
    if info.version_yanked:
        add("YANKED", 25, f"Version {info.evaluated_version} was yanked by its maintainers",
            "Releases are yanked when they are broken or unsafe.")
    if info.deprecated:
        add("DEPRECATED", 25, "Deprecated by its maintainers", info.deprecated)

    # --- impersonation ---------------------------------------------------
    _lookalike(dep, info, now, add)

    # --- takeover / install-time execution -------------------------------
    if info.new_publisher:
        add("NEW_PUBLISHER", 20, "Latest release came from a first-time publisher",
            f"'{info.new_publisher}' never published an earlier version. Account takeovers and "
            "handed-over projects look like this; review the release before upgrading.")
    if info.install_scripts:
        add("INSTALL_SCRIPT", 12, "Runs a script at install time",
            f"Defines {', '.join(info.install_scripts)}; code executes on every machine that installs it.")

    # --- health ----------------------------------------------------------
    if age is not None and age < 30:
        add("NEW_PACKAGE", 25, f"Brand-new package (first published {age} days ago)",
            "Too young to have earned trust; most malicious packages are caught in their first weeks.")
    elif age is not None and age < 90:
        add("NEW_PACKAGE", 15, f"New package (first published {age} days ago)",
            "Little track record so far.")

    stale = _age_days(info.latest_release, now)
    if stale is not None and stale > 4 * 365:
        add("STALE", 20, f"No release in {_years(stale)}", "Likely abandoned; security fixes are unlikely.")
    elif stale is not None and stale > 2 * 365:
        add("STALE", 10, f"No release in {_years(stale)}", "May no longer be maintained.")
    if info.scorecard_maintained == 0 and (stale is None or stale > 365):
        add("UNMAINTAINED_REPO", 10, "Source repository shows no recent activity",
            "OpenSSF Scorecard 'Maintained' check scored 0/10.")

    if downloads is not None and downloads < 100:
        add("LOW_DOWNLOADS", 15, f"Almost nobody uses it ({downloads} downloads last week)",
            "Problems in rarely used packages go unnoticed for longer.")
    elif downloads is not None and downloads < 1000:
        add("LOW_DOWNLOADS", 8, f"Rarely used ({downloads} downloads last week)", "")

    if info.maintainers == 1:
        add("SINGLE_MAINTAINER", 8, "Single maintainer", "One compromised or unavailable account affects every user.")
    if info.repo_url is None:
        add("NO_REPO", 8, "No source repository linked", "The published code cannot be compared with public source.")
    elif info.scorecard is not None and info.scorecard < 4:
        add("LOW_SCORECARD", 8, f"Low OpenSSF Scorecard ({info.scorecard:.1f}/10)",
            f"Weak security practices in {info.repo_url}.")
    if info.release_count == 1 and (age is None or age >= 90):
        add("SINGLE_RELEASE", 5, "Only one release ever published", "")

    if not dep.pinned:
        add("UNPINNED", 3, "Version is not pinned",
            f"'{dep.spec or 'any version'}' lets a future (possibly compromised) release install automatically.")

    if downloads is not None and downloads >= POPULAR_DOWNLOADS:
        for signal in out:
            if signal.code in DAMPENED:
                signal.weight = signal.weight // 2
    return out


def _lookalike(dep: Dependency, info: PackageInfo, now: datetime, add) -> None:
    match = typosquat.find_lookalike(dep.name, dep.ecosystem)
    if match is None:
        return
    target, kind = match
    downloads = info.downloads_weekly
    if downloads is not None and downloads >= ESTABLISHED_DOWNLOADS:
        return
    age = _age_days(info.first_release, now)
    suspicious = (downloads is not None and downloads < 1000) or (age is not None and age < 90)
    if kind == "typo":
        add("TYPOSQUAT", 60 if suspicious else 45, f"Name looks like a misspelling of '{target}'",
            f"'{dep.name}' is one edit away from the popular package '{target}'. "
            "Typosquatting is the most common way malware reaches developers. Did you mean "
            f"'{target}'?")
    else:
        add("COMBOSQUAT", 30 if suspicious else 15, f"Name builds on the popular package '{target}'",
            f"'{dep.name}' is '{target}' plus a generic prefix/suffix. Check it is the official package.")


def score(signals: list[Signal]) -> int:
    return min(100, sum(s.weight for s in signals))
