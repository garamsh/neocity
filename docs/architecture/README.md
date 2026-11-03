# Architecture

Current architecture of the system and the decisions behind it. This file is the index; workers read it before touching code, the PM keeps it honest.

## Structure

- **Responsibility documents** (one `.md` per domain or concern, e.g. `memory.md`, `gateway.md`) — the single source of truth for how the system is shaped *now*. To know the current state, read only these.
- **`adr/`** — the record of individual decisions: direction taken, context, rejected alternatives.

## Rules

1. **Responsibility document skeleton**: current decisions (consolidated) / rationale summary with links to the relevant ADRs / open questions. Do not copy ADR content — synthesize the present state.
2. **ADRs are append-only.** Once merged, the body is frozen. Exceptions: updating the status field (`accepted` → `superseded by ADR-XXXX`), fixing typos or broken links, and appending a dated entry under `## Errata` when the decision stands but a fact supporting it was wrong — the erratum names what falsified it, and the original text stays intact. A changed decision means a new ADR that supersedes the old one — never an edit.
3. **Decisions land in pairs.** A PR that adds or supersedes an ADR must update the affected responsibility documents in the same PR. A PR with only one of the two is rejected — the final state must always live in the responsibility documents.
4. Keep this index current: every responsibility document and every ADR is listed here. An ADR covers a decision that changes a project rule — a settled choice that does not affect the rules does not earn one.
5. **Every domain or concern has a responsibility document.** The PR that introduces one adds its document; the PR that removes one deletes it. A system with no code yet has no more than `structure.md` (rule 6), and that is the correct state.
6. **Structural rules live in `structure.md`.** A shape adopted by the project imposes rules — what the unit is, what may reference what, when a new one is earned — and they belong in the responsibility document for that concern, where a reviewer cites one by line and a correction is an ordinary edit. Not the ADR: rule 2 freezes that body, so superseding the choice of shape would be the price of fixing a rule's wording.
7. **A point the structural rules leave to the project is settled in an ADR** — the one recording the shape where the question is met when that shape is adopted, a later one where it is met later. Which side is taken decides what counts as a defect, so it is a decision under rule 4, and the ADR still holds the record after bootstrap. Nothing obliges a project to settle a question it has not met: until then the point waits under `structure.md`'s open questions, where it binds nothing, and moves into that document's current decisions when the project takes a side.
8. **Structural rules yield only to what a framework imposes.** Where `structure.md` and a convention file decide the same case differently, `structure.md` governs, unless the stack-specific file that applies decides it as its framework imposes it — the alternative fails to build or run, so the project cannot restate it. A stack file's spelling of a structural rule is not that: the rule it spells is `structure.md`'s. The shape adopted by the project is this project's own refinement of what a stack-neutral file states for every project. A project with no `structure.md` meets no such conflict, and this rule asks nothing of it.

## Index

The C++ sources are partitioned into purpose folders under `src/`, and the Godot project lives in `project/`. The `survival` library holds the rules of Monster Mouth Escape, the one game, with no Godot dependency. The C++/Godot host opens on the game's title screen, which starts the game, shows the controls, or quits.

- [`structure.md`](structure.md) — purpose folders, independent rule boundaries, composition rules, repository layout, and unresolved structural choices.
- [`session.md`](session.md) — title screen, controls page, run lifetime, input isolation, and fixed-step delivery to the survival game.
- [`adr/0012-finish-the-city-in-a-pixel-grid-light-pass.md`](adr/0012-finish-the-city-in-a-pixel-grid-light-pass.md) — one full-screen shader pass over the drawn city, under the interface, that moves the acid and spreads light in dithered steps on the pixels of the art beneath, with the throat, flesh, steam and spores moving on the real clock; the engine's glow and 2D HDR rejected as smooth.
- [`adr/0011-open-the-city-to-a-bright-throat-over-glowing-acid.md`](adr/0011-open-the-city-to-a-bright-throat-over-glowing-acid.md) — an open city screen: no walls under the walkways, dark walkways on supports with lit edges, a bright throat without haze raised over the distant city, and the acid band glowing along the bottom of the play area at every acid level; supersedes ADR 0009's haze.
- [`adr/0010-frame-the-hud-in-the-corners-like-the-concept.md`](adr/0010-frame-the-hud-in-the-corners-like-the-concept.md) — an interface of dark pixel panels with a thick light rim in a pixel font, kept to the screen's corners, with a key cap over a human who can act, one goal line, notes that fade, one task card at base, and every key and recipe on the pause screen; replaces the flat interface of #107.
- [`adr/0009-follow-the-humans-with-a-close-camera.md`](adr/0009-follow-the-humans-with-a-close-camera.md) — a camera that follows the humans in the city at twice the scene's size, frames two together, ignores an AI partner and stays inside the map, with words and goal pointers on the screen and a layered, glowing backdrop; supersedes ADR 0007's one screen; its haze superseded by ADR 0011.
- [`adr/0008-hold-the-collecting-actor-instead-of-the-shared-clock.md`](adr/0008-hold-the-collecting-actor-instead-of-the-shared-clock.md) — a collection holds the actor who makes it while the shared clock runs on, and the supply lands as small bags across the map nearly every day.
- [`adr/0007-move-actors-through-a-side-view-with-gravity.md`](adr/0007-move-actors-through-a-side-view-with-gravity.md) — side-view exploration: fixed-point sub-cell positions on a fixed physics step, gravity, a capped jump, ladders, stairs and ramps, falls into the acid, and one screen; replaces the temporary four-direction grid; its one screen superseded by ADR 0009.
- [`adr/0006-ship-one-game-behind-a-title-screen.md`](adr/0006-ship-one-game-behind-a-title-screen.md) — one game behind a title screen, with the developer page, the experiments, and the ascent prototype removed; supersedes ADR 0005 and ADR 0004's `gameplay` and `camera` folders, ascent level scene, and `AscentScene` root.
- [`adr/0005-isolate-experiment-rules-behind-one-host.md`](adr/0005-isolate-experiment-rules-behind-one-host.md) — one self-contained folder per experiment behind a developer page; supersedes ADR 0004's single rules boundary; superseded by ADR 0006.
- [`survival.md`](survival.md) — independent progression rules of the monster-mouth escape game, shared state, the per-feature file ownership plan, and open questions.
- [`presentation.md`](presentation.md) — the GDExtension's entry point, composition site, input, rendering, HUD, and menus.
- [`adr/0004-adopt-the-godot-cpp-template-layout.md`](adr/0004-adopt-the-godot-cpp-template-layout.md) — godot-cpp template layout with `project/` and purpose folders under `src/`; supersedes ADR 0001 and ADR 0003's partitioning and target names; its single rules boundary superseded by ADR 0005, its `gameplay` and `camera` folders, ascent level scene, and `AscentScene` root by ADR 0006.
- [`adr/0003-implement-the-ascent-domain-in-cpp.md`](adr/0003-implement-the-ascent-domain-in-cpp.md) — C++ rules library and godot-cpp adapter, with each toolchain selection and its source; partitioning and target names superseded by ADR 0004.
- [`adr/0002-isolate-the-ascent-engine-from-presentation.md`](adr/0002-isolate-the-ascent-engine-from-presentation.md) — engine/presentation dependency boundary; superseded by ADR 0003.
- [`godot-runtime.md`](godot-runtime.md) — minimal Godot host, resource ownership, and import/startup checks.
- [`adr/0001-partition-the-system-by-domain.md`](adr/0001-partition-the-system-by-domain.md) — domain-oriented partitioning; superseded by ADR 0004.
- [`adr/0000-template.md`](adr/0000-template.md) — template for future architecture decisions.
