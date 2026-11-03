# ADR 0009: Follow the humans with a close camera

> Status: accepted
> Date: 2026-11-03

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

[ADR 0007](0007-move-actors-through-a-side-view-with-gravity.md) kept the whole 36×16 map on one screen, so a character figure stood about 7% of the screen's height, against about 13% in the concept `search-mockup-v1.png` at commit `241124d5ba2dcef937c904623f1e5e3cb03fbcf5`. The user found the map's mood and scale far from the concept, and [issue #128](https://github.com/garamsh/neocity/issues/128) records the PM's decision: a camera that follows the human's character at twice the scene's size, frames two humans together, ignores an AI partner, and never shows past the map's edges, with the concept's saturated, glowing mood and backdrop layers that move apart as the camera moves. The rules, their numbers, and the map stay as they are.

Alternatives considered:

- **Keep one screen and enlarge only the sprites.** Characters taller than two cells would cover the walkways and the words above them, and the map would still read as a board.
- **A split screen for two players.** Each half is narrower than the map's ladders and gaps need to be read, and the issue asks for one shared frame.
- **Godot's `Camera2D` and a scene tree of sprites.** The map is drawn in code on one canvas; a camera node would move only that canvas and not the words, which must stay at their own size on the screen, and it would put the camera's state in the host's tree rather than in the drawing that reads it.
- **Darker backdrop tints for contrast.** They keep the walkways readable but are what made the screen look unlike the concept.

## Decision

The city is drawn in the scene's own pixels through a camera that eases toward the humans in the city on the real clock: one human at twice the scene's size, a little ahead of where it faces; two framed together between once and twice. It never shows past the map's sides or the scene's top and bottom. The map's words, and pointers to goals off the screen, are drawn on the screen at their own size. The backdrop is three layers that slide less than the map, the farther the less, the throat and the city faded toward one dark violet by translucent hazes laid over them, the throat more, and the light of lamps, drips and the acid is added over the scene on a layer that blends by adding.

## Consequences

- A human's character stands 12% or more of the screen's height, and the scene reads closer to the concept.
- Most of the map is off the screen, so the NEXT line's goal gets a pointer at the screen's edge when it is off the screen, and words for what is off the screen are not drawn.
- The checks that read the screen measure through the camera's mapping, which the city leaves on the canvas, and measure each look on the frames that show it.
- Two humans far apart are drawn at a zoom between once and twice, which is not a whole multiple, so pixel art then shows uneven pixel sizes.
- The world, its light and the interface are drawn on three canvas items, so the scene is drawn from one frame of the run read once per process, with the camera and what it shows worked out once for that frame.
- The haze is a blend toward one colour, not a multiplied tint: it pulls the far layers toward that colour and flattens their contrast instead of scaling each colour channel, so the walkways still read over them.

## Signs the decision was wrong

- Players lose track of a goal or a partner off the screen, and the pointers do not bring it back.
- Two players often stand far enough apart that the camera stays near once the scene's size, where the uneven pixels and small figures return.
- Walkways stop reading over the brighter backdrop on maps the checks do not measure, and the haze has to thicken until the backdrop reads as dark as the old tints.
