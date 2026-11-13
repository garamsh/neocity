# Day-one HD environment art

The native 1920×1080 environment assets and reference composite for [issue #210](https://github.com/garamsh/neocity/issues/210), under [goal #207](https://github.com/garamsh/neocity/issues/207).

## Status and source

- Connected to the city renderer by #212 under [ADR 0025](../../../../docs/architecture/adr/0025-draw-the-city-with-native-hd-art.md). The kitchen, interface, icons and non-city figures retain their existing `art/` resources. Creative acceptance remains the PM/user decision.
- Newly drawn with built-in ImageGen. The search mockup at `241124d5ba2dcef937c904623f1e5e3cb03fbcf5` supplies style, palette, shapes and lighting only; none of its pixels is adopted into an asset.
- [Manifest](manifest.json): every environment PNG's dimensions, role, source hash and retouch status. [Provenance](../../../../docs/art/production/day-one-hd-provenance.txt): exact prompts, original generated sources and ImageMagick processing/composition operations.
- The environment manifest owns #210’s resources. `characters/manifest.json` separately owns #211’s city poses, with a shared pivot at (128, 204). UI and icons remain in `art/`.

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

## Runtime checks

- The renderer converts backgrounds to layout units with `1 / ART_SCALE`, and cell art and city actors with `1 / (2 * ART_SCALE)`. The close camera and canvas restore one native picture pixel per 1080p pixel. Other output sizes follow Godot’s fractional canvas scale.
- `python3 tools/art/pixelize.py check project/levels/monster_mouth_escape/art_hd` checks both manifests for dimensions, hashes and native fine-edge detail. The same check runs after the legacy set in `make test`. Generated HD art keeps its own colours and is not subject to the legacy palette or mockup-source requirements.
- The existing twenty-column repeat, floor rows and exposed deck contours remain. Tilted cells now draw level surfaces at their collision floors.

## Evidence

- [Runtime entry at 1080p](../../../../docs/architecture/evidence/survival/212-city-entry-1080.png) and [mockup beside runtime](../../../../docs/architecture/evidence/survival/212-mockup-beside-city.png): seed 11, day one, both actors just set out at the entrance; the original is resized to 1920×1080 on the left, the unscaled native capture is on the right. The existing map retains the loft offset and both actors initially share the entrance.

- [Native target](../../../../docs/art/production/day-one-hd-target.png): environment assets only, assembled at one image pixel per screen pixel; no characters or HUD, and no runtime behavior implied.
- [Comparison](../../../../docs/art/production/day-one-hd-beside-mockup.png): original mockup on the left, normalized to 1920×1080 for comparison only; new target on the right at native resolution.
- [Join sheet](../../../../docs/art/production/day-one-hd-seams.png): rightmost 96 pixels followed by leftmost 96 pixels of each deck band, then each wall band, at native size. Shorter bands are centered on 108px rows.
- Target placements and crops are recorded in the provenance, separately from the later runtime integration.

## System assets for goal #218

The resources for [issue #222](https://github.com/garamsh/neocity/issues/222) are newly drawn with built-in ImageGen, using the mockup's palette and panel treatment and the HD chef's density. They are not connected to runtime drawing in this PR. Creative acceptance remains with the user.

| Folder | Contract |
|---|---|
| `mutants/` | Mutated rat and former human citizen, each idle plus four walk frames. Right-facing, mirror for left. 256×224 transparent canvases, pivot (128, 204), foot baseline y=204; suggested walk playback 6 fps. Human fits 150×170; rat fits 184×96. |
| `obstacles/` | Static acid vent and flesh spikes, each 224×224; bottom pivot (112, 208). |
| `base/` | Refrigerator closed/open, 288×288; bottom pivot (144, 272). |
| `containers/` | Crate stack, locker and shelf closed/open pairs. Both states share their canvas and bottom pivot. Individual sizes and pivots are in the manifest. |
| `icons/` | Seven ingredients and eleven dishes matching the legacy filenames; 84×84 transparent canvases with visible art fitted within 76×76. |
| `ui/` | Empty inventory/search/fridge grids, reusable panel and slot, gold selection outline, action prompt, weight trough, three top HUD plates, sun, clock, acid droplet and gauge fill. No raster words or numbers. |

- The root [manifest](manifest.json) also owns these 53 PNGs; existing environment entries and the separate character manifest retain their ownership. Every new file records dimensions, hash and source crop or composition.
- Draw at native pixels with nearest sampling. The mutants use the same pivot contract as the HD actors. World objects expose bottom pivots; their visible bounds are art, not collision rectangles.
- UI frames carry `slice_margins_ltrb` in the manifest. Keep the four corners at native size and repeat or nearest-stretch only the intervening edges and center. `panel.png`: 16px; `slot.png`: 14px; `selection.png`: 16px; `action_prompt.png`: 20px; HUD plates: 28px left/right and 12px top/bottom; `weight_bar.png`: 10px. These are margins on each side, not a resize factor.
- The assembled grid images are optional fixed layouts: inventory 6×4, search 4×3, fridge 6×5. Slots are 96×96, start at (18, 80) and advance by 100px in either direction. Header and footer spaces are blank for code-drawn labels and values. Runtime inventory capacity is decided by the feature owner; reusable frames support other layouts.
- [Contact sheet](../../../../docs/art/production/system-hd-contact-sheet.png): every new resource, all mutant frames and native chef reference. [Provenance](../../../../docs/art/production/system-hd-provenance.txt) and [exact prompts](../../../../docs/art/production/system-hd-prompts.json) retain the source and processing record.
