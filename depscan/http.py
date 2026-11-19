"""Tiny JSON-over-HTTPS client with an on-disk cache. Standard library only."""
from __future__ import annotations

import hashlib
import json
import os
import time
import urllib.error
import urllib.request
from pathlib import Path

USER_AGENT = "depscan/1.0"
DAY = 24 * 3600


class NotFound(Exception):
    """The resource does not exist (HTTP 404)."""


class FetchError(Exception):
    """The data source could not be reached or returned an error."""


def default_cache_dir() -> Path:
    if os.environ.get("DEPSCAN_CACHE_DIR"):
        return Path(os.environ["DEPSCAN_CACHE_DIR"])
    base = os.environ.get("XDG_CACHE_HOME") or os.path.join(os.path.expanduser("~"), ".cache")
    return Path(base) / "depscan"


class Fetcher:
    """Fetches JSON documents. Tests replace this with an in-memory fake."""

    def __init__(self, cache_dir: Path | None = None, ttl: int = DAY, timeout: int = 20) -> None:
        self.cache_dir = cache_dir
        self.ttl = ttl
        self.timeout = timeout

    def get(self, url: str, retries: int = 2):
        return self._request(url, None, retries)

    def post(self, url: str, body: dict, retries: int = 2):
        return self._request(url, json.dumps(body, sort_keys=True).encode(), retries)

    # -- internals -------------------------------------------------------
    def _cache_path(self, url: str, data: bytes | None) -> Path | None:
        if self.cache_dir is None:
            return None
        digest = hashlib.sha256(url.encode() + b"\0" + (data or b"")).hexdigest()
        return self.cache_dir / digest[:2] / f"{digest}.json"

    def _request(self, url: str, data: bytes | None, retries: int):
        path = self._cache_path(url, data)
        if path is not None and path.exists() and time.time() - path.stat().st_mtime < self.ttl:
            try:
                cached = json.loads(path.read_text())
            except (OSError, ValueError):
                cached = None
            if cached is not None:
                if cached.get("not_found"):
                    raise NotFound(url)
                return cached["body"]

        headers = {"User-Agent": USER_AGENT, "Accept": "application/json"}
        if data is not None:
            headers["Content-Type"] = "application/json"
        last_error: Exception | None = None
        for attempt in range(retries + 1):
            try:
                req = urllib.request.Request(url, data=data, headers=headers)
                with urllib.request.urlopen(req, timeout=self.timeout) as resp:
                    body = json.loads(resp.read().decode("utf-8"))
                self._store(path, {"body": body})
                return body
            except urllib.error.HTTPError as exc:
                if exc.code == 404:
                    self._store(path, {"not_found": True})
                    raise NotFound(url) from None
                last_error = FetchError(f"HTTP {exc.code} from {url}")
                if exc.code not in (429, 500, 502, 503, 504):
                    break
            except (urllib.error.URLError, TimeoutError, OSError, ValueError) as exc:
                last_error = FetchError(f"{type(exc).__name__} fetching {url}: {exc}")
            if attempt < retries:
                time.sleep(1.5 * (attempt + 1))
        raise last_error or FetchError(url)

    @staticmethod
    def _store(path: Path | None, payload: dict) -> None:
        if path is None:
            return
        try:
            path.parent.mkdir(parents=True, exist_ok=True)
            tmp = path.with_suffix(f".{os.getpid()}.tmp")
            tmp.write_text(json.dumps(payload))
            tmp.replace(path)
        except OSError:
            pass  # caching is best-effort
