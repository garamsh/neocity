# Session

The main menu, the developer page, run lifetime, and fixed-step delivery to the survival and ascent games.

## Current decisions

- `src/session/` receives the ascent engine, its camera and fresh-run reset, a survival run factory, the experiment catalog, a mount operation, and a `CinematicStage` from `src/scene.cpp`. It decides which screen is open and delegates drawing to rendering and HUD.
- The start screen is the main menu: 1 opens the survival game's choice of players, 2 opens the developer page, and 3 plays the earlier ascent prototype. The choice of players takes 1 for the chef, 2 for the appraiser, and 3 for two local players. The developer page numbers the [registered experiments](experiments.md#registered-experiments). M goes back one level: from a game, the choice of players, or the developer page to the main menu, and from an experiment to the developer page.
- Every transition clears accumulated time, pending input edges, and the open game's run, closes any cinematic, and unmounts any experiment. Controls must be released before the next screen accepts input; an experiment's scene is mounted only then, so a held key cannot become its first action.
- An experiment, once mounted, reads its own input and steps its own rules; session only mounts and unmounts it.
- For the ascent game, frame time is bounded and accumulated into fixed 120 Hz rule ticks. Input edges are retained until a tick consumes them, preserving input sampled between ticks.
- For the survival game, `SurvivalRun` accumulates real frame time, unbounded, into fixed 100ms steps, as the [PM decision on #28](https://github.com/garamsh/neocity/issues/28#issuecomment-6553578708) requires, so time is neither lost nor doubled at any frame rate. The step clock runs in every phase: each step applies both actors' commands and calls `Exploration::advance` once with `Simulation`'s acid, and `Exploration` spends exploration time only while someone explores, so expiry leaves no remainder to carry and workshop steps spend nothing. A new run starts from an empty accumulator.
- A survival step reads a fresh view for each actor after applying the other's action. Edges are retained until a step consumes them. A direction press shorter than a step walks one cell; a key held across a step stops walking when released.
- When two humans finish the day in one step, only the first command runs; `SurvivalRun` compares the day before each day-finishing action.
- `SurvivalRun` ignores the last human's leave, because a party of AI alone has no key to take an actor back. When two humans leave in one step, the chef's leave applies and the appraiser's human stays.
- `SurvivalRun` calls `Escape::check` after each command, so a finished day rolls before the partner's command can set out. An escape ends the step and the run takes no further step.
- `SurvivalRun` calls `Exploration::damage` when an actor steps into a hazard cell from outside it, and records actions, hazard hits, falls, expiry, destruction, and exposure as events for the screen.
- `project/e2e/survival.gd` plays the game with real key events: two full days as the chef with AI, a third and fourth day ended unfed, a run as the appraiser, and the second key set. `project/tests/survival_timing.gd` injects action values to verify the fixed step at 480 and 5 frames per second, retained edges, expiry, the one-day guard, and the last-human guard. `project/e2e/escape.gd` plays from the main menu to the ending with real key events, once at 60 and once at 5 frames per second so that one frame spans two steps, and drives the cinematic contract below with no asset, a working stand-in scene, and broken ones; it installs each stand-in with `Resource.take_over_path`, so nothing is written to the art folders. `project/e2e/experiments.gd` drives both menus and an experiment with real Godot key events. `project/tests/session_timing.gd` injects action values at a faster frame cadence to verify partial-tick time disposal across a transition, accumulation, and retained input edges.

### Cinematic contract

The agreement between the host and the opening and ending art of [#29](https://github.com/garamsh/neocity/issues/29). The seam is `CinematicStage`, which `src/scene.cpp` provides to session.

- **Location:** `project/levels/monster_mouth_escape/cinematics/intro/intro.tscn` and `project/levels/monster_mouth_escape/cinematics/ending/ending.tscn`, each scene with its assets in its own folder. Neither exists yet.
- **Scene:** the root is a `CanvasItem`, such as a `Node2D` or a `Control`, because the host paints an opaque dark stage that would hide a 3D root. It declares a `finished` signal with no arguments and emits it once when playback is done; with arguments the host cannot receive it and stays on the scene. The host adds the root as a child of the game scene.
- **Request:** session plays the intro when a run starts and the ending when the run escapes.
- **End:** on `finished` the host closes the scene.
- **Skip:** the interact key of any human closes the scene at once, so a scene cannot rely on reaching its own end.
- **Input block:** while a cinematic is open the run takes no key and no time, and the skip does not reach it. M still leaves for the main menu and ends the run.
- **Return:** the intro hands over to day one at base. The ending closes the run and returns to the main menu.
- **Fallback:** a scene that is missing, that does not load as a scene, that does not instantiate, whose root is not a `CanvasItem`, or that has no `finished` signal opens a placeholder card instead, under the same skip, input block, and return. The host never aborts for a cinematic. A skip-free card stands 4 seconds for the intro and 6 for the ending; both lengths are temporary.

## Rationale

[ADR 0005](adr/0005-isolate-experiment-rules-behind-one-host.md) shares the window and menus while each experiment stays self-contained. Session orchestration leaves the `AscentScene` composition contract thin and avoids a generic plugin framework.

## Open questions

- Pausing a survival run: M ends it, because no save or pause exists yet.
