# Fails unless the Make targets the jobs of .github/workflows/ci.yml call cover
# exactly what `make ci` runs, so a target added to `ci` or `test` cannot run
# locally and silently never in CI. Both sides are expanded through Make's own
# rule database: a target without a recipe, such as `ci` or `test`, stands for
# its prerequisites; a target with a recipe is a check that runs.

import re
import subprocess
import sys

WORKFLOW = ".github/workflows/ci.yml"

# --question runs no recipe; it exits 1 when a target is out of date and 2 on an error.
make = subprocess.run(
    ["make", "--print-data-base", "--no-builtin-rules", "--no-builtin-variables", "--question"],
    capture_output=True,
    text=True,
)
if make.returncode not in (0, 1):
    sys.exit(f"check-ci-coverage: make could not read Makefile:\n{make.stderr}")
database = make.stdout
rules = {}
target = None
for line in database.splitlines():
    if line.startswith("# Not a target") or not line.strip():
        target = None
        continue
    rule = re.match(r"^([A-Za-z0-9_.-]+):(?!=)(.*)$", line)
    if rule:
        target = rule.group(1)
        rules[target] = {"prerequisites": rule.group(2).split(), "recipe": False}
    elif target and line.startswith("#  recipe to execute"):
        rules[target]["recipe"] = True


def expand(name):
    if name not in rules:
        sys.exit(f"check-ci-coverage: {WORKFLOW} calls make {name}, which Makefile does not define.")
    if rules[name]["recipe"]:
        return {name}
    return set().union(*(expand(p) for p in rules[name]["prerequisites"]))


# A job's run: line calls make with literal target names, or with $TARGETS, which a
# job's matrix sets to one entry of its `targets:` list.
text = open(WORKFLOW).read()
called = []
for command in re.findall(r"^ *(?:- )?run: .*?\bmake ([^\n#]+)", text, re.MULTILINE):
    for word in command.split():
        if word == "$TARGETS":
            entries = re.findall(r"^ +- (test[^\n]*)$", text, re.MULTILINE)
            called += [name for entry in entries for name in entry.split()]
        elif word.startswith("$"):
            sys.exit(f"check-ci-coverage: cannot read the targets behind {word} in {WORKFLOW}.")
        else:
            called.append(word)
if not called:
    sys.exit(f"check-ci-coverage: read no make call in {WORKFLOW}.")

expected = expand("ci")
actual = set().union(*(expand(name) for name in called))
for name in sorted(expected - actual):
    print(f"check-ci-coverage: make ci runs {name}; no job in {WORKFLOW} does.")
for name in sorted(actual - expected):
    print(f"check-ci-coverage: a job in {WORKFLOW} runs {name}; make ci does not.")
if expected != actual:
    sys.exit(1)
print(
    f"check-ci-coverage: the jobs in {WORKFLOW} run all {len(expected)} targets make ci runs "
    f"({' '.join(sorted(expected))}), selected by: the recipes `make ci` reaches in Make's rule database."
)
