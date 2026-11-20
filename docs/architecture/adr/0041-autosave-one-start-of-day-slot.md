# ADR 0041: autosave one start-of-day slot

> Status: proposed
> Date: 2026-11-20

## Context

- A player needs to resume a run from the beginning of its most recently opened day, including day one.
- Progress lives in progression, workshop and exploration rules; saving only the date would lose inventory, knowledge, food, supply and random history.
- The title already offers new-game setup. Opening that setup must preserve the existing slot until a replacement run starts its first day.

## Decision

- Keep one versioned start-of-day checkpoint under `user://`. Overwrite it at each new day, after the ending judgment and before another command or elapsed time can change that day's state; remove it for either ending or acid loss.
- Serialize persistent rule state and local roles in pure C++. Reconstruct the fixed base positions, fresh clocks and date-dependent world from the checkpoint and current content. Preserve the workshop random sequence and remaining container stock.
- Offer Continue first on the title only for a checkpoint parsed against the current format and content. Resume directly at base without replaying the intro; new-game setup leaves the slot intact.
- Treat file reads as a trust boundary: bound input and collection sizes, parse numbers and variants, prove identities and start-of-day invariants, and reject incompatible versions or timing. Expected I/O failure hides Continue or reports a failed write without stopping play.

## Consequences

- Resuming restores the beginning of the day, discarding later actions in that day. It does not save a pause, held input, presentation animation, tutorial history or cinematic playback.
- A completed run cannot be resumed. A failed write may leave no readable checkpoint; the title offers Continue only after a successful parse.
- The format version must change when persistent state or content compatibility changes. There is no migration, manual save, multiple slot selection or recovery journal.
- A direct state checkpoint avoids replaying a growing command history at startup. Full mid-day snapshots were rejected because the required recovery point has fixed positions and clocks and needs no transient search or physics state.

## Signs the decision was wrong

- Players repeatedly need recovery within a day rather than at its beginning.
- Interrupted overwrites lose enough progress to require transactional replacement or a backup slot.
- Content updates routinely require preserving old runs, making version rejection insufficient.
