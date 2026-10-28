# Godot runtime

The minimal engine host and the checks that establish it can start.

## Current decisions

- The root `project.godot` launches `main.tscn` with the C++ AscentScene as the composition root. `neocity.gdextension` loads the optimized CMake-built library from the ignored `build/` directory for both editor and game.
- Standard Godot 4.7.2 hosts window, input, imported textures, and canvas primitives. The pure C++ engine and C++ presentation adapter are owned by [ascent](ascent.md); GDScript remains verification-only.
- C++20 with godot-cpp 10.0.0-stable targeting the Godot 4.7 API. godot-cpp and GoogleTest are pinned by archive digest in the CMake files; CMake, Ninja, clang-format, and clang-tidy are pinned by wheel hash in `Makefile` and installed under ignored `.tools/cpp` by the first entry point that needs them. The production GPU backend and broader platform integration remain open.
- `make build` configures and compiles the C++ targets before Godot import. `make test` runs the engine unit tests through CTest, shipped-scene readiness, and live input checks. `make lint` runs clang-tidy and `make format` checks clang-format; `make ci` runs the full set. Concept images in `docs/`, local tools, C++ source, and the build directory are excluded from Godot import.
- macOS setup installs the verified official Godot under ignored `.tools/` and requires the Xcode Command Line Tools for the compiler and Python; no global PATH changes or account authentication are required. CI prepares Python with the pinned setup action and checksum-verified Linux Godot, then runs the same Make entry point with the runner's system compiler.

## Rationale

[ADR 0001](adr/0001-partition-the-system-by-domain.md) selects domain ownership. [ADR 0002](adr/0002-isolate-the-ascent-engine-from-presentation.md) keeps game rules independent of the host, and [ADR 0003](adr/0003-implement-the-ascent-domain-in-cpp.md) selects C++ with godot-cpp. GDExtension supplies the smallest native binding for this slice without moving rules into GDScript or claiming a standalone renderer.

## Open questions

- Production graphics backend, export targets, audio, and distributable packaging.
- Additional domain composition and host services when later game behavior needs them.
