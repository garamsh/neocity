#!/bin/bash
# Temporary (#39): repeats a true first import, as a fresh runner does it.
attempts=$1
failures=0
for i in $(seq 1 "$attempts"); do
	rm -rf project/.godot
	home=$(mktemp -d "$RUNNER_TEMP/home.XXXXXX")
	log=$(mktemp "$RUNNER_TEMP/log.XXXXXX")
	HOME=$home timeout -k 5 180 godot --headless --path project --import > "$log" 2>&1
	rc=$?
	if [ "$rc" -ne 0 ]; then
		failures=$((failures + 1))
		echo "MEASURE attempt=$i rc=$rc"
		tail -n 8 "$log"
		sh .github/scripts/report-godot-crash.sh || echo "report failed"
		sudo rm -f "$RUNNER_TEMP"/core.*
	fi
	rm -rf "$home" "$log"
done
echo "MEASURE attempts=$attempts failures=$failures"
