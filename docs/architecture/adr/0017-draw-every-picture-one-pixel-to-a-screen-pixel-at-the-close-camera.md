# ADR 0017: Draw every picture one pixel to a screen pixel at the close camera

> Status: accepted; the 1280×720 screen at whole multiples superseded by ADR 0024
> Date: 2026-11-04

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

The concept `search-mockup-v1.png` at commit `241124d5ba2dcef937c904623f1e5e3cb03fbcf5`, 1672×941, carries detail every one or two of its pixels. At `17b4d09ba32a21e1160654fa7ccb7aa98edd4119` the 1280×720 screen mixed three densities: the throat and the acid band were drawn four screen pixels to a picture pixel, the teeth two, and the characters, walkways and props, drawn in the scene's pixels through the camera at twice the scene's size (`CLOSE` in `src/rendering/camera.cpp`), two. Cut at the same scale, the game read two to four times coarser than the concept. [Issue #186](https://github.com/garamsh/neocity/issues/186) asks for one density, one picture pixel to one screen pixel at the default camera, set here and reached in four stages: the backgrounds, then the walkways, buildings, props and ground, then the characters, then the icons and interface.

The camera does not hold one zoom. Two humans are framed together, and [ADR 0009](0009-follow-the-humans-with-a-close-camera.md) eased the zoom anywhere between once and twice the scene's size as they part, a scale at which a picture drawn with nearest filtering shows some of its pixels one screen pixel wide and some two. Art at the new density only makes that worse: below twice the scene's size every picture pixel is smaller than a screen pixel.

Alternatives considered for the zoom:

- **Keep the eased zoom with nearest filtering.** Between the two zooms the screen drops some picture pixels and doubles none evenly, so columns of one picture pixel come and go as the camera moves.
- **Keep the eased zoom with linear filtering.** Every picture pixel is blended with its neighbours, which is the smooth look ADR 0012 rejected for light.
- **Draw the scene at twice its size into a picture of its own and scale that picture down.** The same fractional scale reaches the screen, on one picture instead of many.
- **Never zoom out, and give two players a split screen.** It changes the session and the interface, and ADR 0009 already rejected a split screen as narrower than the map's ladders and gaps need.
- **Never zoom out, and keep two humans inside one close frame.** Holding a human back changes the rules, which the issue keeps.
- **Two zooms only, twice the scene's size and its own size.** At twice, a picture drawn at twice the scene's resolution is one picture pixel to a screen pixel; at its own size each screen pixel takes exactly two by two picture pixels, which a copy of the picture at half its size, its mipmap, holds averaged.

For the pictures that do not stand in the map's cells, the backdrop layers, the sea in the scene, the teeth and the drawn steam and spores, nothing ties their size to the scene, so they need not scale with the camera at all.

## Decision

Every picture is drawn one picture pixel to one screen pixel at the default camera. A picture that does not stand in the map's cells, the throat, the city, the acid band, the sea in the scene, the teeth, and the steam and spores drawn over them, is drawn at its own size on the screen whatever the camera's zoom. A picture that stands in the map's cells is drawn at twice the scene's resolution, half a scene pixel to its pixel, once its stage redraws it, and the camera draws the scene at twice its size or at its own size, never between: two humans are framed at twice while they fit and at once otherwise, and the zoom steps once the wanted zoom has held for half the camera's ease, so a step wanted only for a moment is never drawn. At the scene's own size a picture at twice its resolution is read from its half-size mipmap, so each screen pixel shows two by two of its pixels averaged. The finishing pass works the acid's effects out per screen pixel, and every other effect per pixel of the scene's art.

## Consequences

- The sizes of stages 2 to 4 follow: a world, walkway, ground or decoration picture is twice its cell's scene size in pixels, 64 per 32-pixel cell; a character frame is 128×128 pixels, drawn at half size in the scene; the interface's pictures are drawn at their own size on the screen.
- `tools/art/pixelize.py check` fails a picture in `backgrounds/` drawn in blocks of two or more pixels, and `project/e2e/survival_capture.gd` fails a city frame drawn at a zoom other than once or twice, or whose teeth, backdrop or acid stand in pixels larger than a screen pixel at twice the scene's size. A later stage adds its folder to that check.
- Until stages 2 to 4 land, the pictures in the map's cells are still drawn at the scene's resolution, two screen pixels to their pixel at twice the scene's size, beside a backdrop drawn at one.
- Two humans parting past what fits at twice the scene's size see the frame jump to the scene's own size instead of easing out; the camera's centre still eases.
- At the scene's own size a picture drawn at twice the resolution shows half its detail. The first stage that draws such a picture imports it with mipmaps and draws the city with the nearest-mipmap filter.
- The sea in the scene no longer grows with the camera: at the scene's own size its sunken trains are drawn as large as at twice.
- The finishing pass reads the band's and the sea's columns from where each picture starts, so their ripples, bubbles and runs of light move with them.

## Signs the decision was wrong

- Players find the jump between the two zooms jarring, and it needs a transition, or the camera an eased zoom drawn some other way.
- Two players spend most of a run apart, so the scene is mostly drawn at its own size, where the new pictures lose half their detail.
- The denser backdrop competes with the walkways, and the walkways' faces stop reading darker than it.
