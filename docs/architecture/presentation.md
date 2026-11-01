# Presentation

How the GDExtension presents both games and the menus through input, camera motion, and a drawn frame, and hosts the experiment scenes.

## Current decisions

- The `neocity` GDExtension library is built from `src/register_types.cpp`, `src/scene.cpp`, and the session, camera, input, rendering, and HUD folders. It depends on the rules libraries' public surfaces and godot-cpp 10.0.0-stable, and compiles each experiment's `view/` node. The root `build_profile.json` limits the generated bindings to the Godot classes it uses.
- `src/register_types.cpp` is the extension entry point (`neocity_init`) and registers `AscentScene` and each experiment's node class.
- `src/scene.cpp` (`AscentScene`) is the composition site and frame adapter: it constructs the ascent engine, camera, session, drawing, and one `SurvivalRules` per survival run, forwards frame input and drawing to session, and mounts the experiment scene session selects as its child. It exposes `survival_status()` and `pin_survival_seed()` for the Godot checks; an unpinned run takes its seed from Godot's random generator. It also provides session's `CinematicStage`: it loads a cinematic scene as its child, closes it on the `finished` signal, and reports a scene that cannot play instead of aborting, unlike the experiment mount, whose scenes are always registered.
- `src/session/` owns selection and run orchestration as described in [session](session.md).
- `src/input/` translates Godot input actions into menu actions, the ascent command, and the survival input of each human actor. `project.godot` binds the `survival_1_*` and `survival_2_*` actions.
- `src/camera/` follows the leading actor vertically.
- `src/rendering/` loads the level textures and Godot's fallback font, and paints the city depth, course, obstacles, and actors. It owns the shared drawing primitives and color palette. `src/rendering/survival.cpp` paints the survival game in `_draw` primitives: the monster's mouth with its acid line, the city grid with a mark for every destroyed, depleted, or unavailable object, and the workshop. It reuses the ascent atlas's chef and appraiser sprites.
- `src/hud/` paints the interface and the pause/completion overlays over the world, using the rendering primitives. It also draws the main menu, the developer page, the choice of players, and the survival HUD. The survival HUD reads one `SurvivalFrame` of snapshots and shows the day, acid, time, each actor's controller, health, stamina, location and tools, the pack, the base tool states, the recipes, the last day's result, an event log, and a NEXT line naming what to do now. Under a cinematic it paints the dark stage, or the placeholder card when no scene can play, as the [cinematic contract](session.md#cinematic-contract) describes.
- The NEXT line asks `Companion` what the human's key would do with the current view, so the offer it names is the command the key runs.
- An experiment's node presents itself, in 3D for `pan_dodge`, with Godot's primitive meshes, materials, lights, camera, and labels, as [experiments](experiments.md) describes.
- Godot provides window, input, resource, canvas, and 3D primitives. No custom GPU backend is implemented.

## Rationale

[ADR 0003](adr/0003-implement-the-ascent-domain-in-cpp.md) selects godot-cpp for the adapter. [ADR 0004](adr/0004-adopt-the-godot-cpp-template-layout.md) splits it into one folder per presentation purpose with the entry point and composition site at the `src/` root. [ADR 0005](adr/0005-isolate-experiment-rules-behind-one-host.md) adds shared session orchestration while preserving that entry contract, and lets experiments present themselves in their own scenes.

## Open questions

- Authored animation, audio, and a production rendering backend beyond the Godot canvas.
- Export targets and packaging outside the locally verified development build.
