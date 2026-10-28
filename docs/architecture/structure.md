# Structure

The purpose-folder shape selected for the C++ sources and the Godot project, and the boundaries it imposes on future code.

## Current decisions

The C++ sources are partitioned by purpose: each folder directly under `src/` holds one thing the game does. Godot and C++ are implementation technologies within this shape, not folder names.

- **A purpose is the unit.** One shallow folder per purpose directly under `src/`, named for what it does: `gameplay`, `camera`, `input`, `rendering`, `hud`.
- **A new purpose earns a new folder, and a folder that mixes two purposes is split.** Folders are created when the purpose is named, not when its code outgrows a file.
- **No generic container.** No `common`, `shared`, `util`, or similar folder that no single purpose owns.
- **`gameplay` is the one build-enforced boundary.** It holds the game rules, depends on nothing in Godot, and builds as its own target, whose only include directory is `src/gameplay/include`. A gameplay source that includes a Godot or godot-cpp header fails to compile. Its public surface is `include/gameplay/`; everything in `src/gameplay/src/` is internal, and other folders include only `<gameplay/...>`.
- **Presentation depends on gameplay, never the reverse.** `camera`, `input`, `rendering`, and `hud` read gameplay's commands and snapshots and do not reimplement collision or progression. Renderer-specific values do not cross into gameplay.
- **A cycle between folders means the split is wrong**, and so does a file that belongs in two of them.
- **The application has one composition site**, `src/scene.cpp`. It is the one place that constructs concrete implementations.
- **Every other unit receives what it depends on and constructs none of it.**
- **The composition site stays thin.** It constructs and passes; it decides nothing else.
- `src/register_types.cpp` is the GDExtension entry point, at the `src/` root as in the godot-cpp template.

This rule governs over `docs/convention/simplicity.md` §Rules, "A division is a response, not a plan", for the folders under `src/`: the requester chose to create the purpose folders now so the structure holds as the game grows, and under `docs/architecture/README.md` §Rules 8 a structural rule here decides that case.

## Repository layout

- `src/` holds the C++ sources, one folder per purpose, with `register_types.cpp` and `scene.*` at its root.
- `project/` is the Godot project and holds every Godot resource. A level's scene and resources sit in `project/levels/<level>/`; the first level is `project/levels/ascent/`. `project.godot` and the extension's `bin/neocity.gdextension` sit in `project/` itself.
- The GDScript verification harnesses sit in `project/tests/` and `project/e2e/`, outside the C++ source tree.

## Rationale

[ADR 0004](adr/0004-adopt-the-godot-cpp-template-layout.md) records the purpose folders and the godot-cpp template layout, superseding the domain partitioning of [ADR 0001](adr/0001-partition-the-system-by-domain.md). [ADR 0002](adr/0002-isolate-the-ascent-engine-from-presentation.md) introduced the rules/presentation boundary and [ADR 0003](adr/0003-implement-the-ascent-domain-in-cpp.md) implements both sides in C++; that boundary is now the `gameplay` target.

## Open questions

- When `gameplay` grows enough to need subfolders, and how they are cut.
- Whether a future host requires a different composition site; the current one is the C++ `AscentScene` loaded by Godot.
- Whether production needs a graphics backend beyond the Godot canvas; the current integration is a godot-cpp GDExtension.
