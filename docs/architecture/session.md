# Session

The main menu, the developer page, run lifetime, and fixed-step delivery to the survival and ascent games.

## Current decisions

- `src/session/` receives the ascent engine, its camera and fresh-run reset, a survival run factory, the experiment catalog, and a mount operation from `src/scene.cpp`. It decides which screen is open and delegates drawing to rendering and HUD.
- The start screen is the main menu: 1 opens the survival game's choice of players, 2 opens the developer page, and 3 plays the earlier ascent prototype. The choice of players takes 1 for the chef, 2 for the appraiser, and 3 for two local players. The developer page numbers the [registered experiments](experiments.md#registered-experiments). M goes back one level: from a game, the choice of players, or the developer page to the main menu, and from an experiment to the developer page.
- Every transition clears accumulated time, pending input edges, and the open game's run, and unmounts any experiment. Controls must be released before the next screen accepts input; an experiment's scene is mounted only then, so a held key cannot become its first action.
- An experiment, once mounted, reads its own input and steps its own rules; session only mounts and unmounts it.
- For the ascent game, frame time is bounded and accumulated into fixed 120 Hz rule ticks. Input edges are retained until a tick consumes them, preserving input sampled between ticks.
- For the survival game, `SurvivalRun` accumulates real frame time, unbounded, into fixed 100ms steps, as the [PM decision on #28](https://github.com/garamsh/neocity/issues/28#issuecomment-6553578708) requires, so time is neither lost nor doubled at any frame rate. The step clock runs in every phase: each step applies both actors' commands and calls `Exploration::advance` once with `Simulation`'s acid, and `Exploration` spends exploration time only while someone explores, so expiry leaves no remainder to carry and workshop steps spend nothing. A new run starts from an empty accumulator.
- A survival step reads a fresh view for each actor after applying the other's action. Edges are retained until a step consumes them. A direction press shorter than a step walks one cell; a key held across a step stops walking when released.
- When two humans finish the day in one step, only the first command runs; `SurvivalRun` compares the day before each day-finishing action.
- `SurvivalRun` calls `Exploration::damage` when an actor steps into a hazard cell from outside it, and records actions, hazard hits, falls, expiry, destruction, and exposure as events for the screen.
- `project/e2e/survival.gd` plays the game with real key events: two full days as the chef with AI, a third and fourth day ended unfed, a run as the appraiser, and the second key set. `project/tests/survival_timing.gd` injects action values to verify the fixed step at 480 and 5 frames per second, retained edges, expiry, and the one-day guard. `project/e2e/experiments.gd` drives both menus and an experiment with real Godot key events. `project/tests/session_timing.gd` injects action values at a faster frame cadence to verify partial-tick time disposal across a transition, accumulation, and retained input edges.

## Rationale

[ADR 0005](adr/0005-isolate-experiment-rules-behind-one-host.md) shares the window and menus while each experiment stays self-contained. Session orchestration leaves the `AscentScene` composition contract thin and avoids a generic plugin framework.

## Open questions

- Pausing a survival run: M ends it, because no save or pause exists yet.
