# Pan dodge

The independent rules for the direct-movement frying-pan experiment.

## Current decisions

- `src/pan_dodge/` builds the `pan_dodge` library and its co-located `pan_dodge_test` cases. It configures independently of the application and depends only on the C++ standard library at runtime.
- `include/pan_dodge/` exposes direction commands and value snapshots. No Godot, ascent, or tilt types enter this boundary.
- A blue circular player moves directly in a circular pan, stops on release, and cannot move faster diagonally. Three fixed circular hazards end the run on contact; the marked opening at the top is the only escape.
- Rules advance in fixed 120 Hz ticks. Terminal outcomes freeze movement; reset restores the initial positions and playing outcome.
- Geometry and tuning are reversible experiment choices, not a selected final game or a balance claim.

## Rationale

[ADR 0005](adr/0005-isolate-experiment-rules-behind-one-host.md) keeps each experiment's commands, state, movement, contacts, and outcomes independent while sharing a host. Direct control and static hazards provide the comparison against [pan tilt](pan-tilt.md).

## Open questions

- Whether direct movement is worth developing beyond this experiment.
- Final geometry, hazard patterns, and tuning after human play feedback.
