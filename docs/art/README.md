# Art direction

The project's visual direction and the concept images, references, and mockups that explain it.

## Current direction

No visual style or target image has been selected yet. Genre, dimensionality, camera, palette, character design, environment design, and UI appearance remain open.

Keep the current direction in this document. Add a separate topic document only when a distinct subject needs one, move that subject there rather than copying it, and link it from here. Do not create a parallel `docs/art-direction.md` or repeat visual decisions in the root README.

## Images and context

Store concept art, reference images, and screen mockups beside this document under `docs/art/`. Add subdirectories when actual material needs grouping. Embed each image with a relative Markdown path and descriptive alternative text, placing its interpretation beside it.

Record the following with each image:

| Field | Content |
|---|---|
| Status | Reference, candidate, or approved target; references and candidates do not establish the project's style |
| Intent | What the image explores or demonstrates |
| Adopted elements | The specific colors, forms, lighting, composition, or other details agreed for use; leave unselected elements open |
| Open questions | Which visible details have not been decided |
| Source | Creator and original source; for generated work, the tool and available model, prompt, and settings needed to understand its origin |
| Decision record | Who approved a target and the linked issue or PR recording the decision, when available |

No concept images or mockups have been added yet. Add real material rather than placeholder images or an empty gallery.

## Using this material

Use the repository's [propose-art skill](../../.agents/skills/propose-art/SKILL.md) for art proposals and scoped visual implementation; the [collaboration procedure](../collaboration.md) covers ownership and handoffs. Art work may include code when the task scopes it.

Before visual work, read the current direction and open the relevant images themselves. An image's caption identifies which elements apply; do not infer approval of every detail from its inclusion. Proposed changes remain candidates until approved, and an accepted change updates the current direction and its images together in a pull request.

This directory holds design evidence and readable previews. Assets loaded by the game belong with the implementation that owns them; link between a design image and its production asset rather than maintaining two authoritative copies. If an editable source cannot be previewed directly, place a preview beside it and identify their relationship.

Record engine and rendering architecture in the architecture documents; link those decisions here when they affect appearance. Art approval does not silently choose a rendering implementation.
