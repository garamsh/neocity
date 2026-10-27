# Collaboration

How contributors turn proposals into scoped work and reviewed changes in this repository.

## Starting work

- Read [AGENTS.md](../AGENTS.md) for authority and specialty boundaries; a human and the AI acting for them use the same assignment.
- Use the existing [proposal template](../.github/ISSUE_TEMPLATE/proposal.md) for an art, story, game-direction, or technical proposal. State the requested decision, alternatives, affected paths, and evidence. An image or story draft remains a candidate until the issue's creative decision-maker accepts it.
- Before implementation, the PM scopes a [task](../.github/ISSUE_TEMPLATE/task.md) or records the equivalent task brief on the existing issue: one accountable owner, specialty, exact paths, constraints, acceptance criteria, and the decision-maker for creative work. Keep related proposal and implementation issues linked.
- An accepted art task may include visual code and assets together. Define the desired visible behavior and the allowed implementation paths; selecting the art specialty does not grant all code paths or restrict the worker to image files.
- If implementation needs a shared API, another owner's file, or a change to gameplay rules, record the dependency and let the PM coordinate scope before changing that part. Continue independent work already in scope. Do not split one feature into competing branches that edit the same files.

## Delivering a change

1. Work on the issue's branch or isolated checkout. A contributor's personal account and credentials identify their work; do not share the PM's account or credentials to submit a PR.
2. Read the conventions governing the actual changed files and the architecture documents when code or decisions change. Read [art direction](art/README.md) and open its relevant images before visual work. Keep story facts in their existing owning document or content source; introduce their first document with actual story content rather than an empty story hierarchy.
3. Keep current decisions in repository documents, source assets beside their documented previews, and runtime assets with their implementation owners. Use issue comments for discussion and link the accepted outcome from the PR; a chat or external board is not a second authoritative copy.
4. Run `make ci` and the checks the implemented behavior requires. For art code, include before/after images or a recording and relevant scene, interaction, or rendering checks; for story work, include the changed content and branch/data checks; for engine work, include correctness checks and repeatable measurements for performance claims. Mark unavailable game checks unverified rather than claiming they ran.
5. Open a PR linked to its issue using the repository template. Include the complete changed-path scope and relevant creative decision record within the template fields. The PM reviews and merges under the repository's review convention; creative feedback does not authorize a worker to merge.

## Shared tools and tracking

- Use repository-local skills for repeatable procedures. [propose-art](../.agents/skills/propose-art/SKILL.md) covers art candidates and issue-scoped visual implementation; invoke it with `$propose-art` in an environment that has loaded the repository skill. A skill provides instructions and grants no additional GitHub or filesystem permission.
- Start a contributor AI session with the authority role `Worker`, its specialty, the issue URL, and the requested deliverable. For art, invoke `$propose-art`; changing the session's specialty does not grant PM authority.
- Keep durable role authority in `AGENTS.md`, domain boundaries in the architecture documents, and the current visual direction in `docs/art/`. Skills read those sources rather than maintaining copies of their rules.
- Start with the existing proposal/task templates and issue assignees. If a board becomes useful, configure one GitHub Project with backlog, ready, in progress, review, and done states and specialty views. Record exact ownership and scope on the issue rather than relying on a board column or label.
- Create story or engine skills when their actual recurring workflow is known. Do not manufacture a skill, agent, or source-code folder for each anticipated specialty.
- For a web environment that has not loaded repository-local skills, provide the skill and repository guidance through its supported context/distribution mechanism and verify the tools and repository access. Do not assume a web chat has the same installed skills or credentials as a local Codex session.

## Access and review setup

This is a setup recommendation, not a claim that hosted permissions or checks are already enabled:

- For an organization repository, use teams to manage membership and choose repository access by need: Read for proposal-only contributors, Triage for backlog coordination, and Write for contributors who push branches. Keep administration with the repository administrators; a specialty name is not a GitHub access tier.
- Require PRs and the project's CI result before changes reach `main`; prohibit direct pushes and configure bypasses deliberately. Where authors and the reviewer use separate accounts, require the designated owner's review and dismiss stale approvals after relevant changes. Preserve the documented single-account review procedure where one account is used.
- Use CODEOWNERS to route review of real paths to their maintainers, including the guidance and skill files themselves. A code-owner entry is not a directory write restriction. Requiring multiple specialties' approval needs an explicit review policy; listing two owners on one pattern does not require both to approve.
- Keep a task's path allowance concrete. A future automated scope check must compare against the accepted task paths; it must not reject all code in an art PR. No scope check or branch-protection configuration is introduced by this documentation change.

The access recommendations were checked against GitHub's documentation at commit `9568e964d18d76267faf57050dc662c450a13214`: [repository roles](https://github.com/github/docs/blob/9568e964d18d76267faf57050dc662c450a13214/content/organizations/managing-user-access-to-your-organizations-repositories/managing-repository-roles/repository-roles-for-an-organization.md) and [code owners](https://github.com/github/docs/blob/9568e964d18d76267faf57050dc662c450a13214/content/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/about-code-owners.md). The branch/PR approach follows [GitHub flow](https://github.com/github/docs/blob/9568e964d18d76267faf57050dc662c450a13214/content/get-started/using-github/github-flow.md).
