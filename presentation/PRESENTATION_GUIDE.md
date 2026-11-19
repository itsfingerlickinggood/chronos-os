# Presentation Guide for depscan

Everything you need to present this project, in simple language.

- **Slides:** `depscan-presentation.pptx` (13 slides, about 12 to 15 minutes). A PDF copy is included as a backup.
- **Before you present:** put your name, roll number and guide's name on slide 1.
- Every slide already has speaker notes inside PowerPoint (View → Notes). The same script is below.

---

## 1. The project in 30 seconds

> "Every app today is built using hundreds of ready-made code packages written by strangers.
> Some of these packages are fake, abandoned or have security bugs. My tool, depscan, reads the
> list of packages a project uses, checks each one using free public data, and gives it a risk
> score from 0 to 100 with a clear reason. It works from the command line, on GitHub pull
> requests, and as a web dashboard. It costs nothing to run."

Learn this by heart. If someone asks "what is your project?", this is the answer.

---

## 2. Simple meanings of the words you will use

| Word | Simple meaning |
|---|---|
| **Package / dependency** | Ready-made code written by someone else that your project uses |
| **Registry** | The online store where packages are kept. PyPI is for Python, npm is for JavaScript |
| **Supply chain attack** | Attacking an app by poisoning one of the packages it uses, instead of attacking the app directly |
| **Typosquatting** | Uploading a package with a name that is a spelling mistake of a famous one (`reqeusts` for `requests`) |
| **Dependency confusion** | A company uses a private package name; an attacker registers the same name publicly, and the installer picks the attacker's one |
| **Vulnerability** | A known security bug in a particular version of a package |
| **CVE / advisory** | An official public record of a vulnerability |
| **OSV.dev** | A free database from Google that lists known vulnerabilities in open-source packages |
| **Pinned version** | Writing the exact version (`requests==2.31.0`) so a different version cannot install by itself |
| **Install script** | Code that a package runs automatically the moment you install it |
| **GitHub Action** | A small program GitHub runs for you automatically, for example on every pull request |
| **Pull request (PR)** | A request to add your code changes to a project, which others review first |
| **Heuristic** | A rule of thumb. It is usually right, but not always |

---

## 3. What to say on each slide

**Slide 1 — Title** (30 sec)
Good morning everyone. My project is called depscan. It is a tool that checks the software
packages a project uses and tells you which ones are risky, and why. I will explain the problem
first, then show how my tool solves it.

**Slide 2 — Today's software is built from other people's code** (1 min)
Today, no one writes all the code for an app. We install packages, which are ready-made code
written by other people. One command adds a package, and a normal app ends up with hundreds of
them. The problem is that we trust all this code without checking it. If even one package is
bad, our whole app is in danger.

**Slide 3 — Four ways a bad package gets into a project** (1.5 min)
There are four common ways. One: fake names. The attacker uploads a package whose name is a
spelling mistake of a famous one. Two: a company uses a private package name and an attacker
registers that same name publicly. This is called dependency confusion. Three: old versions with
known security bugs. Four: packages that nobody looks after any more.

**Slide 4 — This has already happened in the real world** (1 min)
These are real cases. In 2017 a fake package called crossenv copied the real cross-env and stole
passwords. In 2018 a new maintainer took over event-stream and added harmful code. In 2021 a
researcher used private package names to get his code inside dozens of large companies. In 2024
the xz backdoor almost reached millions of Linux servers.

**Slide 5 — depscan gives every package a risk score** (1.5 min)
My solution is depscan. It reads your package list, collects public information about each
package, and gives a score from 0 to 100. Most importantly, it explains the score. Here
`reqeusts` gets 100 because it does not exist publicly and its name looks like a typing mistake
of `requests`.

**Slide 6 — How it works in four steps** (1 min)
Step one, read the files that list the packages. Step two, look up each package in public
sources. Step three, add points for every warning sign. Step four, show the report. It checks
many packages at the same time, so 19 packages take about 2.5 seconds.

**Slide 7 — What depscan looks for** (1.5 min)
These are the main warning signs and their points. The strongest is a package already reported
as malware. Then a package that does not exist publicly, known security bugs, and lookalike
names. Each sign has fixed points, so the score can always be explained.

**Slide 8 — Reading the score** (1 min)
We add the points of all warning signs, with a maximum of 100. 0 to 24 is low, 25 to 49 medium,
50 to 74 high, 75 and above critical. A high score means "look closer". It is a warning, not a
final judgement.

**Slide 9 — Three ways to use it** (1 min, then the demo)
First, the command line. Second, a GitHub Action that checks every pull request and comments
when a risky package is added. Third, a web dashboard, shown on the right. *Now switch to the
live demo.*

**Slide 10 — Results on the demo projects** (1 min)
I made two demo projects with risky packages on purpose. depscan scanned 19 packages in about
2.5 seconds: 4 critical, 9 high, 1 medium and 5 low. It caught the fake names, the vulnerable
versions and the missing private package. The healthy package numpy scored zero. All 77
automatic tests pass.

**Slide 11 — Built with free tools only** (45 sec)
It is written in Python and uses only what comes with Python. The data comes from free public
sources. GitHub Actions runs it and GitHub Pages hosts the dashboard. Total cost is zero rupees.

**Slide 12 — Limitations and future work** (1 min)
It checks only the packages listed directly, not the hidden ones they bring along. It supports
Python and JavaScript only. The score is a warning, so a person still decides. In future I can
add lock-file support, more languages and trend tracking.

**Slide 13 — Conclusion** (30 sec)
Risky packages are a real problem. depscan finds them in seconds and explains why. It is free
and fits into normal developer work. Thank you. I am happy to take questions.

---

## 4. Live demo (3 minutes)

### Set up before the presentation

You need Python 3.11 or newer and an internet connection.

```bash
git clone https://github.com/itsfingerlickinggood/chronos-os.git
cd chronos-os
pip install .
depscan examples --html report.html
```

Run the last command **once before you present**. It saves the answers on your laptop for 24
hours, so the demo is fast even on slow college Wi-Fi. Keep `report.html` open in a browser tab
as a backup in case the internet fails completely.

### Demo steps

| Step | Type this | Say this |
|---|---|---|
| 1 | `cat examples/python-app/requirements.txt` | "This is a normal package list. It looks harmless." |
| 2 | `depscan examples` | "depscan checks all 19 packages. Red ones at the top are the most risky." |
| 3 | *(scroll up to `reqeusts`)* | "This one scored 100. The name does not exist publicly and it is a spelling mistake of `requests`." |
| 4 | `depscan check lodash@4.17.15 -e npm` | "I can also check one package. This old lodash version has known security bugs, and the tool tells me which version fixes them." |
| 5 | `depscan check numpy` | "A healthy package gets a low score, so the tool does not cry wolf." |
| 6 | `depscan examples --html report.html` then open `report.html` | "This is the dashboard. I can filter by risk level and click a package to see every reason." |
| 7 | `depscan examples --fail-on high; echo $?` | "It prints 1, meaning 'fail'. This is how it blocks a risky pull request automatically." (On Windows PowerShell use `echo $LASTEXITCODE`.) |

If something breaks during the demo, stay calm, open the saved `report.html`, and continue.

---

## 5. Questions the examiner may ask

**Q1. What is the main idea of your project?**
It scores how risky each third-party package in a project is, using free public data, and
explains the reasons.

**Q2. Why is this needed? Tools like `npm audit` and `pip-audit` already exist.**
Those tools only report *known* vulnerabilities. A brand-new fake package has none, so they miss
it. depscan also looks at warning signs such as lookalike names, missing packages, new owners and
abandoned projects.

**Q3. How do you detect a lookalike (typosquatted) name?**
I compare the name with a list of the most-downloaded packages. If it is exactly one edit away
(one letter added, removed, changed, or two neighbouring letters swapped), it is flagged. This
is measured with an "edit distance" algorithm. I also catch removed dashes and swapped characters
like `0` for `o`.

**Q4. Will it wrongly flag good packages?**
Sometimes, because these are rules of thumb. To reduce this, popular packages are never flagged
as lookalikes, very short names are ignored, and weak signs count for half on packages with more
than a million weekly downloads. The tool always shows its reasons so a person can decide.

**Q5. How is the score calculated?**
Each warning sign has fixed points. I add them and cap the total at 100. 0–24 low, 25–49 medium,
50–74 high, 75–100 critical.

**Q6. How did you choose the points?**
By how strongly each sign indicates real danger. Confirmed malware gets 100. A missing package
gets 70 because it is directly exploitable. Small hygiene issues like an unpinned version get 3.
The weights are my design decision, tuned on the demo projects; they are not from a published
standard. Calibrating them on a large dataset is future work.

**Q7. Where does the data come from?**
The PyPI and npm registries for package details, OSV.dev for vulnerabilities and malware reports,
the npm and pypistats services for download counts, and deps.dev for the OpenSSF Scorecard of
the source repository. All are free and need no key.

**Q8. What is dependency confusion?**
A company uses a private package, say `acme-billing`. That name is not on the public registry.
An attacker registers `acme-billing` publicly with a higher version number, and the company's
installer downloads the attacker's package. depscan flags any package name that is missing from
the public registry.

**Q9. Why does your tool have zero dependencies?**
A security tool that installs many packages would itself be a risk. So I used only Python's
standard library.

**Q10. Is it safe to run? Does it download the packages?**
No. It only reads information *about* packages. It never downloads or runs their code.

**Q11. What happens if the internet or one data source is down?**
The scan still finishes. That package is reported with whatever was learned and is marked as
incomplete. Results are also saved for 24 hours.

**Q12. How did you test it?**
I wrote 77 automatic tests using pytest. They use a fake network, so they run offline in under
a second. GitHub runs them on Python 3.11, 3.12 and 3.13 on every push.

**Q13. How does the GitHub Action work?**
On a pull request it compares the package list before and after the change, scans only the new
or changed packages, posts a comment with the result, and can fail the check if the risk is too
high.

**Q14. What are the limitations?**
It checks only directly listed packages, supports only Python and JavaScript, and its scores are
warnings, not proof.

**Q15. What would you add next?**
Lock-file support to check hidden (transitive) packages, more languages such as Java and Go, and
a history of scans to show how risk changes over time.

**Q16. Which part was the hardest?**
*(Answer honestly in your own words. Good options: reducing false alarms in lookalike detection,
or merging duplicate vulnerability records that come from different databases.)*

**Q17. Is this an AI project?**
No. It uses clear rules and public data, not machine learning. That is a deliberate choice: every
result can be explained, and it is free to run.

---

## 6. Tips for the day

- Speak slowly. One idea per slide.
- Do not read the slides. Look at the audience and use the slides as reminders.
- If you do not know an answer, say: "I have not explored that yet, but I think it could be done by…"
- Know your numbers: **19** packages, **2.5** seconds, **77** tests, **4** data sources, **₹0** cost.
- Read `docs/PROJECT_REPORT.md` once fully before the viva.
