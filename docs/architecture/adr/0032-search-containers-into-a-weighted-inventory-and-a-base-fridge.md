# ADR 0032: Search containers into a weighted inventory and a base fridge

> Status: accepted
> Date: 2026-11-16

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

[Issue #227](https://github.com/garamsh/neocity/issues/227), a part of goal #218, replaces collecting one ingredient at a time from a source with stock by searching a container. Its numbers are the PM's temporary values: an actor has 12 inventory slots and carries at most 20 weight; each search container holds 2 to 6 slots; the base fridge has 30 slots, no weight limit and no spoilage; a search screen shows the container beside the actor's own slots, with transfer between them; the bottom bar of seven bag slots is removed and the inventory is seen on a screen of its own.

[ADR 0008](0008-hold-the-collecting-actor-instead-of-the-shared-clock.md) held an actor for 1.5 seconds per collection. [ADR 0022](0022-cut-the-interface-frames-and-bag-icons-from-the-mockup.md) set seven bag slots at the bottom of the screen. A search that is open until the actor closes it, with the container's slots visible, replaces the first; a screen of the inventory replaces the second.

Alternatives considered:

- Keep the timed hold and add slots behind it. Rejected: the hold existed to price one collection, and a transfer screen has no single collection to price.
- Let two actors open one container at once. Rejected: the issue requires that two actors cannot empty the same container at once, and a shared open container would need a merge rule for two cursors.
- Weigh dishes as well as ingredients. Rejected for now: a dish is not carried, since stored food stays at base, so the weight of 2 given for a dish is not applied anywhere.
- Keep the `Npc` and `Shop` source kinds. Rejected: every source is now a container, so the kinds name how it is drawn, not what it is.

## Decision

A source of supply becomes a search container with a number of slots and the ingredients in them, shared by both actors for the whole run and kept as map state. An actor searches the container in reach, which holds the actor in place with the shared clock running until the search is closed, and moves one ingredient at a time between the container and the inventory; only one actor has a container open. An actor has `inventory_slots` slots and, over `max_weight`, walks and runs at `overweight_percent` of its speed until it drops enough to come under. At base the same screen moves ingredients between the inventory and the fridge, and appraisal and cooking draw on both inventories and the fridge together.

The `Npc` and `Shop` source kinds are replaced by `SourceKind::Crates`, `Locker`, `Shelf` and `Bag`, which are presentation categories only. Vending machines are `Locker`, the vendor and keeper carts `Crates`, the shop shelves, the cold locker and the tank locker `Shelf`, and the bags and the dock crate `Bag`. The rendering keeps the same art for each.

The AI companion searches the container in reach, takes the first ingredient until its slots or weight are full, closes the search, and at base stores what it carries in the fridge. This is a temporary policy and is recorded in [survival](../survival.md).

## Consequences

Easier: a container's contents are visible and persist, so a partner who took the best items is a thing the player sees; carrying has a cost, so a trip has a limit besides the clock; stored food and spare ingredients keep across days in the fridge.

Harder: a human spends time on a screen while the clock runs, and the AI's greedy take can empty a container before a human reaches it. The 1.5-second collection time and the seven-slot bag no longer exist, and with them the bag's icons in the corner interface that ADR 0022 cut.

Ruled out: spoilage, a weight on the fridge, and weight on dishes, until a later decision takes them up.

## Signs the decision was wrong

Players empty every container on a day and carry nothing back for want of a reason to leave items, so the weight limit changes no route. The AI's first-item take starves a human partner on most days. The transfer screen is closed more often than it is used, because the shared clock makes a search too expensive to open. A third kind of carrying, such as dishes, needs weight and the numbers here have to be redone for it.
