# Toxic Night map kit

Source ownership, exports and visual evidence for the interior and street map-art deliveries of [#276](https://github.com/garamsh/neocity/issues/276).

## Sources and exports

- Direction: user-selected F, Toxic Night, in [goal #268](https://github.com/garamsh/neocity/issues/268). The [street reference](https://github.com/garamsh/neocity/blob/35a53ee444965e50eba276c823cbf8273ba47d53/docs/art/concept-art/toxic-night/f-street.png) and [building reference](https://github.com/garamsh/neocity/blob/35a53ee444965e50eba276c823cbf8273ba47d53/docs/art/concept-art/toxic-night/f-building.png) establish material, color and pixel treatment; their geometry is not a collision specification.
- Status: candidate art under the selected direction; final creative acceptance and #281 integration remain pending.
- The [2026-11-20 PM scope](https://github.com/garamsh/neocity/issues/276#issuecomment-7042249960) replaces the earlier continuation. This pass redraws eighteen interior and street assets at a 144-pixel cell grid, proposed in [ADR 0043](../../../architecture/adr/0043-author-map-art-on-a-144-pixel-cell-grid.md).
- [Original HD sources](hd-sources/), [exact prompts](hd-sources/prompts.json) and [provenance](provenance.json) record six built-in ImageGen outputs. No paid provider or external upload was used. The earlier `source.png` and `backdrop-source.png` belong to the first kit and its stored runtime evidence; the current preparation command does not use them.
- Edit the eighteen `.aseprite` files here. Each contains its complete native runtime canvas; names retain the destination folder as a prefix.
- Export: `python3 tools/art/toxic_night.py`. It calls Aseprite 1.3 in batch mode and updates the HD manifest's source/output hashes and roles.
- Source assembly: `python3 tools/art/toxic_night.py --prepare`. This replaces native edits from `hd-sources/`, using nearest resampling, a 96-color undithered palette and binary alpha. Normal export preserves native edits.
- Decks have six 126-pixel bands; walls have 144/144/144/162-pixel bands; shop bands are 288 pixels high. Their twenty-cell period is 2880 pixels. Mirrored half-periods meet with identical edge columns. The deck face recedes below an 18-pixel contact rim.
- Rooms are separate single-floor panels: kitchen/noodle 864×288, cold-store/service 432×432, with the bottom edge as floor anchor. Each shows an enclosed room, rear furniture, foreground walking space, a door and a ladder. The cold-store and service panels are reusable per floor; distinct variants for each floor are not supplied.
- #281 must draw complete panels into destination world areas and align traversal equipment to actual routes. Painted doors and ladders are visual access cues, not proof of gameplay connections. The room art contains no sky, street or exterior shell.
- Canvas dimensions follow 1.5× the earlier kit, rounding half-pixels upward for city height (1103) and vending-machine height (266). Their physical destination proportions remain the renderer's responsibility.
- The terminal skyline uses a reflected pair in the lower half of its 3456×1728 transparent canvas, preserving fine source pixels. It is not a vertically enlarged copy of the earlier kit.
- No new runtime filenames or imports are introduced. `manifest.json` lists the eighteen HD paths separately from unchanged assets. The legacy map cutter selects its remaining inputs from that manifest.

## HD review evidence

| Evidence | Meaning |
|---|---|
| [Interior comparison](hd-interiors-comparison.png) | F-building reference above four individually usable room exports |
| [Street comparison](hd-street-comparison.png) | F-street reference beside an offline composition using the new street pieces |
| [HD kit board](hd-kit-board.png) | All eighteen current PNG exports |

- Recompose: `python3 tools/art/toxic_night_review.py --hd`. This uses current exports and the pinned reference commit, with nearest sampling and no display.
- These boards are asset evidence. They do not establish runtime movement, camera framing, collision contacts, shop lighting or floor-by-floor traversal.
- The room and street sources and every final export were visually inspected. Full runtime verification belongs with #281 and the PM's bundle checks.

## First-kit runtime evidence

The first-kit comparisons place the selected direction on the left and actual private-display captures on the right, both at half size with nearest sampling. They compare atmosphere and pixel treatment; mockup geometry is not collision data. Native captures remain in [captures/](captures/). They predate the HD redraw and do not show the current eighteen exports.

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
- The street comparison still shows the earlier bright throat, acid lake and support artwork. Its full-screen value balance is not a claim that the unfinished backdrop pass is accepted. UI, item badges and character art belong to another delivery.

## Delivery boundary

- Delivered in this pass: eighteen HD native Aseprite sources and PNGs, four enclosed floor panels, street skyline and terminal panorama, decks, walls, shop fronts, ladders and selected street props, source provenance, comparisons and proposed ADR 0043.
- Remaining art: throat, acid lake, aquarium backdrop/decks, support sheet, other passages and props, rubble, containers and hazards. They retain their prior density and need the next pass before #276 is complete. Mutant frames also retain their prior sources.
- #281 owns interior presentation and #200, #239 and #262 items 1–3. Runtime room placement, route alignment, lighting and framing have not been verified here.
- The PM must update the presentation responsibility document and ADR index before landing ADR 0043; these are outside the Worker's allowed paths.
- Full `make ci`, final runtime captures and bundle QA remain the PM's work. This delivery does not close #276 or its related issues.
