---
name: create-experiment
description: Build, change, or retire a playable developer-page experiment in neocity — a design-free test game under src/experiment/<id>/ with its own C++ rules library, Godot scene, record, catalog entry, and checks. Use when a user asks an AI to make, prototype, or try a new game idea, mini-game, or play style on the developer page, or to change an existing experiment.
---
# Create experiment

Turn a requested play idea into a self-contained experiment the developer page lists, delivered as one pull request.

## Establish the task

Read repository-root `AGENTS.md`, `docs/convention/README.md` and the convention files it lists for C++, Godot, testing, git, and documentation, then `docs/architecture/README.md`, `docs/architecture/structure.md`, `docs/architecture/experiments.md`, and `docs/collaboration.md` §Adding an experiment. Those documents hold the rules; this skill only orders the work.

Find the scoped issue and its comments. It names the experiment id, allowed paths, constraints, acceptance criteria, and the creative decision-maker. If there is none, write the idea up as a proposal per `docs/collaboration.md` and stop until the PM scopes it; a user's request to an AI does not scope a change by itself. A path the work needs that the issue does not allow goes back to the PM before you edit it.

Read one registered experiment end to end — its record, library, tests, `view/` node, and scene — and copy its idioms rather than inventing new ones.

## Build it

1. Write the rules first as the library in `src/experiment/<id>/`, with its own `CMakeLists.txt`, public header under `include/<id>/`, and co-located unit tests. Keep it free of Godot so it builds and tests from its own folder.
2. Write the Godot node in `src/experiment/<id>/view/` and the scene `project/experiment/<id>/<id>.tscn` whose root is that node. Plain shapes and text only, unless the issue approved art.
3. Register it in every place `docs/architecture/experiments.md` §Registration and launch lists.
4. Write the record `src/experiment/<id>/README.md` from `docs/development/experiment-template.md`. Record who proposed the idea with the Discussion permalink, and which parts an AI wrote, with the tool and model.
5. Add the experiment's route to `project/e2e/experiments.gd` and update the line `Makefile` matches.
6. Update `docs/architecture/experiments.md` §Registered experiments and any other document the change leaves stale, in the same pull request.

## Verify and deliver

Run `make ci`. Build and test the library alone with the commands in its record. Show each unit test fails when its behaviour is removed, one behaviour at a time, and record which behaviour broke which test. Launch the scene both from the developer page and directly. Capture a real rendered frame with a display; where none is available, say so instead of claiming one.

Report fun and balance as unverified; checks and captures do not show them.

Stage only this task's files, push the issue's branch, and open or update one pull request with `.github/PULL_REQUEST_TEMPLATE.md`, linking the issue and the Discussion comment. Do not merge it or change conventions, other experiments, or the main game.

## Retire or change

Changing an experiment keeps its record current in the same pull request. Retiring one deletes both folders, its catalog entry, its registrations, and its harness route, and removes it from §Registered experiments; the record goes with it, since deleted documents are not archived.
