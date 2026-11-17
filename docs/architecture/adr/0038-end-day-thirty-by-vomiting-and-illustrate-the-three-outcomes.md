# ADR 0038: End day thirty by vomiting and illustrate the three outcomes

The completed-day limit and the still-image presentation of satisfaction, vomiting and acid failure.

> Status: proposed
> Date: 2026-11-18

## Context

- Goal #268 chooses a deterministic vomited-out ending after thirty days and an image with text for each outcome. Issue #270 implements the three outcomes that do not need a map escape device.
- ADR 0033 judges endings from workshop state only. The completed-day count belongs to progression, so that input no longer suffices.
- Acid already ends the run as a loss. Changing its phase would discard the loss-first rule and its existing presentation contract.

## Decision

- Extend the ending judgments to read the public progression snapshot as well as workshop state. After day thirty finishes, choose `VomitedOut` if the run remains in the workshop and satisfaction has not already won. A full-acid loss takes precedence over either ending.
- Retain the ordered rule table and the existing cinematic stage contract. Its ending and loss fallback screens draw one outcome-specific Toxic Night illustration and a caption; the intro remains a placeholder. End-of-day base timeouts are judged too.

## Consequences

- Thirty completed days mean the next day counter is 31. Starting day thirty alone does not end the run, and there is no random draw.
- Satisfaction remains first in the table. Acid failure remains `Phase::Lost` with no `Ending` value, but receives the same image-and-text treatment.
- The three pictures live with the level; their originals and generation prompts live in `docs/art/production/endings/`. They depict environments without the chef or appraiser.
- Skip, main-menu return, six-second duration and optional animated-scene overrides remain the existing cinematic contract. Animation and the map escape device remain outside this decision.

## Signs the decision was wrong

- A new ending needs state that neither snapshot provides, or an outcome requires a different transition contract.
- Players cannot read the caption within the existing six-second duration, or the illustrations obscure which outcome occurred.
