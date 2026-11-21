# ADR 0046: Name the set-out key in the first day's base tip

> Status: proposed
> Date: 2026-11-21

Supersedes, for the set-out tip only, the rule that a tip names the action and not its key, which [ADR 0014](0014-name-the-action-and-show-only-the-movement-keys.md) decides and [ADR 0034](0034-show-exploration-as-the-mockup-top-bar-and-one-action-prompt.md) leaves standing for the tips it moves to base; every other tip, and every other decision of both, stands.

## Context

[Issue #268](https://github.com/garamsh/neocity/issues/268) found that a first-time player at base on day one is told to set out and not which key does it: the base's prompts name the search key, the interact key and the fridge, and the travel key appears nowhere on the screen until the player is in the city. The owner decided in [a comment on #268](https://github.com/garamsh/neocity/issues/268#issuecomment-7063704743) that the existing TIP line takes the key and no new band or panel is made.

Alternatives considered:

- A departure band naming the key under the base. Rejected by the owner's decision above: the screen takes no new element for one key.
- Name the key in every tip. Rejected: only the first day's first action is blocked on a key the screen does not show; the other tips follow prompts that already name their keys.
- Name the key in the How to play page's line too. Rejected: that page lists the tips with no run and so no key set; the controls page beside it lists every key.

## Decision

The set-out tip drawn at base appends the travel key of each human who has a key set, read from the same key table the prompts and the controls page read: `Set out with [Q].` for one human, `Set out with [Q] (P1) or [U] (P2).` for two. The tip's other words, its place, its timing and every other tip are unchanged. The How to play page keeps the tip's line without a key.

## Consequences

Easier: a first-time player learns the set-out key from the line already on the screen; the keys come from the key table, so a rebound key cannot leave the line wrong.

Harder: the set-out tip is the one tip that names a key, so the checks that no tip names a key allow exactly that key on that tip and fail for any other key in any tip; the tip is longer, so its panel is wider.

Ruled out: a second tip or band for the same key until a later decision adds one.

## Signs the decision was wrong

Players still ask how to set out after reading the tip, or the tip's panel no longer fits the base's header row beside its neighbours. Other tips start to need a key, which would make naming keys in tips the rule and not an exception.
