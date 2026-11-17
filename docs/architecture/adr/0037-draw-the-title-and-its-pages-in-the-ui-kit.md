# ADR 0037: Draw the title and its pages in the UI kit

> Status: accepted
> Date: 2026-11-17

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

[Issue #249](https://github.com/garamsh/neocity/issues/249), a part of goal #246, asks that the title screen and the pages behind it, Controls, How to play and Settings, look like the approved concept's UI kit: riveted dark metal frames with worn rust edges, gold and cream lettering, small magenta flesh drips at the corners and orange lanterns. Until now the title drew the legacy key art, the legacy logo and cut panel and button frames from `rendering::panel` and `rendering::button`, and the pages drew the same cut frames over a veiled key art. `src/rendering/` and `src/hud/survival.*` belong to other issues of the goal, so a kit frame cannot be added there; the title's code is in `src/hud/menus.*` and `src/hud/display_rows.*`.

Alternatives considered:

- Add the title picture to the `rendering::Image` enum. Rejected: `src/rendering/` is another issue's, and the enum loads every picture at start for every screen.
- Draw the logo apart from the picture, as the legacy art did. Rejected: the generated picture carries a logo plate that matches the kit, and a second plate over it was a duplicate.
- Cut the kit frames as pictures, as the legacy frames are. Rejected: a frame drawn from rectangles in steps of one kit pixel, three screen pixels, fits any area, so the one routine serves the menu panel, the foot, the pages and the arrows of a settings row.
- Change the art manifest to list the new picture. Rejected: it is shared with the issue that owns the backgrounds; the picture stays outside the manifest and the check of its pixels is the capture below.

## Decision

The title screen is the picture `art_hd/title/key_art.png` across the whole screen with a menu panel at the right and a foot panel along the bottom. The menu panel holds the five entries as buttons: a dark metal button for each entry, and a gold one, which slides between entries with the cursor, under the marked entry. Each entry keeps its number cap, its name and its summary. The foot panel keeps the two lines it had. The entries, their order, their keys, the cursor, a click on an entry and every function of a page are unchanged; only what is drawn is new.

The Controls, How to play and Settings pages are a kit panel over the same picture under a dark veil, with a kit button as the way back. The arrows of a Settings row are kit buttons too.

`src/hud/title.hpp` and `title.cpp` hold the title and the kit's panel and button; the title's entries moved there from `menus.hpp`. The picture is loaded where it is drawn and held by the canvas, because a texture freed before the frame is rendered draws white. The routines record every frame they draw in the `title_frames` meta of the canvas, each with its kind, `art`, `panel`, `button` or `selected`, and its area, as the HUD's panels are recorded in `hud_panels`, so that a harness reads what was drawn.

### Provenance of the picture

`art_hd/title/key_art.png` was generated with the Codex CLI's built-in image generation, with no paid credits, from one prompt and two attached references. The prompt asked for one 16:9 title picture in the kit's pixel style: a broken cyberpunk street inside the monster's mouth, the right 40 percent dark and quiet for the menu, the Chef and the Appraiser small on a ruined deck at the left, and the logo "MONSTER MOUTH ESCAPE" on a riveted metal plate at the upper left as the only text. The references were the approved UI kit, `concept-4-ui-sheet.png`, and the street mood, `keep-round3-11.png`, of the concept round. The tool returned `title-art.png`, 1672x941, sha256 `41075d6d291408af2943c60a3fbf1095d16678439a928985e4c5171837876e87`. It was resized with `magick title-art.png -filter Lanczos -resize 1920x1080! key_art.png` and not retouched. The art is the project's own work product; the references are the project's own concept pictures.

### Checks

`project/e2e/title.gd` reads `title_frames` headless: the picture covers the screen, the menu panel and the foot panel are drawn, each entry has its button, the gold button stands on the marked entry and follows the cursor, and a page draws one panel and its way back with no gold button. `project/e2e/title_capture.gd` runs on a real 1920x1080 window and reads the saved pixels: gold under the marked entry, dark metal under the others, the picture's gold lettering where the logo is, and a steel rim on each panel. The captures are in `docs/architecture/evidence/title/`.

## Consequences

Easier: the title and its pages share one routine for a frame, so a page added later draws in the kit with one call; what the title draws is read by a harness.

Harder: the survival setup screen, in `src/hud/survival.cpp`, still draws the legacy key art and cut frames, so the choice of players is not in the kit until the issue that owns that file moves it; the legacy `title/logo.png` and `Image::TitleLogo` are no longer drawn by the title and stay for that screen's sake; and the new picture sits outside the art manifest, so no check measures its pixel grid.

Ruled out: a new `rendering::Image` for the title picture, and a second logo over the picture.

## Signs the decision was wrong

The setup screen next to the title looks like another game. The menu's right panel hides the picture's detail at a size where the entries are no easier to read. A frame drawn in code diverges from the kit's art where that art is cut by the issue that owns it. The captured pixels pass while a player sees a flat or white title.
