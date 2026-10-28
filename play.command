#!/bin/sh
set -eu
cd "$(dirname "$0")"
if [ ! -x .tools/Godot.app/Contents/MacOS/Godot ]; then
    echo "Run scripts/setup-macos.sh once to install the project-local tools."
    exit 1
fi
make build
exec .tools/Godot.app/Contents/MacOS/Godot --path project
