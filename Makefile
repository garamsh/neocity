GODOT ?= godot
GODOT_VERSION := 4.7.2.stable.official.ed1daf0bf

.PHONY: lint build test ci

lint:
	./check-contents-list.sh
	./check-object-names.sh
	./check-convention-index.sh
	./check-section-citations.sh
	./check-heading-prefixes.sh

build:
	@test "$$("$(GODOT)" --version)" = "$(GODOT_VERSION)" || { echo "Expected Godot $(GODOT_VERSION)" >&2; exit 1; }
	"$(GODOT)" --headless --path . --import

test: build
	"$(GODOT)" --headless --path . --script tests/godot_startup.gd --check-only
	# Godot can exit zero after startup errors; require the readiness report.
	@log=$$(mktemp); trap 'rm -f "$$log"' EXIT HUP INT TERM; \
	"$(GODOT)" --headless --path . --main-loop GodotStartup --quit-after 2 > "$$log" 2>&1; status=$$?; \
	cat "$$log"; \
	test "$$status" -eq 0 && grep -Fxq 'Startup: 1 configured main scene ready.' "$$log"

ci: lint
	$(MAKE) test
