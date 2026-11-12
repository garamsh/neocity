# ADR 0018: Cut the backdrop from the search mockup

> Status: accepted; the cut at the mockup's own size superseded by ADR-0020; city art superseded by ADR 0025
> Date: 2026-11-04

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

[ADR 0017](0017-draw-every-picture-one-pixel-to-a-screen-pixel-at-the-close-camera.md) sets the density of the concept `search-mockup-v1.png` at commit `241124d5ba2dcef937c904623f1e5e3cb03fbcf5`, 1672×941, for every picture. The first stage of [issue #186](https://github.com/garamsh/neocity/issues/186) reached it by redrawing the throat, the city, the acid and the teeth from their earlier generations with Recraft and reducing the paintings to the master palette. On review of [PR #187](https://github.com/garamsh/neocity/pull/187), on 2026-11-04, the user rejected those redraws: the game is to show the mockup as it is drawn, not a new picture in its style. The test is that the city screen, set beside the mockup, looks the same.

The mockup is one flat picture. Its throat, its city and its acid lake stand behind walkways, shops, a vending machine, two characters, a lamp and the interface, all of which the game draws itself. It also draws the monster's fangs in the throat, a few large cream fangs hanging over its middle and right, while [ADR 0015](0015-stand-lit-shops-on-short-supports-under-the-teeth.md) hangs a row of teeth from a separate picture along the whole top of the screen, cut fang by fang and varied so the row never repeats, and [ADR 0011](0011-open-the-city-to-a-bright-throat-over-glowing-acid.md) pulls the throat's near-white parts toward its magenta, which turns the mockup's cream fangs pink.

Alternatives considered:

- **Redraw each layer in the mockup's style from earlier generations**, as the first stage did. The user rejected the result: a new painting reduced to 32 colours is not the mockup.
- **Draw the whole mockup as one flat picture behind the map.** Nothing would slide with the camera, and the mockup's own walkways, shops and characters would stand behind the game's.
- **Reduce the cut layers to the master palette, as the rest of the set is.** The mockup is painted in hundreds of thousands of colours; 32 of them change how it reads, which is what the test forbids.
- **Fill what the map and the interface cover with new art over the whole mockup's width.** Recraft's erase filled the mockup's left and right shop buildings with flat purple fog, which shows above the city. Cut narrower, each layer meets its mirrored copy at its edges instead, as the earlier layers did.
- **Keep the row of teeth in front of the throat's own fangs.** The screen then shows two sets of teeth, one of them not the mockup's.

## Decision

The backdrop's three layers are cut from the search mockup with its own pixels, neither redrawn nor reduced to the palette: the throat with its fleshy palate, its fangs and its uvula; the swallowed city of towers and the elevated railway; and the acid lake with its sunken trains. Where the mockup shows the map, its characters or the interface over a layer, those pixels are replaced by Recraft's erase of that region, which fills it with what lies around it, and a layer's pixels above the city's skyline or the acid's surface are clear. Each layer is cut narrower than the mockup where it would otherwise hold its walkways and shops, and is drawn beside mirrored copies of itself. The layers stand so that the first city screen shows each where the mockup does. The throat's fangs are the mockup's, in their own perspective: they hang from the top of the screen with the throat and move as it does, and no row of teeth is drawn along the top of the screen. The throat is drawn in its own colours, its fangs cream.

This supersedes ADR 0015's teeth hung along the top of the screen whatever the camera shows, in front of the backdrop, and ADR 0011's pulling of the throat's near-white parts toward its magenta. ADR 0017's density is unchanged: the mockup's density is that rule.

## Consequences

- `backgrounds/jaw.png` and the drawing of the teeth fang by fang go, with the capture's checks that the fangs vary and hang in one row. The capture now checks that the throat's own fangs show over the top of the play area.
- The colour limit and the lone-pixel rule of the art folder's clean-art rules do not apply to `backgrounds/`, whose layers keep the mockup's colours. The density rule does.
- `tools/art/pixelize.py` reads the mockup from the repository's history and cuts each layer from it with the fills and the clear regions `tools/art/assets.json` lists, so the layers are regenerated, not edited.
- The layers align with the mockup only on the first city screen. Wherever else the camera goes, each slides by its own share of the camera's movement, as before.
- The mockup's backdrop is darker than the redrawn one: its walkways read 0.10 to 0.13 in luminance under a backdrop of 0.21 to 0.24. Until the walkways are drawn from it too, the game's walkways read as little as 0.048 under the new backdrop, and the capture's value check holds them to 0.03.
- What Recraft's erase filled shows wherever the map leaves the backdrop open: under the city's railway and in the acid lake's lower rows, as painted flesh and acid with less detail than the mockup around them.

## Signs the decision was wrong

- Set beside the mockup, the city screen does not read as the same picture, and the layers need more of the mockup, or the map more of its layout, to match it.
- The erased regions read as smears where the map leaves them open, and they need to be drawn by hand.
- The mirrored copies at a layer's edges read as a seam or a repeat where the camera shows them.
- A layer's fangs or uvula appear twice on one screen.
