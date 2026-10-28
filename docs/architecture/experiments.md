# Experiments

The developer-page experiments: what one is made of, how it is registered and run, how it is verified, and how it relates to the main game.

## Current decisions

### Registered experiments

| Id | Record | Scene |
|---|---|---|
| `pan_dodge` | [`src/experiment/pan_dodge/README.md`](../../src/experiment/pan_dodge/README.md) | `project/experiment/pan_dodge/pan_dodge.tscn` |

### What an experiment is made of

- One `snake_case` id, used for both of its folders, its catalog entry, and its library and test target names.
- `src/experiment/<id>/` holds an independent rules library under the structural rules in [structure](structure.md), its Godot node in `view/`, and its record `README.md`. The record follows the [experiment template](../development/experiment-template.md) and is the experiment's responsibility document: its rules, controls, provenance, verification, and status live there, not in `docs/architecture/`.
- `project/experiment/<id>/<id>.tscn` is a scene whose root is the experiment's node. Any resource used only by the experiment sits beside it.
- The node builds its own camera, lights, meshes, and HUD from Godot primitives, reads Godot input actions, and steps its rules from `_physics_process`, which `project/project.godot` runs at 120 Hz. R restarts the run through the rules library's reset.
- Design-free by default: plain shapes and text. Art, story, or characters need their own scoped approval.

### Registration and launch

Besides its two folders, an experiment touches exactly these shared files, each with one entry:

- `src/experiment/catalog.hpp`: id, title, one-line summary, and scene path, in developer-page order. No folder discovery or manifest parsing happens at runtime.
- `src/register_types.cpp`: its node class.
- Root `CMakeLists.txt`: its library's `add_subdirectory`, its `view/` sources in the `neocity` target, and its library in that target's links.
- `build_profile.json`: any Godot class its node newly uses.
- `project/e2e/experiments.gd` and the line `Makefile` matches: its live route.

The developer page selects at most nine experiments, the digit keys `option_1` to `option_9` in `project/project.godot`; a tenth fails to compile.
- The developer page numbers the catalog. Choosing an entry mounts its scene under the host once every control is released; M removes it and returns to the developer page. Running the scene file directly plays the identical scene.
- An experiment never mutates the host, the main game, or another experiment.

### Verification

- Unit tests cover the rules' observable behaviour, each shown to fail when its behaviour is removed.
- The library configures, builds, and tests from its own folder without Godot.
- `project/e2e/experiments.gd` drives the shipped host with real key events: open the developer page, mount, play to each terminal outcome, retry, return, and launch the scene directly. Each new experiment adds its route to that harness, and the line it prints is matched in `Makefile`.
- Correctness results are kept apart from play feedback. Passing checks or a capture do not show that an experiment is fun or balanced.

### Main game

- The main menu's first entry is the main game, currently the [ascent](gameplay.md) course. An experiment becomes part of the main game only by an explicit direction decision from the issue's creative decision-maker and a separately scoped integration issue; registering or merging an experiment grants neither.

## Rationale

[ADR 0005](adr/0005-isolate-experiment-rules-behind-one-host.md) records why experiments are self-contained folders behind one host, registered at compile time, and played from the same scene in both entries.

## Open questions

- Space-bar jumping in `pan_dodge` and [smonidema's proposal](https://github.com/garamsh/neocity/discussions/15#discussioncomment-19232632) for character-specific jump puzzles with rankings, including whether rankings are local or online, are deferred by [issue #16](https://github.com/garamsh/neocity/issues/16).
- Whether experiments outgrow one shared live harness file.
