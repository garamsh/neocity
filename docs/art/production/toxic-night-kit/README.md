# Toxic Night map kit

Source ownership, exports and visual evidence for the first map-art delivery of [#276](https://github.com/garamsh/neocity/issues/276).

## Sources and exports

- Direction: user-selected F, Toxic Night, in [goal #268](https://github.com/garamsh/neocity/issues/268). The [street reference](https://github.com/garamsh/neocity/blob/35a53ee444965e50eba276c823cbf8273ba47d53/docs/art/concept-art/toxic-night/f-street.png) and [building reference](https://github.com/garamsh/neocity/blob/35a53ee444965e50eba276c823cbf8273ba47d53/docs/art/concept-art/toxic-night/f-building.png) are on the art-direction branch; they are not captures of this kit.
- Status: candidate runtime assets under the selected direction; final creative acceptance remains pending.
- [Provenance](provenance.json), [foreground prompt](prompt.txt) and [backdrop prompt](backdrop-prompt.txt) record built-in ImageGen generation. No paid provider or external upload was used.
- Edit the `.aseprite` files here. Each contains a native runtime canvas; names retain the destination folder as a prefix. The repeated foreground bands share modules from `source.png`; the skyline comes from the middle strip of `backdrop-source.png`. The latter sheet's throat and acid were not adopted.
- Export: `python3 tools/art/toxic_night.py`. It calls `aseprite --batch <source.aseprite> --save-as <runtime.png>` for every owned file and updates `art_hd/manifest.json` with source/output digests and roles.
- Initial assembly only: `python3 tools/art/toxic_night.py --prepare`. This replaces the native Aseprite files from the recorded atlases and discards subsequent native edits. Normal export leaves those edits intact.
- Grid: 96 native pixels per map cell, one native pixel per canvas pixel at close zoom; nearest sampling and hard alpha. Decks have six 84-pixel bands, walls retain their 96/96/96/108-pixel bands and twenty-cell period. Deck faces recede below the unmodified contact rim.
- Four separate room pictures belong to Kitchen Shop, Noodle Shop, Cold Store and Aquarium Service Room. The service room uses glass tanks and pumps, independently of the cold-store lockers.
- The legacy map cutter now selects its inputs from the manifest. It never processes entries owned by this kit or the earlier street repaint.

## Review evidence

The comparisons place the selected direction on the left and actual private-display captures on the right, both at half size with nearest sampling. They compare atmosphere and pixel treatment; mockup geometry is not collision data. Native captures remain in [captures/](captures/).

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

- Delivered: eighteen native Aseprite sources and PNG exports, street skyline and terminal scenery, deck and wall bands, shop fronts, four interior pictures, selected street props, explicit interior assignment and #259 ownership protection.
- Retained art: throat, acid lake, aquarium backdrop and decks, support sheet, broken passages, bags, rubble, containers and dangers. These need the next kit pass before #276's full art criterion is met.
- Follow-up: #200's complete actual-movement/contact-line evidence and failing control; #239's per-shop lighting verdict and unlit control; #262's empty-container rejection, eastern camera framing and seams/label polish.
- Full `make ci`, complete final capture checks and bundle QA remain the PM's work. This delivery does not close #276 or its related issues.
