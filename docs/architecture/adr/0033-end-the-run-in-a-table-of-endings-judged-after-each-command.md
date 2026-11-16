# ADR 0033: End the run in a table of endings judged after each command

> Status: accepted
> Date: 2026-11-16

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

[Issue #228](https://github.com/garamsh/neocity/issues/228), a part of goal #218, replaces the only way a run ended well. Until now `Escape` rolled a 20% chance after each finished day from day 30, in a stream of its own, and a hit set `Phase::Escaped`. The PM decided under #218 that there is no random escape and that the one ending is "monster satisfied": when the days the monster was fed to satisfaction total 7, not necessarily in a row, the run ends at the end of that day. A full acid is still a loss and wins over the ending on the same day. Vomiting and an exit from the map are later goals, so the structure has to take a second ending without a rewrite.

Where the issue left a choice open, the simplest was taken. The satisfaction score is the `WorkshopConfig::satisfying_score` that already decides `MonsterReaction::Satisfied`, so no second threshold exists. The count lives in the workshop, which already sees every feeding, and a judgment reads it.

Alternatives considered:

- Keep `Escape` and add a second class beside it for each new ending. Rejected: each host call site would grow with every ending, and the order between endings would be spread over callers.
- Count consecutive satisfied days. Rejected: the decision says they need not follow each other.
- Count satisfied days in `Endings`, from the last day outcome it sees after each command. Rejected: a command that is not checked between two feedings would lose a day, and the count would be a second source of what the workshop already knows.
- A virtual class per ending. Rejected: an ending is a kind and a predicate with no state of its own, so a table of functions is the smaller form.
- Keep `Phase::Escaped` and add a phase per ending. Rejected: every reader of the phase would change with each ending; `Phase::Ended` with `Snapshot::ending` keeps the phase set closed.
- Cut the random stream but keep the day-30 horizon. Rejected: nothing in the new rule refers to day 30, and the supply and AI-party tests state their own horizon.

## Decision

`Ending` is an enumeration, today `MonsterSatisfied`. `Endings` receives a `Simulation`, a `Workshop` and an `EndingConfig` and holds a table pairing each `Ending` with a judgment, a function from the workshop snapshot and the config to a boolean. `check()` does nothing outside the workshop phase, so a party in the city and a finished run are left alone. Otherwise the first judgment in the table that is reached ends the run through `Simulation::end`, which sets `Phase::Ended` and `Snapshot::ending`; `check()` returns the ending. The host calls it after every command, as it called the escape check. A loss is `Phase::Lost` and is decided inside the command that raises acid, so by the time `check()` runs it finds the run over and returns nothing.

A new ending is one enumerator, one judgment in the table and one presentation in the HUD's ending card. `Escape`, `Phase::Escaped`, the escape stream, the escape miss event and the `escape_rolls` status field are removed.

`MonsterSatisfied` is reached at `EndingConfig::satisfied_days` days, 7 as `TEMPORARY_ENDING`, counted by `WorkshopSnapshot::satisfied_days`. The unit is the day: a feeding that scores the satisfying score or more adds one, and no other day changes the count.

## Consequences

Easier: a second ending is a local change in the rules and the HUD. A reader of the phase tells an ended run from a lost one and then reads which ending. The run no longer depends on a random draw, so a seed replays the same ending day for the same play, and the end-to-end harnesses lose the 30-day wait.

Harder: every part that matched `Phase::Escaped` or the escape status names matches `Phase::Ended` and the `ending` field. The end-to-end harness keeps its `escape.gd` file name and the CI job `test-escape`, because the workflow is not changed here.

Ruled out: a chance of ending without the player's play, an ending counted from consecutive days, and a second source of the satisfied-day count.

## Signs the decision was wrong

The judgments need more than the workshop snapshot, such as map or party state, and the table's signature has to grow for every ending. Endings must be ordered or exclusive on the same day and "first in the table" is no longer enough. Seven satisfied days arrive so early that the run ends before the street's damage stages are seen, or so late that the acid decides every run, which would put the numbers, not the structure, in question.
