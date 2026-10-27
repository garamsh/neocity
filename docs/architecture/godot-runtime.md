# Godot runtime

The minimal engine host and the checks that establish it can start.

## Current decisions

- `project.godot` lives at the repository root and selects `main.tscn` as the entry scene. Its root is a neutral `Node`, with no game domains or runtime scripts.
- The standard Godot 4.7.2 editor supplies the bootstrap host. Compatibility rendering opens its empty local window; this setting does not choose the production visual backend.
- Custom engine and visual systems will be authored in Rust, following the requester's clarification recorded on [task #4](https://github.com/garamsh/neocity/issues/4). No Rust runtime exists in this bootstrap. GDScript is limited to the startup harness, and Godot/Rust integration is not yet selected.
- `make build` imports the project; `make test` observes the configured scene becoming ready through Godot's normal startup, using a native GDScript main loop. These are distinct checks; neither exports a game.
- Non-resource documentation is excluded from import by `docs/.gdignore`. Authored resources and script UID metadata are tracked; `.godot/` remains a disposable cache.
- CI installs the official Linux editor using a recorded release checksum, then invokes the same `make ci` entry point as local contributors.

## Rationale

[ADR 0001](adr/0001-partition-the-system-by-domain.md) leaves game domains and Godot/Rust integration until implementation establishes them. The empty host supplies a runnable entry point without constructing domain implementations or settling those open choices. The startup harness observes the shipped scene rather than assembling a separate scene graph.

The engine release, root project location, and checksum are bootstrap configuration; they introduce no new architectural rule and require no additional ADR.

## Open questions

- Gameplay, dimensionality, export targets, and production scripting language.
- The Rust toolchain, integration/binding mechanism, graphics backend, and concrete boundary between custom Rust subsystems and Godot host services. Rust ownership of custom engine and visual implementation is selected; these implementation details remain open.
- The composition site and domain resource paths when the first game behavior is implemented.
