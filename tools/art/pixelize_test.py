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


def problems(rows, path="characters/test.png"):
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

    def test_a_layer_cut_from_the_concept_keeps_its_colours(self):
        painted = [[(x * 10, y * 10, (x + y) * 5, 255) for x in range(24)] for y in range(24)]
        self.assertEqual(problems(painted, "backgrounds/test.png"), [])
        found = problems(painted, "workshop/test.png")
        self.assertEqual(len(found), 2)
        self.assertIn("lone pixels", found[0])
        self.assertIn("colours, over the workshop limit", found[1])

    def test_the_screen_pixel_rule_skips_the_sprites(self):
        self.assertEqual(problems(pixelize.upscale(sprite(), 2), "characters/test.png"), [])

    def test_original_character_cuts_keep_colours_but_reject_enlarged_pixels(self):
        dense = [[(x * 10, y * 10, (x + y) * 5, 255) for x in range(24)] for y in range(24)]
        self.assertEqual(problems(dense, "characters/chef_idle.png"), [])
        blocks = pixelize.upscale(dense, 2)
        found = problems(blocks, "characters/chef_idle.png")
        self.assertEqual(len(found), 1)
        self.assertIn("drawn in blocks", found[0])
        self.assertEqual(problems(blocks, "backgrounds/test.png"), found)


class ArticulateTest(unittest.TestCase):
    def test_the_zero_pose_retains_every_original_pixel(self):
        base = sprite()
        rig = {"arm": {"mask": [[[0, 0], [10, 0], [10, 22], [0, 22]]], "pivot": [10, 10]}}
        self.assertEqual(pixelize.articulate(base, rig, {"limbs": {}}), base)

    def test_a_raised_limb_keeps_its_original_colour_and_moves_only_its_pixels(self):
        base = [[CLEAR] * 8 for _ in range(8)]
        base[4][3], base[4][4], base[2][5] = LIGHT, MID, DARK
        rig = {"arm": {"mask": [[[2, 3], [5, 3], [5, 6], [2, 6]]], "pivot": [3, 4]}}
        self.assertEqual(pixelize.articulate(base, rig, {"limbs": {}}), base)
        raised = pixelize.articulate(base, rig, {"limbs": {"arm": {"angle": -90}}})
        self.assertEqual(raised[4][3], LIGHT)
        self.assertEqual(raised[3][3], MID)
        self.assertEqual(raised[4][4], CLEAR)
        self.assertEqual(raised[2][5], DARK)
        grounded = pixelize.articulate(base, rig, {"limbs": {"arm": {"angle": -90, "floor": 6}}})
        self.assertEqual(grounded[6][3], LIGHT)
        self.assertEqual(grounded[5][3], MID)
        self.assertEqual(grounded[2][5], DARK)


class ResampleTest(unittest.TestCase):
    def test_each_pixel_is_the_average_of_the_area_it_covers(self):
        # Five columns into two: each output column covers two and a half source columns.
        rows = [[(0, 0, 0, 255), (0, 0, 0, 255), (100, 0, 0, 255), (200, 0, 0, 255), (200, 0, 0, 255)]]
        self.assertEqual(pixelize.resample(rows, 2, 1), [[(20, 0, 0, 255), (180, 0, 0, 255)]])

    def test_a_clear_pixel_lends_no_colour_to_an_opaque_one(self):
        rows = [[(250, 10, 10, 255), (0, 0, 0, 0)]]
        self.assertEqual(pixelize.resample(rows, 1, 1), [[(250, 10, 10, 128)]])

    def test_the_same_source_always_gives_the_same_pixels(self):
        rows = [[((x * 37 + y * 11) % 256, (x * y) % 256, (x + y) % 256, 255) for x in range(17)] for y in range(13)]
        self.assertEqual(pixelize.resample(rows, 13, 10), pixelize.resample(rows, 13, 10))
        self.assertEqual(len(pixelize.resample(rows, 13, 10)), 10)
        self.assertEqual(len(pixelize.resample(rows, 13, 10)[0]), 13)


class CutTest(unittest.TestCase):
    def test_erasing_baked_words_preserves_the_nearest_border_pixels(self):
        source = [[DARK] * 6 for _ in range(5)]
        source[2][2] = LIGHT
        source[2][3] = MID
        asset = {"path": "ui/test.png", "source": "test", "crop": [0, 0, 6, 5],
                 "size": [6, 5], "cut": {"interior": [2, 1, 2, 3], "fill_at": [0, 0]}}
        rows, _ = pixelize.cut(asset, None, {"test": source}, {})
        self.assertEqual(rows, [[DARK] * 6 for _ in range(5)])
        source[2][1] = LIGHT
        control, _ = pixelize.cut(asset, None, {"test": source}, {})
        self.assertEqual(control[2][1:4], [LIGHT, DARK, DARK])
        self.assertEqual(source[2][2:4], [LIGHT, MID])

    def test_flesh_feathering_preserves_blue_and_dark_outlines_and_stops_at_its_band(self):
        pink, blue = (200, 40, 170, 255), (30, 40, 140, 255)
        near_blue = (180, 40, 190, 255)
        source = [[CLEAR] * 4] + [[pink, blue, DARK, near_blue] for _ in range(4)]
        asset = {"path": "backgrounds/test.png", "source": "test", "crop": [0, 0, 4, 5],
                 "size": [4, 5], "cut": {"feather_flesh": 2}}
        rows, _ = pixelize.cut(asset, None, {"test": source}, {})
        self.assertEqual(rows, [[CLEAR] * 4, [(200, 40, 170, 0), blue, DARK, near_blue],
                                [(200, 40, 170, 128), blue, DARK, near_blue],
                                [pink, blue, DARK, near_blue], [pink, blue, DARK, near_blue]])
        asset["cut"] = {}
        unchanged, _ = pixelize.cut(asset, None, {"test": source}, {})
        self.assertEqual(unchanged, source)

    def test_a_polygon_keeps_its_silhouette_even_when_the_background_has_the_same_colour(self):
        asset = {
            "path": "walkway/test.png", "source": "test",
            "crop": [0, 0, 6, 4], "size": [6, 4],
            "cut": {"mask": [[[0, 0], [4, 0], [0, 4]], [[4, 2], [6, 2], [6, 4], [4, 4]]]},
        }
        source = [[DARK] * 6 for _ in range(4)]
        rows, _ = pixelize.cut(asset, None, {"test": source}, {})
        self.assertEqual(rows, [
            [DARK, DARK, DARK, CLEAR, CLEAR, CLEAR],
            [DARK, DARK, CLEAR, CLEAR, CLEAR, CLEAR],
            [DARK, CLEAR, CLEAR, CLEAR, DARK, DARK],
            [CLEAR, CLEAR, CLEAR, CLEAR, DARK, DARK],
        ])

    def test_a_polygon_outside_the_left_edge_keeps_no_pixels(self):
        source = [[DARK] * 6 for _ in range(4)]
        offscreen = [[-4, 0], [-1, 0], [-1, 4], [-4, 4]]
        onscreen = [[1, 0], [4, 0], [4, 4], [1, 4]]
        self.assertEqual(pixelize.polygon_mask(source, [offscreen]), [[CLEAR] * 6 for _ in range(4)])
        self.assertEqual(pixelize.polygon_mask(source, [offscreen, onscreen]),
                         [[CLEAR, DARK, DARK, DARK, CLEAR, CLEAR] for _ in range(4)])

    def test_a_fill_changes_only_the_named_polygon_with_the_original_pixels_outside_it(self):
        asset = {"path": "backgrounds/test.png", "source": "original", "crop": [0, 0, 4, 2],
                 "size": [4, 2], "cut": {"fills": [{"source": "repair", "at": [0, 0],
                     "polygons": [[[1, 0], [3, 0], [3, 1], [1, 1]]]}]}}
        original = [[DARK] * 4 for _ in range(2)]
        rows, _ = pixelize.cut(asset, None, {"original": original, "repair": [[LIGHT] * 4 for _ in range(2)]}, {})
        self.assertEqual(rows, [[DARK, LIGHT, LIGHT, DARK], [DARK] * 4])
        self.assertEqual(original, [[DARK] * 4 for _ in range(2)])

    def test_a_clear_colour_rule_with_within_rects_clears_only_inside_them(self):
        pink = (230, 60, 200, 255)
        asset = {
            "path": "walkway/test.png",
            "source": "test",
            "crop": [0, 0, 4, 1],
            "size": [4, 1],
            "cut": {"clear_colours": [{"hues": [260, 350], "saturation": 0.3, "lightness": 0.25, "within": [[2, 0, 2, 1]]}]},
        }
        rows, _ = pixelize.cut(asset, None, {"test": [[pink, DARK, pink, DARK]]}, {})
        self.assertEqual(rows, [[pink, DARK, CLEAR, DARK]])


class HdArtTest(unittest.TestCase):
    def test_native_detail_passes_but_nearest_double_at_the_same_dimensions_fails(self):
        fine = [[LIGHT if (x + y) % 2 else DARK for x in range(16)] for y in range(16)]
        coarse = [[LIGHT if (x // 2 + y // 2) % 2 else DARK for x in range(16)] for y in range(16)]
        self.assertEqual(pixelize.hd_problems(fine, [16, 16]), [])
        self.assertTrue(pixelize.hd_problems(coarse, [16, 16]))

    def test_manifest_dimensions_are_measured(self):
        fine = [[LIGHT if (x + y) % 2 else DARK for x in range(16)] for y in range(16)]
        self.assertTrue(pixelize.hd_problems(fine, [17, 16]))


if __name__ == "__main__":
    unittest.main()
