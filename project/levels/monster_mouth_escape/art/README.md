# Monster-mouth escape art

The runtime art set for the monster-mouth escape game and how to draw it.

[`manifest.json`](manifest.json) lists every PNG in this folder with its pixel size, scale, colour count, and what it depicts. [`tools/art/pixelize.py`](../../../../tools/art/pixelize.py) writes the PNGs and the manifest from [`tools/art/assets.json`](../../../../tools/art/assets.json); edit the spec and rerun it rather than editing a PNG. [Provenance](../../../../docs/art/production/monster-mouth-escape-asset-provenance.txt) records where each source came from. The [review sheet](../../../../docs/art/production/monster-mouth-escape-review-v4.png) shows the set beside the concept images, with a street composed from the walkway pieces and decorations. `pixelize.py check` measures every PNG listed in the manifest against the clean-art rules below and exits 1 when one breaks them.

## Use

- Godot imports every file here, and each PNG's `.import` file is committed beside it. `src/rendering/` draws the world, the characters and the kitchen from this folder, and `src/hud/` draws the hearts and status icons from `ui/`, the item icons (`icons/`) and the title art (`title/`) from it, and draws its panels and buttons in code.
- Folders: `backgrounds/`, `characters/`, `world/`, `walkway/`, `ground/`, `decor/`, `icons/`, `workshop/`, `ui/`, `title/`.
- The map grid is 36×16 cells of 32 px in a 1280×720 scene, which the city's camera shows at twice its size or between once and twice for two players; the backdrop layers and the interface are drawn at their own size on the screen. World tiles are one cell wide; `walkway/deck.png`, `platform.png`, `bridge.png` and `world/waterline.png` repeat horizontally, the supports vertically, and `building_wall.png` and the sunken buildings' walls in both directions without a seam.
- Every PNG is drawn at 1:1 pixels of its layer: the scene's for the world and characters, the screen's for the backdrop and the interface. Scale 2 or 4 means the art is drawn on a 2×2 or 4×4 pixel grid; draw it at its listed size, not enlarged again. Sample with nearest filtering.
- Every pixel is fully opaque or fully clear.
- No image contains text except `title/logo.png`. The engine draws every word and number.

## Clean-art rules

Every PNG keeps these, and `pixelize.py check` measures them on the art's own pixel grid:

- Clusters, not noise: at most 1% of an image's opaque pixels are lone, sharing their colour with none of their eight neighbours. `pixelize.py process` merges lone pixels into the colour around them and keeps only the few that contrast most, such as an eye.
- Colours: at most 16 for a character, 12 for every other sprite, icon and HUD picture, 16 for the logo, and 32 for a full-screen layer, the kitchen and the key art.
- One outline rule for the world and character sprites (`characters/`, `world/`, `walkway/`, `ground/`, `decor/`): a selective outline lit from above. An edge facing up shows its surface colour without a dark line; every other edge facing clear space takes a darker colour of the sprite's own, so at least 95% of them are no lighter than the pixel inside them. The icons keep their one-pixel dark outline.
- One light, from above, for the same sprites: what faces up is on average no darker than what faces down. Light from above survives the runtime mirroring of a character.

## Layers

- `backgrounds/throat.png` (1280×720, opaque) is the back layer.
- `backgrounds/city.png` (1280×480) is the middle layer: a swallowed city of tilted towers sunk in flesh, with an elevated railway and a train skewered into the flesh, clear above the skyline. Draw it behind the walkways with its bottom edge below the acid surface, so the acid covers its base.
- `backgrounds/acid.png` (1280×492) is the acid, both the band nearest behind the map and the sea in the scene. Its surface is near the top edge: move the layer so that edge follows the acid level, and fill any screen area below it with its bottom row.
- The backdrop layers are not tiles; drawn beside a mirrored copy, each meets it without a seam.
- `glow.tres` is the blend-add material of the layer that adds light over the scene.

## Characters

- 64×64 each, scale 1. Each actor's frames share one scale, so the figure is about 48 to 56 px tall; draw it taller than its cell: the frame's bottom edge on the feet, centred on them.
- Each frame faces right, or is seen from behind; mirror it horizontally at runtime for the other direction.
- Frames, for the chef and the appraiser: `idle`; `walk_1` to `walk_4`, a walk cycle of contact, passing, contact and passing; `run_1` and `run_2`; `jump`, rising; `fall`; `land`; and `climb`, seen from behind, mirrored to alternate the reaching arm.

## Walkways

Every walkway is drawn under the floor line of its cell, where an actor's feet are.

| Piece | File | Size |
|---|---|---|
| Slab, the walked-on surface: violet stone blocks with an orange lit top over a steel beam; repeats horizontally | `walkway/deck.png` | 32×28 |
| Slab variants: a riveted joint, cracked blocks | `walkway/deck_rivets.png`, `walkway/deck_worn.png` | 32×28 |
| Slab eaten by the acid: pitted stone, green stains, a holed beam | `walkway/deck_corroded.png` | 32×28 |
| Snapped end of a slab, torn on its left or right | `walkway/end_left.png`, `walkway/end_right.png` | 32×32 |
| Supports; repeat vertically | `walkway/pillar.png`, `walkway/column.png` | 24×32, 24×32 |
| Flesh grown over a slab's front edge, dripping; hangs from just under its top | `walkway/flesh_drip.png` | 40×32 |

- Every slab variant has the outer columns of `deck.png` on both sides, and each snapped end on its joined side, so any two placed side by side meet without a seam.
- A ramp is one cell of slab tilted from the floor line of its own cell down to the row below, and stairs are the same slab cut into steps, both drawn from the slab pictures.

## Sunken buildings

Every run of walkway but the pier, the planks and the tilted ground is the roof or a floor of a building sunk in the acid, whose wall fills the run's width from under the slab down to the acid.

| Piece | File | Size |
|---|---|---|
| Walls: cracked concrete, an office tower's window grid, brick with tall windows, a stucco tenement with balconies; each repeats in both directions | `world/facade_concrete.png`, `world/facade_office.png`, `world/facade_brick.png`, `world/facade_tenement.png` | 64×64 |
| Fixtures on a wall: an air conditioner, a dead neon sign with a blank face, a rolled-down shutter, a lit shop window, a drainpipe | `world/wall_ac.png`, `world/wall_sign.png`, `world/wall_shutter.png`, `world/wall_shop_window.png`, `world/wall_pipe.png` | 26×18, 30×16, 28×22, 30×30, 12×40 |
| Flesh grown over a wall's foot where the acid meets it; repeats horizontally, its bottom on the acid surface | `world/waterline.png` | 64×18 |

## Ground

What stands on or replaces a walkway in a cell of a ground kind other than road:

| Kind | File | Size |
|---|---|---|
| Rubble, impassable; stands on its deck | `ground/rubble.png` | 32×24 |
| Wrecked car sunk into a walkway; stands on its deck | `ground/car_buried.png` | 32×24 |
| Plank bridge; repeats horizontally, its top on the floor line | `ground/bridge_plank.png` | 32×14 |

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
