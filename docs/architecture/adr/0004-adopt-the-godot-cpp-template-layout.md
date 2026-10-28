# ADR 0004: Adopt the godot-cpp template layout

> Status: accepted
> Date: 2026-10-28

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

The Godot project root was the repository root. Godot's importer scans every folder under the project root, so the C++ sources, `build/`, `docs/` and `.tools/` each needed a `.gdignore`, and `make build` recreated `build/.gdignore` to hide the test `project.godot` that the fetched godot-cpp archive carries. On 2026-10-28 the requester decided to move to the layout of the official template while the codebase is small (issue #12).

The reference is [`godotengine/godot-cpp-template` at `d36f9d03e9e02dad22829f1faf4e03a347616df5`](https://github.com/godotengine/godot-cpp-template/tree/d36f9d03e9e02dad22829f1faf4e03a347616df5): the Godot project in `project/` with `project/project.godot`, the extension's `.gdextension` file in `project/bin/`, C++ sources in `src/`, and `CMakeLists.txt` and `build_profile.json` at the repository root.

Alternatives considered:

- **Keep the repository root as the Godot project.** Every new top-level folder that is not a Godot resource would need its own `.gdignore`, and a missing one shows up only as import noise or a stray resource.
- **Adopt the template whole**, including its flat `src/`, `SConstruct`, `methods.py`, `.clang-format`, `doc_classes/`, `.vscode/` and git submodule. The flat `src/` would discard the domain folders of ADR 0001 and the engine/adapter split of ADR 0003; the build system, formatter style and fetch method are ADR 0003's decisions and out of scope here.

## Decision

Take the template's split between the Godot project and the C++ sources, and keep the domain shape inside `src/`:

- `project/` is the Godot project: `project.godot`, `main.tscn`, `bin/neocity.gdextension`, the ascent textures in `project/ascent/`, and the GDScript harnesses in `project/tests/` and `project/e2e/`.
- `src/<domain>/` holds each domain's C++ packages; ascent's are `src/ascent/engine` and `src/ascent/godot`.
- `CMakeLists.txt` and `build_profile.json` sit at the repository root. `build/` stays the CMake build directory, and the extension library is written into `project/bin/`.
- godot-cpp stays a pinned FetchContent archive.

## Consequences

- Godot sees only `project/`; no `.gdignore` is needed for the C++ sources, documents, build directory or local tools.
- Every Godot command names `--path project`, and every `res://` path is relative to `project/`.
- The GDScript harnesses run as `--main-loop` classes, which Godot registers only from inside the project, so they live in `project/tests/` and `project/e2e/` rather than at the repository root.
- A domain's Godot resources and its C++ adapter are in two trees: `project/<domain>/` and `src/<domain>/godot/`.
- The built library lands in a tracked folder, so `.gitignore` excludes everything in `project/bin/` except the `.gdextension` file and its `.uid`.

## Signs the decision was wrong

- Folders other than Godot resources start appearing under `project/` and need `.gdignore` files again.
- Changes to one domain routinely touch both `project/<domain>/` and `src/<domain>/godot/` in ways that a single folder would have kept together, or the two drift apart.
- An export or editor workflow needs the C++ sources or build output inside the Godot project.
