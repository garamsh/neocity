# ADR 0015: Stand lit shops on short supports under the teeth

> Status: accepted
> Date: 2026-11-04

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

After [ADR 0011](0011-open-the-city-to-a-bright-throat-over-glowing-acid.md) and [ADR 0012](0012-finish-the-city-in-a-pixel-grid-light-pass.md) the city screen still read far from the concept `search-mockup-v1.png` at commit `241124d5ba2dcef937c904623f1e5e3cb03fbcf5`, and [issue #153](https://github.com/garamsh/neocity/issues/153) names three gaps, with the rules and the cell layout unchanged:

- The concept's shops are warm rooms with a roof and walls standing on the walkways. The corner shop hung under the overpass's deck on a footing of its own with two pillars down into the acid, and the shop, stall and vending machine pictures were dull props.
- Every walkway stood on supports under its ends and every fourth column between, down to the walkway below or into the acid, so the rooftop, the arcade, the overpass and the ledge, four to fourteen cells above the acid, drew dark pillars the whole height of the screen: the crowding ADR 0011 names as a sign it was wrong.
- The throat's teeth hang from the top of the screen at the back of the backdrop, pulled toward the throat's magenta, so whenever the camera is low on the map the scenery, the supports and the walkways above the humans cover them.

Alternatives considered:

- **Move the throat up and down with the camera.** The teeth would still sit behind everything above the humans, which is what covers them when the camera is low.
- **Hang the teeth in front of the whole city.** They then cover the walkways at the top of the screen: in the capture's `24-city-on-ladder`, with the chef on the ladder to the rooftop, the rooftop's lit edge read 0.098 under the teeth, darker than the scenery above it, so the ground-contrast check failed.
- **Thin the supports only, every one still reaching down.** Fewer full-height pillars still cross the open backdrop between the high walkways and the acid. The supports between a run's ends do go, since a run stands on its ends in the concept.
- **Stand the corner shop on the overpass, over its building's cells.** The neon ledge runs a cell above the overpass there, so a shop as tall as the concept's would stand through the ledge's deck; and the shop's frame and count, at its source's cell under the overpass, would sit under it instead of in it.
- **Light the shops with halos on the glow layer.** The glow layer is drawn from the content alone and does not know which shops the acid has destroyed, and the finishing pass already spreads the light of bright art around it.

## Decision

Shops, stalls and vending machines are drawn from pictures of lit structures: a shop with a roof and brick walls whose open front glows amber, a stall under a tin roof with a lit counter, a vending machine lit from inside. A shop inside a building under a walkway is such a room in the building's place, its roof under that walkway's deck and its floor on its own footing of deck, which stands on supports as a walkway does. A walkway stands on a support under each end only, none between, and a support reaches down to the walkway below or into the acid only where that, the acid read at its surface at acid 0, is at most four and a half cells under the deck; a walkway higher than that stands on a short brace under each end, one cell long, on a girder along the walkway between them. The monster's upper teeth, cream as in the concept, hang along the top of the screen whatever the camera shows, breathing with the throat and sliding sideways a quarter faster than the map: in front of the backdrop, the acid, the scenery behind the map and the supports, and behind the buildings, the shops, the walkways and everything on them.

## Consequences

- The open backdrop runs from the high walkways down to the acid, and only the walkways near the acid or over another walkway stand on full supports.
- The teeth show along the top of the screen at every camera position, between the walkways and buildings there, and never hide a walkway, a ladder, a source or an actor.
- A shop's frame and count stay at its source's cell, inside the room under the walkway, since the collection rule reads that cell.
- The shop, stall and vending machine pictures come from new generations through the same pipeline as the rest of the art set; the earlier shop and vending machine generations leave it.
- The capture checks measure, on the city frames that show them, that the shops' fronts read warm and bright, that the backdrop shows where the high walkways' supports used to reach down, and that the teeth show across the top of the play area.

## Signs the decision was wrong

- Players read a high walkway on braces as floating, and full supports have to come back under it, sparser.
- With the camera low, the walkways and buildings at the top of the screen leave too little of the teeth showing for players to read the mouth, and the teeth have to come in front of more of the city.
- A shop room under a walkway reads as a place players can walk into, and the rules have to give its floor ground or its stock a place on the walkway above.
