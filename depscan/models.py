from __future__ import annotations

from dataclasses import dataclass, field
from datetime import datetime

LEVELS = ["low", "medium", "high", "critical"]


@dataclass
class Dependency:
    """One declared dependency, as written in a manifest file."""

    name: str
    ecosystem: str  # "pypi" or "npm"
    spec: str = ""  # raw version specifier, e.g. "==2.0" or "^4.17.1"
    version: str | None = None  # concrete version to evaluate, when one can be derived
    pinned: bool = False
    source: str = ""
    dev: bool = False
    direct_url: str | None = None  # installed from a URL / git / path instead of the registry

    @property
    def key(self) -> tuple[str, str]:
        return (self.ecosystem, self.name)


@dataclass
class Vulnerability:
    id: str
    summary: str
    severity: str  # MALWARE, CRITICAL, HIGH, MEDIUM, LOW or UNKNOWN
    fixed_in: str | None = None
    aliases: list[str] = field(default_factory=list)

    @property
    def url(self) -> str:
        return f"https://osv.dev/vulnerability/{self.id}"


@dataclass
class PackageInfo:
    """Everything we learned about a package from public data sources."""

    name: str
    ecosystem: str
    exists: bool = True
    latest_version: str | None = None
    evaluated_version: str | None = None
    version_exists: bool | None = None
    version_yanked: bool = False
    deprecated: str | None = None
    first_release: datetime | None = None
    latest_release: datetime | None = None
    release_count: int = 0
    downloads_weekly: int | None = None
    maintainers: int | None = None
    new_publisher: str | None = None
    install_scripts: list[str] = field(default_factory=list)
    repo_url: str | None = None
    scorecard: float | None = None
    scorecard_maintained: int | None = None
    stars: int | None = None
    vulnerabilities: list[Vulnerability] = field(default_factory=list)
    errors: list[str] = field(default_factory=list)


@dataclass
class Signal:
    code: str
    weight: int
    title: str
    detail: str = ""


@dataclass
class Result:
    dep: Dependency
    info: PackageInfo
    signals: list[Signal]
    score: int
    level: str
    is_new: bool | None = None  # only set when a baseline was supplied
