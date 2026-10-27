# Godot runtime

The minimal engine host and the checks that establish it can start.

## Current decisions

- `project.godot` lives at the repository root and selects `main.tscn` as the entry scene. Its root is a neutral `Node`, with no game domains or runtime scripts.
- The standard Godot 4.7.2 editor is the selected engine. Compatibility rendering gives the empty host a local window without selecting 2D or 3D gameplay.
- `make build` imports the project; `make test` observes the configured scene becoming ready through Godot's normal startup, using a native GDScript main loop. These are distinct checks; neither exports a game.
- Non-resource documentation is excluded from import by `docs/.gdignore`. Authored resources and script UID metadata are tracked; `.godot/` remains a disposable cache.
- CI installs the official Linux editor using a recorded release checksum, then invokes the same `make ci` entry point as local contributors.

## Rationale

[ADR 0001](adr/0001-partition-the-system-by-domain.md) leaves game domains and Godot/Rust integration until implementation establishes them. The empty host supplies a runnable entry point without constructing domain implementations or settling those open choices. The startup harness observes the shipped scene rather than assembling a separate scene graph.

The engine release, root project location, and checksum are bootstrap configuration; they introduce no new architectural rule and require no additional ADR.

## Open questions

- Gameplay, dimensionality, export targets, and production scripting language.
- The Rust integration mechanism and the division of engine and visualization responsibilities.
- The composition site and domain resource paths when the first game behavior is implemented.
