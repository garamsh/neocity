# Day-one HD environment art

The native 1920×1080 environment assets and reference composite for [issue #210](https://github.com/garamsh/neocity/issues/210), under [goal #207](https://github.com/garamsh/neocity/issues/207).

## Status and source

- Candidate art for PM review; these files are not connected to the game. The existing `art/` resources remain the runtime set.
- Newly drawn with built-in ImageGen. The search mockup at `241124d5ba2dcef937c904623f1e5e3cb03fbcf5` supplies style, palette, shapes and lighting only; none of its pixels is adopted into an asset.
- [Manifest](manifest.json): every environment PNG's dimensions, role, source hash and retouch status. [Provenance](../../../../docs/art/production/day-one-hd-provenance.txt): exact prompts, original generated sources and ImageMagick processing/composition operations.
- Characters, UI and icons belong to separate issues. This manifest covers only the environment delivered by #210.

## Native layout

| Resource | Contract |
|---|---|
| `backgrounds/` | Throat 1011×689; city 1920×735; acid 1920×507. Transparent city sky and lake skyline keep the layers separate. |
| `walkway/deck.png` | Six full-width 1920×84 bands in the existing role order: kitchen roof, tower roof, loft, upper walkway, lower walkway, pier. |
| `world/building_wall.png` | Four full-width bands, heights 96, 96, 96 and 108. |
| `world/shop.png` | Two full-width 192px bands, kitchen front at left and tower front at right. |
| `walkway/structure.png` | 1920×549 support layer; target origin (0, 292). |
| Other `world/` and `ground/` PNGs | Original filenames and roles, native dimensions 1.5× the old set. See the manifest for individual dimensions. |

- Draw at native pixels with nearest sampling; a map cell is 96 picture pixels at this target density. Deck and wall bands repeat horizontally across their full 1920px period. Their outer columns match through reflected source strips; the cells inside a band are contiguous portions, not independent repeating tiles.
- Prop canvases retain padding and centered silhouettes. The ferry's complete silhouette fits inside its 384×150 canvas without enlargement. The ladder repeats vertically; the other props do not repeat.
- PNG import metadata uses lossless compression with mipmaps disabled. No gameplay, layout, font or runtime binding is changed here.
- Images contain no text except the ACID marking on the support pillar. The kiosk display uses abstract bars.

## Evidence

- [Native target](../../../../docs/art/production/day-one-hd-target.png): environment assets only, assembled at one image pixel per screen pixel; no characters or HUD, and no runtime behavior implied.
- [Comparison](../../../../docs/art/production/day-one-hd-beside-mockup.png): original mockup on the left, normalized to 1920×1080 for comparison only; new target on the right at native resolution.
- [Join sheet](../../../../docs/art/production/day-one-hd-seams.png): rightmost 96 pixels followed by leftmost 96 pixels of each deck band, then each wall band, at native size. Shorter bands are centered on 108px rows.
- Target placements and crops are recorded in the provenance, separately from the later runtime integration.
