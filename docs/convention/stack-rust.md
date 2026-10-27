# Rust stack

The Rust form of package tooling, check commands, types, unsafe boundaries, comments, and test placement.

## Package tooling

- Use Cargo. With the first Rust package, commit `Cargo.toml`, its generated `Cargo.lock`, and `rust-toolchain.toml` specifying the selected release and the `rustfmt` and `clippy` components. Record the package's `edition` and `rust-version` explicitly; choose their values when implementation begins.
- Update `Cargo.lock` through Cargo. Build, lint, and test with `--locked`; exclude `target/` from version control.

Toolchain-file syntax was checked against the [rustup overrides guide](https://github.com/rust-lang/rustup/blob/6c588a11329dff0bea76ebf79307aec4f1f57b87/doc/user-guide/src/overrides.md); lockfile behavior against the [Cargo guide](https://github.com/rust-lang/cargo/blob/749d85d9fc53250737d2d8ba0d431a00c44982bb/doc/book/src/guide/cargo-toml-vs-cargo-lock.md).

## Entry points

The root Cargo workspace contains the ascent engine and Godot adapter packages. The Make entry points run the following commands from that workspace root; build the Rust release library before Godot import, and retain the host integration checks alongside the Rust tests.

| Name | Rust command |
|---|---|
| `make format` | `cargo fmt --all -- --check` |
| `make lint` | `cargo clippy --workspace --all-targets --locked -- -D warnings`, alongside the document checks |
| `make test` | `cargo test --workspace --locked`, including library documentation tests |
| `make build` | `cargo build --workspace --release --locked` |

Apply formatting with `cargo fmt --all`. Add explicit feature and target combinations when implementation requires them; do not assume every feature can be enabled together.

Command usage was checked against [rustfmt](https://github.com/rust-lang/rustfmt/blob/ad7516ba22245c40faeae4668cc75d89f2550b92/README.md), [Clippy](https://github.com/rust-lang/rust-clippy/blob/b5cede0349356d8a70185fa3bb86d77093c29fea/README.md), and [Cargo's test reference](https://github.com/rust-lang/cargo/blob/749d85d9fc53250737d2d8ba0d431a00c44982bb/doc/book/src/commands/cargo-test.md), and the installed Cargo 1.99.0 command help. This records documentation checks, not a selected project toolchain.

## Types and unsafe boundaries

- Use `Option<T>` for absence, `Result<T, E>` and `?` for fallible operations, enums for state variants, and newtype structs for domain primitives.
- Keep engine and foreign-function `unsafe` operations inside small blocks behind a safe Rust API. State the caller's obligations in an unsafe function's rustdoc `# Safety` section; explain why an unsafe operation satisfies its obligations in an adjacent `// SAFETY:` comment.
- Resolve domain module visibility through explicit exports: `pub` for the declared public surface and narrower visibility for internals. The domain-to-package mapping is selected with implementation, not prescribed by a crate per future domain.

Unsafe encapsulation was checked against [The Rust Programming Language, Unsafe Rust](https://github.com/rust-lang/book/blob/1500248d8f230566e4ec9f27fcbb8fe9e2898ab1/src/ch20-01-unsafe-rust.md).

## Comments and tests

- Item documentation uses `///`; crate and module documentation uses `//!`; ordinary comments use `//`.
- Unit tests use a co-located `#[cfg(test)] mod tests` with `#[test]` functions. Library integration tests live in the package's `tests/` directory. Executable documentation examples run as rustdoc tests.
- End-to-end tests live under repository-root `e2e/`; select their runner with the first runnable application. Library integration tests in `tests/` do not establish that the shipped Godot and Rust application works together.

Placement was checked against [The Rust Programming Language, Test Organization](https://github.com/rust-lang/book/blob/1500248d8f230566e4ec9f27fcbb8fe9e2898ab1/src/ch11-03-test-organization.md).
