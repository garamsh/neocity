# Ascent

The playable ingredient expedition and its engine/presentation responsibilities.

## Current decisions

- `ascent/engine` is a C++ static library that depends only on the standard library. `src/simulation.cpp` owns fixed-step movement, input edges, run state, checkpoint recovery, character identity, and ingredient completion. `src/collision.cpp` owns one-way landings and obstacle contact; `src/course.cpp` owns course surfaces and obstacle patterns. `include/ascent/world_snapshot.hpp` exposes framework-independent presentation values, and `include/ascent/engine.hpp` with it is the public boundary. Unit tests sit beside the sources they test as `src/*_test.cpp`.
- `ascent/godot` is a GDExtension library that depends on that public boundary and godot-cpp 10.0.0-stable. `src/scene.cpp` is the composition root and frame adapter; `src/input.cpp` translates Godot actions, `src/camera.cpp` follows the actor, `src/drawing.cpp` selects sprites, city depth, and canvas effects, and `src/hud.cpp` owns the interface and pause/completion overlays from snapshots. `src/register_types.cpp` is the extension entry point. `build_profile.json` limits the generated bindings to the Godot classes the adapter uses. Its assets are owned under `assets/`.
- The slice has one ascending course, moving ledges, patrol/flying/projectile obstacles, a checkpoint, retries, an ingredient goal, pause, and restart. Physics is stepped at 120 Hz independently of render cadence.
- One collision body represents the party. Tab changes the lead identity; the companion is a delayed visual trace without an independent collision or damage body. This is an explicit prototype control choice, not final dual-character gameplay.
- The shipped scene loads the C++ extension through `neocity.gdextension`; Godot provides window/input/resource/canvas primitives. Custom movement, collision, progression, camera policy, and drawing composition are C++ code. No custom GPU backend is implemented.

## Rationale

[ADR 0002](adr/0002-isolate-the-ascent-engine-from-presentation.md) records the engine/presentation boundary, and [ADR 0003](adr/0003-implement-the-ascent-domain-in-cpp.md) its C++ implementation and toolchain. The selected [jump-map art](../art/README.md) motivates a readable side-on course with layered neon depth; it does not supply collision-valid geometry or a final animation set.

## Open questions

- Final dual-character interaction and collision model, authored animation, audio, stage count, and encounter tuning.
- Production rendering backend and optimization beyond the current Godot canvas adapter.
- Export targets and packaging outside the locally verified macOS development build.
