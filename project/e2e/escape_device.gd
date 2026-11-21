class_name EscapeDeviceCheck
extends SceneTree
## Walks both humans from the first base to the broken device on the aquarium's service roof on real
## keys, searches the three containers that hold its parts, presses the interact key at the device with
## two parts, which must change nothing, and with three, which must end the run as escaped. Headless.

## The first day the device works is set to day one so the walk does not wait for day ten.
const ESCAPE_DAY: String = "1"
const DEVICE: Vector2i = Vector2i(6, 4)
const PARTS_NEEDED: int = 3
## Each stop of the route, and what the chef searches there: the cold store's upper shelf, the aquarium's
## service shelf and its roof crate, in the order the route passes them.
const STREET: Array[Vector2i] = [
	Vector2i(11, 9), Vector2i(13, 9), Vector2i(16, 9), Vector2i(17, 8), Vector2i(29, 9), Vector2i(31, 9),
	Vector2i(35, 9), Vector2i(36, 8), Vector2i(38, 8), Vector2i(40, 7), Vector2i(44, 6), Vector2i(47, 6),
	Vector2i(47, 4), Vector2i(39, 4),
]
const TO_AQUARIUM: Array[Vector2i] = [
	Vector2i(47, 4), Vector2i(47, 6), Vector2i(49, 8), Vector2i(50, 7), Vector2i(51, 6), Vector2i(52, 5),
	Vector2i(60, 5), Vector2i(60, 11), Vector2i(62, 11),
]
const TO_SERVICE_SHELF: Array[Vector2i] = [Vector2i(9, 10), Vector2i(9, 7), Vector2i(6, 7)]
const TO_DEVICE: Array[Vector2i] = [Vector2i(4, 7), Vector2i(4, 4), DEVICE]
const ROOF_CRATE: Vector2i = Vector2i(8, 4)

var scene: MainScene
var clock: float = 0.0
var held_keys: Dictionary[Key, bool] = {}

func _initialize() -> void:
	Engine.max_fps = 60
	_run.call_deferred()

func _run() -> void:
	await process_frame
	scene = current_scene as MainScene
	scene.pin_survival_seed(11)
	scene.pin_scene_clock(clock)
	OS.set_environment("NEOCITY_BASE_SECONDS", "0")
	OS.set_environment("NEOCITY_COOK_SECONDS", "0")
	OS.set_environment("NEOCITY_EXPLORATION_SECONDS", "300")
	OS.set_environment("NEOCITY_ESCAPE_DAY", ESCAPE_DAY)
	await _tap(KEY_1)
	await _tap(KEY_3)
	await _tap(KEY_E)
	await _tap(KEY_Q)
	await _tap(KEY_U)
	var state: Dictionary = scene.survival_status()
	if state["phase"] != "exploration" or state["day"] != 1:
		_abort("the run did not set out on day 1: %s" % [_describe(state)])
		return
	for target: Vector2i in STREET:
		if not await _move(target):
			return
	if not await _collect(1):
		return
	for target: Vector2i in TO_AQUARIUM:
		if not await _move(target):
			return
	for target: Vector2i in TO_SERVICE_SHELF:
		if not await _move(target):
			return
	if not await _collect(2):
		return
	for target: Vector2i in TO_DEVICE:
		if not await _move(target):
			return
	if not await _use_device_refused():
		return
	if not await _move(ROOF_CRATE) or not await _collect(3):
		return
	if not await _move(DEVICE):
		return
	await _tap(KEY_E)
	state = scene.survival_status()
	if state.get("ending", "") != "escaped":
		_abort("three parts at the device on day %s did not end the run as escaped: %s" % [state["day"], _describe(state)])
		return
	print("Escape device: finished: pass; the run ended as escaped on day %d with %d parts." % [state["day"], PARTS_NEEDED])
	quit(0)

## With two parts, the interact key at the device ends nothing and takes nothing, the control for the
## acceptance that follows: the same place, the same day and the same key, one part fewer.
func _use_device_refused() -> bool:
	var before: Dictionary = scene.survival_status()
	var chef: Dictionary = before["chef"]
	var held: int = chef["parts"]
	if held != PARTS_NEEDED - 1 or _cell(chef) != DEVICE:
		_abort("the chef did not stand at the device with two parts: %s" % [_describe(before)])
		return false
	await _tap(KEY_E)
	var after: Dictionary = scene.survival_status()
	if after.has("ending") or after["phase"] != "exploration" or after["chef"]["parts"] != held:
		_abort("two parts at the device changed the run: %s" % [_describe(after)])
		return false
	return true

## The chef opens the container at its feet, takes everything in it and closes it, and has `parts` parts after.
func _collect(parts: int) -> bool:
	await _tap(KEY_R)
	var chef: Dictionary = scene.survival_status()["chef"]
	if _count(chef, "searching_items") < 1:
		_abort("no container with items opened at %s: %s" % [_cell(chef), chef])
		return false
	for attempt: int in range(12):
		var open_chef: Dictionary = scene.survival_status()["chef"]
		if _count(open_chef, "searching_items") < 1:
			break
		await _tap(KEY_E)
	await _tap(KEY_R)
	chef = scene.survival_status()["chef"]
	if _count(chef, "searching") != -1 or _count(chef, "parts") != parts:
		_abort("searching did not leave the chef %d parts with the container closed: %s" % [parts, chef])
		return false
	return true

func _count(body: Dictionary, field: String) -> int:
	var value: int = body[field]
	return value

func _cell(body: Dictionary) -> Vector2i:
	var column: int = body["x"]
	var row: int = body["y"]
	return Vector2i(column, row)

func _describe(state: Dictionary) -> String:
	return "day=%s phase=%s map=%s chef=%s appraiser=%s ending=%s" % [state["day"], state["phase"], state["map"], state["chef"], state["appraiser"], state.get("ending", "none")]

## Walks the chef and the appraiser to `target` on real keys.
func _move(target: Vector2i) -> bool:
	var last: Dictionary[String, int] = {}
	var stalled: Dictionary[String, int] = {}
	for frame: int in range(900):
		var state: Dictionary = scene.survival_status()
		var reached: bool = true
		for role: String in ["chef", "appraiser"]:
			var body: Dictionary = state[role]
			var column: int = body["x"]
			var row: int = body["y"]
			var at: Vector2i = Vector2i(column, row)
			var standing: String = body["stance"]
			var crossed: bool = state["map"] == "Aquarium" and target == Vector2i(62, 11)
			var done: bool = crossed or (at == target and standing != "airborne")
			reached = reached and done
			var left: Key = KEY_A if role == "chef" else KEY_J
			var right: Key = KEY_D if role == "chef" else KEY_L
			var up: Key = KEY_W if role == "chef" else KEY_I
			var down: Key = KEY_S if role == "chef" else KEY_K
			var jump: Key = KEY_SPACE if role == "chef" else KEY_SEMICOLON
			var run: Key = KEY_SHIFT if role == "chef" else KEY_H
			var sideways: bool = not done and target.x != at.x
			_key(left, sideways and target.x < at.x)
			_key(right, sideways and target.x > at.x)
			_key(up, not done and not sideways and target.y < at.y)
			_key(down, not done and not sideways and target.y > at.y)
			_key(run, sideways and absi(target.x - at.x) > 3)
			var sub_x: int = body["sub_x"]
			stalled[role] = stalled.get(role, 0) + 1 if sideways and sub_x == last.get(role, -1) and standing == "standing" else 0
			last[role] = sub_x
			var gap_takeoff: bool = target.y < at.y and target.x - at.x == 2 and sub_x % 32 >= 16
			var climb_step: bool = target.y < at.y and absi(target.x - at.x) <= 1
			_key(jump, not done and standing == "standing" and (gap_takeoff or climb_step or stalled[role] > 6))
		if reached:
			_release_movement()
			await process_frame
			await process_frame
			return true
		if state["phase"] != "exploration" or state["chef"]["location"] != "city" or state["appraiser"]["location"] != "city":
			_abort("a human left the city before %s: %s" % [target, _describe(state)])
			return false
		clock += 0.1
		scene.pin_scene_clock(clock)
		await process_frame
	_abort("did not reach %s: %s" % [target, _describe(scene.survival_status())])
	return false

func _abort(reason: String) -> void:
	push_error(reason)
	print("Escape device: finished: fail; ", reason)
	quit(1)

func _release_movement() -> void:
	for code: Key in [KEY_A, KEY_D, KEY_W, KEY_S, KEY_SPACE, KEY_SHIFT, KEY_J, KEY_L, KEY_I, KEY_K, KEY_SEMICOLON, KEY_H]:
		_key(code, false)

func _tap(code: Key) -> void:
	_key(code, true)
	await process_frame
	await process_frame
	_key(code, false)
	for frame: int in range(8):
		await process_frame

func _key(code: Key, pressed: bool) -> void:
	if held_keys.get(code, false) == pressed:
		return
	held_keys[code] = pressed
	var event: InputEventKey = InputEventKey.new()
	event.physical_keycode = code
	event.pressed = pressed
	Input.parse_input_event(event)
