# `pan_dodge`

Dodge hazards dropped onto a frying pan that sways by itself, and escape through an exit that travels around its rim.

## Provenance

| Field | Record |
|---|---|
| Proposer | sauce-git, [Discussion #15 comment](https://github.com/garamsh/neocity/discussions/15#discussioncomment-19232539) ("엄청나게 큰 후라이팬 위에서 장애물을 피하며 탈출"); 3D, a self-moving pan, and spawning/removing hazards requested in [PR #17](https://github.com/garamsh/neocity/pull/17#issuecomment-6463182460) |
| Issue | [#16](https://github.com/garamsh/neocity/issues/16) |
| Creative decision-maker | sauce-git, the requesting project owner named on #16 |
| Specification | sauce-git's comments; scope written by AI (Codex as PM, then Claude Opus 5.5 as PM) on #16 |
| Implementation | Accountable: sauce-git. First 2D version written by Codex (gpt-6-astra); this 3D version written by Claude Opus 5.5 |
| Assets | none (Godot primitive meshes and the fallback font) |

## Play

| Field | Record |
|---|---|
| Goal | Reach the green gate on the rim and pass through it without touching a landed hazard |
| Controls | WASD or arrow keys move; R starts a fresh run; M returns to the developer page when mounted |
| Rules | The player moves directly at a fixed speed, diagonals included, and the pan's sway adds a drift. The rim blocks the player except at the gate, which travels around it. From 1.5 s, a hazard drops every second: alternately one whose shadow follows the player until it locks shortly before landing, and one at a random spot. A landed hazard ends the run on contact, slides with the sway, and is removed after 3 s or when it slides past the rim. Escape or a hit freezes everything. Reset restores the start, including the same drop sequence |
| Presentation | 3D, angled camera. Grey disc and torus: the pan, tilting with the sway. Blue cylinder: player. Red balls: hazards. Orange shadow: a drop still following the player; red shadow: a locked drop. Green bar: the gate |

## Run

| Field | Record |
|---|---|
| Scene | `project/experiment/pan_dodge/pan_dodge.tscn` |
| From the game | Main menu 2, then developer page 1 |
| Directly | `godot --path project res://experiment/pan_dodge/pan_dodge.tscn`, after `make build` |
| Rules alone | `.tools/cpp/bin/cmake -S src/experiment/pan_dodge -B build/pan-dodge-only -G Ninja -DCMAKE_MAKE_PROGRAM="$PWD/.tools/cpp/bin/ninja"`, then `.tools/cpp/bin/cmake --build build/pan-dodge-only` and `.tools/cpp/bin/ctest --test-dir build/pan-dodge-only --output-on-failure` |

## Verification

| Field | Record |
|---|---|
| Unit tests | Nine `PanDodge.*` cases in `src/engine_test.cpp`, run with the Rules alone commands. The revision, results, and the behaviours removed to see each fail are in [PR #17](https://github.com/garamsh/neocity/pull/17)'s Verification, at the head it states |
| Live checks | `godot --headless --path project --main-loop PanExperimentsCheck --fixed-fps 120` (`project/e2e/experiments.gd`), revision as above: developer page, mount after release, movement, sway and gate travel, a hit, an escape, drops and removals, unmount, and direct launch |
| Capture | [`docs/architecture/evidence/pan-experiments/pan-dodge.png`](../../../docs/architecture/evidence/pan-experiments/pan-dodge.png), from `godot --path project --main-loop PanExperimentsCapture --fixed-fps 120` on the tree of the commit that last changed the image, `git log -1 -- docs/architecture/evidence/pan-experiments/pan-dodge.png` |
| Play feedback | none yet |
| Not verified | Fun and balance; platforms other than Linux x86_64; platform export |

## Status

| Field | Record |
|---|---|
| State | Implemented in [PR #17](https://github.com/garamsh/neocity/pull/17) |
| Deferred | Space-bar jumping, from the [#16 scope decision](https://github.com/garamsh/neocity/issues/16#issuecomment-6463884135) |
| Main game | not accepted |
