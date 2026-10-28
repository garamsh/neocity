# Structure

The purpose-folder shape selected for the C++ sources and the Godot project, and the boundaries it imposes on future code.

## Current decisions

The C++ sources are partitioned by purpose: each folder directly under `src/` holds one thing the game does. Godot and C++ are implementation technologies within this shape, not folder names.

- **A purpose is the unit.** One shallow folder per purpose directly under `src/`, named for what it does: `gameplay`, `experiment`, `session`, `camera`, `input`, `rendering`, `hud`.
- **An experiment is the unit inside `experiment`.** `src/experiment/<id>/` holds everything one experiment owns: its rules library, its Godot node in `view/`, and its record. `src/experiment/catalog.hpp` registers them. This is the one purpose with a second level; no other purpose nests by kind.
- **A new purpose earns a new folder, and a folder that mixes two purposes is split.** Folders are created when the purpose is named, not when its code outgrows a file.
- **No generic container.** No `common`, `shared`, `util`, or similar folder that no single purpose owns.
- **Each rule library is a build-enforced boundary.** `gameplay` and every experiment's library own their simulation and depend only on the C++ standard library at runtime. Each target exposes only its own `include/<library>/` directory; its `src/` is internal. Consumers include only `<library/...>`, never internal headers. No rule library may include or link another rule library or Godot.
- **Presentation depends on rules, never the reverse.** `session`, `camera`, `input`, `rendering`, `hud`, and each experiment's `view/` consume commands and snapshots without reimplementing collision or progression. Renderer-specific values do not cross into rule libraries.
- **An experiment depends only on its own library and Godot.** Its `view/` does not include the host purposes, the main game, or another experiment; the host reaches an experiment only through its catalog entry and scene.
- **Each new rules boundary earns its place through independent behavior.** Similar value shapes do not authorize a shared engine or plugin framework.
- **A cycle between folders means the split is wrong**, and so does a file that belongs in two of them.
- **The application has one composition site**, `src/scene.cpp`. It is the one place that constructs the main game's and the host's concrete implementations. An experiment's node constructs its own rules engine, since its scene must also run without the host.
- **Every other unit receives what it depends on and constructs none of it.**
- **The composition site stays thin.** It constructs and passes; it decides nothing else.
- `src/register_types.cpp` is the GDExtension entry point, at the `src/` root as in the godot-cpp template.

This rule governs over `docs/convention/simplicity.md` §Rules, "A division is a response, not a plan", for the folders under `src/`: the requester chose to create the purpose folders now so the structure holds as the game grows, and under `docs/architecture/README.md` §Rules 8 a structural rule here decides that case.

## Repository layout

- `src/` holds the C++ sources, one folder per purpose, with `register_types.cpp` and `scene.*` at its root.
- `project/` is the Godot project and holds every Godot resource. A level's scene and resources sit in `project/levels/<level>/`; the first level is `project/levels/ascent/`. An experiment's scene and its own resources sit in `project/experiment/<id>/`, mirroring its source folder. `project.godot` and the extension's `bin/neocity.gdextension` sit in `project/` itself.
- The GDScript verification harnesses sit in `project/tests/` and `project/e2e/`, outside the C++ source tree.

The `project/experiment/<id>/` placement governs over `docs/convention/stack-godot.md` §Naming and resources, "Keep a scene's scripts and dedicated resources beside it in its level folder", for experiment scenes, under `docs/architecture/README.md` §Rules 8: an experiment is not a level of the main game, and one id names both of its folders.

## Rationale

[ADR 0004](adr/0004-adopt-the-godot-cpp-template-layout.md) records the purpose folders and the godot-cpp template layout, superseding the domain partitioning of [ADR 0001](adr/0001-partition-the-system-by-domain.md). [ADR 0002](adr/0002-isolate-the-ascent-engine-from-presentation.md) introduced the rules/presentation boundary and [ADR 0003](adr/0003-implement-the-ascent-domain-in-cpp.md) implements both sides in C++; the ascent boundary remains the `gameplay` target. [ADR 0005](adr/0005-isolate-experiment-rules-behind-one-host.md) adds the `experiment` purpose with one self-contained folder per experiment.

## Open questions

- When `gameplay` grows enough to need subfolders, and how they are cut.
- Whether a future host requires a different composition site; the current one remains the C++ `AscentScene` loaded by Godot, with session orchestration supplied as a dependency.
- Whether production needs a graphics backend beyond the Godot canvas; the current integration is a godot-cpp GDExtension.
