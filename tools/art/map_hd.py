#!/usr/bin/env python3
"""Rebuild legacy fixed-map cuts whose manifest still names map-hd-sources."""
import hashlib
import json
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[2]
LEGACY_SOURCE = 'docs/art/production/map-hd-sources/'


def main(root=ROOT, run=subprocess.run, measure=subprocess.check_output):
    art = root / 'project/levels/monster_mouth_escape/art_hd'
    manifest_path = art / 'manifest.json'
    manifest = json.loads(manifest_path.read_text())
    count = 0
    for entry in manifest['files']:
        if not entry.get('source', '').startswith(LEGACY_SOURCE):
            continue
        source = root / entry['source']
        target = art / entry['path']
        command = ['magick', str(source)]
        if entry['crop']:
            command += ['-crop', entry['crop'], '+repage']
        command += ['-filter', 'Lanczos', '-resize', entry['resize']]
        if source.name in ('structures.png', 'terminal.png'):
            command += ['-channel', 'A', '-threshold', '50%', '+channel']
        command += ['-depth', '8', 'PNG32:' + str(target)]
        run(command, check=True)
        dimensions = measure(
            ['magick', 'identify', '-format', '%w %h', str(target)], text=True)
        entry['size'] = [int(value) for value in dimensions.split()]
        entry['sha256'] = hashlib.sha256(target.read_bytes()).hexdigest()
        entry['source_sha256'] = hashlib.sha256(source.read_bytes()).hexdigest()
        count += 1
    manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'Rebuilt {count} legacy cuts selected by manifest source prefix {LEGACY_SOURCE}; '
          'all other owners preserved.')


if __name__ == '__main__':
    main()
