"""Export the shelter layers and compose their review evidence without a display."""

import hashlib
import io
import json
from pathlib import Path
import subprocess

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[4]
HERE = Path(__file__).resolve().parent
ART = ROOT / 'project/levels/monster_mouth_escape/art_hd'
BASE = '2ebcc10375ef1e730df6693865ce6a63fad2d743'
NEAREST = Image.Resampling.NEAREST


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def sprite(path):
    image = Image.open(path).convert('RGBA')
    image.putalpha(image.getchannel('A').point(lambda value: 255 if value >= 128 else 0))
    return image


def export():
    manifest_path = ART / 'manifest.json'
    manifest = json.loads(manifest_path.read_text())
    records = {entry['path']: entry for entry in manifest['files']}

    def save(name, image, source, role, processing):
        target = ART / 'base' / name
        image.save(target)
        records['base/' + name] = {
            'path': 'base/' + name, 'size': list(image.size), 'issue': 268,
            'role': role, 'source': str((HERE / source).relative_to(ROOT)),
            'source_sha256': digest(HERE / source), 'sha256': digest(target),
            'export': 'python3 docs/art/production/base-shelter-268/export.py',
            'retouch': processing,
            'provenance': 'docs/art/production/base-shelter-268/README.md',
        }

    background = Image.open(HERE / 'background-source.png').convert('RGBA') \
        .resize((1920, 1104), NEAREST).crop((0, 0, 1920, 1080))
    save('kitchen.png', background, 'background-source.png',
         'Shelter under the teeth; no characters, foreground counter or crane',
         'nearest 1920x1104; crop bottom 24px to align the runtime floor without padding')

    front_source = sprite(HERE / 'front-source.png')
    front = Image.new('RGBA', (1920, 1080))
    front.alpha_composite(front_source.crop((0, 628, 1435, 755))
                          .resize((1490, 80), NEAREST), (0, 712))
    front.alpha_composite(front_source.crop((0, 755, 1435, 845))
                          .resize((1490, 90), NEAREST), (0, 792))
    save('kitchen_front.png', front, 'front-source.png',
         'Transparent near workbenches, produce crates and platform fascia; draw after actors',
         'binary alpha; two nearest crops align worktop and fascia to the 792px floor')

    crane = sprite(HERE / 'crane-source.png')
    # The reviewed source separates the mast, vertical cable and suspended cradle.
    crane = crane.resize((660, 642), NEAREST)
    fixed = crane.copy()
    fixed.paste((0, 0, 0, 0), (200, 125, 660, 642))
    save('feed_crane.png', fixed, 'crane-source.png',
         'Fixed hoist and mast; canvas placement 1240,150; engine connection pending',
         'nearest 660x642; binary alpha; remove moving cable and cradle region')

    cable = crane.crop((410, 125, 442, 355))
    save('feed_crane_cable.png', cable, 'crane-source.png',
         'Vertical cable; extend downward from canvas 1650,275 to cradle ring',
         'nearest 660x642; binary alpha; cable crop 410,125,442,355')
    cradle = crane.crop((290, 355, 570, 575))
    save('feed_crane_cradle.png', cradle, 'crane-source.png',
         'Empty triangular cradle; rest canvas placement 1530,505; translate on feeding',
         'nearest 660x642; binary alpha; cradle crop 290,355,570,575')

    manifest['files'] = list(records.values())
    manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')
    compose(background, front, fixed, cable, cradle)


def compose(background, front, fixed, cable, cradle):
    def actors(image):
        for actor, x in (('appraiser', 225), ('chef', 780)):
            image.alpha_composite(Image.open(ART / f'characters/{actor}_idle.png')
                                  .convert('RGBA'), (x, 588))

    after = background.copy()
    actors(after)
    after.alpha_composite(front)
    after.alpha_composite(fixed, (1240, 150))
    after.alpha_composite(cable, (1650, 275))
    after.alpha_composite(cradle, (1530, 505))
    after.save(HERE / 'composite.png')
    before = Image.open(io.BytesIO(subprocess.check_output([
        'git', 'show', BASE + ':project/levels/monster_mouth_escape/art_hd/base/kitchen.png'
    ], cwd=ROOT))).convert('RGBA')
    actors(before)
    draw = ImageDraw.Draw(before)
    draw.rectangle((1245, 225, 1905, 243), fill='#464156')
    draw.rectangle((1245, 243, 1905, 245), fill='#8ceb5c')
    draw.rectangle((1482, 243, 1518, 258), fill='#464156')
    draw.rectangle((1498, 258, 1501, 285), fill='#464156')
    draw.rectangle((1491, 285, 1509, 303), fill='#464156')
    board = Image.new('RGB', (2880, 650), '#10101b')
    font = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', 21)
    draw = ImageDraw.Draw(board)
    approved = Image.new('RGBA', (1920, 1080), '#10101b')
    approved.alpha_composite(Image.open(HERE / 'approved-a.png').convert('RGBA')
                             .crop((0, 116, 1920, 820)), (0, 116))
    panels = [before, approved, after]
    labels = ['OLD BASE / runtime sprites + reconstructed crane',
              'APPROVED A / background only; original mockup figures',
              'NEW LAYERS / actual runtime sprites and foot positions']
    for index, (panel, label) in enumerate(zip(panels, labels)):
        board.paste(panel.convert('RGB').resize((960, 540), NEAREST), (index * 960, 50))
        draw.text((index * 960 + 14, 15), label, font=font, fill='#f1dfc7')
    draw.text((14, 610), 'ART COMPOSITION, NOT A GAME CAPTURE. Feet: y792; sprites: x225 / x780, y588. Engine overlay/crane wiring pending.',
              font=font, fill='#c9bfd0')
    board.save(HERE / 'before-after.png')


if __name__ == '__main__':
    export()
