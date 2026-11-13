# ADR 0031: Darken interiors and fade between maps in the presentation

> Status: accepted
> Date: 2026-11-14

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

[Issue #224](https://github.com/garamsh/neocity/issues/224), under [goal #218](https://github.com/garamsh/neocity/issues/218), finishes what [ADR 0029](0029-draw-fixed-map-themes-and-interiors-at-their-floors.md) left to it: an actor inside a street building shows that building's interior with the outside darkened, and a move between the street and the aquarium fades. [ADR 0027](0027-fix-the-street-and-aquarium-maps-and-damage-them-by-day.md) has the rules change maps at once, in one movement unit, and a fade must not change that.

## Decision

Both effects are presentation only and read what the rules already report; `src/survival/` does not change.

- While an actor drawn in the city is inside a street building's interior zone and no actor drawn in the city is outside every interior zone, everything outside that zone's rectangle is drawn under a veil of the background colour, easing in and out on `Art`'s eased values. The outside stays lit while any actor is out in it, so the city around a walking actor is never dimmed for a partner standing in a shop. The interior background and the actors in the zone are not darkened. Nothing about the building's floor, collision or the actor's input changes.
- A change of map starts a map fade of 0.6 seconds of frame time: the screen darkens to black over the first half and clears over the second. `Session` keeps the frame drawn before the change and shows it, with its own camera, through the first half; the run's own frame is shown from the half on. Each frame the status reports, the rules' map and the AI's steps are the run's, so a check or the status reader sees the new map at once.
- The fade takes no input and no time from the run. It runs on the same frame time as the fade between days, so a pause stops it, and the run is not held while it plays, as the rules' own timing in `survival.md` states: time and input are the rules', the fade is only drawing.

## Consequences

- Neither effect adds a rule, a setting or a test seam in the survival library.
- For up to 0.3 seconds after a map change the screen still shows the map just left while the run is already on the new one; an actor can be walked on, or the screen lock and the arrival cell act, behind the black.
- A click in that first half acts on what the screen shows, as every click does; its targets are those of the old frame.
- The veil is a plain rectangle frame around each interior, so it needs no new art and no shader.

## Signs the decision was wrong

- Players move or act in the new map unseen during the fade out, and report losing track of an actor: then the run should be held for the fade, which needs a rule in `survival.md`.
- The darkened outside hides a mutant or an obstacle an actor in the building needs to see.
- Players expect the outside to darken as soon as one actor enters a shop while the other walks outside it.
- The held frame of the old map is mistaken for the new one, or the fade is mistaken for a stall.
