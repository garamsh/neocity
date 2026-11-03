# ADR 0016: Teach each concept once in a tip that never holds the run

> Status: accepted
> Date: 2026-11-04

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

A first run showed every concept at once on day one or only afterwards, in the day's result, as QA's playtest in [#125](https://github.com/garamsh/neocity/issues/125) records: setting out, moving, collecting, a find marked ?, cooking, feeding and the new-food score, the acid rising, a hated dish, a damaged tool, and the escape from day 30. The NEXT line says what to do, but not why. The day-one key flows of the Godot harnesses assume nothing stops input, so a card that waits for a key would change every one of them for no gain. [ADR 0013](0013-click-what-the-screen-shows-as-its-key.md) makes what the screen draws clickable as a key, and the placeholder cinematic cards still named their keys (E and M), against [ADR 0014](0014-name-the-action-and-show-only-the-movement-keys.md).

## Decision

Each concept gets one line, a tip, which opens the first time a run shows that concept and never again in that run; a new run opens them afresh. Tips never hold the run: one shows at a time, and it closes on any click, on the next action a human takes, or after a few seconds of play. A tip stands where it covers no HUD panel, note or click target, nor in the city any of the map's words, and is left out while no such place is free. The same lines make a How to play page on the title screen, entry 3, with Quit moved to entry 4. The placeholder cards draw a Skip and a Main menu button, which press every human's interact key and M.

The rejected alternative was a card per concept that waits for a key, which #125 proposed first. It stops the clock the player is learning to use, and every harness that plays day one would have to dismiss it.

## Consequences

- A player can play straight through and read each tip as it comes, or ignore it; no key or click is spent on it.
- The session keeps which tips a run has opened; the rules (`src/survival/**`) are only read to decide when a concept shows.
- A tip's place depends on what the frame drew, so it can move between frames as the map's words move, and a crowded frame leaves it out while its time runs on.
- The title screen's number keys change: 3 opens How to play and 4 quits.

## Signs the decision was wrong

- Players miss the tips because they close on the next action before they are read, and ask for them on the pause screen.
- New players still stop at the same places the playtest found, such as treating the first early return as a failure.
- A tip jumps around the screen as the map's words move, and reads as noise.
