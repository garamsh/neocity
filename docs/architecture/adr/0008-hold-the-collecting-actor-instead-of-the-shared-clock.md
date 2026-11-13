# ADR 0008: Hold the collecting actor instead of the shared clock

> Status: accepted; its collection of tools superseded by ADR-0026
> Date: 2026-11-03

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

A collection spent 12 seconds of a 45-second shared clock at once, so a day held four collections between both actors, and in one-player play the AI partner often spent them before the human reached a source. [Issue #121](https://github.com/garamsh/neocity/issues/121) measured a human following NEXT in the city for 1 to 9 seconds a day, and on day 1 collecting nothing. The side view of [ADR 0007](0007-move-actors-through-a-side-view-with-gravity.md) asks the player to walk, climb, and jump, which a day that short leaves no time for.

The PM chose, on the issue, that a collection holds only the actor who makes it while the day runs on real time, the same rule for a human and AI, and that the supply spread out in space and time, small bags landing across the map every day from day 2 to day 50, so that walking, not collecting, fills the day. The user's decision that AI's share is not limited stands.

Alternatives considered:

- **Keep the shared cost and only relabel the clock as collections left.** The human's time in the city stays a few seconds a day, which is the problem.
- **A longer day and a cheaper shared cost.** A collection still takes time from the partner, so whoever reaches a source first still takes the other's day.
- **Hold the actor but keep eight large bags near the entrance.** Measured on the issue: the supply, not the time, then decides how many collections a day holds; days with nothing to take give a human no reason to set out, and a longer hold fills the day with collecting instead of walking.
- **Supply that refills.** The PM did not approve a new rule of that kind.

## Decision

A collection records its item at once and holds the collecting actor in place for the collection time, during which it takes no movement and no command; the shared clock runs on for everyone, both actors may collect at once, and a hold ends when its time has passed, when the actor falls into the acid or is exhausted, or when the day expires, keeping the item. The tide brings a few small bags to every lane of the map every day from day 2 to day 50 and takes each back a few days later.

## Consequences

- A human and an AI partner collect side by side, and neither spends the other's day.
- The day is real time, so the screen's clock reads as seconds, and the action card shows how long a hold has left.
- The number of collections a day is bounded by the supply in reach, so balance is tuned by where and when bags land, and a supply change is checked against both the loss rate and how long a human stays out walking.
- A held actor cannot return to base, so a human waits out the hold before going home.
- Many short-lived bags make the map's sources change every day while they land; the screen hides a bag once it is empty or carried off, and the event log leaves the tide's bags out.

## Signs the decision was wrong

- Players stand still collecting more than they walk, so the hold has become the day again.
- An AI partner still takes every bag before the human reaches one, because space, not time, now decides who gets there first.
- The daily bags make the map too busy to read, or the balance needs per-day bag tables that a simpler supply rule would replace.
