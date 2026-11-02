class_name SurvivalTimingCheck
extends SceneTree
## Exercises the survival host's fixed-step time through the shipped composition.
## Run with --fixed-fps 480 for sub-step frames and --fixed-fps 5 for frames that
## span several steps. Run with no --fixed-fps and `-- real-clock` for frames
## slowed for real, the only ones that reach the engine's frame time cap.

const BUDGET_MS: int = 45000
const STEP_MS: int = 100
## Above this frame time Godot's default of 8 steps at 120 Hz cuts `delta`.
const DEFAULT_CAP_MS: int = 67
const RULE_TICKS_PER_FRAME: int = 8
## No AI collects in this party, so only the steps these checks count spend the clock.
const TWO_PLAYERS: int = 3
## The rule tick that hits an idle player in pan_dodge.
const FIRST_HIT_TICK: int = 300

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
	if OS.get_cmdline_user_args().has("real-clock"):
		await _real_frames()
	elif frame_seconds > 0.1:
		await _coarse_frames()
	else:
		await _fine_frames()
	if failed:
		return
	print("Survival timing: ", checks, " integration checks passed.")
	quit(0)

func _fine_frames() -> void:
	await _start(TWO_PLAYERS)
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
	_same_frame_tap(KEY_D)
	await _frames(60)
	if not _check(_actor("chef", "x") == x + 1, "a press and release inside one frame walks exactly one cell"):
		return
	await _align_to_step()
	x = _actor("chef", "x")
	Input.action_press("survival_1_run")
	_same_frame_tap(KEY_D)
	await _frames(60)
	Input.action_release("survival_1_run")
	if not _check(_actor("chef", "x") == x + 2, "a tap with the run key held runs one step"):
		return
	await _align_to_step()
	x = _actor("chef", "x")
	Input.action_press("survival_1_right")
	await _frames(72)
	Input.action_release("survival_1_right")
	await _frames(96)
	if not _check(_actor("chef", "x") == x + 1, "a key held across one step and released mid-step walks one cell"):
		return
	await _pause_holds_the_run()
	if failed:
		return
	await _keys_do_not_cross_a_pause()
	if failed:
		return
	await _confirmation_leaves_the_run_as_it_was()
	if failed:
		return
	await _menu_drops_input()
	if failed:
		return
	await _expiry_leaves_nothing_behind()
	if failed:
		return
	await _one_end_of_day_per_step()

## Esc stops the clock and keeps the part of a step already played.
func _pause_holds_the_run() -> void:
	await _align_to_step()
	var before: int = _int("remaining_ms")
	var elapsed: int = _int("elapsed_ms")
	await _frames(30)
	await _one_frame("pause")
	if not _check(_interruption() == "paused", "the pause key pauses a survival run"):
		return
	await _frames(480)
	if not _check(_int("remaining_ms") == before and _int("elapsed_ms") == elapsed, "a second paused spends neither clock"):
		return
	var at: int = _actor("chef", "x")
	Input.action_press("survival_1_right")
	Input.action_press("survival_1_travel")
	await _frames(60)
	Input.action_release("survival_1_right")
	Input.action_release("survival_1_travel")
	await _frames(1)
	if not _check(_actor("chef", "x") == at and _location("chef") == "city", "keys pressed while paused apply no command"):
		return
	await _one_frame("pause")
	if not _check(_interruption() == "", "the pause key continues the run"):
		return
	await _frames(8)
	if not _check(_int("remaining_ms") == before, "the paused time is not spent on resuming"):
		return
	await _frames(16)
	_check(_int("remaining_ms") == before - STEP_MS, "the part of a step played before the pause still counts")

## A key held across a pause, or pressed just before it, does nothing on resuming.
func _keys_do_not_cross_a_pause() -> void:
	await _align_to_step()
	Input.action_press("survival_1_right")
	await _frames(1)
	await _one_frame("pause")
	var x: int = _actor("chef", "x")
	var before: int = _int("remaining_ms")
	await _one_frame("pause")
	if not _check(_interruption() == "resuming", "a held key keeps the run waiting"):
		return
	await _frames(240)
	if not _check(_actor("chef", "x") == x and _int("remaining_ms") == before, "the run waits on a held key without moving or spending time"):
		return
	Input.action_release("survival_1_right")
	await _frames(2)
	if not _check(_interruption() == "", "the run continues once the key is up"):
		return
	await _frames(96)
	if not _check(_actor("chef", "x") == x, "a key held across a pause walks nothing"):
		return
	await _align_to_step()
	Input.action_press("survival_1_travel")
	await _frames(1)
	Input.action_release("survival_1_travel")
	await _one_frame("pause")
	await _one_frame("pause")
	await _frames(96)
	if not _check(_location("chef") == "city", "a press still pending when the pause opens is dropped"):
		return
	await _one_frame("survival_1_right")
	await _frames(60)
	_check(_actor("chef", "x") == x + 1, "a fresh press walks after a pause")

## M asks first, and cancelling changes nothing.
func _confirmation_leaves_the_run_as_it_was() -> void:
	await _align_to_step()
	await _frames(30)
	var before: Dictionary = scene.survival_status()
	var remaining: int = _int("remaining_ms")
	await _one_frame("menu")
	if not _check(_interruption() == "confirm_leave" and scene.screen_name() == "survival", "M mid-run asks before leaving"):
		return
	var prompt: Array = scene.survival_status()["prompt"]
	if not _check(str(prompt).contains("not saved"), "the question says progress is not saved"):
		return
	await _frames(480)
	if not _check(_int("remaining_ms") == remaining, "the question holds the clock"):
		return
	await _one_frame("menu")
	if not _check(_interruption() == "confirm_leave", "a second M does not answer the question"):
		return
	await _one_frame("pause")
	if not _check(_interruption() == "" and scene.survival_status() == before, "cancelling leaves every value as it was"):
		return
	await _frames(24)
	if not _check(_int("remaining_ms") == remaining - STEP_MS, "the run goes on after cancelling"):
		return
	await _one_frame("pause")
	await _one_frame("confirm")
	if not _check(_interruption() == "paused", "Enter answers nothing on the pause screen"):
		return
	await _one_frame("menu")
	if not _check(_interruption() == "confirm_leave", "M on the pause screen asks before leaving"):
		return
	await _one_frame("pause")
	if not _check(_interruption() == "paused", "cancelling from the pause screen returns to it"):
		return
	await _one_frame("pause")
	_check(_interruption() == "", "the run continues from the pause screen")

## A travel press still pending when the menu opens does not reach the next run.
func _menu_drops_input() -> void:
	await _leave_run()
	await _frames(4)
	await _start(1)
	await _frames(1)
	if not _check(_int("remaining_ms") == BUDGET_MS and _location("chef") == "base", "a new run opens with a full clock at base"):
		return
	Input.action_press("survival_1_travel")
	await _frames(1)
	Input.action_release("survival_1_travel")
	await _leave_run()
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
	await _leave_run()
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
	if not _check(_int("day") == 4, "each player can end a day in its own step"):
		return
	await _switch_and_leave()

## Two humans keep both actors through a swap, then one leaves to AI, and the
## human left drives either actor with the first key set but cannot leave.
func _switch_and_leave() -> void:
	await _one_frame("survival_1_switch")
	await _frames(96)
	if not _check(_actor("chef", "key_set") == 0 and _actor("appraiser", "key_set") == 1, "a swap leaves two humans on their actors"):
		return
	await _one_frame("survival_2_leave")
	await _frames(96)
	if not _check(_actor("chef", "key_set") == 0 and _actor("appraiser", "key_set") == -1, "leaving hands only that actor to AI"):
		return
	await _one_frame("survival_1_switch")
	await _frames(96)
	if not _check(_actor("chef", "key_set") == -1 and _actor("appraiser", "key_set") == 0, "a lone human takes the other actor on the first key set"):
		return
	var day: int = _int("day")
	await _one_frame("survival_1_end_day")
	await _frames(96)
	if not _check(_int("day") == day + 1, "the first key set drives the swapped actor"):
		return
	await _one_frame("survival_1_leave")
	await _frames(96)
	if not _check(_actor("chef", "key_set") == -1 and _actor("appraiser", "key_set") == 0, "the last human stays when leaving"):
		return
	await _both_leave()

## Two humans leaving in one step still leave one human.
func _both_leave() -> void:
	await _leave_run()
	await _frames(4)
	await _start(3)
	if not _check(_actor("chef", "key_set") == 0 and _actor("appraiser", "key_set") == 1, "a new run has two humans"):
		return
	Input.action_press("survival_1_leave")
	Input.action_press("survival_2_leave")
	await _frames(1)
	Input.action_release("survival_1_leave")
	Input.action_release("survival_2_leave")
	await _frames(96)
	_check(_actor("chef", "key_set") == -1 and _actor("appraiser", "key_set") == 0, "both leaving in one step keeps one human")

func _coarse_frames() -> void:
	await _start(TWO_PLAYERS)
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
	await _frames(1)
	x = _actor("chef", "x")
	_same_frame_tap(KEY_D)
	await _frames(3)
	if not _check(_actor("chef", "x") == x + 1, "a press and release inside one slow frame walks one cell, not one per step"):
		return
	x = _actor("chef", "x")
	_same_frame_tap(KEY_D)
	await _frames(1)
	_same_frame_tap(KEY_D)
	await _frames(3)
	if not _check(_actor("chef", "x") == x + 2, "same-direction taps in consecutive slow frames walk one cell each"):
		return
	Input.action_press("survival_1_left")
	await _frames(2)
	x = _actor("chef", "x")
	Input.action_press("survival_1_right")
	await _frames(1)
	if not _check(_actor("chef", "x") == x - 2, "a second key pressed and kept down leaves the held direction walking"):
		return
	Input.action_release("survival_1_right")
	_same_frame_tap(KEY_D)
	await _frames(1)
	x = _actor("chef", "x")
	await _frames(1)
	_check(_actor("chef", "x") == x, "a tap beside a held key walks one step of its frame and the held key the rest")
	Input.action_release("survival_1_left")
	if failed:
		return
	await _frames(1)
	await _pause_on_slow_frames()

## The frame of the press was played and spends its time; the paused ones do not.
func _pause_on_slow_frames() -> void:
	for round: int in range(3):
		var before: int = _int("remaining_ms")
		await _one_frame("pause")
		if not _check(_interruption() == "paused" and _int("remaining_ms") == before - 2 * STEP_MS, "a slow frame that pauses spends itself once"):
			return
		await _frames(5)
		if not _check(_int("remaining_ms") == before - 2 * STEP_MS, "slow paused frames spend nothing"):
			return
		await _one_frame("pause")
		if not _check(_interruption() == "" and _int("remaining_ms") == before - 2 * STEP_MS, "resuming on a slow frame spends nothing"):
			return
		await _frames(1)
		if not _check(_int("remaining_ms") == before - 4 * STEP_MS, "the next slow frame spends itself once"):
			return

## `--fixed-fps` bypasses the engine's cap on `delta`, so this run must not use it.
func _real_frames() -> void:
	if not _check(not OS.get_cmdline_args().has("--fixed-fps"), "the real clock run has no fixed frame time"):
		return
	await _start(TWO_PLAYERS)
	await _one_frame("survival_1_travel")
	await _delayed_frames(3, 2 * STEP_MS)
	if not _check(_location("chef") == "city", "the party sets out on the real clock"):
		return
	for row: Vector2i in [Vector2i(30, 16), Vector2i(10, 200), Vector2i(3, 1000), Vector2i(1, 2500)]:
		if not await _spends_real_time(row.x, row.y):
			return
	var x: int = _actor("chef", "x")
	_same_frame_tap(KEY_D)
	await _delayed_frames(3, 250)
	if not _check(_actor("chef", "x") == x + 1, "a press and release inside one 250ms frame walks one cell"):
		return
	x = _actor("chef", "x")
	_same_frame_tap(KEY_D)
	await _delayed_frames(1, 250)
	_same_frame_tap(KEY_D)
	await _delayed_frames(3, 250)
	if not _check(_actor("chef", "x") == x + 2, "same-direction taps in consecutive 250ms frames walk one cell each"):
		return
	if not await _pause_ignores_the_real_clock():
		return
	await _experiment_keeps_its_delivery()

## Real time passing under a pause reaches neither clock.
func _pause_ignores_the_real_clock() -> bool:
	await _one_frame("pause")
	var before: int = _int("remaining_ms")
	await _delayed_frames(4, 250)
	if not _check(_interruption() == "paused" and _int("remaining_ms") == before, "a second of real time paused spends nothing"):
		return false
	await _one_frame("pause")
	if not _check(_interruption() == "" and before - _int("remaining_ms") <= 2 * STEP_MS, "resuming spends no paused time"):
		return false
	return await _spends_real_time(3, 250)

## The cap raised for survival's clock must not reach an experiment's rule ticks.
func _experiment_keeps_its_delivery() -> void:
	await _leave_run()
	await _frames(4)
	await _one_frame("option_2")
	await _frames(4)
	await _one_frame("option_1")
	await _frames(4)
	var dodge: PanDodgeScene = scene.get_node_or_null("Experiment") as PanDodgeScene
	if not _check(dodge != null, "the developer page mounts pan_dodge on the real clock"):
		return
	await _delayed_frames(1, 16)
	var ticks: int = dodge.run_tick()
	var physics: int = Engine.get_physics_frames()
	await _delayed_frames(30, 16)
	if not _check(dodge.run_tick() - ticks == Engine.get_physics_frames() - physics, "a frame under the limit loses no rule tick"):
		return
	await _delayed_frames(1, 200)
	ticks = dodge.run_tick()
	physics = Engine.get_physics_frames()
	await _delayed_frames(5, 200)
	if not _check(Engine.get_physics_frames() - physics > 5 * RULE_TICKS_PER_FRAME, "slow frames run more physics ticks than the limit"):
		return
	if not _check(dodge.run_tick() - ticks == 5 * RULE_TICKS_PER_FRAME, "pan_dodge runs its limit of rule ticks in each slow frame"):
		return
	await _delayed_frames(2, FIRST_HIT_TICK * 1000 / 120 + 500)
	_check(dodge.outcome() == "playing", "a stall longer than the first hit leaves the idle player playing")

## Exploration time matches the wall clock to within two steps over `count` frames.
func _spends_real_time(count: int, delay_ms: int) -> bool:
	await process_frame
	var before: int = _int("remaining_ms")
	var started: int = Time.get_ticks_msec()
	await _delayed_frames(count, delay_ms)
	await _frames(2)
	var real_ms: int = Time.get_ticks_msec() - started
	var spent_ms: int = before - _int("remaining_ms")
	if delay_ms > DEFAULT_CAP_MS and not _check(real_ms >= count * delay_ms, "%d frames were slowed by %dms" % [count, delay_ms]):
		return false
	return _check(absi(real_ms - spent_ms) <= 2 * STEP_MS, "%d frames of %dms spend real time: real %dms, game %dms" % [count, delay_ms, real_ms, spent_ms])

func _delayed_frames(count: int, delay_ms: int) -> void:
	for frame: int in range(count):
		OS.delay_msec(delay_ms)
		await process_frame

## Queues both key events for one frame, as a tap inside a long frame arrives.
func _same_frame_tap(code: Key) -> void:
	for pressed: bool in [true, false]:
		var event: InputEventKey = InputEventKey.new()
		event.physical_keycode = code
		event.pressed = pressed
		Input.parse_input_event(event)

func _start(option: int) -> void:
	await _one_frame("option_1")
	await _frames(4)
	await _one_frame("option_" + str(option))
	await _frames(4)
	await _one_frame("survival_1_interact")

## M asks first mid-run; Enter accepts.
func _leave_run() -> void:
	await _one_frame("menu")
	await _one_frame("confirm")

func _interruption() -> String:
	var name: String = scene.survival_status().get("interruption", "")
	return name

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
