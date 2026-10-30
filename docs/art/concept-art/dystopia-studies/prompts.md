# Dystopia concept image provenance

Generation inputs and observed outputs for the four candidate images in this exhibit.

## Status and source

- All four images and the exhibit text are AI-authored proposals, not adopted game direction, canon, production assets, or playable screenshots.
- Source of the ideas: the user–AI conversation dated 2026-10-30 (Asia/Seoul), scoped in [issue #19](https://github.com/garamsh/neocity/issues/19) and [the PM assignment](https://github.com/garamsh/neocity/issues/19#issuecomment-6504854269). No external discussion or third-party author is claimed.
- Authoring agent: Codex. Generation tool: built-in `image_gen.imagegen`, one independent call per image. Mode: new image generation; `transparent_background: false`.
- Model, seed, quality parameter: not exposed. Dimensions were read from the saved PNG headers. Originals were copied byte-for-byte; no post-generation image editing was performed.
- UTC tool timings below are observed call start/completion times, not model compute times. The local source paths identify this session's output; the adjacent relative PNG links are the portable files.

## Inspected references

The author opened these five images before writing the prompts. Their visual traits were described in text. No reference image paths or conversation-image parameters were passed to the generation tool, and none was an edit target.

| Reference | Role in these proposals |
|---|---|
| [ART-CHAR-001](../characters/chef-sheet-v1.png) | Yellow skin, green hair, expressive face and magenta ear details; occupation and tools changed |
| [ART-CHAR-002](../characters/detective-sheet-v1.png) | Green skin, angular head, bolts, X eye mark, suit and magenta details; profession-specific tools |
| [ART-BOARD-001](../environments/city-moodboard-v1.png) | Neon city depth and palette, without its text or story claims |
| [ART-MAP-002](../gameplay/night-market-v1.png) | Worn surfaces, localized lighting and readable characters |
| [ART-MAP-001](../gameplay/underground-refinery-v1.png) | Industrial layers and contrast between work area and atmospheric depth, without its tank premise |

## Image records

### ART-DYS-001

- Status: candidate; version 1; creative acceptance pending.
- File: [demolition-v1.png](demolition-v1.png).
- Observed generation start: 2026-10-29T18:22:43.395Z; completion: 2026-10-29T18:24:40.703Z.
- Original output: `/home/sauce/.codex/generated_images/01a188e4-bb81-7551-878d-7417ada278a3/exec-8631516e-0ec9-48ab-bfff-39fb3b75900b.png`.
- Output dimensions: 1672 × 941 pixels (PNG).
- Intended reading: occupied windows contradict clearance paperwork; shutdown controls and a locked demolition switch.
- Limits: a still image communicates the proposal, not validated controls, timing, fairness or implemented rules. Abstract marks stand in for future legible records; visible details are not adopted canon.

Exact prompt:

```text
Use case: stylized-concept. Asset type: new landscape game concept illustration for an offline proposal exhibit, not a playable screenshot.
Style: richly atmospheric neon dystopian pixel art, coherent crisp pixel clusters and hard pixel edges, detailed worn industrial surfaces, dark indigo and desaturated teal base, restrained cyan and magenta lights, warm amber active work surfaces, hazy city depth. Fixed side/oblique scene, readable interaction surfaces, thoughtful composition.
Two protagonists must be recognizable: (1) short expressive yellow-skinned field worker with messy dark-green hair, big round white eyes, broad face, magenta ear studs, pale low work cap, cream work clothes adapted to profession; worried determined expression, never a chef hat or frying pan. (2) short green-skinned inspector with angular tall head, two metal head bolts, visible dark red X mark over one eye, dark suit with magenta lapels/tie, serious face. Keep both faces readable and large enough to recognize. These identities and palette were described from reference sheets inspected by the author; no input image is being edited.
Landscape 16:9-ish composition. No written text, letters, numbers, captions, logos, watermark or floating game HUD. Mechanical gauges may show ticks and colored light without characters. No gore, combat, explosions or jumping. Make a new independent illustration. Primary request: demolition technician and residency inspector. Foreground cutaway municipal control room overlooks an old apartment facade across a narrow gap; the facade fills the upper/right two thirds. Yellow technician at left grips a disconnected power breaker near visibly distinct water valve and heating pipe controls. Green suited inspector at right studies occupancy papers and a physical stamp beside a protected red demolition switch still under a locked transparent cover. Across the facade, some windows are dark but a candle and a person's silhouette remain in one, and several figures cluster around a tiny warm lamp in another after heating shutdown. A cyan electric window and amber candle windows distinguish states. Readable visual tension: the paperwork implies emptiness but occupied windows contradict it. Both protagonists are inside the control room, hands near relevant tools, eyes toward evidence. No destruction.
```

### ART-DYS-002

- Status: candidate; version 1; creative acceptance pending.
- File: [relay-v1.png](relay-v1.png).
- Observed generation start: 2026-10-29T18:22:43.395Z; completion: 2026-10-29T18:23:40.133Z.
- Original output: `/home/sauce/.codex/generated_images/01a188e4-bb81-7551-878d-7417ada278a3/exec-04a9e948-1c93-40b4-a454-76a3fcfc5ce7.png`.
- Output dimensions: 1672 × 941 pixels (PNG).
- Intended reading: a routing desk links power and heat costs with a distant apartment and vehicle.
- Limits: a still image communicates the proposal, not validated controls, timing, fairness or implemented rules. Abstract marks stand in for future legible records; visible details are not adopted canon.

Exact prompt:

```text
Use case: stylized-concept. Asset type: new landscape game concept illustration for an offline proposal exhibit, not a playable screenshot.
Style: richly atmospheric neon dystopian pixel art, coherent crisp pixel clusters and hard pixel edges, detailed worn industrial surfaces, dark indigo and desaturated teal base, restrained cyan and magenta lights, warm amber active work surfaces, hazy city depth. Fixed side/oblique scene, readable interaction surfaces, thoughtful composition.
Two protagonists must be recognizable: (1) short expressive yellow-skinned field worker with messy dark-green hair, big round white eyes, broad face, magenta ear studs, pale low work cap, cream work clothes adapted to profession; worried determined expression, never a chef hat or frying pan. (2) short green-skinned inspector with angular tall head, two metal head bolts, visible dark red X mark over one eye, dark suit with magenta lapels/tie, serious face. Keep both faces readable and large enough to recognize. These identities and palette were described from reference sheets inspected by the author; no input image is being edited.
Landscape 16:9-ish composition. No written text, letters, numbers, captions, logos, watermark or floating game HUD. Mechanical gauges may show ticks and colored light without characters. No gore, combat, explosions or jumping. Make a new independent illustration. Primary request: signal technician and communications censor in a small elevated relay room at night. Yellow technician, tool belt and pale work cap, patches a vivid cyan cable into a large tactile radio patchboard with tunable dials, analog waveform traces without text, power battery gauge, separate thick state-broadcast cable, few red/amber indicator lamps and hot cooling coils. Green suited censor with head bolts and X eye wears headphones, holds a receiver and clipboard, hesitating over a route switch. Their workbench occupies foreground and left; wide window on right overlooks the same distant apartment block, one blinking warm window, antenna glow, and a tiny dark patrol vehicle with red lamp parked below. Composition visually connects cable route to distant illuminated apartment; obvious cost of routing is heat/power, signal origin exposed. No holographic HUD, no magnifier mandatory, no abstract cyberpunk action.
```

### ART-DYS-003

- Status: candidate; version 1; creative acceptance pending.
- File: [lifespan-v1.png](lifespan-v1.png).
- Observed generation start: 2026-10-29T18:22:43.395Z; completion: 2026-10-29T18:26:50.917Z.
- Original output: `/home/sauce/.codex/generated_images/01a188e4-bb81-7551-878d-7417ada278a3/exec-c12292e3-68f6-4ecb-aaf9-59a0b551b9c5.png`.
- Output dimensions: 1672 × 941 pixels (PNG).
- Intended reading: physical repair components, two allotment devices and an official ledger share one workbench.
- Limits: a still image communicates the proposal, not validated controls, timing, fairness or implemented rules. Abstract marks stand in for future legible records; visible details are not adopted canon.

Exact prompt:

```text
Use case: stylized-concept. Asset type: new landscape game concept illustration for an offline proposal exhibit, not a playable screenshot.
Style: richly atmospheric neon dystopian pixel art, coherent crisp pixel clusters and hard pixel edges, detailed worn industrial surfaces, dark indigo and desaturated teal base, restrained cyan and magenta lights, warm amber active work surfaces, hazy city depth. Fixed side/oblique scene, readable interaction surfaces, thoughtful composition.
Two protagonists must be recognizable: (1) short expressive yellow-skinned field worker with messy dark-green hair, big round white eyes, broad face, magenta ear studs, pale low work cap, cream work clothes adapted to profession; worried determined expression, never a chef hat or frying pan. (2) short green-skinned inspector with angular tall head, two metal head bolts, visible dark red X mark over one eye, dark suit with magenta lapels/tie, serious face. Keep both faces readable and large enough to recognize. These identities and palette were described from reference sheets inspected by the author; no input image is being edited.
Landscape 16:9-ish composition. No written text, letters, numbers, captions, logos, watermark or floating game HUD. Mechanical gauges may show ticks and colored light without characters. No gore, combat, explosions or jumping. Make a new independent illustration. Primary request: clock repair artisan and lifespan appraiser at an intimate battered workbench and customer hatch. Yellow artisan with green hair, pale work cap and cream workshop apron uses tweezers on an opened citizen wristwatch-like life allotment device, visible gears and three removable physical components arranged neatly on velvet. Green suited bolt-headed X-eye appraiser holds an open official ledger beside two differently lit wrist devices, one amber low dial and one cyan high dial, with a physical transfer cable between them; a stamp sits nearby. At the customer hatch behind them, a tired clothed civilian quietly presents a wrist and waits. A small pulse monitor with regular wave and failing life-device gauge suggests physical health differs from official allotment. Tight fixed oblique composition, large readable hands, components, contrasting gauge levels and ledger, lamp pooled warm light, cold neon leaking through shutter. No readable writing/numbers, no extra branching story, no death imagery.
```

### ART-DYS-004

- Status: candidate; version 1; creative acceptance pending.
- File: [windows-v1.png](windows-v1.png).
- Observed generation start: 2026-10-29T18:22:43.395Z; completion: 2026-10-29T18:26:06.049Z.
- Original output: `/home/sauce/.codex/generated_images/01a188e4-bb81-7551-878d-7417ada278a3/exec-c01e3c92-a5c9-411a-8de8-06ef7d46bfe7.png`.
- Output dimensions: 1672 × 941 pixels (PNG).
- Intended reading: wet marks, wiped glass and an approaching inspection drone surround the suspended platform.
- Limits: a still image communicates the proposal, not validated controls, timing, fairness or implemented rules. Abstract marks stand in for future legible records; visible details are not adopted canon.

Exact prompt:

```text
Use case: stylized-concept. Asset type: new landscape game concept illustration for an offline proposal exhibit, not a playable screenshot.
Style: richly atmospheric neon dystopian pixel art, coherent crisp pixel clusters and hard pixel edges, detailed worn industrial surfaces, dark indigo and desaturated teal base, restrained cyan and magenta lights, warm amber active work surfaces, hazy city depth. Fixed side/oblique scene, readable interaction surfaces, thoughtful composition.
Two protagonists must be recognizable: (1) short expressive yellow-skinned field worker with messy dark-green hair, big round white eyes, broad face, magenta ear studs, pale low work cap, cream work clothes adapted to profession; worried determined expression, never a chef hat or frying pan. (2) short green-skinned inspector with angular tall head, two metal head bolts, visible dark red X mark over one eye, dark suit with magenta lapels/tie, serious face. Keep both faces readable and large enough to recognize. These identities and palette were described from reference sheets inspected by the author; no input image is being edited.
Landscape 16:9-ish composition. No written text, letters, numbers, captions, logos, watermark or floating game HUD. Mechanical gauges may show ticks and colored light without characters. No gore, combat, explosions or jumping. Make a new independent illustration. Primary request: window-cleaning worker and missing-person archivist on a suspended highrise service platform against a huge glass/concrete apartment facade. Yellow worker with green hair, low pale cap, cream utility overalls and safety harness holds spray bottle and long squeegee; water sprayed on a dark dirty pane briefly reveals luminous cyan handprints and simple scratched human silhouettes beneath grime, not readable words. Next pane has a sharply wiped clean strip erasing part of a trace. Green suited bolt-headed X-eye archivist secured beside worker records the erased shape on a clipboard, small magnifier at belt. Platform lift controls and taut suspension cables clearly visible. A small inspection drone approaches at upper right, red scan light grazing the glass; moving platform can cover the uncleared clue. Wide fixed facade view, city haze far below with magenta reflections; focus on wet/reveal/clean choice, precarious work rather than platform jumping. Both characters foreground at readable scale.
```
