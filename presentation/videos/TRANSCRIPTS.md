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
