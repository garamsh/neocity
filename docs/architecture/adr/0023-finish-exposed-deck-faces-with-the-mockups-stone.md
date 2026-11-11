# ADR 0023: Finish exposed deck faces with the mockup's stone

> Status: accepted
> Date: 2026-11-06

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

[ADR 0020](0020-cut-the-city-from-the-mockup-at-the-screens-size.md) keeps each screen-sized mockup band at its map column and permits only a warm top-edge treatment. The user's [finishing feedback in #198](https://github.com/garamsh/neocity/issues/198) identifies the rectangular exposed deck ends as unfinished. Cutting a continuous band at a cell boundary exposes a straight vertical face and bottom corner that the original illustration did not draw as an end.

The [prototype review](https://github.com/garamsh/neocity/issues/198#issuecomment-6681971643) accepts recessing the lower face while retaining the full walked-on edge. The [scope decision](https://github.com/garamsh/neocity/issues/198#issuecomment-6686160610) asks for that exception to ADR 0020 to be recorded, and the [finishing review](https://github.com/garamsh/neocity/issues/198#issuecomment-6686374184) adopts the refined contour and loft underside. The existing stairs' mismatch with the simulation's flat cell floors belongs to [#200](https://github.com/garamsh/neocity/issues/200), separately from this local finishing work.

Alternatives considered:

- Keep each rectangular cut: the exposed vertical ends and lower corners remain the reported defect.
- Replace every band with end tiles: this replaces the mockup's columns and introduces another asset set for the same stone.
- Recess the walked-on edge or change the map: this changes the apparent landing boundary or gameplay instead of finishing the face below it.

## Decision

Keep ADR 0020's sources, screen density, band placement and full walked-on edge, but recess exposed deck faces and their loft underside in broken stone courses, with a narrow shaded rim taken from the same texture. Current standing neighbours determine which ends are exposed; level interiors and the existing stair shape remain unchanged.

This supersedes only ADR 0020's restriction of walkway pixel treatment to the lit top edge; its other decisions remain in force.

## Consequences

- End faces retain the source's texture and density without a new raster source, stretching the stone, or changing collision.
- The end treatment follows newly exposed terrain after destruction, without recomputing the original stairs' orientation.
- Stone contrast and walkway density measurements follow the loaded texture's alpha through the actual drawn fragments. The surrounding void remains scenery; the original thresholds, minimum observations and scenery, shop and teeth measurements remain.
- End checks pair actual stone with its nearest void inside the old rectangular bounds and a neighbouring uninterrupted interior. Restoring rectangular faces must fail them.

## Signs the decision was wrong

- The contour reads as repeated teeth or exposes a seam where the loft underside meets its deck.
- A visible walked-on edge no longer agrees with the original cell boundary, or an interior gains a false end.
- Source texture becomes broader than one screen pixel at the close camera, or a newly exposed end retains a rectangular saw cut.
