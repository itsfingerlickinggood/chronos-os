"""Render scan results as a terminal table, JSON, Markdown or a standalone HTML page."""
from __future__ import annotations

import json
from datetime import datetime, timezone
from importlib import resources

from . import __version__
from .models import LEVELS, Result
from .scanner import summarize

MARKER = "<!-- depscan-report -->"
_COLOR = {"critical": "\033[1;31m", "high": "\033[31m", "medium": "\033[33m", "low": "\033[32m"}
_RESET, _DIM, _BOLD = "\033[0m", "\033[2m", "\033[1m"
_EMOJI = {"critical": "🔴", "high": "🟠", "medium": "🟡", "low": "🟢"}


def _iso(dt: datetime | None) -> str | None:
    return dt.isoformat() if dt else None


def _version(r: Result) -> str:
    """Best human-readable version for a result."""
    return r.info.evaluated_version or r.dep.version or r.dep.spec or ""


def to_dict(results: list[Result], generated_at: datetime | None = None) -> dict:
    generated_at = generated_at or datetime.now(timezone.utc)
    items = []
    for r in results:
        info = r.info
        items.append({
            "name": r.dep.name,
            "ecosystem": r.dep.ecosystem,
            "spec": r.dep.spec,
            "version": _version(r),
            "latest_version": info.latest_version,
            "pinned": r.dep.pinned,
            "dev": r.dep.dev,
            "source": r.dep.source,
            "score": r.score,
            "level": r.level,
            "is_new": r.is_new,
            "signals": [{"code": s.code, "weight": s.weight, "title": s.title, "detail": s.detail}
                        for s in r.signals],
            "vulnerabilities": [{"id": v.id, "summary": v.summary, "severity": v.severity,
                                 "fixed_in": v.fixed_in, "aliases": v.aliases, "url": v.url}
                                for v in info.vulnerabilities],
            "facts": {
                "downloads_weekly": info.downloads_weekly,
                "first_release": _iso(info.first_release),
                "latest_release": _iso(info.latest_release),
                "release_count": info.release_count,
                "maintainers": info.maintainers,
                "repository": info.repo_url,
                "scorecard": info.scorecard,
                "stars": info.stars,
            },
            "errors": info.errors,
        })
    return {
        "tool": "depscan",
        "version": __version__,
        "generated_at": generated_at.isoformat(timespec="seconds"),
        "summary": summarize(results),
        "results": items,
    }


def render_json(results: list[Result]) -> str:
    return json.dumps(to_dict(results), indent=2) + "\n"


# ---------------------------------------------------------------- terminal
def render_table(results: list[Result], color: bool = False, verbose: bool = False) -> str:
    def paint(text: str, code: str) -> str:
        return f"{code}{text}{_RESET}" if color else text

    if not results:
        return "No dependencies found.\n"

    rows = [(r.level.upper(), str(r.score), f"{r.dep.name}", _version(r) or "-",
             r.dep.ecosystem, r.signals[0].title if r.signals else "no risk signals") for r in results]
    headers = ("RISK", "SCORE", "PACKAGE", "VERSION", "ECO", "MAIN REASON")
    widths = [max(len(h), *(len(row[i]) for row in rows)) for i, h in enumerate(headers[:-1])]
    widths[2], widths[3] = min(widths[2], 40), min(widths[3], 16)

    def line(cells, level: str | None = None) -> str:
        parts = [cells[i][: widths[i]].ljust(widths[i]) for i in range(5)]
        if level:
            parts[0] = paint(parts[0], _COLOR[level])
        return "  ".join(parts + [cells[5]]).rstrip()

    out = [paint(line(headers), _BOLD), paint("-" * (sum(widths) + 30), _DIM)]
    out += [line(row, r.level) for row, r in zip(rows, results)]

    detailed = [r for r in results if verbose or r.level != "low"]
    if detailed:
        out.append("")
    for r in detailed:
        if not r.signals:
            continue
        title = f"{r.dep.name} {_version(r)}".strip()
        out.append(paint(f"{title}  [{r.level} {r.score}/100]  {r.dep.source}", _COLOR[r.level]))
        for s in r.signals:
            out.append(f"  +{s.weight:<3} {s.title}")
            if s.detail:
                out.append(paint(f"       {s.detail}", _DIM))
        for v in r.info.vulnerabilities[:5]:
            fix = f" (fixed in {v.fixed_in})" if v.fixed_in else ""
            out.append(paint(f"       - {v.severity:<8} {v.id}  {v.summary[:70]}{fix}", _DIM))
        if len(r.info.vulnerabilities) > 5:
            out.append(paint(f"       ... and {len(r.info.vulnerabilities) - 5} more", _DIM))
        out.append("")

    s = summarize(results)
    counts = ", ".join(f"{s['by_level'][lvl]} {lvl}" for lvl in reversed(LEVELS))
    noun = "package" if s["total"] == 1 else "packages"
    out.append(f"{s['total']} {noun} scanned: {counts}.")
    if s["incomplete"]:
        out.append(f"Warning: {s['incomplete']} package(s) could not be fully checked (network errors).")
    return "\n".join(out) + "\n"


# ---------------------------------------------------------------- markdown
def _md(text: str) -> str:
    return str(text).replace("|", "\\|").replace("\n", " ").replace("<", "&lt;")


def render_markdown(results: list[Result], only_new: bool = False) -> str:
    s = summarize(results)
    out = [MARKER, "## Dependency risk report", ""]
    if not results:
        out.append("No new or changed dependencies to review." if only_new else "No dependencies found.")
        return "\n".join(out) + "\n"

    scope = "new or changed packages" if only_new else "packages"
    counts = " · ".join(f"{_EMOJI[lvl]} {s['by_level'][lvl]} {lvl}" for lvl in reversed(LEVELS))
    out += [f"**{s['total']} {scope} scanned** — {counts}", ""]

    risky = [r for r in results if r.level != "low"]
    if not risky:
        out.append("No package scored above the low-risk threshold.")
    else:
        out += ["| Risk | Score | Package | Version | Why |", "|---|---:|---|---|---|"]
        for r in risky:
            reasons = "; ".join(_md(sig.title) for sig in r.signals[:2])
            if len(r.signals) > 2:
                reasons += f" (+{len(r.signals) - 2} more)"
            version = _version(r) or "-"
            out.append(f"| {_EMOJI[r.level]} {r.level} | {r.score} | `{_md(r.dep.name)}` ({r.dep.ecosystem}) "
                       f"| `{_md(version)}` | {reasons} |")
        out.append("")
        for r in risky:
            version = _version(r)
            out += [f"<details><summary><b>{_md(r.dep.name)}</b> {_md(version)} — {r.level} "
                    f"({r.score}/100)</summary>", ""]
            for sig in r.signals:
                detail = f" — {_md(sig.detail)}" if sig.detail else ""
                out.append(f"- **{_md(sig.title)}** (+{sig.weight}){detail}")
            if r.info.vulnerabilities:
                out += ["", "| Advisory | Severity | Fixed in | Summary |", "|---|---|---|---|"]
                for v in r.info.vulnerabilities[:8]:
                    out.append(f"| [{v.id}]({v.url}) | {v.severity.lower()} | {v.fixed_in or '-'} "
                               f"| {_md(v.summary)} |")
                if len(r.info.vulnerabilities) > 8:
                    out.append(f"\n…and {len(r.info.vulnerabilities) - 8} more advisories.")
            out += ["", "</details>", ""]

    low = s["by_level"]["low"]
    if low and risky:
        out.append(f"{low} other package(s) are low risk.")
    if s["incomplete"]:
        out.append(f"\n> {s['incomplete']} package(s) could not be fully checked because a data source "
                   "was unreachable.")
    out += ["", f"<sub>Generated by depscan {__version__}. Scores are heuristics, not verdicts.</sub>"]
    return "\n".join(out) + "\n"


# ---------------------------------------------------------------- html
def render_html(results: list[Result], title: str = "Dependency risk report") -> str:
    template = resources.files("depscan.data").joinpath("report.html").read_text(encoding="utf-8")
    data = to_dict(results)
    data["title"] = title
    payload = json.dumps(data).replace("</", "<\\/")  # keep </script> from ending the data block
    return template.replace("__DEPSCAN_TITLE__", _html_escape(title)).replace("__DEPSCAN_DATA__", payload)


def _html_escape(text: str) -> str:
    return text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace('"', "&quot;")
