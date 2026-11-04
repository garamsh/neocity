# Monster-mouth escape art

The runtime art set for the monster-mouth escape game and how to draw it.

[`manifest.json`](manifest.json) lists every PNG in this folder with its pixel size, scale, colour count, and what it depicts. [`tools/art/pixelize.py`](../../../../tools/art/pixelize.py) writes the PNGs and the manifest from [`tools/art/assets.json`](../../../../tools/art/assets.json); edit the spec and rerun it rather than editing a PNG. [Provenance](../../../../docs/art/production/monster-mouth-escape-asset-provenance.txt) records where each source came from. The [review sheet](../../../../docs/art/production/monster-mouth-escape-review-v4.png) is a record of version 4 of the set, which later versions did not redraw: it shows that version beside the concept images, with a street composed from that version's walkway pieces and decorations, which the pictures cut from the mockup have since replaced. `pixelize.py check` measures every PNG listed in the manifest against the clean-art rules below and exits 1 when one breaks them.

## Use

- Godot imports every file here, and each PNG's `.import` file is committed beside it. `src/rendering/` draws the world, the characters and the kitchen from this folder, and on the map the item icons (`icons/`) and the full heart from `ui/`. `src/hud/` draws the hearts and the day and clock icons from `ui/`, the item icons and the title art (`title/`) from it, and draws its panels and buttons in code.
- Folders: `backgrounds/`, `characters/`, `world/`, `walkway/`, `ground/`, `icons/`, `workshop/`, `ui/`, `title/`.
- The map grid is 40×16 cells of 32 px in a 1280×720 scene, which the city's camera shows at twice its size, or at its own size for two players who do not fit at twice; the backdrop layers, the sea in the scene and the interface are drawn at their own size on the screen at every zoom. Columns 0 to 19 are the search mockup's first city screen, and columns 20 to 39 show the same columns again.
- Every PNG is drawn at 1:1 pixels of its layer: the scene's for the world and characters, the screen's for the backdrop and the interface. Scale 2 or 4 means the art is drawn on a 2×2 or 4×4 pixel grid; draw it at its listed size, not enlarged again. Sample with nearest filtering.
- [ADR 0017](../../../../docs/architecture/adr/0017-draw-every-picture-one-pixel-to-a-screen-pixel-at-the-close-camera.md) sets one density, one picture pixel to a screen pixel at the default camera. `backgrounds/` is drawn so, at scale 1, and `walkway/`, `world/` and `ground/` are cut at twice the scene's resolution, 64 px a cell. The other folders keep their scale until their stage redraws them at twice the scene's resolution: 64 px a cell, 128×128 character frames.
- Every pixel is fully opaque or fully clear.
- No image contains text except `title/logo.png`. The engine draws every word and number.

## Clean-art rules

Every PNG keeps these, and `pixelize.py check` measures them on the art's own pixel grid, and the one about screen pixels on the PNG's own pixels:

- Clusters, not noise: at most 1% of an image's opaque pixels are lone, sharing their colour with none of their eight neighbours. `pixelize.py process` merges lone pixels into the colour around them and keeps only the few that contrast most, such as an eye.
- Colours, by folder: at most 16 in `characters/`; 12 in `icons/` and `ui/`; 32 in `workshop/` and `title/`, but 16 for the logo.
- `backgrounds/`, `walkway/`, `world/` and `ground/` are cut from the search mockup with its own pixels, as [ADR 0020](../../../../docs/architecture/adr/0020-cut-the-city-from-the-mockup-at-the-screens-size.md) decides, and keep its colours: the lone-pixel rule, the colour limit and the sprite rules below do not apply to them. The noodle vendor, a person, keeps its earlier picture until the characters' stage.
- One picture pixel to a screen pixel in `backgrounds/`, `walkway/`, `world/` and `ground/`: at least 10% of the pixels that differ from a neighbour along a row, and along a column, differ from both, so stand one pixel wide or tall. Art drawn in blocks of two or more pixels has none.
- One outline rule for the character sprites (`characters/`): a selective outline lit from above. An edge facing up shows its surface colour without a dark line; every other edge facing clear space takes a darker colour of the sprite's own, so at least 95% of them are no lighter than the pixel inside them. The icons keep their one-pixel dark outline.
- One light, from above, for the same sprites: what faces up is on average no darker than what faces down. Light from above survives the runtime mirroring of a character.

## Layers

The whole search mockup, `docs/art/concept-art/monster-mouth-escape/search-mockup-v1.png` at commit `241124d5ba2dcef937c904623f1e5e3cb03fbcf5`, is resampled once to the 1280×720 screen, and the three layers are cut from that copy with its own pixels: what the mockup shows of the map, its characters and the interface over a layer is filled by a Recraft erase of that region. `tools/art/assets.json` lists each layer's fills and clear regions in the mockup's pixels and its crop in the screen's.

- `backgrounds/throat.png` (674×459, opaque) is the back layer: the fleshy palate with the mockup's cream fangs hanging from its top edge, over the pink throat and its uvula. Draw it with its top edge at the screen's top.
- `backgrounds/city.png` (536×398) is the middle layer: a swallowed city of tilted towers with orange windows and an elevated railway with lit trains, sunk in flesh, clear above the skyline.
- `backgrounds/acid.png` (1133×292) is the acid, both the band nearest behind the map and the sea in the scene, each drawn at its own size on the screen: the glowing acid lake with sunken trains and broken pillars, its surface 61 rows below the picture's top edge, under a strip of the far shore's ruins that meets the city layer's bottom on the first city screen. Move the layer so that surface follows the acid level, and fill any screen area below it with its bottom row.
- On the first city screen each layer stands where the mockup shows it.
- `glow.tres` is the blend-add material of the layer that adds light over the scene.

## Characters

- 64×64 each, scale 1. Each actor's frames share one scale, so the figure is about 48 to 56 px tall; draw it taller than its cell: the frame's bottom edge on the feet, centred on them.
- Each frame faces right, or is seen from behind; mirror it horizontally at runtime for the other direction.
- Frames, for the chef and the appraiser: `idle`; `walk_1` to `walk_4`, a walk cycle of contact, passing, contact and passing; `run_1` and `run_2`; `jump`, rising; `fall`; `land`; and `climb`, seen from behind, mirrored to alternate the reaching arm.

## Walkways and buildings

Each picture is cut from the mockup at the screen's size where the first city screen draws it, at twice the scene's resolution: a cell is 64 of its pixels. A cell shows the part of a band at its column, counted from 0 to 19 again in the east half.

| Picture | File | Bands |
|---|---|---|
| Walkway decks, drawn under the floor line of their cell | `walkway/deck.png` | One 56-pixel band per floor line: the kitchen roof, the shop roof, the kitchen loft, the upper walkway, the lower walkway, and the lower walkway's plain deck for the pier |
| Posts, chain, gauge pillar, cable and lamp, one picture under each 20 columns | `walkway/structure.png` | One |
| Building fronts, a cell deep per row, and the kitchen loft's face under its deck | `world/building_wall.png` | Four |
| Lit shop fronts, two cells deep over their floor | `world/shop.png` | Two: the kitchen loft's and the gauge ledge's |

- Where the mockup shows its backdrop between these pieces, the cut clears the backdrop's magenta and blue, so the game's own layers show through when the camera moves; each deck band's lit top rows are kept whole.
- The decks keep a lit top edge drawn over their top rows, for the walked-on surface. A ramp is one cell of deck tilted from the floor line of its own cell down to the row below, and stairs are the same deck cut into steps.
- `ground/rubble.png` stands on its deck in a cell of rubble ground.
- The ladders, doors, vending machine, cart, kiosk, cold store, ferry, washed-up bag and acid drip in `world/` are each the mockup's own piece.
