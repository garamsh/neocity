# neocity

A neon-city ascent course and a developer page of playable experiments, in one Godot-hosted C++ application.

## Play locally

On macOS, install the project-local tools once with `./scripts/setup-macos.sh`, then double-click `play.command` in Finder. The tools stay under ignored `.tools/`; the installer verifies the official Godot archive and does not modify global PATH or require an account. Xcode Command Line Tools must be present for native compilation and supply the Python that installs the pinned C++ tools under `.tools/cpp` on the first build.

`play.command` builds and imports the release library on the first run. For development, run `make run` to rebuild before playing. On other development systems, provide a C++20 compiler, Python 3 with `venv`, and standard Godot 4.7.2 on PATH, then use `make run`. Linux x86_64 with GCC 14.2 and Clang 19.1 is verified locally; other platforms, macOS included, are not.

The game opens on the main menu: **1** plays the main game, currently the ascent course below, and **2** opens the developer page. The developer page lists playable experiments by number; each is a plain prototype for trying an idea, not the main game. M goes back one level. Release held keys after a menu choice before giving the next input.

| Experiment | Try it |
|---|---|
| [`pan_dodge`](src/experiment/pan_dodge/README.md) | 3D. Move with WASD or the arrow keys on a pan that sways by itself, dodge the hazards dropping onto it, and leave through the green gate travelling around the rim. R restarts. |

An experiment's scene also runs on its own, for example `godot --path project res://experiment/pan_dodge/pan_dodge.tscn` after `make build`. To add one, see [Contributing](#contributing).

The ascent course keeps its original controls:

| Ascent input | Action |
|---|---|
| A / D or left / right | Move |
| Space, W, or up | Jump; hold for a higher jump |
| Tab | Change the leading character |
| R | Retry at the reached checkpoint; after completion, begin a new run |
| Escape | Pause or continue |

Climb the maintenance platforms, avoid urban pests, reach the cyan checkpoint, and collect the glowing spice fruit at the top. There is one course, not the full campaign. The companion follows a delayed visual trace; the party has one collision body. This reversible prototype control choice is not the final two-character mechanic.

## Engine and screen ownership

- `src/gameplay` owns deterministic rules, movement, collisions, obstacles, checkpoints, and completion. It has no Godot or rendering dependencies and can be tested by itself.
- `src/experiment/<id>/` holds one experiment: a rules library with no Godot, ascent, or mutual dependencies, its Godot node in `view/`, and its record. `src/experiment/catalog.hpp` lists them for the developer page.
- `src/session` owns the main menu, the developer page, and run timing. `src/input`, `src/camera`, `src/rendering`, and `src/hud` own input translation, the camera, resources and canvas drawing, and the HUD; `src/scene.cpp` remains the composition root and `src/register_types.cpp` the extension entry point. They depend only on the rule libraries' public commands and value snapshots. Custom engine and screen orchestration are C++; Godot still supplies window, input, canvas, and 3D services. No standalone GPU backend is claimed.
- `project/` is the Godot project: the extension's `bin/neocity.gdextension`, the ascent level's scene and textures in `levels/ascent/`, each experiment's scene in `experiment/<id>/`, and the GDScript checks. Open `project/project.godot` in the editor after `make build`.
- [Architecture](docs/architecture/README.md) records package boundaries. [Art direction](docs/art/README.md) identifies the selected concepts, production-source provenance, and remaining decisions.

## Checks and toolchains

```sh
make ci
```

- `make lint`: clang-tidy with warnings as errors.
- `make format`: clang-format check in LLVM style; apply it with `.tools/cpp/bin/clang-format --style=LLVM -i` on the changed files.
- `make build`: optimized CMake build of the rule libraries and extension, pinned Godot version, and resource import.
- `make test`: every rule library's unit tests through CTest, configured main-scene readiness, ascent move/jump/retry input checks, 18 live checks of both menus and the `pan_dodge` experiment, and three session integration checks for partial-tick time and input edges.
- `make ci`: the complete set. Once `build/` is configured, rules-only checks run without Godot: `.tools/cpp/bin/cmake --build build --target gameplay_test pan_dodge_test`, then `.tools/cpp/bin/ctest --test-dir build`.

Each experiment's rules library also configures from its own folder without fetching or building Godot; its record lists the commands. After the first Make entry point installs the pinned tools, run them for every experiment from the repository root:

```sh
for experiment in src/experiment/*/; do
  id=$(basename "$experiment")
  .tools/cpp/bin/cmake -S "$experiment" -B "build/$id-only" -G Ninja -DCMAKE_MAKE_PROGRAM="$PWD/.tools/cpp/bin/ninja" -DCMAKE_BUILD_TYPE=Release
  .tools/cpp/bin/cmake --build "build/$id-only"
  .tools/cpp/bin/ctest --test-dir "build/$id-only" --output-on-failure --no-tests=error
done
```

After `make build`, reproduce the real-input menu, experiment, and direct-launch checks with:

```sh
godot --headless --path project --main-loop PanExperimentsCheck --fixed-fps 120
```

Remove `--headless` to watch the same key-driven run in one window. The harness only presses keys: it steers toward the travelling gate, circles to dodge drops, and stands still to be hit; it does not set positions or outcomes. `godot --path project --main-loop PanExperimentsCapture --fixed-fps 120` saves real main-menu, developer-page, and `pan_dodge` frames as `user://main-menu.png`, `user://developer.png`, and `user://pan-dodge.png`; it requires a rendering display. The ascent capture uses `--main-loop AscentCapture` and selects the main game through the menu. Rendered examples: [main menu](docs/architecture/evidence/pan-experiments/main-menu.png), [developer page](docs/architecture/evidence/pan-experiments/developer.png), and [`pan_dodge`](docs/architecture/evidence/pan-experiments/pan-dodge.png).

C++20, godot-cpp 10.0.0-stable targeting the Godot 4.7 API, and standard Godot 4.7.2 are selected for this slice; [ADR 0003](docs/architecture/adr/0003-implement-the-ascent-domain-in-cpp.md) lists every toolchain pin and its source. The native binding follows the [Godot 4.7 godot-cpp CMake documentation](https://docs.godotengine.org/en/4.7/tutorials/scripting/cpp/build_system/cmake.html). `project/bin/neocity.gdextension` selects the optimized library that `make build` writes into `project/bin/`, for both editor and game; build before importing a fresh checkout.

CI uses the pinned Python setup action and the checksum-verified Linux Godot helper, then calls the same `make ci`. Dependabot covers GitHub Actions; @sauce-git maintains Godot pins/runner selection and the C++ toolchain and dependency pins in `Makefile` and the CMake files. No platform export or audio pipeline is selected.

## Contributing

Read [AGENTS.md](AGENTS.md), [conventions](docs/convention/README.md), [architecture](docs/architecture/README.md), and [collaboration](docs/collaboration.md) before changing responsibilities. Changes land through the existing PR template. Use the repository's propose-art skill for scoped visual work. To add a developer-page experiment, follow [collaboration](docs/collaboration.md#adding-an-experiment) and the create-experiment skill. Final art and gameplay acceptance remain separate from engine tests.
