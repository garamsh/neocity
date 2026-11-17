#!/usr/bin/env python3
"""Cut the #251 street and kitchen art from the recorded Codex sources with ImageMagick.

Run from anywhere: python3 docs/art/production/street_art_251.py
Each CUTS row is (source, crop, resize, destination, role, alpha_threshold).
The art_hd manifest entry of every destination is replaced or added.
"""
import hashlib
import json
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[3]
SOURCE = ROOT / 'docs/art/production/street-art-251-sources'
ART = ROOT / 'project/levels/monster_mouth_escape/art_hd'
CUTS = [
    ('kitchen.png', None, '1920x1080!', 'base/kitchen.png', 'Base kitchen background without characters, fridge, crane or interface', False),
]


def main():
    manifest_path = ART / 'manifest.json'
    manifest = json.loads(manifest_path.read_text())
    for original, crop, size, destination, role, alpha in CUTS:
        source = SOURCE / original
        target = ART / destination
        command = ['magick', str(source)]
        if crop:
            command += ['-crop', crop, '+repage']
        command += ['-filter', 'Lanczos', '-resize', size]
        if alpha:
            command += ['-channel', 'A', '-threshold', '50%', '+channel']
        command += ['-depth', '8', ('PNG32:' if alpha else 'PNG24:') + str(target)]
        subprocess.run(command, check=True)
        dimensions = subprocess.check_output(['magick', 'identify', '-format', '%w %h', str(target)], text=True)
        entry = {
            'path': destination,
            'size': [int(value) for value in dimensions.split()],
            'role': role,
            'sha256': hashlib.sha256(target.read_bytes()).hexdigest(),
            'issue': 251,
            'source': 'docs/art/production/street-art-251-sources/' + original,
            'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
            'crop': crop,
            'resize': size,
            'retouch': 'ImageMagick crop and one Lanczos resample; no painted pixels',
        }
        rows = manifest['files']
        for index, row in enumerate(rows):
            if row['path'] == destination:
                rows[index] = {**row, **entry}
                break
        else:
            rows.append(entry)
    manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'Wrote {len(CUTS)} PNGs and updated their HD manifest entries.')


if __name__ == '__main__':
    main()
