# ADR 0044: Cap the interior zoom at one and a half times the street's

> Status: proposed
> Date: 2026-11-21

Supersedes the zoom range of the **Camera** decision in [ADR 0042](0042-explore-building-interiors-from-inside.md); every other decision of 0042 stands.

## Context

[Issue #281](https://github.com/garamsh/neocity/issues/281) found a character drawn nearly as tall as a small shop's room. Characters and map art go through one camera transform, once, and characters are drawn at scale 1 (`paint_actor` in `src/rendering/survival.cpp`); the cause is the zoom. ADR 0042 frames the lit floors at the largest whole zoom from 1 to 6 at which they fit, and a kitchen or noodle shop room, two cell rows (64 units) tall, fit at 6, three times the street's close zoom of 2. Its pictures are authored at that height, so they cannot be redrawn taller or stretched without a new art decision. Before this decision the street, kitchen, noodle shop, Cold Store and Aquarium service room measured at zooms 2, 6, 6, 3 and 4.

## Decision

The camera frames the bright floors together at the largest whole zoom from 1 to 3 at which they fit the play area, centred on them and kept to the building's sides. 3 is one and a half times the street's close zoom of 2, so a character is drawn at most one and a half times as tall inside any building as on the street, and as tall in every building whose lit floors fit at 3. Zoom stays a whole number. `rendering::inside_of` still frames the lit floors and `rendering::aim` chooses the zoom. Room art is neither redrawn nor stretched, and the area outside the room stays masked.

## Consequences

- A small shop's room fills less of the play area (576 of 1280 units wide for the kitchen), with the masked dark around it; characters stay the size they are in the next room.
- Two lit floors that fit only at zoom 2 are drawn at 2, still within the cap but smaller than a single room.
- `StairsCheck -- --interior-zoom` runs headless in `make test-survival`: it reads the zoom the camera chooses on the street and in each building and fails when a building exceeds 1.5 times the street's or two buildings differ.

## Signs the decision was wrong

- Players cannot read a small room's props at zoom 3.
- A building is drawn at a zoom above 3 or on a fraction.
- The art is redrawn at a room height where zoom 3 leaves a character too small.
