# neocity

A playable C++-driven neon-city jump-course prototype hosted by Godot.

## Play locally

On macOS, install the project-local tools once with `./scripts/setup-macos.sh`, then double-click `play.command` in Finder. The tools stay under ignored `.tools/`; the installer verifies the official Godot archive and does not modify global PATH or require an account. Xcode Command Line Tools must be present for native compilation and supply the Python that installs the pinned C++ tools under `.tools/cpp` on the first build.

`play.command` builds and imports the release library on the first run. For development, run `make run` to rebuild before playing. On other development systems, provide a C++20 compiler, Python 3 with `venv`, and standard Godot 4.7.2 on PATH, then use `make run`. Linux x86_64 with GCC 14.2 and Clang 19.1 is verified locally; other platforms, macOS included, are not.

| Input | Action |
|---|---|
| A / D or left / right | Move |
| Space, W, or up | Jump; hold for a higher jump |
| Tab | Change the leading character |
| R | Retry at the reached checkpoint; after completion, begin a new run |
| Escape | Pause or continue |

Climb the maintenance platforms, avoid urban pests, reach the cyan checkpoint, and collect the glowing spice fruit at the top. There is one course, not the full campaign. The companion follows a delayed visual trace; the party has one collision body. This reversible prototype control choice is not the final two-character mechanic.

## Engine and screen ownership

- `ascent/engine` owns deterministic rules, movement, collisions, obstacles, checkpoints, and completion. It has no Godot or rendering dependencies and can be tested by itself.
- `ascent/godot` owns Godot input translation, camera, resources, canvas drawing, HUD, and the composition root. It depends only on the engine's public commands and value snapshots. Custom engine and screen orchestration are C++; Godot still supplies window/input/canvas services. No standalone GPU backend is claimed.
- [Architecture](docs/architecture/README.md) records package boundaries. [Art direction](docs/art/README.md) identifies the selected concepts, production-source provenance, and remaining decisions.

## Checks and toolchains

```sh
make ci
```

- `make lint`: repository documentation checks and clang-tidy with warnings as errors.
- `make format`: clang-format check in LLVM style; apply it with `.tools/cpp/bin/clang-format --style=LLVM -i` on the changed files.
- `make build`: optimized CMake build of both C++ targets, pinned Godot version, and resource import.
- `make test`: engine unit tests through CTest, configured main-scene readiness, and live Godot move/jump/retry input checks.
- `make ci`: the complete set. Once `build/` is configured, engine-only checks run without Godot: `.tools/cpp/bin/cmake --build build --target ascent_engine_test`, then `.tools/cpp/bin/ctest --test-dir build`.

C++20, godot-cpp 10.0.0-stable targeting the Godot 4.7 API, and standard Godot 4.7.2 are selected for this slice; [ADR 0003](docs/architecture/adr/0003-implement-the-ascent-domain-in-cpp.md) lists every toolchain pin and its source. The native binding follows the [Godot 4.7 godot-cpp CMake documentation](https://docs.godotengine.org/en/4.7/tutorials/scripting/cpp/build_system/cmake.html). `neocity.gdextension` selects the optimized library in `build/` for both editor and game; build before importing a fresh checkout.

CI uses the pinned Python setup action and the checksum-verified Linux Godot helper, then calls the same `make ci`. Dependabot covers GitHub Actions; @sauce-git maintains Godot pins/runner selection and the C++ toolchain and dependency pins in `Makefile` and the CMake files. No platform export or audio pipeline is selected.

## Contributing

Read [AGENTS.md](AGENTS.md), [conventions](docs/convention/README.md), [architecture](docs/architecture/README.md), and [collaboration](docs/collaboration.md) before changing responsibilities. Changes land through the existing PR template. Use the repository's propose-art skill for scoped visual work; final art/gameplay acceptance remains separate from engine tests.
