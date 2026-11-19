"""Known-vulnerability lookup against OSV.dev (free, no API key)."""
from __future__ import annotations

import math

from . import versions
from .http import Fetcher
from .models import Vulnerability

OSV_URL = "https://api.osv.dev/v1/query"
ECOSYSTEM = {"pypi": "PyPI", "npm": "npm"}
SEVERITY_RANK = {"MALWARE": 5, "CRITICAL": 4, "HIGH": 3, "MEDIUM": 2, "UNKNOWN": 1, "LOW": 0}

_CVSS3 = {
    "AV": {"N": 0.85, "A": 0.62, "L": 0.55, "P": 0.2},
    "AC": {"L": 0.77, "H": 0.44},
    "UI": {"N": 0.85, "R": 0.62},
    "CIA": {"H": 0.56, "L": 0.22, "N": 0.0},
    "PR_U": {"N": 0.85, "L": 0.62, "H": 0.27},
    "PR_C": {"N": 0.85, "L": 0.68, "H": 0.5},
}


def cvss3_base_score(vector: str) -> float | None:
    """Base score of a CVSS v3.x vector string, or None if it cannot be parsed."""
    try:
        parts = dict(p.split(":", 1) for p in vector.split("/")[1:])
        changed = parts["S"] == "C"
        c, i, a = (_CVSS3["CIA"][parts[k]] for k in ("C", "I", "A"))
        pr = _CVSS3["PR_C" if changed else "PR_U"][parts["PR"]]
        exploitability = 8.22 * _CVSS3["AV"][parts["AV"]] * _CVSS3["AC"][parts["AC"]] * pr * _CVSS3["UI"][parts["UI"]]
    except (KeyError, ValueError, IndexError):
        return None
    iss = 1 - (1 - c) * (1 - i) * (1 - a)
    impact = 7.52 * (iss - 0.029) - 3.25 * (iss - 0.02) ** 15 if changed else 6.42 * iss
    if impact <= 0:
        return 0.0
    total = min((1.08 if changed else 1.0) * (impact + exploitability), 10)
    return math.ceil(total * 10 - 1e-9) / 10


def _severity(vuln: dict) -> str:
    specific = vuln.get("database_specific") or {}
    # MAL-* ids come from the OpenSSF malicious-packages feed; CWE-506 is "embedded malicious code".
    if str(vuln.get("id", "")).startswith("MAL-") or "CWE-506" in (specific.get("cwe_ids") or []):
        return "MALWARE"
    label = str(specific.get("severity") or "").upper()
    if label == "MODERATE":
        label = "MEDIUM"
    if label in ("CRITICAL", "HIGH", "MEDIUM", "LOW"):
        return label
    for entry in vuln.get("severity") or []:
        if entry.get("type") == "CVSS_V3":
            score = cvss3_base_score(entry.get("score", ""))
            if score is None:
                continue
            if score >= 9:
                return "CRITICAL"
            if score >= 7:
                return "HIGH"
            if score >= 4:
                return "MEDIUM"
            return "LOW"
    return "UNKNOWN"


def _fixed_in(vuln: dict, name: str, version: str | None) -> str | None:
    """The fix version for the release line `version` sits in, if OSV records one."""
    containing: list[str] = []
    later: list[str] = []
    for affected in vuln.get("affected") or []:
        if str((affected.get("package") or {}).get("name", "")).lower() != name.lower():
            continue
        for rng in affected.get("ranges") or []:
            if rng.get("type") not in ("ECOSYSTEM", "SEMVER"):
                continue
            introduced = None
            for event in rng.get("events") or []:
                if "introduced" in event:
                    introduced = event["introduced"]
                fixed = event.get("fixed")
                if not fixed:
                    continue
                if version is None:
                    later.append(fixed)
                elif versions.less(version, fixed):
                    in_range = introduced in (None, "0") or not versions.less(version, introduced)
                    (containing if in_range else later).append(fixed)
    pool = containing or later
    if not pool:
        return None
    return min(pool, key=lambda v: versions.parse(v) or (10**9,))


def fetch_vulnerabilities(name: str, ecosystem: str, version: str | None, fetcher: Fetcher) -> list[Vulnerability]:
    body: dict = {"package": {"name": name, "ecosystem": ECOSYSTEM[ecosystem]}}
    if version:
        body["version"] = version
    raw: list[dict] = []
    for _ in range(5):  # follow pagination, bounded
        page = fetcher.post(OSV_URL, body) or {}
        raw.extend(page.get("vulns") or [])
        token = page.get("next_page_token")
        if not token:
            break
        body = {**body, "page_token": token}

    # The same issue is often published by several databases (GHSA, PYSEC, CVE...). Merge them.
    groups: list[tuple[set[str], Vulnerability]] = []
    for item in raw:
        if item.get("withdrawn"):
            continue
        ids = {item.get("id", "")} | set(item.get("aliases") or [])
        vuln = Vulnerability(
            id=item.get("id", "?"),
            summary=(item.get("summary") or (item.get("details") or "").strip().split("\n")[0])[:200],
            severity=_severity(item),
            fixed_in=_fixed_in(item, name, version),
            aliases=sorted(set(item.get("aliases") or [])),
        )
        for known_ids, existing in groups:
            if known_ids & ids:
                known_ids |= ids
                if SEVERITY_RANK[vuln.severity] > SEVERITY_RANK[existing.severity]:
                    existing.severity = vuln.severity
                if not existing.summary:
                    existing.summary = vuln.summary
                existing.fixed_in = existing.fixed_in or vuln.fixed_in
                existing.aliases = sorted((set(existing.aliases) | ids) - {existing.id})
                break
        else:
            groups.append((ids, vuln))
    result = [v for _, v in groups]
    result.sort(key=lambda v: (-SEVERITY_RANK[v.severity], v.id))
    return result
