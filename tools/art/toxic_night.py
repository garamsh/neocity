#!/usr/bin/env python3
"""Export the Toxic Night Aseprite kit; --prepare rebuilds it from the HD sources."""
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
# Band proportions stay fixed while the authored cell grows to 144 pixels.
ASSETS = {
    'backgrounds/throat.png': 'Opaque Toxic Night magenta palate, cream fangs and red uvula',
    'backgrounds/acid.png': 'Toxic Night luminous acid lake and drowned trains; transparent above ruins',
    'backgrounds/aquarium.png': 'Toxic Night teal aquarium tanks, broken glass and amber work lights',
    'backgrounds/city.png': 'Toxic Night ruined skyline and derailed viaduct',
    'world/terminal.png': 'Toxic Night terminal skyline behind eastern floors',
    'walkway/deck.png': 'Six twenty-cell deck bands; flat contact at row zero',
    'world/building_wall.png': 'Four twenty-cell violet masonry and window bands',
    'world/shop.png': 'Two twenty-cell lit kitchen and noodle frontage bands',
    'world/interior_kitchen.png': 'Enclosed kitchen floor with door, preparation benches and ladder',
    'world/interior_noodle.png': 'Enclosed noodle dining floor with counter, boiler, doors and ladder',
    'world/interior_cold_store.png': 'Enclosed cold-store floor with personnel door, food lockers and ladder',
    'world/interior_service.png': 'Enclosed aquarium service floor with door, tanks, pumps and ladder',
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
BACKGROUND_SIZES = {'throat': '1517x1034!', 'acid': '2880x761!',
                    'aquarium': '2880x1620!'}


def run(*args):
    subprocess.run([str(part) for part in args], check=True,
                   env={**os.environ, 'DISPLAY': '', 'WAYLAND_DISPLAY': ''})


def source_path(destination):
    return KIT / (destination.replace('/', '-')[:-4] + '.aseprite')


def prepare():
    """Replace native sources from the recorded HD redraws, discarding native edits."""
    with tempfile.TemporaryDirectory(prefix='w276-kit-') as temporary:
        work = pathlib.Path(temporary)
        sources = KIT / 'hd-sources'

        def crop(name, box):
            target = work / (name + '.png')
            run('magick', sources / 'street-atlas.png',
                '-crop', box, '+repage', target)
            return target

        def save(destination, *command):
            target = work / 'assembled.png'
            run('magick', *command, '+dither', '-colors', '96',
                '-channel', 'A', '-threshold', '50%', '+channel',
                '-strip', '-depth', '8', 'PNG32:' + str(target))
            run('aseprite', '--batch', target, '--save-as', source_path(destination))

        def strip(module, height, name):
            piece = work / (name + '.png')
            run('magick', module, '-filter', 'Point', '-resize', f'288x{height}!', piece)
            mirror = work / (name + '-mirror.png')
            run('magick', piece, '-flop', mirror)
            period = work / (name + '-period.png')
            run('magick', piece, mirror, '+append', period)
            target = work / (name + '-strip.png')
            run('magick', '-size', f'2880x{height}', 'tile:' + str(period), target)
            return target

        save('backgrounds/city.png', sources / 'city.png', '-fuzz', '8%',
             '-transparent', '#303030', '-filter', 'Point', '-resize', '2880x1103!')
        skyline = work / 'terminal-skyline.png'
        run('magick', sources / 'city.png', '-fuzz', '8%', '-transparent', '#303030',
            '-filter', 'Point', '-resize', '1728x864!', skyline)
        mirror = work / 'terminal-mirror.png'
        run('magick', skyline, '-flop', mirror)
        save('world/terminal.png', skyline, mirror, '+append', '-gravity', 'south',
             '-background', 'none', '-extent', '3456x1728')
        bands = []
        for index, x in enumerate([8, 270, 515, 770, 1030, 1140]):
            module = crop(f'deck{index}', f'384x126+{x}+34')
            band = strip(module, 126, f'deck{index}')
            run('magick', band, '-region', '2880x18+0+0',
                '-modulate', '150', '+region', band)
            run('magick', band, '-region', '2880x108+0+18',
                '-modulate', '50', '+region', band)
            bands.append(band)
        save('walkway/deck.png', *bands, '-append')
        bands = []
        for index, (x, height) in enumerate(zip([8, 390, 775, 1144], [144, 144, 144, 162])):
            module = crop(f'wall{index}', f'384x168+{x}+164')
            bands.append(strip(module, height, f'wall{index}'))
        save('world/building_wall.png', *bands, '-append')
        bands = []
        for name, box in [('kitchen', '640x180+352+348'),
                          ('noodle', '470x180+1050+348')]:
            bands.append(strip(crop(name, box), 288, name))
        save('world/shop.png', *bands, '-append')
        for name, size in [('kitchen', '864x288!'), ('noodle', '864x288!'),
                           ('cold_store', '432x432!'), ('service', '432x432!')]:
            save(f'world/interior_{name}.png', sources / (name + '.png'),
                 '-filter', 'Point', '-resize', size)
        props = [
            ('world/ladder.png', '108x206+146+550', '144x144!', '144x144'),
            ('world/fire_escape.png', '108x206+146+550', '144x144!', '144x144'),
            ('world/vending_machine.png', '184x207+474+550', '204x266', '216x266'),
            ('world/kiosk.png', '280x207+810+548', '144x125', '144x144'),
            ('world/cold_store.png', '315x214+1190+543', '428x332', '432x432'),
            ('world/door_cold_store.png', '315x214+1190+543', '144x288!', '144x288'),
            ('world/ferry.png', '425x199+20+794', '402x225', '576x225'),
            ('world/kiosk_ruined.png', '290x206+472+791', '425x206', '432x216'),
            ('world/vendor_cart.png', '294x209+820+790', '192x144', '288x144'),
        ]
        for destination, box, fit, canvas in props:
            module = crop(destination.replace('/', '-'), box)
            save(destination, module, '-fuzz', '8%', '-transparent', '#303030',
                 '-trim', '+repage', '-filter', 'Point', '-resize', fit,
                 '-gravity', 'south', '-background', 'none', '-extent', canvas)
    prepare_backgrounds()


def prepare_backgrounds():
    """Build native background canvases from the recorded redraws."""
    with tempfile.TemporaryDirectory(prefix='w276-backgrounds-') as temporary:
        target = pathlib.Path(temporary) / 'assembled.png'
        for name, size in BACKGROUND_SIZES.items():
            source = KIT / 'hd-sources' / (name + '.png')
            if name == 'acid':
                source = pathlib.Path(temporary) / 'anchored-acid.png'
                original = KIT / 'hd-sources/acid.png'
                # Keep the earlier lake's far waterline at one third of its canvas.
                run('magick', '(', original, '-crop', '2103x300+0+0', '+repage',
                    '-filter', 'Point', '-resize', '2880x250!', ')',
                    '(', original, '-crop', '2103x448+0+300', '+repage',
                    '-filter', 'Point', '-resize', '2880x511!', ')', '-append', source)
            run('magick', source, '-filter', 'Point', '-resize', size,
                '+dither', '-colors', '96',
                '-channel', 'A', '-threshold', '50%', '+channel',
                '-strip', '-depth', '8', 'PNG32:' + str(target))
            run('aseprite', '--batch', target, '--save-as',
                source_path(f'backgrounds/{name}.png'))


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
                 'retouch': '144px-cell redraw; nearest resampling, 96-color un-dithered cleanup; Aseprite batch export',
                 'pixels_per_cell': 144}
        entries[destination] = entry
    manifest['files'] = list(entries.values())
    manifest['map_kit'] = {'issue': 276, 'source': 'docs/art/production/toxic-night-kit/',
                           'grid': '144 authored pixels per cell; destination world areas owned by #281',
                           'sampling': 'nearest', 'status': 'interiors, street and three backgrounds redrawn; supports, other props, containers and hazards pending',
                           'hd_paths': list(ASSETS)}
    manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'Exported {len(ASSETS)} native Aseprite map assets selected by ASSETS.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepare', action='store_true', help='replace native sources from the recorded HD images')
    parser.add_argument('--prepare-backgrounds', action='store_true',
                        help='replace only the three native background sources')
    args = parser.parse_args()
    if args.prepare:
        prepare()
    elif args.prepare_backgrounds:
        prepare_backgrounds()
    export()
