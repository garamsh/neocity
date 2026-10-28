#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
case "$(uname -m)" in
    arm64 | x86_64) ;;
    *) echo "This installer supports Intel and Apple Silicon macOS." >&2; exit 1 ;;
esac
if ! xcrun --find clang++ >/dev/null 2>&1 || ! python3 -m venv --help >/dev/null 2>&1; then
    echo "Install the Xcode Command Line Tools first: xcode-select --install" >&2
    exit 1
fi
mkdir -p .tools
touch .tools/.gdignore
if [ ! -x .tools/Godot.app/Contents/MacOS/Godot ]; then
    curl -fLsS --retry 3 https://github.com/godotengine/godot-builds/releases/download/4.7.2-stable/Godot_v4.7.2-stable_macos.universal.zip -o .tools/godot.zip
    printf '%s  %s\n' 38aa16e5bba2083941fc5b3e54be0089bd4cc35e32415f5b9fd9a8a6a7b9818255d44532ea8ef94b5aef56c4b407c2d634fa4f657e4ebe681ebbf59b7bac69ca .tools/godot.zip | shasum -a 512 -c -
    ditto -xk .tools/godot.zip .tools
    rm .tools/godot.zip
fi
.tools/Godot.app/Contents/MacOS/Godot --version
xcrun clang++ --version | head -n 1
python3 --version
printf '%s\n' 'Project-local tools ready. The first build installs the pinned C++ tools under .tools/cpp. Double-click play.command to build and play.'
