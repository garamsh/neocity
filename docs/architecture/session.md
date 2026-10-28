# Session

The common menu, active-run lifetime, and timing for the three selectable rule engines.

## Current decisions

- `src/session/` receives the independently constructed pan engines, ascent engine, camera, and legacy reset operation from `src/scene.cpp`. It selects and advances one engine at a time and delegates drawing to rendering and HUD.
- The initial screen is a keyboard menu: 1 selects direct dodge, 2 selects tilt escape, and 3 selects the preserved ascent reference. M returns from any run; R restarts pan experiments while retaining ascent's original retry semantics.
- Selection, menu return, and pan restart clear accumulated time, pending input edges, and run state. Controls must be released before the next run accepts input, so held movement, retry, or selection cannot leak across a transition.
- Frame time is bounded and accumulated into fixed 120 Hz rule ticks. Ascent edges are retained until a tick consumes them, preserving input sampled between ticks.
- `project/e2e/experiments.gd` drives the configured main scene with real Godot key events, including contact losses, complete escape routes, replay, and switching. The ascent harness selects its reference through the same menu before checking its original controls.

## Rationale

[ADR 0005](adr/0005-isolate-experiment-rules-behind-one-host.md) shares window and run selection while keeping rules independent. Session orchestration leaves the existing `AscentScene` composition contract thin and avoids a generic plugin or scenario-engine framework.

## Open questions

- Additional host behavior only when another approved experiment needs it.
