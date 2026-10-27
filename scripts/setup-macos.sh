#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
case "$(uname -m)" in
    arm64) rust_host=aarch64-apple-darwin ;;
    x86_64) rust_host=x86_64-apple-darwin ;;
    *) echo "This installer supports Intel and Apple Silicon macOS." >&2; exit 1 ;;
esac
mkdir -p .tools
touch .tools/.gdignore
export CARGO_HOME="$PWD/.tools/cargo"
export RUSTUP_HOME="$PWD/.tools/rustup"
if [ ! -x .tools/cargo/bin/rustup ]; then
    curl -fLsS --retry 3 "https://static.rust-lang.org/rustup/dist/$rust_host/rustup-init" -o .tools/rustup-init
    curl -fLsS --retry 3 "https://static.rust-lang.org/rustup/dist/$rust_host/rustup-init.sha256" -o .tools/rustup-init.sha256
    expected=$(awk '{print $1}' .tools/rustup-init.sha256)
    printf '%s  %s\n' "$expected" .tools/rustup-init | shasum -a 256 -c -
    chmod +x .tools/rustup-init
    .tools/rustup-init -y --no-modify-path --profile minimal --default-toolchain 1.99.1 --component rustfmt --component clippy
    rm .tools/rustup-init .tools/rustup-init.sha256
else
    .tools/cargo/bin/rustup toolchain install 1.99.1 --profile minimal --component rustfmt --component clippy
fi
if [ ! -x .tools/Godot.app/Contents/MacOS/Godot ]; then
    curl -fLsS --retry 3 https://github.com/godotengine/godot-builds/releases/download/4.7.2-stable/Godot_v4.7.2-stable_macos.universal.zip -o .tools/godot.zip
    printf '%s  %s\n' 38aa16e5bba2083941fc5b3e54be0089bd4cc35e32415f5b9fd9a8a6a7b9818255d44532ea8ef94b5aef56c4b407c2d634fa4f657e4ebe681ebbf59b7bac69ca .tools/godot.zip | shasum -a 512 -c -
    ditto -xk .tools/godot.zip .tools
    rm .tools/godot.zip
fi
.tools/Godot.app/Contents/MacOS/Godot --version
.tools/cargo/bin/rustc --version
printf '%s\n' 'Project-local tools ready. Double-click play.command to build and play.'
