# neocity

A Godot game project in preparation, with contribution conventions and documentation checks in place.

## Project status

- Godot is the intended game engine; no engine version is pinned yet.
- Genre, gameplay, 2D or 3D, target platforms, scripting language, and development tools are undecided.
- The repository has no Godot project or playable build yet, so there is no game run command.
- System structure remains undecided; the [architecture index](docs/architecture/README.md) records the current documentation state.

## Checks

Run the documentation checks locally with Git, Make, and a POSIX shell with `awk` available:

```sh
make ci
```

`make lint` runs the same documentation checks. The [CI workflow](.github/workflows/ci.yml) invokes `make ci` for pull requests and pushes to `main`. Game formatting, tests, and builds have no commands yet.

[Dependabot](.github/dependabot.yml) proposes weekly updates to GitHub Actions dependencies. @garamsh maintains the hosted runner selection in the CI workflow; inspect the resolved runner image in the run's job setup log when investigating environment changes.

## Contributing

Read [AGENTS.md](AGENTS.md), the [conventions index](docs/convention/README.md), and the [architecture index](docs/architecture/README.md) before contributing. Changes land through pull requests using the [PR template](.github/PULL_REQUEST_TEMPLATE.md).
