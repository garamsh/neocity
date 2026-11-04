GODOT ?= $(if $(wildcard .tools/Godot.app/Contents/MacOS/Godot),$(CURDIR)/.tools/Godot.app/Contents/MacOS/Godot,godot)
GODOT_VERSION := 4.7.2.stable.official.ed1daf0bf
PYTHON ?= python3

# C++ tools are hash-pinned PyPI wheels installed into ignored .tools/cpp; one
# line per tool, with a hash for each supported platform (macOS x86_64/arm64,
# Linux x86_64). @sauce-git moves these pins together with the FetchContent
# pins in CMakeLists.txt and src/survival/CMakeLists.txt.
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
ART_DIR := $(PROJECT_DIR)/levels/monster_mouth_escape/art
CONFIGURE := $(CMAKE) -S . -B $(BUILD_DIR) -G Ninja -DCMAKE_MAKE_PROGRAM=$(TOOLS)/bin/ninja -DCMAKE_BUILD_TYPE=Release -DPython3_EXECUTABLE=$(TOOLS)/bin/python

CPP_FILES := git ls-files --cached --others --exclude-standard -- 'src/*.cpp' 'src/*.hpp'
CPP_UNITS := git ls-files --cached --others --exclude-standard -- 'src/*.cpp'
CLANG_TIDY_CHECKS := -*,bugprone-*,clang-analyzer-*,cppcoreguidelines-*,misc-*,modernize-*,performance-*,portability-*,readability-*,-bugprone-easily-swappable-parameters,-cppcoreguidelines-avoid-magic-numbers,-cppcoreguidelines-pro-type-union-access,-misc-include-cleaner,-modernize-use-trailing-return-type,-portability-avoid-pragma-once,-readability-function-cognitive-complexity,-readability-identifier-length,-readability-magic-numbers,-readability-math-missing-parentheses,-readability-trailing-comma
GODOT_SCRIPTS := git ls-files --cached --others --exclude-standard -- '$(PROJECT_DIR)/tests/*.gd' '$(PROJECT_DIR)/e2e/*.gd'
CLANG_TIDY_JOBS := $(shell getconf _NPROCESSORS_ONLN)
CLANG_TIDY_FLAGS := -p $(BUILD_DIR) --quiet --checks='$(CLANG_TIDY_CHECKS)' --warnings-as-errors='*' --header-filter='$(CURDIR)/src/'
ifeq ($(shell uname -s),Darwin)
CLANG_TIDY_FLAGS += --extra-arg=-isysroot$(shell xcrun --show-sdk-path)
endif

.PHONY: lint format build test test-unit test-startup test-title test-timing test-survival test-escape test-loss test-audio ci run

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
	printf '%s\n' $$files | xargs -n 1 -P $(CLANG_TIDY_JOBS) sh -c 'out=$$("$$0" "$$@" 2>&1) || { printf "%s\n" "$$out"; exit 1; }' $(CLANG_TIDY) $(CLANG_TIDY_FLAGS); \
	echo "clang-tidy checked $$count C++ translation units, selected by: $(CPP_UNITS)"

format: $(TOOLS_STAMP)
	@set -e; files=$$($(CPP_FILES)); count=$$(printf '%s\n' $$files | grep -c .) || true; \
	test "$$count" -gt 0 || { echo "no C++ file was read" >&2; exit 1; }; \
	$(CLANG_FORMAT) --style=LLVM --dry-run --Werror $$files; \
	echo "clang-format checked $$count C++ files, selected by: $(CPP_FILES)"

# Runs the Godot import in argv[2:] and stops it once argv[1] seconds pass: SIGQUIT
# first, which Godot does not handle, so it dumps core where it hung; then SIGKILL.
define GODOT_IMPORT_WITH_LIMIT
import signal, subprocess, sys
limit = int(sys.argv[1])
child = subprocess.Popen(sys.argv[2:])
try:
    status = child.wait(limit)
except subprocess.TimeoutExpired:
    print(f"Godot import exceeded the {limit} s limit (GODOT_IMPORT_LIMIT); stopping it with SIGQUIT.", file=sys.stderr, flush=True)
    child.send_signal(signal.SIGQUIT)
    try:
        child.wait(20)
    except subprocess.TimeoutExpired:
        child.kill()
        child.wait()
    sys.exit(124)
sys.exit(status if status >= 0 else 128 - status)
endef
export GODOT_IMPORT_WITH_LIMIT
GODOT_IMPORT_LIMIT := 60

# Godot 4.7.2 crashes on exit when an import finds a new extension and regenerates
# docs (godotengine/godot#111048); a listed extension loads at boot and is not new.
# A crash inside the allocator can hang Godot's crash handler, so the import has a limit.
build: $(TOOLS_STAMP)
	$(CONFIGURE)
	$(CMAKE) --build $(BUILD_DIR)
	@test "$$("$(GODOT)" --version)" = "$(GODOT_VERSION)" || { echo "Expected Godot $(GODOT_VERSION)" >&2; exit 1; }
	mkdir -p $(PROJECT_DIR)/.godot
	printf '%s\n' 'res://bin/neocity.gdextension' > $(PROJECT_DIR)/.godot/extension_list.cfg
	$(PYTHON) -c "$$GODOT_IMPORT_WITH_LIMIT" $(GODOT_IMPORT_LIMIT) "$(GODOT)" --headless --path $(PROJECT_DIR) --import

test: test-unit test-startup test-title test-timing test-survival test-escape test-loss test-audio

test-unit: build
	$(CTEST) --test-dir $(BUILD_DIR) --output-on-failure --no-tests=error
	@echo "ctest ran every test registered in $(BUILD_DIR), selected by: gtest_discover_tests in src/survival/CMakeLists.txt"
	@$(PYTHON) -m unittest discover -s tools/art -p '*_test.py'
	@echo "unittest ran the tests listed above, selected by: discovery of tools/art/*_test.py"
	@log=$$(mktemp); trap 'rm -f "$$log"' EXIT HUP INT TERM; \
	$(PYTHON) tools/art/pixelize.py check $(ART_DIR) > "$$log" 2>&1 || { cat "$$log"; exit 1; }; \
	tail -n 1 "$$log"
	@echo "pixelize.py check measured the PNGs listed in $(ART_DIR)/manifest.json"

# A --main-loop harness that fails to parse raises a dialog and waits on it even
# with --headless; --check-only answers with its exit status instead.
test-startup: build
	@files=$$($(GODOT_SCRIPTS)); count=$$(printf '%s\n' $$files | grep -c .) || true; \
	test "$$count" -gt 0 || { echo "no GDScript harness was read" >&2; exit 1; }; \
	log=$$(mktemp); trap 'rm -f "$$log"' EXIT HUP INT TERM; \
	for file in $$files; do \
		"$(GODOT)" --headless --path $(PROJECT_DIR) --script "res://$${file#$(PROJECT_DIR)/}" --check-only > "$$log" 2>&1 || { \
			cat "$$log"; echo "GDScript parse check failed: $$file" >&2; exit 1; }; \
	done; \
	echo "Godot parse-checked $$count GDScript files, selected by: $(GODOT_SCRIPTS)"
	@log=$$(mktemp); trap 'rm -f "$$log"' EXIT HUP INT TERM; \
	"$(GODOT)" --headless --path $(PROJECT_DIR) --main-loop GodotStartup --quit-after 2 > "$$log" 2>&1; status=$$?; \
	cat "$$log"; test "$$status" -eq 0 && grep -Fxq 'Startup: 1 configured main scene ready.' "$$log"

test-title: build
	@log=$$(mktemp); trap 'rm -f "$$log"' EXIT HUP INT TERM; \
	"$(GODOT)" --headless --path $(PROJECT_DIR) --main-loop TitleMenuCheck --fixed-fps 60 > "$$log" 2>&1; status=$$?; \
	cat "$$log"; test "$$status" -eq 0 && grep -Fxq 'Title: 40 live checks passed (entries, cursor, wrap, controls, back, shortcuts, how to play, player choice, controls keys, new game, mouse, quit).' "$$log"

	@log=$$(mktemp); trap 'rm -f "$$log"' EXIT HUP INT TERM; \
	"$(GODOT)" --headless --path $(PROJECT_DIR) --main-loop KeyCoverageCheck --fixed-fps 60 > "$$log" 2>&1; status=$$?; \
	cat "$$log"; test "$$status" -eq 0 && grep -Fxq 'Keys: 22 live checks passed (survival player one, survival player two).' "$$log"

test-timing: build
	@log=$$(mktemp); trap 'rm -f "$$log"' EXIT HUP INT TERM; \
	"$(GODOT)" --headless --path $(PROJECT_DIR) --main-loop SurvivalTimingCheck --fixed-fps 480 > "$$log" 2>&1; status=$$?; \
	cat "$$log"; test "$$status" -eq 0 && grep -Fxq 'Survival timing: 63 integration checks passed.' "$$log"

	@log=$$(mktemp); trap 'rm -f "$$log"' EXIT HUP INT TERM; \
	"$(GODOT)" --headless --path $(PROJECT_DIR) --main-loop SurvivalTimingCheck --fixed-fps 5 > "$$log" 2>&1; status=$$?; \
	cat "$$log"; test "$$status" -eq 0 && grep -Fxq 'Survival timing: 19 integration checks passed.' "$$log"

	@log=$$(mktemp); trap 'rm -f "$$log"' EXIT HUP INT TERM; \
	"$(GODOT)" --headless --path $(PROJECT_DIR) --main-loop SurvivalTimingCheck -- real-clock > "$$log" 2>&1; status=$$?; \
	cat "$$log"; test "$$status" -eq 0 && grep -Fxq 'Survival timing: 15 integration checks passed.' "$$log"

test-survival: build
	@log=$$(mktemp); trap 'rm -f "$$log"' EXIT HUP INT TERM; \
	"$(GODOT)" --headless --path $(PROJECT_DIR) --main-loop SurvivalPlayCheck --fixed-fps 60 > "$$log" 2>&1; status=$$?; \
	cat "$$log"; test "$$status" -eq 0 && grep -Fxq 'Survival: 258 live checks passed (menus, days, return, appraisal, cooking, dish choice, feeding, map, tools, roles, pause, leaving, hand-off, mouse, quit).' "$$log"

test-escape: build
	@log=$$(mktemp); trap 'rm -f "$$log"' EXIT HUP INT TERM; \
	"$(GODOT)" --headless --path $(PROJECT_DIR) --main-loop SurvivalEscapeCheck --fixed-fps 60 > "$$log" 2>&1; status=$$?; \
	cat "$$log"; test "$$status" -eq 0 && grep -Fxq 'Escape: 35 live checks passed (fallback, input block, days, escape, ending, scene, skip, broken assets, click, menu, instant finish).' "$$log"

	@log=$$(mktemp); trap 'rm -f "$$log"' EXIT HUP INT TERM; \
	"$(GODOT)" --headless --path $(PROJECT_DIR) --main-loop SurvivalEscapeCheck --fixed-fps 5 > "$$log" 2>&1; status=$$?; \
	cat "$$log"; test "$$status" -eq 0 && grep -Fxq 'Escape slow frames: 4 live checks passed.' "$$log"

	@log=$$(mktemp); trap 'rm -f "$$log"' EXIT HUP INT TERM; \
	"$(GODOT)" --headless --path $(PROJECT_DIR) --main-loop SurvivalEscapeCheck -- real-clock > "$$log" 2>&1; status=$$?; \
	cat "$$log"; test "$$status" -eq 0 && grep -Fxq 'Escape real frames: 4 live checks passed.' "$$log"

test-loss: build
	@log=$$(mktemp); trap 'rm -f "$$log"' EXIT HUP INT TERM; \
	"$(GODOT)" --headless --path $(PROJECT_DIR) --main-loop SurvivalLossCheck --fixed-fps 60 > "$$log" 2>&1; status=$$?; \
	cat "$$log"; test "$$status" -eq 0 && grep -Fxq 'Loss: 29 live checks passed (acid, loss, input block, card, skip, click, scene, broken asset, menu, instant finish).' "$$log"

	@log=$$(mktemp); trap 'rm -f "$$log"' EXIT HUP INT TERM; \
	"$(GODOT)" --headless --path $(PROJECT_DIR) --main-loop SurvivalLossCheck --fixed-fps 5 > "$$log" 2>&1; status=$$?; \
	cat "$$log"; test "$$status" -eq 0 && grep -Fxq 'Loss slow frames: 2 live checks passed.' "$$log"

test-audio: build
	@log=$$(mktemp); trap 'rm -f "$$log"' EXIT HUP INT TERM; \
	"$(GODOT)" --headless --path $(PROJECT_DIR) --main-loop AudioFeedbackCheck --fixed-fps 60 > "$$log" 2>&1; status=$$?; \
	cat "$$log"; test "$$status" -eq 0 && grep -Fxq 'Audio: 33 live checks passed (menus, music, tension, walking, collecting, travel, workshop, feeding, new day, acid, burn, repair, fall, flourishes, effects off, music off).' "$$log"

ci: lint format test

run: build
	"$(GODOT)" --path $(PROJECT_DIR)
