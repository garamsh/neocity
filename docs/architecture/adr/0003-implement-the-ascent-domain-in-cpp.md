# ADR 0003: Implement the ascent domain in C++

> Status: accepted
> Date: 2026-10-28

Append-only: once merged, the body below is not rewritten. A fact later found wrong is corrected in an appended note, not edited out; a revised decision is a new ADR that supersedes this one.

## Context

On 2026-10-28 the requester replaced Rust with C++ for the custom engine and presentation code (issue #9). ADR 0002 recorded the engine/presentation boundary together with its language: two Cargo packages and godot-rust. The boundary stays; the language and toolchain change, so ADR 0002 is superseded rather than amended. ADR 0001's domain partitioning is unchanged, and no port interfaces are introduced: the engine makes no outbound call, so a port would hide nothing.

Alternatives considered:

- **SCons**, godot-cpp's primary build system. CMake is the documented, actively supported secondary system; it was preferred because the engine library, its unit tests, and pinned fetches of both dependencies are expressed in one build, and it exports the compile database the static analyser reads.
- **A Godot engine module** instead of a GDExtension: out of scope for issue #9.
- **A git submodule** for godot-cpp: a pinned FetchContent archive needs no extra checkout step for CI or a macOS player.
- **doctest or Catch2** for unit tests: GoogleTest was preferred for its documented FetchContent pattern and CMake's built-in test discovery for it.

## Decision

Keep ascent as one domain with two C++ targets: `ascent_engine`, a static library depending only on the C++ standard library, and `neocity_ascent`, a godot-cpp GDExtension library consuming the engine's public command/snapshot headers. Each selection and the source it was verified against:

| Selection | Source |
|---|---|
| C++20 for project targets | godot-cpp builds its own target as C++17 (`cmake/godotcpp.cmake` at the godot-cpp commit below); GoogleTest v1.18.0 requires at least C++17 (`docs/quickstart-cmake.md` at its commit below) |
| godot-cpp 10.0.0-stable, commit `507ed9d840c01a3c5b2a39af8bb4000bfac30bf5`, API version 4.7 | [godot-cpp README at that commit](https://github.com/godotengine/godot-cpp/blob/507ed9d840c01a3c5b2a39af8bb4000bfac30bf5/README.md): from 10.x godot-cpp is versioned independently and targets a Godot version through `api_version`. Its `gdextension/extension_api-4-7.json` equals the API dumped by Godot 4.7.2.stable.official.ed1daf0bf with `--dump-extension-api`, except for the version header |
| CMake 4.4.4 with Ninja 1.13.2 | [Godot 4.7 documentation, godot-cpp CMake build system](https://github.com/godotengine/godot-docs/blob/9adca4c1c72917bfe1b7be3108abed5ce26696a6/tutorials/scripting/cpp/build_system/cmake.rst) (branch `4.7` at that commit): CMake is supported and secondary to SCons |
| GoogleTest v1.18.0, commit `063de7e9578f82b369302001269680b4b1553359` | [GoogleTest CMake quickstart at that commit](https://github.com/google/googletest/blob/063de7e9578f82b369302001269680b4b1553359/docs/quickstart-cmake.md): FetchContent and `gtest_discover_tests` |
| clang-format 23.1.3, LLVM predefined style | PyPI release `clang-format` 23.1.3 |
| clang-tidy 23.1.3 | PyPI release `clang-tidy` 23.1.3 |
| Python 3.13 in CI through `actions/setup-python` v7.1.0 | the action's README and `action.yml` at tag v7.1.0 |

godot-cpp and GoogleTest are fetched as GitHub commit archives pinned by SHA-256. CMake, Ninja, clang-format, and clang-tidy are installed from PyPI wheels pinned by SHA-256 per supported platform into the ignored project-local `.tools/cpp`; the clang wheels are third-party packaging of the LLVM release named by their version. The GitHub Advisory Database returned no advisory for any of these packages or actions on 2026-10-28.

## Consequences

- Engine tests run with no Godot, window, or godot-cpp header: the engine target's only include directory is its own `include/`, so an engine source that includes a Godot header fails to compile.
- The engine keeps Rust's single-precision arithmetic only if every literal is a `float`; `-Wconversion` and `-Wdouble-promotion` with warnings as errors reject an implicit promotion.
- godot-cpp's CMake path may lack a feature its SCons path has. The bindings are generated only for the classes the adapter uses, listed in a godot-cpp build profile.
- Formatting and lint results depend on the pinned LLVM tool versions, so a contributor uses the project-local tools rather than a system install.
- Rust's checked indexing becomes `at()`; an out-of-range access terminates the process instead of corrupting state.
- The toolchain pins are not covered by Dependabot and are moved by a named owner.

## Signs the decision was wrong

- An engine source needs a godot-cpp type, or the adapter starts reimplementing rules the engine owns.
- A needed godot-cpp feature exists only in its SCons build, and the CMake build grows workarounds for it.
- The PyPI tool wheels stop being published for a supported platform, or format and lint results differ between macOS and Linux with the same pins.
- Engine behaviour diverges from the course, timing, or recovery rules the Rust slice shipped, as observed by the ported tests or the live gameplay checks.
