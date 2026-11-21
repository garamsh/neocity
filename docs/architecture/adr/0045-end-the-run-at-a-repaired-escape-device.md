# ADR 0045: End the run at a repaired escape device

A fourth ending, reached in the city: carry three parts to a broken device on the aquarium roof from day ten.

> Status: proposed
> Date: 2026-11-21

## Context

- Goal #268 chooses a map escape device as the fourth ending. The PM fixed its details in issue #304: one device on the aquarium's rooftop route, three parts scattered over both maps, usable from day ten, deterministic, with acid loss first.
- [ADR 0033](0033-end-the-run-in-a-table-of-endings-judged-after-each-command.md) and [ADR 0038](0038-end-day-thirty-by-vomiting-and-illustrate-the-three-outcomes.md) judge endings only in the workshop. The device is used by an actor standing in the city beside it, which the workshop table cannot see.

## Decision

- Add `Ending::Escaped`. `Exploration::escape` judges it from `ExplorationConfig::escape`: the actor is in the city on the device's map, not searching, within the collection distance of the device, on or after its first day, and carries its number of `ESCAPE_PART`. It then calls `Simulation::escape`, which ends a run that is in the city.
- The part is an ingredient identity that is no food, so the existing containers, inventory weight and transfer screen carry it. Appraisal, recipes and AI skip it.
- A human uses the device with the interact key in the city (`UseDevice`), so no new key exists. The values are temporary: 3 parts, day 10, weight 2, the device at the service roof, the parts in three containers.

## Consequences

- A run that is over answers `RunOver`, so a full-acid loss beats an escape, and the workshop endings never hold in the same command as an escape.
- The `Endings` table stays workshop-only; the escape is the one ending outside it.
- The ending card shows the escape's text, and the device in one colour until its picture is drawn.

## Signs the decision was wrong

- A later ending reached in the city needs a second command of this kind, so that the endings want one table that reads both phases.
- Players cannot find the device or the parts without a goal pointer, so that the NEXT line has to name them.
