# Presentation

How the GDExtension presents the main game and the menus through input, camera motion, and a drawn frame, and hosts the experiment scenes.

## Current decisions

- The `neocity` GDExtension library is built from `src/register_types.cpp`, `src/scene.cpp`, and the session, camera, input, rendering, and HUD folders. It depends on the rules libraries' public surfaces and godot-cpp 10.0.0-stable, and compiles each experiment's `view/` node. The root `build_profile.json` limits the generated bindings to the Godot classes it uses.
- `src/register_types.cpp` is the extension entry point (`neocity_init`) and registers `AscentScene` and each experiment's node class.
- `src/scene.cpp` (`AscentScene`) is the composition site and frame adapter: it constructs the ascent engine, camera, session, and drawing, forwards frame input and drawing to session, and mounts the experiment scene session selects as its child.
- `src/session/` owns selection and run orchestration as described in [session](session.md).
- `src/input/` translates Godot input actions into menu actions and the ascent command.
- `src/camera/` follows the leading actor vertically.
- `src/rendering/` loads the level textures and Godot's fallback font, and paints the city depth, course, obstacles, and actors. It owns the shared drawing primitives and color palette.
- `src/hud/` paints the interface and the pause/completion overlays over the world, using the rendering primitives. It also draws the main menu and the developer page.
- An experiment's node presents itself, in 3D for `pan_dodge`, with Godot's primitive meshes, materials, lights, camera, and labels, as [experiments](experiments.md) describes.
- Godot provides window, input, resource, canvas, and 3D primitives. No custom GPU backend is implemented.

## Rationale

[ADR 0003](adr/0003-implement-the-ascent-domain-in-cpp.md) selects godot-cpp for the adapter. [ADR 0004](adr/0004-adopt-the-godot-cpp-template-layout.md) splits it into one folder per presentation purpose with the entry point and composition site at the `src/` root. [ADR 0005](adr/0005-isolate-experiment-rules-behind-one-host.md) adds shared session orchestration while preserving that entry contract, and lets experiments present themselves in their own scenes.

## Open questions

- Authored animation, audio, and a production rendering backend beyond the Godot canvas.
- Export targets and packaging outside the locally verified development build.
