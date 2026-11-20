# ADR 0042: Explore building interiors from inside

> Status: proposed
> Date: 2026-11-20

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

[Issue #281](https://github.com/garamsh/neocity/issues/281), under [goal #268](https://github.com/garamsh/neocity/issues/268), records the user's decision that entering a building must feel like exploring its inside. [ADR 0039](0039-connect-multi-floor-interiors-to-a-rooftop-route.md) and [ADR 0031](0031-darken-interiors-and-fade-between-maps-in-the-presentation.md) show an occupied building as a cut-away: every floor's room band, the outside walls, the street and the sky stay on screen, the outside darkened by a veil. The decision also draws map art at a denser pixel grid ([#276](https://github.com/garamsh/neocity/issues/276)), but `rendering::Art` sizes a picture from its texture's pixels, so a finer replacement would draw smaller.

## Decision

Supersede the cut-away display of ADR 0039 and ADR 0031's veil. Their routes, floors, movement and screen-lock rules stand.

- **Only the inside.** While at least one actor is in the city and every actor in the city is inside the same building of the current map, the frame draws that building's inside and nothing else: no throat, city, acid, steam, spores, street, sky, other buildings, outer walls, obstacle glow, goal pointers or names. The backdrop is the background colour, and an opaque mask hides all of the map beyond the building's extent. The outside is not darkened; it is absent.
- **One bright floor.** The floor an actor stands on, and the shop's one room, are bright. Every other floor of the building is covered with the background colour at 0.88, so it is present but unread. The cover eases in and out on the real clock as an actor changes floor. An actor stands on the highest floor whose floor line is at or below its feet. When the two actors stand on different floors, both are bright.
- **Camera.** The camera frames the bright floors together at the largest whole zoom, from 1 to 6, at which they fit the play area, centred on them and kept to the building's sides. Zoom is always a whole number, so a picture pixel stays a whole square of screen pixels.
- **One actor outside.** When one actor is outside, or the actors are in different buildings, the screen is the outside view, not darkened, and no cut-away shows the whole building: the building shows the bright rooms its actors stand on, and every other floor of it is covered solid, so the player sees neither the places not yet visited nor the inside of a floor nobody is on. Nothing else of the outside changes, and the inside is not drawn full screen until every actor in the city is inside the one building.
- **Map art by world area.** The renderer sizes map art by the world area its picture covers, a table in `src/rendering/drawing.cpp`, and derives the picture's pixel density from its texture. A replacement of the same picture at a higher resolution keeps its size and position on screen. Interface art, characters and the title are outside this rule.

## Consequences

- The scene takes two views. `rendering::inside_of` decides which from the game view, and the camera, the city drawing, the backdrop, the glow canvas and the labels all follow it, carried in `rendering::View` so no other module changes.
- The measuring checks read `inside_view`, `inside_extent`, and each room's `lit` and `cover` instead of the veil's `interior_veil`.
- A floor is covered rather than hidden, so a floor's art stays in the draw call and costs the same as before.
- The finish pass runs only for a frame that notes the acid's waterline, so it is skipped inside, which is wanted: no acid is on screen.
- The decision keeps ADR 0039's floors, ladders, damage stages and collision, and the shared-screen rules of building entry, a lone actor, joint travel and the fade.

## Signs the decision was wrong

- A frame inside still draws any pixel of the street, sky, acid or another building.
- An actor inside is framed so that it cannot be seen, or the zoom between floors is not a whole number.
- A higher-resolution replacement of a map picture draws at another size.
- Players lose track of where they are after entering a building for lack of any outside cue.
