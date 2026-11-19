"""Loose version handling that works well enough for both PyPI and npm."""
from __future__ import annotations

import re

_RELEASE = re.compile(r"^[vV=\s]*(\d+(?:\.\d+)*)")


def parse(version: str | None) -> tuple[int, ...] | None:
    """Numeric release segment as a tuple, ignoring pre/post/build suffixes."""
    if not version:
        return None
    m = _RELEASE.match(version)
    if not m:
        return None
    parts = [int(p) for p in m.group(1).split(".")]
    while len(parts) > 1 and parts[-1] == 0:
        parts.pop()
    return tuple(parts)


def same(a: str | None, b: str | None) -> bool:
    if a is None or b is None:
        return False
    if a.strip().lower() == b.strip().lower():
        return True
    pa, pb = parse(a), parse(b)
    return pa is not None and pa == pb and _suffix(a) == _suffix(b)


def _suffix(version: str) -> str:
    m = _RELEASE.match(version)
    return version[m.end():].strip().lower() if m else version


def less(a: str, b: str) -> bool:
    pa, pb = parse(a), parse(b)
    if pa is None or pb is None:
        return False
    return pa < pb


def highest(versions: list[str]) -> str | None:
    parsed = [(parse(v), v) for v in versions if parse(v) is not None]
    return max(parsed)[1] if parsed else None
