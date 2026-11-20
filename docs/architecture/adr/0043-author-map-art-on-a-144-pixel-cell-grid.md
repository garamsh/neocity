# ADR 0043: Author map art on a 144-pixel cell grid

> Status: proposed
> Date: 2026-11-20

The authored map-art density and independently usable interior floor panels for #276.

## Context

The [2026-11-20 PM scope](https://github.com/garamsh/neocity/issues/276#issuecomment-7042249960), under [goal #268](https://github.com/garamsh/neocity/issues/268), requests roughly 1.5 times the Toxic Night reference density and enclosed rooms explored from inside. ADR 0040's 96-pixel cell and equipment close-ups do not meet that brief. [Issue #281](https://github.com/garamsh/neocity/issues/281) owns drawing by destination world area, independently of texture dimensions.

## Decision

Propose 144 authored pixels per map cell, replacing only ADR 0040's 96-pixel density decision. Retain its Aseprite ownership and manifest export arrangement. Redraw source detail rather than enlarging the earlier kit, and use nearest sampling, hard alpha and undithered palette cleanup for exports.

Keep the existing runtime filenames, aspect ratios to the nearest output pixel, band proportions and normalized floor anchors. Six deck bands become 126 pixels high, wall bands become 144/144/144/162, and shop bands become 288; their twenty-cell period becomes 2880. A non-integral half-pixel canvas extent rounds upward: city height 735 becomes 1103 and vending-machine height 177 becomes 266. A world-area destination preserves their physical proportions independently of that rounding.

Author each interior picture as one independently usable enclosed floor, with furniture at the rear, depth, a clear foreground walking strip, a door and visible access equipment. Do not bake a multi-floor building, exterior wall, sky or street into a room panel. The kitchen and noodle panels are 864×288; cold-store and service-room panels are 432×432. Their bottom edge remains the floor anchor. The renderer owns per-floor placement and traversable stairs or ladders; painted access equipment alone does not establish a gameplay connection.

## Consequences

- The first HD delivery replaces the eighteen interior and street-kit exports; the manifest names that subset explicitly. Throat, acid lake, aquarium, support layer, other props, containers and hazards still require the same-density pass.
- Individual floor panels can be placed independently without cutting through furniture in a building-wide image. The cold-store and service-room panels are reusable across their respective floors; distinct floor variants are not delivered in this pass.
- Preserve fine source pixels when a panoramic canvas is larger than its source: the terminal panorama joins a reflected skyline pair above its bottom anchor, leaving transparent space above, rather than enlarging every source pixel into a coarse block.
- No camera, collision, movement, UI or character density changes here. Asset boards show artwork only; #281 and bundle QA must establish runtime placement, light and contact lines.
- Before this proposal lands, the PM must pair it with the affected presentation responsibility document and add it to the architecture index. Those paths are outside this Worker's allowance.

## Signs the decision was wrong

- Enlarging a prior 96-pixel export reproduces the new room details, rather than revealing independently drawn furniture and material marks.
- Nearest sampling leaves only multi-pixel blocks at the intended density, or smooth filtering is needed to hide an edge.
- A floor panel requires showing outdoor scenery, slicing furniture, or shifting the floor anchor to fit its destination.
- Painted ladders imply a route that the world-area renderer cannot align to the actual traversal geometry.
