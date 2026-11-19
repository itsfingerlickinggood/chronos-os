# Video transcripts

The words spoken in each video, scene by scene. Captions in the videos show the same text.

## Video 1: What is depscan? (about 3 minutes)

What is depscan? A simple explanation. In the next two minutes, you will understand what this project does, even if you have never written code.

Think of an app like a house made of Lego bricks. The developer does not make every brick. Most bricks are ready-made, and come from strangers on the internet. These ready-made bricks are called packages. A normal app uses hundreds of them.

Here is the problem. Nobody checks these bricks. If just one brick is bad, the whole house is in danger. A bad package can steal passwords, or break the app.

Bad packages get in using four common tricks. Trick one: a fake name. The real package is called "requests". The fake one swaps two letters. It is easy to miss. Trick two: a name that nobody owns. An attacker can grab it and put bad code there. Trick three: an old version with known security holes. Trick four: an abandoned package that nobody fixes any more.

This is not just theory. It has really happened. In 2017, a fake package stole passwords from developers. In 2018, a popular package got a new owner, who secretly added harmful code. In 2021, a researcher used the nobody-owns-this-name trick to get his code inside dozens of big companies. And in 2024, a hidden backdoor almost reached millions of servers.

So what does depscan do? Think of it as a security guard at the gate. Every package must pass through the guard. The guard checks each one, and gives it a danger score from 0 to 100. Safe packages get a low score. Dangerous ones get a high score.

The best part: it always tells you why. For example, this package does not exist in the public store. That adds 70 points. Its name looks like a spelling mistake of "requests". That adds 45 more. The score stops at 100. That means critical. Do not use it.

Reading the score is like reading a traffic light. Green means it looks fine. Yellow means take a look. Orange means check before using. And red means stop.

Three things make this project special. One: it is completely free. No payment, and no account. Two: it explains every result in plain words. Three: it works in seconds.

That is depscan. A security guard for the code you borrow from strangers. Watch the next video to see it working.

## Video 2: How depscan works (about 2 minutes 40 seconds)

How depscan works, step by step. Let us watch it check a real project.

Every project has a shopping list of the packages it needs. It is a simple text file. Each line is one package, and its version number. This list looks harmless. But is it?

To check the list, you type one short command. depscan reads the list, and checks every package. A few seconds later, you get a table. The most dangerous packages are at the top. Below the table, it explains the reasons for each risky package.

What happens inside? Four simple steps. Step one: read. It reads the package names from your list. Step two: ask. It asks trusted public websites about each package. How old is it? How many people use it? Does it have known security holes? Step three: score. It adds points for every warning sign it finds. Step four: report. It shows you the result.

You can also see the results as a web page. At the top, you see how many packages are critical, high, medium and low. Click any package to see every reason behind its score. You can also filter the list, to see only the most dangerous ones.

The best way to use it is automatically. When a teammate tries to add a new package to the project, depscan checks it straight away. If the package is risky, it leaves a warning comment, and blocks the change. So bad packages are stopped before they get in.

Here is how the project is organised. Each part has one job. The reader understands your package list. The collectors fetch facts from the internet. The name checker spots fake lookalike names. The scorer adds up the points. The reporter shows the result as a table, a web page, or a comment. And 77 automatic tests make sure it all keeps working.

Everything it uses is free. A free language, free public data, and free hosting. The total cost is zero.

Let us recap. depscan reads your package list, asks public sources, adds up the warning signs, and shows a clear score. A simple tool, for a real problem. Thank you for watching.

## Video 3: The complete explanation (about 9 minutes 20 seconds)

depscan: the complete explanation. This video answers every question a teacher might ask about this project. You do not need to know any coding. Everything is explained with simple examples.

First question. What is your project? depscan is a safety checker for software. It looks at the ready-made parts an app is built from, and warns you if any part is dangerous. In one line: it is a security guard for borrowed code.

Second question. What problem does it solve? Nobody builds an app from nothing. Developers borrow ready-made parts, called packages, from the internet. It is like building a house with bricks from hundreds of strangers. A normal app has hundreds of these bricks, and almost nobody checks them. If one brick is bad, the whole house is in danger.

Third question. Why is this necessary? How do bad packages get in? There are four common tricks. One: a fake name that looks like a famous one. Two: a name that nobody owns, which an attacker grabs. Three: an old version with known security holes. Four: an abandoned package that nobody repairs.

Fourth question. Has this really happened? Yes. In September 2025, attackers sent a fake email to the person who looks after some very popular packages. They stole his login details, and published harmful new versions of packages like chalk and debug. Together, the affected packages are downloaded more than two billion times every week. The harmful code tried to steal people's digital money. And it was not the first time. Similar attacks happened in 2017, 2018, 2021 and 2024.

Fifth question. How would your project help in a case like that? We tested it. We asked depscan to check that harmful version of chalk. It answered: critical, 100 out of 100. Reported as malicious. To be honest, depscan cannot catch a brand new attack in the first minutes. It depends on public reports. But once experts report a bad package, everyone using depscan is warned. It also warns you when your versions are not locked, so a harmful new version cannot slip in by itself.

Sixth question. Who would use this in the real world? Students and solo developers can check a project before submitting it. Small companies can check every new package before it is added. And open-source projects can run it automatically on every change, for free.

Seventh question. How does it work? Four simple steps. Read. Ask. Score. Report. Read: it reads the list of packages your project uses. Ask: it asks trusted public websites about each package. Score: it adds points for every warning sign. Report: it shows you the result, with reasons.

Eighth question. What is the architecture? That simply means: what are the main parts, and how are they connected? On the left are the inputs. These are the package lists from your project. In the middle is depscan itself. It has five workers: a reader, collectors, a name checker, a scorer and a reporter. At the top are the information sources on the internet that the collectors talk to. On the right are the outputs: a table on your screen, a web page, and a comment on GitHub. Information flows from left to right, like a factory line.

Ninth question. What does each part of the code do? Think of depscan as a small office with different workers. The reader is like a receptionist. It reads your package list, and writes down each name and version. The messenger carries questions to the internet and brings back answers. It remembers answers for one day, so it never asks twice. The collectors are researchers. They find out how old a package is, how many people use it, and whether it has known security holes. The name checker is a spelling expert. It compares each name with famous package names, to catch fakes.

The scorer is the judge. It adds points for every warning sign, and decides: low, medium, high or critical. The manager organises everyone. It checks many packages at the same time, which makes it fast. The reporter is the writer. It turns the results into a table, a web page, or a comment. The front desk is where you type the command. And the tests are inspectors. 77 automatic checks make sure nothing is broken.

Let us follow one package through the office. The reader finds the name "reqeusts" in the list. The collectors ask the public store. The store says: this package does not exist. That is 70 points. The name checker notices it is almost the same as the famous package "requests". That is 45 more. The judge adds them up. The score stops at 100. Critical. The reporter prints the warning, with both reasons.

Tenth question. What outside tools did you use? Python is the programming language. PyPI and npm are the public stores where packages live. OSV.dev is a public list of known security problems, run by Google. deps.dev gives a health score for the place where a package's code is kept. GitHub Actions runs depscan automatically, and GitHub Pages shows the web page. All of these are free. The total cost of the project is zero.

Eleventh question. How is the score calculated? Every warning sign has fixed points. Reported harmful code: 100. A package that does not exist: 70. Known security holes: up to 60. A lookalike name: 45 or more. Smaller signs, like a very new package, or a version that is not locked, add fewer points. Add the points together. The maximum is 100. 0 to 24 is low. 25 to 49 is medium. 50 to 74 is high. 75 and above is critical.

Twelfth question. How do you use it, and what do you see? You type one command. A few seconds later you get a table, with the most dangerous packages at the top. You can also open the results as a web page, and click any package to see the reasons. And on GitHub, it checks every new package automatically, and blocks the risky ones.

Thirteenth question. How do you know it works? We built two demo projects with risky packages on purpose. depscan checked 19 packages in about two and a half seconds. It found 5 critical and 8 high risk packages: fake names, a harmful package, and old versions with security holes. A healthy package, numpy, scored zero. Good packages are not blamed. And 77 automatic tests check the code every time it changes.

Fourteenth question. What can it not do? It only checks the packages you list directly, not the hidden ones they bring along. It works for Python and JavaScript projects only. And the score is a warning, not a final verdict. A person still decides. In the future, it can check hidden packages too, and support more programming languages.

Finally: how should you present this project? Start with the problem: the house and the bad brick. Then tell the real story from 2025. Then show the tool running. Then explain the four steps, and the workers in the office. End with the results, and the limits. Being honest about limits earns respect.

Remember this one line: depscan is a security guard for the code you borrow from strangers. It reads your list, asks public sources, adds up the warning signs, and explains the danger. Now you can explain this project to anyone. Good luck.
