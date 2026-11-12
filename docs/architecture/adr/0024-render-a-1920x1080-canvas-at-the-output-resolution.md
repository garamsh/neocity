# ADR 0024: Render a 1920×1080 canvas at the output resolution

> Status: accepted
> Date: 2026-11-12

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

[Issue #209](https://github.com/garamsh/neocity/issues/209) belongs to goal [#207](https://github.com/garamsh/neocity/issues/207), which moves the game's art to a 1920×1080 screen. Until now `project/project.godot` drew the screen as a 1280×720 viewport stretched by whole multiples only, as `docs/architecture/godot-runtime.md` recorded: a window or full screen showed it at the largest whole multiple that fits, in a black border, so a 1920×1080 display showed it once inside a border and the player had no way to choose a size.

The existing layouts, the art cut at the 1280×720 screen by [ADR 0020](0020-cut-the-city-from-the-mockup-at-the-screens-size.md) and [ADR 0022](0022-cut-the-interface-frames-and-bag-icons-from-the-mockup.md), and the checks of `project/e2e/` are all written in 1280×720 units. The redrawn art of the goal's later issues is not here yet.

Alternatives considered for how the drawing reaches the window:

- **Keep the viewport stretch and a 1920×1080 viewport.** The drawing is a fixed-size picture copied to the window, so a window that is not a whole multiple of it shows a border or a coarse copy, and the picture is never drawn at the window's real resolution.
- **Stretch as canvas items at fractional scale and rewrite every layout in canvas units.** Every constant of `src/hud/`, `src/rendering/` and the e2e checks moves at once, and the existing art would be redrawn by code before the new art exists.
- **Stretch as canvas items at fractional scale, with the existing layouts scaled onto the canvas.** The canvas is 1920×1080, the drawing code keeps its 1280×720 units, and one scale of 1.5 takes them onto the canvas until the art is redrawn for it.

## Decision

The logical canvas is 1920×1080, set in `project/project.godot` with the stretch mode `canvas_items`, the aspect `keep` and the scale mode `fractional`: Godot draws the canvas at the window's real output resolution, with no whole-multiple limit, and letterboxes any other aspect ratio. `src/rendering/screen.hpp` is the one place that holds the canvas size, the art's scale of 1.5 and the 1280×720 layout size the drawing code is still written in. `MainScene` gives the root viewport a `global_canvas_transform` that scales by that 1.5, so the existing art and layouts keep their composition on the canvas, and the mouse position the scene reads is still in layout units.

The player chooses the window's size from 1280×720, 1600×900, 1920×1080, 2560×1440 and 3840×2160, and between a window and full screen, on the pause screen and on a Settings page the title reaches. `src/rendering/display.cpp` keeps the choice in `user://display.cfg` and applies it to the window: full screen fills the display at its own size, since Godot has no exclusive full screen that changes the monitor's mode, and windowed sets the chosen size, centred. In full screen the resolution row shows the display's size as its value and its arrows are refused with a reason, saving nothing, so a choice that would change nothing on screen is never stored or confirmed by a sound; making the window a window applies the chosen size again. A window cannot be chosen larger than the display: the list stops at the largest size that fits, as it does at its ends, and a saved size larger than the display stays in the file while the window opens at the largest that fits. A file that is missing or holds a value that is not one of the choices gives the defaults, 1920×1080 in full screen, the mode the game started in before. A choice that cannot be written to the file stays in force for the session, is reported by the session's display status and in the engine's error log, and the next choice that can be written clears it.

## Consequences

- The 1.5 stays until the art is redrawn for the canvas; a later issue of the goal removes it picture by picture, and `ART_SCALE` is then the one constant to retire.
- The canvas is drawn at the window's real resolution, so a picture scaled by 1.5 on a 1920×1080 canvas covers one and a half canvas pixels per art pixel, which nearest filtering shows as columns of one and two pixels. [ADR 0017](0017-draw-every-picture-one-pixel-to-a-screen-pixel-at-the-close-camera.md) decided one picture pixel to a screen pixel at the 1280×720 screen at whole multiples, which no longer holds here; the density target stays and the redrawn art of the goal restores it at the new size.
- `project/e2e/survival_capture.gd` measures the 1280×720 screen at whole multiples of it; it is a capture harness and no check of `make test` reads it, so its window-size measures are out of date until it is rewritten for the canvas.
- The title has a fifth entry, Settings, after Quit and with no number key, because `src/input` binds the number keys 1 to 4; a later change to that file can number it.
- The finishing shader's dither and the acid's effects are laid out in layout units, so they are 1.5 screen pixels at 1920×1080 and scale with the window.
- `Window` and `ConfigFile` are not in the project's trimmed godot-cpp bindings, so `display.cpp` reaches them by property and method name.

## Signs the decision was wrong

- A redrawn picture needs its own scale beside the 1.5, which would show that the single global transform is too blunt a carrier for the transition.
- The mouse lands off what it points at at some window size or aspect ratio, which would show the layout-unit pointer does not follow the canvas.
- Players report that the full screen resolution row is a dead control, which would show that the choice should also size the full-screen canvas.
