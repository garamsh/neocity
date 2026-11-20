"""Export Toxic Night screen assets and build illustrative comparison boards."""

import hashlib
import io
import json
from pathlib import Path
import subprocess

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[4]
HERE = Path(__file__).resolve().parent
ART = ROOT / 'project/levels/monster_mouth_escape/art_hd'
BASE = '35a53ee444965e50eba276c823cbf8273ba47d53'
NEAREST = Image.Resampling.NEAREST
FONT = ROOT / 'project/levels/monster_mouth_escape/fonts/Tiny5-Regular.ttf'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def panel(image, box, accent='#8ceb5c'):
    """Illustrate the existing code-drawn kit, without claiming a capture."""
    draw = ImageDraw.Draw(image)
    x, y, right, bottom = box
    draw.rectangle(box, fill='#101634', outline='#4a5680', width=3)
    draw.rectangle((x + 7, y + 7, right - 7, bottom - 7), outline=accent)


def text(image, at, words, size=24, fill='#f3eee0'):
    draw = ImageDraw.Draw(image)
    draw.fontmode = '1'
    draw.text(at, words, font=ImageFont.truetype(str(FONT), size), fill=fill)


def paste(image, path, at, size=None):
    part = Image.open(path).convert('RGBA')
    if size:
        part = part.resize(size, NEAREST)
    image.alpha_composite(part, at)


def comparison(name, reference, preview, note):
    board = Image.new('RGBA', (3840, 1176), '#0b0b17')
    text(board, (24, 12), 'TOXIC NIGHT / selected direction reference', 32)
    text(board, (1944, 12), 'PRODUCTION ASSET STUDY / NOT A GAME CAPTURE', 32)
    paste(board, reference, (0, 64), (1920, 1080))
    board.alpha_composite(preview, (1920, 64))
    text(board, (24, 1148), note, 24)
    board.convert('RGB').save(HERE / f'{name}-comparison.png')


def main():
    manifest_path = ART / 'manifest.json'
    manifest = json.loads(manifest_path.read_text())
    for name, target in (('title', 'title/key_art.png'), ('base', 'base/kitchen.png')):
        source = HERE / f'{name}-source.png'
        image = Image.open(source).convert('RGBA')
        if name == 'base':
            # Width-based export aligns the generated floor with the runtime floor.
            height = round(image.height * 1920 / image.width)
            exported = Image.new('RGBA', (1920, 1080), '#090a14')
            exported.alpha_composite(image.resize((1920, height), NEAREST))
            processing = f'nearest 1920x{height}; dark bottom padding to 1920x1080'
        else:
            exported = image.resize((1920, 1080), NEAREST)
            processing = 'nearest 1920x1080'
        output = ART / target
        exported.save(output)
        record = dict(path=target, size=[1920, 1080], issue=274,
                      role='Toxic Night title with embedded logo' if name == 'title' else
                           'Toxic Night kitchen without characters, crane or interface',
                      sha256=digest(output), source=str(source.relative_to(ROOT)),
                      source_sha256=digest(source), crop=None, resize=processing,
                      provenance='docs/art/production/toxic-night-screens/provenance.txt')
        for index, entry in enumerate(manifest['files']):
            if entry['path'] == target:
                manifest['files'][index] = record
                break
        else:
            manifest['files'].append(record)
    manifest['system_assets']['hud_icons'] = {
        'issue': 274, 'consumer': 'src/hud/survival.cpp',
        'status': 'HD item icons connected in HUD; world source icons unchanged',
        'canvas_pixels': 84, 'layout_units': 56, 'sampling': 'nearest'}
    manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')

    title = Image.open(ART / 'title/key_art.png').convert('RGBA')
    for actor, x in (('chef', 135), ('appraiser', 390)):
        paste(title, ART / f'characters/{actor}_idle.png', (x, 645))
    panel(title, (1155, 210, 1875, 876))
    for i, label in enumerate(('NEW GAME', 'CONTROLS', 'HOW TO PLAY', 'QUIT', 'SETTINGS')):
        y = 285 + i * 111
        panel(title, (1191, y, 1839, y + 99), '#b0f431' if i == 0 else '#4a5680')
        text(title, (1299, y + 24), label, 36, '#b0f431' if i == 0 else '#f3eee0')
    comparison('title', ROOT / 'docs/art/concept-art/toxic-night/f-street.png', title,
               'Reference establishes atmosphere; menu/characters are an illustrative composition of production assets.')

    base = Image.open(ART / 'base/kitchen.png').convert('RGBA')
    for actor, x in (('appraiser', 225), ('chef', 780)):
        paste(base, ART / f'characters/{actor}_idle.png', (x, 588))
    draw = ImageDraw.Draw(base)
    draw.rectangle((1245, 225, 1905, 243), fill='#464156')
    draw.rectangle((1245, 243, 1905, 245), fill='#8ceb5c')
    draw.rectangle((1482, 243, 1518, 258), fill='#464156')
    draw.rectangle((1498, 258, 1501, 285), fill='#464156')
    paste(base, ART / 'icons/dish_fish_stew.png', (1458, 303))
    for box, label in (((12, 12, 300, 84), 'DAY 02'),
                       ((768, 12, 1152, 84), 'ACID 62%'),
                       ((1668, 12, 1908, 84), '04:59')):
        panel(base, box)
        text(base, (box[0] + 26, box[1] + 16), label, 36)
    panel(base, (30, 180, 1150, 234))
    text(base, (51, 190), 'TIP   Cook an identified ingredient into food.', 24)
    panel(base, (30, 243, 1230, 369))
    text(base, (51, 258), 'Day 1: Fish Stew. The monster liked it.', 24)
    text(base, (51, 291), 'New food + ingredients + taste + fine = 90, needs 60', 24)
    text(base, (51, 324), 'Acid 70 to 62. Map changes: 0', 24)
    for x, right, label in ((30, 636, 'APPRAISE'), (660, 1257, 'COOK'), (1278, 1890, 'FEED')):
        panel(base, (x, 834, right, 1056), '#ff5cbe' if label == 'FEED' else '#8ceb5c')
        text(base, (x + 30, 849), label, 36)
    for x, item, quantity in ((54, 'mushroom', '3'), (138, 'tomato', '2'),
                              (684, 'meat', '2'), (768, 'fish', '1'), (852, 'kelp', '4')):
        panel(base, (x, 894, x + 78, 972), '#4a5680')
        paste(base, ART / f'icons/ingredient_{item}.png', (x - 3, 891))
        ImageDraw.Draw(base).rectangle((x + 51, 945, x + 75, 969), fill='#080a18')
        text(base, (x + 55, 943), quantity, 24)
    text(base, (948, 898), 'Fish Stew', 24)
    text(base, (948, 934), 'READY RECIPES 3/11', 24)
    paste(base, ART / 'icons/dish_fish_stew.png', (1302, 894))
    panel(base, (1668, 894, 1866, 978))
    text(base, (1702, 912), 'SERVE', 36)
    comparison('base', ROOT / 'docs/art/concept-art/toxic-night/f-base.png', base,
               'Sample day-two overlay; all values are illustrative. Runtime capture and interactions remain PM bundle checks.')

    hud = Image.new('RGBA', (1920, 1080), '#0b0b17')
    text(hud, (48, 32), 'HUD / HD object icons and quantity badges', 48)
    for i, file in enumerate(sorted((ART / 'icons').glob('*.png'))):
        x, y = 48 + i % 9 * 200, 160 + i // 9 * 190
        panel(hud, (x, y, x + 126, y + 126))
        paste(hud, file, (x + 21, y + 21))
        ImageDraw.Draw(hud).rectangle((x + 92, y + 90, x + 119, y + 119), fill='#080a18')
        text(hud, (x + 99, y + 89), '2', 24)
    for i, file in enumerate(('sun.png', 'clock.png')):
        paste(hud, ART / f'ui/{file}', (48 + i * 120, 620))
    text(hud, (48, 740), 'APPRAISE: available quantity; COOK: available, or 1 per occupied cooking slot', 32)
    text(hud, (48, 804), 'TRANSFER: one item per slot; wN remains its weight; ? remains unidentified', 32)
    comparison('hud', ROOT / 'docs/art/concept-art/toxic-night/f-base.png', hud,
               'Existing HD icons newly connected in HUD; samples show quantity 2. No gameplay stacking is introduced.')

    for name, target in (('title', 'title/key_art.png'), ('base', 'base/kitchen.png')):
        old = Image.open(io.BytesIO(subprocess.check_output(
            ['git', 'show', f'{BASE}:{(ART / target).relative_to(ROOT)}'], cwd=ROOT))).convert('RGBA')
        board = Image.new('RGB', (3840, 1144), '#0b0b17')
        board.paste(old.resize((1920, 1080), NEAREST), (0, 64))
        board.paste(Image.open(ART / target), (1920, 64))
        text(board, (24, 12), 'REPLACED ASSET', 32)
        text(board, (1944, 12), 'TOXIC NIGHT REPLACEMENT / NOT A GAME CAPTURE', 32)
        board.save(HERE / f'{name}-before-after.png')
    print('Exported 2 backgrounds, updated manifest and wrote 5 asset comparison boards.')


if __name__ == '__main__':
    main()
