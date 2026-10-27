# Ascent

The playable ingredient expedition and its engine/presentation responsibilities.

## Current decisions

- `ascent/engine` is a dependency-free Cargo package. `simulation.rs` owns fixed-step movement, input edges, run state, checkpoint recovery, character identity, and ingredient completion. `collision.rs` owns one-way landings and obstacle contact; `course.rs` owns course surfaces and obstacle patterns. `world_snapshot.rs` exposes framework-independent presentation values. `lib.rs` is its public boundary.
- `ascent/godot` depends on that public boundary and godot-rust 0.5.5. `scene.rs` is the composition root and frame adapter; `input.rs` translates Godot actions, `camera.rs` follows the actor, `drawing.rs` selects sprites, city depth, and canvas effects, and `hud.rs` owns the interface and pause/completion overlays from snapshots. Its assets are owned under `assets/`.
- The slice has one ascending course, moving ledges, patrol/flying/projectile obstacles, a checkpoint, retries, an ingredient goal, pause, and restart. Physics is stepped at 120 Hz independently of render cadence.
- One collision body represents the party. Tab changes the lead identity; the companion is a delayed visual trace without an independent collision or damage body. This is an explicit prototype control choice, not final dual-character gameplay.
- The shipped scene loads the Rust extension through `neocity.gdextension`; Godot provides window/input/resource/canvas primitives. Custom movement, collision, progression, camera policy, and drawing composition are Rust code. No custom GPU backend is implemented.

## Rationale

[ADR 0002](adr/0002-isolate-the-ascent-engine-from-presentation.md) records the engine/presentation boundary. The selected [jump-map art](../art/README.md) motivates a readable side-on course with layered neon depth; it does not supply collision-valid geometry or a final animation set.

## Open questions

- Final dual-character interaction and collision model, authored animation, audio, stage count, and encounter tuning.
- Production rendering backend and optimization beyond the current Godot canvas adapter.
- Export targets and packaging outside the locally verified macOS development build.
