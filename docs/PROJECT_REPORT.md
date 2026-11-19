# Project Report: depscan — A Dependency Risk Scanner for Software Supply Chain Security

**Semester VI Project**

| | |
|---|---|
| Student | *[Your name]* |
| Roll number | *[Your roll number]* |
| Guide | *[Guide's name]* |
| Industry partner | *[Partner organisation, if any]* |
| Repository | https://github.com/itsfingerlickinggood/chronos-os |

## 1. Abstract

Modern software is assembled from open-source packages: a typical web application
declares a few dozen dependencies and installs hundreds. Each one is code written
by a stranger that runs with the application's full privileges. Attackers exploit
this trust by publishing packages whose names imitate popular ones, by taking over
abandoned projects, and by registering public packages under a company's private
package names.

**depscan** is a command-line tool, GitHub Action and web dashboard that reads a
project's dependency manifest, gathers facts about every package from free public
data sources, and produces an explainable risk score from 0 to 100 for each one.
It runs with no paid services, no API keys and no third-party runtime libraries.

## 2. Problem statement

Developers add a dependency with a single command and rarely examine it. Existing
safeguards have gaps:

- **Vulnerability scanners** (`pip-audit`, `npm audit`) report only *known* CVEs. A brand-new malicious package has none.
- **Commercial platforms** (Snyk, Socket) cover more ground but are paid products for teams, and their scoring is not transparent.
- **Manual review** does not scale to hundreds of packages.

Students, open-source maintainers and small teams therefore have no simple, free
way to answer: *"Which of my dependencies deserve a closer look, and why?"*

### Real incidents that motivate the project

| Year | Incident | What happened | Signal in depscan |
|---|---|---|---|
| 2017 | `crossenv` (npm) | Typosquat of `cross-env` stole environment variables | `TYPOSQUAT`, `MALWARE` |
| 2018 | `event-stream` (npm) | A new maintainer was handed the project and added a backdoor | `NEW_PUBLISHER`, `STALE` |
| 2021 | Dependency confusion | A researcher got code running inside dozens of large companies by publishing public packages under their private package names | `NOT_FOUND` |
| 2021 | `ua-parser-js` (npm) | Maintainer account hijacked; malicious releases ran install scripts | `INSTALL_SCRIPT`, `NEW_PUBLISHER` |
| 2024 | `xz-utils` | A long-running social-engineering campaign against a single overworked maintainer | `SINGLE_MAINTAINER` |
| 2025 | `chalk`, `debug` and others (npm) | A phishing email gave attackers the maintainer's account; they published releases that stole cryptocurrency | `MALWARE` (once reported), `UNPINNED` |

## 3. Objectives

1. Parse Python and JavaScript dependency manifests reliably.
2. Collect package health, popularity and vulnerability data from free public APIs.
3. Detect typosquatting and dependency-confusion risks.
4. Combine the findings into a transparent, explainable score.
5. Fit into a developer's workflow: terminal, pull request review and a shareable dashboard.
6. Keep the tool itself free of third-party dependencies.

## 4. Scope

**In scope:** PyPI and npm ecosystems; `requirements.txt` (including `-r` includes),
`pyproject.toml` (PEP 621 and Poetry) and `package.json`; directly declared dependencies.

**Out of scope for this version:** transitive dependencies from lockfiles, other
ecosystems (Maven, Go, Cargo), and static analysis of package source code.

## 5. System design

### 5.1 Architecture

```text
┌───────────────┐   ┌────────────┐   ┌─────────────────────────┐   ┌────────────┐   ┌──────────────┐
│ Manifest files│──►│  Parsers   │──►│ Collectors (parallel,   │──►│  Scoring   │──►│  Reporters   │
│ requirements, │   │ parsers.py │   │ cached)                 │   │ scoring.py │   │  report.py   │
│ pyproject,    │   └────────────┘   │ registries.py, osv.py   │   │typosquat.py│   │ table, JSON, │
│ package.json  │                    └───────────┬─────────────┘   └────────────┘   │ Markdown,HTML│
└───────────────┘                                │                                  └──────────────┘
                              PyPI · npm · OSV.dev · pypistats · deps.dev
```

### 5.2 Modules

| Module | Responsibility |
|---|---|
| `parsers.py` | Turns manifest files into `Dependency` records; discovers manifests in a directory |
| `http.py` | JSON client on `urllib` with retries and a 24-hour disk cache |
| `registries.py` | PyPI and npm metadata, download counts, OpenSSF Scorecard |
| `osv.py` | Vulnerability lookup, advisory de-duplication, CVSS v3 base-score calculation |
| `typosquat.py` | Lookalike detection against the most-downloaded package names |
| `scoring.py` | Converts facts into weighted signals and a level |
| `scanner.py` | Orchestrates parallel lookups, de-duplication and baseline comparison |
| `report.py` | Terminal table, JSON, Markdown and standalone HTML dashboard |
| `cli.py` | Command-line interface and exit codes |
| `action.yml` | GitHub Action wrapper that comments on pull requests |

### 5.3 Data model

- **Dependency**: name, ecosystem, version specifier, resolved version, pinned?, source file, dev?, direct URL.
- **PackageInfo**: existence, versions, release dates, downloads, maintainers, install scripts, repository, Scorecard, vulnerabilities, errors.
- **Signal**: code, weight, title, detail.
- **Result**: dependency + info + signals + score + level + new-in-this-change flag.

### 5.4 Key algorithms

**Typosquat detection.** A name is compared with the most-downloaded packages of
its ecosystem using three tests: (1) equality after removing separators
(`pythondateutil` vs `python-dateutil`), (2) equality after reversing common
character substitutions (`djang0` vs `django`), and (3) an *optimal string
alignment* edit distance of exactly 1, which counts an insertion, deletion,
substitution or swap of adjacent letters as one edit (`reqeusts` vs `requests`).
To limit false positives, targets shorter than five characters are ignored, names
that are themselves popular are never flagged, and a lookalike with more than
50,000 weekly downloads is treated as an established project.

**Vulnerability de-duplication.** OSV aggregates several databases, so one flaw
often appears as a GHSA, a PYSEC and a CVE record. Records that share an id or
alias are merged, keeping the highest severity. When a record has no severity
label, the CVSS v3 base score is computed from its vector string.

**Fix recommendation.** For each advisory the tool selects the fix version of the
release line the project is on (the range whose `introduced` ≤ current < `fixed`).

**Scoring.** `score = min(100, Σ signal weights)`. Weak heuristics are halved for
packages above one million weekly downloads. Levels: low < 25 ≤ medium < 50 ≤ high < 75 ≤ critical.
The full weight table is in the README.

## 6. Implementation

| | |
|---|---|
| Language | Python 3.11+ (standard library only at runtime) |
| Concurrency | `concurrent.futures.ThreadPoolExecutor` |
| Dashboard | One self-contained HTML file; vanilla JavaScript; light and dark themes; no external assets |
| Automation | GitHub Actions (composite action, CI matrix, scheduled dashboard build) |
| Hosting | GitHub Pages |
| Testing | pytest |

Security considerations in the tool itself:

- Package names and advisory text are untrusted input. The dashboard builds its DOM with `textContent` only and escapes the embedded JSON, so a hostile package description cannot inject script.
- Action inputs are passed to the shell through environment variables, not interpolated into the script.
- The tool only reads metadata. It never downloads or executes package code.

## 7. Testing

77 automated tests run offline in under a second against an in-memory fake of the network.

| Area | Examples of what is verified |
|---|---|
| Parsers | Extras, environment markers, hashes, `-r` include cycles, npm aliases, git URLs, Poetry tables |
| Typosquat | Known attacks are caught; no popular package is flagged as a typo of another |
| OSV | CVSS scores match published values; duplicates merge; malware is recognised; withdrawn advisories are ignored |
| Registries | Missing packages and versions, yanked releases, new-publisher detection, unpublished npm stubs |
| Scoring | Each signal's weight, popularity dampening, the 100-point cap, level thresholds |
| CLI | Report files, `--fail-on` exit codes, baseline comparison, bad input, HTML injection attempt |

Continuous integration runs the suite on Python 3.11, 3.12 and 3.13.

## 8. Results

Scanning the two demo projects in `examples/` (19 dependencies) takes about
2.5 seconds on a cold cache and finds 5 critical, 8 high, 1 medium and 5 low-risk packages, including:

| Package | Score | Finding |
|---|---:|---|
| `crossenv` | 100 | Malicious-package advisory; typosquat of `cross-env` |
| `event-stream` 3.3.6 | 100 | The backdoored 2018 release, flagged from its malicious-package advisory |
| `reqeusts` | 100 | Not on PyPI and one transposition away from `requests` |
| `axios` 0.21.0 | 89 | 25 known vulnerabilities; release deprecated by its maintainers |
| `pycrypto` 2.6.1 | 74 | Critical vulnerabilities with no fix; no release in over twelve years |
| `acme-internal-billing` | 70 | Private-looking name absent from PyPI (dependency confusion) |
| `lodash` 4.17.15 | 64 | 5 known vulnerabilities, fixed in later releases |
| `numpy` 1.26.4 | 0 | Healthy and pinned |

*(Counts reflect the public databases on the day of the scan and will change.)*

## 9. Limitations

- Only directly declared dependencies are scanned.
- npm version ranges are evaluated at their lowest allowed version.
- PyPI download counts depend on a rate-limited third-party service and may be unknown.
- Heuristic signals can flag legitimate packages; the tool reports reasons so a human can decide.

## 10. Future scope

1. Lockfile support (`package-lock.json`, `poetry.lock`, `uv.lock`) for transitive dependencies.
2. More ecosystems: Maven, Go modules, Cargo.
3. Detect maintainer changes on PyPI through release attestations.
4. Static inspection of install scripts for network and shell access.
5. Trend tracking: store each scan and chart risk over time.
6. Policy files so a team can accept a finding with a recorded reason.

## 11. Conclusion

depscan shows that meaningful supply-chain risk assessment is possible with free
public data and a small, dependency-free codebase. It catches the main attack
patterns seen in real incidents (typosquatting, dependency confusion, abandoned
and vulnerable packages) and explains every finding, which makes it usable by
developers who are not security specialists.

## 12. References

1. OSV — Open Source Vulnerabilities database. https://osv.dev
2. OpenSSF Scorecard. https://securityscorecards.dev
3. Open Source Insights (deps.dev). https://deps.dev
4. PyPI JSON API. https://docs.pypi.org/api/json/
5. npm registry API. https://github.com/npm/registry
6. A. Birsan, "Dependency Confusion: How I Hacked Into Apple, Microsoft and Dozens of Other Companies", 2021.
7. CVE-2024-3094, the xz-utils backdoor.
8. FIRST, Common Vulnerability Scoring System v3.1 Specification. https://www.first.org/cvss/v3.1/specification-document
9. OpenSSF malicious-packages repository. https://github.com/ossf/malicious-packages
