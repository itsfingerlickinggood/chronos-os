from datetime import datetime, timezone

import pytest

from depscan.http import NotFound

NOW = datetime(2026, 11, 1, tzinfo=timezone.utc)


class FakeFetcher:
    """In-memory stand-in for the network. Unknown URLs behave like HTTP 404."""

    def __init__(self, routes=None):
        self.routes = dict(routes or {})
        self.posts = []

    def get(self, url, retries=2):
        if url not in self.routes:
            raise NotFound(url)
        return self.routes[url]

    def post(self, url, body, retries=2):
        self.posts.append(body)
        key = (url, body["package"]["name"])
        return self.routes.get(key, {})


def pypi_doc(version="2.0.0", releases=None, urls=None):
    releases = releases or {"1.0.0": "2020-01-01T00:00:00Z", version: "2026-06-01T00:00:00Z"}
    return {
        "info": {"version": version, "project_urls": urls if urls is not None else
                 {"Source": "https://github.com/acme/widget"}, "classifiers": []},
        "releases": {v: [{"upload_time_iso_8601": t, "yanked": False}] for v, t in releases.items()},
    }


def npm_doc(versions, latest=None, maintainers=("alice", "bob"), publishers=None, scripts=None,
            deprecated=None):
    latest = latest or list(versions)[-1]
    publishers = publishers or {}
    return {
        "dist-tags": {"latest": latest},
        "maintainers": [{"name": m} for m in maintainers],
        "time": dict(versions),
        "versions": {
            v: {
                "_npmUser": {"name": publishers.get(v, "alice")},
                "scripts": (scripts or {}) if v == latest else {},
                "repository": {"url": "git+https://github.com/acme/widget.git"},
                **({"deprecated": deprecated} if deprecated and v == latest else {}),
            }
            for v in versions
        },
    }


@pytest.fixture
def now():
    return NOW
