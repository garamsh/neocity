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
    'walkway/aquarium_deck.png': 'Four-cell teal grating with flat top, lit vents and corroded braces',
    'walkway/structure.png': 'Support sheet with amber lamp, broken piers, cable, chain and warning ticks',
    'world/ladder_broken.png': 'Broken iron ladder with surviving amber rungs and flesh growth',
    'world/door_broken.png': 'Torn violet shutter passage with amber lamp and rubble',
    'world/noodle_vendor.png': 'Side-facing noodle vendor with cap, apron and bowl',
    'world/bag_washed_up.png': 'Wet ochre backpack with straps and corroded clasps',
    'world/acid_drip.png': 'Magenta attachment and vertical lime acid droplets',
    'ground/rubble.png': 'Violet concrete rubble and bent rusty rebar',
    'world/span_wreck.png': 'Collapsed teal span end with snapped girder and wires',
    'containers/crates_closed.png': 'Closed stacked food crates with corroded iron corners',
    'containers/crates_open.png': 'Matching crate stack with upper lid raised',
    'containers/locker_closed.png': 'Closed oxidized teal supply locker',
    'containers/locker_open.png': 'Matching teal locker with door open and empty shelves',
    'containers/shelf_closed.png': 'Closed violet supply cabinet with roller shutters',
    'containers/shelf_open.png': 'Matching supply cabinet with raised shutters and food tins',
    'obstacles/acid_vent.png': 'Riveted vent with amber lights and luminous lime acid bubbles',
    'obstacles/flesh_spikes.png': 'Three ivory spikes growing from a magenta tendon mound',
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
    prepare_props()


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


def prepare_props():
    """Rebuild the remaining static map pieces at their recorded normalized anchors."""
    layout = json.loads((KIT / 'hd-sources/props-layout.json').read_text())
    with tempfile.TemporaryDirectory(prefix='w276-props-') as temporary:
        work = pathlib.Path(temporary)
        for destination, asset in layout['assets'].items():
            canvas = work / 'canvas.png'
            run('magick', '-size', 'x'.join(map(str, asset['size'])), 'xc:none',
                '-colorspace', 'sRGB', '-type', 'TrueColorAlpha', 'PNG32:' + str(canvas))
            for index, part in enumerate(asset['parts']):
                module = work / f'part-{index}.png'
                x, y, width, height = part['bounds']
                command = ['magick', KIT / 'hd-sources' / part['source'],
                           '-crop', part['crop'], '+repage',
                           '-channel', 'A', '-threshold', '50%', '+channel',
                           '-trim', '+repage']
                if part.get('flop'):
                    command.append('-flop')
                run(*command, '-filter', 'Point', '-resize', f'{width}x{height}!', module)
                run('magick', canvas, module, '-geometry', f'+{x}+{y}', '-composite', canvas)
            if destination == 'walkway/aquarium_deck.png':
                # Repeat a reflected half-period; both outer columns and the contact row meet.
                half = work / 'half.png'
                mirror = work / 'mirror.png'
                run('magick', canvas, '-crop', '288x126+0+0', '+repage', half)
                run('magick', half, '-flop', mirror)
                run('magick', half, mirror, '+append', canvas)
                run('magick', canvas, '-fill', '#88afae',
                    '-draw', 'rectangle 0,0 575,1', canvas)
            run('magick', canvas, '+dither', '-colors', '96',
                '-channel', 'A', '-threshold', '50%', '+channel',
                '-strip', '-depth', '8', 'PNG32:' + str(canvas))
            run('aseprite', '--batch', canvas, '--save-as', source_path(destination))
    print(f"Prepared {len(layout['assets'])} static map sources selected by props-layout.json.")


def export():
    """Export each native source and record its sole owner in the HD manifest."""
    manifest_path = ART / 'manifest.json'
    manifest = json.loads(manifest_path.read_text())
    entries = {entry['path']: entry for entry in manifest['files']}
    props = json.loads((KIT / 'hd-sources/props-layout.json').read_text())['assets']
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
        if destination in props:
            entry.update(props[destination].get('anchors', {}))
            entry['preparation'] = 'docs/art/production/toxic-night-kit/hd-sources/props-layout.json'
        if destination in ('walkway/deck.png', 'walkway/aquarium_deck.png',
                           'backgrounds/throat.png', 'backgrounds/acid.png',
                           'backgrounds/aquarium.png'):
            entry['retouch'] = '144px-cell kit; native #262 pixel finishing; Aseprite batch export'
        entries[destination] = entry
    for destination, source_name, size, role, issue in (
            ('endings/escaped.png', 'endings/escaped.png', None,
             'Chef and appraiser escape through the damaged aquarium-roof device', 304),
            ('base/feed_crane.png', 'art-polish-262/crane-source.png', '660x224!',
             'Corroded feeding crane; engine placement pending, not drawn by this PR', 262)):
        source = ROOT / 'docs/art/production' / source_name
        target = ART / destination
        if size:
            run('magick', source, '-filter', 'Point', '-resize', size,
                '-channel', 'A', '-threshold', '50%', '+channel',
                '-strip', 'PNG32:' + str(target))
        else:
            target.write_bytes(source.read_bytes())
        dimensions = subprocess.check_output(
            ['magick', 'identify', '-format', '%w %h', str(target)], text=True)
        entries[destination] = {
            'path': destination, 'size': [int(v) for v in dimensions.split()],
            'role': role, 'issue': issue,
            'source': str(source.relative_to(ROOT)),
            'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
            'sha256': hashlib.sha256(target.read_bytes()).hexdigest(),
            'export': 'python3 tools/art/toxic_night.py',
            'retouch': 'nearest 660x224; binary alpha' if size else 'unmodified generated original',
        }
    manifest['files'] = list(entries.values())
    manifest['map_kit'] = {'issue': 276, 'source': 'docs/art/production/toxic-night-kit/',
                           'grid': '144 authored pixels per cell; destination world areas owned by #281',
                           'sampling': 'nearest', 'status': 'all static map art redrawn; mutant animation frames retain their prior sources',
                           'hd_paths': list(ASSETS)}
    manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'Exported {len(ASSETS)} native Aseprite map assets selected by ASSETS.')


def check():
    """Verify native-source round trips, static sprite alpha and normalized floor contracts."""
    from pixelize import read_png

    manifest = json.loads((ART / 'manifest.json').read_text())
    entries = {entry['path']: entry for entry in manifest['files']}
    layout = json.loads((KIT / 'hd-sources/props-layout.json').read_text())
    baseline = json.loads(subprocess.check_output([
        'git', 'show', layout['baseline'] +
        ':project/levels/monster_mouth_escape/art_hd/manifest.json'
    ], cwd=ROOT))
    earlier = {entry['path']: entry for entry in baseline['files']}
    with tempfile.TemporaryDirectory(prefix='w276-check-') as temporary:
        target = pathlib.Path(temporary) / 'export.png'
        for destination in ASSETS:
            entry = entries[destination]
            source = source_path(destination)
            if hashlib.sha256(source.read_bytes()).hexdigest() != entry['source_sha256']:
                raise ValueError(f'{destination}: native source digest differs')
            run('aseprite', '--batch', source, '--save-as', target)
            if target.read_bytes() != (ART / destination).read_bytes():
                raise ValueError(f'{destination}: native source export differs')
        for destination, asset in layout['assets'].items():
            entry = entries[destination]
            width, height, rows = read_png(ART / destination)
            if [width, height] != asset['size']:
                raise ValueError(f'{destination}: canvas differs from recorded layout')
            occupied = [y + 1 for y, row in enumerate(rows) if any(pixel[3] for pixel in row)]
            if not occupied or any(pixel[3] not in (0, 255) for row in rows for pixel in row):
                raise ValueError(f'{destination}: empty sprite or translucent pixel')
            bottom = max(occupied)
            expected = max(part['bounds'][1] + part['bounds'][3] for part in asset['parts'])
            if abs(bottom - expected) > 1:
                raise ValueError(f'{destination}: visible bottom anchor moved')
            previous_png = pathlib.Path(temporary) / 'previous.png'
            previous_png.write_bytes(subprocess.check_output([
                'git', 'show', layout['baseline'] +
                ':project/levels/monster_mouth_escape/art_hd/' + destination
            ], cwd=ROOT))
            _, _, old_rows = read_png(previous_png)
            previous_bottom = max(y + 1 for y, row in enumerate(old_rows)
                                  if any(pixel[3] >= 128 for pixel in row))
            if abs(bottom - previous_bottom * 1.5) > 1:
                raise ValueError(f'{destination}: normalized historical floor anchor moved')
            old = earlier[destination]
            if abs(width - old['size'][0] * 1.5) > 0.5 or abs(height - old['size'][1] * 1.5) > 0.5:
                raise ValueError(f'{destination}: canvas no longer follows the 1.5x grid')
            for key in ('pivot', 'baseline_y'):
                if key in old:
                    values = zip(entry[key], old[key]) if key == 'pivot' else [(entry[key], old[key])]
                    if any(abs(new - previous * 1.5) > 0.5 for new, previous in values):
                        raise ValueError(f'{destination}: normalized {key} moved')
            if destination == 'walkway/aquarium_deck.png':
                if any(pixel[3] != 255 for pixel in rows[0]):
                    raise ValueError('aquarium deck: top contact row has a hole')
                if any(row[0] != row[-1] for row in rows):
                    raise ValueError('aquarium deck: outer repeat columns differ')
        for destination, band_height, band_count, rim in (
                ('walkway/deck.png', 126, 6, 18),
                ('walkway/aquarium_deck.png', 126, 1, 45)):
            previous_png = pathlib.Path(temporary) / 'contact-baseline.png'
            previous_png.write_bytes(subprocess.check_output([
                'git', 'show', '053e0dfb96409f85aa23f92d19425d6f5ccba909:'
                'project/levels/monster_mouth_escape/art_hd/' + destination
            ], cwd=ROOT))
            width, height, rows = read_png(ART / destination)
            old_width, old_height, old_rows = read_png(previous_png)
            if (width, height) != (old_width, old_height):
                raise ValueError(f'{destination}: polish changed the canvas')
            for band in range(band_count):
                start = band * band_height
                if rows[start:start + rim] != old_rows[start:start + rim]:
                    raise ValueError(f'{destination}: polish changed a walked contact rim')
            if any(pixel[3] != old[3] for row, old_row in zip(rows, old_rows)
                   for pixel, old in zip(row, old_row)):
                raise ValueError(f'{destination}: polish changed the alpha silhouette')
        for destination in ('base/feed_crane.png', 'endings/escaped.png'):
            entry = entries[destination]
            source = ROOT / entry['source']
            target = ART / destination
            if hashlib.sha256(source.read_bytes()).hexdigest() != entry['source_sha256']:
                raise ValueError(f'{destination}: generated source digest differs')
            if hashlib.sha256(target.read_bytes()).hexdigest() != entry['sha256']:
                raise ValueError(f'{destination}: generated export digest differs')
            width, height, _ = read_png(target)
            if [width, height] != entry['size']:
                raise ValueError(f'{destination}: generated canvas differs')
    print(f'Checked {len(ASSETS)} native PNG round trips and source digests; '
          f"{len(layout['assets'])} static canvases, hard-alpha sprites and floor anchors; "
          'container/hazard pivots and aquarium deck contact/repeat columns pass; '
          'polished deck rims/alpha match the assigned baseline; two generated asset digests pass.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepare', action='store_true', help='replace native sources from the recorded HD images')
    parser.add_argument('--prepare-backgrounds', action='store_true',
                        help='replace only the three native background sources')
    parser.add_argument('--prepare-props', action='store_true',
                        help='replace only the seventeen static prop, support and aquarium-deck sources')
    parser.add_argument('--check', action='store_true', help='check native exports and static floor contracts')
    args = parser.parse_args()
    if args.check:
        check()
        raise SystemExit(0)
    if args.prepare:
        prepare()
    elif args.prepare_backgrounds:
        prepare_backgrounds()
    elif args.prepare_props:
        prepare_props()
    export()
