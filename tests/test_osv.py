import pytest

from depscan import osv
from tests.conftest import FakeFetcher


@pytest.mark.parametrize("vector,score", [
    ("CVSS:3.1/AV:N/AC:L/PR:N/UI:N/S:U/C:H/I:H/A:H", 9.8),
    ("CVSS:3.1/AV:N/AC:L/PR:N/UI:R/S:C/C:L/I:L/A:N", 6.1),
    ("CVSS:3.1/AV:L/AC:H/PR:H/UI:R/S:U/C:L/I:N/A:N", 1.8),
    ("CVSS:3.0/AV:N/AC:L/PR:L/UI:N/S:C/C:H/I:H/A:H", 9.9),
    ("CVSS:3.1/AV:N/AC:L/PR:N/UI:N/S:U/C:N/I:N/A:N", 0.0),
])
def test_cvss3_base_score(vector, score):
    assert osv.cvss3_base_score(vector) == score


def test_cvss3_rejects_garbage():
    assert osv.cvss3_base_score("not a vector") is None


def _vuln(id_, severity=None, aliases=(), fixed=(("0", "1.5.0"),), cvss=None, cwe=()):
    return {
        "id": id_, "summary": f"summary of {id_}", "aliases": list(aliases),
        "database_specific": {"severity": severity, "cwe_ids": list(cwe)} if severity or cwe else {},
        "severity": [{"type": "CVSS_V3", "score": cvss}] if cvss else [],
        "affected": [{"package": {"name": "widget"}, "ranges": [
            {"type": "ECOSYSTEM", "events": [{"introduced": i}, {"fixed": f}]} for i, f in fixed]}],
    }


def fetch(vulns, version="1.2.0"):
    fetcher = FakeFetcher({(osv.OSV_URL, "widget"): {"vulns": vulns}})
    return osv.fetch_vulnerabilities("widget", "pypi", version, fetcher), fetcher


def test_duplicates_from_different_databases_are_merged():
    found, fetcher = fetch([
        _vuln("PYSEC-2024-1", aliases=["CVE-2024-1"]),
        _vuln("GHSA-aaaa", severity="HIGH", aliases=["CVE-2024-1"]),
        _vuln("GHSA-bbbb", severity="MODERATE"),
    ])
    assert [(v.id, v.severity) for v in found] == [("PYSEC-2024-1", "HIGH"), ("GHSA-bbbb", "MEDIUM")]
    assert "GHSA-aaaa" in found[0].aliases
    assert fetcher.posts[0] == {"package": {"name": "widget", "ecosystem": "PyPI"}, "version": "1.2.0"}


def test_severity_falls_back_to_cvss_vector():
    found, _ = fetch([_vuln("PYSEC-1", cvss="CVSS:3.1/AV:N/AC:L/PR:N/UI:N/S:U/C:H/I:H/A:H")])
    assert found[0].severity == "CRITICAL"


def test_malware_is_recognised():
    found, _ = fetch([_vuln("MAL-2025-1"), _vuln("GHSA-cccc", severity="HIGH", cwe=["CWE-506"])])
    assert [v.severity for v in found] == ["MALWARE", "MALWARE"]


def test_fix_comes_from_the_release_line_in_use():
    lines = (("1.0", "1.2.9"), ("2.0", "2.4.1"), ("3.0", "3.0.2"))
    found, _ = fetch([_vuln("GHSA-dddd", severity="LOW", fixed=lines)], version="2.1.0")
    assert found[0].fixed_in == "2.4.1"


def test_withdrawn_advisories_are_ignored():
    withdrawn = {**_vuln("GHSA-eeee", severity="HIGH"), "withdrawn": "2024-01-01T00:00:00Z"}
    assert fetch([withdrawn])[0] == []
