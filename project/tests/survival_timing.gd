class_name SurvivalTimingCheck
extends SceneTree
## Exercises the survival host's fixed-step time through the shipped composition.
## Run with --fixed-fps 480 for sub-step frames and --fixed-fps 5 for frames that
## span several steps.

const BUDGET_MS: int = 45000
const STEP_MS: int = 100

var scene: AscentScene
var checks: int = 0
var failed: bool = false
var frame_seconds: float = 0.0

func _initialize() -> void:
	_run.call_deferred()

func _process(delta: float) -> bool:
	frame_seconds = delta
	return false

func _run() -> void:
	await _frames(2)
	scene = current_scene as AscentScene
	scene.pin_survival_seed(3)
	if frame_seconds > 0.1:
		await _coarse_frames()
	else:
		await _fine_frames()
	if failed:
		return
	print("Survival timing: ", checks, " integration checks passed.")
	quit(0)

func _fine_frames() -> void:
	await _start(1)
	await _one_frame("survival_1_travel")
	await _frames(60)
	if not _check(_location("chef") == "city", "a one-frame travel tap between steps is retained"):
		return
	await _align_to_step()
	var before: int = _int("remaining_ms")
	if not _check(before % STEP_MS == 0, "exploration time moves in 100ms units"):
		return
	await _frames(20)
	if not _check(_int("remaining_ms") == before, "a frame shorter than a step spends nothing"):
		return
	await _frames(30)
	if not _check(_int("remaining_ms") == before - STEP_MS, "a step's worth of frames spends exactly one step"):
		return
	await _align_to_step()
	before = _int("remaining_ms")
	await _frames(482)
	if not _check(_int("remaining_ms") == before - 1000, "one second of frames spends one second, neither lost nor doubled"):
		return
	await _align_to_step()
	var x: int = _actor("chef", "x")
	await _one_frame("survival_1_right")
	await _frames(60)
	if not _check(_actor("chef", "x") == x + 1, "a one-frame direction tap walks exactly one cell"):
		return
	await _align_to_step()
	x = _actor("chef", "x")
	Input.action_press("survival_1_right")
	await _frames(72)
	Input.action_release("survival_1_right")
	await _frames(96)
	if not _check(_actor("chef", "x") == x + 1, "a key held across one step and released mid-step walks one cell"):
		return
	await _menu_drops_input()
	if failed:
		return
	await _expiry_leaves_nothing_behind()
	if failed:
		return
	await _one_end_of_day_per_step()

## A travel press still pending when the menu opens does not reach the next run.
func _menu_drops_input() -> void:
	Input.action_press("menu")
	await _frames(2)
	Input.action_release("menu")
	await _frames(4)
	await _start(1)
	await _frames(1)
	if not _check(_int("remaining_ms") == BUDGET_MS and _location("chef") == "base", "a new run opens with a full clock at base"):
		return
	Input.action_press("survival_1_travel")
	await _frames(1)
	Input.action_release("survival_1_travel")
	Input.action_press("menu")
	await _frames(2)
	Input.action_release("menu")
	await _frames(4)
	await _start(1)
	await _frames(96)
	_check(_location("chef") == "base" and _int("remaining_ms") == BUDGET_MS, "a pending edge and partial step are dropped with the run")

func _expiry_leaves_nothing_behind() -> void:
	await _one_frame("survival_1_travel")
	await _frames(60)
	if not _check(_location("chef") == "city", "the party sets out for the expiry check"):
		return
	await _until_workshop()
	if not _check(_int("remaining_ms") == 0 and _int("elapsed_ms") == BUDGET_MS, "expiry spends exactly the budget"):
		return
	await _frames(240)
	if not _check(_int("elapsed_ms") == BUDGET_MS, "time after expiry reaches no one"):
		return
	await _one_frame("survival_1_end_day")
	await _frames(60)
	if not _check(_int("day") == 2 and _int("remaining_ms") == BUDGET_MS, "the next day opens with a full budget"):
		return
	await _frames(240)
	_check(_int("remaining_ms") == BUDGET_MS, "the workshop keeps the new budget")

func _one_end_of_day_per_step() -> void:
	Input.action_press("menu")
	await _frames(2)
	Input.action_release("menu")
	await _frames(4)
	await _start(3)
	if not _check(_actor("chef", "key_set") == 0 and _actor("appraiser", "key_set") == 1, "two players hold one key set each"):
		return
	await _align_to_step()
	Input.action_press("survival_1_end_day")
	Input.action_press("survival_2_end_day")
	await _frames(1)
	Input.action_release("survival_1_end_day")
	Input.action_release("survival_2_end_day")
	await _frames(96)
	if not _check(_int("day") == 2, "both players ending the day in one step end one day"):
		return
	await _one_frame("survival_1_end_day")
	await _frames(96)
	await _one_frame("survival_2_end_day")
	await _frames(96)
	_check(_int("day") == 4, "each player can end a day in its own step")

func _coarse_frames() -> void:
	await _start(1)
	await _one_frame("survival_1_travel")
	await _frames(1)
	if not _check(_location("chef") == "city", "a slow frame still carries a tap"):
		return
	var before: int = _int("remaining_ms")
	var x: int = _actor("chef", "x")
	Input.action_press("survival_1_right")
	await _frames(1)
	if not _check(_int("remaining_ms") == before - 2 * STEP_MS, "a 200ms frame spends two 100ms steps"):
		return
	if not _check(_actor("chef", "x") == x + 2, "a held key walks both steps of a slow frame"):
		return
	Input.action_release("survival_1_right")

func _start(option: int) -> void:
	await _one_frame("option_1")
	await _frames(4)
	await _one_frame("option_" + str(option))
	await _frames(4)

func _one_frame(action: String) -> void:
	Input.action_press(action)
	await _frames(1)
	Input.action_release(action)
	await _frames(1)

## Waits until a step has just run, so the next 48 frames hold exactly one step.
func _align_to_step() -> void:
	var seen: int = _int("remaining_ms")
	for frame: int in range(96):
		await process_frame
		if _int("remaining_ms") != seen:
			return

func _until_workshop() -> void:
	for frame: int in range(BUDGET_MS / STEP_MS * 48 + 960):
		await process_frame
		var phase: String = scene.survival_status()["phase"]
		if phase == "workshop":
			return

func _location(who: String) -> String:
	var actor: Dictionary = scene.survival_status()[who]
	var location: String = actor["location"]
	return location

func _actor(who: String, field: String) -> int:
	var actor: Dictionary = scene.survival_status()[who]
	var value: int = actor[field]
	return value

func _int(field: String) -> int:
	var value: int = scene.survival_status()[field]
	return value

func _frames(count: int) -> void:
	for frame: int in range(count):
		await process_frame

func _check(condition: bool, name: String) -> bool:
	if condition:
		checks += 1
		return true
	push_error("Survival timing check failed: ", name, " ", scene.survival_status())
	failed = true
	quit(1)
	return false
