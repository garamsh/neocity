# Session

The main menu, the developer page, run lifetime, and fixed-step delivery to the main game.

## Current decisions

- `src/session/` receives the ascent engine, its camera and fresh-run reset, the experiment catalog, and a mount operation from `src/scene.cpp`. It decides which screen is open and delegates drawing to rendering and HUD.
- The start screen is the main menu: 1 plays the main game, 2 opens the developer page. The developer page numbers the [registered experiments](experiments.md#registered-experiments). M goes back one level: from the main game or the developer page to the main menu, and from an experiment to the developer page.
- Every transition clears accumulated time, pending input edges, and the main game's run, and unmounts any experiment. Controls must be released before the next screen accepts input; an experiment's scene is mounted only then, so a held key cannot become its first action.
- An experiment, once mounted, reads its own input and steps its own rules; session only mounts and unmounts it.
- For the main game, frame time is bounded and accumulated into fixed 120 Hz rule ticks. Input edges are retained until a tick consumes them, preserving input sampled between ticks.
- `project/e2e/experiments.gd` drives both menus and an experiment with real Godot key events. `project/tests/session_timing.gd` injects action values at a faster frame cadence to verify partial-tick time disposal across a transition, accumulation, and retained input edges.

## Rationale

[ADR 0005](adr/0005-isolate-experiment-rules-behind-one-host.md) shares the window and menus while each experiment stays self-contained. Session orchestration leaves the `AscentScene` composition contract thin and avoids a generic plugin framework.

## Open questions

- A main-game selection beyond the ascent course, once a direction is chosen.
