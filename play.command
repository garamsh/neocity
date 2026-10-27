#!/bin/sh
set -eu
cd "$(dirname "$0")"
if [ ! -x .tools/Godot.app/Contents/MacOS/Godot ] || [ ! -x .tools/cargo/bin/cargo ]; then
    echo "Run scripts/setup-macos.sh once to install the project-local tools."
    exit 1
fi
export CARGO_HOME="$PWD/.tools/cargo"
export RUSTUP_HOME="$PWD/.tools/rustup"
make build
exec .tools/Godot.app/Contents/MacOS/Godot --path .
