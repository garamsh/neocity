# ADR 0005: Isolate experiment rules behind one host

> Status: accepted
> Date: 2026-10-28

## Context

[Issue #16](https://github.com/garamsh/neocity/issues/16), following [Discussion #15](https://github.com/garamsh/neocity/discussions/15), authorizes two basic frying-pan experiments in one executable: direct movement and tilt-driven motion affecting both player and hazards. Their rules must remain independent, and the existing ascent reference must remain runnable. ADR 0004's single gameplay boundary cannot describe these separate simulations.

## Decision

Extend the purpose-folder layout with independent `pan_dodge` and `pan_tilt` C++ rule libraries, each owning its commands, state, movement, contact, outcomes, reset, and standalone tests. Retain the existing Godot project and `AscentScene` composition root; inject the engines into a `session` host for selection, timing, input lifetime, and presentation dispatch.

This supersedes only ADR 0004's designation of `gameplay` as the single build-enforced rules boundary. Its purpose folders, Godot project layout, and composition-site choice remain in effect; [structure](../structure.md) records the consolidated rules.

## Consequences

- Each experiment can be built and verified without Godot, ascent, or the other experiment. Similar geometry value types remain local rather than creating a shared rules dependency.
- A single window, menu, input device, and canvas host serve the experiments and ascent. Session transitions discard old input and timing state.
- The root composition constructs implementations; session orchestrates them, input translates keys, and rendering and HUD draw their values.
- A shared configurable game engine, dynamic plugin loader, multiple Godot projects, and separate graphics backends are rejected: none is needed to compare these two control schemes.
- Final game selection, production artwork, and balance acceptance remain outside this implementation decision.

## Signs the decision was wrong

- One experiment repeatedly needs another experiment's internal state or rule changes to remain correct.
- The common host begins reimplementing collision, physics, or outcomes instead of delivering commands and drawing snapshots.
- A new approved experiment cannot use the common window without substantial unrelated changes to existing rules.
