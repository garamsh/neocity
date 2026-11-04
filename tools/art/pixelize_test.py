"""Unit tests of the clean-art check in pixelize.py: python3 -m unittest tools/art/pixelize_test.py"""

import unittest

import pixelize

CLEAR = (0, 0, 0, 0)
LIGHT = (220, 200, 160, 255)
MID = (140, 90, 120, 255)
DARK = (30, 20, 40, 255)


def sprite():
    """A 20x20 block on a clear border: a lit top row, a mid body and a dark side and bottom edge."""
    rows = [[CLEAR] * 22 for _ in range(22)]
    for y in range(1, 21):
        for x in range(1, 21):
            if y == 1:
                rows[y][x] = LIGHT
            elif y == 20 or x in (1, 20):
                rows[y][x] = DARK
            else:
                rows[y][x] = MID
    return rows


def problems(rows, path="decor/test.png"):
    found, _ = pixelize.problems(path, rows, 1)
    return found


class CheckTest(unittest.TestCase):
    def test_a_clean_lit_sprite_breaks_no_rule(self):
        self.assertEqual(problems(sprite()), [])

    def test_scattered_lone_pixels_break_the_lone_rule(self):
        rows = sprite()
        self.assertEqual(problems(rows), [])
        for y in range(3, 19, 4):
            for x in range(3, 19, 4):
                rows[y][x] = (10, 200, 30, 255)
        found = problems(rows)
        self.assertEqual(len(found), 1)
        self.assertIn("lone pixels", found[0])

    def test_colours_over_the_folder_limit_break_the_colour_rule(self):
        def striped(count):
            rows = sprite()
            for i in range(count - 3):
                for x in range(2, 20):
                    rows[2 + i][x] = (40 + i * 10, 60, 200, 255)
            return rows

        self.assertEqual(problems(striped(16), "characters/test.png"), [])
        found = problems(striped(17), "characters/test.png")
        self.assertEqual(len(found), 1)
        self.assertIn("17 colours", found[0])

    def test_edges_lighter_than_inside_break_the_outline_rule(self):
        rows = sprite()
        for y in range(2, 21):
            for x in (1, 20):
                rows[y][x] = LIGHT
        found = problems(rows)
        self.assertEqual(len(found), 1)
        self.assertIn("edges darker", found[0])

    def test_light_from_below_breaks_the_light_rule(self):
        rows = sprite()
        for x in range(1, 21):
            rows[1][x] = DARK
            rows[20][x] = (250, 250, 250, 255)
        found = [problem for problem in problems(rows) if "lit from below" in problem]
        self.assertEqual(len(found), 1)

    def test_the_outline_and_light_rules_skip_the_full_screen_layers(self):
        rows = sprite()
        for x in range(1, 21):
            rows[1][x] = DARK
            rows[20][x] = (250, 250, 250, 255)
        found = problems(rows, "backgrounds/test.png")
        self.assertEqual([problem for problem in found if "edges darker" in problem or "lit from below" in problem], [])

    def test_a_backdrop_layer_drawn_in_blocks_breaks_the_screen_pixel_rule(self):
        dense = [[LIGHT if (x + y) % 2 else MID for x in range(24)] for y in range(24)]
        self.assertEqual(problems(dense, "backgrounds/test.png"), [])
        blocks = pixelize.upscale([row[::2] for row in dense[::2]], 2)
        found = problems(blocks, "backgrounds/test.png")
        self.assertEqual(len(found), 1)
        self.assertIn("drawn in blocks", found[0])

    def test_the_screen_pixel_rule_skips_the_sprites(self):
        self.assertEqual(problems(pixelize.upscale(sprite(), 2), "decor/test.png"), [])


if __name__ == "__main__":
    unittest.main()
