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

The comparison files in this directory place the selected direction beside actual private-display captures. They compare atmosphere and pixel treatment; mockup geometry is not collision data. The kit board shows every exported asset independently of the runtime.

## Delivery boundary

- Delivered: eighteen native Aseprite sources and PNG exports, street skyline and terminal scenery, deck and wall bands, shop fronts, four interior pictures, selected street props, explicit interior assignment and #259 ownership protection.
- Retained art: throat, acid lake, aquarium backdrop and decks, support sheet, broken passages, bags, rubble, containers and dangers. These need the next kit pass before #276's full art criterion is met.
- Follow-up: #200's complete actual-movement/contact-line evidence and failing control; #239's per-shop lighting verdict and unlit control; #262's empty-container rejection, eastern camera framing and seams/label polish.
- Full `make ci`, complete final capture checks and bundle QA remain the PM's work. This delivery does not close #276 or its related issues.
