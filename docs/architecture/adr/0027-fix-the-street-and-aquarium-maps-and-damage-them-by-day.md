# ADR 0027: Fix the street and aquarium maps and damage them by day

> Status: accepted
> Date: 2026-11-13

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

[Issue #223](https://github.com/garamsh/neocity/issues/223), under goal [#218](https://github.com/garamsh/neocity/issues/218), asks for two fixed maps in place of the one 40 by 16 city of [ADR 0019](0019-lay-the-city-out-as-the-first-mockup-twice-over.md): a street at least three screens wide and an aquarium at least two, a ladder or stair between floors, buildings a party can walk into, three damage stages instead of acid-driven destruction, and a way between the maps that both actors must take together. The PM's decisions on the issue are binding: the theme building is a flooded aquarium with a teal interior, the stages run over days 1 to 4, 5 to 9 and 10 onward, and the map changes only when both actors are in the exit zone.

Until now every thing on the map broke at a configured world time or peak acid, so the layout a party met depended on how far the acid had risen and how much time had passed, and the washed-up bags carried acid limits that took supply away as acid rose. That made the map a function of two running quantities that a map designer cannot place on a screen, and it made a path check impossible to state: there was no fixed set of maps to check.

## Decision

The city is two fixed maps, `MapId::Street` (64 by 16) and `MapId::Aquarium` (44 by 16), written out in `content.cpp`. The seed is a run identifier only: the maps, the ground, the supply and every source's place and stock are the same for every seed, and a test holds two seeds to the same world.

Damage follows the day alone. `World::update(day)` replaces the world-time and peak-acid input, and an object has `exposed_from_day` and `destroyed_from_day` in place of the time and acid thresholds. The stages begin on days 1, 5 and 10 (`DamageStage::Standing`, `Cracked`, `Ruined`), so the map changes only between days, while every actor is at base. Acid still rises and still loses the run at full acid, but it no longer destroys anything or limits any source or bag. `Simulation`'s world time stays the run's clock and no longer feeds the world. [ADR 0019](0019-lay-the-city-out-as-the-first-mockup-twice-over.md)'s layout and the world-time and acid thresholds of the earlier world model are replaced, not wrapped: the old threshold types, the seeded layout code and its tests are deleted.

Sources acid used to destroy now stand for the whole run. The shallows, the tomato and pickle machines, the lost basket, the noodle vendor, the kitchen shop and the cold store door supply on every day, and the cold locker behind the door stays reachable. Day stages take away only passages and buildings that no source depends on: the roof ladder goes on day 5 and the fire escape then carries the kitchen roof; the rotten bridge and the old kiosk go on day 10 and the drifted ferry, which arrives on day 5 with its bag, carries the east. Washed-up bags land by day alone, three a day from day 2 to day 50 in the same three lanes and with the same cargo switch on day 19, and the tide takes an uncollected bag back five days after it lands. Their acid limits are removed. The seed moves no supply.

Two actors move between maps together. An `Exploration` has `MapExit` records, each a zone on one map and an arrival cell on another, and after each movement unit the map changes only when both actors stand in the same exit zone; both then stand at the arrival cell. One actor in the zone changes nothing. `enter` is refused while the other actor is in the city on a map other than the street, and a party that is entirely at base is on the street again, so a day always starts there. `collect` refuses a source on another map with `OutOfRange`. Interiors are zones of the street map that name a building; the snapshot names the building an actor is inside, so the screen can show whether it is.

The street is a base entrance at its west end, the aquarium entrance at its east end, the kitchen shop, noodle shop and cold store as enterable buildings with interior zones, and floors on three or more rows joined by ladders, stair steps and jumps. The aquarium is a lobby by the way back to the street, an upper and a lower gallery, a stair, ladders, and a skywalk that arrives with the ruined stage. The AI stays on the street: joint travel for AI is [#225](https://github.com/garamsh/neocity/issues/225)'s.

A path check guards every map at every stage: a unit test walks each map on every day from 1 to 60 over the same `CellMap` and `links` that movement uses, from the cell a party starts on, and every source on offer, every exit and every interior zone must be reached. The test also cuts a passage on each stage and requires the check to name what it strands.

## Consequences

- A designer can read a map and its three stages off `content.cpp` and the check proves a stage cannot strand a source, exit or interior; adding a passage that a stage removes without an alternative fails the test.
- The first escape check, the AI-only run to day 30 and the daily bags no longer depend on acid, so balance is tuned by the day and the supply alone.
- Acid has one job, to lose the run. The old rule that rising acid only takes supply away is gone with its thresholds.
- Interior darkening, fades and the teal aquarium art are not here: the screen draws the current map with existing art, and [#224](https://github.com/garamsh/neocity/issues/224) and [#230](https://github.com/garamsh/neocity/issues/230) finish them.
- A fall while collecting is no longer reachable by a passage breaking mid-hold, because the day changes only at base; the two tests that built that case were deleted rather than rewritten.
- The Makefile verdicts hard-code e2e check counts, so a change to a check count must change both.

## Signs the decision was wrong

- Designers want a map to change within a day, or because of acid again; day granularity then has to give way to a time input, and the check has to run over mid-day states.
- The path check passes while a human cannot finish a route, because jumps `links` finds are too tight for a real hand; the check would then need a margin.
- The AI cannot be taught to travel and stays on the street, so the aquarium's sources are dead stock for every AI-only party.
- A seed is wanted to vary the map after all, and the two-seeds test has to be loosened.
