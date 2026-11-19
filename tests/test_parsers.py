import json

import pytest

from depscan import parsers


def by_name(deps):
    return {d.name: d for d in deps}


def test_requirements_txt(tmp_path):
    (tmp_path / "base.txt").write_text("urllib3==2.2.1\n")
    req = tmp_path / "requirements.txt"
    req.write_text(
        "# comment\n"
        "Django==4.2.1  # inline comment\n"
        "requests[security]>=2.0,<3 ; python_version > '3.8'\n"
        "Flask_Login\n"
        "cryptography==42.0.0 \\\n    --hash=sha256:abc\n"
        "-r base.txt\n"
        "--index-url https://example.invalid/simple\n"
        "git+https://github.com/acme/tool.git@v1#egg=acme-tool\n"
        "widget @ https://example.invalid/widget.whl\n"
    )
    deps = by_name(parsers.parse_file(req))

    assert deps["django"].pinned and deps["django"].version == "4.2.1"
    assert not deps["requests"].pinned and deps["requests"].spec == ">=2.0,<3"
    assert deps["flask-login"].spec == ""  # PEP 503 normalised name
    assert deps["cryptography"].version == "42.0.0"
    assert deps["urllib3"].version == "2.2.1"  # pulled in through -r
    assert deps["acme-tool"].direct_url.startswith("git+https://")
    assert deps["widget"].direct_url == "https://example.invalid/widget.whl"
    assert len(deps) == 7


def test_requirements_include_cycle(tmp_path):
    (tmp_path / "a.txt").write_text("-r b.txt\nfoo==1\n")
    (tmp_path / "b.txt").write_text("-r a.txt\nbar==2\n")
    assert sorted(d.name for d in parsers.parse_requirements(tmp_path / "a.txt")) == ["bar", "foo"]


def test_wildcard_pin_is_not_pinned():
    dep = parsers.parse_requirement_line("django==4.2.*")
    assert not dep.pinned and dep.version is None


def test_package_json(tmp_path):
    path = tmp_path / "package.json"
    path.write_text(json.dumps({
        "dependencies": {"express": "^4.17.1", "lodash": "4.17.15", "any": "*",
                         "fork": "github:acme/fork", "short": "acme/short#main",
                         "alias": "npm:real-pkg@^2.0.0"},
        "devDependencies": {"@types/node": "~20.1.0"},
    }))
    deps = by_name(parsers.parse_file(path))

    assert deps["lodash"].pinned and deps["lodash"].version == "4.17.15"
    assert not deps["express"].pinned and deps["express"].version == "4.17.1"
    assert deps["any"].version is None
    assert deps["fork"].direct_url and deps["short"].direct_url
    assert deps["real-pkg"].version == "2.0.0"  # alias resolved to the real package
    assert deps["@types/node"].dev


def test_pyproject(tmp_path):
    path = tmp_path / "pyproject.toml"
    path.write_text(
        '[project]\ndependencies = ["httpx==0.27.0", "rich>=13"]\n'
        '[project.optional-dependencies]\ndev = ["pytest"]\n'
        '[tool.poetry.dependencies]\npython = "^3.11"\nclick = "8.1.7"\npendulum = "^3.0"\n'
        'mylib = { git = "https://github.com/acme/mylib.git" }\n'
    )
    deps = by_name(parsers.parse_file(path))

    assert deps["httpx"].pinned and not deps["rich"].pinned
    assert deps["pytest"].dev
    assert deps["click"].pinned and deps["click"].version == "8.1.7"
    assert not deps["pendulum"].pinned
    assert deps["mylib"].direct_url
    assert "python" not in deps


def test_discover_skips_vendor_dirs(tmp_path):
    (tmp_path / "requirements-dev.txt").write_text("pytest\n")
    (tmp_path / "notes.txt").write_text("hello\n")
    (tmp_path / "web").mkdir()
    (tmp_path / "web" / "package.json").write_text("{}")
    (tmp_path / "web" / "node_modules" / "x").mkdir(parents=True)
    (tmp_path / "web" / "node_modules" / "x" / "package.json").write_text("{}")
    found = [p.relative_to(tmp_path).as_posix() for p in parsers.discover(tmp_path)]
    assert found == ["requirements-dev.txt", "web/package.json"]


def test_invalid_json_raises_parse_error(tmp_path):
    path = tmp_path / "package.json"
    path.write_text("{not json")
    with pytest.raises(parsers.ParseError):
        parsers.parse_file(path)
