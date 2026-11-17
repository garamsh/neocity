# ADR 0036: Draw the exploration UI and the base in the kit style

> Status: accepted
> Date: 2026-11-17

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

[Issue #250](https://github.com/garamsh/neocity/issues/250), a part of goal #246, asks that the exploration screens carry the look of concept 4's UI kit and that the base become concept 3's cook-feed screen: a kitchen background with a deck of APPRAISE, COOK and FEED panels. [ADR 0034](0034-show-exploration-as-the-mockup-top-bar-and-one-action-prompt.md) drew the top plates and the action prompt from art plates, and the base held a legacy kitchen crop with sprites, a task card and a block per actor. [ADR 0035](0035-lower-acid-on-feeding-time-the-cooking-and-give-the-base-five-minutes.md) gave the snapshot a running cook, its ingredient slots and the base's own clock. [ADR 0037](0037-draw-the-title-and-its-pages-in-the-ui-kit.md) drew the title in the kit with `hud::kit_panel` and `hud::kit_button`.

Alternatives considered:

- Keep the art plates for the top bar and restyle only the base. Rejected: the plates are fixed-size pictures, and the kit's panels fit their words.
- Reuse `hud::kit_panel` of the title for every panel. Rejected: it has one accent and no slot or bar; the exploration screens need the cyan, gold, magenta and green edges the concept shows, so `rendering` gets its own primitives and the title's helpers stay for the title and the choice of players.
- Move the chef and the appraiser into the HUD as sprites. Rejected: `rendering::paint_workshop` already draws them with the existing idle frames over the kitchen picture, and the base has click areas over them.

## Decision

`rendering` draws three kit primitives in code: `kit_panel` with an accent and an optional drip, `kit_slot` ringed in gold when marked, and `kit_bar` for a gauge or a progress. The top plates, the action prompt, the transfer screen's frame and the base's panels use them. The base is `art_hd/base/kitchen.png` over the whole screen and a deck along the bottom: APPRAISE with the unidentified ingredients in slots, COOK with its ingredient slots and a progress bar that fills with the snapshot's cooking progress, and FEED with the stored dishes, a SERVE button and the end-day line. The top clock shows the base's own time at base while the base has a clock. The actor blocks and FRIDGE buttons are gone: the prompts name SEARCH Fridge, COOK and APPRAISE, and the chef and the appraiser are click areas over the figures `paint_workshop` draws on the kitchen floor. The feed crane is drawn in code over the monster's mouth, holding the stored dish, and lowers it while the reaction to a fed dish plays. A cook refused because one is running says so. The choice of players is drawn with the title's kit helpers.

## Consequences

Easier: one set of primitives gives every panel the same edge, and a new panel is a rectangle and an accent; the base's checks read the same targets as before, moved onto panels.

Harder: the chef's and the appraiser's click areas stand where `paint_workshop` draws them, so a moved figure moves its area by hand; two kit helper sets, the title's and `rendering`'s, exist until a later change joins them.

Ruled out: art plates for the top bar.

## Signs the decision was wrong

A figure moves and the click areas no longer stand on the chef or the appraiser. The two helper sets drift apart so that the title and the base no longer look like one kit. The deck's panels cover what the kitchen art shows.
