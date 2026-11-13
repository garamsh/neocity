# ADR 0030: Patrolling mutants and fixed obstacles send an actor away

> Status: accepted
> Date: 2026-11-13

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

[ADR 0026](0026-remove-health-stamina-carried-tools-and-passage-repair.md) left the city with nothing that can stop a walk: acid drips have no effect and a fall into the acid is the only way to be sent home. [Issue #226](https://github.com/garamsh/neocity/issues/226), part of goal [#218](https://github.com/garamsh/neocity/issues/218), asks for danger that costs time, not the run: mutants that patrol the maps, and obstacles that stand in fixed cells. A character that touches one is sent away and returns beside its partner after ten seconds of exploration time. [ADR 0028](0028-lock-the-screen-to-both-actors.md) holds both actors to one screen while both are in the city, so what the lock does with an actor that is away has to be decided with the rule.

Alternatives considered:

- **Send the touched actor to base.** Base is a place the actor walks out of again at the entrance; the party would be split across the map, and the partner's collections would go on with a lock that has nobody to hold it to. Returning beside the partner keeps the pair together, which is what the screen lock is for.
- **Keep the absent actor's body on the map and lock to it.** A body the player cannot move would pin the partner to the spot where it was touched for ten seconds, so a touch would also punish the partner.
- **A mutant that chases or dodges.** More shapes of play and a path search for each mutant; a patrol is one row and two cells, so a player reads it at once and the unit tests can name every position it takes.
- **An AI that plans a path around each mutant.** It would need a search over time for each mutant. The AI routes around obstacles, which never move, and only waits for a mutant: it goes idle while a mutant on its row is, within two movement units, in a cell it could step into, computed from the patrol's fixed range and speed. It cannot pass a patrol that fills a one-cell-wide route, so a touch stays possible.

## Decision

Both live in the map data and are read by the `survival` library's `Exploration`. An `Obstacle` is a map and a cell. A `Patrol` is a map and two cells of one row, `from` on the left and `to` on the right; a mutant walks it back and forth, turning at the centre of each end cell. Mutants move one step of eight sub-cells each movement unit of 100 ms, on the shared clock, so they move only while exploration time runs. The old acid drip cells are obstacles, and the street and the aquarium list at least three mutants and three obstacles, and two and two.

An actor touches a mutant or an obstacle when its feet stand in the cell of an obstacle, or in the cell a mutant stands in. The check runs after each physics tick of the actor and after each mutant step. A touched actor is absent for `ExplorationConfig::absence`, ten seconds of exploration time, a temporary value. The absence is positive whenever the config has patrols or obstacles.

While absent an actor is in the city and not on the map: it takes no movement, collection or return command, it is not touched again, and its body stays where it was touched only so the snapshot has a place to report. When the time is up it takes its partner's body, position and stance, and plays on.

The screen lock ignores an absent actor: the lock holds a pair, and an actor that is not on the map is not held to the screen, nor does it hold anyone. The partner moves as it does with the other at base. The camera and every other reader of the snapshot read `absent` as "not on the map". The AI treats an absent partner as it does one at base.

If both actors are touched, both return to base. This is the case where the second is touched while the first is still absent, and the case where the partner is not in the city at all, so a lone actor that touches goes home. They keep their inventories and the day, and the workshop opens as after any return. Expiry of the exploration time while an actor is absent returns everyone to base as it does for any actor, and the absence ends with it.

The AI never routes through an obstacle cell: `CellMap` takes the obstacle cells, and a link into one is no link, so every route, the AI's errands and the path check alike, goes around. The path check of every map on every day counts obstacles as blocked, so a party can still reach every source, exit and interior zone.

`SurvivalRun`'s status reports each actor's `absent_ms`, -1 on the map, and `mutants`, the current map's mutants with `sub_x` and `sub_y`, so an end-to-end check reads the rule from the status and not from pixels.

## Consequences

- A touch costs ten seconds of the day, the actor's and by the shared clock the party's, and costs nothing else; the run is lost only by the acid.
- A lone actor and a pair that both touch are sent home, so the cost of a second touch is a finished trip, not ten seconds. A human can send an AI partner home by leading it into a mutant, and the AI's short wait cannot always avoid one.
- Mutants are drawn by the renderer in a later change; until then the status and the snapshot carry them and nothing draws them, and an actor that is absent is still drawn at its body.
- The HUD still names a return from a touch as a fall until the screen is changed to tell the two apart.

## Signs the decision was wrong

- Players or the AI are sent away so often on the spine of a map that a day ends on time before a collection, and mutants have to be moved or slowed until it does not.
- An AI-only run that walks blindly into mutants stops finishing days with food, so the AI waits for a mutant within two movement units.
- Ten seconds is so short that a touch is not felt, or so long that the partner waits; the temporary value needs another.
