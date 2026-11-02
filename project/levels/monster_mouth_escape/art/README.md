# Monster-mouth escape art

The runtime art set for the monster-mouth escape game and how to draw it.

[`manifest.json`](manifest.json) lists every PNG in this folder with its pixel size, scale, colour count, and what it depicts. [`tools/art/pixelize.py`](../../../../tools/art/pixelize.py) writes the PNGs and the manifest from [`tools/art/assets.json`](../../../../tools/art/assets.json); edit the spec and rerun it rather than editing a PNG. [Provenance](../../../../docs/art/production/monster-mouth-escape-asset-provenance.txt) records where each source came from. The [review sheet](../../../../docs/art/production/monster-mouth-escape-review-v2.png) shows the set beside the concept images.

## Use

- No file here is loaded by the game yet. The empty `.gdignore` is temporary: it keeps Godot from importing this folder until #92 loads the art, deletes it, and commits the `.import` files Godot then writes.
- Folders: `backgrounds/`, `characters/`, `world/`, `icons/`, `workshop/`, `ui/`, `title/`.
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
