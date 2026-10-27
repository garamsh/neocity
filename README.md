# neocity

A minimal Godot host for a game project with Rust planned for custom engine and visualization work.

## Project status

- The host uses the standard [Godot 4.7.2 stable release](https://godotengine.org/download/archive/4.7.2-stable/). Custom engine and visual systems will be authored in Rust; the Rust toolchain, Godot integration, graphics backend, and concrete subsystem boundaries remain undecided. This bootstrap supplies the host only, with no Rust implementation.
- The first-release concept is a small single-player 2D jump-map adventure; its selected art direction and open design details are recorded in [art direction and images](docs/art/README.md). Target platforms and production gameplay scripting remain undecided. GDScript is used only by the native startup check.
- The main scene is an empty `Node`; running it opens a blank window. There is no playable game yet.
- The domain-oriented structure is recorded in the [architecture documents](docs/architecture/README.md). Game domains have not been identified yet.

## Running locally

Install the standard Godot 4.7.2 editor and make its executable available as `godot` on `PATH`. `godot --version` must match `GODOT_VERSION` in the [Makefile](Makefile). On macOS the executable is inside `Godot.app/Contents/MacOS/Godot`.

From the repository root:

```sh
godot --editor --path .
godot --path .
```

The first command opens the editor; the second starts the configured main scene. Close the window to stop it.

## Checks

With Godot, Git, Make, and a POSIX shell with `awk` available:

```sh
make ci
```

- `make lint` runs the document checks.
- `make build` verifies the engine version and imports the project headlessly.
- `make test` imports first, parses the smoke harness, and checks that the configured main scene becomes ready. It exits after that check and reports the scene path.
- `make ci` runs the document, import, and startup checks in order. Set `GODOT=/path/to/godot` when invoking Make if the executable is elsewhere.

The [CI workflow](.github/workflows/ci.yml) invokes `make ci` for pull requests and pushes to `main`. Its [Linux setup helper](.github/scripts/setup-godot.sh) verifies the editor archive against the repository-pinned checksum from the official release before extracting it. No export targets, gameplay tests, coverage tool, or formatter have been selected.

[Dependabot](.github/dependabot.yml) proposes weekly updates to GitHub Actions dependencies. @sauce-git maintains the Godot version and checksum pins and the hosted runner selection; inspect the resolved runner image in the run's job setup log when investigating environment changes.

## Contributing

Read [AGENTS.md](AGENTS.md), the [conventions index](docs/convention/README.md), and the [architecture index](docs/architecture/README.md) before contributing. Changes land through pull requests using the [PR template](.github/PULL_REQUEST_TEMPLATE.md).

Read [collaboration](docs/collaboration.md) for issue ownership, art/story/engine specialties, and the proposal-to-PR workflow.

For visual work, use the repository's `$propose-art` skill and read the [art direction and images](docs/art/README.md). Concept art, references, and mockups are documented together there.
