# neocity

A playable Rust-driven neon-city jump-course prototype hosted by Godot.

## Play locally

On macOS, install the project-local tools once with `./scripts/setup-macos.sh`, then double-click `play.command` in Finder. The tools stay under ignored `.tools/`; the installer verifies the official Godot archive and does not modify global PATH or require an account. Xcode Command Line Tools must be present for native compilation.

`play.command` builds and imports the release library on the first run. For development, run `make run` to rebuild before playing. On other development systems, provide Rust 1.99.1 and standard Godot 4.7.2 on PATH, then use `make run`; those platforms are not locally verified here.

| Input | Action |
|---|---|
| A / D or left / right | Move |
| Space, W, or up | Jump; hold for a higher jump |
| Tab | Change the leading character |
| R | Retry at the reached checkpoint; after completion, begin a new run |
| Escape | Pause or continue |

Climb the maintenance platforms, avoid urban pests, reach the cyan checkpoint, and collect the glowing spice fruit at the top. There is one course, not the full campaign. The companion follows a delayed visual trace; the party has one collision body. This reversible prototype control choice is not the final two-character mechanic.

## Engine and screen ownership

- `ascent/engine` owns deterministic rules, movement, collisions, obstacles, checkpoints, and completion. It has no Godot or rendering dependencies and can be tested by itself.
- `ascent/godot` owns Godot input translation, camera, resources, canvas drawing, HUD, and the composition root. It depends only on the engine's public commands and value snapshots. Custom engine and screen orchestration are Rust; Godot still supplies window/input/canvas services. No standalone GPU backend is claimed.
- [Architecture](docs/architecture/README.md) records package boundaries. [Art direction](docs/art/README.md) identifies the selected concepts, production-source provenance, and remaining decisions.

## Checks and toolchains

```sh
make ci
```

- `make lint`: repository documentation checks and Cargo Clippy with warnings denied.
- `make format`: Rust formatting check.
- `make build`: locked Rust release build, pinned Godot version, and resource import.
- `make test`: locked workspace tests, configured main-scene readiness, and live Godot move/jump/retry input checks.
- `make ci`: the complete set. Engine-only checks can run with `cargo test -p neocity-ascent-engine --locked`, without Godot.

Rust 1.99.1, godot-rust 0.5.5 targeting the Godot 4.7 API, and standard Godot 4.7.2 are selected for this slice. The native binding follows the [godot-rust setup](https://godot-rust.github.io/book/intro/hello-world.html). The development build disables debug symbols and incremental artifacts to limit disk usage. `neocity.gdextension` selects the release library for both editor and game; build before importing a fresh checkout.

CI uses the pinned Rust setup action and the checksum-verified Linux Godot helper, then calls the same `make ci`. Dependabot covers GitHub Actions; @sauce-git maintains Godot pins/runner selection, and @smonidema reviews Rust toolchain/binding updates. No platform export or audio pipeline is selected.

## Contributing

Read [AGENTS.md](AGENTS.md), [conventions](docs/convention/README.md), [architecture](docs/architecture/README.md), and [collaboration](docs/collaboration.md) before changing responsibilities. Changes land through the existing PR template. Use the repository's propose-art skill for scoped visual work; final art/gameplay acceptance remains separate from engine tests.
