# ADR 0034: Show exploration as the mockup's top bar and one action prompt

> Status: accepted; the rule that a tip names no key superseded by ADR 0046 for the set-out tip
> Date: 2026-11-16

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

[Issue #229](https://github.com/garamsh/neocity/issues/229), a part of goal #218, asks that exploration look like the mockup `search-mockup-v1.png` at commit `241124d5ba2dcef937c904623f1e5e3cb03fbcf5`: a bar of three plates across the top and nothing else but what the player can act on. [ADR 0010](0010-frame-the-hud-in-the-corners-like-the-concept.md) kept the HUD to the four corners of the screen, with a goal line, fading notes and, along the bottom, the actors, the bag and the action. [ADR 0032](0032-search-containers-into-a-weighted-inventory-and-a-base-fridge.md) removed the bag, which left the bottom row of actor blocks, the action panel with its movement-key line and an INVENTORY button, and the pause chip under the clock. The three plates, the sun, the clock and the action prompt are now drawn art under `project/levels/monster_mouth_escape/art_hd/ui/`, from [#222](https://github.com/garamsh/neocity/issues/222).

[ADR 0014](0014-name-the-action-and-show-only-the-movement-keys.md) took the key caps off the screen so that the action's name stood alone and the movement keys were the only keys shown. The issue's prompt reads "[R] SEARCH ..." and so shows a key.

Alternatives considered:

- Keep the NEXT line and the notes in exploration beside the plates. Rejected: the issue asks that exploration show only the plates, the prompts and the transfer screen, and the NEXT line and the notes are what made the screen crowded.
- Keep the pause chip under the clock. Rejected: it is not one of the three plates, and Esc pauses.
- Leave the travel key unnamed in exploration. Rejected: the issue shows the action the key does now, and with no key line and no pause chip the screen would then say nothing about the way back to base.
- Name the action without the key, as ADR 0014 decides. Rejected: the issue states the prompt's form, and the cap tells a player which key a click on the prompt stands for.
- Keep the first-play tips in the city. Rejected: a tip is a second thing drawn over the play area; at base the screen has room for it.

## Decision

Exploration shows three plates in the top band, one action prompt for each human who has an action to take, and the transfer screen while one is open, and nothing else. The plates are the day with a sun, the acid with a gauge that fills with it, and the time left with a clock. A prompt stands at the bottom right and names a key of the human as a cap and the action beside it. It names the search key with SEARCH and the container's name, or CLOSE while the transfer screen is open; when the search key has nothing to do and the actor is in the city, it names the travel key with where it goes, TO BASE. With two humans a player tag leads each prompt. It is hidden while a search holds the actor. A click on a prompt presses the key it names, as [ADR 0013](0013-click-what-the-screen-shows-as-its-key.md) decides, and a click on a slot of the transfer screen is unchanged.

The NEXT line, the notes, the tips, the actor blocks, the key line, the INVENTORY button and the pause chip are not drawn in exploration. The key list stays on the pause screen and the controls page. What `survival_status()` reports is unchanged.

Base uses the same plates. It keeps the task card, the kitchen's panels and the FRIDGE and CLOSE buttons of ADR 0032, and adds one line saying that moving and jumping do not work there. The first-play tips open at base only.

The prompt's key caps amend the sentence of ADR 0014 that removes key caps from the screen; its other decisions stand. This replaces the four-corner layout, the fading notes and the bottom row of ADR 0010, and the interface-frame and icon cuts of ADR 0022 that the plates and prompt replace; the frames the menus and base panels use stay.

## Consequences

Easier: the play area is free of everything but the plates and, at a container, one prompt; a player sees at once that a key acts and which one; the HUD's checks reduce to the number of plates and prompts.

Harder: a player reads the goal only from the map's marks and its pointers, since the NEXT line is not drawn; a log event such as a burn or a destroyed passage is not shown in exploration; and a player who plays by mouse alone cannot pause, because the pause chip is gone and ADR 0013 holds that a click presses a key only for what the screen draws; the way back to base is the prompt's and stays a click.

Ruled out: a HUD element beyond the plates and the prompts during exploration, until a later decision adds one.

## Signs the decision was wrong

Players cannot tell where to go once the NEXT line is gone, and the pointers and map marks do not stand in for it. A burn or a fall goes unnoticed for want of a note. A mouse-only player is stuck for want of a way to pause. Players ask for the keys on screen again, which the controls page does not answer.
