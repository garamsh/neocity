#!/usr/bin/env python3
"""Export the Toxic Night Aseprite kit; --prepare rebuilds sources from its atlas."""
import argparse
import hashlib
import json
import os
import pathlib
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
KIT = ROOT / 'docs/art/production/toxic-night-kit'
ART = ROOT / 'project/levels/monster_mouth_escape/art_hd'
# Native canvases retain the renderer's existing band layout and cell density.
ASSETS = {
    'backgrounds/city.png': 'Toxic Night ruined skyline and derailed viaduct',
    'world/terminal.png': 'Toxic Night terminal skyline behind eastern floors',
    'walkway/deck.png': 'Six twenty-cell deck bands; flat contact at row zero',
    'world/building_wall.png': 'Four twenty-cell violet masonry and window bands',
    'world/shop.png': 'Two twenty-cell lit kitchen and noodle frontage bands',
    'world/interior_kitchen.png': 'Kitchen stove and ingredient shelves',
    'world/interior_noodle.png': 'Noodle boiler, bowls and exhaust hood',
    'world/interior_cold_store.png': 'Cold-store refrigeration lockers and pipes',
    'world/interior_service.png': 'Aquarium service pumps and glass tanks',
    'world/ladder.png': 'One repeating corroded iron ladder cell',
    'world/fire_escape.png': 'One repeating corroded fire-escape ladder cell',
    'world/vending_machine.png': 'Broken red vending machine with neon rim',
    'world/kiosk.png': 'Battered kiosk with awning and warm work lights',
    'world/cold_store.png': 'Cold-store shutter with hazard-marked jambs',
    'world/door_cold_store.png': 'Steel shutter passage with hazard-marked jambs',
    'world/ferry.png': 'Wrecked passenger rail carriage',
    'world/kiosk_ruined.png': 'Collapsed kiosk and broken pink sign',
    'world/vendor_cart.png': 'Corroded noodle counter and boiler',
}


def run(*args):
    subprocess.run([str(part) for part in args], check=True,
                   env={**os.environ, 'DISPLAY': '', 'WAYLAND_DISPLAY': ''})


def source_path(destination):
    return KIT / (destination.replace('/', '-')[:-4] + '.aseprite')


def prepare():
    """Assemble native sources once; later edits belong in the Aseprite files."""
    with tempfile.TemporaryDirectory(prefix='w276-kit-') as temporary:
        work = pathlib.Path(temporary)
        source = KIT / 'source.png'

        def crop(name, box):
            target = work / (name + '.png')
            run('aseprite', '--batch', source, '--crop', box, '--save-as', target)
            return target

        def save(destination, *command):
            target = work / 'assembled.png'
            run('magick', *command, '-strip', 'PNG32:' + str(target))
            run('aseprite', '--batch', target, '--save-as', source_path(destination))

        skyline = work / 'skyline.png'
        run('aseprite', '--batch', KIT / 'backdrop-source.png',
            '--crop', '0,304,1717,299', '--save-as', skyline)
        for destination, size in [('backgrounds/city.png', '1920x735!'),
                                  ('world/terminal.png', '2304x1152!')]:
            save(destination, skyline, '-fuzz', '8%', '-transparent', '#303030',
                 '-filter', 'Point', '-resize', size,
                 '-channel', 'A', '-threshold', '50%', '+channel')
        deck = crop('deck', '0,25,1536,216')
        wall = crop('wall', '0,256,1536,248')
        kitchen = crop('kitchen', '0,516,256,240')
        noodle = crop('noodle', '256,516,256,240')
        cold = crop('cold', '512,516,256,240')
        service = crop('service', '1024,516,256,240')
        bands = []
        for index in range(6):
            piece = work / f'deck{index}.png'
            run('magick', deck, '-crop', f'384x84+{(index % 4) * 384}+0',
                '+repage', '-write', str(piece), '+delete', '-size', '1920x84',
                'tile:' + str(piece), str(piece))
            bands.append(piece)
        stacked = work / 'deck-stacked.png'
        run('magick', *bands, '-append', str(stacked))
        # Recess the face below the untouched twelve-pixel contact rim.
        for row in range(6):
            run('magick', stacked, '-region', f'1920x72+0+{row * 84 + 12}',
                '-modulate', '55', '+region', str(stacked))
        save('walkway/deck.png', stacked)
        bands = []
        for index, height in enumerate([96, 96, 96, 108]):
            piece = work / f'wall{index}.png'
            run('magick', wall, '-crop', f'384x{height}+{index * 384}+{index * 40}',
                '+repage', '-write', str(piece), '+delete', '-size', f'1920x{height}',
                'tile:' + str(piece), str(piece))
            bands.append(piece)
        save('world/building_wall.png', *bands, '-append')
        bands = []
        for index, module in enumerate([kitchen, noodle]):
            piece = work / f'shop{index}.png'
            run('magick', module, '-filter', 'Point', '-resize', '384x192!',
                '-write', str(piece), '+delete', '-size', '1920x192',
                'tile:' + str(piece), str(piece))
            bands.append(piece)
        save('world/shop.png', *bands, '-append')
        for name, module, size in [('kitchen', kitchen, '576x192!'),
                                   ('noodle', noodle, '576x192!'),
                                   ('cold_store', cold, '288x288!'),
                                   ('service', service, '288x288!')]:
            save(f'world/interior_{name}.png', module, '-filter', 'Point', '-resize', size)
        props = [
            ('world/ladder.png', '96,770,68,96', '96x96!'),
            ('world/fire_escape.png', '96,770,68,96', '96x96!'),
            ('world/vending_machine.png', '290,790,162,225', '136x177'),
            ('world/kiosk.png', '522,782,237,234', '96x83'),
            ('world/cold_store.png', '781,799,230,216', '285x221'),
            ('world/door_cold_store.png', '781,799,230,216', '96x192!'),
            ('world/ferry.png', '1027,813,250,202', '268x150'),
            ('world/kiosk_ruined.png', '1294,800,234,215', '283x137'),
        ]
        canvases = {'world/ladder.png': '96x96', 'world/fire_escape.png': '96x96',
                    'world/vending_machine.png': '144x177', 'world/kiosk.png': '96x96',
                    'world/cold_store.png': '288x288', 'world/door_cold_store.png': '96x192',
                    'world/ferry.png': '384x150', 'world/kiosk_ruined.png': '288x144'}
        for destination, box, fit in props:
            module = crop(destination.replace('/', '-'), box)
            save(destination, module, '-fuzz', '8%', '-transparent', '#303030',
                 '-filter', 'Point', '-resize', fit, '-gravity', 'south',
                 '-background', 'none', '-extent', canvases[destination],
                 '-channel', 'A', '-threshold', '50%', '+channel')
        save('world/vendor_cart.png', noodle, '-crop', '256x120+0+120', '+repage',
             '-filter', 'Point', '-resize', '128x96!', '-gravity', 'south',
             '-background', 'none', '-extent', '192x96')


def export():
    """Export each native source and record its sole owner in the HD manifest."""
    manifest_path = ART / 'manifest.json'
    manifest = json.loads(manifest_path.read_text())
    entries = {entry['path']: entry for entry in manifest['files']}
    for destination, role in ASSETS.items():
        source = source_path(destination)
        target = ART / destination
        run('aseprite', '--batch', source, '--save-as', target)
        dimensions = subprocess.check_output(
            ['magick', 'identify', '-format', '%w %h', str(target)], text=True)
        entry = {'path': destination, 'size': [int(v) for v in dimensions.split()],
                 'role': role, 'sha256': hashlib.sha256(target.read_bytes()).hexdigest(),
                 'issue': 276, 'source': str(source.relative_to(ROOT)),
                 'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
                 'export': 'python3 tools/art/toxic_night.py',
                 'retouch': 'Aseprite 1.3 batch export; nearest-only atlas assembly recorded in tools/art/toxic_night.py'}
        entries[destination] = entry
    manifest['files'] = list(entries.values())
    manifest['map_kit'] = {'issue': 276, 'source': 'docs/art/production/toxic-night-kit/',
                           'grid': '96 native pixels per cell; one canvas pixel per native pixel at close zoom',
                           'sampling': 'nearest', 'status': 'first kit; creative review pending'}
    manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'Exported {len(ASSETS)} native Aseprite map assets selected by ASSETS.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepare', action='store_true', help='replace native sources from the recorded atlas')
    args = parser.parse_args()
    if args.prepare:
        prepare()
    export()
