import pytest

from depscan.typosquat import find_lookalike, osa_distance, popular


@pytest.mark.parametrize("a,b,expected", [
    ("requests", "requests", 0),
    ("reqeusts", "requests", 1),   # swapped neighbours
    ("request", "requests", 1),    # missing letter
    ("requestss", "requests", 1),  # extra letter
    ("requezts", "requests", 1),   # wrong letter
    ("flask", "flasks2", 2),    # two extra letters
])
def test_osa_distance(a, b, expected):
    assert osa_distance(a, b, 2) == expected


@pytest.mark.parametrize("name,ecosystem,target", [
    ("reqeusts", "pypi", "requests"),
    ("nunpy", "pypi", "numpy"),
    ("python-dateutils", "pypi", "python-dateutil"),
    ("pythondateutil", "pypi", "python-dateutil"),  # separator dropped
    ("djang0", "pypi", "django"),                   # digit for letter
    ("loadsh", "npm", "lodash"),
    ("crossenv", "npm", "cross-env"),
    ("expres", "npm", "express"),
])
def test_typos_are_caught(name, ecosystem, target):
    assert find_lookalike(name, ecosystem) == (target, "typo")


@pytest.mark.parametrize("name,ecosystem", [
    ("requests", "pypi"), ("numpy", "pypi"), ("Python_Dateutil", "pypi"),
    ("flask-cors", "pypi"), ("requests-oauthlib", "pypi"),
    ("react", "npm"), ("lodash", "npm"), ("@types/node", "npm"), ("@acme/lodahs", "npm"),
])
def test_legitimate_names_are_not_flagged(name, ecosystem):
    assert find_lookalike(name, ecosystem) is None


def test_combosquat():
    assert find_lookalike("python-requests", "pypi") == ("requests", "combo")
    assert find_lookalike("express-js", "npm") == ("express", "combo")


def test_no_popular_package_flags_another():
    for ecosystem in ("pypi", "npm"):
        assert [n for n in popular(ecosystem) if find_lookalike(n, ecosystem)] == []
