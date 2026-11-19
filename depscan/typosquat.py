"""Detect package names that look like a misspelling of a popular package."""
from __future__ import annotations

import re
from functools import lru_cache
from importlib import resources

from .parsers import normalize_pypi

# Only the most-downloaded names are worth impersonating; a longer list just adds false positives.
TARGET_LIMIT = {"pypi": 1000, "npm": 400}
MIN_TARGET_LEN = 5

_HOMOGLYPHS = [("rn", "m"), ("vv", "w"), ("0", "o"), ("1", "l"), ("5", "s"), ("3", "e")]
_AFFIXES = {
    "pypi": (["python-", "py-", "py"], ["-python", "-py", "-lib", "-pkg", "-sdk", "-api", "-dev"]),
    "npm": (["node-", "js-"], ["-js", "js", ".js", "-node", "-lib", "-pkg", "-sdk", "-api", "-dev"]),
}


@lru_cache(maxsize=None)
def popular(ecosystem: str) -> dict[str, int]:
    """Popular package name -> approximate weekly downloads, most popular first."""
    text = resources.files("depscan.data").joinpath(f"popular_{ecosystem}.txt").read_text()
    out: dict[str, int] = {}
    for line in text.splitlines():
        if not line or line.startswith("#"):
            continue
        name, _, count = line.partition("\t")
        downloads = int(count or 0)
        if ecosystem == "pypi":
            name, downloads = normalize_pypi(name), downloads * 7 // 30  # list stores monthly
        out[name] = downloads
    return out


@lru_cache(maxsize=None)
def _targets(ecosystem: str) -> tuple[str, ...]:
    names = list(popular(ecosystem))[: TARGET_LIMIT[ecosystem]]
    return tuple(n for n in names if len(n) >= MIN_TARGET_LEN)


def osa_distance(a: str, b: str, limit: int = 2) -> int:
    """Optimal string alignment distance (insert, delete, substitute, swap neighbours)."""
    if abs(len(a) - len(b)) > limit:
        return limit + 1
    prev2: list[int] = []
    prev = list(range(len(b) + 1))
    for i, ca in enumerate(a, 1):
        cur = [i] + [0] * len(b)
        for j, cb in enumerate(b, 1):
            cost = 0 if ca == cb else 1
            cur[j] = min(prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + cost)
            if i > 1 and j > 1 and ca == b[j - 2] and a[i - 2] == cb:
                cur[j] = min(cur[j], prev2[j - 2] + 1)
        prev2, prev = prev, cur
    return prev[-1]


def _squash(name: str) -> str:
    return re.sub(r"[-_.]", "", name)


def _deglyph(name: str) -> str:
    for fake, real in _HOMOGLYPHS:
        name = name.replace(fake, real)
    return name


def find_lookalike(name: str, ecosystem: str) -> tuple[str, str] | None:
    """Return (popular_name, kind) if `name` imitates a popular package.

    kind is "typo" for near-identical spellings and "combo" for a popular name
    wrapped in a generic prefix/suffix (python-requests, lodash-js ...).
    """
    name = normalize_pypi(name) if ecosystem == "pypi" else name.lower()
    known = popular(ecosystem)
    if name in known or name.startswith("@"):
        return None  # scoped npm packages are namespaced, so not typosquats of unscoped names

    targets = _targets(ecosystem)
    squashed, deglyphed = _squash(name), _deglyph(_squash(name))
    for target in targets:
        t = _squash(target)
        if squashed == t or (deglyphed == _deglyph(t) and squashed != t):
            return target, "typo"
    for target in targets:
        if abs(len(name) - len(target)) <= 1 and osa_distance(name, target, 1) == 1:
            return target, "typo"

    prefixes, suffixes = _AFFIXES[ecosystem]
    target_set = set(targets)
    for prefix in prefixes:
        if name.startswith(prefix) and name[len(prefix):] in target_set:
            return name[len(prefix):], "combo"
    for suffix in suffixes:
        if name.endswith(suffix) and name[: -len(suffix)] in target_set:
            return name[: -len(suffix)], "combo"
    return None
