class_name PanExperimentsCheck
extends SceneTree
## Drives the shipped menu and both engines using real Godot key events.

var scene: AscentScene
var held: Dictionary[Key, bool] = {}
var checks: int = 0

func _initialize() -> void:
	_run.call_deferred()

func _run() -> void:
	await process_frame
	await process_frame
	scene = current_scene as AscentScene
	if not _check(scene != null and scene.screen_name() == "menu", "configured startup menu"):
		return
	await _tap(KEY_1)
	if not _check(scene.screen_name() == "dodge", "direct selection"):
		return
	var start: Vector2 = scene.actor_position()
	_key(KEY_W, true)
	await _frames(30)
	_key(KEY_W, false)
	if not _check(scene.actor_position().y < start.y - 40.0, "direct movement"):
		return
	await _tap(KEY_R)
	if not _check(scene.actor_position().distance_to(start) < 0.01 and scene.outcome() == "playing", "direct restart"):
		return
	_key(KEY_D, true)
	await _until_outcome()
	_key(KEY_D, false)
	if not _check(scene.outcome() == "hit", "direct contact loss"):
		return
	await _tap(KEY_R)
	await _direct_win()
	if not _check(scene.outcome() == "escaped", "direct escape route"):
		return
	await _tap(KEY_R)
	if not _check(scene.actor_position().distance_to(start) < 0.01 and scene.outcome() == "playing", "restart after direct win"):
		return
	_key(KEY_D, true)
	await _frames(12)
	_key(KEY_R, true)
	_key(KEY_M, true)
	await _frames(2)
	_key(KEY_M, false)
	_key(KEY_2, true)
	await _frames(20)
	if not _check(scene.screen_name() == "menu", "held transition input is discarded"):
		return
	_release()
	await _frames(3)
	await _tap(KEY_2)
	if not _check(scene.screen_name() == "tilt", "tilt selection after release"):
		return
	start = scene.actor_position()
	var hazard_start: Vector2 = scene.hazard_position()
	await _frames(20)
	if not _check(scene.actor_position() == start and scene.pan_tilt() == Vector2.ZERO and scene.actor_velocity() == Vector2.ZERO, "fresh session has no old movement or tilt"):
		return
	_key(KEY_D, true)
	await _frames(30)
	_key(KEY_D, false)
	if not _check(scene.actor_position().x > start.x + 1.0 and scene.hazard_position().x > hazard_start.x + 1.0 and scene.pan_tilt().x > 0.5, "shared tilt moves player and hazard"):
		return
	await _tap(KEY_R)
	if not _check(scene.actor_position() == start and scene.hazard_position() == hazard_start and scene.actor_velocity() == Vector2.ZERO and scene.pan_tilt() == Vector2.ZERO, "tilt reset clears every body"):
		return
	_key(KEY_A, true)
	await _until_outcome()
	_key(KEY_A, false)
	if not _check(scene.outcome() == "hit", "tilt contact loss"):
		return
	await _tap(KEY_R)
	await _tilt_win()
	if not _check(scene.outcome() == "escaped", "tilt escape route"):
		return
	await _tap(KEY_R)
	if not _check(scene.actor_position() == start and scene.outcome() == "playing" and scene.actor_velocity() == Vector2.ZERO, "restart after tilt win"):
		return
	_key(KEY_W, true)
	await _frames(12)
	await _tap(KEY_M)
	_release()
	await _frames(3)
	await _tap(KEY_1)
	await _frames(20)
	if not _check(scene.screen_name() == "dodge" and scene.actor_position() == Vector2(-170.0, 130.0) and scene.outcome() == "playing", "switch back discards old state and held input"):
		return
	await _tap(KEY_M)
	await _tap(KEY_3)
	if not _check(scene.screen_name() == "ascent", "legacy ascent remains selectable"):
		return
	await _tap(KEY_M)
	if not _check(scene.screen_name() == "menu", "legacy ascent returns to common menu"):
		return
	print("Experiments: ", checks, " live checks passed (menu, direct, tilt, loss, win, replay, switch).")
	quit(0)

func _direct_win() -> void:
	var waypoints: Array[Vector2] = [Vector2(-170.0, -160.0), Vector2(0.0, -160.0), Vector2(0.0, -270.0)]
	var waypoint: int = 0
	for tick in range(800):
		if scene.outcome() != "playing":
			break
		var target: Vector2 = waypoints[waypoint]
		var position: Vector2 = scene.actor_position()
		if waypoint < 2 and position.distance_to(target) < 3.0:
			waypoint += 1
			target = waypoints[waypoint]
		_drive(target - position, 1.0)
		await process_frame
	_release()

func _tilt_win() -> void:
	var waypoints: Array[Vector2] = [Vector2(-170.0, -135.0), Vector2(0.0, -170.0), Vector2(0.0, -280.0)]
	var waypoint: int = 0
	for tick in range(1400):
		if scene.outcome() != "playing":
			break
		var target: Vector2 = waypoints[waypoint]
		var position: Vector2 = scene.actor_position()
		if waypoint < 2 and position.distance_to(target) < 25.0:
			waypoint += 1
			target = waypoints[waypoint]
		_drive(2.0 * (target - position) - scene.actor_velocity(), 0.1)
		await process_frame
	_release()

func _drive(force: Vector2, deadzone: float) -> void:
	_key(KEY_A, force.x < -deadzone)
	_key(KEY_D, force.x > deadzone)
	_key(KEY_W, force.y < -deadzone)
	_key(KEY_S, force.y > deadzone)

func _until_outcome() -> void:
	for tick in range(600):
		if scene.outcome() != "playing":
			return
		await process_frame

func _tap(code: Key) -> void:
	_key(code, true)
	await _frames(2)
	_key(code, false)
	await _frames(3)

func _frames(count: int) -> void:
	for frame in range(count):
		await process_frame

func _key(code: Key, pressed: bool) -> void:
	if held.get(code, false) == pressed:
		return
	held[code] = pressed
	var event: InputEventKey = InputEventKey.new()
	event.physical_keycode = code
	event.pressed = pressed
	Input.parse_input_event(event)

func _release() -> void:
	for code: Key in held:
		_key(code, false)

func _check(condition: bool, description: String) -> bool:
	if not condition:
		push_error("Experiment check failed: " + description)
		_release()
		quit(1)
		return false
	checks += 1
	return true
