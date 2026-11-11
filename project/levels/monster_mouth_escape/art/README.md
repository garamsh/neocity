# Monster-mouth escape art

The runtime art set for the monster-mouth escape game and how to draw it.

[`manifest.json`](manifest.json) lists every PNG in this folder with its pixel size, scale, colour count, and what it depicts. [`tools/art/pixelize.py`](../../../../tools/art/pixelize.py) writes the PNGs and the manifest from [`tools/art/assets.json`](../../../../tools/art/assets.json); edit the spec and rerun it rather than editing a PNG. [Provenance](../../../../docs/art/production/monster-mouth-escape-asset-provenance.txt) records where each source came from. The [review sheet](../../../../docs/art/production/monster-mouth-escape-review-v4.png) is a record of version 4 of the set, which later versions did not redraw: it shows that version beside the concept images, with a street composed from that version's walkway pieces and decorations, which the pictures cut from the mockup have since replaced. `pixelize.py check` measures every PNG listed in the manifest against the clean-art rules below and exits 1 when one breaks them.

## Use

- Godot imports every file here, and each PNG's `.import` file is committed beside it. `src/rendering/` draws the world, the characters and the kitchen from this folder, and on the map the item icons (`icons/`) and the full heart from `ui/`. `src/hud/` draws the hearts and the day and clock icons from `ui/`, the item icons and the title art (`title/`) from it, and shares the original mockup frame cuts with the menus and base screens.
- Folders: `backgrounds/`, `characters/`, `world/`, `walkway/`, `ground/`, `icons/`, `workshop/`, `ui/`, `title/`.
- The map grid is 40×16 cells of 32 px in a 1280×720 scene, which the city's camera shows at twice its size, or at its own size for two players who do not fit at twice; the backdrop layers, the sea in the scene and the interface are drawn at their own size on the screen at every zoom. Columns 0 to 19 are the search mockup's first city screen, and columns 20 to 39 show the same columns again.
- Every PNG is drawn at 1:1 pixels of its layer: the scene's for the world, the screen's for the backdrop and the interface, and half their native size in the scene for the character cuts. Scale 2 or 4 means the art is drawn on a 2×2 or 4×4 pixel grid; draw it at its listed size, not enlarged again. Sample with nearest filtering.
- [ADR 0017](../../../../docs/architecture/adr/0017-draw-every-picture-one-pixel-to-a-screen-pixel-at-the-close-camera.md) sets one density, one picture pixel to a screen pixel at the default camera. `backgrounds/` is drawn so, at scale 1, and `walkway/`, `world/` and `ground/` are cut at twice the scene's resolution, 64 px a cell. `characters/` uses 128×128 frames at twice the scene's resolution too. The interface frames retain native nine-pixel corners and repeat their border strips; item icons are 56×56 at native screen density.
- Sprite masks are fully opaque or fully clear. The city backdrop's pink tissue edge has coverage alpha after resampling; blue-led architecture and dark outlines stay opaque.
- No image contains text except `title/logo.png`. The engine draws every word and number.

## Clean-art rules

Every PNG keeps these, and `pixelize.py check` measures them on the art's own pixel grid, and the one about screen pixels on the PNG's own pixels:

- Clusters, not noise: at most 1% of an image's opaque pixels are lone, sharing their colour with none of their eight neighbours. `pixelize.py process` merges lone pixels into the colour around them and keeps only the few that contrast most, such as an eye.
- Colours, by folder: at most 12 in `icons/` and `ui/`; 32 in `workshop/` and `title/`, but 16 for the logo.
- `backgrounds/`, `walkway/`, `world/` and `ground/` are cut from the search mockup with its own pixels, as [ADR 0020](../../../../docs/architecture/adr/0020-cut-the-city-from-the-mockup-at-the-screens-size.md) decides, and keep its colours: the lone-pixel rule, the colour limit and the sprite rules below do not apply to them. The noodle vendor keeps its earlier picture. The chef and appraiser cuts and their articulated poses keep the mockup's colours and light too, as [ADR 0021](../../../../docs/architecture/adr/0021-keep-the-mockup-characters-and-articulate-their-pixels.md) decides; they share these exemptions.
- One picture pixel to a screen pixel in `backgrounds/`, `walkway/`, `world/`, `ground/` and the chef and appraiser frames: at least 10% of the pixels that differ from a neighbour along a row, and along a column, differ from both, so stand one pixel wide or tall. Art drawn in blocks of two or more pixels has none.
- [ADR 0022](../../../../docs/architecture/adr/0022-cut-the-interface-frames-and-bag-icons-from-the-mockup.md) exempts exactly nine interface cuts from the colour and lone-pixel limits: `ui/frame_{panel,button,slot}.png`, `ui/icon_{day,clock}.png`, and `icons/ingredient_{mushroom,tomato,fish,pickles}.png`. Their source colours remain intact. The other seventeen icons keep the twelve-colour cleanup, reprocessed from larger raw sources at 56 pixels.
- The icons keep their one-pixel dark outline. The character cuts retain their original outlines and light through runtime mirroring.

## Layers

The whole search mockup, `docs/art/concept-art/monster-mouth-escape/search-mockup-v1.png` at commit `241124d5ba2dcef937c904623f1e5e3cb03fbcf5`, is resampled once to the 1280×720 screen, and the three layers are cut from that copy with its own pixels: what the mockup shows of the map, its characters and the interface over a layer is filled only in its named repair regions. The city uses the local inpaint source recorded in the manifest; the throat and lake retain the earlier Recraft erases. `tools/art/assets.json` lists each layer's fills and clear regions in the mockup's pixels and its crop in the screen's.

- `backgrounds/throat.png` (674×459, opaque) is the back layer: the fleshy palate with the mockup's cream fangs hanging from its top edge, over the pink throat and its uvula. Draw it with its top edge at the screen's top and continue its bottom row behind the other layers.
- `backgrounds/city.png` (1280×490) is the middle layer: a swallowed city of tilted towers with orange windows and an elevated railway with lit trains, sunk in flesh, clear above the final composite's skyline and continuous through the lower flesh between its buildings. Only pink tissue along the upper cut blends into the throat. Its full-width source preserves the original outside the repair polygons; only the occluded foreground areas use the recorded inpaint source.
- `backgrounds/acid.png` (1280×338) is the acid, both the band nearest behind the map and the sea in the scene, each drawn at its own size on the screen: the glowing acid lake with sunken trains and broken pillars, its surface 107 rows below the picture's top edge, with the original far shore masked to its skyline above that surface. Only the background band includes the shore; the foreground sea starts at its water surface. Move each surface with its acid level, keeping the near sea below the far shore in screen perspective. Continue deep water below the picture in alternating reflected strips of its final 64 rows.
- On the first city screen each layer stands where the mockup shows it.
- `glow.tres` is the blend-add material of the layer that adds light over the scene.

## Characters

- 128×128 each, scale 1, drawn at half their native size in the scene and one native pixel per screen pixel at the close camera. The chef stands 115 screen pixels tall and the appraiser 72, the original mockup's different proportions. The first city screen permits a height difference of at most 6 pixels from those silhouettes. Each actor's frames share its scale and foot baseline; the chef's spatula extends farther right of its feet than its body does.
- Idle cuts the mockup's standing figures, retaining their RGB values inside explicit silhouettes and clearing backdrop hues at their edges. The motion frames articulate those same pixels with nearest sampling; the head and torso retain their original detail. No new raster generation is used. Each frame faces right; mirror it horizontally at runtime for the other direction.
- Frames, for the chef and the appraiser: `idle`; `walk_1` to `walk_4`, a walk cycle of contact, passing, contact and passing; `run_1` and `run_2`; `jump`, rising; `fall`; `land`; and `climb`, a side-facing reach with one raised knee, mirrored on successive ladder steps.
- The [screen comparison](../../../../docs/architecture/evidence/survival/191-mockup-beside-city.png) places the 1280×720 resampled original on the left and captured first city screen on the right. The [4× close-up](../../../../docs/architecture/evidence/survival/191-closeup-4x.png) has the chef above the appraiser, with the original crop, transparent idle cut and captured base actor in its three columns. The [motion strip](../../../../docs/architecture/evidence/survival/191-motion.png) uses that same row order and columns idle, walk 1–4, run 1–2, jump, fall, land and climb, at native size. Captures use seed 11 and the harness's pinned scene clock. The city's source icon plates reserve the drawn actor-frame areas before placement, as [#192](https://github.com/garamsh/neocity/issues/192#issuecomment-6663494906) requires.


## Walkways and buildings

Each picture is cut from the mockup at the screen's size where the first city screen draws it, at twice the scene's resolution: a cell is 64 of its pixels. A cell shows the part of a band at its column, counted from 0 to 19 again in the east half.

| Picture | File | Bands |
|---|---|---|
| Walkway decks, drawn under the floor line of their cell | `walkway/deck.png` | One 56-pixel band per floor line: the kitchen roof, the shop roof, the kitchen loft, the upper walkway, the lower walkway, and the lower walkway's plain deck for the pier |
| Posts, chain, gauge pillar, cable and lamp, one picture under each 20 columns | `walkway/structure.png` | One |
| Building fronts, a cell deep per row, and the kitchen loft's face under its deck | `world/building_wall.png` | Four |
| Lit shop fronts, two cells deep over their floor | `world/shop.png` | Two: the kitchen loft's and the gauge ledge's |

- The cut keeps explicit source-coordinate silhouettes of decks, walls, shop fronts and supports, so the backdrop between them remains clear regardless of its colour. The last upper-walkway cell takes a level deck patch from beside it: the map's step starts one cell farther east than the mockup's.
- The decks keep a lit top edge drawn over their top rows, for the walked-on surface. At runtime an exposed side below that full-width edge is recessed in broken stone courses, its narrow rim shaded with the same source pixels; the loft underside continues that local silhouette. Current standing terrain determines exposure, including ends revealed by destruction, while uninterrupted interiors retain their original band. [ADR 0023](../../../../docs/architecture/adr/0023-finish-exposed-deck-faces-with-the-mockups-stone.md) records this local exception to ADR 0020. A ramp is one cell of deck tilted from the floor line of its own cell down to the row below, and stairs are the same deck cut into steps.
- `ground/rubble.png` stands on its deck in a cell of rubble ground.
- The ladders, doors, vending machine, cart, kiosk, cold store, ferry, washed-up bag and acid drip in `world/` are each the mockup's own piece.

## Interface

- Panel, button and bag-slot borders are cuts of the same 1280×720 mockup. Their interiors are cleared of baked words and pictures, and the scenery outside their corners is transparent. The native nine-pixel corners stay fixed; repeated edge strips adapt to each screen's content without enlarging the borders. Small gauge troughs remain drawn on whole pixels.
- The original bag pictures map to existing IDs: mushroom → Mushroom 4; the can with a tomato on its label → Tomato 1; fish → Fish 2; pickle jar → Pickles 5. No item, recipe or inventory rule changes.
- Seven framed slots replace the mockup's six. Both actors retain status, three health hearts, stamina and tool status; damage has its own line. The action area retains current action names and the two humans' movement controls. Its two-player card is 136 pixels high, with taller buttons and separate key lines so the native borders stand clear of the words. Runtime text, Tiny5's grid and minimum size, clicks, tips, logs and cooking details remain.
- The original source and its SHA256, cut coordinates, silhouettes and policy are recorded in the spec, manifest and provenance. The other icons use their existing raw sources, rather than enlarging earlier runtime PNGs. No new raster was generated for this stage.
- Evidence: [source beside first city](../../../../docs/architecture/evidence/survival/192-mockup-beside-city.png), source left and captured city right at 1280×720 each; [interface close-ups](../../../../docs/architecture/evidence/survival/192-interface-closeup-2x.png), top source, top capture, bottom source, bottom capture at 2×; [all twenty-one icons](../../../../docs/architecture/evidence/survival/192-icons-3x.png), at 3× in manifest order, seven across. The first-city capture uses seed 11 and the capture harness's pinned clock.
