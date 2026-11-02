class_name SurvivalLossCheck
extends SceneTree
## Plays the survival game from the main menu to a loss with real key events, and
## drives the cinematic contract for the loss with no asset, a working scene, and
## broken ones.

const SEED: int = 7
const LOSS: String = "res://levels/monster_mouth_escape/cinematics/loss/loss.tscn"
const WORKING: String = "res://e2e/cinematic_fixture.tscn"
const UNSIGNED: String = "res://e2e/cinematic_unsigned_fixture.tscn"
const FINISHES_IN_READY: String = "res://e2e/cinematic_ready_fixture.tscn"
const CARD_FRAMES: int = 372
const DAY_LIMIT: int = 30

var scene: MainScene
var held: Dictionary[Key, bool] = {}
var checks: int = 0
var failed: bool = false
## Keeps each taken-over resource alive; the cache forgets a path when it is freed.
var stand_in: Resource
var frame_seconds: float = 0.0

func _initialize() -> void:
	_run.call_deferred()

func _process(delta: float) -> bool:
	frame_seconds = delta
	return false

func _run() -> void:
	await _frames(2)
	scene = current_scene as MainScene
	if not _check(scene != null and scene.screen_name() == "main", "the game opens on the main menu"):
		return
	scene.pin_survival_seed(SEED)
	if frame_seconds > 0.1:
		await _lose()
		if failed:
			return
		_check(_str("phase") == "lost" and _str("cinematic") == "loss", "a slow frame opens the loss card on the day acid fills")
		if not failed:
			print("Loss slow frames: ", checks, " live checks passed.")
			quit(0)
		return
	for part: Callable in [_never_feeding_loses, _skip_the_card, _card_times_out, _scene_loss, _broken_scene, _menu_leaves_the_card, _instant_loss]:
		await part.call()
		if failed:
			return
	print("Loss: ", checks, " live checks passed (acid, loss, input block, card, skip, scene, broken asset, menu, instant finish).")
	quit(0)

## Starts a run, skips the intro, and presses only the end-day key until it ends.
func _lose() -> void:
	await _tap(KEY_1)
	await _tap(KEY_1)
	await _tap(KEY_E)
	while _str("phase") == "workshop" and _int("day") < DAY_LIMIT:
		var day: int = _int("day")
		await _tap(KEY_G)
		await _until("the day ends", func() -> bool: return _int("day") == day + 1, 30)

func _never_feeding_loses() -> void:
	if not _check(not ResourceLoader.exists(LOSS), "no cinematic asset exists at the contract path"):
		return
	await _tap(KEY_1)
	await _tap(KEY_1)
	await _tap(KEY_E)
	var full: int = _int("full_acid")
	if not _check(full > 0 and _int("acid") == 0, "a run opens with the rules' full acid and none risen"):
		return
	await _tap(KEY_G)
	await _until("the first unfed day ends", func() -> bool: return _int("day") == 2, 30)
	var penalty: int = _int("acid")
	if not _check(penalty > 0 and _str("phase") == "workshop", "an unfed day raises acid and the run goes on"):
		return
	while _str("phase") == "workshop" and _int("day") < DAY_LIMIT:
		var day: int = _int("day")
		if not _check(_int("acid") < full, "the run goes on while acid is below full"):
			return
		await _tap(KEY_G)
		await _until("the day ends", func() -> bool: return _int("day") == day + 1, 30)
	if not _check(_str("phase") == "lost" and _int("acid") == full, "acid at full loses the run"):
		return
	if not _check(_int("day") - 1 == ceili(float(full) / penalty) and _int("escape_rolls") == 0, "the loss comes on the day acid fills, with no escape roll"):
		return
	if not _check(_str("cinematic") == "loss" and _str("cinematic_source") == "fallback", "a missing loss scene takes the fallback card"):
		return
	if not _check(_str("next").contains("acid"), "the screen names the loss"):
		return
	var before: Dictionary = scene.survival_status()
	await _tap(KEY_Q)
	await _tap(KEY_G)
	await _tap(KEY_F)
	await _tap(KEY_R)
	await _frames(30)
	if not _check(_same_state(before), "no key changes a lost run"):
		return
	await _tap(KEY_M)
	_check(scene.screen_name() == "main", "M leaves the card for the main menu")

func _skip_the_card() -> void:
	await _lose()
	await _tap(KEY_E)
	_check(scene.screen_name() == "main" and scene.survival_status().is_empty(), "E skips the card and the run is closed")

func _card_times_out() -> void:
	await _lose()
	if not await _until("the loss card times out", func() -> bool: return scene.screen_name() == "main", CARD_FRAMES):
		return
	_check(scene.survival_status().is_empty(), "the card's end closes the run")

func _scene_loss() -> void:
	_stand_in(load(WORKING), LOSS)
	await _lose()
	if not _check(_str("cinematic") == "loss" and _str("cinematic_source") == "scene" and _mounted(), "a loss asset plays as a scene"):
		return
	var before: Dictionary = scene.survival_status()
	await _tap(KEY_G)
	if not _check(_same_state(before), "a playing scene blocks the game's keys"):
		return
	if not await _until("the loss scene reports its end", func() -> bool: return scene.screen_name() == "main", 120):
		return
	_check(not _mounted() and scene.survival_status().is_empty(), "the loss scene returns to the main menu")

func _broken_scene() -> void:
	_stand_in(load(UNSIGNED), LOSS)
	await _lose()
	if not _check(_str("cinematic_source") == "fallback" and not _mounted(), "a scene without the finished signal takes the fallback"):
		return
	await _tap(KEY_E)
	_check(scene.screen_name() == "main", "E skips the fallback for a broken scene")

func _menu_leaves_the_card() -> void:
	_stand_in(load(WORKING), LOSS)
	await _lose()
	await _tap(KEY_M)
	_check(scene.screen_name() == "main" and scene.survival_status().is_empty() and not _mounted(), "M leaves a playing loss scene and ends the run")

## A crash here ends the process, which `make test` reads as a failure.
func _instant_loss() -> void:
	_stand_in(load(FINISHES_IN_READY), LOSS)
	await _tap(KEY_1)
	await _tap(KEY_1)
	await _tap(KEY_E)
	while scene.screen_name() != "main" and _int("day") < DAY_LIMIT:
		var day: int = _int("day")
		await _tap(KEY_G)
		if not await _until("the day ends or the loss closes", func() -> bool: return scene.screen_name() == "main" or _int("day") == day + 1, 30):
			return
	_check(scene.screen_name() == "main" and scene.survival_status().is_empty() and not _mounted(), "an instant loss closes the run and returns to the main menu")

## Serves `resource` from `path` without writing to the art team's folder.
func _stand_in(resource: Resource, path: String) -> void:
	stand_in = resource
	resource.take_over_path(path)

func _mounted() -> bool:
	return scene.get_node_or_null("Cinematic") != null

## Compares the run's state, leaving out the event log's order of arrival.
func _same_state(before: Dictionary) -> bool:
	var now: Dictionary = scene.survival_status()
	for field: String in ["day", "phase", "acid", "remaining_ms", "elapsed_ms", "rolls", "escape_rolls", "foods", "chef", "appraiser", "stock", "objects"]:
		if now[field] != before[field]:
			return false
	return true

func _str(field: String) -> String:
	var value: String = scene.survival_status().get(field, "")
	return value

func _int(field: String) -> int:
	var value: int = scene.survival_status().get(field, 0)
	return value

func _until(name: String, condition: Callable, limit: int) -> bool:
	for frame: int in range(limit):
		var done: bool = condition.call()
		if done:
			return true
		await process_frame
	return _check(false, name)

func _tap(code: Key) -> void:
	_key(code, true)
	await _frames(2)
	_key(code, false)
	await _frames(8)

func _frames(count: int) -> void:
	for frame: int in range(count):
		await process_frame

func _key(code: Key, pressed: bool) -> void:
	if held.get(code, false) == pressed:
		return
	held[code] = pressed
	var event: InputEventKey = InputEventKey.new()
	event.physical_keycode = code
	event.pressed = pressed
	Input.parse_input_event(event)

func _check(condition: bool, name: String) -> bool:
	if condition:
		checks += 1
		return true
	push_error("Loss check failed: ", name, " ", scene.survival_status() if scene != null else {})
	failed = true
	quit(1)
	return false
