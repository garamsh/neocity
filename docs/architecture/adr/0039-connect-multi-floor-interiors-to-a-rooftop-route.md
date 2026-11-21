# ADR 0039: Connect multi-floor interiors to a rooftop route

> Status: proposed
> Date: 2026-11-18

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

[Issue #271](https://github.com/garamsh/neocity/issues/271), under [goal #268](https://github.com/garamsh/neocity/issues/268), requires enterable buildings with searchable floors, interior stairs and ladders, a roof connection, and the outside darkened around an occupied cut-away. ADR 0027 already supplies shared collision and route planning, while ADRs 0029 and 0031 supply interior backgrounds and a presentation veil. A single interior rectangle proves only that a building can be entered, not that its upper floors or roof remain reachable.

## Decision

Keep buildings within their existing map and express their floors, stairs, ladders and open halls through the existing world objects. Expand the cold store to three searchable floors and a roof connected to the terminal catwalk by a dipping route, and add a two-floor service room with a roof in the aquarium lobby. Preserve the existing damage stages, movement rules, screen lock, and ADR 0027's limit on uninterrupted floor runs.

Give interiors a map identity and named floor and roof targets. Require every target to be reached by the same physics links as movement on every checked day, and require cut-passage controls to report stranded targets by name. Draw occupied multi-floor buildings as separate room bands at their collision floors, using existing interior art at native density until the modular kit replaces it. Apply ADR 0031's outside veil to occupied zones on either map, retaining its rule that an actor outside keeps the outside lit.

## Consequences

- No separate interior map, transition, HUD mode, session state or movement system is introduced. AI and humans use the same routes, and climbing never releases the screen lock.
- Opening a building's hall makes its background walls traversable; terrain and ladders still determine the route between floors.
- Reachability covers floors and roofs independently of a source's interaction range. Removing either service ladder reports the floor or roof it isolates, even while the ground floor remains reachable.
- The new structures persist through damage. Existing stage-dependent bridge and ladder alternatives remain responsible for the older routes.
- Interior artwork is temporary reuse, including the aquarium's metal room backgrounds. This decision settles structure and presentation behavior rather than the final Toxic Night kit.

## Signs the decision was wrong

- A path check passes while an actor following that route cannot reach its named floor or roof.
- A wall opening admits movement where the cut-away promises a solid barrier.
- The roof connection prevents the companion from keeping up within the screen lock.
- Floor-aligned room bands or the outside veil obscure another actor, a passage or a hazard.
