# ADR 0022: Cut the interface frames and bag icons from the mockup

> Status: accepted
> Date: 2026-11-06

Append-only: once merged, the body below is not rewritten. A changed decision means a new ADR.

## Context

[Issue #192](https://github.com/garamsh/neocity/issues/192#issuecomment-6670011576) applies the user's original-mockup direction to the interface. The [PM adoption](https://github.com/garamsh/neocity/issues/192#issuecomment-6670742939) accepts this source-cut policy. The code-drawn frames of ADR 0010 approximate its outlines but omit their detail. The original search mockup at commit `241124d5ba2dcef937c904623f1e5e3cb03fbcf5` supplies the frame borders and four ingredient pictures, and the larger recorded sources supply the other icons.

Alternatives considered:

- Keep the code-drawn frames: their different rim and corner pixels remain visible beside the original.
- Stretch complete frame pictures: the borders change thickness when their panels change shape.
- Enlarge the existing icon PNGs: enlargement adds no source detail.
- Generate replacement pictures: the existing sources already depict every current item.

## Decision

Cut the panel, button and bag-slot frames from the same once-resampled 1280×720 mockup used for the city and characters. Clear their baked text and pictures inside the border and make the scenery outside their corners transparent. Retain the border's original colours and pixels; keep its nine-pixel corners at native size and repeat its edge strips between them. Shared title, setup, base and pause panels use these frames too. A marked action keeps a dark fill, gold words and a gold selection rule.

Cut the mushroom, tomato can, fish and pickle jar from the original bag. They represent the existing Mushroom, Tomato, Fish and Pickles IDs, respectively. Reprocess the other seventeen ingredient, tool and dish icons from their recorded larger raw sources at 56 pixels, with their existing twelve-colour cleanup. The four original icon cuts, original sun and clock cuts, and three frame cuts retain source colours and density rather than the twelve-colour and lone-pixel cleanup; this exception names only those nine files.

Keep Tiny5's grid and minimum size, the current seven bag slots, both actors' status, health, stamina and tool state, every count and identification mark, action names, movement keys, goal, log, tips and cooking information. The two actor blocks remain beside native framed portraits; damaged-tool status occupies its own line. The seven slots use the original frame adapted to the available width, in place of the mockup's six. The action area retains room for two humans' controls and the longer current action names. Source icon plates reserve the drawn actor frame areas before placement, retaining counts and the existing label-placement rules.

This replaces ADR 0010's code-drawn frames and accent-filled buttons. Its information layout, font decision and the input and tip decisions of ADRs 0013, 0014 and 0016 remain in effect.

## Consequences

- Borders and four item pictures can be compared directly with their source cuts; the art spec and manifest identify each exception and crop.
- Panel interiors contain no baked words; all words and changing values remain runtime text.
- More space is assigned to the bag, and tool damage moves below stamina so both actors' information fits beside their portraits.
- Source plates can move farther from their cells to avoid the original character silhouettes.
