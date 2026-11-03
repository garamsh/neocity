# ADR 0007: Move actors through a side view with gravity

> Status: accepted; the one screen superseded by ADR-0009
> Date: 2026-11-03

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

Exploration moved both actors one cell at a time on a four-direction grid, which [survival](../survival.md) recorded as a temporary representation with final physics undecided. The screen draws that grid from the side, so walking up a stair was a step north on a map. The user asked for a proper game and chose a true side-scroller, which [issue #109](https://github.com/garamsh/neocity/issues/109) records with the PM's decisions: positions finer than a cell on a deterministic fixed step, gravity, a jump across one- or two-cell gaps with a capped height, ladders, stairs, and ramps for climbing, a fall into acid or off the map under the existing fall rule, and the whole map kept on one screen.

The survival library has no Godot dependency, a run replays the same for the same command order however frames are split, and AI and the screen's NEXT line plan routes over the same rule that moves the actors.

Alternatives considered:

- **Keep the grid and only draw it as a side view.** There is no gravity or jump, so the map cannot ask for either; the user chose the side-scroller over this.
- **Floating-point positions and velocities.** Rounding then depends on how the time is split and on the platform, so the same run could end in a different cell.
- **Godot's physics bodies.** The rules would leave the survival library for the host, and AI and the tests could no longer plan or replay a route apart from it.
- **A scrolling camera over a larger map.** The PM kept the single-screen view, so both players and the whole city stay in sight.

## Decision

Each actor's feet are a position in fixed-point sub-cell units, 32 to a cell, moved by a 20ms physics step of integer speeds, five steps to a 100ms movement unit. Gravity pulls an actor that stands on nothing; a jump has a fixed upward speed, so its height is capped and its reach crosses a gap of two cells at the same height or one cell a row up. Walkways are one-row floors that an actor jumps up through and lands on; ladders are climbed between floors, and stairs and ramps are one-cell steps walked up and down. An actor that drops below every object has fallen into the acid and returns to base under the existing fall rule. Routes are planned by running the same physics step from each standing cell, and the whole map stays on one screen.

## Consequences

- Movement, AI routes, and the NEXT line read one physics rule, so a route that the planner finds is one the body walks, and replaying a run gives the same positions.
- A map is now a set of one-row floors, ladders, steps, and gaps; the content tests hold that shape, and a map change is checked against the jump's reach as well as the walkways.
- A human can miss a jump or step off an edge, so a fall is a consequence of play, not only of a breaking passage.
- Route planning runs the physics for every link from every reachable cell, so its cost grows with the map's cells and the length of a jump; that suits a one-screen map and would need another search for a much larger one.
- Speeds, the jump's height, and the step length are temporary tuning values in `PHYSICS`; changing them changes which gaps a map may hold.

## Signs the decision was wrong

- Players find the 100ms movement unit or the 20ms step too coarse to aim a jump, and the fixed step has to be shortened until it costs the frame budget.
- Maps need slopes, moving platforms, or bodies wider than a cell, which the cell-based floors and point feet cannot express.
- The route planner's cost forces caching or approximations that let AI choose a route the physics does not walk.
