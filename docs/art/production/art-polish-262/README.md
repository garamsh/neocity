Native map finishing evidence and the unintegrated crane artwork for #262.

# Map and base polish

- Scope: [#262 section C, items 5, 6, 7 and 9](https://github.com/garamsh/neocity/issues/262#issuecomment-7060041404), plus [#304](https://github.com/garamsh/neocity/issues/304)'s escape ending picture.
- Direction: [Toxic Night](../../README.md#current-direction). Status: candidate implementation; PM creative and runtime review remain pending.
- Baseline: `053e0dfb96409f85aa23f92d19425d6f5ccba909`.
- Native edits: five existing kit sources retain canvas dimensions, alpha silhouettes and floor contacts. The street's first 18 rows per 126-pixel band and the aquarium's first 45 rows are unchanged. Recessed metal side plates, seams, riveted brackets and local amber lamps sit below them. Sparse pipes and short cables hang below two of the six street bands, near their ends; they do not occupy the walked surface.
- Background edits affect pink tissue, green water, cyan tanks and amber/gold trim by colour selection; existing dark pixels and other foreground assets are retained. No full-screen darkening is applied.
- Recipe: `tools/art/toxic_night_polish.lua` records the one-time native pixel edits. Run Aseprite batch with `--script-param kind=<kind> --script tools/art/toxic_night_polish.lua` on the baseline source; kinds are `deck`, `aquarium_deck`, `throat`, `acid`, `aquarium`. Do not repeat the colour edit on an already edited source. Ordinary `python3 tools/art/toxic_night.py` exports the edited sources and preserves the native changes.

## Review evidence

| Item | Board | Reading |
|---|---|---|
| 5 | [Deck sides](5-decks-before-after.png) | Matching native street and aquarium crops, before left / after right |
| 6 | [Background values](6-backgrounds-before-after.png) | Matching tissue, acid and tank/trim crops, before left / after right |
| 7 | [Crane](7-crane-before-after.png) | Code rectangles reconstructed over the kitchen at rest / proposed image placement; not runtime captures |
| 9 | [Pipe/cable groups](9-props-before-after.png) | Matching underside crops near two band ends, before left / after right |

- Recompose: `python3 tools/art/toxic_night_review.py --polish`. Boards read the baseline through Git and the current exports; no display is opened.
- The boards establish asset differences, not runtime visibility, lighting, movement or camera framing. No Godot display run was made. The PM owns final bundle QA.

## Crane source and engine handoff

- `crane-source.png`: unmodified built-in ImageGen output, 2154×730 RGBA, generated 2026-11-21 with `transparent_background: true`. Reference: `docs/art/concept-art/toxic-night/f-base.png`, material and crane only. [Exact submitted prompt](crane-prompt.txt). Model, seed and quality were not exposed. No paid credits or external upload.
- `art_hd/base/feed_crane.png`: nearest-sampled 660×224, alpha threshold at 50%, metadata stripped. `tools/art/toxic_night.py` reproduces it and records both digests in the HD manifest.
- `src/hud/survival.cpp`'s `paint_workshop_art` currently draws the crane as rectangles. This art PR stops at the image handoff and changes no `src/` file. Replace those rectangles with the supplied artwork through the existing HD-image helper, starting at layout `(830,150)` with size `(440,149.333)` (canvas `(1245,225)`, size `660×224`). The supplied crop board previews this placement.
- Preserve the lowering state and food rendering in the engine change. The fixed beam/support must remain still while the cable extends and cradle moves; do not translate the complete sprite. The engine worker can separate the transparent image into fixed support/beam, cable and cradle regions. In the 660×224 export, cable columns are approximately 260–283, the moving yoke/cradle is within `(211,117,130,103)`, and the fixed support occupies the left 115 pixels. Reposition the food over the cradle centre around canvas x1518; confirm the final bounds in bundle QA.
- Integration and animation are deliberately pending under the task's stop-and-report instruction. The escape picture's source and provenance live with [the ending set](../endings/README.md).
