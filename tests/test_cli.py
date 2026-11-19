import json

import pytest

from depscan import cli, osv, report
from depscan.scanner import scan
from tests.conftest import NOW, FakeFetcher, npm_doc, pypi_doc

ROUTES = {
    "https://pypi.org/pypi/widget/json": pypi_doc(),
    "https://pypistats.org/api/packages/widget/recent": {"data": {"last_week": 90_000}},
    "https://pypi.org/pypi/oldlib/json": pypi_doc("1.1.0", {"1.0.0": "2015-01-01T00:00:00Z",
                                                            "1.1.0": "2016-01-01T00:00:00Z"}),
    "https://registry.npmjs.org/gadget": npm_doc({"1.0.0": "2021-01-01T00:00:00Z",
                                                  "1.1.0": "2026-09-01T00:00:00Z"}),
    "https://api.npmjs.org/downloads/point/last-week/gadget": {"downloads": 55_000},
    (osv.OSV_URL, "oldlib"): {"vulns": [
        {"id": "GHSA-crit", "summary": "Remote code <script> | execution",
         "database_specific": {"severity": "CRITICAL"},
         "affected": [{"package": {"name": "oldlib"}, "ranges": [
             {"type": "ECOSYSTEM", "events": [{"introduced": "0"}, {"fixed": "1.2.0"}]}]}]},
        {"id": "GHSA-high", "summary": "Another one", "database_specific": {"severity": "HIGH"}},
    ]},
}


@pytest.fixture
def project(tmp_path, monkeypatch):
    monkeypatch.setattr(cli, "Fetcher", lambda cache_dir=None: FakeFetcher(ROUTES))
    monkeypatch.chdir(tmp_path)
    (tmp_path / "requirements.txt").write_text("widget==2.0.0\noldlib==1.0.0\nreqeusts==1.0\n")
    (tmp_path / "package.json").write_text(json.dumps(
        {"dependencies": {"gadget": "1.1.0"}, "devDependencies": {"ghost-internal-pkg": "1.0.0"}}))
    return tmp_path


def test_scan_table_and_reports(project, capsys):
    code = cli.main(["scan", ".", "--json", "out/report.json", "--markdown", "out/report.md",
                     "--html", "out/report.html"])
    out = capsys.readouterr().out
    assert code == 0
    assert out.index("reqeusts") < out.index("oldlib") < out.index("widget")  # most risky first
    assert "5 packages scanned: 2 critical, 1 high, 0 medium, 2 low." in out

    data = json.loads((project / "out/report.json").read_text())
    levels = {r["name"]: r["level"] for r in data["results"]}
    assert levels == {"reqeusts": "critical", "oldlib": "critical", "ghost-internal-pkg": "high",
                      "widget": "low", "gadget": "low"}
    oldlib = next(r for r in data["results"] if r["name"] == "oldlib")
    assert [v["id"] for v in oldlib["vulnerabilities"]] == ["GHSA-crit", "GHSA-high"]
    assert oldlib["vulnerabilities"][0]["fixed_in"] == "1.2.0"

    markdown = (project / "out/report.md").read_text()
    assert markdown.startswith(report.MARKER)
    assert "Remote code &lt;script> \\| execution" in markdown  # table-breaking text is escaped

    html = (project / "out/report.html").read_text()
    assert "__DEPSCAN_DATA__" not in html and '"name": "oldlib"' in html


def test_html_report_cannot_be_broken_out_of(project):
    html = report.render_html(scan(cli._parse_all([project / "requirements.txt"], True),
                                   FakeFetcher({**ROUTES, (osv.OSV_URL, "oldlib"): {"vulns": [
                                       {"id": "GHSA-x", "summary": "</script><script>alert(1)</script>"}]}}),
                                   now=NOW))
    assert "</script><script>alert(1)" not in html


def test_fail_on_threshold(project):
    assert cli.main(["scan", "requirements.txt", "--fail-on", "critical"]) == 1
    assert cli.main(["scan", "package.json", "--fail-on", "critical"]) == 0
    assert cli.main(["scan", "package.json", "--fail-on", "high"]) == 1
    assert cli.main(["scan", "package.json", "--no-dev", "--fail-on", "medium"]) == 0


def test_shorthand_without_subcommand(project, capsys):
    assert cli.main(["requirements.txt", "--format", "json"]) == 0
    assert len(json.loads(capsys.readouterr().out)["results"]) == 3


def test_only_new_against_baseline(project, capsys):
    base = project / "base"
    base.mkdir()
    (base / "requirements.txt").write_text("widget==2.0.0\noldlib==0.9.0\n")
    code = cli.main(["scan", "requirements.txt", "--baseline-dir", "base", "--only-new", "--format", "json"])
    names = sorted(r["name"] for r in json.loads(capsys.readouterr().out)["results"])
    assert code == 0
    assert names == ["oldlib", "reqeusts"]  # widget is unchanged, oldlib changed version, reqeusts is new


def test_only_new_with_nothing_new(project, capsys):
    base = project / "base"
    base.mkdir()
    (base / "requirements.txt").write_text((project / "requirements.txt").read_text())
    cli.main(["scan", "requirements.txt", "--baseline-dir", "base", "--only-new", "--format", "markdown"])
    assert "No new or changed dependencies" in capsys.readouterr().out


def test_check_single_package(project, capsys):
    assert cli.main(["check", "gadget@1.1.0", "-e", "npm"]) == 0
    assert "gadget" in capsys.readouterr().out
    assert cli.main(["check", "oldlib==1.0.0", "--fail-on", "high"]) == 1


def test_usage_errors(project, capsys):
    assert cli.main(["scan", "missing.txt"]) == 2
    (project / "empty").mkdir()
    assert cli.main(["scan", "empty"]) == 2
    (project / "bad").mkdir()
    (project / "bad" / "package.json").write_text("{oops")
    assert cli.main(["scan", "bad"]) == 2


def test_duplicate_declarations_are_reported_once(project, capsys):
    (project / "requirements-dev.txt").write_text("widget==2.0.0\n")
    cli.main(["scan", ".", "--format", "json"])
    names = [r["name"] for r in json.loads(capsys.readouterr().out)["results"]]
    assert names.count("widget") == 1
