# ADR 0048: Follow the actors through a room that fills the screen

> Status: proposed
> Date: 2026-11-22

Supersedes the Camera decision of [ADR 0042](0042-explore-building-interiors-from-inside.md) (its framing of the lit floors, its zoom range already replaced by [ADR 0044](0044-cap-the-interior-zoom.md)) and all of [ADR 0044](0044-cap-the-interior-zoom.md); every other decision of 0042 stands.

## Context

[Issue #268](https://github.com/garamsh/neocity/issues/268) chose direction A for the inside of a building: the room fills the screen's width and about half its height, a soft oval vignette darkens only the edges, the door the actors came in by is drawn open onto the street, and the camera follows the actors so the room scrolls past, in place of the fixed, centred room view of ADR 0042 and ADR 0044. The mockup draws a character about a quarter of the room's height.

The room pictures and the characters are on different scales. A shop room is 192 by 64 layout units, two cells tall; a character's frame is 74.7 layout units, and the opaque body about 65, so at the street's zoom a character is as tall as the room, and ADR 0044 capped the zoom to keep it from being taller. A zoom that fills the screen with a 64-unit room is 8, at which a character drawn whole would stand four times as tall on the screen as on the street. The street already follows its actors with a camera that aims at them, eases toward the aim and keeps to the map's bounds; the two differ only in the bounds, the zoom and, here, the actors' size.

## Decision

- Inside a building the camera aims, eases and steps its zoom exactly as on the street, with the building's extent as its bounds: the view stays within the room's sides, and centres on a room narrower than itself. The zoom is the whole number, from 1 to 8, that brings the rooms the actors stand on nearest to 71% of the screen's height: 8 for a shop's room, 1536 by 512 units, about one and a fifth screens wide, so it scrolls. The look-ahead, the actors' distance behind the middle of the screen, is 160 layout pixels at every zoom; two actors inside a building look ahead together as far as they face the same way, and on the street a pair is framed centred as before.
- Actors are drawn smaller inside a building: by the street's close zoom over the camera's, so a character is as tall on the screen as on the street, one picture pixel to a canvas pixel at every zoom and so not resampled, and a quarter of a shop room's height. Their bodies, collision, badges and tags are unchanged, and the labels follow the drawn size. A character shrinks on entering a building, from the street's zoom of 2 to the room's 8, in the same size on the screen but in a room four times its height.
- A soft oval vignette is drawn over the whole screen while the view is only a building's inside, under the interface: clear toward the middle, darkening to the background colour past the screen's corners.
- The door of the room picture at the building's lowest floor is drawn open: its opening shows the street, cut from the city layer with the acid's glow at its foot, and the door leaf hangs swung back at its hinge. The opening is a rectangle per room picture, in `src/rendering/survival.cpp`. No new art is made.
- The room picture is drawn over the map's decks that reach into it, which it draws for itself.
- Rooms keep their two rows; the three-row room is not drawn.

## Alternatives left for later

- Redraw the rooms at the world scale of the characters: room art about four times taller than 192 by 64, some fourteen rows tall for a character a quarter of its height, with the survival library's building sizes changed to match. It would make the shrinking of the actors unnecessary and is the faithful answer if the rooms are ever redrawn; it is art and content work, and sauce kept the two rows.
- Zoom alone, with the actors at full size: the room fills the screen but a character is as tall as the room, the problem ADR 0044 capped.

## Consequences

- The room fills 71% of the screen's height and the screen's width, scrolling 256 pixels across the kitchen and the noodle shop; the Cold Store's floors and the service room are drawn at the zoom their lit floors call for, 5 and 3 or less, with the actors the street's size on the screen.
- `StairsCheck -- --interior-zoom` reads that the chef is drawn no taller on the screen in any building than on the street, equally tall in all of them, and each building at the street's zoom or nearer.
- `InteriorCheck` walks both humans along the kitchen and reads that the camera's centre differs between the door end, the middle and the far side, that the view stays within the room's sides, that the humans stand within a band around the screen's middle in the middle of the room and within the screen at its ends, and that an open door and the vignette are drawn inside and neither on the street.
- The map's ladders are drawn over a room at the room's zoom, one cell, 256 pixels, wide each, though the room picture draws its own; this is left as it was.

## Signs the decision was wrong

- Players do not read a character a quarter of the room's height as the same character as on the street.
- The door's street cut shows a seam against its frame in captures.
- A building's lit floors zoom the camera in and out as actors change floors, disturbing the follow.
