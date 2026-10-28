# Experiment record template

The fields of an experiment's record, `src/experiment/<id>/README.md`.

Copy the block below into the record, replace each prompt with a fact or with `unknown` / `not implemented`, and keep the record current in every pull request that changes the experiment. The [experiments](../architecture/experiments.md) document states what an experiment must contain; this template only records it.

```markdown
# `<id>`

One line: what this experiment lets someone try.

## Provenance

| Field | Record |
|---|---|
| Proposer | Person who proposed the idea, with the Discussion comment permalink |
| Issue | Issue URL that scoped the work |
| Creative decision-maker | Person named on the issue |
| Specification | Who wrote it; the AI tool and model if one drafted it |
| Implementation | Accountable person; the AI tool and model if one wrote code |
| Assets | `none` (plain shapes), or creator, source, and AI involvement for each |

## Play

| Field | Record |
|---|---|
| Goal | What the player tries to do |
| Controls | Every input and its action |
| Rules | Movement, hazards, success, failure, and what reset restores |
| Presentation | 2D or 3D, and what each shape and colour means |

## Run

| Field | Record |
|---|---|
| Scene | `project/experiment/<id>/<id>.tscn` |
| From the game | Main menu 2, then the developer page number |
| Directly | `godot --path project res://experiment/<id>/<id>.tscn`, after `make build` |
| Rules alone | The configure, build, and `ctest` commands for `src/experiment/<id>` |

## Verification

| Field | Record |
|---|---|
| Unit tests | Revision, command, result, and the behaviours removed to see each test fail |
| Live checks | Revision, harness command, and what it drives |
| Capture | Image path, revision, and the command that produced it |
| Play feedback | Who played, when, and what they observed; `none yet` until someone has |
| Not verified | Platforms, hardware, and behaviour no check covers |

## Status

| Field | Record |
|---|---|
| State | Proposed, implemented, or retired, with the pull request |
| Deferred | Requested behaviour not built yet, with its issue or comment |
| Main game | `not accepted`, or the decision and integration issue that accepted it |
```
