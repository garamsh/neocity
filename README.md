# neocity

A game project in preparation using Godot together with Rust, with contribution conventions and documentation checks in place.

## Project status

- Godot and Rust are the intended stack. Rust is planned for custom engine and visualization work; their integration mechanism and division of responsibilities remain undecided. Neither toolchain is pinned yet.
- Genre, gameplay, 2D or 3D, target platforms, Godot-side scripting language, and additional development tools are undecided.
- The repository has no Godot project or playable build yet, so there is no game run command.
- The domain-oriented structure is recorded in the [architecture documents](docs/architecture/README.md). Game domains have not been identified yet.

## Checks

Run the documentation checks locally with Git, Make, and a POSIX shell with `awk` available:

```sh
make ci
```

`make lint` runs the same documentation checks. The [CI workflow](.github/workflows/ci.yml) invokes `make ci` for pull requests and pushes to `main`. Game formatting, tests, and builds have no commands yet.

[Dependabot](.github/dependabot.yml) proposes weekly updates to GitHub Actions dependencies. @sauce-git maintains the hosted runner selection in the CI workflow; inspect the resolved runner image in the run's job setup log when investigating environment changes.

## Contributing

Read [AGENTS.md](AGENTS.md), the [conventions index](docs/convention/README.md), and the [architecture index](docs/architecture/README.md) before contributing. Changes land through pull requests using the [PR template](.github/PULL_REQUEST_TEMPLATE.md).

Read [collaboration](docs/collaboration.md) for issue ownership, art/story/engine specialties, and the proposal-to-PR workflow.

For visual work, use the repository's `$propose-art` skill and read the [art direction and images](docs/art/README.md). Concept art, references, and mockups are documented together there.
