#!/usr/bin/env python3
"""Cut the fixed-map HD resources from the recorded ImageGen sources with ImageMagick."""
import hashlib
import json
import pathlib
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'docs/art/production/map-hd-sources'
ART = ROOT / 'project/levels/monster_mouth_escape/art_hd'
CUTS = [
    ('aquarium.png', None, '1920x1080!', 'backgrounds/aquarium.png', 'Aquarium wall and flooded tanks'),
    ('terminal.png', None, '2304x1152!', 'world/terminal.png', 'Street terminal scenery behind the fixed floors'),
    ('interiors.png', '1717x295+0+0', '576x192!', 'world/interior_kitchen.png', 'Occupied kitchen loft background'),
    ('interiors.png', '1717x300+0+306', '576x192!', 'world/interior_noodle.png', 'Occupied noodle loft background'),
    ('interiors.png', '576x288+550+615', '288x288!', 'world/interior_cold_store.png', 'Occupied cold store background'),
    ('structures.png', '660x145+50+214', '384x84!', 'walkway/aquarium_deck.png', 'Four-cell teal deck band; level top at source row zero'),
    ('structures.png', '290x210+785+192', '96x96!', 'world/span_wreck.png', 'Collapsed span end; reflect for the other end'),
    ('structures.png', '750x335+10+620', '288x144!', 'world/kiosk_ruined.png', 'Day-ten collapsed kiosk'),
    ('structures.png', '92x96+1106+572', '96x96!', 'world/fire_escape.png', 'Day-five fire escape repeating ladder segment'),
]


def main():
    manifest_path = ART / 'manifest.json'
    manifest = json.loads(manifest_path.read_text())
    paths = {row[3] for row in CUTS}
    manifest['files'] = [row for row in manifest['files'] if row['path'] not in paths]
    for original, crop, size, destination, role in CUTS:
        source = SOURCE / original
        target = ART / destination
        command = ['magick', str(source)]
        if crop:
            command += ['-crop', crop, '+repage']
        command += ['-filter', 'Lanczos', '-resize', size]
        if original in ('structures.png', 'terminal.png'):
            command += ['-channel', 'A', '-threshold', '50%', '+channel']
        command += ['-depth', '8', 'PNG32:' + str(target)]
        subprocess.run(command, check=True)
        dimensions = subprocess.check_output(['magick', 'identify', '-format', '%w %h', str(target)], text=True)
        manifest['files'].append({
            'path': destination,
            'size': [int(value) for value in dimensions.split()],
            'role': role,
            'sha256': hashlib.sha256(target.read_bytes()).hexdigest(),
            'source': 'docs/art/production/map-hd-sources/' + original,
            'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
            'crop': crop,
            'resize': size,
            'retouch': 'ImageMagick crop and one Lanczos resample; no painted pixels',
        })
    manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'Wrote {len(CUTS)} fixed-map PNGs and updated their HD manifest entries.')


if __name__ == '__main__':
    main()
