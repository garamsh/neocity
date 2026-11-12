class_name TitleMenuCheck
extends SceneTree
## Drives the title screen with real key events: each entry by cursor and by
## number, the controls page and back, the How to play page and back, the
## choice of players by cursor, the controls page against the pause screen's
## keys, a new game, the same entries with real mouse events, and quitting.

## How long, in milliseconds of the real clock, a panel takes to rise into
## place, with a margin over the 220 of src/rendering/drawing.cpp.
const ENTRANCE_MS: int = 300
## A word of each concept the How to play page explains, in its order: set
## out, move, collect, appraise, cook, feed, tool damage, a hated dish, acid,
## and the escape.
const HOW_TO_PLAY_WORDS: Array[String] = ["Set out", "jump", "Collect", "?", "cooks", "Feeding", "tool", "hated", "acid", "way out"]

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
		print("Title: ", checks, " live checks passed (entries, cursor, wrap, controls, back, shortcuts, how to play, player choice, controls keys, new game, mouse, quit).")

func _run() -> void:
	await _frames(2)
	scene = current_scene as MainScene
	if not _check(scene != null and scene.screen_name() == "main", "the game opens on the title screen"):
		return
	if not await _cursor_chooses_each_entry():
		return
	if not await _numbers_choose_entries():
		return
	if not await _how_to_play_explains_each_concept():
		return
	if not await _cursor_chooses_each_player():
		return
	if not await _new_game_starts_a_run():
		return
	if not await _controls_list_the_pause_keys():
		return
	if not await _mouse_drives_the_menus():
		return
	await _quit_ends_the_game()

func _cursor_chooses_each_entry() -> bool:
	if not _names_no_key("the title screen", "Click an entry to choose it. Controls lists every key."):
		return false
	await _tap(KEY_ENTER)
	if not _check(scene.screen_name() == "survival_setup", "Enter on the first entry, New game, opens the choice of players"):
		return false
	if not _names_no_key("the choice of players", "Click who is playing."):
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
	await _tap(KEY_ENTER)
	if not _check(scene.screen_name() == "settings", "W then up wrap from New game to Settings, the fifth and last entry"):
		return false
	await _tap(KEY_M)
	if not _check(scene.screen_name() == "main", "M goes back from the Settings page"):
		return false
	await _tap(KEY_DOWN)
	await _tap(KEY_ENTER)
	if not _check(scene.screen_name() == "survival_setup", "down from Settings wraps to New game"):
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

## Starts and ends on the choice of players. 3 opens the How to play page,
## which draws a line on each concept and names no key, and Esc and M go back.
func _how_to_play_explains_each_concept() -> bool:
	await _tap(KEY_ESCAPE)
	await _tap(KEY_3)
	if not _check(scene.screen_name() == "how_to_play", "3 opens the How to play page"):
		return false
	var lines: PackedStringArray = scene.get_meta("screen_text", PackedStringArray())
	var missing: Array[String] = []
	for row: int in range(HOW_TO_PLAY_WORDS.size()):
		# The title, the lead line, then one line per concept.
		if lines.size() <= row + 2 or not lines[row + 2].contains(HOW_TO_PLAY_WORDS[row]):
			missing.append(HOW_TO_PLAY_WORDS[row])
	if not _check(missing.is_empty(), "the How to play page draws a line on each concept, in order: missing %s in %s" % [missing, lines]):
		return false
	if not _names_no_key("the How to play page", "Back to the title"):
		return false
	await _tap(KEY_ESCAPE)
	if not _check(scene.screen_name() == "main", "Esc goes back from the How to play page"):
		return false
	await _tap(KEY_3)
	await _tap(KEY_M)
	if not _check(scene.screen_name() == "main", "M goes back from the How to play page"):
		return false
	await _tap(KEY_1)
	return _check(scene.screen_name() == "survival_setup", "1 opens the choice of players after the How to play page")

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

## A new game as the chef, left first by a click on the question's leave as it
## is drawn while it still rises into place, then by M and Enter.
func _new_game_starts_a_run() -> bool:
	await _tap(KEY_1)
	await _tap(KEY_E)
	if not _check(scene.screen_name() == "survival" and _actor("chef", "key_set") == 0, "New game then 1 starts the survival game as the chef"):
		return false
	# The first question of the process rises into place on the real clock, and
	# headless frames take far less than its 220 ms.
	_key(KEY_M, true)
	await _frames(1)
	_key(KEY_M, false)
	await _frames(1)
	var rising: Rect2 = scene.get_meta("interruption_button", Rect2())
	await _click_at(Vector2(rising.position.x + 12.0, rising.end.y - 2.0))
	var left_by_click: bool = scene.screen_name() == "main"
	await _tap(KEY_1)
	await _tap(KEY_1)
	await _tap(KEY_E)
	await _tap(KEY_M)
	OS.delay_msec(ENTRANCE_MS)
	await _frames(2)
	var risen: Rect2 = scene.get_meta("interruption_button", Rect2())
	if not _check(risen.has_area() and rising.size == risen.size and rising.position.y >= risen.position.y + 3.0, "setup: the question's button was drawn still rising into place: %s, resting at %s" % [rising, risen]):
		return false
	if not _check(left_by_click, "a click on the bottom of leave, as drawn while the question rises, leaves the run"):
		return false
	await _tap(KEY_ENTER)
	return _check(scene.screen_name() == "main" and scene.survival_status().is_empty(), "leaving the run returns to the title screen")

## The entry under the mouse lights, and a click chooses what its number key
## chooses; the controls page's way back goes back. The mouse leaves the
## entries before the keys take over again.
func _mouse_drives_the_menus() -> bool:
	_point_at(_target("option_2"))
	await _frames(4)
	if not _check(_hovered() == "option_2", "the entry under the mouse lights"):
		return false
	if not _check(await _click("option_2") and scene.screen_name() == "controls", "a click on Controls opens the controls"):
		return false
	if not _check(await _click("menu") and scene.screen_name() == "main", "a click on the way back returns to the title"):
		return false
	if not _check(await _click("option_3") and scene.screen_name() == "how_to_play", "a click on How to play opens its page"):
		return false
	if not _check(await _click("menu") and scene.screen_name() == "main", "a click on the How to play page's way back returns to the title"):
		return false
	if not _check(await _click("option_1") and scene.screen_name() == "survival_setup", "a click on New game opens the choice of players"):
		return false
	if not _check(await _click("option_3") and scene.screen_name() == "survival" and _actor("appraiser", "key_set") == 1, "a click on Two players starts two players"):
		return false
	# The run opens on its intro, which M leaves at once.
	await _tap(KEY_M)
	_point_at({"rect": Rect2(0, 0, 1, 1)})
	await _frames(4)
	return _check(scene.screen_name() == "main" and _hovered() == "", "the run is left and nothing is lit away from the entries")

## New game is marked after the run; three steps down mark Quit. The pass
## line prints from `_finalize` only.
func _quit_ends_the_game() -> void:
	await _tap(KEY_DOWN)
	await _tap(KEY_S)
	await _tap(KEY_DOWN)
	if not _check(scene.screen_name() == "main", "stepping the cursor opens nothing"):
		return
	awaiting_quit = true
	await _tap(KEY_ENTER)
	await _frames(30)
	_check(false, "Enter on Quit ends the game")

## Whether the last frame drew `hint` and no Enter, Esc, W/S or M: key, as
## the screen's keys are left to the controls page.
func _names_no_key(where: String, hint: String) -> bool:
	var lines: PackedStringArray = scene.get_meta("screen_text", PackedStringArray())
	var keyed: Array[String] = []
	for line: String in lines:
		for key: String in ["Enter", "Esc", "W/S", "M:"]:
			if line.contains(key):
				keyed.append(line)
	return _check(lines.has(hint) and keyed.is_empty(), "%s names no key: %s" % [where, keyed])

func _actor(who: String, field: String) -> int:
	var actor: Dictionary = scene.survival_status()[who]
	var value: int = actor[field]
	return value

## The target the last frame drew whose click presses `press`; empty with none.
func _target(press: String) -> Dictionary:
	var targets: Array = scene.get_meta("pointer_targets", [])
	for target: Dictionary in targets:
		if target["press"] == press:
			return target
	return {}

## What the target under the mouse presses; empty over none.
func _hovered() -> String:
	var targets: Array = scene.get_meta("pointer_targets", [])
	for target: Dictionary in targets:
		if target["hovered"]:
			return str(target["press"])
	return ""

## Moves the mouse to the middle of `target`, a point on the screen the scene
## draws, through the window's stretch.
func _point_at(target: Dictionary) -> void:
	var area: Rect2 = target["rect"]
	var event: InputEventMouseMotion = InputEventMouseMotion.new()
	event.position = root.get_final_transform() * area.get_center()
	event.global_position = event.position
	Input.parse_input_event(event)

## Points at the target pressing `press` and clicks it with the left button;
## false when the screen shows no such target.
func _click(press: String) -> bool:
	var target: Dictionary = _target(press)
	if target.is_empty():
		return false
	_point_at(target)
	await _frames(2)
	var area: Rect2 = target["rect"]
	await _click_at(area.get_center())
	return true

## Clicks the left button at `point` on the screen the scene draws, moving the
## mouse there in the same frame.
func _click_at(point: Vector2) -> void:
	var motion: InputEventMouseMotion = InputEventMouseMotion.new()
	motion.position = root.get_final_transform() * point
	motion.global_position = motion.position
	Input.parse_input_event(motion)
	for pressed: bool in [true, false]:
		var event: InputEventMouseButton = InputEventMouseButton.new()
		event.button_index = MOUSE_BUTTON_LEFT
		event.pressed = pressed
		event.position = motion.position
		event.global_position = event.position
		Input.parse_input_event(event)
		await _frames(2 if pressed else 8)

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
