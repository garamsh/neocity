#!/usr/bin/env python3
"""Export the recorded Toxic Night character sheets with Pillow."""

from collections import deque
import hashlib
import io
import json
from pathlib import Path
import subprocess

from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parents[2]
PRODUCTION = ROOT / 'docs/art/production'
ART = ROOT / 'project/levels/monster_mouth_escape/art_hd/characters'
POSES = ('idle', 'walk_1', 'walk_2', 'walk_3', 'walk_4', 'run_1',
         'run_2', 'jump', 'fall', 'land', 'climb')
BASE = 'fc43577baa32db564805dd482ab04f29fa90a4f5'


def comparison():
    """Place identity references and old/new idles beside one another."""
    board = Image.new('RGBA', (1536, 1080), '#252335')
    draw = ImageDraw.Draw(board)
    font = ImageFont.load_default(size=21)
    draw.text((24, 16), 'TOXIC NIGHT / identity comparison / asset previews, not runtime captures', font=font)
    for x, label in ((24, 'Original identity sheet'), (390, 'First concept crop'),
                     (780, 'New idle / nearest 2x'), (1288, 'Replaced idle / 1x')):
        draw.text((x, 64), label, font=font)
    original = Image.open(ROOT / 'docs/art/concept-art/toxic-night/original-concept-v1.png')
    for actor, y, sheet_name, sheet_crop, concept_crop in (
            ('chef', 140, 'chef', (20, 20, 345, 465), (270, 345, 590, 635)),
            ('appraiser', 620, 'detective', (35, 45, 335, 425), (540, 480, 795, 715))):
        draw.text((24, y - 30), actor.upper(), font=font)
        sheet = Image.open(ROOT / f'docs/art/concept-art/characters/{sheet_name}-sheet-v1.png')
        board.paste(sheet.crop(sheet_crop), (24, y))
        board.paste(original.crop(concept_crop), (390, y + 60))
        new = Image.open(ART / f'{actor}_idle.png').resize((512, 448), Image.Resampling.NEAREST)
        board.alpha_composite(new, (760, y))
        old_path = str((ART / f'{actor}_idle.png').relative_to(ROOT))
        old = Image.open(io.BytesIO(subprocess.check_output(['git', 'show', f'{BASE}:{old_path}'], cwd=ROOT))).convert('RGBA')
        board.alpha_composite(old, (1280, y + 204))
        draw.text((780, y + 420), '172 px' if actor == 'chef' else '166 px / stooped', font=font)
        draw.text((1288, y + 420), '172 px' if actor == 'chef' else '108 px', font=font)
    board.convert('RGB').save(PRODUCTION / 'characters-toxic-night-comparison.png')


def largest_silhouette(alpha):
    """Keep the figure and discard disconnected generation debris."""
    pixels = set((x, y) for y in range(alpha.height) for x in range(alpha.width)
                 if alpha.getpixel((x, y)) >= 128)
    largest = set()
    while pixels:
        start = pixels.pop()
        component = {start}
        pending = deque([start])
        while pending:
            x, y = pending.popleft()
            for point in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
                if point in pixels:
                    pixels.remove(point)
                    component.add(point)
                    pending.append(point)
        if len(component) > len(largest):
            largest = component
    if not largest:
        raise ValueError('source cell has no figure')
    result = Image.new('L', alpha.size)
    for point in largest:
        result.putpixel(point, 255)
    return result


def main():
    manifest = json.loads((ART / 'manifest.json').read_text())
    manifest.update(version=2, issue='https://github.com/garamsh/neocity/issues/269',
                    provenance='docs/art/production/characters-toxic-night-provenance.txt',
                    status='Toxic Night identity redraw; creative acceptance pending')
    frames = []
    for actor, height in (('chef', 172), ('appraiser', 166)):
        source = PRODUCTION / f'characters-toxic-night-{actor}-source.png'
        sheet = Image.open(source).convert('RGBA')
        if sheet.size != (1536, 1024):
            raise ValueError(f'unexpected sheet size: {source}: {sheet.size}')
        cells = []
        crops = []
        for index, pose in enumerate(POSES):
            row, column = divmod(index, 4)
            # Chef's climbing hat extends above the nominal third-row boundary.
            top, bottom = ((0, 335), (342, 674), (675, 1024))[row] if actor == 'chef' else ((0, 341), (341, 682), (682, 1024))[row]
            crop = (column * 384, top, (column + 1) * 384, bottom)
            cell = sheet.crop(crop)
            mask = largest_silhouette(cell.getchannel('A'))
            if actor == 'appraiser':
                mask = mask.filter(ImageFilter.MinFilter(3))
            cell.putalpha(mask)
            cells.append(cell)
            crops.append(crop)
        idle_bounds = cells[0].getbbox()
        scale = height / (idle_bounds[3] - idle_bounds[1])
        strip = Image.new('RGBA', (256 * len(POSES), 224), '#252335')
        for index, (pose, cell, crop) in enumerate(zip(POSES, cells, crops)):
            bounds = cell.getbbox()
            figure = cell.crop(bounds)
            size = (round(figure.width * scale), round(figure.height * scale))
            figure = figure.resize(size, Image.Resampling.NEAREST)
            floor = {'jump': 186, 'fall': 196}.get(pose, 204)
            placement = (128 - size[0] // 2, floor - size[1])
            canvas = Image.new('RGBA', (256, 224))
            canvas.alpha_composite(figure, placement)
            target = ART / f'{actor}_{pose}.png'
            canvas.save(target)
            actual = canvas.getbbox()
            if actual != (placement[0], placement[1], placement[0] + size[0], floor):
                raise ValueError(f'clipped or empty frame: {target}')
            frames.append(dict(
                file=target.name, actor=actor, pose=pose, size=[256, 224],
                baseline_y=204, pivot=[128, 204],
                visible_bounds=[actual[2] - actual[0], actual[3] - actual[1], actual[0], actual[1]],
                lowest_pixel_exclusive_y=floor,
                source=str(source.relative_to(ROOT)),
                source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                source_crop=[crop[0], crop[1], crop[2] - crop[0], crop[3] - crop[1]],
                source_visible_bounds=list(bounds), source_scale=scale,
                resize_pixels=list(size), placement=list(placement),
                alpha_cleanup='threshold 128; largest 4-connected silhouette' +
                    ('; erode 1 source pixel to remove white matte fringe' if actor == 'appraiser' else ''),
                sha256=hashlib.sha256(target.read_bytes()).hexdigest()))
            strip.alpha_composite(canvas, (index * 256, 0))
        strip.save(PRODUCTION / f'characters-toxic-night-{actor}-motion.png')
    manifest['frames'] = frames
    (ART / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    comparison()
    print(f'Exported {len(frames)} PNGs, 2 preview strips and the character manifest.')


if __name__ == '__main__':
    main()
