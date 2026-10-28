# ADR 0005: Isolate experiment rules behind one host

> Status: accepted
> Date: 2026-10-28

## Context

[Discussion #15](https://github.com/garamsh/neocity/discussions/15) chose to build several design-free test games and decide by playing them, all inside one game. [Issue #16](https://github.com/garamsh/neocity/issues/16) first added two frying-pan experiments behind a common menu. The requester's review of [PR #17](https://github.com/garamsh/neocity/pull/17#issuecomment-6463142257) then asked for a developer page reached from the main screen, one folder per experiment under `src/experiment/`, documentation and a skill an AI can follow to build a new experiment, and — [in a second comment](https://github.com/garamsh/neocity/pull/17#issuecomment-6463182460) — a 3D dodge pan that moves by itself and spawns and removes hazards, with the tilt experiment removed. ADR 0004 places one shallow purpose folder per runtime purpose and names `gameplay` the single rules boundary; neither describes contributor-owned experiments.

## Decision

Group experiments under one `experiment` purpose: each experiment owns `src/experiment/<id>/`, holding an independent C++ rules library with its own build and tests, its Godot node, and its record, plus a directly runnable scene in `project/experiment/<id>/`. A compile-time catalog in `src/experiment/` registers each scene; the host's main menu offers the main game and a developer page that lists the catalog and mounts the chosen scene. The first experiment is the 3D `pan_dodge`.

This supersedes ADR 0004's designation of `gameplay` as the single build-enforced rules boundary and its one-level placement for experiment code. Its purpose folders for the main game and the host, the Godot project layout, and the `AscentScene` composition root remain; [structure](../structure.md) consolidates the rules.

## Consequences

- A contributor adds an experiment by creating one source folder and one scene folder, plus one entry in each of a fixed list of shared registration files that [experiments](../experiments.md#registration-and-launch) keeps; no other experiment or main-game rule changes.
- The developer page and direct launch play the same scene, so an experiment cannot behave differently between the two entries.
- An experiment's Godot node draws itself in 2D or 3D with Godot primitives and reads its own input; the host only mounts and unmounts it. The host keeps no per-experiment commands, so it cannot enforce fixed-tick delivery for experiments — each node steps its rules from Godot's physics tick.
- A shared configurable game engine, runtime discovery of experiment folders, a plugin loader, separate Godot projects, and per-experiment graphics backends are rejected: none is needed to try ideas side by side.
- Final game selection, production art, and balance remain outside this decision; an experiment is promoted into the main game only by a separate decision.

## Signs the decision was wrong

- Adding an experiment repeatedly requires changing another experiment, the main game, or host code outside the registration list.
- Experiments begin sharing rule code through the host or through each other.
- Contributors cannot add an experiment from the documents and skill alone, without asking how.
