# Godot runtime

The minimal engine host and the checks that establish it can start.

## Current decisions

- The root `project.godot` launches `main.tscn` with the Rust AscentScene as the composition root. `neocity.gdextension` loads the Cargo release library.
- Standard Godot 4.7.2 hosts window, input, imported textures, and canvas primitives. The pure Rust engine and Rust presentation adapter are owned by [ascent](ascent.md); GDScript remains verification-only.
- Rust 1.99.1 and godot-rust 0.5.5 are pinned with a Cargo lockfile. The binding targets the Godot 4.7 API. The production GPU backend and broader platform integration remain open.
- `make build` builds Rust before Godot import. `make test` runs Rust tests, shipped-scene readiness, and live input checks. `make ci` includes lint/format and the full set. Concept images in `docs/`, local tools, Rust source, and Cargo output are excluded from Godot import where appropriate.
- macOS setup installs verified official tools under ignored `.tools/`; no global PATH changes or account authentication are required. CI prepares its pinned Rust toolchain and checksum-verified Linux Godot, then runs the same Make entry point.

## Rationale

[ADR 0001](adr/0001-partition-the-system-by-domain.md) selects domain ownership. [ADR 0002](adr/0002-isolate-the-ascent-engine-from-presentation.md) keeps game rules independent of the host. GDExtension supplies the smallest native binding for this slice without moving rules into GDScript or claiming a standalone renderer.

## Open questions

- Production graphics backend, export targets, audio, and distributable packaging.
- Additional domain composition and host services when later game behavior needs them.
