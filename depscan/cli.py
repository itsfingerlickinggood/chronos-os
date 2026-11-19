from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path

from . import __version__, parsers, report
from .http import Fetcher, default_cache_dir
from .models import LEVELS, Dependency
from .scanner import at_least, scan

COMMANDS = {"scan", "check"}


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="depscan",
        description="Score the supply-chain risk of your PyPI and npm dependencies.",
    )
    parser.add_argument("--version", action="version", version=f"depscan {__version__}")
    sub = parser.add_subparsers(dest="command")

    common = argparse.ArgumentParser(add_help=False)
    common.add_argument("--format", choices=["table", "json", "markdown"], default="table",
                        help="what to print to the terminal (default: table)")
    common.add_argument("--json", metavar="FILE", dest="json_out", help="also write a JSON report")
    common.add_argument("--markdown", metavar="FILE", dest="markdown_out", help="also write a Markdown report")
    common.add_argument("--html", metavar="FILE", dest="html_out", help="also write a standalone HTML dashboard")
    common.add_argument("--fail-on", choices=LEVELS + ["none"], default="none",
                        help="exit with status 1 if any package is at or above this level")
    common.add_argument("--no-cache", action="store_true", help="do not read or write the on-disk cache")
    common.add_argument("--workers", type=int, default=8, help="parallel lookups (default: 8)")
    common.add_argument("-v", "--verbose", action="store_true", help="show details for low-risk packages too")

    p_scan = sub.add_parser("scan", parents=[common], help="scan manifest files or directories")
    p_scan.add_argument("paths", nargs="*", default=["."],
                        help="requirements*.txt, pyproject.toml, package.json or directories (default: .)")
    p_scan.add_argument("--no-dev", action="store_true", help="skip development-only dependencies")
    p_scan.add_argument("--baseline-dir", metavar="DIR",
                        help="directory holding the previous version of the same files (same relative paths)")
    p_scan.add_argument("--only-new", action="store_true",
                        help="with --baseline-dir: report only added or changed dependencies")
    p_scan.add_argument("--title", default="Dependency risk report", help="title for the HTML report")

    p_check = sub.add_parser("check", parents=[common], help="check a single package by name")
    p_check.add_argument("package", help="name, optionally with a version: requests==2.19.0 or lodash@4.17.15")
    p_check.add_argument("-e", "--ecosystem", choices=["pypi", "npm"], default="pypi")
    return parser


def _collect_files(paths: list[str]) -> list[Path]:
    files: list[Path] = []
    for raw in paths:
        path = Path(raw)
        if path.is_dir():
            files.extend(parsers.discover(path))
        elif path.is_file():
            files.append(path)
        else:
            raise parsers.ParseError(f"{raw}: no such file or directory")
    return files


def _parse_all(files: list[Path], include_dev: bool) -> list[Dependency]:
    deps: list[Dependency] = []
    for path in files:
        deps.extend(d for d in parsers.parse_file(path) if include_dev or not d.dev)
    return deps


def _baseline(files: list[Path], baseline_dir: str) -> list[Dependency]:
    deps: list[Dependency] = []
    root = Path(baseline_dir)
    for path in files:
        old = root / path
        if not old.is_file() and path.is_absolute():
            old = root / path.name
        if old.is_file():
            try:
                deps.extend(parsers.parse_file(old))
            except parsers.ParseError:
                pass  # an unreadable old file just means everything counts as new
    return deps


def _check_dependency(text: str, ecosystem: str) -> Dependency:
    if ecosystem == "npm":
        name, sep, version = text.rpartition("@") if "@" in text[1:] else (text, "", "")
        return parsers.parse_npm_spec(name if sep else text, version, "command line")
    dep = parsers.parse_requirement_line(text.replace("@", "==") if "==" not in text else text, "command line")
    if dep is None:
        raise parsers.ParseError(f"cannot parse package name: {text}")
    return dep


def main(argv: list[str] | None = None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    # `depscan requirements.txt` is shorthand for `depscan scan requirements.txt`.
    if argv and argv[0] not in COMMANDS and argv[0] not in ("-h", "--help", "--version"):
        argv.insert(0, "scan")
    elif not argv:
        argv = ["scan"]
    args = build_parser().parse_args(argv)

    try:
        baseline = None
        if args.command == "check":
            deps = [_check_dependency(args.package, args.ecosystem)]
            only_new = False
        else:
            files = _collect_files(args.paths)
            if not files:
                print("depscan: no requirements*.txt, pyproject.toml or package.json found.", file=sys.stderr)
                return 2
            deps = _parse_all(files, include_dev=not args.no_dev)
            if args.baseline_dir:
                baseline = _baseline(files, args.baseline_dir)
            only_new = bool(args.only_new and baseline is not None)
    except parsers.ParseError as exc:
        print(f"depscan: {exc}", file=sys.stderr)
        return 2

    fetcher = Fetcher(cache_dir=None if args.no_cache else default_cache_dir())
    results = scan(deps, fetcher, workers=args.workers, baseline=baseline)
    if only_new:
        results = [r for r in results if r.is_new]

    if args.format == "json":
        sys.stdout.write(report.render_json(results))
    elif args.format == "markdown":
        sys.stdout.write(report.render_markdown(results, only_new))
    else:
        color = sys.stdout.isatty() and not os.environ.get("NO_COLOR")
        sys.stdout.write(report.render_table(results, color=color, verbose=args.verbose))

    outputs = [
        (args.json_out, lambda: report.render_json(results)),
        (args.markdown_out, lambda: report.render_markdown(results, only_new)),
        (args.html_out, lambda: report.render_html(results, getattr(args, "title", "Dependency risk report"))),
    ]
    for target, render in outputs:
        if target:
            path = Path(target)
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(render(), encoding="utf-8")

    if args.fail_on != "none" and at_least(results, args.fail_on):
        return 1
    return 0
