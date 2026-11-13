# ADR 0029: Draw fixed map themes and interiors at their floors

> Status: proposed
> Date: 2026-11-13

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

[Issue #230](https://github.com/garamsh/neocity/issues/230), under [goal #218](https://github.com/garamsh/neocity/issues/218), supplies HD art for ADR 0027's fixed street and flooded aquarium, three enterable street buildings and day-based damage. ADR 0025's environment already supplies the street's western shops, violet stone and swallowed-city background; repeating those structures over the terminal and aquarium gives both places the same silhouette and hides their different purpose.

## Decision

Retain the existing HD street layers and deck bands, add terminal scenery behind its fixed floors, and draw the aquarium with its own teal flooded-tank background and grated deck band. Draw occupied street interiors from three dedicated backgrounds on the existing interior zones; darkening and map-transition fades remain #224's responsibility.

Anchor every terrain deck, including the ferry's deck, on its collision floor. Read the existing object states to select collapsed span ends and kiosk rubble, and show the replacement fire escape with its own ladder segment; art never changes map layout or damage timing. All new runtime images belong to the HD manifest and keep ADR 0025's density conversion and nearest sampling.

## Consequences

- Both map themes use the existing canvas adapter and the rules' map identity; the aquarium no longer repeats the street's external supports.
- Collapsed span art hangs at the two ends rather than filling the missing route, while alternative paths remain entirely owned by content and physics.
- Interior art can be used by #224 without another asset pipeline or a gameplay change.
- The original generated sources, exact prompts and ImageMagick cuts remain in the repository; creative acceptance stays with the user.

## Signs the decision was wrong

- An apparent landable top sits above or below a floor the actor follows.
- Aquarium scenery can be mistaken for a street section, or a destroyed span still appears to bridge its gap.
- Interior art obscures the actor or requires moving a fixed building zone to fit.
