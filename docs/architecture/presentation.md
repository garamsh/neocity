# Presentation

How the GDExtension presents the selected rules library through input, camera motion, and a drawn frame.

## Current decisions

- The `neocity` GDExtension library is built from `src/register_types.cpp`, `src/scene.cpp`, and the session, camera, input, rendering, and HUD folders. It depends on the three rules libraries' public surfaces and godot-cpp 10.0.0-stable. The root `build_profile.json` limits the generated bindings to the Godot classes it uses.
- `src/register_types.cpp` is the extension entry point (`neocity_init`) and registers `AscentScene`.
- `src/scene.cpp` (`AscentScene`) is the composition site and frame adapter: it constructs the independent engines, camera, session, and drawing, then forwards frame input and drawing to session.
- `src/session/` owns selection and run orchestration as described in [session](session.md).
- `src/input/` translates Godot input actions into host actions and each rules library's separate command.
- `src/camera/` follows the leading actor vertically.
- `src/rendering/` loads the level textures and Godot's fallback font, and paints the city depth, course, obstacles, and actors. It owns the shared drawing primitives and color palette. Its pan painter draws basic circular bodies, rim, exit, and tilt indicator directly from the corresponding rule snapshots.
- `src/hud/` paints the interface and the pause/completion overlays over the world, using the rendering primitives. It also draws the common menu, pan instructions, and terminal outcomes.
- Godot provides window, input, resource, and canvas primitives. No custom GPU backend is implemented.

## Rationale

[ADR 0003](adr/0003-implement-the-ascent-domain-in-cpp.md) selects godot-cpp for the adapter. [ADR 0004](adr/0004-adopt-the-godot-cpp-template-layout.md) splits it into one folder per presentation purpose with the entry point and composition site at the `src/` root. [ADR 0005](adr/0005-isolate-experiment-rules-behind-one-host.md) adds shared session orchestration while preserving that entry contract.

## Open questions

- Authored animation, audio, and a production rendering backend beyond the Godot canvas.
- Export targets and packaging outside the locally verified development build.
