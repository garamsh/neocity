# ADR 0004: Adopt the godot-cpp template layout

> Status: accepted; single rules boundary superseded by ADR-0005
> Date: 2026-10-28

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

The Godot project root was the repository root. Godot's importer scans every folder under the project root, so the C++ sources, `build/`, `docs/` and `.tools/` each needed a `.gdignore`, and `make build` recreated `build/.gdignore` to hide the test `project.godot` that the fetched godot-cpp archive carries. The C++ code sat in one domain folder, `ascent`, with an engine package and a Godot package, as ADR 0001 and ADR 0003 decided.

On 2026-10-28 the requester decided, while the codebase is small (issue #12):

- to move to the layout of the official godot-cpp template;
- to drop the `ascent` domain folder and keep `ascent` only as the name of the first level;
- to organize `src/` into shallow purpose folders, created now so the structure holds as the game grows;
- to name the rules folder `gameplay`, not `engine` (confusable with Godot) or `core` (a catch-all).

References:

- [`godotengine/godot-cpp-template` at `d36f9d03e9e02dad22829f1faf4e03a347616df5`](https://github.com/godotengine/godot-cpp-template/tree/d36f9d03e9e02dad22829f1faf4e03a347616df5): the Godot project in `project/` with `project/project.godot`, the extension's `.gdextension` file in `project/bin/`, `src/register_types.cpp` and the other C++ sources in `src/`, and `CMakeLists.txt` and `build_profile.json` at the repository root.
- [Godot 4.7 project organization guide at godot-docs `9adca4c1c72917bfe1b7be3108abed5ce26696a6`](https://github.com/godotengine/godot-docs/blob/9adca4c1c72917bfe1b7be3108abed5ce26696a6/tutorials/best_practices/project_organization.rst): its example keeps a level's scenes and assets together under `/levels/riverdale/`.

Alternatives considered:

- **Keep the repository root as the Godot project.** Every new top-level folder that is not a Godot resource would need its own `.gdignore`, and a missing one shows up only as import noise or a stray resource.
- **Keep domain folders inside `src/`.** The requester replaced domain partitioning with purpose folders.
- **Adopt the template whole**, including its flat `src/`, `SConstruct`, `methods.py`, `.clang-format`, `doc_classes/`, `.vscode/` and git submodule. A flat `src/` would lose the build-enforced gameplay boundary; the build system, formatter style and fetch method are ADR 0003's decisions and out of scope here.

## Decision

Take the template's split between the Godot project and the C++ sources, and partition `src/` by purpose. This supersedes ADR 0001's domain partitioning, and ADR 0003's single-domain framing and target names; ADR 0003's toolchain selections stand.

- `project/` is the Godot project: `project.godot`, `bin/neocity.gdextension`, the GDScript harnesses in `tests/` and `e2e/`, and one folder per level under `levels/`. The first level, `levels/ascent/`, holds its scene `ascent.tscn` and its textures.
- `src/` holds one shallow folder per purpose: `gameplay` (the game rules, with public headers in `include/gameplay/` and implementation and tests in `src/`), `camera`, `input`, `rendering` and `hud`. `register_types.cpp` and the composition site `scene.cpp` sit at the `src/` root.
- CMake targets are `gameplay` (static library), `gameplay_test`, and `neocity`, the GDExtension built from everything outside `gameplay`. The library is `project/bin/libneocity.*` with entry symbol `neocity_init`. Namespaces follow the folders under `neocity::`.
- `CMakeLists.txt` and `build_profile.json` sit at the repository root; `build/` stays the CMake build directory. godot-cpp stays a pinned FetchContent archive.
- Kept from earlier decisions: gameplay depends on nothing in Godot, the application has one composition site, and no generic common, shared or util folder exists.

## Consequences

- Godot sees only `project/`; no `.gdignore` is needed for the C++ sources, documents, build directory or local tools.
- Every Godot command names `--path project`, and every `res://` path is relative to `project/`.
- The GDScript harnesses run as `--main-loop` classes, which Godot registers only from inside the project, so they live in `project/tests/` and `project/e2e/`.
- The gameplay boundary stays enforced by the build: the `gameplay` target's only include directory is its own `include/`. The other purpose folders share one target, so their separation is held by review, not by the compiler.
- Purpose folders exist before most of them hold more than one file pair, contrary to the default of starting flat; `structure.md` records that this rule governs here.
- The built library lands in a tracked folder, so `.gitignore` excludes everything in `project/bin/` except the `.gdextension` file and its `.uid`.

## Signs the decision was wrong

- Folders other than Godot resources start appearing under `project/` and need `.gdignore` files again.
- A change routinely edits several purpose folders together, or a file cannot be placed in one purpose without argument.
- A generic folder appears to hold what two purposes share, or purpose folders start including each other in a cycle.
- `gameplay` grows a dependency on Godot, or presentation code starts reimplementing rules gameplay owns.
