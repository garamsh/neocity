class_name TitleMenuCheck
extends SceneTree
## Drives the title screen with real key events: each entry by cursor and by
## number, the controls page and back, the choice of players by cursor, the
## controls page against the pause screen's keys, a new game, and quitting.

var scene: MainScene
var held: Dictionary[Key, bool] = {}
var checks: int = 0
var failed: bool = false
var awaiting_quit: bool = false

func _initialize() -> void:
	_run.call_deferred()

## Only the game's own quit reaches here with the pass line owed.
func _finalize() -> void:
	if awaiting_quit and not failed:
		print("Title: ", checks, " live checks passed (entries, cursor, wrap, controls, back, shortcuts, player choice, controls keys, new game, quit).")

func _run() -> void:
	await _frames(2)
	scene = current_scene as MainScene
	if not _check(scene != null and scene.screen_name() == "main", "the game opens on the title screen"):
		return
	if not await _cursor_chooses_each_entry():
		return
	if not await _numbers_choose_entries():
		return
	if not await _cursor_chooses_each_player():
		return
	if not await _new_game_starts_a_run():
		return
	if not await _controls_list_the_pause_keys():
		return
	await _quit_ends_the_game()

func _cursor_chooses_each_entry() -> bool:
	await _tap(KEY_ENTER)
	if not _check(scene.screen_name() == "survival_setup", "Enter on the first entry, New game, opens the choice of players"):
		return false
	await _tap(KEY_ESCAPE)
	if not _check(scene.screen_name() == "main", "Esc goes back from the choice of players"):
		return false
	await _tap(KEY_S)
	await _tap(KEY_ENTER)
	if not _check(scene.screen_name() == "controls", "S then Enter opens the controls"):
		return false
	await _tap(KEY_ESCAPE)
	if not _check(scene.screen_name() == "main", "Esc goes back from the controls"):
		return false
	await _tap(KEY_ENTER)
	if not _check(scene.screen_name() == "controls", "the cursor stays on the entry it went back from"):
		return false
	await _tap(KEY_M)
	if not _check(scene.screen_name() == "main", "M goes back from the controls"):
		return false
	await _tap(KEY_W)
	await _tap(KEY_UP)
	await _tap(KEY_DOWN)
	await _tap(KEY_ENTER)
	if not _check(scene.screen_name() == "survival_setup", "W, up and down wrap past Quit back to New game"):
		return false
	await _tap(KEY_M)
	return _check(scene.screen_name() == "main", "M goes back from the choice of players")

func _numbers_choose_entries() -> bool:
	await _tap(KEY_2)
	if not _check(scene.screen_name() == "controls", "2 opens the controls"):
		return false
	await _tap(KEY_ESCAPE)
	await _tap(KEY_1)
	return _check(scene.screen_name() == "survival_setup", "1 opens the choice of players")

## Starts on the choice of players with its first entry marked, and ends there.
## M leaves each run at once, during its opening cinematic.
func _cursor_chooses_each_player() -> bool:
	await _tap(KEY_S)
	if not _check(scene.screen_name() == "survival_setup", "stepping the player-choice cursor starts nothing"):
		return false
	await _tap(KEY_ENTER)
	if not _check(scene.screen_name() == "survival" and _actor("appraiser", "key_set") == 0 and _actor("chef", "key_set") == -1, "S then Enter on the choice of players plays the appraiser"):
		return false
	await _tap(KEY_M)
	await _tap(KEY_1)
	await _tap(KEY_UP)
	await _tap(KEY_W)
	await _tap(KEY_ENTER)
	if not _check(scene.screen_name() == "survival" and _actor("chef", "key_set") == 0 and _actor("appraiser", "key_set") == 1, "the choice keeps its cursor, and up then W wrap to two players"):
		return false
	await _tap(KEY_M)
	await _tap(KEY_1)
	await _tap(KEY_DOWN)
	await _tap(KEY_ENTER)
	if not _check(scene.screen_name() == "survival" and _actor("chef", "key_set") == 0 and _actor("appraiser", "key_set") == -1, "down wraps past two players back to the chef"):
		return false
	await _tap(KEY_M)
	await _tap(KEY_1)
	return _check(scene.screen_name() == "survival_setup", "1 opens the choice of players again")

## The controls page lists the pause screen's two-player rows, then the pause
## and leave keys.
func _controls_list_the_pause_keys() -> bool:
	await _tap(KEY_2)
	var controls: PackedStringArray = MainScene.controls_keys()
	await _tap(KEY_ESCAPE)
	await _tap(KEY_1)
	await _tap(KEY_3)
	if not _check(scene.screen_name() == "survival" and _actor("appraiser", "key_set") == 1, "3 starts two players"):
		return false
	var pause: Array = scene.survival_status()["pause_keys"]
	await _tap(KEY_M)
	var same: bool = controls.size() == pause.size() + 2
	for row: int in range(min(controls.size(), pause.size())):
		same = same and controls[row] == pause[row]
	if not _check(same and pause.size() > 0, "the controls page lists the pause screen's keys: %s against %s" % [controls, pause]):
		return false
	return _check(controls[-2] == "Pause or continue: Esc / Esc" and controls[-1] == "Leave the run; Esc cancels: M, then Enter / M, then Enter" and scene.screen_name() == "main", "the controls page ends with the pause and leave keys")

func _new_game_starts_a_run() -> bool:
	await _tap(KEY_1)
	await _tap(KEY_E)
	if not _check(scene.screen_name() == "survival" and _actor("chef", "key_set") == 0, "New game then 1 starts the survival game as the chef"):
		return false
	await _tap(KEY_M)
	await _tap(KEY_ENTER)
	return _check(scene.screen_name() == "main" and scene.survival_status().is_empty(), "leaving the run returns to the title screen")

## New game is marked after the run; two steps down mark Quit. The pass line
## prints from `_finalize` only.
func _quit_ends_the_game() -> void:
	await _tap(KEY_DOWN)
	await _tap(KEY_S)
	if not _check(scene.screen_name() == "main", "stepping the cursor opens nothing"):
		return
	awaiting_quit = true
	await _tap(KEY_ENTER)
	await _frames(30)
	_check(false, "Enter on Quit ends the game")

func _actor(who: String, field: String) -> int:
	var actor: Dictionary = scene.survival_status()[who]
	var value: int = actor[field]
	return value

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
	push_error("Title check failed: ", name, " ", scene.screen_name() if scene != null else "")
	failed = true
	quit(1)
	return false
