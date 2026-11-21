"""Regression checks for Toxic Night export source ownership."""
import json
import pathlib
import tempfile
import unittest
from unittest.mock import patch

import toxic_night


class ToxicNightTest(unittest.TestCase):
    def test_exports_owned_and_new_sources_and_preserves_other_owners(self):
        with tempfile.TemporaryDirectory(prefix='w268b-source-owner-') as temporary:
            root = pathlib.Path(temporary)
            art = root / 'project/levels/monster_mouth_escape/art_hd'
            kit = root / 'docs/art/production/toxic-night-kit'
            (kit / 'hd-sources').mkdir(parents=True)
            (kit / 'hd-sources/props-layout.json').write_text('{"assets": {}}')
            for folder in ('world', 'base', 'endings'):
                (art / folder).mkdir(parents=True)
            assets = {'world/terminal.png': 'Replaced native source',
                      'world/shop.png': 'Owned native source',
                      'world/kiosk.png': 'New native source'}
            for destination in assets:
                (kit / (destination.replace('/', '-')[:-4] + '.aseprite')).write_bytes(b'native source')
            for source in ('endings/escaped.png', 'art-polish-262/crane-source.png'):
                path = root / 'docs/art/production' / source
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b'generated source')
            foreign = [
                {'path': 'world/terminal.png', 'source': 'docs/art/production/repaint/terminal.png',
                 'sha256': 'retained native digest', 'role': 'Retained native role'},
                {'path': 'base/feed_crane.png', 'source': 'docs/art/production/base-shelter-268/crane-source.png',
                 'sha256': 'retained crane digest', 'role': 'Retained crane role'},
            ]
            for entry in foreign:
                (art / entry['path']).write_bytes(b'redraw owned by another source')
            owned = {'path': 'world/shop.png',
                     'source': 'docs/art/production/toxic-night-kit/world-shop.aseprite'}
            ending = {'path': 'endings/escaped.png',
                      'source': 'docs/art/production/endings/escaped.png'}
            for entry in (owned, ending):
                (art / entry['path']).write_bytes(b'previous export')
            manifest = art / 'manifest.json'
            manifest.write_text(json.dumps({'files': foreign + [owned, ending]}))

            def export_image(*args):
                source = args[2] if args[0] == 'aseprite' else args[1]
                target = pathlib.Path(str(args[-1]).removeprefix('PNG32:'))
                target.write_bytes(b'exported ' + pathlib.Path(source).read_bytes())

            with patch.multiple(toxic_night, ROOT=root, ART=art, KIT=kit, ASSETS=assets), \
                    patch.object(toxic_night, 'run', side_effect=export_image), \
                    patch.object(toxic_night.subprocess, 'check_output', return_value='2 2'):
                toxic_night.export()

            result = {entry['path']: entry for entry in json.loads(manifest.read_text())['files']}
            for entry in foreign:
                with self.subTest(destination=entry['path']):
                    self.assertEqual((art / entry['path']).read_bytes(), b'redraw owned by another source')
                    self.assertEqual(result[entry['path']], entry)
            for destination in ('world/shop.png', 'world/kiosk.png'):
                self.assertEqual((art / destination).read_bytes(), b'exported native source')
                self.assertEqual(result[destination]['size'], [2, 2])
            self.assertEqual((art / 'endings/escaped.png').read_bytes(), b'generated source')
            self.assertEqual(result['endings/escaped.png']['size'], [2, 2])


if __name__ == '__main__':
    unittest.main()
