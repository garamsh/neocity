# Structure

The domain-oriented shape selected for the system and the boundaries it imposes on future code.

## Current decisions

The system uses domain-oriented partitioning. The first implemented domain is `ascent`; introduce additional domains only with behavior that establishes their subject matter. Godot and C++ are implementation technologies within this shape, not the names of game domains.

- **A domain is the unit.** One folder per domain, named for its subject matter and never for a technical role.
- **A domain is reached only through what it declares public.** Everything else it holds is internal, and reaching past that surface is a defect even where the language permits it.
- **Whether a domain may depend on a sibling at all is contested.** *Domain-Driven Design* (Evans, 2003) gives no single answer: its relations between contexts run from Shared Kernel, where two share code, to Separate Ways, where neither may reference the other. A project takes one, records which, and a reference the recorded choice does not permit is a defect.
- **Shared code sits below every domain, never beside one.** What two domains both need is extracted below them, or owned by one and reached through its declared surface; it never becomes a sibling folder no domain owns.
- **A cycle between domains means the split is wrong**, and so does a file that belongs in two of them.
- **A new domain is earned** by subject matter with its own lifecycle and vocabulary, which something outside it depends on, and whose public surface can stay stable while its inside changes.

- **The application has one composition site.** It is the one place that constructs concrete implementations, and it decides which implementation each declared abstraction receives.
- **Every other unit receives what it depends on and constructs none of it**, whatever the shape makes the unit — a domain, a layer, or an adapter.
- **The composition site stays thin.** It constructs and passes; it decides nothing else.

## Engine and presentation boundary

- `ascent/engine` owns game rules and its public commands/value snapshots, declared in `ascent/engine/include/`. It has no Godot, graphics, file, clock, or platform-input dependency; its build target has no Godot or godot-cpp include directory, so an engine source that includes one fails to compile.
- `ascent/godot` owns the host adapter, resource loading, input translation, camera, and drawing. It may depend on the engine's public API; the engine may not depend on this adapter. Renderer-specific values do not cross inward.
- Files are grouped by their actual responsibility; do not introduce generic common/shared containers. A change within one package (`ascent/engine`, `ascent/godot`) must not reach into the other's private sources under `src/`.
- `ascent/godot/src/scene.cpp` is the application composition site. Frame/input/drawing adapters consume the engine; they do not reimplement collision or progression. Public contract changes are coordinated explicitly.

## Rationale

[ADR 0001](adr/0001-partition-the-system-by-domain.md) records domain ownership across the Godot host and the custom implementation language. Keeping behavior and its presentation together by subject gives future changes an owner without imposing a technical layer for every language. [ADR 0002](adr/0002-isolate-the-ascent-engine-from-presentation.md) added the explicit engine/presentation boundary within the first domain; [ADR 0003](adr/0003-implement-the-ascent-domain-in-cpp.md) keeps that boundary and implements both sides in C++.

## Open questions

- Which additional game domains future behavior establishes; ascent is the first.
- Whether and how a domain may depend on a sibling. Record a policy in an ADR when the first cross-domain dependency is proposed; until then no sibling-dependency variant is selected here.
- How additional domains map onto packages and resources; ascent is mapped in its responsibility document.
- Whether a future host requires a different composition adapter; the current site is the C++ AscentScene loaded by Godot.
- Whether production needs a graphics backend beyond Godot canvas; the current integration is a godot-cpp GDExtension.
