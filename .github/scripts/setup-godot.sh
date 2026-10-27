#!/bin/sh
set -eu

# SHA512-SUMS.txt from the official 4.7.2-stable release; updated by @sauce-git.
archive=Godot_v4.7.2-stable_linux.x86_64.zip
checksum=9aa00f7a605200940bce3027a567b782f49bd8e940dd06ae9e987bd65aee1b1467edd56ed84fcdcbdd44354bf613bdbb4e5d2913e925850368e150c59ed54c65
install_dir=$(mktemp -d "${RUNNER_TEMP:?}/godot.XXXXXX")
cd "$install_dir"
curl --fail --location --silent --show-error --retry 3 \
	"https://github.com/godotengine/godot-builds/releases/download/4.7.2-stable/$archive" \
	--output "$archive"
printf '%s  %s\n' "$checksum" "$archive" | sha512sum --check -
unzip -q "$archive"
mv Godot_v4.7.2-stable_linux.x86_64 godot
chmod +x godot
rm "$archive"
printf '%s\n' "$install_dir" >> "${GITHUB_PATH:?}"
./godot --version
