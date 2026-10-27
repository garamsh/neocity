# Art direction

The proposed visual direction, source images, and interpretation of neocity's art references.

## Current direction

This is a documentation-only candidate assembled from the art designer's supplied images and instructions on 2026-10-27, against repository commit `b6e54685a932bc8bb9b750f098646943c9b2199a`. The requested visual baseline below is recorded separately from details merely visible in a mockup. Formal creative acceptance and the implementation task remain to be recorded on [issue #1](https://github.com/garamsh/neocity/issues/1); no image is labeled an approved production target.

| Topic | Art designer's requested direction | Still open |
|---|---|---|
| Presentation | A 2D game with spatial depth rather than an entirely flat presentation | Rendering technique, projection, production resolution, pixel grid, and asset dimensions |
| Screen behavior | Dynamic screen/composition transitions while retaining the supplied visual character | Transition triggers, camera movement, timing, and which scenes receive special framing |
| Tone | Dark, unsettling, and visually rich; both protagonists should carry a darker character impression | Specific world history, plot, horror intensity, and final expression selection |
| Protagonists | The supplied yellow chef and green detective form the character reference baseline | Final turnaround, animation inventory, combat behavior, and controls |
| Environments | Use the underground mockup, night-market mockup, and city board as visual evidence | Final map layout, location names, and the set of playable locations |
| Screen candidates | The board's Battle / Challenge panel is an entrance-screen candidate; its bottom Same city panorama is an ending-credits background candidate | Meaning of the entrance, final artwork, text placement, duration, and animation |

The source sheets contain cheerful and comic expressions alongside darker ones. Their inclusion records design vocabulary; the requested darker tone governs subsequent expression studies. Warm lamps or colorful signage can appear as local accents within an oppressive setting.

No story premise, game title, cooking system, investigation system, two-character control scheme, combat mechanic, quest, or dialogue is established by this art document. Text and interface elements drawn inside the images are reference content only.

## Character treatment

| Character | Preserve in visual studies | Develop or validate |
|---|---|---|
| Chef, ART-CHAR-001 | Yellow skin, dark green hair, tall white hat, white double-breasted coat, magenta ear accents, dark lower body, pan silhouette, exaggerated facial shapes | Cold or unsettling expressions rather than a default welcoming smile; pose and hat silhouette at gameplay scale; exact costume wear and staining |
| Detective, ART-CHAR-002 | Green skin, angular head silhouette, two metal bolts, X-shaped eye mark, large magnifying glass, dark suit, red/magenta accents | Suspicious or severe expression, readable glass and eye mark, separation from cool dark scenery; exact species and meaning of visible marks remain narrative decisions |

- Keep both characters recognizable in the same scene and under changing light. Their yellow/green identities and pan/glass silhouettes should survive a reduced-size review.
- Treat character sheets as concept sheets, not production-ready animation atlases. Pose rows do not establish frame counts, timing, direction coverage, or an animation pipeline.
- Keep line weight, pixel treatment, and apparent character scale coherent within a scene. Final pixel density and sprite proportions require an explicit visual test.
- Test the chef's white costume against bright lamps and the detective's black suit against dark machinery; distinguish them through local values, edge light, and background placement.

## Camera and transitions

The visual objective is depth within 2D gameplay. The references demonstrate elevated oblique views, layered foregrounds, visible floor and wall faces, occlusion, stairs, and localized light. They do not select an engine camera or require actual 3D geometry.

| Composition study | Reference | What to carry forward | Undecided behavior |
|---|---|---|---|
| Playable street | ART-MAP-002 | Readable ground plane, oblique building faces, cutaway interior, foreground framing, light reflected on wet surfaces | Traversal rules, camera tracking, room boundaries, and interaction controls |
| Deep interior | ART-MAP-001 | Elevated walkway over a visibly lower basin, stairs, vertical tanks, pillars, layered railings, foreground pipes | Exact map geometry, traversable levels, camera angle, and visibility rules |
| Ordinary versus enhanced depth | ART-STUDY-002 | Compare restrained depth with stronger foreground layers, light, vapor, and reflections | Whether there are separate scene modes, how frequently they occur, and how they transition |
| Entrance artwork | ART-BOARD-001, Battle / Challenge panel | Aggressive diagonal action, compressed silhouettes, crowded hostile shapes, high visual tension | Whether this opens the game, a chapter, a map, or an encounter; no battle system is inferred |
| Credits artwork | ART-BOARD-001, bottom Same city panorama | Wide skyline, two protagonists seen from behind, distant city layers, broad sky | Credits typography, layout, scrolling direction, crop, and motion |

Candidate transition studies for later comparison, not accepted camera behavior:

- Move from a readable gameplay frame to a wider view that reveals the scale of the location, then return without losing the protagonists' positions.
- Change depth emphasis through foreground movement, layered background movement, lighting, or framing. Compare these approaches before specifying one implementation.
- Use the entrance panel's compressed energy and the credits panorama's distant stillness as different screen rhythms within the same art language.

Acceptance of a later transition study should require visible spatial continuity, readable characters and paths during motion, and a settled frame whose pixel treatment matches the surrounding gameplay. The supplied still images cannot verify animation, parallax, a camera system, or runtime performance.

## Environment and light

| Element | Reference reading | Proposed use |
|---|---|---|
| Dark base | Indigo, charcoal, desaturated teal, deep violet | Establish silhouettes and atmosphere without obscuring the playable ground |
| Local accents | Cyan glass/forensic light, magenta signage, red warning lamps, amber work lights | Direct attention and distinguish places; avoid giving every surface equal brightness |
| Materials | Wet paving, rusted metal, stained tile, worn concrete, old equipment | Suggest age, density, and neglect through surface variation and selective highlights |
| Depth cues | Thick platforms, visible wall faces, foreground occlusion, stairs, distant architecture | Make vertical distance and overlapping spaces legible in a 2D composition |
| Atmosphere | Vapor, shadow, reflected colored light, distant haze | Emphasize selected layers while keeping active characters and interactions clear |
| Unease | Biological silhouettes in industrial vessels, cramped passages, concealed interiors | Explore a darker mood; the contents of the vessels and their story meaning remain undecided |

The city board includes brighter and more playful panels. Those broaden the reference vocabulary but do not override the art designer's darker tone. Labels such as Dream / Weird world, Kitchen, and Hideout do not commit the project to those locations, mechanics, or narrative functions.

## Reference gallery

Original files are copied byte-for-byte with stable repository names. Version `v1` means the first imported source version, not a production revision. Provenance identifies what is known; supplying an image does not establish its creator, rights, model, or production readiness.

### ART-CHAR-001: Chef design

![Yellow chef concept sheet with white hat, green hair, expressions, pan, and pose studies](concept-art/characters/chef-sheet-v1.png)

| Field | Record |
|---|---|
| Status | Candidate; v1 |
| Intent | Establish the chef's identity, palette relationships, silhouette, props, and range of facial shapes |
| Adopted elements | Requested character baseline: yellow skin, green hair, white hat/coat, magenta accents, pan; final production target pending |
| Open questions | Final dark expression set, proportions, costume condition, playable directions, and animation frames; sheet smiles are not the default mood requirement |
| Source | Art designer supplied `레트로 픽셀 셰프 캐릭터 시트.png`; original creator and generation/editing history not supplied |
| Decision record | Selected as an art reference in the 2026-10-27 request; issue #1 is the tracking context, with formal image acceptance not recorded |

### ART-CHAR-002: Detective design

![Green detective concept sheet with head bolts, X eye mark, magnifying glass, suit, expressions, and pose studies](concept-art/characters/detective-sheet-v1.png)

| Field | Record |
|---|---|
| Status | Candidate; v1 |
| Intent | Establish the detective's silhouette, face marks, costume, glass, and expression vocabulary |
| Adopted elements | Requested character baseline: green skin, bolts, X eye mark, oversized glass, dark suit and red/magenta accents; final production target pending |
| Open questions | Final dark expression set, head proportions, eye/glass treatment, directions, and animation; origin and anatomy are not established |
| Source | Art designer supplied `픽셀 아트 외계인 탐정 캐릭터 시트.png`; original creator and generation/editing history not supplied |
| Decision record | Selected as an art reference in the 2026-10-27 request; formal acceptance remains to be recorded on issue #1 |

### ART-MAP-001: Underground depth study

![Oblique pixel-art refinery mockup with chef and detective on a raised walkway above a flooded basin and tall illuminated vessels](concept-art/gameplay/underground-refinery-v1.png)

| Field | Record |
|---|---|
| Status | Candidate; v1; concept mockup, not a running game |
| Intent | Demonstrate the requested darker tone and pronounced depth in a 2D gameplay composition |
| Adopted elements | Requested visual direction: oppressive interior, elevated oblique framing, vertical separation, industrial layering, localized cyan/red light, darker protagonist treatment |
| Open questions | Exact map, tank contents, names, plot, blood-like marks, HUD, health system, item slots, quest text, and input prompts; warm work light is an accent rather than a tone requirement |
| Source | Art designer supplied `ChatGPT 이미지 2026년 10월 27일 오후 03_20_48.png`, identified in this discussion as the earlier assistant-generated underground concept; built-in ImageGen, model not reported, opaque reference-guided output. Available generation prompt is preserved in [the source note](concept-art/gameplay/underground-refinery-prompt.txt). No fixed size, seed, or quality setting was exposed in that call |
| Decision record | Included in the designer's selected art set on 2026-10-27; embedded memory-food premise and ledger objective remain unapproved assistant suggestions |

### ART-MAP-002: Night-market composition

![Oblique pixel-art night market with neon signs, wet paving, a cutaway kitchen, foreground rooftops, and both protagonists](concept-art/gameplay/night-market-v1.png)

| Field | Record |
|---|---|
| Status | Candidate; v1; concept mockup, not a running game |
| Intent | Demonstrate layered urban composition, environmental density, localized light, and character readability |
| Adopted elements | Requested aesthetic reference: neon over a dark base, wet reflections, overlapping architecture, oblique ground view, interior/exterior layering |
| Open questions | Actual map, NPC species, readable sign text, quests, UI layout, hearts, selected portraits, footprints, E/Tab controls, food/evidence systems, and the chef's cheerful expression |
| Source | Art designer supplied `eb9c1f90-f5ed-448c-bbfc-99858a15d19a.png`; previously presented as a generated gameplay reference, but generator, model, original prompt, and settings are not available in the supplied source |
| Decision record | Included in the designer's selected art set on 2026-10-27; this is a gameplay-composition reference, not the entrance-screen panel |

### ART-BOARD-001: City and screen candidates

![Composite pixel-art board showing the two protagonists, city skyline, kitchen, alley, underground, surreal space, market, hideout, action panel, and bottom skyline panorama](concept-art/environments/city-moodboard-v1.png)

| Field | Record |
|---|---|
| Status | Candidate; v1; multi-panel concept board |
| Intent | Collect location moods and distinguish gameplay scenes from entrance and credits artwork candidates |
| Adopted elements | Requested aesthetic reference: neon city layers, strong protagonist silhouettes, atmospheric interiors; designer specifically nominated the Battle / Challenge panel for entrance imagery and the bottom Same city panorama for an ending-credits background |
| Open questions | Final crops, redrawing, typography, motion, narrative meaning, actual locations, and entrance-screen context; board text and working title are not approved copy |
| Source | Art designer supplied `네온 도시의 요리사와 탐정들.png`; creator, generator, model, original prompt, and settings not supplied |
| Decision record | Art designer clarified the panel mapping in this discussion on 2026-10-27; formal creative acceptance remains to be recorded on issue #1 |

Panel interpretation:

- **Entrance candidate:** the Battle / Challenge panel at the right of the row immediately above the bottom panorama. It shows the chef lunging diagonally and the detective among hostile shapes. Use its composition and tension as entrance-art evidence; do not interpret the drawn combat as an accepted gameplay system.
- **Ending-credits candidate:** the full-width bottom strip marked Same city, different eyes, showing both protagonists from behind against a pink/violet skyline. The intended use is a background while credits roll. It does not establish the story ending or a warm reconciliation. The credits layout still needs a legibility test over the skyline.
- **Other panels:** environment and palette studies. No panel assigns a mandatory scene, camera mode, enemy roster, or gameplay activity.

### ART-STUDY-001: Earlier sprite exploration

![Earlier chef sprite study with facial expressions and several standing and walking poses](concept-art/characters/chef-sprite-study-v1.png)

| Field | Record |
|---|---|
| Status | Reference; v1; supporting earlier material, not an additional selected target |
| Intent | Reconcile the earlier chef sprite-sheet source requested by issue #1 and compare character readability |
| Adopted elements | None beyond the chef identity described under Character treatment |
| Open questions | Whether any pose or proportion is useful; cheerful expressions and apparent frame ordering are not adopted |
| Source | Supplied earlier in this discussion as `복고풍 픽셀 셰프 스프라이트 시트2.png`; creator, tool, model, prompt, and settings not supplied |
| Decision record | Included for source completeness under issue #1; not among the five images selected in the current request |

### ART-STUDY-002: Earlier depth comparison

![Stacked mockups of the same night-market scene, with flatter presentation above and stronger depth, foreground overlap, reflections, and localized light below](concept-art/gameplay/flat-depth-comparison-v1.png)

| Field | Record |
|---|---|
| Status | Reference; v1; supporting earlier material, not an approved mode specification |
| Intent | Reconcile the ordinary/special comparison requested by issue #1 and support a visible depth comparison |
| Adopted elements | None as a separate gameplay mode; use to compare depth and framing treatments under Camera and transitions |
| Open questions | Whether such modes exist, their frequency, how the camera moves between them, exact perspective, and runtime technique |
| Source | Supplied earlier in this discussion as `a8f4466a-6522-4a96-889a-b8dfb480f12b.png`; previously described as a generated comparison, but generator, model, prompt, and settings not available |
| Decision record | Included for source completeness under issue #1; not among the five images selected in the current request |

## Pending decisions

- Record the named creative decision-maker and formal scope/acceptance on issue #1 before turning this candidate into an approved production target. The current task records the art designer's intent without claiming PM approval.
- Specify which transitions should feel dynamic and compare motion studies. Do not infer continuous rotation, a switch between 2D and 3D, or a fixed ordinary/special ratio from these still images.
- Test final entrance artwork and ending-credits composition separately from gameplay. The board panels are references, not finished full-screen assets.
- Confirm source creators and generation history where unknown, and clear reuse before production release. Original supplied files are retained without invented attribution.
- Additional earlier illustrations/background references named generically in issue #1 are outside this selected set and have not been fully inventoried. Five currently selected images plus the earlier chef sprite study and flat/depth comparison are imported; the issue's full historical source inventory is not claimed complete.
- Gameplay, story, final HUD, camera implementation, target resolution, animation specifications, and runtime assets require separate decisions. Neither character's profession commits the game to cooking or detective mechanics.

## Using this material

Use the repository's [propose-art skill](../../.agents/skills/propose-art/SKILL.md) and [collaboration procedure](../collaboration.md) for the proposal-to-task handoff. This document is the single art-direction entry point; do not create a parallel art-direction document.

- Cite an image identifier, repository-relative path, and the specific interpreted elements in a later task. Example: ART-MAP-001, `docs/art/concept-art/gameplay/underground-refinery-v1.png`, elevated walkway and localized cyan lighting only.
- Read the relevant image and its metadata before implementation. Inclusion in the gallery does not approve every visible element.
- Keep design evidence here. Put eventual game-loaded assets with the implementation that owns them and link them to the reference; these images are not production sprite atlases or scene files.
- Update the requested direction and the relevant metadata together when a creative decision is accepted. Link the actual issue/PR decision and retain the distinction between source references, candidates, and approved targets.
- Verify documentation links and `make ci` for this change. Runtime camera motion, character readability in motion, imports, and performance remain unverified because this deliverable contains no playable implementation.
