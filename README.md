# neocity

Two independent frying-pan control experiments and the existing neon-city ascent reference, in one Godot-hosted C++ application.

## Play locally

On macOS, install the project-local tools once with `./scripts/setup-macos.sh`, then double-click `play.command` in Finder. The tools stay under ignored `.tools/`; the installer verifies the official Godot archive and does not modify global PATH or require an account. Xcode Command Line Tools must be present for native compilation and supply the Python that installs the pinned C++ tools under `.tools/cpp` on the first build.

`play.command` builds and imports the release library on the first run. For development, run `make run` to rebuild before playing. On other development systems, provide a C++20 compiler, Python 3 with `venv`, and standard Godot 4.7.2 on PATH, then use `make run`. Linux x86_64 with GCC 14.2 and Clang 19.1 is verified locally; other platforms, macOS included, are not.

The initial menu offers **1: direct dodge**, **2: tilt escape**, and **3: ascent reference**. M returns to that menu from any run. Release held keys after selecting, restarting, or returning to the menu before giving the next input.

| Pan input | Action |
|---|---|
| WASD or arrow keys | Move directly in experiment 1; tilt the whole pan in experiment 2 |
| R | Start a fresh experiment, including after success or failure |
| M | Return to the common menu |

Avoid the red circles and move the blue player through the green opening at the top. Direct movement stops on release. Tilt moves both the player and the hazards: release levels the pan but leaves inertia, so reverse direction early to brake. Contact displays a loss; escaping displays a win. These are plain geometric experiments, not a final game selection or a claim of tested fun or balance.

For a direct escape, move up along the left side to above the highest red circle, right toward the opening, then up through it. For tilt, take the left route, brake before the upper rim, steer right toward the opening, and tilt upward. The input-only harness below reproduces a complete winning route for each mode, as well as losses and restarts.

The ascent reference keeps its original controls:

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
- `src/pan_dodge` and `src/pan_tilt` independently own direct movement and tilt physics, contact, outcomes, and reset. Each exposes its own commands and snapshots and has no Godot, ascent, or mutual dependencies.
- `src/session` owns common selection and run timing. `src/input`, `src/camera`, `src/rendering`, and `src/hud` own input translation, the camera, resources and canvas drawing, and the HUD; `src/scene.cpp` remains the composition root and `src/register_types.cpp` the extension entry point. They depend only on the rule libraries' public commands and value snapshots. Custom engine and screen orchestration are C++; Godot still supplies window/input/canvas services. No standalone GPU backend is claimed.
- `project/` is the Godot project: the extension's `bin/neocity.gdextension`, the ascent level's scene and textures in `levels/ascent/`, and the GDScript checks. Open `project/project.godot` in the editor after `make build`.
- [Architecture](docs/architecture/README.md) records package boundaries. [Art direction](docs/art/README.md) identifies the selected concepts, production-source provenance, and remaining decisions.

## Checks and toolchains

```sh
make ci
```

- `make lint`: clang-tidy with warnings as errors.
- `make format`: clang-format check in LLVM style; apply it with `.tools/cpp/bin/clang-format --style=LLVM -i` on the changed files.
- `make build`: optimized CMake build of the rule libraries and extension, pinned Godot version, and resource import.
- `make test`: all three rule libraries' unit tests through CTest, configured main-scene readiness, ascent move/jump/retry input checks, 18 live experiment checks for menu selection, direct movement, shared tilt, losses, wins, replay, and transitions, and three session integration checks for partial-tick time and input edges.
- `make ci`: the complete set. Once `build/` is configured, rules-only checks run without Godot: `.tools/cpp/bin/cmake --build build --target gameplay_test pan_dodge_test pan_tilt_test`, then `.tools/cpp/bin/ctest --test-dir build`.

Each new rule library also configures from its own folder without fetching or building Godot. After the first Make entry point installs the pinned tools, run both independent builds from the repository root:

```sh
for experiment in dodge tilt; do
  .tools/cpp/bin/cmake -S "src/pan_$experiment" -B "build/pan-$experiment-only" -G Ninja -DCMAKE_MAKE_PROGRAM="$PWD/.tools/cpp/bin/ninja" -DCMAKE_BUILD_TYPE=Release
  .tools/cpp/bin/cmake --build "build/pan-$experiment-only"
  .tools/cpp/bin/ctest --test-dir "build/pan-$experiment-only" --output-on-failure --no-tests=error
done
```

After `make build`, reproduce the real-input escape routes and transition checks with:

```sh
godot --headless --path project --main-loop PanExperimentsCheck --fixed-fps 120
```

Remove `--headless` to watch the same key-driven run in one window. The harness steers the tilt player along three waypoints, using opposite tilt to brake; it does not set positions or outcomes. `godot --path project --main-loop PanExperimentsCapture --fixed-fps 120` saves actual menu/direct/tilt frames as `user://pan-menu.png`, `user://pan-direct.png`, and `user://pan-tilt.png`; it requires a rendering display. The preserved ascent capture uses `--main-loop AscentCapture` and now selects ascent through the menu. Actual rendered examples: [menu](docs/architecture/evidence/pan-experiments/menu.png), [direct dodge](docs/architecture/evidence/pan-experiments/direct.png), and [tilt escape](docs/architecture/evidence/pan-experiments/tilt.png).

C++20, godot-cpp 10.0.0-stable targeting the Godot 4.7 API, and standard Godot 4.7.2 are selected for this slice; [ADR 0003](docs/architecture/adr/0003-implement-the-ascent-domain-in-cpp.md) lists every toolchain pin and its source. The native binding follows the [Godot 4.7 godot-cpp CMake documentation](https://docs.godotengine.org/en/4.7/tutorials/scripting/cpp/build_system/cmake.html). `project/bin/neocity.gdextension` selects the optimized library that `make build` writes into `project/bin/`, for both editor and game; build before importing a fresh checkout.

CI uses the pinned Python setup action and the checksum-verified Linux Godot helper, then calls the same `make ci`. Dependabot covers GitHub Actions; @sauce-git maintains Godot pins/runner selection and the C++ toolchain and dependency pins in `Makefile` and the CMake files. No platform export or audio pipeline is selected.

## Contributing

Read [AGENTS.md](AGENTS.md), [conventions](docs/convention/README.md), [architecture](docs/architecture/README.md), and [collaboration](docs/collaboration.md) before changing responsibilities. Changes land through the existing PR template. Use the repository's propose-art skill for scoped visual work; final art/gameplay acceptance remains separate from engine tests.
