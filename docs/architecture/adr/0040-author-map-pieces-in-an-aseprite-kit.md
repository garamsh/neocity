# ADR 0040: Author map pieces in an Aseprite kit

> Status: proposed
> Date: 2026-11-18

The map kit's source ownership, pixel grid and interior artwork selection.

## Context

[Issue #276](https://github.com/garamsh/neocity/issues/276), under [goal #268](https://github.com/garamsh/neocity/issues/268), applies the selected Toxic Night direction to the maps. The old map cutter duplicates asset ownership in its cut list and overwrites the newer street repaint (#259). Multi-floor interiors currently select pictures by list position, causing the aquarium service room to inherit cold-store art.

## Decision

Keep editable map pieces in Aseprite files alongside the recorded generation sources and export them through Aseprite's batch CLI. The HD manifest records each runtime file's owning source, source digest, output digest and export command. The legacy cutter selects only entries still owned by its legacy source directory; adopting a new source removes that file from the old cutter's work without a second exclusion list.

Use the existing 96-native-pixel cell and native nearest sampling at the close camera. Retain the renderer's twenty-cell deck and wall bands and its floor-aligned room bands. Shared source modules provide corroded violet masonry, selective amber lights, pink neon and acid-green leaks. Each occupied building selects its own interior picture by its canonical object name, independently of the order of interiors. An unknown building has no fallback artwork.

This first kit replaces the street skyline, terminal scenery, deck and wall bands, shop fronts, kitchen and noodle interiors, cold-store interior and service-room interior, and selected props. Existing throat, acid lake, aquarium backdrop, aquarium decks, other props and dangers remain for the next art pass. Collision, map layout, camera and movement remain unchanged.

## Consequences

- Artists can edit the native Aseprite sources and export without rerunning image generation or atlas preparation.
- The manifest is the asset ownership record; source ownership changes accompany replacement artwork.
- Renaming or adding an interior building requires updating its explicit artwork assignment.
- The legacy cutter no longer owns the repainted terminal or collapsed kiosk.
- The atlas preparation command intentionally replaces native sources; the normal export preserves Aseprite edits.
- Existing stair-contact, shop-light, camera and empty-container findings remain follow-up work. This decision does not relax their checks.

## Signs the decision was wrong

- Exporting the same Aseprite files changes runtime pixels or the manifest.
- A legacy export overwrites a file owned by a different source.
- Reordering interiors changes their artwork, or a newly added building silently inherits another building's room.
- Kit pieces require a different collision floor or smooth filtering to meet.
