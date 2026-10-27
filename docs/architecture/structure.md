# Structure

The domain-oriented shape selected for the system and the boundaries it imposes on future code.

## Current decisions

The system uses domain-oriented partitioning. No game domains or application folders exist yet; introduce a domain with the behavior that establishes its subject matter. Godot and Rust are implementation technologies within this shape, not the names of game domains.

- **A domain is the unit.** One folder per domain, named for its subject matter and never for a technical role.
- **A domain is reached only through what it declares public.** Everything else it holds is internal, and reaching past that surface is a defect even where the language permits it.
- **Whether a domain may depend on a sibling at all is contested.** *Domain-Driven Design* (Evans, 2003) gives no single answer: its relations between contexts run from Shared Kernel, where two share code, to Separate Ways, where neither may reference the other. A project takes one, records which, and a reference the recorded choice does not permit is a defect.
- **Shared code sits below every domain, never beside one.** What two domains both need is extracted below them, or owned by one and reached through its declared surface; it never becomes a sibling folder no domain owns.
- **A cycle between domains means the split is wrong**, and so does a file that belongs in two of them.
- **A new domain is earned** by subject matter with its own lifecycle and vocabulary, which something outside it depends on, and whose public surface can stay stable while its inside changes.

- **The application has one composition site.** It is the one place that constructs concrete implementations, and it decides which implementation each declared abstraction receives.
- **Every other unit receives what it depends on and constructs none of it**, whatever the shape makes the unit — a domain, a layer, or an adapter.
- **The composition site stays thin.** It constructs and passes; it decides nothing else.

## Rationale

[ADR 0001](adr/0001-partition-the-system-by-domain.md) records domain ownership across the planned Godot and Rust stack. Keeping behavior and its presentation together by subject gives future changes an owner without imposing a technical layer for every language.

## Open questions

- Which game domains the first implemented behavior establishes.
- Whether and how a domain may depend on a sibling. Record a policy in an ADR when the first cross-domain dependency is proposed; until then no sibling-dependency variant is selected here.
- How domain ownership maps onto Cargo packages and Godot resource paths.
- Where the composition site lives, and how Godot instantiates scenes within it.
- How Godot and Rust integrate, and which engine and visualization responsibilities each owns.
