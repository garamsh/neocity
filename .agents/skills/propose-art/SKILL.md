---
name: propose-art
description: Create or revise game art proposals, concept images, mockups, and issue-scoped visual implementation such as scenes, shaders, animation, UI presentation, or C++ visualization. Use for an art deliverable, not general engine or story work.
---
# Propose art

Turn an art request into visual evidence and a reviewable repository change, including visual code when the accepted task calls for it.

## Establish the task

Read repository-root `AGENTS.md`, `docs/convention/README.md`, `docs/collaboration.md`, and the linked issue with its comments. Read `docs/art/README.md` and open the relevant images themselves; use the image's stated interpretation rather than treating every visible detail as approved.

Identify whether the request is a candidate proposal or implementation of an accepted direction. Use the existing proposal/task templates. If there is no issue, prepare a scoped proposal and file it when the user's request authorizes publication; otherwise return the proposal for triage. Before repository implementation, obtain the PM-recorded task brief described in `docs/collaboration.md`. Do not ask again for scope or publication already authorized by that brief and the current request.

Derive allowed paths, visible acceptance criteria, the creative decision-maker, and required checks from that task. Visual work can include Godot and C++ code; inspect the relevant files and governing stack conventions before editing them. A requirement outside the accepted scope goes back to the PM for coordination, while independent in-scope work can continue.

## Produce the deliverable

For concept work, use the available image-generation or editing tool when requested and iterate against the brief. For a supplied reference, inspect it before editing. Do not generate images for a code-only visual task unless they serve its requested evidence. If generation is unavailable, report that limitation rather than substituting an unrequested implementation or claiming an image exists.

Store design images and their interpretation together under `docs/art/`, following its README's metadata and status rules. Update the existing topic rather than creating another art-direction document. New directions stay candidates unless the named decision-maker has approved them; link the actual approval when applying one.

Implement only the task's visual behavior and allowed code paths. Keep production assets with their implementation owner and link them to the design reference. A local shader, scene, animation, UI, or C++ visualization change is permitted when scoped; changing shared rendering infrastructure, engine interfaces, gameplay, or narrative meaning requires a coordinated task scope. Do not choose an engine binding or create a speculative runtime merely to display a concept image.

## Verify and deliver

Inspect the complete branch diff against its actual PR base, including new files and both sides of renames, and compare changed paths to the accepted task. Preserve unrelated work. If unexpected or out-of-scope changes exist, report them and do not stage or publish them; do not reset or delete another contributor's files. Stage this task's files explicitly.

Run `make ci` plus the available checks for the code changed. For visual code, capture the requested before/after or interaction evidence and record the scene, configuration, and conditions it represents. If the runtime or runner is unavailable, state what was not verified. Do not treat an image preview or resource import as proof of runtime behavior.

When PR publication is authorized, push the task branch and open or update one PR using `.github/PULL_REQUEST_TEMPLATE.md`, linking the issue and evidence within its fields. Read the PR back to confirm its head, changed files, and checks. Report the result and remaining decisions. Do not merge the PR or change repository settings, guidance, skills, or workflow configuration as part of an art task.
