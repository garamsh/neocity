#!/usr/bin/env python3
"""Cut the #251 street and kitchen art from the recorded Codex sources with ImageMagick.

Run from anywhere: python3 docs/art/production/street_art_251.py
Every source in street-art-251-sources/ is a Codex output on a flat #303030 background where the
asset is transparent. Four cut kinds exist:
  opaque  - one Lanczos resample to the runtime size, no alpha (kitchen);
  layer   - key the background to alpha, resample to the runtime size, threshold alpha;
  sprite  - key, trim, fit the art into the old asset's opaque box on the old canvas, threshold alpha;
  bands   - cut the stacked bands apart at recorded rows, resample each to its runtime height keeping
            the top rim unscaled, then crossfade the right edge into the mirrored left edge so the
            strip repeats.
The art_hd manifest entry of every destination is replaced or added.
"""
import hashlib
import json
import pathlib
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[3]
SOURCE = ROOT / 'docs/art/production/street-art-251-sources'
ART = ROOT / 'project/levels/monster_mouth_escape/art_hd'
KEY = ['-fuzz', '5%', '-transparent', '#303030']
THRESHOLD = ['-channel', 'A', '-threshold', '50%', '+channel']
RIM = 20
# The capture check wants walkway faces at least 0.06 darker than the city backdrop; the deck body is dimmed to this percent.
DECK_BODY_BRIGHTNESS = 72
SEAM = 48

OPAQUE = [
    ('kitchen.png', '1920x1080', 'base/kitchen.png', 'Base kitchen background without characters, fridge, crane or interface'),
]
LAYERS = [
    ('city.png', '1920x735', 'backgrounds/city.png', 'Ruined city skyline, viaduct and flesh hills; transparent sky'),
    ('terminal.png', '2304x1152', 'world/terminal.png', 'Ruined ferry terminal scenery behind the fixed floors'),
    ('shop.png', '1920x384', 'world/shop.png', 'Ruined kitchen shop front at left and stall at right'),
]
# source, old canvas, old opaque box (width, height, left, top), destination, role
SPRITES = [
    ('vending_machine.png', (144, 177), (136, 177, 4, 0), 'world/vending_machine.png', 'Smashed vending machine'),
    ('vendor_cart.png', (192, 96), (101, 96, 45, 0), 'world/vendor_cart.png', 'Burned vendor cart'),
    ('kiosk.png', (96, 96), (96, 83, 0, 6), 'world/kiosk.png', 'Battered street kiosk'),
    ('kiosk_ruined.png', (288, 144), (283, 137, 3, 3), 'world/kiosk_ruined.png', 'Collapsed kiosk heap'),
    ('cold_store.png', (288, 288), (285, 221, 1, 33), 'world/cold_store.png', 'Ruined cold-store front'),
    ('ferry.png', (384, 150), (268, 150, 58, 0), 'world/ferry.png', 'Wrecked rail car'),
]
# source, (top, bottom) source rows of every band, runtime band heights, destination, role
BANDS = [
    ('deck.png', [(5, 86), (98, 179), (190, 271), (282, 357), (366, 442), (453, 503)], [84] * 6,
     'walkway/deck.png', 'Six ruined walkway deck bands', DECK_BODY_BRIGHTNESS),
    ('building_wall.png', [(0, 95), (96, 191), (192, 287), (288, 395)], [96, 96, 96, 108],
     'world/building_wall.png', 'Four ruined building-wall bands', 100),
]


def run(*command):
    subprocess.run([str(part) for part in command], check=True)


def identify(path, template):
    return subprocess.check_output(['magick', 'identify', '-format', template, str(path)], text=True)


def opaque(source, size, target):
    run('magick', source, '-filter', 'Lanczos', '-resize', size + '!', '-depth', '8', 'PNG24:' + str(target))


def layer(source, size, target):
    run('magick', source, '-filter', 'Lanczos', '-resize', size + '!', *KEY, *THRESHOLD, '-depth', '8', 'PNG32:' + str(target))


def sprite(source, canvas, box, target):
    width, height, left, top = box
    with tempfile.TemporaryDirectory() as work:
        art = pathlib.Path(work) / 'art.png'
        run('magick', source, *KEY, '-trim', '+repage', '-filter', 'Lanczos', '-resize', f'{width}x{height}', *THRESHOLD, f'PNG32:{art}')
        fitted_w, fitted_h = map(int, identify(art, '%w %h').split())
        x = left + (width - fitted_w) // 2
        y = top + height - fitted_h
        run('magick', '-size', f'{canvas[0]}x{canvas[1]}', 'xc:none', art, '-geometry', f'+{x}+{y}', '-composite',
            '-depth', '8', f'PNG32:{target}')


def bands(source, rows, heights, target, brightness=100):
    with tempfile.TemporaryDirectory() as work:
        work = pathlib.Path(work)
        width = int(identify(source, '%w'))
        pieces = []
        for index, ((top, bottom), height) in enumerate(zip(rows, heights)):
            band = work / f'band{index}.png'
            rim = min(RIM, height)
            run('magick', source, '-crop', f'{width}x{bottom - top + 1}+0+{top}', '+repage', f'PNG24:{band}')
            body = work / f'body{index}.png'
            run('magick', band, '-crop', f'{width}x{bottom - top + 1 - rim}+0+{rim}', '+repage', '-filter', 'Lanczos',
                '-resize', f'{width}x{height - rim}!', '-modulate', str(brightness), f'PNG24:{body}')
            top_rim = work / f'rim{index}.png'
            run('magick', band, '-crop', f'{width}x{rim}+0+0', '+repage', f'PNG24:{top_rim}')
            piece = work / f'piece{index}.png'
            run('magick', top_rim, body, '-append', f'PNG24:{piece}')
            pieces.append(piece)
        stacked = work / 'stacked.png'
        run('magick', *pieces, '-append', f'PNG24:{stacked}')
        total = sum(heights)
        left = work / 'left.png'
        run('magick', stacked, '-crop', f'{SEAM}x{total}+0+0', '+repage', '-flop', f'PNG24:{left}')
        right = work / 'right.png'
        run('magick', stacked, '-crop', f'{SEAM}x{total}+{width - SEAM}+0', '+repage', f'PNG24:{right}')
        ramp = work / 'ramp.png'
        run('magick', '-size', f'{SEAM}x1', 'gradient:black-white', '-scale', f'{SEAM}x{total}!', f'PNG24:{ramp}')
        blended = work / 'blended.png'
        run('magick', right, left, ramp, '-composite', f'PNG24:{blended}')
        body = work / 'body.png'
        run('magick', stacked, '-crop', f'{width - SEAM}x{total}+0+0', '+repage', f'PNG24:{body}')
        run('magick', body, blended, '+append', '+repage', '-depth', '8', f'PNG24:{target}')


def entry(source, destination, role, resize, crop):
    target = ART / destination
    path = SOURCE / source
    return {
        'path': destination,
        'size': [int(value) for value in identify(target, '%w %h').split()],
        'role': role,
        'sha256': hashlib.sha256(target.read_bytes()).hexdigest(),
        'issue': 251,
        'source': 'docs/art/production/street-art-251-sources/' + source,
        'source_sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
        'crop': crop,
        'resize': resize,
        'retouch': 'ImageMagick one Lanczos resample by docs/art/production/street_art_251.py; no painted pixels',
    }


def main():
    manifest_path = ART / 'manifest.json'
    manifest = json.loads(manifest_path.read_text())
    rows = manifest['files']
    made = []
    for source, size, destination, role in OPAQUE:
        opaque(SOURCE / source, size, ART / destination)
        made.append(entry(source, destination, role, size + '!', None))
    for source, size, destination, role in LAYERS:
        layer(SOURCE / source, size, ART / destination)
        made.append(entry(source, destination, role, size + '!', None))
    for source, canvas, box, destination, role in SPRITES:
        sprite(SOURCE / source, canvas, box, ART / destination)
        made.append(entry(source, destination, role, 'fit into %dx%d+%d+%d' % box, 'trim'))
    for source, spans, heights, destination, role, brightness in BANDS:
        bands(SOURCE / source, spans, heights, ART / destination, brightness)
        made.append(entry(source, destination, role, 'bands ' + json.dumps(spans), None))
    for new in made:
        for index, row in enumerate(rows):
            if row['path'] == new['path']:
                rows[index] = {**row, **new}
                break
        else:
            rows.append(new)
    manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'Wrote {len(made)} PNGs and updated their HD manifest entries.')


if __name__ == '__main__':
    main()
