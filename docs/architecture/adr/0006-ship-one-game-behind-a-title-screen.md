# ADR 0006: Ship one game behind a title screen

> Status: accepted
> Date: 2026-11-02

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

ADR 0005 put a developer page of self-contained experiments behind the main menu, beside Monster Mouth Escape and the earlier ascent prototype that ADR 0004 had placed in the `gameplay` and `camera` folders, with its scene `project/levels/ascent/ascent.tscn` holding the `AscentScene` composition root. On 2026-11-02 the user instructed, as [issue #91](https://github.com/garamsh/neocity/issues/91) records, that everything on the main screen made for testing and for developers be removed and the main screen be built properly. The issue scopes that as one game, Monster Mouth Escape, opening on its own title screen, with the developer page, its experiments, and the ascent prototype removed from the menu and the repository. The survival screen still draws the chef and the appraiser from the ascent atlas, so that one asset stays until the world rendering changes it.

Alternatives considered:

- **Keep the experiments or the prototype behind a hidden or debug-only entry.** The issue removes them from the repository as well as the menu.
- **Keep the host scene in `project/levels/ascent/`.** That folder named a level the game no longer has; only the atlas remains there.
- **Put the host scene in the game's level folder, `project/levels/monster_mouth_escape/`.** The scene hosts the title screen as well as the game, so it is not a level, and that folder is the cinematic scenes' location.

## Decision

The application holds one game. It opens on a title screen offering New game, Controls, and Quit; New game opens the existing choice of players. The `experiment` purpose, its catalog and developer page, the `pan_dodge` experiment, the `gameplay` and `camera` folders, and the ascent level's scene and city texture are removed. The composition root is the `MainScene` node in `project/main.tscn`, at the Godot project's root.

This supersedes ADR 0005 in full, and ADR 0004's `gameplay` and `camera` purpose folders, its `levels/ascent/` level, and its `AscentScene` composition root. ADR 0004's template layout, the remaining purpose folders, and the single composition site stand. `project/levels/ascent/ascent_atlas.png` stays where it is until the survival screen stops drawing from it.

## Consequences

- `survival` is the only rules library, and `MainScene` the only node class the extension registers.
- A playable idea outside the game has no place in the application; trying one needs a new decision.
- The `gameplay_test` and `pan_dodge_test` suites, the ascent and experiment harnesses, and the 120 Hz physics tick the ascent and experiments ran on go with them.
- Contributor documents and skills written for experiments describe nothing that exists until their owners remove them.

## Signs the decision was wrong

- Prototypes for new ideas start appearing inside the `survival` library or the session, because they have nowhere else to run.
- A developer or debug menu is reintroduced to reach tools the title screen cannot.
