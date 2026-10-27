CARGO := $(if $(wildcard .tools/cargo/bin/cargo),$(CURDIR)/.tools/cargo/bin/cargo,cargo)
GODOT ?= $(if $(wildcard .tools/Godot.app/Contents/MacOS/Godot),$(CURDIR)/.tools/Godot.app/Contents/MacOS/Godot,godot)
GODOT_VERSION := 4.7.2.stable.official.ed1daf0bf
ifneq ($(wildcard .tools/cargo/bin/cargo),)
export CARGO_HOME := $(CURDIR)/.tools/cargo
export RUSTUP_HOME := $(CURDIR)/.tools/rustup
endif

.PHONY: lint format build test ci run

lint:
	./check-contents-list.sh
	./check-object-names.sh
	./check-convention-index.sh
	./check-section-citations.sh
	./check-heading-prefixes.sh
	$(CARGO) clippy --workspace --all-targets --locked -- -D warnings

format:
	$(CARGO) fmt --all -- --check

build:
	$(CARGO) build --workspace --release --locked
	@test "$$("$(GODOT)" --version)" = "$(GODOT_VERSION)" || { echo "Expected Godot $(GODOT_VERSION)" >&2; exit 1; }
	@mkdir -p target; touch target/.gdignore
	"$(GODOT)" --headless --path . --import

test: build
	$(CARGO) test --workspace --locked
	"$(GODOT)" --headless --path . --script tests/godot_startup.gd --check-only
	@log=$$(mktemp); trap 'rm -f "$$log"' EXIT HUP INT TERM; \
	"$(GODOT)" --headless --path . --main-loop GodotStartup --quit-after 2 > "$$log" 2>&1; status=$$?; \
	cat "$$log"; test "$$status" -eq 0 && grep -Fxq 'Startup: 1 configured main scene ready.' "$$log"
	@log=$$(mktemp); trap 'rm -f "$$log"' EXIT HUP INT TERM; \
	"$(GODOT)" --headless --path . --main-loop AscentGameplayCheck --fixed-fps 120 > "$$log" 2>&1; status=$$?; \
	cat "$$log"; test "$$status" -eq 0 && grep -Fxq 'Gameplay: 3 live input checks passed (move, jump, retry).' "$$log"

ci: lint format test

run: build
	"$(GODOT)" --path .
