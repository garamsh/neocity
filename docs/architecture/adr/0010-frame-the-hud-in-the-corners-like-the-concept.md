# ADR 0010: Frame the HUD in the corners like the concept

> Status: accepted
> Date: 2026-11-03

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

Since [#107](https://github.com/garamsh/neocity/issues/107) the interface has been flat, drawn in code and set in Inter: rounded panels with a one-pixel edge and a soft shadow. In the city it showed ten cards at all times, a top band of 62 pixels and a bottom band of about 180, a third of the screen, holding every key, the pack's notes and a NEXT bar; at base three large panels listed every recipe. The user found the screen crowded and unlike the concept `search-mockup-v1.png` at commit `241124d5ba2dcef937c904623f1e5e3cb03fbcf5`, and [issue #119](https://github.com/garamsh/neocity/issues/119) records the PM's decision: information in the four corners, what to do shown where it happens, one short goal line, details only on demand, and the concept's look, dark pixel panels with a thick light rim, a pixel font and pixel icons. The rules and their numbers stay as they are.

Alternatives considered:

- **Keep the flat panels and only remove cards.** It frees the screen but leaves the interface looking unlike the concept, which the user named as the other half of the problem.
- **Nine-slice panel pictures in the art set.** The frames are rectangles of a few flat colours, which code draws on whole pixels exactly; pictures would add files, a stretch rule and a generation step for no difference on the screen.
- **A pixel font for numbers and titles only, Inter for sentences.** Two typefaces side by side read as two styles, and the concept sets every word in one pixel face.
- **A typeface on a strict pixel grid, such as Press Start 2P.** It is crisp only at whole multiples of its grid and wide enough that the goal line and the pause screen's sentences no longer fit at 13 pixels and up.

## Decision

The interface is drawn in pixel panels, filled dark inside a thick light rim between dark lines, lit from above, with cut corners, all in code on whole pixels, and set in Pixelify Sans in three weights. The city shows the day, the acid and the time in the top corners, the two actors, the bag and the action in the bottom corners, a key cap over the head of a human whose interact key acts now, one goal line under the acid and the log as notes that fade; base shows one task card in place of its panels; every key, every recipe and what was eaten are on the pause screen.

## Consequences

- The city's interface covers two bands, 48 pixels at the top and 84 at the bottom, with the goal line and the fading notes over the play area, where the cards covered about 242 pixels before.
- A player learns the keys from the pause screen, which the Esc hint names, and the recipes from the recipe book there; the screen no longer names every key for the current place.
- Pixelify Sans is drawn with antialiasing and its outlines are not on a strict pixel grid, so its letters are slightly soft-edged at every size; its ligatures are turned off, since its `fi` reads as `A`.
- Changing words keep being listed with the width their place allows, so the checks measure them in the new face.
- The source names a player reads come from one function, so the map can use the same words.

## Signs the decision was wrong

- Players miss what a key does at a place because the bottom corner shows only the interact key and the pause screen holds the rest.
- The fading notes go unread, and events such as a burn or a destroyed passage are missed.
- The pixel face reads poorly at 13 pixels on the goal line, and the small words have to grow until the bands are as tall as the cards were.
