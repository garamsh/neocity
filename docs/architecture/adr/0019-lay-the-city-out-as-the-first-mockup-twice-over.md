# ADR 0019: Lay the city out as the first mockup, twice over

> Status: accepted
> Date: 2026-11-04

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

The concept `search-mockup-v1.png` at commit `241124d5ba2dcef937c904623f1e5e3cb03fbcf5` shows one screen of the city, and [issue #188](https://github.com/garamsh/neocity/issues/188) asks that the city be laid out as it is, so that the screen a human first sees on setting out has its walkways, buildings, ladders and bridge where the mockup has them. From left to right the mockup shows a kitchen shop on stilts, with a ladder from the walkway to its loft and one from the loft to its roof; a long upper walkway that steps down to a lower one holding a vending machine and a bag; a rope bridge; and a pillar with an acid gauge and a ladder down into the acid, under a second shop; and the acid lake under all of it.

The 36 by 16 map before this was a broken road of other places: a dock, a main street, a ramp to an overpass, an arcade, a market and a plaza. None of them is in the mockup.

The camera follows a lone human at twice the scene's size, as [ADR 0009](0009-follow-the-humans-with-a-close-camera.md) decides, so one screen shows 20 columns of 32-pixel cells across 1280 pixels. Twenty columns of the mockup hold a vending machine, a bag, two shops and three ladders, while the rules ask for more: a ladder and a door that break and are each the only way to what lies behind them, a building that falls open, an NPC with a cart, a bridge that falls and a ferry that reopens the far side, acid-only ground that drowns low rows first, and three lanes of washed-up bags spread so a day is spent walking. The library integration tests in `content_test.cpp` check that an AI party feeds every day to day 30 on five seeds and that the first day ends by time while stock remains, and those hold only with that much on the map.

Alternatives considered:

- **One screen, 20 columns, only what the mockup shows.** It drops the breaking door, the falling bridge and ferry, the kiosk, the NPC and two of the three bag lanes, so the run's supply and its changes over thirty days go with them.
- **The mockup's screen at the start, the old road beyond it.** The rest of the city would again be places the mockup does not have, the drift the issue was filed against.
- **The mockup's screen scaled to a different cell size.** The cell size is the drawing's, under `src/rendering/`, which the issue keeps out of this change.

## Decision

The map is 40 by 16 cells: the mockup's screen laid out at columns 0 to 19, where the camera rests when a human first sets out, and the same structure again at columns 20 to 39, its kitchen shop, walkways, bridge and ledge in the same rows, carrying the rest of the run's places. The rows below the walkways hold only the shallows and the piers at the foot of the two ladders down from the ledges, and the sunken train and the drifting ferry between them, where the mockup shows the acid lake.

## Consequences

- The places of the first screen are named for what the mockup shows there: the upper and lower walkways, the kitchen loft, roof and shop, the rope bridge, the gauge ledge and ladder, the tower shop and its roof. The east half names its twins after them, and the objects the run needs that the mockup does not show, the kiosk wreck, the noodle vendor, the cold store, the sunken train, the ferry and the pier, stand in that half. Names of places the new layout does not have, the dock, the main street, the market, the arcade and the plaza among them, leave the map.
- A walk east crosses the same structure twice, so a day's errands climb and descend the same kinds of ladder in both halves.
- The shop, vending machine, bag and walkway pictures, and the camera that keeps a human's feet a little lower on the screen than the mockup draws them, are presentation's, and are matched to the mockup in their own changes.
- Every place in the run, the breaking passages and the ferry included, stands in a half of the mockup's structure, so a further place needs a third half or a change to this decision.

## Signs the decision was wrong

- Players lose their way between the two halves because they look alike, and the east half needs landmarks of its own that the mockup does not show.
- The walk across two halves leaves too little of the day for the first day to reach a source of every kind, or the supply checks in `content_test.cpp` fail on a later balance change because the two halves crowd the lanes onto too few cells.
- The rows under the walkways, kept nearly empty for the acid lake, leave too little low ground for acid to take first, and acid stops mattering to where supply lands.
