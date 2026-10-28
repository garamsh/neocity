# ADR 0002: Isolate the ascent engine from presentation

> Status: superseded by ADR-0003
> Date: 2026-10-27

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

The first playable ascent introduces both game rules and a Godot screen. The requester requires independently changeable engine and presentation responsibilities and package paths that expose ownership. ADR 0001 selected domain ownership but explicitly left engine independence open. The requester permits either domain-oriented or hexagonal organization and does not require a named architecture for its own sake.

## Decision

Keep ascent as one domain with two Cargo packages: a framework-independent engine and a Godot adapter consuming its public commands and value snapshots. Record the enforceable dependency and composition rules in `structure.md`. Use concrete command/snapshot boundaries without speculative port traits or a generic shared package.

## Consequences

- Engine tests run without Godot or a window. Sprite, input, camera, and canvas changes remain in the adapter.
- Changes to public command/snapshot meaning require coordinated updates; separation does not promise that incompatible contract changes leave consumers untouched.
- Godot supplies the existing host and rendering primitives through godot-rust GDExtension; the custom simulation and presentation orchestration are authored in Rust. This is not a standalone GPU renderer.
- A different host can consume the engine contract later without moving game rules into the current adapter.

## Signs the decision was wrong

- The engine begins importing Godot, image files, input codes, or canvas objects.
- Presentation implements landing, damage, or progression rules instead of consuming engine values.
- Routine sprite or camera edits require engine changes, or a common package becomes a mixed ownership container.
