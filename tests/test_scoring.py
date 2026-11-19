from datetime import timedelta

from depscan import scoring
from depscan.models import Dependency, PackageInfo, Vulnerability
from tests.conftest import NOW


def healthy(name="widget", ecosystem="pypi", **overrides) -> PackageInfo:
    info = PackageInfo(
        name, ecosystem, latest_version="2.0.0", evaluated_version="2.0.0",
        first_release=NOW - timedelta(days=2000), latest_release=NOW - timedelta(days=30),
        release_count=40, downloads_weekly=80_000, maintainers=3, repo_url="github.com/acme/widget",
        scorecard=7.5, scorecard_maintained=10,
    )
    for key, value in overrides.items():
        setattr(info, key, value)
    return info


def pinned(name="widget", ecosystem="pypi") -> Dependency:
    return Dependency(name, ecosystem, "==2.0.0", "2.0.0", True)


def codes(dep, info):
    return {s.code: s.weight for s in scoring.evaluate(dep, info, NOW)}


def test_healthy_pinned_package_scores_zero():
    assert scoring.evaluate(pinned(), healthy(), NOW) == []


def test_levels():
    assert [scoring.level_for(s) for s in (0, 24, 25, 49, 50, 74, 75, 100)] == \
        ["low", "low", "medium", "medium", "high", "high", "critical", "critical"]


def test_unpinned():
    assert codes(Dependency("widget", "pypi", ">=1.0"), healthy()) == {"UNPINNED": 3}


def test_vulnerability_weights_are_capped():
    vulns = [Vulnerability(f"GHSA-{i}", "x", "CRITICAL", fixed_in=f"2.0.{i}") for i in range(5)]
    signals = scoring.evaluate(pinned(), healthy(vulnerabilities=vulns), NOW)
    assert signals[0].code == "VULNERABLE" and signals[0].weight == scoring.VULN_CAP
    assert "Upgrade to 2.0.4 or later" in signals[0].detail


def test_malware_is_always_critical():
    vulns = [Vulnerability("MAL-2025-1", "Malicious code in widget", "MALWARE")]
    signals = scoring.evaluate(pinned(), healthy(vulnerabilities=vulns), NOW)
    assert scoring.level_for(scoring.score(signals)) == "critical"


def test_missing_package_is_dependency_confusion_risk():
    result = codes(pinned("acme-internal-billing"), PackageInfo("acme-internal-billing", "pypi", exists=False))
    assert result == {"NOT_FOUND": 70}


def test_missing_typo_package_is_critical():
    result = codes(pinned("reqeusts"), PackageInfo("reqeusts", "pypi", exists=False))
    assert set(result) == {"NOT_FOUND", "TYPOSQUAT"}
    assert scoring.score(scoring.evaluate(pinned("reqeusts"), PackageInfo("reqeusts", "pypi", exists=False), NOW)) == 100


def test_typosquat_weight_depends_on_track_record():
    young = healthy("reqeusts", downloads_weekly=40, first_release=NOW - timedelta(days=10))
    assert codes(pinned("reqeusts"), young)["TYPOSQUAT"] == 60
    unknown = healthy("reqeusts", downloads_weekly=None)
    assert codes(pinned("reqeusts"), unknown)["TYPOSQUAT"] == 45
    established = healthy("reqeusts", downloads_weekly=200_000)
    assert "TYPOSQUAT" not in codes(pinned("reqeusts"), established)


def test_age_and_staleness():
    brand_new = healthy(first_release=NOW - timedelta(days=5), latest_release=NOW - timedelta(days=5))
    assert codes(pinned(), brand_new)["NEW_PACKAGE"] == 25
    assert codes(pinned(), healthy(latest_release=NOW - timedelta(days=3 * 365)))["STALE"] == 10
    assert codes(pinned(), healthy(latest_release=NOW - timedelta(days=6 * 365)))["STALE"] == 20


def test_npm_takeover_signals():
    info = healthy(ecosystem="npm", new_publisher="mallory", install_scripts=["postinstall"], maintainers=1)
    result = codes(pinned(ecosystem="npm"), info)
    assert result == {"NEW_PUBLISHER": 20, "INSTALL_SCRIPT": 12, "SINGLE_MAINTAINER": 8}


def test_weak_heuristics_are_halved_for_very_popular_packages():
    info = healthy(ecosystem="npm", new_publisher="mallory", install_scripts=["install"], maintainers=1,
                   downloads_weekly=5_000_000, deprecated="old")
    result = codes(pinned(ecosystem="npm"), info)
    assert result == {"DEPRECATED": 25, "NEW_PUBLISHER": 10, "INSTALL_SCRIPT": 6, "SINGLE_MAINTAINER": 4}


def test_hygiene_signals():
    info = healthy(downloads_weekly=12, repo_url=None, release_count=1, scorecard=None,
                   version_yanked=True, version_exists=False)
    assert codes(pinned(), info) == {"VERSION_NOT_FOUND": 30, "YANKED": 25, "LOW_DOWNLOADS": 15,
                                     "NO_REPO": 8, "SINGLE_RELEASE": 5}


def test_low_scorecard_and_unmaintained_repo():
    info = healthy(scorecard=2.0, scorecard_maintained=0, latest_release=NOW - timedelta(days=500))
    assert codes(pinned(), info) == {"UNMAINTAINED_REPO": 10, "LOW_SCORECARD": 8}


def test_direct_url():
    dep = Dependency("fork", "npm", "github:acme/fork", direct_url="github:acme/fork")
    assert codes(dep, PackageInfo("fork", "npm")) == {"DIRECT_URL": 20}


def test_failed_lookup_adds_no_signals():
    info = PackageInfo("widget", "pypi", errors=["registry lookup failed: timeout"])
    assert scoring.evaluate(pinned(), info, NOW) == []


def test_score_is_capped_at_100():
    signals = scoring.evaluate(pinned("reqeusts"), healthy(
        "reqeusts", downloads_weekly=3, deprecated="x", version_yanked=True,
        vulnerabilities=[Vulnerability("GHSA-1", "x", "CRITICAL"), Vulnerability("GHSA-2", "x", "CRITICAL")]), NOW)
    assert sum(s.weight for s in signals) > 100 and scoring.score(signals) == 100
