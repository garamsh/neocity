# ADR 0015: Stand lit shops on short supports under the teeth

> Status: accepted
> Date: 2026-11-04

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

After [ADR 0011](0011-open-the-city-to-a-bright-throat-over-glowing-acid.md) and [ADR 0012](0012-finish-the-city-in-a-pixel-grid-light-pass.md) the city screen still read far from the concept `search-mockup-v1.png` at commit `241124d5ba2dcef937c904623f1e5e3cb03fbcf5`, and [issue #153](https://github.com/garamsh/neocity/issues/153) names three gaps, with the rules and the cell layout unchanged:

- The concept's shops are warm rooms with a roof and walls standing on the walkways. The corner shop hung under the overpass's deck on a footing of its own with two pillars, and the shop, stall and vending machine pictures were dull props.
- Every walkway stood on supports down to the walkway below or into the acid, so the rooftop, the arcade, the overpass and the ledge, four to fourteen cells above the acid, drew dark pillars the whole height of the screen: the crowding ADR 0011 names as a sign it was wrong.
- The throat's teeth hang from the top of the screen behind the city, so whenever the camera is low on the map the walkways and buildings above the humans cover them, and the teeth were pulled toward the throat's magenta.

Alternatives considered:

- **Move the throat up and down with the camera.** The teeth would still sit behind the walkways above the humans, which are what covers them when the camera is low.
- **Thin the supports only, every one still reaching down.** Fewer full-height pillars still cross the open backdrop between the high walkways and the acid.
- **Draw the corner shop where its building is, under the overpass.** That is where the shop's cells are, but nothing stands under it, so it can only hang; the concept's shops stand on the floor the humans walk on.
- **Light the shops with halos on the glow layer.** The glow layer is drawn from the content alone and does not know which shops the acid has destroyed, and the finishing pass already spreads the light of bright art around it.

## Decision

Shops, stalls and vending machines are drawn from pictures of lit structures: a shop with a roof and brick walls whose open front glows amber, a stall under a tin roof with a lit counter, a vending machine lit from inside. A building with a shop inside stands on the walkway over its cells, as wide as the building, and its rubble lies there once it is destroyed. A support reaches down to the walkway below or into the acid only where that, the acid read at its surface at acid 0, is at most four and a half cells under the deck; a walkway higher than that stands on a short brace under each end, one cell long, on a girder along the walkway between them. The monster's upper teeth hang along the top of the screen in front of the city, under the interface, breathing with the throat and sliding sideways a quarter faster than the map, whatever the camera shows.

## Consequences

- The open backdrop runs from the high walkways down to the acid, and only the walkways near the acid or over another walkway stand on full supports.
- The teeth cover the top of the play area in front of the map. With the camera at the scene's top they hang above the map; with the camera lower they cover the top rows of what is drawn there, while the map's words, drawn on the main canvas above the city's, stay over them.
- A shop's frame and count stay at its source's cell, under the walkway its picture stands on, since the collection rule reads that cell.
- The shop, stall and vending machine pictures come from new generations through the same pipeline as the rest of the art set; the earlier shop and vending machine generations leave it.
- The capture checks measure, on the city frames that show them, that the shops' fronts read warm and bright, that the backdrop shows where the high walkways' supports used to reach down, and that the teeth show across the top of the play area.

## Signs the decision was wrong

- Players read a high walkway on braces as floating, and full supports have to come back under it, sparser.
- The teeth hide a walkway, a ladder or an actor players need above them, and the jaw has to rise or narrow when the camera is low.
- A shop standing over its cells reads as a place players can enter, and its stock has to move to the walkway the picture stands on, in the rules.
