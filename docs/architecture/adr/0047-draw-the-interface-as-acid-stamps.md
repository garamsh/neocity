# ADR 0047: Draw the interface as acid stamps

> Status: proposed
> Date: 2026-11-22

Supersedes ADR 0036's kit style for the interface: navy kit panels, a steel rim, rivets, drips and the pixel font. ADR 0036's layout and the base's composition stand.

## Context

[Issue #268](https://github.com/garamsh/neocity/issues/268) found two earlier interface finishes, the kit and a plainer one, read as a generic app laid over the game. The owner chose the third direction, "Acid stamp" (pick 3 on the issue): near-black purple ink behind every panel, a solid lime fill with a magenta offset shadow for what is chosen, and a pair of modern faces. The kit's pixel face, rivets and drips are removed from the interface; only the game's art stays pixel.

Alternatives considered:

- Keep the kit and recolour it. Rejected: the owner's objection was the kit's shape language, not its colours.
- A second set of primitives beside the kit's. Rejected: two sets would leave every screen choosing between them.
- A pixel-grid face with lime colours. Rejected by the owner's choice of faces.

## Decision

- The interface draws from one palette, `rendering::palette()`, whose tokens are the ink behind panels, the card, the lavender outline, lime, the purple shadow, magenta, cyan and cream, with the colours sampled from the chosen mockups.
- A few primitives in `rendering` take a state, default, focus or pressed: the choice card, the main action button and the key cap. The plate, the danger plate, the progress bar and the side fade take none. Each state is a row of data, a shadow, a ring, an edge, a fill and a drop, drawn by one routine. Focus is lime with dark ink words on a magenta offset shadow and a white ring; pressed drops 3 layout units, 4 screen pixels, on a shrunk shadow; a card at rest is dark with a thin lavender outline. Danger is a magenta outline with a triangle and a warning word, progress is a cyan bar with a number, and lime is never the danger colour.
- Titles, role names and outcome titles are Bricolage Grotesque ExtraBold; body, buttons, keys and HUD numbers are Chivo, Regular or Bold. Both are under the SIL Open Font License 1.1, whose text ships beside each file. Each of the three faces is a font resource under `project/levels/monster_mouth_escape/fonts/` that sets the variable font's weight, so the extension loads them through the resource loader.
- Every interface screen composes these primitives: the title, the choice of players, the end screens, the controls, How to play and Settings pages, the pause and leave screens, the exploration HUD and the base deck. Screens hold no colour literals and no per-screen style branches.
- Motion happens only on a state change: the press drop and the existing slide of the highlight.
- The kit's panel, button, slot and bar functions, the three frame pictures they loaded and the pixel font are removed.

## Consequences

Easier: one change of a token or a state row restyles every screen; a new screen is composed, not styled.

Harder: the faces are proportional and wider than the pixel face, so interface sizes step in 7 layout units, not 8, and no word is under 14.

Ruled out: a pixel or bitmap face and the old kit's ornaments in the interface.

## Signs the decision was wrong

A screen needs its own colour or a style branch to read; a third state row is added for one screen.
