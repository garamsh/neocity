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
