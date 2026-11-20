# Toxic Night map kit

Source ownership, exports and visual evidence for the static map-art deliveries of [#276](https://github.com/garamsh/neocity/issues/276).

## Sources and exports

- Direction: user-selected F, Toxic Night, in [goal #268](https://github.com/garamsh/neocity/issues/268). The [street reference](https://github.com/garamsh/neocity/blob/35a53ee444965e50eba276c823cbf8273ba47d53/docs/art/concept-art/toxic-night/f-street.png) and [building reference](https://github.com/garamsh/neocity/blob/35a53ee444965e50eba276c823cbf8273ba47d53/docs/art/concept-art/toxic-night/f-building.png) establish material, color and pixel treatment; their geometry is not a collision specification.
- Status: candidate art under the selected direction; final creative acceptance and runtime verification remain pending.
- The [2026-11-20 PM scope](https://github.com/garamsh/neocity/issues/276#issuecomment-7042249960) replaces the earlier continuation. The first HD pass redraws eighteen interior and street assets; the next pass adds three backgrounds and the final static pass adds seventeen decks, supports, passages, props, rubble, containers and hazards at the 144-pixel cell grid in [ADR 0043](../../../architecture/adr/0043-author-map-art-on-a-144-pixel-cell-grid.md).
- [Original HD sources](hd-sources/), [exact prompts](hd-sources/prompts.json) and [provenance](provenance.json) record thirteen built-in ImageGen outputs. No paid provider or external upload was used. The earlier `source.png` and `backdrop-source.png` belong to the first kit and its stored runtime evidence; the current preparation command does not use them.
- Edit the thirty-eight `.aseprite` files here. Each contains its complete native runtime canvas; names retain the destination folder as a prefix.
- Export: `python3 tools/art/toxic_night.py`. It calls Aseprite 1.3 in batch mode and updates the HD manifest's source/output hashes and roles.
- Source assembly: `python3 tools/art/toxic_night.py --prepare`. This replaces native edits from `hd-sources/`, using nearest resampling, a 96-color undithered palette and binary alpha. Normal export preserves native edits.
- Decks have six 126-pixel bands; walls have 144/144/144/162-pixel bands; shop bands are 288 pixels high. Their twenty-cell period is 2880 pixels. Mirrored half-periods meet with identical edge columns. The deck face recedes below an 18-pixel contact rim.
- Rooms are separate single-floor panels: kitchen/noodle 864×288, cold-store/service 432×432, with the bottom edge as floor anchor. Each shows an enclosed room, rear furniture, foreground walking space, a door and a ladder. The cold-store and service panels are reusable per floor; distinct variants for each floor are not supplied.
- #281 must draw complete panels into destination world areas and align traversal equipment to actual routes. Painted doors and ladders are visual access cues, not proof of gameplay connections. The room art contains no sky, street or exterior shell.
- Canvas dimensions follow 1.5× the earlier kit, rounding half-pixels upward for city height (1103) and vending-machine height (266). Their physical destination proportions remain the renderer's responsibility.
- The terminal skyline uses a reflected pair in the lower half of its 3456×1728 transparent canvas, preserving fine source pixels. It is not a vertically enlarged copy of the earlier kit.
- The background-first continuation adds native sources for throat (1517×1034), acid (2880×761) and aquarium (2880×1620), with their prior runtime filenames and import metadata. Aspect ratios round within one output pixel. The acid source is normalized in two bands to keep its far waterline at one third of the canvas; neither ruin tips nor the water is cut away.
- `python3 tools/art/toxic_night.py --prepare-backgrounds` rebuilds only these three native sources, then exports all thirty-eight kit entries. `--prepare` rebuilds the entire kit. No new runtime filenames or imports are introduced. `manifest.json` lists the thirty-eight HD paths separately from unchanged assets. The legacy map cutter selects its remaining inputs from that manifest.

- Final static preparation: `python3 tools/art/toxic_night.py --prepare-props` rebuilds only the seventeen remaining static sources, then exports the kit. [Layout recipe](hd-sources/props-layout.json) pins the previous revision, atlas crops, canvas extents and visible bounds. Native edits survive normal export; preparation replaces them.
- Verify source digests, all thirty-eight Aseprite-to-PNG round trips, seventeen static canvases, binary alpha, floor anchors, normalized pivots and aquarium contact/repeat columns: `python3 tools/art/toxic_night.py --check`.
- Static canvases scale by 1.5 with upward rounding. Container and hazard pivots scale by the same factor; normalized floor contacts stay fixed. The vendor fits proportionally inside its previous visible bounds, keeping its bottom anchor. Support pieces keep their normalized positions; the warning pillar uses abstract ticks instead of letters.
- The aquarium deck is 576×126, with an opaque two-pixel top contact rim at row zero and a reflected half-period for matching outer edge columns. The support canvas is 2880×824. No runtime filenames or import settings change.

## HD review evidence

| Evidence | Meaning |
|---|---|
| [Interior comparison](hd-interiors-comparison.png) | F-building reference above four individually usable room exports |
| [Street comparison](hd-street-comparison.png) | F-street reference beside an offline composition using the current street pieces and new throat/acid layers and redrawn support sheet |
| [HD kit board](hd-kit-board.png) | All thirty-eight current PNG exports |
| [Background comparison](hd-backgrounds-comparison.png) | Earlier background exports beside the three redraws, normalized with nearest sampling |
| [Container comparison](hd-containers-comparison.png) | F-building beside all six open/closed container states |
| [Hazard and deck comparison](hd-hazards-deck-comparison.png) | F-street beside aquarium deck, collapsed span end, hazards and rubble |
| [Support and prop comparison](hd-supports-props-comparison.png) | F-street beside supports, damaged passages, vendor, backpack and drip |
| [Static before/after](hd-props-before-after.png) | All seventeen earlier exports enlarged by nearest sampling beside the new exports |
| [Support details](hd-supports-detail.png) | Earlier pixels enlarged beside new lamp, pier and warning-pillar crops |
| [Background details](hd-backgrounds-detail.png) | Matching world-sized crops: earlier 96px regions expanded to 144px beside native 144px redraw regions, both shown at 3× |

- Recompose: `python3 tools/art/toxic_night_review.py --hd`. This uses current exports and the pinned reference commit, with nearest sampling and no display.
- These boards are asset evidence. They do not establish runtime movement, camera framing, collision contacts, shop lighting or floor-by-floor traversal.
- The seventeen final static exports and all four new source atlases were visually inspected, including enlarged vendor and support-lamp details. Full runtime verification belongs with #281 and the PM's bundle checks.

## First-kit runtime evidence

The first-kit comparisons place the selected direction on the left and actual private-display captures on the right, both at half size with nearest sampling. They compare atmosphere and pixel treatment; mockup geometry is not collision data. Native captures remain in [captures/](captures/). They predate the HD redraw and do not show the current thirty-eight exports.

| Evidence | Subject |
|---|---|
| [Street comparison](street-comparison.png) | Starting street, new skyline, decks, walls and props |
| [Cold-store comparison](cold-store-comparison.png) | Three floor-aligned rooms with refrigeration lockers |
| [Service-room comparison](service-room-comparison.png) | Two rooms with aquarium tanks and pumps |
| [Kitchen comparison](kitchen-comparison.png) | Occupied kitchen loft with stove and shelves |
| [Noodle comparison](noodle-comparison.png) | Occupied noodle loft with bowls and boiler |
| [Kit board](kit-board.png) | All eighteen exports, shown independently of the runtime |

- Runtime code and artwork: `ff5debed412fce1a780cd0a1a8db908be361d3a8`; the continuation adds the shop tour and evidence tooling, without redrawing the kit. Seed 11, two human key sets, native 1920×1080 canvas, pinned scene clock.
- Full map tour: `godot --path project --display-driver wayland --audio-driver Dummy --disable-vsync --fixed-fps 60 --main-loop StairsCheck -- --map-art-tour`. Both maps passed on days 1, 5 and 10, including paired travel, drawn collision floors, three cold-store rooms and two service rooms.
- Shop tour: the same command with `--kit-interiors` instead of `--map-art-tour`, on day 1. Both humans reach each shop by its ladder; the check reads the picture actually drawn. This is an evidence route, not an additional movement model.
- Run either tour only in a private `kwin_wayland --virtual --width 1920 --height 1080 --no-lockscreen --no-global-shortcuts` session, with a private runtime socket and isolated `XDG_DATA_HOME` and `XDG_CONFIG_HOME`, after checking the one-minute load is below 6. No user-display window was opened.
- Recompose from the stored captures: `python3 tools/art/toxic_night_review.py`. To import fresh tour output, add `--captures <user-data-directory>/survival`. Reference PNGs are read from the recorded art-direction commit, without downloading or uploading files.
- The first-kit runtime street comparison still shows the earlier throat, acid lake and support artwork; the HD street comparison uses the new throat, acid and support sheet. Its offline composition does not establish final runtime lighting or creative acceptance. UI, item badges and character art belong to another delivery.

## Delivery boundary

- Delivered across the HD passes: thirty-eight native Aseprite sources and PNGs, covering all static map art. This final static pass adds seventeen exports, four built-in generated atlases, exact prompts, the layout recipe, provenance and offline comparisons.
- Remaining art: no static map pieces remain at the earlier density. Ten mutant animation frames remain at their prior density for a separate animation review; this delivery keeps the seventeen static replacements together. Characters, UI, title, icons and base assets are unchanged.
- #281 owns interior presentation and #200, #239 and #262 items 1–3. Runtime room placement, route alignment, lighting, container use and framing have not been verified here. The current renderer uses fixed world footprints for the redrawn map pieces and hazards; the container PNGs remain reusable art exports.
- This continuation consolidates only the two PM-authorized density-status sentences in `presentation.md`; ADR 0043 and the architecture index are unchanged.
- Full `make ci`, final runtime captures and bundle QA remain the PM's work. This delivery does not close #276 or its related issues.
