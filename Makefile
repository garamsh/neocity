GODOT ?= $(if $(wildcard .tools/Godot.app/Contents/MacOS/Godot),$(CURDIR)/.tools/Godot.app/Contents/MacOS/Godot,godot)
GODOT_VERSION := 4.7.2.stable.official.ed1daf0bf
PYTHON ?= python3

# C++ tools are hash-pinned PyPI wheels installed into ignored .tools/cpp; one
# line per tool, with a hash for each supported platform (macOS x86_64/arm64,
# Linux x86_64). @sauce-git moves these pins together with the FetchContent
# pins in CMakeLists.txt and src/gameplay/CMakeLists.txt.
CPP_TOOL_REQUIREMENTS := \
	'cmake==4.4.4 --hash=sha256:883962a72b1b3a16ff445427f24c8caca9797dd9e563bbe208cbcd89e877f9d3 --hash=sha256:f773a0544c66370408f451acd487c2c643b0dfebb48c5d64f3638bc1bb820238' \
	'ninja==1.13.2 --hash=sha256:fd82e26c0706ad4ab88e5fdd26f3fab0a987a90f810160f6c322e752c6af298b --hash=sha256:65a24341b5ac09fcadcc37082660be40a94174e51a937fabf6e2cae26225fa2c' \
	'clang-format==23.1.3 --hash=sha256:0b8e86cc5a870ca42989bd1ccfe5220fbab96e386922d0a8f0ca350db30593df --hash=sha256:79f53ac55434af6dda88eb90eeb65d903ff920c910f78041de41e3ca666dfba5 --hash=sha256:58bf1804216501669832a39f2415e14284337438a062ff564729792adec664ac' \
	'clang-tidy==23.1.3 --hash=sha256:6d745b16e375bfc2219bca5f6697dc17348313f6b1fc58f8fa7321172a59ecdf --hash=sha256:cbd31dea271cee7ed5b5048847e73b8208c8dde90e321e09c20d9146011f8ff2 --hash=sha256:6b2558bc396a6e792190700134609e041f657d7f8d15385e8b0aa66f5ffe21b5'

TOOLS := $(CURDIR)/.tools/cpp
TOOLS_STAMP := $(TOOLS)/requirements.txt
CMAKE := $(TOOLS)/bin/cmake
CTEST := $(TOOLS)/bin/ctest
CLANG_FORMAT := $(TOOLS)/bin/clang-format
CLANG_TIDY := $(TOOLS)/bin/clang-tidy
BUILD_DIR := build
PROJECT_DIR := project
CONFIGURE := $(CMAKE) -S . -B $(BUILD_DIR) -G Ninja -DCMAKE_MAKE_PROGRAM=$(TOOLS)/bin/ninja -DCMAKE_BUILD_TYPE=Release -DPython3_EXECUTABLE=$(TOOLS)/bin/python

CPP_FILES := git ls-files --cached --others --exclude-standard -- 'src/*.cpp' 'src/*.hpp'
CPP_UNITS := git ls-files --cached --others --exclude-standard -- 'src/*.cpp'
CLANG_TIDY_CHECKS := -*,bugprone-*,clang-analyzer-*,cppcoreguidelines-*,misc-*,modernize-*,performance-*,portability-*,readability-*,-bugprone-easily-swappable-parameters,-cppcoreguidelines-avoid-magic-numbers,-cppcoreguidelines-pro-type-union-access,-misc-include-cleaner,-modernize-use-trailing-return-type,-portability-avoid-pragma-once,-readability-function-cognitive-complexity,-readability-identifier-length,-readability-magic-numbers,-readability-math-missing-parentheses,-readability-trailing-comma
CLANG_TIDY_FLAGS := -p $(BUILD_DIR) --quiet --checks='$(CLANG_TIDY_CHECKS)' --warnings-as-errors='*' --header-filter='$(CURDIR)/src/'
ifeq ($(shell uname -s),Darwin)
CLANG_TIDY_FLAGS += --extra-arg=-isysroot$(shell xcrun --show-sdk-path)
endif

.PHONY: lint format build test ci run

$(TOOLS_STAMP): Makefile
	$(PYTHON) -m venv $(TOOLS)
	printf '%s\n' $(CPP_TOOL_REQUIREMENTS) > $@.tmp
	$(TOOLS)/bin/python -m pip install --quiet --disable-pip-version-check --require-hashes --only-binary=:all: --no-deps -r $@.tmp
	mv $@.tmp $@

lint: $(TOOLS_STAMP)
	$(CONFIGURE)
	$(CMAKE) --build $(BUILD_DIR) --target generate_bindings
	@set -e; files=$$($(CPP_UNITS)); count=$$(printf '%s\n' $$files | grep -c .) || true; \
	test "$$count" -gt 0 || { echo "no C++ translation unit was read" >&2; exit 1; }; \
	$(CLANG_TIDY) $(CLANG_TIDY_FLAGS) $$files; \
	echo "clang-tidy checked $$count C++ translation units, selected by: $(CPP_UNITS)"

format: $(TOOLS_STAMP)
	@set -e; files=$$($(CPP_FILES)); count=$$(printf '%s\n' $$files | grep -c .) || true; \
	test "$$count" -gt 0 || { echo "no C++ file was read" >&2; exit 1; }; \
	$(CLANG_FORMAT) --style=LLVM --dry-run --Werror $$files; \
	echo "clang-format checked $$count C++ files, selected by: $(CPP_FILES)"

build: $(TOOLS_STAMP)
	$(CONFIGURE)
	$(CMAKE) --build $(BUILD_DIR)
	@test "$$("$(GODOT)" --version)" = "$(GODOT_VERSION)" || { echo "Expected Godot $(GODOT_VERSION)" >&2; exit 1; }
	"$(GODOT)" --headless --path $(PROJECT_DIR) --import

test: build
	$(CTEST) --test-dir $(BUILD_DIR) --output-on-failure --no-tests=error
	@echo "ctest ran every test registered in $(BUILD_DIR), selected by: gtest_discover_tests in src/gameplay/CMakeLists.txt"
	"$(GODOT)" --headless --path $(PROJECT_DIR) --script res://tests/godot_startup.gd --check-only
	@log=$$(mktemp); trap 'rm -f "$$log"' EXIT HUP INT TERM; \
	"$(GODOT)" --headless --path $(PROJECT_DIR) --main-loop GodotStartup --quit-after 2 > "$$log" 2>&1; status=$$?; \
	cat "$$log"; test "$$status" -eq 0 && grep -Fxq 'Startup: 1 configured main scene ready.' "$$log"
	@log=$$(mktemp); trap 'rm -f "$$log"' EXIT HUP INT TERM; \
	"$(GODOT)" --headless --path $(PROJECT_DIR) --main-loop AscentGameplayCheck --fixed-fps 120 > "$$log" 2>&1; status=$$?; \
	cat "$$log"; test "$$status" -eq 0 && grep -Fxq 'Gameplay: 3 live input checks passed (move, jump, retry).' "$$log"

ci: lint format test

run: build
	"$(GODOT)" --path $(PROJECT_DIR)
