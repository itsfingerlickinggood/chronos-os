"""Collect package metadata from PyPI, npm, pypistats and deps.dev (all free, no keys)."""
from __future__ import annotations

import re
from datetime import datetime, timezone
from urllib.parse import quote

from . import osv, typosquat, versions
from .http import Fetcher, FetchError, NotFound
from .models import Dependency, PackageInfo

_REPO = re.compile(r"(github\.com|gitlab\.com|bitbucket\.org)[/:]([\w.-]+)/([\w.-]+)", re.I)
INSTALL_HOOKS = ("preinstall", "install", "postinstall")


def parse_time(value: str | None) -> datetime | None:
    if not value:
        return None
    try:
        dt = datetime.fromisoformat(value.replace("Z", "+00:00"))
    except ValueError:
        return None
    return dt if dt.tzinfo else dt.replace(tzinfo=timezone.utc)


def find_repo(*candidates) -> str | None:
    """First source-repository URL found, normalised to host/owner/repo."""
    for candidate in candidates:
        if isinstance(candidate, dict):
            candidate = candidate.get("url")
        m = _REPO.search(str(candidate or ""))
        if m and m.group(2).lower() not in ("sponsors", "orgs", "users"):
            repo = m.group(3).removesuffix(".git")
            return f"{m.group(1).lower()}/{m.group(2)}/{repo}"
    return None


def collect(dep: Dependency, fetcher: Fetcher) -> PackageInfo:
    """Gather every signal source for one dependency. Never raises for a missing source."""
    if dep.direct_url:
        return PackageInfo(dep.name, dep.ecosystem)
    try:
        info = _pypi(dep, fetcher) if dep.ecosystem == "pypi" else _npm(dep, fetcher)
    except FetchError as exc:
        info = PackageInfo(dep.name, dep.ecosystem)
        info.errors.append(f"registry lookup failed: {exc}")
        return info
    if not info.exists:
        return info

    try:
        info.vulnerabilities = osv.fetch_vulnerabilities(dep.name, dep.ecosystem, info.evaluated_version, fetcher)
    except (FetchError, NotFound) as exc:
        info.errors.append(f"vulnerability lookup failed: {exc}")
    _scorecard(info, fetcher)
    return info


# ---------------------------------------------------------------- PyPI
def _pypi(dep: Dependency, fetcher: Fetcher) -> PackageInfo:
    info = PackageInfo(dep.name, "pypi")
    try:
        doc = fetcher.get(f"https://pypi.org/pypi/{quote(dep.name)}/json")
    except NotFound:
        info.exists = False
        return info

    meta = doc.get("info") or {}
    releases = doc.get("releases") or {}
    uploaded = {}
    for version, files in releases.items():
        times = [t for t in (parse_time(f.get("upload_time_iso_8601")) for f in files or []) if t]
        if times:
            uploaded[version] = min(times)
    info.latest_version = meta.get("version")
    info.release_count = len(uploaded)
    if uploaded:
        info.first_release = min(uploaded.values())
        info.latest_release = max(uploaded.values())

    info.evaluated_version = dep.version or info.latest_version
    if dep.version:
        match = next((v for v in releases if versions.same(v, dep.version)), None)
        info.version_exists = match is not None
        if match is not None:
            info.evaluated_version = match
            files = releases[match] or []
            info.version_yanked = bool(files) and all(f.get("yanked") for f in files)
    elif meta.get("yanked"):
        info.version_yanked = True

    classifiers = meta.get("classifiers") or []
    if any("Development Status :: 7 - Inactive" in c for c in classifiers):
        info.deprecated = "marked 'Development Status :: 7 - Inactive' by its author"

    urls = meta.get("project_urls") or {}
    preferred = [v for k, v in urls.items() if re.search(r"source|repo|code|github", k, re.I)]
    info.repo_url = find_repo(*preferred, *urls.values(), meta.get("home_page"), meta.get("download_url"))

    known = typosquat.popular("pypi").get(dep.name)
    if known is not None:
        info.downloads_weekly = known
    else:
        try:
            stats = fetcher.get(f"https://pypistats.org/api/packages/{quote(dep.name)}/recent", retries=0)
            info.downloads_weekly = int((stats.get("data") or {}).get("last_week"))
        except (FetchError, NotFound, TypeError, ValueError):
            pass  # pypistats is rate limited; download count simply stays unknown
    return info


# ---------------------------------------------------------------- npm
def _npm(dep: Dependency, fetcher: Fetcher) -> PackageInfo:
    info = PackageInfo(dep.name, "npm")
    try:
        doc = fetcher.get("https://registry.npmjs.org/" + quote(dep.name, safe="@"))
    except NotFound:
        info.exists = False
        return info

    all_versions = doc.get("versions") or {}
    times = {v: parse_time(t) for v, t in (doc.get("time") or {}).items() if v in all_versions}
    times = {v: t for v, t in times.items() if t}
    if not all_versions:  # unpublished packages keep a stub document without versions
        info.exists = False
        return info

    info.latest_version = (doc.get("dist-tags") or {}).get("latest") or versions.highest(list(all_versions))
    info.release_count = len(all_versions)
    if times:
        info.first_release = min(times.values())
        info.latest_release = max(times.values())
    info.maintainers = len(doc.get("maintainers") or []) or None

    info.evaluated_version = info.latest_version
    if dep.version:
        info.version_exists = dep.version in all_versions
        if info.version_exists:
            info.evaluated_version = dep.version
    manifest = all_versions.get(info.evaluated_version) or {}

    if manifest.get("deprecated"):
        info.deprecated = str(manifest["deprecated"])[:200]
    scripts = manifest.get("scripts") or {}
    info.install_scripts = [h for h in INSTALL_HOOKS if scripts.get(h)]
    info.repo_url = find_repo(manifest.get("repository"), doc.get("repository"), doc.get("homepage"))

    # Did someone who never published this package before push the newest release?
    ordered = sorted(times, key=lambda v: times[v])
    if len(ordered) >= 5 and ordered[-1] == info.latest_version:
        def publisher(version: str) -> str | None:
            return ((all_versions.get(version) or {}).get("_npmUser") or {}).get("name")

        latest_publisher = publisher(ordered[-1])
        earlier = {publisher(v) for v in ordered[:-1]} - {None}
        if latest_publisher and earlier and latest_publisher not in earlier:
            info.new_publisher = latest_publisher

    try:
        stats = fetcher.get("https://api.npmjs.org/downloads/point/last-week/" + quote(dep.name, safe="@"))
        info.downloads_weekly = int(stats.get("downloads"))
    except (FetchError, NotFound, TypeError, ValueError):
        info.downloads_weekly = typosquat.popular("npm").get(dep.name)
    return info


# ---------------------------------------------------------------- deps.dev
def _scorecard(info: PackageInfo, fetcher: Fetcher) -> None:
    """OpenSSF Scorecard for the source repository, via Google's deps.dev."""
    if not info.repo_url or not info.repo_url.startswith("github.com/"):
        return
    try:
        doc = fetcher.get("https://api.deps.dev/v3/projects/" + quote(info.repo_url, safe=""), retries=1)
    except (FetchError, NotFound):
        return
    info.stars = doc.get("starsCount")
    card = doc.get("scorecard") or {}
    if isinstance(card.get("overallScore"), (int, float)):
        info.scorecard = float(card["overallScore"])
    for check in card.get("checks") or []:
        if check.get("name") == "Maintained" and isinstance(check.get("score"), int):
            info.scorecard_maintained = check["score"]
