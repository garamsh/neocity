# ADR 0021: Keep the mockup characters and articulate their pixels

> Status: accepted
> Date: 2026-11-06

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

[Issue #191](https://github.com/garamsh/neocity/issues/191) asks for the chef and appraiser as drawn in the search mockup at commit `241124d5ba2dcef937c904623f1e5e3cb03fbcf5`. The earlier 64-pixel frames changed their faces and proportions, and the close camera doubled their pixels. Keeping the old 16-colour, lone-pixel and selective-outline cleanup would alter the original cuts again. The [PM's task decision](https://github.com/garamsh/neocity/issues/191#issuecomment-6662970622) grants preserving the original colours and density for these characters.

Alternatives considered:

- Enlarge the old pictures: preserves their changed faces and enlarged pixels.
- Redraw every pose: introduces another interpretation of the original faces and clothing.
- Adopt the concept sheets' back views for climbing: changes the chef's bare feet and the figures' proportions.

## Decision

Cut idle from the same 1280×720 resampling of the mockup as the city, retaining the original RGB inside explicit silhouettes, and articulate those pixels for movement using nearest sampling. Use 128×128 frames at half size in the scene, one native pixel per screen pixel at the close camera; preserve the original different heights, 115 pixels for the chef and 72 for the appraiser, within 6 pixels on the first city screen.

The chef and appraiser frames retain the source's colours, outlines and light instead of the earlier character cleanup, while keeping the screen-pixel density requirement. Climbing may face sideways with a reaching arm and raised knee. The same idle cuts supply base actors and HUD portraits.

## Consequences

- Faces, hats, coats, skin, the lens and the chef's bare feet retain the original pixels throughout motion.
- Every motion frame can be regenerated from the historical mockup and committed articulation specification without a new raster generation.
- The two characters differ in height because the source does; they do not share the earlier uniform figure height.
- Rigid articulation limits how far a limb can turn before its join shows. It does not invent unseen anatomy.
- Capture checks measure projected alpha height and rendered pixel density inside the current character texture's opaque silhouette, excluding background, interface and overlapping actors.

## Signs the decision was wrong

- A motion frame separates a limb or changes the silhouette enough that it no longer reads as the original character.
- A frame's feet slide away from its ground position or a portrait loses part of the face.
- Background pixels included in a silhouette or shader detail allow an enlarged character to pass the density check.
