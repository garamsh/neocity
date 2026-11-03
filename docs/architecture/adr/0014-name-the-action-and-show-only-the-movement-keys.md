# ADR 0014: Name the action on the screen and show only the movement keys

> Status: accepted
> Date: 2026-11-04

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

[ADR 0013](0013-click-what-the-screen-shows-as-its-key.md) made every command except movement reachable by a click on what the screen draws. The screen still named each command by its key: "[E] COLLECT", "[Q]/[U] Set out", "[G] End the day unfed", a key cap over the head of a human who could act ([ADR 0010](0010-frame-the-hud-in-the-corners-like-the-concept.md)), the Esc cap on the pause chip, and the menus' footers. [#154](https://github.com/garamsh/neocity/issues/154) asks that the always-on hints keep only the movement keys, since a click now does the rest, and that the HUD's smaller area be measured.

## Decision

The screen names each action, not its key: buttons, the NEXT line, the task card, the pause chip, the pause screen's buttons and the menus' footers say what a click does. Only the movement keys, move, run and jump, stay on screen during play. Every key is still bound and is listed on the pause screen and the controls page. The key cap over a human's head is removed, because the lit action button already shows who can act.

The rejected alternative was to keep the head cap with the action's word in it. It repeats the action button and covers more of the map, and the issue asks for less HUD.

## Consequences

- A player who wants the keys finds them on the pause screen or the controls page; the screen no longer teaches them while playing.
- Harnesses that follow the NEXT line map its action words to keys, as the controls page does for a player.
- The swap key (Tab), which no click reaches, is named only on the pause screen and the controls page.
- The menu entries keep their numbers, which the number keys still choose.

## Signs the decision was wrong

- Players who prefer the keyboard keep opening the pause screen to find a key.
- New players do not find the swap between actors, since nothing during play names it.
