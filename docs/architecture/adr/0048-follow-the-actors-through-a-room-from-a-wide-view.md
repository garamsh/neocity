# ADR 0048: Follow the actors through a room from a wide view

> Status: proposed
> Date: 2026-11-22

Supersedes the Camera decision of [ADR 0042](0042-explore-building-interiors-from-inside.md) (its framing of the lit floors, its zoom range already replaced by [ADR 0044](0044-cap-the-interior-zoom.md)) and all of [ADR 0044](0044-cap-the-interior-zoom.md); every other decision of 0042 stands.

## Context

[Issue #268](https://github.com/garamsh/neocity/issues/268) chose direction A for the inside of a building: a wide view close to the street's zoom, a soft oval darkening of the screen's edges, the door the actors came in by drawn open onto the street, and a camera that follows the actors so the room scrolls past, in place of the fixed, centred room view of ADR 0042 and ADR 0044. A room's picture is 192 by 64 scene pixels (the kitchen and the noodle shop) or 96 by 96 (a floor of the Cold Store and of the service room), narrower than the screen at any whole zoom that keeps a character near the street's size, so the room cannot fill the screen and the camera's centre, not its edges, is what the room's bounds hold. The street already follows its actors with a camera that aims at them, eases toward the aim and keeps to the map's bounds; the two differ only in the bounds.

## Decision

- Inside a building the camera aims, eases and steps its zoom exactly as on the street: the same target, a little ahead of the way the actors face, and the same two whole zooms, 2 (the street's close zoom) and 1. A character is as tall inside a building as on the street. The building's extent is data to that one step: the camera's centre is kept within it, so the view past the room's sides, top and bottom shows only the darkness around the room, and a room scrolls across the screen as the actors walk through it.
- Two actors in the city look ahead together, by the share of the look-ahead their facings have in common, on the street as inside; one actor looks ahead as before. The look-ahead is 48 scene pixels, so that it does not carry the centre to a 192-pixel room's side before the actors are halfway along it.
- A soft oval vignette is drawn over the whole screen while the view is only a building's inside, under the interface: clear toward the middle, darkening to the background colour past the screen's corners.
- The door of the room picture at the building's lowest floor is drawn open: its opening shows the street, cut from the city layer with the acid's glow at its foot, and the door leaf hangs swung back at its hinge. The opening is a rectangle per room picture, in `src/rendering/survival.cpp`. No new art is made.
- Rooms keep their two rows; the three-row room is not drawn.

## Consequences

- The room is drawn at 384 of 1280 layout units across the screen for a shop (576 under ADR 0044), with the darkness and the vignette around it.
- `StairsCheck -- --interior-zoom` reads that each building is drawn at the street's zoom, and no longer the 1.5 cap or the centring of the lit rooms.
- `InteriorCheck` walks both humans along the kitchen and reads that the camera's centre differs between the door end, the middle and the far side, stays within the room, keeps the humans within a band around the screen's middle, and that an open door and the vignette are drawn inside and neither on the street.
- A pair of actors on the street is also framed ahead of the way they face; the screen lock's room for two actors still holds.
- The multi-floor buildings are drawn whole at the street's zoom while their floors fit it, and at the zoom of 1 when two actors stand floors apart that do not.

## Signs the decision was wrong

- A shop's room at the street's zoom is too small for players to read its props.
- The look-ahead of 48 pixels does not read as a follow on the street.
- The door's street cut shows a seam against its frame in captures.
