# ADR 0011: Open the city to a bright throat over glowing acid

> Status: accepted
> Date: 2026-11-03

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

[ADR 0009](0009-follow-the-humans-with-a-close-camera.md) laid a translucent dark violet haze over the throat and the swallowed city so the walkways, drawn brighter than their own colours, read over them, and pinned the acid band 140 pixels above the old bottom of the play area. [#141](https://github.com/garamsh/neocity/issues/141) then filled the space under every run of walkway with the wall of a building sunk in the acid. With both, the city screen at `909e8d2478529c9b0dee17f044173a1401758109` was walls and walkways in one dull violet: the throat's teeth and uvula, the distant city and the acid band were hidden behind them, and the acid showed nowhere at acid 0. The user found the screen far from the concept `search-mockup-v1.png` at commit `241124d5ba2dcef937c904623f1e5e3cb03fbcf5`, whose values fall in three layers: dark walkways on piles with lit rims in front, a bright saturated magenta throat with teeth, uvula and city silhouettes behind them, and glowing green acid with sunken trains along the bottom. [Issue #151](https://github.com/garamsh/neocity/issues/151) records the PM's decision to match that composition, with the rules and the cell layout unchanged.

Alternatives considered:

- **Keep the sunken buildings and brighten the backdrop around them.** The walls cover most of the play area under the walkways, so the backdrop would still show only in the few rows above the top walkway.
- **Keep the walkways brighter than the backdrop and only thin the haze.** The walkway faces and the throat then sit at nearly the same value, which is the flat look the user objected to.
- **Widen the drawn spacing between walkway rows.** It would open more backdrop between rows, but it breaks the camera's one scale from the scene to the screen that every check reads through `map_view`. With the walls gone, the rows at twice the scene's size already leave the backdrop open between them, so the spacing was left as it is.
- **New backdrop art.** The throat, city and acid pictures already hold the concept's teeth, uvula, towers and sunken trains; they were hidden by placement and haze, not missing.

## Decision

The space under the walkways is open: no building walls are drawn there, and every walkway deck but the planks, the tilted ground and the landmarks other than the pier stands on supports down to the walkway below or into the acid. The backdrop has no haze; the throat is raised so its teeth and uvula hang over the distant city, whose towers stand below them, and its near-white parts are pulled toward its magenta; the acid band glows above its own colours along the bottom of the play area at every acid level. The walkways and their supports are drawn darker than their own colours under a walked-on top edge lit warm and bright, so the screen's values run dark foreground, bright backdrop, brightest acid.

## Consequences

- The capture checks measure the three layers on the city frames: the walkway faces darker than the backdrop above them, and, where the acid sea in the scene does not cover the acid band, the band glowing green and brighter than the backdrop, at acid 0 as at every other level.
- A walkway reads over the backdrop by its lit top edge, not by its face, so the ground-contrast check of [#105](https://github.com/garamsh/neocity/issues/105) measures that edge.
- The sunken buildings' walls, fixtures and waterline leave the art set, and the check that the walkway's edge reads over those walls goes with them.
- The backdrop behind the map is brighter and busier than under the haze, so words on the map keep their opaque plates.
- The rows stay 32 scene pixels apart, so the camera's mapping and every check that reads through it are unchanged.

## Signs the decision was wrong

- Players lose a walkway's edge against the teeth or the bright centre of the throat, and the lit edge has to grow until the decks read as bright as before.
- The supports under the high walkways crowd the open backdrop at the top of the screen, and they have to be cut short or thinned.
- Players still find the rows too tight to read the backdrop between them, and the drawn spacing has to widen after all.
