# ADR 0035: Lower acid on feeding, time the cooking and give the base five minutes

> Status: accepted
> Date: 2026-11-17

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

[Issue #248](https://github.com/garamsh/neocity/issues/248), a part of goal #246, asks for three rules the game lacked. A fed monster only held the acid, so a run could never recover from a bad day. Cooking was an instant command, so the chef had nothing to wait for and the screen nothing to show. The base had no clock, which `survival.md` listed as an open question, so a day was only the exploration budget of 18 seconds.

The screen needs three things from the session that it could not read: how far a cook has come, what is in the ingredient slots, and how much base time is left.

Alternatives considered:

- Keep the base clock in `Simulation` beside the exploration budget. Rejected: the base time passes only in the workshop phase and ends the day by a `Workshop` command, and `Simulation` knows neither the cook nor the unfed penalty; the clock would need both passed in.
- Give a cook that refuses while another runs its own `WorkshopResult::Busy`. Rejected for now: `src/hud/survival.cpp` switches over `WorkshopResult` with no default, so a new enumerator breaks a file this issue does not own. `Full` is used, and a later change that owns the HUD can split it.
- Let a cook that is still running when the day ends carry over. Rejected: its ingredients were spent for a day the monster is already past, and a carried cook would let a player cook through the unfed penalty.
- Roll the cook's success when the cook starts. Rejected: the roll then precedes the wait it decides, and a failed dish would be known at the start; the roll is made when the cook ends, with the ingredients spent at the start.
- Change the length of the days in the Godot checks' scripts. Rejected: the checks are other workers' files and assert the 18 second budget; the content reads three environment variables instead, and the Makefile and the unit tests set the short values.

## Decision

A feeding that satisfies the monster lowers acid by `WorkshopConfig::satisfied_acid_drop`, 8 in the content, after the day's rise, never below zero; `DayOutcome::acid_drop` records it. Cooking takes `cook_time` of base time, 8 seconds in the content: `cook` moves the recipe's ingredients into the cooking slots and spends them, and the dish, or the failure, comes when the time has passed. One cook runs at a time, and one still running when the day ends is lost. The base has `base_time`, 5 minutes in the content, a day's allowance that runs only in the workshop phase; when it is spent the day ends unfed at `timeout_acid_rise`, 12 in the content. Exploration is also 5 minutes. With no `cook_time` a dish is done inside `cook`, and with no `base_time` the base has no clock, which is what the library and its unit tests use unless they set one.

`Workshop` keeps the base clock and the cook, and the session advances it on the same 100ms unit as exploration. `survival_status()` reports `cooking`, `cooking_progress` (0 to 1), `cooking_food`, `ingredient_slots`, `ingredient_slot_count`, `base_timed` and `base_remaining_ms` for the screen to draw.

`temporary_content` reads `NEOCITY_EXPLORATION_SECONDS`, `NEOCITY_BASE_SECONDS` (0 for no clock) and `NEOCITY_COOK_SECONDS`. The Makefile exports 18, 0 and 0 and the unit tests register the same, so the existing checks keep their days.

## Consequences

Easier: a run can recover by feeding well; the screen can draw a cook and the base clock from the session; the AI-only content tests and the Godot checks run short days without a change of their own.

Harder: a refusal for a running cook reads as a full fridge on the screen until the HUD gives it its own result; the AI chef keeps asking to cook while one runs and is refused each time, rather than waiting; the content now has two day lengths, the default and the checks'; and a run launched without the environment sees 10 minute days.

Ruled out: a base with no limit in the game itself, and a cook that outlives its day.

## Signs the decision was wrong

Players leave dishes cooking at the day's end and lose them without having understood why. The base's five minutes is too long or too short for a human to cook, feed and set out, and the cook's 8 seconds is not worth a wait. A player is told the fridge is full when the slots are busy. The environment variables are set in a place nobody meant, or the day length of the checks and of the game drift apart.
