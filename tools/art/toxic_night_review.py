#!/usr/bin/env python3
"""Compose kit and reference boards from the recorded native runtime captures."""
import argparse
import json
import pathlib
import shutil
import subprocess
import tempfile

from toxic_night import ART, ASSETS, KIT, ROOT, run

REFERENCE = '35a53ee444965e50eba276c823cbf8273ba47d53'
BACKGROUND_BASELINE = 'b47f41b7a53f784d0f05371d0bf9b020bf20ccf8'
PROPS_BASELINE = 'bf1ad36459b4f9632560f41d29d00ba2832af21d'
CAPTURES = {
    'street': ('230-street-day-1.png', 'street'),
    'cold-store': ('271-cold-store-day-1.png', 'building'),
    'service-room': ('271-service-room-day-1.png', 'building'),
    'kitchen': ('276-interior-kitchen.png', 'building'),
    'noodle': ('276-interior-noodle.png', 'building'),
}


def compose_polish():
    """Show #262 native edits against the assigned base, without a game display."""
    baseline = '053e0dfb96409f85aa23f92d19425d6f5ccba909'
    output = ROOT / 'docs/art/production/art-polish-262'
    with tempfile.TemporaryDirectory(prefix='w262a-review-') as temporary:
        work = pathlib.Path(temporary)
        groups = {
            '5-decks': [('walkway/deck.png', '864x126+0+0'),
                        ('walkway/aquarium_deck.png', '576x126+0+0')],
            '6-backgrounds': [('backgrounds/throat.png', '680x420+700+100'),
                              ('backgrounds/acid.png', '864x350+200+350'),
                              ('backgrounds/aquarium.png', '864x640+900+300')],
            '9-props': [('walkway/deck.png', '200x108+0+144'),
                        ('walkway/deck.png', '200x108+2680+522')],
        }
        for item, crops in groups.items():
            command = ['magick', 'montage', '-font', 'DejaVu-Sans', '-pointsize', '18',
                       '-fill', '#ded7ed', '-background', '#100e20']
            for index, (path, box) in enumerate(crops):
                old = work / 'old.png'
                old.write_bytes(subprocess.check_output([
                    'git', 'show', baseline + ':project/levels/monster_mouth_escape/art_hd/' + path
                ], cwd=ROOT))
                for label, source in [('before', old), ('after', ART / path)]:
                    target = work / f'{item}-{index}-{label}.png'
                    command_crop = ['magick', source, '-crop', box, '+repage']
                    if item == '9-props':
                        command_crop += ['-filter', 'Point', '-resize', '300%']
                    run(*command_crop, target)
                    command += ['-label', f'{path} / {label}', str(target)]
            geometry = {'5-decks': '864x160>+12+28',
                        '6-backgrounds': '864x640>+12+28',
                        '9-props': '600x324+12+28'}[item]
            run(*command, '-filter', 'Point', '-geometry', geometry,
                '-tile', '2x', '-strip', output / (item + '-before-after.png'))
        kitchen = ART / 'base/kitchen.png'
        before = work / 'crane-before.png'
        after = work / 'crane-after.png'
        run('magick', kitchen, '-crop', '660x224+1245+225', '+repage',
            '-fill', '#464156', '-draw',
            'rectangle 0,0 659,17 rectangle 237,18 272,32 rectangle 253,33 255,59 '
            'rectangle 246,60 263,77', '-fill', '#8ceb5c', '-draw',
            'rectangle 0,18 659,20 rectangle 246,75 257,77', before)
        run('magick', kitchen, '-crop', '660x224+1245+225', '+repage',
            ART / 'base/feed_crane.png', '-composite', after)
        run('magick', 'montage', '-font', 'DejaVu-Sans', '-pointsize', '18',
            '-fill', '#ded7ed', '-background', '#100e20',
            '-label', 'before / code shapes reconstructed at rest', before,
            '-label', 'after / proposed sprite placement, engine pending', after,
            '-geometry', '+12+28', '-tile', '2x1', '-strip',
            output / '7-crane-before-after.png')


def compose_hd():
    """Compare the HD exports with the direction without implying a runtime capture."""
    with tempfile.TemporaryDirectory(prefix='w276-hd-review-') as temporary:
        work = pathlib.Path(temporary)
        references = {}
        for name in ('street', 'building'):
            path = work / (name + '.png')
            path.write_bytes(subprocess.check_output([
                'git', 'show', REFERENCE + ':docs/art/concept-art/toxic-night/f-' + name + '.png'
            ], cwd=ROOT))
            references[name] = path

        def montage(paths, target, geometry, tile):
            command = ['magick', 'montage', '-font', 'DejaVu-Sans', '-pointsize', '20',
                       '-fill', '#ded7ed', '-background', '#100e20']
            for label, path in paths:
                command += ['-label', label, str(path)]
            run(*command, '-filter', 'Point', '-geometry', geometry, '-tile', tile,
                '-strip', target)

        montage([(path, ART / path) for path in ASSETS], KIT / 'hd-kit-board.png',
                '640x288>+12+28', '3x')
        groups = {
            'containers': ('building', [path for path in ASSETS if path.startswith('containers/')]),
            'hazards-deck': ('street', ['walkway/aquarium_deck.png', 'world/span_wreck.png',
                                      'obstacles/acid_vent.png', 'obstacles/flesh_spikes.png',
                                      'ground/rubble.png']),
            'supports-props': ('street', ['walkway/structure.png', 'world/ladder_broken.png',
                                        'world/door_broken.png', 'world/noodle_vendor.png',
                                        'world/bag_washed_up.png', 'world/acid_drip.png']),
        }
        for name, (reference, paths) in groups.items():
            pieces = work / (name + '-pieces.png')
            montage([(pathlib.Path(path).name, ART / path) for path in paths],
                    pieces, '288x224+12+18', '3x2')
            montage([('Toxic Night F-' + reference + ' reference', references[reference]),
                     ('144px-cell exports / offline asset board', pieces)],
                    KIT / ('hd-' + name + '-comparison.png'), '960x576+0+12', '2x1')
        layout = json.loads((KIT / 'hd-sources/props-layout.json').read_text())
        pairs = []
        for path in layout['assets']:
            old = work / (path.replace('/', '-') + '-old.png')
            old.write_bytes(subprocess.check_output([
                'git', 'show', PROPS_BASELINE +
                ':project/levels/monster_mouth_escape/art_hd/' + path
            ], cwd=ROOT))
            normalized = work / (path.replace('/', '-') + '-normalized.png')
            size = layout['assets'][path]['size']
            run('magick', old, '-filter', 'Point', '-resize',
                'x'.join(map(str, size)) + '!', normalized)
            pairs.extend([(path + ' / earlier 1.5x', normalized),
                          (path + ' / 144px redraw', ART / path)])
        montage(pairs, KIT / 'hd-props-before-after.png', '576x432>+12+24', '2x')
        structure = work / 'walkway-structure.png-normalized.png'
        details = []
        for name, box in [('lamp', '180x270+1095+15'),
                          ('pier', '144x330+126+390'),
                          ('gauge', '288x396+2325+405')]:
            for label, path in [('earlier 1.5x', structure),
                                ('144px redraw', ART / 'walkway/structure.png')]:
                detail = work / (name + label + '.png')
                run('magick', path, '-crop', box, '+repage', detail)
                details.append((name + ' / ' + label, detail))
        montage(details, KIT / 'hd-supports-detail.png', '288x396+12+24', '2x3')
        backgrounds = []
        details = []
        for name, center in [('throat', (0.7, 0.25)), ('acid', (0.26, 0.65)),
                             ('aquarium', (0.52, 0.6))]:
            old = work / (name + '-old.png')
            old.write_bytes(subprocess.check_output([
                'git', 'show', BACKGROUND_BASELINE +
                ':project/levels/monster_mouth_escape/art_hd/backgrounds/' + name + '.png'
            ], cwd=ROOT))
            new = ART / 'backgrounds' / (name + '.png')
            backgrounds += [(name + ' / earlier export', old),
                            (name + ' / 144px-cell redraw', new)]
            for label, path, side in [('old 96px to 144px', old, 96),
                                       ('new native 144px', new, 144)]:
                width, height = map(int, subprocess.check_output(
                    ['magick', 'identify', '-format', '%w %h', str(path)], text=True).split())
                x, y = (round(center[0] * width - side / 2),
                        round(center[1] * height - side / 2))
                detail = work / (name + '-' + str(side) + '.png')
                run('magick', path, '-crop', f'{side}x{side}+{x}+{y}', '+repage',
                    '-filter', 'Point', '-resize', '432x432!', detail)
                details.append((name + ' / ' + label + ' / 3x', detail))
        montage(backgrounds, KIT / 'hd-backgrounds-comparison.png',
                '960x540>+12+28', '2x3')
        montage(details, KIT / 'hd-backgrounds-detail.png', '432x432+12+28', '2x3')
        rooms = [(name, ART / f'world/interior_{name}.png')
                 for name in ('kitchen', 'noodle', 'cold_store', 'service')]
        room_board = work / 'rooms.png'
        montage(rooms, room_board, '864x432+24+24', '2x2')
        reference = work / 'reference.png'
        montage([('Toxic Night F-building reference / not a floor layout', references['building'])],
                reference, '1824x1026+0+12', '1x1')
        run('magick', reference, room_board, '-background', '#100e20',
            '-gravity', 'center', '-append', '-strip', KIT / 'hd-interiors-comparison.png')

        city = work / 'city-layer.png'
        run('magick', ART / 'backgrounds/city.png', '-filter', 'Point', '-resize', '1920x735!', city)
        street = work / 'street-board.png'
        throat = work / 'throat-layer.png'
        acid = work / 'acid-layer.png'
        run('magick', ART / 'backgrounds/throat.png', '-filter', 'Point',
            '-resize', '1920x1309!', throat)
        run('magick', ART / 'backgrounds/acid.png', '-filter', 'Point',
            '-resize', '1920x507!', acid)
        run('magick', '-size', '1920x1080', 'xc:#100e20', throat,
            '-geometry', '+0+0', '-composite', city,
            '-geometry', '+0+200', '-composite', street)
        supports = work / 'supports-layer.png'
        run('magick', ART / 'walkway/structure.png', '-filter', 'Point',
            '-resize', '1920x549!', supports)
        run('magick', street, acid, '-geometry', '+0+820', '-composite',
            supports, '-geometry', '+0+292', '-composite', street)
        placements = [
            ('world/shop.png', '864x288+0+0', '768x256!', 0, 134),
            ('walkway/deck.png', '2880x126+0+0', '1920x84!', 0, 700),
            ('walkway/deck.png', '1152x126+0+126', '768x84!', 0, 390),
            ('walkway/deck.png', '864x126+0+252', '576x84!', 1344, 220),
            ('world/vending_machine.png', None, '144x177!', 1100, 523),
            ('world/kiosk.png', None, '96x96!', 350, 604),
            ('world/cold_store.png', None, '288x288!', 1450, 412),
            ('world/vendor_cart.png', None, '192x96!', 700, 604),
        ]
        for path, crop, size, x, y in placements:
            module = work / 'module.png'
            command = ['magick', ART / path]
            if crop:
                command += ['-crop', crop, '+repage']
            run(*command, '-filter', 'Point', '-resize', size, module)
            run('magick', street, module, '-geometry', f'+{x}+{y}', '-composite', street)
        montage([('Toxic Night F-street reference', references['street']),
                 ('144px-cell static map kit / offline art composition', street)],
                KIT / 'hd-street-comparison.png', '960x540+0+12', '2x1')
    print('Composed 10 HD boards: kit, interior, backgrounds, background detail, street, '
          'three static-piece reference boards, support details and 17 before/after pairs; '
          'no runtime capture used.')


def compose(captures):
    """Preserve native captures and compare them at half size with the direction."""
    stored = KIT / 'captures'
    stored.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='w276-review-') as temporary:
        work = pathlib.Path(temporary)
        references = {}
        for name in ('street', 'building'):
            path = work / (name + '.png')
            path.write_bytes(subprocess.check_output([
                'git', 'show', REFERENCE + ':docs/art/concept-art/toxic-night/f-' + name + '.png'
            ], cwd=ROOT))
            references[name] = path
        for name, (filename, reference) in CAPTURES.items():
            source = captures / filename
            target = stored / filename
            if source.resolve() != target.resolve():
                shutil.copyfile(source, target)
            subprocess.run([
                'magick', 'montage', '-font', 'DejaVu-Sans', '-pointsize', '20',
                '-fill', '#ded7ed', '-background', '#100e20',
                '-label', 'Toxic Night reference / F-' + reference, str(references[reference]),
                '-label', 'Runtime / ' + name, str(target),
                '-filter', 'Point', '-geometry', '960x540!+0+12', '-tile', '2x1',
                '-strip',
                str(KIT / (name + '-comparison.png'))
            ], check=True)
        command = ['magick', 'montage', '-font', 'DejaVu-Sans', '-pointsize', '16',
                   '-fill', '#ded7ed', '-background', '#100e20']
        for destination in ASSETS:
            command += ['-label', destination, str(ART / destination)]
        command += ['-filter', 'Point', '-geometry', '480x224>+12+28', '-tile', '3x', '-strip',
                    str(KIT / 'kit-board.png')]
        subprocess.run(command, check=True)
    print(f'Composed {len(CAPTURES)} reference comparisons and {len(ASSETS)} kit previews; '
          'selected by CAPTURES and toxic_night.ASSETS.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--captures', type=pathlib.Path, default=KIT / 'captures',
                        help='native StairsCheck output directory')
    parser.add_argument('--hd', action='store_true', help='compose current HD asset comparisons without a display')
    parser.add_argument('--polish', action='store_true', help='compose #262 offline before/after crops')
    args = parser.parse_args()
    if args.polish:
        compose_polish()
        raise SystemExit(0)
    if args.hd:
        compose_hd()
    else:
        compose(args.captures)
