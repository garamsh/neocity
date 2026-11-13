# ADR 0028: Lock the screen to both actors

> Status: accepted
> Date: 2026-11-13

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

[ADR 0009](0009-follow-the-humans-with-a-close-camera.md) had the camera follow the humans in the city and ignore an AI partner, and framed two humans between once and twice the scene's size. [ADR 0027](0027-fix-the-street-and-aquarium-maps-and-damage-them-by-day.md) made map travel a joint act: the map changes only when both actors stand in the same exit zone. [Issue #225](https://github.com/garamsh/neocity/issues/225), part of goal [#218](https://github.com/garamsh/neocity/issues/218), asks for one screen that always holds both actors, so the game reads as a pair walking together and a human can see the partner they are waiting for at an exit.

Alternatives considered:

- **A camera clamp that stops following past a distance.** The rules would still let an actor walk off the screen, and an AI or a test that does not read the camera would never be stopped, so the gap would be a property of the picture and not of the game.
- **Zooming out as the actors part.** Pixel art is drawn at the close camera only ([ADR 0017](0017-draw-every-picture-one-pixel-to-a-screen-pixel-at-the-close-camera.md)), and a pair far apart would be small figures on a large map.
- **Pulling the partner along.** It moves an actor the player did not move, and a held jump or a fall would fight the pull.
- **Stopping the whole movement, jump included, at the range.** A player stopped at the range could not jump a gap towards the partner's side, and a jump is the only way up some steps.

## Decision

While both actors are in the city, a physics step that would widen the gap in x between them past `SCREEN_LOCK` is stopped, in the `survival` library's `Exploration`. The actor takes the step with its jump only: a jump still lifts it and it lands where it left, and climbing and falling are not held. The lock does nothing while an actor is at base. `SCREEN_LOCK` is a temporary 384 sub-cells, twelve cells, of the twenty the 1920×1080 canvas shows at the close camera, leaving four cells at each side of the pair.

The camera frames every actor in the city, human or AI, and no longer ignores an AI partner. Two actors are framed on the middle of their feet at the close zoom, and at the far zoom only when one stands above or below the other beyond the room kept around them. A `static_assert` ties the camera's room to `SCREEN_LOCK`, so the two fit at the close zoom whatever the heights.

The AI keeps within the lock. With a human partner the AI follows; with two AI the appraiser follows and the chef leads. A follower plans only over what lies within the lock of its partner and goes to the exit zone its partner stands in; a leader waits for a follower that has fallen behind. The AI chooses errands on the current map only and never sets out for the aquarium by itself; it reaches another map by following a human through an exit.

## Consequences

- One actor alone cannot reach an exit zone beyond the lock of the other; both must walk there, the one ahead waiting.
- A human cannot walk away from the AI partner, and an AI partner cannot be left behind by a human running ahead; the AI closes in at its walking pace and the human waits at the lock, unless the AI stands on a lower floor whose only climb lies beyond the lock, in which case it waits for the human to come back.
- The camera's look-ahead applies only to a lone actor, so a pair in the city is centred and never looked ahead of.
- An AI-only run stays on the street and finishes its days there, so the aquarium's sources are collected only with a human.
- Two AI actors could stall against the lock; the leash reaches as far as the follower already stands from its leader, and the leader walks to meet a follower whose only link lies beyond the leash. Unit tests guard both: a follower starting beyond the leash, and an AI pair on two levels linked beyond it.

## Signs the decision was wrong

- Players press against the lock often enough to call it a wall, and twelve cells has to be raised until the screen shows less than the room it was cut for.
- An AI that follows reduces the sources it reaches in a day enough that days no longer end on time or stock.
- The aquarium is never collected from in single-player play, and the map is dead weight for a lone human.
