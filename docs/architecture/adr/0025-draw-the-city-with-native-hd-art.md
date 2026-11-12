# ADR 0025: Draw the city with native HD art

> Status: accepted
> Date: 2026-11-13

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

[Issue #212](https://github.com/garamsh/neocity/issues/212), under [goal #207](https://github.com/garamsh/neocity/issues/207), connects the environment and character art delivered by #210 and #211. ADR 0024 supplies a 1920×1080 canvas but retains 1280×720 layout units and the global `ART_SCALE`. Enlarging the old mockup cuts on that canvas loses their native density. The new resources retain the mockup's composition through newly drawn assets instead of adopting its pixels.

The stair drawing previously descended within a cell whose simulation floor is level. Its apparent slope was not a surface the actor could follow. The map and movement rules remain those of ADR 0019; this change corrects their presentation.

## Decision

Draw the city's environment from `art_hd/` and its actors from `art_hd/characters/`. At the close camera, one native asset pixel covers one canvas pixel on the 1920×1080 output. Backgrounds and the sea use `1 / ART_SCALE` layout units per asset pixel; cell art and actors use `1 / (2 * ART_SCALE)` scene units, followed by the existing close zoom of two. Source rectangles use the same density as their image, so full images and band fragments agree. Layout positions, camera behavior and the repeated twenty-column map remain unchanged.

Use the HD character manifest's common pivot and baseline for every city pose. Keep the original character IDs for workshop, menus and portraits, whose layouts and crops belong to the interface work. Images without an HD replacement, including the kitchen, icons and interface, retain their old resources and canvas upscale. The HD city resources also serve later days and retain the existing destruction, acid, repair and stock presentation.

Draw every standing ground cell with a level top at its actual floor line, including tilted slabs. Preserve the exposed-end recesses of ADR 0023 below that edge. A lower neighbour at the same floor meets the full edge without a gap; a change of floor row remains a real drop. Do not invent a diagonal collision surface in the picture.

This replaces ADRs 0018 and 0020's city source and resampling decisions and ADR 0021's city character cuts and articulation. ADR 0022's interface cuts remain in use. ADR 0019's map and ADR 0024's canvas, window and layout decisions remain in effect.

## Consequences

- `ART_SCALE` remains the single transition scale. Retiring it globally would also change the interface and layout coordinates.
- Nearest filtering retains native detail at 1080p. At the distant camera and other output sizes, sampling follows the smaller projection; native density is claimed only at the close camera on the 1920×1080 output. The supplied HD imports have no mipmaps, so the distant view uses nearest samples of the smaller projection.
- The art checker reads both HD manifests, measures native dimensions and fine edge detail, and verifies the asset hashes independently of the legacy palette and mockup-cut rules.
- Captures save the native output. Layout value and placement checks sample a nearest-reduced layout image; density checks read the native capture. Window checks measure fractional canvas scale and centered letterboxing.
- The stair check observes drawn deck fragments and the real actor's floor in both travel directions, with the old stair and an adjacent flat deck as its controls.

## Signs the decision was wrong

- A close-camera HD picture duplicates source pixels at 1080p, or a band join moves when its neighbour is drawn whole.
- A city pose's feet drift from the simulation floor or change anchors when mirrored.
- A level cell acquires a visible slope or gap that its actor cannot follow.
