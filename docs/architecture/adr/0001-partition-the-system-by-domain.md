# ADR 0001: Partition the system by domain

> Status: superseded by ADR-0004
> Date: 2026-10-27

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

The project will use Godot together with Rust, with custom engine and visualization work planned in Rust. Genre and gameplay remain undecided. Domain ownership is the requested bootstrap direction, while concrete subject boundaries cannot yet be inferred from implementation. The documentation needs to retain the ownership rules without creating game features that have not been chosen.

The reference project's [structure document](https://github.com/garamsh/murim-24/blob/22f03a29fd00ee9edcaf5cb74efc5e4d6956cb2f/docs/architecture/structure.md) chooses hexagonal partitioning and an undivided core. It is a precedent for recording rules and reasons, not a domain structure to copy. Layered partitioning would scatter one feature across technical folders; hexagonal partitioning would instead make ports the primary unit. Neither is the requested primary ownership model here. Domain partitioning is taken from the template's [domain candidate](https://github.com/garamsh/convention-driven-project/blob/ed6e790d86c2909ee390cca85eaed896de4182d1/docs/architecture/structures/domain.md).

## Decision

Use domain-oriented partitioning: each implemented subject area owns its files and declared public surface, including its Godot and Rust artifacts. Do not create speculative game domains; leave sibling-domain dependency policy, language-to-package mapping, composition details, and the Godot/Rust integration mechanism open in `structure.md` until implementation meets them.

## Consequences

- Changes are organized by subject rather than by a project-wide folder for each technical layer.
- Native engine work does not automatically earn a new game domain; its placement follows the responsibility it serves.
- The structure's rules and the template's composition rules live in `structure.md`, alongside unresolved choices.
- Choosing domain boundaries before the game is understood risks drawing them incorrectly; no domain folder or additional responsibility document is introduced by this bootstrap.
- Engine independence is not guaranteed by domain partitioning alone. A later decision must settle any required separation between game rules and Godot types.

## Signs the decision was wrong

- Most changes touch several domains at once, and the pair that always move together cannot be released independently.
- The same fix is applied once per domain, and a domain keeps being missed.
- The shared code every domain imports is growing faster than any domain.
- Which domain a new file belongs to cannot be settled without asking someone.
- A domain's public surface has grown until it names what the domain keeps inside.
