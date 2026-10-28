# C++ stack

The C++ form of toolchain, check commands, types, ownership, lint suppressions, comments, and test placement.

## Toolchain

- Project targets build as C++20 with compiler extensions off. Fetched dependencies keep the standard and flags their own build declares.
- CMake with the Ninja generator. The root `CMakeLists.txt` adds one subdirectory per package; each package's `CMakeLists.txt` declares its targets and fetched dependencies.
- Every fetched source is a commit archive in `FetchContent_Declare` with `URL_HASH SHA256=`. CMake, Ninja, clang-format and clang-tidy are PyPI wheels listed in `Makefile` with one `--hash=sha256:` per supported platform, installed with `--require-hashes` into ignored `.tools/cpp`. A pin's comment names who moves it.
- Project targets compile with `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wdouble-promotion -Wshadow`, with `COMPILE_WARNING_AS_ERROR` on.

Checked against godot-cpp's CMake documentation in [godot-docs at `9adca4c1c72917bfe1b7be3108abed5ce26696a6`](https://github.com/godotengine/godot-docs/blob/9adca4c1c72917bfe1b7be3108abed5ce26696a6/tutorials/scripting/cpp/build_system/cmake.rst) and the [GoogleTest CMake quickstart at `063de7e9578f82b369302001269680b4b1553359`](https://github.com/google/googletest/blob/063de7e9578f82b369302001269680b4b1553359/docs/quickstart-cmake.md).

## Entry points

| Name | C++ command |
|---|---|
| `make format` | `clang-format --style=LLVM --dry-run --Werror` over every tracked or new `ascent/*.cpp` and `ascent/*.hpp` |
| `make lint` | `clang-tidy -p build --warnings-as-errors='*'` with the check list in `Makefile` over every `ascent/*.cpp`, after configure and `generate_bindings` |
| `make build` | `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`, then `cmake --build build` |
| `make test` | `ctest --test-dir build --output-on-failure --no-tests=error`, alongside the Godot host checks |

Apply formatting with `clang-format --style=LLVM -i`. A check disabled in the clang-tidy list is disabled for the whole project and named in that list; it is not disabled per file.

## Types and ownership

- `std::optional<T>` for absence; `enum class` for variants, matched by a `switch` with no `default` and followed by `std::abort()` so `-Wswitch` reports a missing case.
- Index runtime-sized containers with `.at()`.
- Hold values by value or `std::unique_ptr`; no owning raw pointer and no `new` outside a framework macro that requires it.
- A public API lives under the package's `include/<package>/`; everything under `src/` is internal to the package.

## Lint suppressions

A `NOLINTNEXTLINE(<check>)` is allowed only for a diagnostic raised inside a third-party macro expansion, such as godot-cpp's `GDCLASS`. It names each check it suppresses, and a comment on the line above names the macro. No suppression covers project code.

## Comments and tests

- `///` for documentation comments on declarations; `//` for ordinary comments.
- Headers use `#pragma once`; source files use `.cpp`, headers `.hpp`.
- Unit tests use GoogleTest, co-located as `src/<unit>_test.cpp`, linked into one test executable per package and registered with `gtest_discover_tests`. Test names read `Suite.SentenceInPascalCase`.
- Integration and end-to-end tests of the shipped application stay in the Godot harnesses under repository-root `tests/` and `e2e/`.
