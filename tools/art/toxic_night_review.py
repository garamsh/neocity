#!/usr/bin/env python3
"""Compose kit and reference boards from the recorded native runtime captures."""
import argparse
import pathlib
import shutil
import subprocess
import tempfile

from toxic_night import ART, ASSETS, KIT, ROOT

REFERENCE = '35a53ee444965e50eba276c823cbf8273ba47d53'
CAPTURES = {
    'street': ('230-street-day-1.png', 'street'),
    'cold-store': ('271-cold-store-day-1.png', 'building'),
    'service-room': ('271-service-room-day-1.png', 'building'),
    'kitchen': ('276-interior-kitchen.png', 'building'),
    'noodle': ('276-interior-noodle.png', 'building'),
}


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
    compose(parser.parse_args().captures)
