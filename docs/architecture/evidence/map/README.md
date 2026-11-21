# Map evidence

Records the maps bundle final check (goal #268, steps 1 and 2) run on `goal/maps` at `a854079`.

## Head

All runs below used `origin/goal/maps` at `a854079` (main with #295 and #296 merged), checked out on `task/268-maps-evidence`, no local changes to code.

## Checks

| Step | Command | Exit code | Result |
|---|---|---|---|
| 1 | `make ci` | 0 | `lint`, `format` and all `GODOT_CHECKS` passed, each finished line matching its expected count (for example `Survival: 294 live checks passed`, `Audio: 29 live checks passed`). The log carries 6 `WARNING` lines for the intro and loss cinematics ("Cinematic breaks the contract" ×5, "Cinematic did not load" ×1); they did not fail a check. |
| 2a | `StairsCheck` default | 0 | `Stairs: finished: pass; 4/4 direction/cell readings, 0 failures.` |
| 2b | `StairsCheck -- --map-art-tour` | 0 | `Stairs: finished: pass; both maps on days 1,5,10, paired human travel, native evidence and floor lines.` |
| 2c | `StairsCheck -- --kit-interiors` | 0 | `Stairs: finished: pass; kitchen and noodle shop use their own pictures after paired ladder travel.` |

Step 2 ran in a private `kwin_wayland --virtual --width 1920 --height 1080` session, `godot --path project --display-driver wayland --audio-driver Dummy --disable-vsync --fixed-fps 60 --main-loop StairsCheck -- <mode flag>`, with `XDG_DATA_HOME` pointing at a fresh directory per mode. The exit codes in the table are Godot's own, written by a wrapper script after the process ended.

## Boards

Each board has the 1920×1080 capture on the left and the mockup on the right at the same scale, with a label bar above each.

- `street-beside-mockup.png`: capture `268-map-street-day-1.png` (`230-street-day-1.png` from `--map-art-tour`) beside `docs/art/concept-art/toxic-night/f-street.png`.
- `building-beside-mockup.png`: capture `268-map-interior-kitchen.png` (`276-interior-kitchen.png` from `--kit-interiors`) beside `docs/art/concept-art/toxic-night/f-building.png`, the interior seen from inside (ADR 0042).

Visible differences, from reading the boards: the street capture shows the HUD in the new layout, the base on three floors at left, the vending machine and the acid pool, but with less ground-level clutter and with the characters smaller than in the mockup. The kitchen capture shows the single-room interior band with the ladder, kitchen props and the exit marker; the mockup shows a cutaway of a multi-floor building. The kitchen capture also has an enlarged appraiser sprite overlapping the room. This README records the difference and does not rule on it.

## Not run

- Step 3, the QA play: runs in a separate QA session.
- `make test-*` targets individually: they ran only as part of `make ci`.
- Captures of the days 5 and 10 street and aquarium views, the cold-store and service rooms, and the noodle shop were produced by the checks but are not committed; only the captures the boards use are.

## Recheck at 825748c

Rerun on `origin/goal/maps` at `825748c` (main with #295 and #296, plus #299 interior zoom cap and centring, #300 Cold Store stair, hall picture and badges inside), checked out on `task/268-maps-recheck`, no local changes to code. The earlier section above stays as recorded at `a854079`.

| Step | Command | Exit code | Result |
|---|---|---|---|
| 1 | `make ci` | 0 | Log `/home/sauce/.cache/w268mr-ci.log` ends `Audio: 29 live checks passed`, then `exit=0`. The log still carries the cinematic `WARNING` lines ("Cinematic breaks the contract"); they did not fail a check. |
| 2a | `StairsCheck` default | 0 | `Stairs: finished: pass; 4/4 direction/cell readings, 0 failures.` |
| 2b | `StairsCheck -- --map-art-tour` | 0 | `Stairs: finished: pass; both maps on days 1,5,10, paired human travel, native evidence and floor lines.` |
| 2c | `StairsCheck -- --kit-interiors` | 0 | `Stairs: finished: pass; kitchen and noodle shop use their own pictures after paired ladder travel.` |

Step 2 ran in a private `kwin_wayland --virtual --width 1920 --height 1080` session, `godot --path project --display-driver wayland --audio-driver Dummy --disable-vsync --fixed-fps 60 --main-loop StairsCheck -- <mode flag>`, with `XDG_DATA_HOME` set to a separate directory per mode. The exit codes are Godot's own, written by a wrapper after the process ended.

### Boards

- `building-beside-mockup.png` (replaced): capture `268r-interior-kitchen.png` (`276-interior-kitchen.png` from `--kit-interiors`, zoom 3, centred) on the left, `docs/art/concept-art/toxic-night/f-building.png` on the right, same scale, labelled.
- `cold-store-floors.png` (new, no mockup): grid of `268r-cold-store-hall-day-1.png`, `268r-cold-store-one-out.png`, `268r-cold-store-two-floors.png` and `268r-cold-store-hall-day-10.png`, each from `--map-art-tour`. The check captures no separate stair, middle-floor, upper-floor or roof frame, so those are read from these four: the hall, one human out with the other floors covered, and two humans on two floors.

`268-map-interior-kitchen.png` stays because the section above still cites it.

### What the boards show

Read from the images by eye. No check measures these, and this README does not rule on them.

| Question | Kitchen board | Cold Store board |
|---|---|---|
| Character at most 1.5x its street height | About 1.5x: the chef is about 260 px tall against about 170 px for the same character in `268-map-street-day-1.png`, with feet clipped by the room's lower edge. At the limit, not clearly under. | About 1.4 to 1.5x (the human on the upper floor is about 245 px). |
| Lit room centred, dark even around it | Yes: the room's centre is at y=451, the play-area centre, and it is centred left to right with dark on all sides. | In the hall frames the room is centred with dark around it. In `two-floors` the lit floors fill the width, 96 px to 1824 px, with unequal dark above and below because the camera holds two floors. |
| Cold Store stair treads meet the floors beside them | Not applicable. | Yes: the right ladder in `two-floors` runs from the upper deck to the floor line of the lower deck, with no gap seen. |
| Hall shows its own inside, not the street front | Yes: the kitchen's own picture. | Yes: freezers and the hall's own tiles in every frame. |
| No badge or tag floats outside the room | The item badge sits inside the room; `P1` is on the ladder. | `P1`, `P2` and the item badges are inside the lit floors. The orange selection box at top left of `two-floors` sits on the covered floor, dimmed. |
| Street terminal's vending machine inside the Cold Store hall (#281, low) | Not applicable. | Yes, still: the red vending machine shows on the lit lower floor in `two-floors`. #281 item stays open. |

## Not run

- QA play: ran on `a854079` and its findings were fixed by #299 and #300.
- `make test-*` targets individually: they ran only as part of `make ci`.
- A separate capture of the Cold Store stair, middle floor, upper floor and roof: `stairs_check.gd` does not produce one.
- Captures of the street, aquarium and service room on days 5 and 10 were produced but are not committed.
