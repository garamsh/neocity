#!/usr/bin/env bash
# Runs a command on a private nested Wayland display, the only way this project
# starts a compositor. The compositor gets its own session bus and an empty
# config directory, so it never reads the user's kwinrc, attaches to the user's
# input method or touches their desktop.
# Usage: tools/private-display.sh [--width W] [--height H] -- command [args...]
# Run Godot inside it with `--display-driver x11`, as the interior-camera
# captures did; its Wayland driver crashed in the day save here.
set -euo pipefail

width=1920
height=1080
while [ "$#" -gt 0 ]; do
  case "$1" in
  --width) width=$2; shift 2 ;;
  --height) height=$2; shift 2 ;;
  --) shift; break ;;
  *) echo "usage: $0 [--width W] [--height H] -- command [args...]" >&2; exit 2 ;;
  esac
done
[ "$#" -gt 0 ] || { echo "no command given" >&2; exit 2; }

load=$(cut -d' ' -f1 /proc/loadavg)
if ! awk -v load="$load" 'BEGIN { exit !(load < 6) }'; then
  echo "one-minute load $load is not below 6; not starting a display" >&2
  exit 1
fi

for tool in dbus-run-session kwin_wayland; do
  command -v "$tool" >/dev/null || { echo "$tool is required" >&2; exit 1; }
done

# The socket path must stay under 108 bytes, so the runtime directory is short.
runtime=$(mktemp -d "${TMPDIR:-/tmp}/nd.XXXXXX")
config=$(mktemp -d "${TMPDIR:-/tmp}/nc.XXXXXX")
trap 'rm -rf "$runtime" "$config"' EXIT
chmod 700 "$runtime"

# kwin takes one session program and quits when it exits, so the command and
# its arguments go into one script, quoted.
printf 'exec' >"$runtime/run"
printf ' %q' "$@" >>"$runtime/run"
chmod 700 "$runtime/run"

dbus-run-session -- env XDG_CONFIG_HOME="$config" XDG_RUNTIME_DIR="$runtime" \
  kwin_wayland --virtual --width "$width" --height "$height" \
  --no-lockscreen --no-global-shortcuts --xwayland --socket nd-wayland \
  --exit-with-session "$runtime/run"
