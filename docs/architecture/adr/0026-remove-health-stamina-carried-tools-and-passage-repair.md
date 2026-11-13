# ADR 0026: Remove health, stamina, carried tools and passage repair

> Status: accepted
> Date: 2026-11-13

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

[Issue #218](https://github.com/garamsh/neocity/issues/218) sets the goal of simplifying a run to finding, appraising, cooking and feeding. The survival rules of [ADR 0007](0007-move-actors-through-a-side-view-with-gravity.md) and [ADR 0008](0008-hold-the-collecting-actor-instead-of-the-shared-clock.md) carried three systems around that loop: health slots lost to acid drips and falls, stamina spent by running, and tools. Tools were found in the city, spent to repair a broken passage, and spent to replace the base magnifier and spatula that exhaustion damaged, which lowered appraisal and cooking success from 90% to 40%.

[Issue #220](https://github.com/garamsh/neocity/issues/220) records the PM's reading: "tools" means the carried tools, passage repair, and the base tool damage and replacement that exhaustion caused. Appraisal and cooking stay with a fixed 90% success. Running stays with no limit. Acid drips have no effect. The HUD's health and stamina display goes, and the bag's slots stay.

## Decision

Remove health, stamina, exhaustion, carried tools, passage repair, and base tool damage and replacement from `Simulation`, `Exploration`, `World`, `Workshop`, `Companion` and `Content`, and from the screen, sound and first-play tips that showed them. A damaged passage stays damaged for the rest of the run. Appraisal and cooking succeed below a fixed 90. Hazard cells stay in `Content` for the screen to draw and nothing in the rules reads them. AI sets out only for a source it can still reach, since a broken passage can no longer be repaired.

## Consequences

- The rules have fewer commands and less state, and a trip ends only by time, a manual return, or a fall into the acid.
- The map loses the objects that only supplied tools (the Tool Bag, Terrace Bag and Supply Crate). Layout, passages and every other source are unchanged.
- A broken passage cuts off what lies behind it for good, so a party can run out of reachable stock. The thirty-day AI-only runs in `content_test.cpp` check that this does not happen before the first escape check.
- The repair and hurt sounds, the REPAIR action and the tool damage tip are removed. The heart and tool art files are left in place, unused, for the art owner to retire.
- The 40% success rate has no use; the 90% stays as a constant.

## Signs the decision was wrong

- Playtesting shows runs with nothing to fear in the city, so the day clock and acid alone give no tension, and a risk needs adding back.
- Broken passages end runs through unreachable stock often enough that a way to reopen them is wanted, which would be a new decision rather than a return of the tools.
