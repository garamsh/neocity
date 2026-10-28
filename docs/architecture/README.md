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

The C++ sources are partitioned into purpose folders under `src/`, and the Godot project lives in `project/`. The `gameplay` library holds the main game's rules, and each experiment under `src/experiment/<id>/` holds its own rules library with no Godot or mutual dependencies. The shared C++/Godot host opens on a main menu that plays the main game or opens a developer page listing the experiments, each a self-contained scene.

- [`structure.md`](structure.md) — purpose folders, independent rule boundaries, composition rules, repository layout, and unresolved structural choices.
- [`experiments.md`](experiments.md) — what an experiment is made of, registration, launch, verification, and promotion; each experiment's rules live in its own record `src/experiment/<id>/README.md`.
- [`session.md`](session.md) — main menu, developer page, run lifetime, input isolation, and fixed-step delivery.
- [`adr/0005-isolate-experiment-rules-behind-one-host.md`](adr/0005-isolate-experiment-rules-behind-one-host.md) — one self-contained folder per experiment behind a developer page; supersedes ADR 0004's single rules boundary.
- [`gameplay.md`](gameplay.md) — game rules of the ascent level, their public surface, and prototype control limitations.
- [`presentation.md`](presentation.md) — the GDExtension's entry point, composition site, input, camera, rendering, HUD, and experiment hosting.
- [`adr/0004-adopt-the-godot-cpp-template-layout.md`](adr/0004-adopt-the-godot-cpp-template-layout.md) — godot-cpp template layout with `project/` and purpose folders under `src/`; supersedes ADR 0001 and ADR 0003's partitioning and target names; its single rules boundary superseded by ADR 0005.
- [`adr/0003-implement-the-ascent-domain-in-cpp.md`](adr/0003-implement-the-ascent-domain-in-cpp.md) — C++ rules library and godot-cpp adapter, with each toolchain selection and its source; partitioning and target names superseded by ADR 0004.
- [`adr/0002-isolate-the-ascent-engine-from-presentation.md`](adr/0002-isolate-the-ascent-engine-from-presentation.md) — engine/presentation dependency boundary; superseded by ADR 0003.
- [`godot-runtime.md`](godot-runtime.md) — minimal Godot host, resource ownership, and import/startup checks.
- [`adr/0001-partition-the-system-by-domain.md`](adr/0001-partition-the-system-by-domain.md) — domain-oriented partitioning; superseded by ADR 0004.
- [`adr/0000-template.md`](adr/0000-template.md) — template for future architecture decisions.
