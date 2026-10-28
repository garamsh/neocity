# Gameplay

The game rules of the playable ascent level and the public surface presentation reads them through.

## Current decisions

- `src/gameplay` builds the `gameplay` static library, which depends only on the C++ standard library. Its public surface is `include/gameplay/engine.hpp` (`AscentEngine` and `PlayerCommand`) and `include/gameplay/world_snapshot.hpp` (framework-independent presentation values). Code outside the folder includes only `<gameplay/...>`.
- `src/simulation.cpp` owns fixed-step movement, input edges, run state, checkpoint recovery, character identity, and ingredient completion. `src/collision.cpp` owns one-way landings and obstacle contact; `src/course.cpp` owns course surfaces and obstacle patterns. These and their headers are internal to the folder.
- Unit tests sit beside the sources they test as `src/*_test.cpp` and build into `gameplay_test`.
- The slice has one ascending course, moving ledges, patrol/flying/projectile obstacles, a checkpoint, retries, an ingredient goal, pause, and restart. Physics is stepped at 120 Hz independently of render cadence.
- One collision body represents the party. Tab changes the lead identity; the companion is a delayed visual trace without an independent collision or damage body. This is an explicit prototype control choice, not final dual-character gameplay.

## Rationale

[ADR 0002](adr/0002-isolate-the-ascent-engine-from-presentation.md) records the rules/presentation boundary, [ADR 0003](adr/0003-implement-the-ascent-domain-in-cpp.md) its C++ implementation and toolchain, and [ADR 0004](adr/0004-adopt-the-godot-cpp-template-layout.md) the `gameplay` folder and target. The selected [jump-map art](../art/README.md) motivates a readable side-on course; it does not supply collision-valid geometry.

## Open questions

- Final dual-character interaction and collision model, stage count, and encounter tuning.
- Whether a second level's rules share this library or need their own course data format.
