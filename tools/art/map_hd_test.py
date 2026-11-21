"""Regression checks for manifest-owned map cuts."""
import hashlib
import json
import pathlib
import tempfile
import unittest

import map_hd


class MapHdTest(unittest.TestCase):
    """The ImageMagick calls are injected, so no test needs the `magick` binary:
    a fake run writes the cut a real one would, and records what it was asked."""

    @staticmethod
    def fake_magick(calls):
        def run(command, check):
            calls.append(command)
            pathlib.Path(command[-1].removeprefix('PNG32:')).write_bytes(b'cut of ' + command[1].encode())
        return run


    def test_rebuilds_legacy_cut_and_preserves_repaint_and_its_metadata(self):
        with tempfile.TemporaryDirectory(prefix='w276-map-cut-') as temporary:
            root = pathlib.Path(temporary)
            art = root / 'project/levels/monster_mouth_escape/art_hd'
            (art / 'world').mkdir(parents=True)
            source = root / map_hd.LEGACY_SOURCE
            source.mkdir(parents=True)
            (source / 'interiors.png').write_bytes(b'legacy sheet')
            repaint_source = root / 'docs/art/production/street-art-251-sources/terminal.png'
            repaint_source.parent.mkdir(parents=True)
            repaint_source.write_bytes(b'repaint source')
            repainted = art / 'world/terminal.png'
            repainted.write_bytes(b'repainted artwork must never be opened')
            owned = {'path': 'world/terminal.png', 'role': 'Repainted terminal',
                     'sha256': hashlib.sha256(repainted.read_bytes()).hexdigest(),
                     'source': 'docs/art/production/street-art-251-sources/terminal.png',
                     'crop': None, 'resize': '4x4!'}
            legacy = {'path': 'world/interior_kitchen.png', 'role': 'Kitchen',
                      'source': map_hd.LEGACY_SOURCE + 'interiors.png',
                      'crop': '2x2+1+1', 'resize': '6x3!'}
            manifest = art / 'manifest.json'
            manifest.write_text(json.dumps({'files': [owned, legacy]}))
            calls = []
            map_hd.main(root, run=self.fake_magick(calls), measure=lambda command, text: '6 3')
            result = json.loads(manifest.read_text())['files']
            self.assertEqual(len(calls), 1)
            self.assertIn(str(source / 'interiors.png'), calls[0])
            self.assertEqual(calls[0][2:4], ['-crop', '2x2+1+1'])
            self.assertEqual(calls[0][calls[0].index('-resize') + 1], '6x3!')
            self.assertNotIn(str(repaint_source), [part for call in calls for part in call])
            self.assertEqual(result[0], owned)
            self.assertEqual(repainted.read_bytes(), b'repainted artwork must never be opened')
            cut = art / legacy['path']
            self.assertTrue(cut.is_file())
            self.assertEqual(result[1]['size'], [6, 3])
            self.assertEqual(result[1]['sha256'], hashlib.sha256(cut.read_bytes()).hexdigest())
            self.assertEqual(result[1]['role'], 'Kitchen')


if __name__ == '__main__':
    unittest.main()
