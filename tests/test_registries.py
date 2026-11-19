from depscan import osv, registries
from depscan.models import Dependency
from tests.conftest import FakeFetcher, npm_doc, pypi_doc

PYPI = "https://pypi.org/pypi/{}/json"
NPM = "https://registry.npmjs.org/{}"
NPM_DL = "https://api.npmjs.org/downloads/point/last-week/{}"
DEPS_DEV = "https://api.deps.dev/v3/projects/github.com%2Facme%2Fwidget"


def test_pypi_metadata_and_scorecard():
    fetcher = FakeFetcher({
        PYPI.format("widget"): pypi_doc(),
        "https://pypistats.org/api/packages/widget/recent": {"data": {"last_week": 420}},
        DEPS_DEV: {"starsCount": 12, "scorecard": {"overallScore": 3.1,
                                                   "checks": [{"name": "Maintained", "score": 0}]}},
        (osv.OSV_URL, "widget"): {"vulns": [{"id": "GHSA-x", "database_specific": {"severity": "HIGH"}}]},
    })
    info = registries.collect(Dependency("widget", "pypi", "==1.0", "1.0", True), fetcher)

    assert info.exists and info.version_exists
    assert info.evaluated_version == "1.0.0"  # "1.0" matched the published "1.0.0"
    assert info.latest_version == "2.0.0" and info.release_count == 2
    assert info.first_release.year == 2020 and info.latest_release.year == 2026
    assert info.downloads_weekly == 420
    assert info.repo_url == "github.com/acme/widget"
    assert (info.scorecard, info.scorecard_maintained, info.stars) == (3.1, 0, 12)
    assert [v.id for v in info.vulnerabilities] == ["GHSA-x"]
    assert fetcher.posts[0]["version"] == "1.0.0"


def test_pypi_missing_package_and_missing_version():
    fetcher = FakeFetcher({PYPI.format("widget"): pypi_doc()})
    assert registries.collect(Dependency("ghost", "pypi"), fetcher).exists is False
    info = registries.collect(Dependency("widget", "pypi", "==9.9", "9.9", True), fetcher)
    assert info.version_exists is False
    assert info.downloads_weekly is None  # pypistats unavailable: unknown, not zero


def test_pypi_yanked_version():
    doc = pypi_doc()
    doc["releases"]["1.0.0"][0]["yanked"] = True
    info = registries.collect(Dependency("widget", "pypi", "==1.0.0", "1.0.0", True),
                              FakeFetcher({PYPI.format("widget"): doc}))
    assert info.version_yanked


def test_popular_pypi_package_uses_bundled_download_count():
    info = registries.collect(Dependency("requests", "pypi"), FakeFetcher({PYPI.format("requests"): pypi_doc()}))
    assert info.downloads_weekly > 1_000_000


def test_npm_metadata():
    versions = {f"1.0.{i}": f"202{i}-01-01T00:00:00Z" for i in range(6)}
    fetcher = FakeFetcher({
        NPM.format("@acme%2Fwidget"): npm_doc(versions, maintainers=("alice",),
                                              publishers={"1.0.5": "mallory"},
                                              scripts={"postinstall": "node setup.js", "test": "jest"},
                                              deprecated="use something else"),
        NPM_DL.format("@acme%2Fwidget"): {"downloads": 77},
    })
    info = registries.collect(Dependency("@acme/widget", "npm", "^1.0.0"), fetcher)

    assert info.latest_version == "1.0.5" and info.release_count == 6
    assert info.maintainers == 1 and info.downloads_weekly == 77
    assert info.new_publisher == "mallory"
    assert info.install_scripts == ["postinstall"]
    assert info.deprecated == "use something else"
    assert info.repo_url == "github.com/acme/widget"


def test_npm_same_publisher_is_not_flagged():
    versions = {f"1.0.{i}": f"202{i}-01-01T00:00:00Z" for i in range(6)}
    fetcher = FakeFetcher({NPM.format("widget"): npm_doc(versions)})
    assert registries.collect(Dependency("widget", "npm"), fetcher).new_publisher is None


def test_npm_unpublished_package_counts_as_missing():
    stub = {"name": "gone", "time": {"unpublished": {"time": "2024-01-01T00:00:00Z"}}}
    assert registries.collect(Dependency("gone", "npm"), FakeFetcher({NPM.format("gone"): stub})).exists is False

    # A single removed version (for example one pulled for being malicious) is still looked up.
    versions = {"1.0.0": "2021-01-01T00:00:00Z", "1.0.2": "2025-09-10T00:00:00Z"}
    fetcher = FakeFetcher({
        NPM.format("widget"): npm_doc(versions),
        (osv.OSV_URL, "widget"): {"vulns": [{"id": "MAL-2025-1", "summary": "Malicious code in widget (npm)"}]},
    })
    info = registries.collect(Dependency("widget", "npm", "1.0.1", "1.0.1", True), fetcher)

    assert info.version_exists is False
    assert info.evaluated_version == "1.0.1"          # not silently replaced by the latest version
    assert fetcher.posts[0]["version"] == "1.0.1"     # the removed release is the one looked up
    assert [v.severity for v in info.vulnerabilities] == ["MALWARE"]


def test_direct_url_dependency_makes_no_requests():
    class Exploding:
        def get(self, *a, **k):
            raise AssertionError("no network expected")
        post = get

    dep = Dependency("fork", "npm", "github:acme/fork", direct_url="github:acme/fork")
    assert registries.collect(dep, Exploding()).vulnerabilities == []


def test_find_repo():
    assert registries.find_repo("git+ssh://git@github.com/acme/widget.git") == "github.com/acme/widget"
    assert registries.find_repo({"url": "https://gitlab.com/acme/widget/-/issues"}) == "gitlab.com/acme/widget"
    assert registries.find_repo("https://github.com/sponsors/acme", "https://example.com") is None

