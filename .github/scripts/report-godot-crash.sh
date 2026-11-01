#!/bin/sh
set -eu

# Runs after a failed job. Prints what each core dump captured by
# setup-godot.sh shows, and how far the import had got when the process died.
found=0
for core in "${RUNNER_TEMP:?}"/core.*; do
	test -e "$core" || continue
	found=$((found + 1))
done
if [ "$found" -eq 0 ]; then
	echo "report-godot-crash: 0 core dumps in $RUNNER_TEMP; no process dumped core in this job."
	exit 0
fi

echo "report-godot-crash: $found core dump(s) in $RUNNER_TEMP."
if ! command -v gdb >/dev/null; then
	sudo apt-get update --quiet --quiet > /dev/null
	sudo apt-get install --quiet --quiet --yes --no-install-recommends gdb > /dev/null
fi
gdb --version | head -n 1

echo "Import output at the time of the report, from project/.godot/imported:"
ls -l project/.godot/imported || true

for core in "${RUNNER_TEMP:?}"/core.*; do
	echo "=== $core"
	gdb --batch --nx \
		-ex 'info threads' \
		-ex 'thread apply all backtrace 40' \
		"$(command -v godot)" "$core"
done
