Provenance and exact generation inputs for the ending stills used by issues #270 and #304.

# Ending image provenance

- Generator: Codex built-in image generation tool, 2026-11-18; model, seed and quality setting not exposed. Opaque output (`transparent_background: false`).
- Direction: Toxic Night side-view pixel art; stepped outlines, fine pixel clusters and flat pixel shading, with near-black blue-violet shadows, acid-green and pink neon. No painterly or soft rendering.
- Style references: all three calls supplied `F-street.png`, `F-building.png` and `F-base.png` from the temporary `.pm-ref/` directory as `referenced_image_paths`, in that order. They were style references only, not edit targets. These mockups are recorded at `docs/art/concept-art/toxic-night/f-street.png`, `f-building.png` and `f-base.png` in art-direction commit `35a53ee444965e50eba276c823cbf8273ba47d53`. The temporary copies were removed after generation.
- Prompts: each adjacent `*-prompt.txt` contains the exact prompt submitted for that image; one built-in call per ending, with the same three references. No CLI/API fallback or postprocessing.
- Sources: the three PNGs beside this file are unmodified generated originals. Runtime files in `project/levels/monster_mouth_escape/art_hd/endings/` are byte-identical copies with the same basenames.
- Dimensions: 1672 by 941 pixels, preserving the existing approximately 16:9 size. The lower quarter is reserved as an empty dark band for the game's caption panel.
- Characters: none; no character sprites were supplied.
- Visual inspection: all three generated results were viewed before selection for hard stepped contours, pixel clusters, discrete shading and an empty caption band. These are artwork inspections, not runtime screen captures; in-game display verification remains with the PM.

| Outcome | Source and runtime basename | SHA-256 |
|---|---|---|
| satisfied | `satisfied.png` | `fdbbbbc38874287ff3fbfb0548bfc0694fc46b04f6a90dfc9c560409a763ecb9` |
| vomited_out | `vomited_out.png` | `e3d6358b7293c7c39f772a9ac46109d4a49e07ece09829886c8cb02364f17361` |
| acid_failure | `acid_failure.png` | `d1bc82748f0485e597f9caddf584b05046386521f8d59cf7176836834cda403c` |

## Escape ending

- `escaped.png` was generated on 2026-11-21 with Codex's built-in ImageGen tool, opaque output. Model, seed and quality were not exposed; no paid credits or external upload were used.
- References, in order: runtime `endings/satisfied.png`, `endings/vomited_out.png`, `endings/acid_failure.png` from `053e0dfb96409f85aa23f92d19425d6f5ccba909`, as style/framing references. [Exact prompt](escaped-prompt.txt).
- The source here and runtime `art_hd/endings/escaped.png` are byte-identical generated originals, 1672×941, with the same empty dark lower quarter. `tools/art/toxic_night.py` copies it and records source/output digests in the HD manifest.
- The chef pulls the appraiser through the battered escape device on an aquarium roof route. They are drawn into this scene, with no pasted runtime sprites, text or HUD. The illustration was visually inspected; runtime selection and creative acceptance remain with the engine worker and PM.
