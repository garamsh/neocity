#!/bin/bash
# Temporary (#39): repeats a true first import with a cold page cache and
# stdout on a pipe, as make ci does it. $2 is the number of CPU hogs.
attempts=$1
hogs=${2:-0}
for _ in $(seq 1 "$hogs"); do yes > /dev/null & done
failures=0
for i in $(seq 1 "$attempts"); do
	rm -rf project/.godot
	home=$(mktemp -d "$RUNNER_TEMP/home.XXXXXX")
	log=$(mktemp "$RUNNER_TEMP/log.XXXXXX")
	sync
	echo 3 | sudo tee /proc/sys/vm/drop_caches > /dev/null
	HOME=$home timeout -k 5 180 godot --headless --path project --import 2>&1 | cat > "$log"
	rc=${PIPESTATUS[0]}
	if [ "$rc" -ne 0 ]; then
		failures=$((failures + 1))
		echo "MEASURE attempt=$i rc=$rc"
		tail -n 8 "$log"
		sh .github/scripts/report-godot-crash.sh || echo "report failed"
		sudo rm -f "$RUNNER_TEMP"/core.*
	fi
	rm -rf "$home" "$log"
done
echo "MEASURE attempts=$attempts failures=$failures hogs=$hogs"
kill $(jobs -p) 2> /dev/null
exit 0
