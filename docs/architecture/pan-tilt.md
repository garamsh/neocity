# Pan tilt

The independent rules for the tilt-driven frying-pan experiment.

## Current decisions

- `src/pan_tilt/` builds the `pan_tilt` library and its co-located `pan_tilt_test` cases, independently of the application. Runtime dependencies are limited to the C++ standard library.
- `include/pan_tilt/` exposes tilt commands and snapshots containing body positions, velocities, tilt, and outcome. It imports no other experiment's types or Godot headers.
- Input changes the pan's tilt rather than directly moving the player. The player and both hazards receive the same acceleration and drag. Releasing input levels the pan gradually; velocity remains, and reverse tilt brakes it.
- The circular rim stops outward player motion and rebounds hazards. Contact loses; the top opening permits escape. Every body freezes after a terminal outcome, and reset clears tilt, velocity, positions, and outcome.
- Rules advance at 120 Hz. Geometry and physical tuning remain experiment choices, with no final game selection or playtested-balance claim.

## Rationale

[ADR 0005](adr/0005-isolate-experiment-rules-behind-one-host.md) gives tilt its own rule boundary. Its shared gravity, inertia, and moving hazards distinguish it from [direct movement](pan-dodge.md) without requiring another rendering engine.

## Open questions

- Whether inertia and moving hazards produce an experience worth developing.
- Tilt response, drag, rebound, and hazard tuning after human play feedback.
