# ADR 0020: Cut the city from the mockup at the screen's size

> Status: superseded in part by ADR-0023; city art superseded by ADR 0025
> Date: 2026-11-05

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

[ADR 0018](0018-cut-the-backdrop-from-the-search-mockup.md) cut the backdrop from the search mockup `search-mockup-v1.png` at commit `241124d5ba2dcef937c904623f1e5e3cb03fbcf5`, 1672×941, with its own pixels. [ADR 0019](0019-lay-the-city-out-as-the-first-mockup-twice-over.md) then laid the map out against the same mockup scaled to the 1280×720 screen, where one 64-pixel cell of the close camera is 83.6 of the mockup's pixels. The second stage of [issue #186](https://github.com/garamsh/neocity/issues/186), [issue #190](https://github.com/garamsh/neocity/issues/190), cuts the walkways, shops and props from the mockup too. The test is still the user's: the city screen, set beside the mockup at the same size, looks the same.

At two scales, the screen cannot pass that test. A walkway cut at the mockup's own size does not fit the cells the map gives it. A backdrop at that size stands 1.31 times larger than the mockup beside it. The mockup has no pixel grid of its own to keep: `tools/art/pixelize.py`'s grid detection finds none (whole-image confidence 2.68 horizontally and 2.43 vertically; the earlier throat crop is 4.15 horizontally and 3.43 vertically, so neither meets the detector's 4.0 on both axes), and the share of its edge pixels one pixel wide is about 0.93 both at 1672 wide and at 1280.

The mockup's walkways are not tiles. Each is one structure: the upper walkway with its blood drips, the lower one with the vending machine, the rope bridge, the gauge ledge. Its posts stand under them at their own places, not under the ends of a run as [ADR 0015](0015-stand-lit-shops-on-short-supports-under-the-teeth.md) places supports.

Alternatives considered:

- **Cut each piece at the mockup's own size.** It fits neither the cells nor the backdrop beside the mockup at the same size.
- **Cut pieces from the mockup as tiles, varied along a row, as the earlier art was drawn.** Such a row repeats a few blocks and is not the mockup's walkway.
- **Draw the whole first screen as one picture.** It cannot show the parts the run changes: the ladder and door that break, the bridge that falls, the decks the acid eats.
- **Move the kitchen loft to row 7, where the mockup's loft floor stands within 13 pixels.** The loft would then lie directly over the upper walkway. `TemporaryContent.EveryWalkwayIsOneRowWithOpenAirAboveIt` in `src/survival/src/content_test.cpp` fails, because every walkway must have open air above it, and that is a rule.

## Decision

The whole mockup is resampled once to the 1280×720 screen. Each pixel is the average of the area of the mockup it covers, weighted by alpha. Every picture of the city is cut from that copy, still one picture pixel to a screen pixel at the close camera. This covers the backdrop's three layers, the walkways, the buildings and shop fronts, the posts, the props, the ladders and the doors.

A picture of the map's cells shows the mockup where the first city screen draws that cell. The map's columns 0 to 19 are the mockup's screen. Columns 20 to 39 show the same columns again.

- **Decks:** one band of the mockup a cell deep per floor line.
- **Building fronts:** a band per building row.
- **Shop fronts:** two cells of the mockup over their floor, across the building standing right over that floor when there is one, as the mockup's kitchen fills its loft.
- **Posts:** the mockup's posts, chain, gauge pillar, cable and lamp, as one picture under each 20-column half.
- **Backdrop between the pieces:** where the mockup shows its backdrop between the posts, decks, building fronts and shop fronts, explicit source-coordinate masks keep the structures and clear those pixels regardless of their hue, so the game's own layers show there when the camera moves.
- **Acid band:** the mockup's lake with its original far shore masked to the skyline above the surface; the city spans the screen, retaining the original exposed background and repairing the foreground occlusions with a recorded local source, and the throat continues below it. The foreground sea draws only water, so its own moving surface never lays a second shore over the lake.

The city layer spans the original screen. Its source remains the mockup outside the explicit foreground-repair polygons; only those polygons take pixels from the locally committed built-in ImageGen edit, whose prompt and digest are recorded in the art provenance. The edit repairs the flat brown regions left by the earlier erase without replacing the original visible throat, train or shore. The generated source is not deterministic; regenerating its runtime derivatives from that committed source is. The skyline follows the final composite, including the flesh between its buildings. Pink tissue along that cut blends into the throat with coverage alpha; blue architecture and dark outlines retain their opaque silhouette.

The near sea keeps its screen surface at the lower of the projected map surface and the far shore: the maximum of their screen rows. The drawing, light, steam and reported surface share that position. This preserves the perspective order when the camera or acid would otherwise lift the near water above the throat. It changes no simulation acid, destruction threshold or movement rule. Mirrored image regions use an explicit transform with positive rectangle sizes, and deep water continues in reflected texture strips rather than stretching one source row into vertical streaks.

The kitchen loft's band is taken from the mockup's loft floor, 53 pixels below the loft row the map gives it. The kitchen front above the loft is taken from the same height.

The scenery the map names (trains, building shells, cables, neon signs, flesh, rubble heaps, broken pillars and lamps) is not drawn apart from these pictures. The mockup's own trains, towers, cable and lamp stand in the backdrop and in the posts' picture.

This supersedes ADR 0018's cut at the mockup's own size and ADR 0015's support under each end of a run. ADR 0017's one picture pixel to a screen pixel is unchanged.

## Consequences

- `tools/art/pixelize.py` resizes a cut asset's source to the screen when the asset says `screen`. It patches out the two characters with the mockup's own pixels beside them, keeps the silhouettes each asset names, and stacks bands. `pixelize_test.py` checks the area average, repeatability, polygon union and off-canvas clipping, and preservation outside a repair polygon.
- `walkway/`, `world/` and `ground/` hold pictures cut from the mockup, at twice the scene's resolution, and `decor/` is gone. The colour limit, the lone-pixel rule and the sprite rules do not apply to them. The density rule does. The noodle vendor, a person, keeps its earlier picture until the characters' stage.
- The camera aims 12 scene pixels above a lone human's feet, not 32. A human on the upper walkway then stands at screen row 325, where the mockup draws it at about 329, and on the lower walkway at 389, where the mockup draws about 383.
- The kitchen loft stands 53 pixels higher on the screen than the mockup's. The kitchen front over it is the mockup's lower 64 rows of the kitchen, not its whole height.
- The decks keep a lit top edge drawn over their top rows, as ADR 0011 decides for the walked-on surface. It is the one change from the mockup's pixels in the walkways.
- A tilted cell that is not a step down to the left is drawn as a level deck. The fallen slab drawn along a run of tilted cells goes, since the map has none.
- Source-coordinate silhouettes replace hue clearing on the decks, walls, shops and supports. The map's last upper-walkway cell gets a level deck patch from its neighbour, so its visible top follows its collision surface rather than the mockup's earlier step.

## Signs the decision was wrong

- The east half reads as a copy of the first screen, and players lose their way between the two.
- A silhouette still carries visible backdrop at its edges, or the city and far shore separate as the camera moves.
- At the scene's own size, the resampled pictures, read from their half-size mipmaps, lose too much of the mockup's detail.
- The kitchen loft's 53-pixel offset reads as wrong beside the mockup, and the map needs a loft row that the open-air rule allows.
