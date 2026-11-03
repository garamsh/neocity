# ADR 0012: Finish the city in a pixel-grid light pass

> Status: accepted
> Date: 2026-11-03

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

After [ADR 0011](0011-open-the-city-to-a-bright-throat-over-glowing-acid.md) the city screen had the concept's composition but stood still: the acid was a flat picture, its light a few translucent rows added over the whole width, and the lamps' and drips' light round discs drawn smooth over the pixel art. The user asked for glowing acid and light that moves like the concept `search-mockup-v1.png` at commit `241124d5ba2dcef937c904623f1e5e3cb03fbcf5`, not plain pixel art, and [issue #144](https://github.com/garamsh/neocity/issues/144) asks for it on the pixel grid, with no smooth blobs, at 60 frames a second at 1280×720, and with the headless checks unaffected. The project renders with `gl_compatibility`.

Alternatives considered, measured on Godot 4.7.2 `gl_compatibility` on an NVIDIA GeForce RTX 3060 with a white square on black:

- **The engine's glow** (`WorldEnvironment` with a canvas background and glow on). It does reach 2D under this renderer, but in the 8-bit buffer a 1.0 white square lit only 0.11 of white 8 pixels away and 0.008 at 24. With `Viewport.use_hdr_2d` an overbright 4.0 square lit 0.31 at 24 pixels, as a smooth round blob that also smears the square itself. Its result cannot be stepped onto the scene's pixels, and a linear HDR canvas would shift every colour the capture checks measure.
- **More translucent shapes on the glow layer.** That is what made the flat rows and round discs; it cannot read what was drawn, so it cannot make the acid ripple or light only what stands near it.
- **One shader per picture.** The acid is drawn in pieces under the band, the sea and every support and walkway in front of it, so a shader on the acid picture alone could not light what stands near it, and a sprite shader per kind of object would spread one effect over many materials.

A `canvas_item` shader reading the screen texture with `filter_linear_mipmap` works under the same renderer: its mipmap 4 spread a white square's light to 0.15 of white 24 pixels away, smooth until the shader steps it.

## Decision

The city is finished by one full-screen pass over the drawn scene and its light, under the interface: a `canvas_item` shader that reads the screen, works each effect out once per pixel of the art it lies on, the acid band's own pixels behind the map and the scene's pixels elsewhere, and steps soft light through a few levels with ordered dithering. It makes both acid surfaces heave with light running along them, crawls a web of light over the acid and bubbles through it, lights green whatever stands near the acid, spills light around bright parts, leans shadows to magenta and darkens the edges toward violet. The painters move the rest on the art's real clock: the throat breathes and drips saliva, flesh on the walkways drips, steam rises off the acid and spores drift in front of and behind the map. The lamps' and drips' light rings are drawn in whole pixel rows, and the acid's flat rows of light go.

## Consequences

- The pass runs only on a frame whose backdrop noted where the acid lies: the city, and the backdrop under the workshop's kitchen. The menus and the interface are not finished.
- The headless checks run on the dummy renderer, which draws nothing, so the pass and its shader do not reach them; the capture checks read the finished frame, and measure the acid's motion between two frames, the scene's pixels below the sea's surface, and the frame rate.
- The pass tells acid from everything else by its colour, green well over red and blue: anything drawn in that green takes the acid's ripple and web.
- The pass reads the screen once per frame and samples its mipmaps; at 1280×720 it costs a copy of the screen and a few texture reads a pixel.
- What moves runs on a real clock and moves nothing in the rules, so a frame is not reproducible from the run's state alone.

## Signs the decision was wrong

- Players read the dithered light as noise over the walkways, and the light has to step smoothly after all.
- Art in the acid's green outside the acid wobbles with the ripple, and the pass needs a mask drawn by the painters instead of the colour test.
- The pass costs the frame rate on the machines the game targets, and the effects have to move into the painters' own drawing.
