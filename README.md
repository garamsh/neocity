# neocity

Monster Mouth Escape, a survival game for one or two players on one computer, built as a Godot-hosted C++ application.

## How to play

A monster has swallowed the city. Play the chef or the appraiser while AI plays the other, or play both with a friend. Walk, run, climb, and jump through the side-on city to gather ingredients and tools before the day's time runs out; a fall into the acid sends you back to base. Appraise them and cook at base, then feed the monster to start the next day. New and delicious food holds the acid back; anything else raises it, and rising acid and time wreck the city and take the stock left in it. A day lasts 18 seconds of real time, a clock both actors share. A collection holds whoever makes it in place for 1.5 seconds while the clock runs on, so both actors can collect at once, and the action button shows the time the hold has left. From day 2 to day 50 three small bags wash up across the city as each day starts, and the tide takes each back after five days; an emptied bag disappears from the map. If the acid fills, the run is lost. From day 30, each finished day has a 20% chance that the party escapes. Either way the run then plays its ending and returns to the title screen, and the E key skips it. The opening, ending, and loss scenes of [#29](https://github.com/garamsh/neocity/issues/29) are not delivered, so a placeholder card stands in for each. The map, balance, keys, and art are temporary. The screen keeps to its corners, in dark pixel panels like the concept art: the day, the acid and the time left along the top, with the Esc key that pauses under the time; each actor's face, hearts and stamina, the bag, and in the city a large button for what the E key does where the player stands, and on what, along the bottom. A key cap over a character's head shows when its key acts there, a line under the acid suggests a next step, and what happened shows as notes that fade. At base one card names what to do now, such as cooking or feeding, and the stored dishes show their scores. The pause screen lists every key and the recipe book with what feeding does. The interface is set in the Tiny5 pixel font, under the SIL Open Font License.

The game opens on the title screen, the monster's mouth behind the game's logo:

- **New game** asks who is playing: **1** the chef, **2** the appraiser, **3** two players.
- **Controls** lists every key for both players, the pause screen's list with the pause and leave keys after it.
- **Quit** closes the game.

On the title screen and on the choice of players, move the cursor with W or up and S or down, and press Enter; 1, 2, and 3 choose an entry directly. M or Esc goes back to the title screen. During a run, M asks first, because progress is not saved. Release held keys after a menu choice before giving the next input.

| Input | Player 1 | Player 2 |
|---|---|---|
| Move | WASD or arrows | IJKL |
| Run | Shift | H |
| Jump | Space | ; |
| Set out or return to base | Q | U |
| Collect or repair; appraise or cook at base | E | O |
| Choose the dish to cook, as the chef at base | W/S or up/down arrows | none |
| Replace a damaged base tool | R | P |
| Feed a stored dish | F | [ |
| End the day unfed | G | ] |
| Swap actor, playing alone | Tab | none |
| Hand your actor to AI, playing together | X | N |
| Pause or continue, listing every key | Esc | Esc |
| Leave the run for the title screen | M, then Enter; Esc cancels | M, then Enter; Esc cancels |
| Change the effects or music volume, on the pause screen | W/S or up/down to choose, A/D or left/right to change | the same |

## Run it

On macOS, install the project-local tools once with `./scripts/setup-macos.sh`, then double-click `play.command` in Finder. The tools stay under ignored `.tools/`; the installer verifies the official Godot archive and does not modify global PATH or require an account. Xcode Command Line Tools must be present for native compilation and supply the Python that installs the pinned C++ tools under `.tools/cpp` on the first build.

`play.command` builds and imports the release library on the first run. For development, run `make run` to rebuild before playing. On other development systems, provide a C++20 compiler, Python 3 with `venv`, and standard Godot 4.7.2 on PATH, then use `make run`. Linux x86_64 with GCC 14.2 and Clang 19.1 is verified locally; other platforms, macOS included, are not.

## Engine and screen ownership

- `src/survival` owns the rules of Monster Mouth Escape and its temporary content, with no Godot dependency.
- `src/session` owns the title screen, its controls page, and run timing. `src/input`, `src/rendering`, and `src/hud` own input translation, resources and canvas drawing, and the HUD and menus; `src/scene.cpp` remains the composition root and `src/register_types.cpp` the extension entry point. They depend only on the rules library's public commands and value snapshots. Custom engine and screen orchestration are C++; Godot still supplies window, input, and canvas services. No standalone GPU backend is claimed.
- `project/` is the Godot project: the host scene `main.tscn`, the extension's `bin/neocity.gdextension`, the game's art set in `levels/monster_mouth_escape/art/`, and the GDScript checks. Open `project/project.godot` in the editor after `make build`.
- [Architecture](docs/architecture/README.md) records package boundaries. [Art direction](docs/art/README.md) identifies the selected concepts, production-source provenance, and remaining decisions.

## Checks and toolchains

```sh
make ci
```

- `make lint`: clang-tidy with warnings as errors.
- `make format`: clang-format check in LLVM style; apply it with `.tools/cpp/bin/clang-format --style=LLVM -i` on the changed files.
- `make build`: optimized CMake build of the rules library and extension, pinned Godot version, and resource import.
- `make test`: the survival library's unit tests through CTest, configured main-scene readiness, live checks of the title screen, live checks pressing the arrow keys and player two's keys, the survival fixed-step checks at two frame rates and on the real clock, and live checks playing the survival game with real key events.
- `make ci`: the complete set. Rules-only checks run without Godot and without a prior `make build`; once the pinned tools are installed by the first Make entry point, from the repository root:

```sh
.tools/cpp/bin/cmake -S . -B build -G Ninja -DCMAKE_MAKE_PROGRAM="$PWD/.tools/cpp/bin/ninja" -DCMAKE_BUILD_TYPE=Release -DPython3_EXECUTABLE="$PWD/.tools/cpp/bin/python"
.tools/cpp/bin/cmake --build build --target survival_test
.tools/cpp/bin/ctest --test-dir build
```

After `make build`, `godot --headless --path project --main-loop TitleMenuCheck --fixed-fps 60` reproduces the title screen's real-key checks; remove `--headless` to watch them in one window.

C++20, godot-cpp 10.0.0-stable targeting the Godot 4.7 API, and standard Godot 4.7.2 are selected for this slice; [ADR 0003](docs/architecture/adr/0003-implement-the-ascent-domain-in-cpp.md) lists every toolchain pin and its source. The native binding follows the [Godot 4.7 godot-cpp CMake documentation](https://docs.godotengine.org/en/4.7/tutorials/scripting/cpp/build_system/cmake.html). `project/bin/neocity.gdextension` selects the optimized library that `make build` writes into `project/bin/`, for both editor and game; build before importing a fresh checkout.

CI uses the pinned Python setup action and the checksum-verified Linux Godot helper, then calls the same `make ci`. Dependabot covers GitHub Actions; @sauce-git maintains Godot pins/runner selection and the C++ toolchain and dependency pins in `Makefile` and the CMake files. No platform export is selected. Sound plays through Godot's audio players on an Effects and a Music bus; every sound is a CC0 file whose source the [sound provenance](docs/art/production/monster-mouth-escape-sound-provenance.txt) records.

## Contributing

Read [AGENTS.md](AGENTS.md), [conventions](docs/convention/README.md), [architecture](docs/architecture/README.md), and [collaboration](docs/collaboration.md) before changing responsibilities. Changes land through the existing PR template. Use the repository's propose-art skill for scoped visual work. Final art and gameplay acceptance remain separate from engine tests.
