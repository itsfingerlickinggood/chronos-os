from __future__ import annotations

from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone

from . import registries, scoring
from .http import Fetcher
from .models import LEVELS, Dependency, PackageInfo, Result


def scan(deps: list[Dependency], fetcher: Fetcher, workers: int = 8,
         now: datetime | None = None, baseline: list[Dependency] | None = None) -> list[Result]:
    """Evaluate every dependency. Results are sorted most risky first."""
    now = now or datetime.now(timezone.utc)

    # The same package is often declared in several files; look each one up once.
    unique: dict[tuple, Dependency] = {}
    for dep in deps:
        unique.setdefault((dep.ecosystem, dep.name, dep.version, dep.direct_url), dep)

    def lookup(dep: Dependency) -> PackageInfo:
        try:
            return registries.collect(dep, fetcher)
        except Exception as exc:  # one bad package must not sink the whole scan
            info = PackageInfo(dep.name, dep.ecosystem)
            info.errors.append(f"unexpected error: {type(exc).__name__}: {exc}")
            return info

    with ThreadPoolExecutor(max_workers=max(1, workers)) as pool:
        infos = dict(zip(unique, pool.map(lookup, unique.values())))

    known = {d.key: d.spec for d in baseline} if baseline is not None else None
    results: list[Result] = []
    seen: set[tuple] = set()
    for dep in deps:
        ident = (dep.ecosystem, dep.name, dep.spec)
        if ident in seen:
            continue
        seen.add(ident)
        info = infos[(dep.ecosystem, dep.name, dep.version, dep.direct_url)]
        signals = scoring.evaluate(dep, info, now)
        signals.sort(key=lambda s: -s.weight)
        total = scoring.score(signals)
        is_new = None if known is None else known.get(dep.key) != dep.spec
        results.append(Result(dep, info, signals, total, scoring.level_for(total), is_new))
    results.sort(key=lambda r: (-r.score, r.dep.ecosystem, r.dep.name))
    return results


def at_least(results: list[Result], level: str) -> list[Result]:
    floor = LEVELS.index(level)
    return [r for r in results if LEVELS.index(r.level) >= floor]


def summarize(results: list[Result]) -> dict:
    counts = {level: 0 for level in LEVELS}
    for r in results:
        counts[r.level] += 1
    return {
        "total": len(results),
        "by_level": counts,
        "vulnerable": sum(1 for r in results if r.info.vulnerabilities),
        "incomplete": sum(1 for r in results if r.info.errors),
    }
