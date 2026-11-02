# Monster-mouth escape art

The runtime art set for the monster-mouth escape game and how to draw it.

[`manifest.json`](manifest.json) lists every PNG in this folder with its pixel size, scale, colour count, and what it depicts. [`tools/art/pixelize.py`](../../../../tools/art/pixelize.py) writes the PNGs and the manifest from [`tools/art/assets.json`](../../../../tools/art/assets.json); edit the spec and rerun it rather than editing a PNG. [Provenance](../../../../docs/art/production/monster-mouth-escape-asset-provenance.txt) records where each source came from. The [review sheet](../../../../docs/art/production/monster-mouth-escape-review-v3.png) shows the set beside the concept images, with a broken-road street composed from the ground tiles and decorations.

## Use

- Godot imports every file here, and each PNG's `.import` file is committed beside it. `src/rendering/` draws the world, the characters and the kitchen from this folder.
- Folders: `backgrounds/`, `characters/`, `world/`, `ground/`, `decor/`, `icons/`, `workshop/`, `ui/`, `title/`.
- The map grid is 36×16 cells of 32 px on a 1280×720 screen. World tiles are one cell; `platform.png` and `bridge.png` repeat horizontally and `building_wall.png` in both directions without a seam.
- Every PNG is drawn at 1:1 screen pixels. Scale 2 or 4 means the art is drawn on a 2×2 or 4×4 pixel grid; draw it at its listed size, not enlarged again. Sample with nearest filtering.
- Every pixel is fully opaque or fully clear.
- No image contains text except `title/logo.png`. The engine draws every word and number.

## Layers

- `backgrounds/throat.png` (1280×720, opaque) is the back layer.
- `backgrounds/city.png` (1280×288) is a distant skyline band. Draw it behind the walkways with its bottom edge below the acid surface, so the acid covers its base.
- `backgrounds/acid.png` (1280×492) is the front layer. Its surface is near the top edge: move the layer so that edge follows the acid level, and fill any screen area below it with its bottom row.

## Characters

- 64×64 each, scale 1, cut from the approved concept sheets ART-CHAR-001 and ART-CHAR-002. The figure is about 48 to 56 px tall, so draw it taller than its cell: the frame's bottom edge on the cell's floor, centred on the cell.
- Each pose faces the way the sheet draws it; mirror horizontally at runtime for the other direction.
- Frames: `idle`, `walk_1` and `walk_2`, and `climb` (seen from behind) for the chef and the appraiser.

## Ground

One tile per ground surface kind, for a map that says which kind each ground cell is. Each is the road seen in section: asphalt with an orange lane dash on top, concrete deck below.

| Kind | File | Size | Joins the road on |
|---|---|---|---|
| Intact road | `ground/road.png` | 32×32 | both sides; repeats horizontally |
| Cracked road | `ground/road_cracked.png` | 32×32 | both sides |
| Pothole | `ground/pothole.png` | 32×32 | both sides |
| Road end, left | `ground/road_end_left.png` | 32×32 | right side; broken at its left edge |
| Road end, right | `ground/road_end_right.png` | 32×32 | left side; broken at its right edge |
| Tilted slab | `ground/slab_tilted.png` | 32×40 | both sides |
| Rubble, impassable | `ground/rubble.png` | 32×48 | both sides |
| Plank bridge | `ground/bridge_plank.png` | 32×14 | itself; repeats horizontally |
| Half-buried car | `ground/car_buried.png` | 32×48 | both sides |

- A tile taller than 32 px rises above its cell: draw its bottom 32 rows on the cell and the rest over the cell above.
- Draw the plank bridge at the top of a gap cell, level with the road surface.
- Every tile that joins the road has the intact road's outer columns on that side, so any two of them placed side by side meet without a seam.
- The kind names are this folder's; #97 adds the per-cell kind, and the code that draws the ground maps its names to these files.

## Decorations

No collision. Each kind has at least two variants; draw them behind the characters, and the ones that stand (facades, pillars, lamps, rubble, tendril, polyps) with their bottom edge on a floor.

| Kind | Files |
|---|---|
| Tilted elevated train car | `decor/train_car_lit.png`, `decor/train_car_wrecked.png` |
| Collapsed building facade | `decor/facade_tall.png`, `decor/facade_low.png` |
| Hanging cables and pipes | `decor/cables_sagging.png`, `decor/cables_hanging.png`, `decor/pipe_bent.png`, `decor/pipes_valve.png` |
| Dead neon sign | `decor/neon_box.png`, `decor/neon_blade.png` |
| Flesh vines and growths | `decor/flesh_vines.png`, `decor/flesh_tendril.png`, `decor/flesh_tumor.png`, `decor/flesh_polyps.png` |
| Rubble pile | `decor/rubble_low.png`, `decor/rubble_high.png` |
| Broken overpass pillar | `decor/pillar_snapped.png`, `decor/pillar_deck.png` |
| Street lamp | `decor/lamp_standing.png`, `decor/lamp_bent.png` |
