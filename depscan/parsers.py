"""Read dependencies out of requirements.txt, pyproject.toml and package.json."""
from __future__ import annotations

import json
import re
import tomllib
from pathlib import Path

from .models import Dependency

SKIP_DIRS = {".git", "node_modules", ".venv", "venv", "env", "__pycache__", "dist", "build",
             ".tox", "site-packages"}

_REQ = re.compile(r"^([A-Za-z0-9][A-Za-z0-9._-]*)\s*(?:\[[^\]]*\])?\s*(.*)$")
_PIN = re.compile(r"^={2,3}\s*([^\s,;*]+)$")
_SEMVER = re.compile(r"^\d+\.\d+\.\d+(?:[-+][0-9A-Za-z.+-]+)?$")
_NPM_NON_REGISTRY = ("git+", "git:", "github:", "gitlab:", "bitbucket:", "http:", "https:",
                     "file:", "link:", "workspace:", "portal:")


class ParseError(Exception):
    pass


def normalize_pypi(name: str) -> str:
    """PEP 503 normalisation: case-insensitive, runs of -_. are equivalent."""
    return re.sub(r"[-_.]+", "-", name).lower()


def is_manifest(path: Path) -> bool:
    name = path.name.lower()
    if name in ("package.json", "pyproject.toml"):
        return True
    return name.endswith(".txt") and "requirements" in name


def discover(root: Path) -> list[Path]:
    """Find supported manifest files below a directory."""
    found: list[Path] = []
    for path in sorted(root.rglob("*")):
        if any(part in SKIP_DIRS for part in path.relative_to(root).parts):
            continue
        if path.is_file() and is_manifest(path):
            found.append(path)
    return found


def parse_file(path: Path) -> list[Dependency]:
    name = path.name.lower()
    try:
        if name == "package.json":
            return parse_package_json(path)
        if name == "pyproject.toml":
            return parse_pyproject(path)
        return parse_requirements(path)
    except (OSError, ValueError) as exc:
        raise ParseError(f"{path}: {exc}") from exc


# ---------------------------------------------------------------- Python
def parse_requirement_line(line: str, source: str = "", dev: bool = False) -> Dependency | None:
    """Parse one PEP 508 style requirement. Returns None for lines we ignore."""
    line = re.split(r"\s+#", " " + line, maxsplit=1)[0].strip()
    line = re.split(r"\s+--hash[=\s]", line, maxsplit=1)[0].strip()
    if not line:
        return None

    if re.match(r"^(git\+|hg\+|svn\+|bzr\+|https?://|file:|\.{0,2}/)", line):
        egg = re.search(r"[#&]egg=([A-Za-z0-9._-]+)", line)
        name = egg.group(1) if egg else line.rstrip("/").split("/")[-1].split("@")[0].removesuffix(".git")
        return Dependency(normalize_pypi(name) or line, "pypi", spec=line, source=source, dev=dev,
                          direct_url=line)

    m = _REQ.match(line)
    if not m:
        return None
    name = normalize_pypi(m.group(1))
    rest = m.group(2).split(";", 1)[0].strip()  # drop environment markers
    if rest.startswith("@"):
        return Dependency(name, "pypi", spec=rest, source=source, dev=dev, direct_url=rest[1:].strip())
    rest = rest.strip("()").replace(" ", "")
    pin = _PIN.match(rest)
    return Dependency(name, "pypi", spec=rest, version=pin.group(1) if pin else None,
                      pinned=bool(pin), source=source, dev=dev)


def parse_requirements(path: Path, _seen: set[Path] | None = None) -> list[Dependency]:
    seen = _seen if _seen is not None else set()
    resolved = path.resolve()
    if resolved in seen:
        return []
    seen.add(resolved)

    deps: list[Dependency] = []
    text = path.read_text(encoding="utf-8", errors="replace").replace("\\\n", " ")
    for raw in text.splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("-"):
            include = re.match(r"^(?:-r|--requirement)[=\s]\s*(\S+)", line)
            editable = re.match(r"^(?:-e|--editable)[=\s]\s*(\S+)", line)
            if include:
                target = path.parent / include.group(1)
                if target.is_file():
                    deps.extend(parse_requirements(target, seen))
            elif editable:
                dep = parse_requirement_line(editable.group(1), str(path))
                if dep is not None and dep.direct_url:
                    deps.append(dep)
            continue  # other pip options (--index-url, -c ...) are not dependencies
        dep = parse_requirement_line(line, str(path))
        if dep is not None:
            deps.append(dep)
    return deps


def parse_pyproject(path: Path) -> list[Dependency]:
    data = tomllib.loads(path.read_text(encoding="utf-8"))
    deps: list[Dependency] = []
    source = str(path)

    def add(lines, dev: bool) -> None:
        for line in lines or []:
            if isinstance(line, str):
                dep = parse_requirement_line(line, source, dev)
                if dep is not None:
                    deps.append(dep)

    project = data.get("project", {})
    add(project.get("dependencies"), False)
    for group in (project.get("optional-dependencies") or {}).values():
        add(group, True)
    for group in (data.get("dependency-groups") or {}).values():
        add(group, True)

    poetry = data.get("tool", {}).get("poetry", {})
    poetry_groups = [(poetry.get("dependencies"), False), (poetry.get("dev-dependencies"), True)]
    poetry_groups += [(g.get("dependencies"), True) for g in (poetry.get("group") or {}).values()]
    for table, dev in poetry_groups:
        for name, value in (table or {}).items():
            if name.lower() == "python":
                continue
            if isinstance(value, dict):
                url = value.get("git") or value.get("url") or value.get("path")
                if url:
                    deps.append(Dependency(normalize_pypi(name), "pypi", spec=str(url), source=source,
                                           dev=dev, direct_url=str(url)))
                    continue
                value = value.get("version", "")
            spec = str(value).strip()
            exact = spec.lstrip("=").strip()
            pinned = bool(re.match(r"^=?=?\s*\d[^\s,*^~<>]*$", spec))
            deps.append(Dependency(normalize_pypi(name), "pypi", spec=spec,
                                   version=exact if pinned else None, pinned=pinned,
                                   source=source, dev=dev))
    return deps


# ---------------------------------------------------------------- npm
def parse_npm_spec(name: str, spec: str, source: str = "", dev: bool = False) -> Dependency:
    spec = (spec or "").strip()
    if spec.startswith(_NPM_NON_REGISTRY) or re.match(r"^[\w.-]+/[\w.-]+(#.*)?$", spec):
        return Dependency(name, "npm", spec=spec, source=source, dev=dev, direct_url=spec)
    if spec.startswith("npm:"):  # alias: "npm:real-name@^1.0.0"
        target = spec[4:]
        real, _, real_spec = target.rpartition("@") if "@" in target[1:] else (target, "", "")
        return parse_npm_spec(real or target, real_spec, source, dev)
    pinned = bool(_SEMVER.match(spec.lstrip("=v")))
    version = None
    base = spec.lstrip("^~>=<v ").split(" ")[0]
    if _SEMVER.match(base):
        # For ranges like ^4.17.1 we evaluate the lowest version the range allows.
        version = base
    return Dependency(name, "npm", spec=spec, version=version, pinned=pinned, source=source, dev=dev)


def parse_package_json(path: Path) -> list[Dependency]:
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise ValueError("package.json must contain a JSON object")
    deps: list[Dependency] = []
    for section, dev in (("dependencies", False), ("optionalDependencies", False),
                         ("peerDependencies", False), ("devDependencies", True)):
        table = data.get(section) or {}
        if not isinstance(table, dict):
            continue
        for name, spec in table.items():
            deps.append(parse_npm_spec(str(name), str(spec), str(path), dev))
    return deps
