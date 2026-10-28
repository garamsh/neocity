class_name PanExperimentsCheck
extends SceneTree
## Drives the shipped menus and the pan dodge experiment with real Godot key events.

const DIRECT_SCENE: String = "res://experiment/pan_dodge/pan_dodge.tscn"

var scene: AscentScene
var held: Dictionary[Key, bool] = {}
var checks: int = 0
var despawned: bool = false

func _initialize() -> void:
	_run.call_deferred()

func _run() -> void:
	await _frames(2)
	scene = current_scene as AscentScene
	if not _check(scene != null and scene.screen_name() == "main", "configured startup main menu"):
		return
	if not _check(InputMap.has_action("option_1") and InputMap.has_action("option_9"), "every developer-page number has an action"):
		return
	await _tap(KEY_2)
	if not _check(scene.screen_name() == "developer", "main menu opens the developer page"):
		return
	await _tap(KEY_M)
	if not _check(scene.screen_name() == "main", "developer page returns to the main menu"):
		return
	await _tap(KEY_2)
	_key(KEY_D, true)
	await _tap(KEY_1)
	await _frames(20)
	if not _check(scene.screen_name() == "pan_dodge" and _dodge() == null, "experiment waits while a control is held"):
		return
	_release()
	await _frames(3)
	var dodge: PanDodgeScene = _dodge()
	if not _check(dodge != null and dodge.run_tick() <= 4 and absf(dodge.player_position().x) < 3.0, "release mounts a fresh run without the held input"):
		return
	var start: Vector2 = dodge.player_position()
	var tilt: Vector2 = dodge.pan_tilt()
	var exit: float = dodge.exit_angle()
	_key(KEY_W, true)
	await _frames(30)
	_key(KEY_W, false)
	if not _check(dodge.player_position().y < start.y - 25.0, "direct movement"):
		return
	if not _check(dodge.pan_tilt() != tilt and dodge.exit_angle() > exit, "the pan sways and its exit travels"):
		return
	await _tap(KEY_R)
	if not _check(dodge.run_tick() <= 4 and dodge.outcome() == "playing" and dodge.hazard_count() == 0, "retry starts a fresh run"):
		return
	await _until_outcome(dodge, 600)
	if not _check(dodge.outcome() == "hit" and dodge.hazard_count() >= 1, "an idle player is hit by the dropped hazard"):
		return
	await _tap(KEY_R)
	await _chase_exit(dodge)
	if not _check(dodge.outcome() == "escaped", "steering through the travelling exit escapes"):
		return
	await _tap(KEY_R)
	var spawned: int = await _orbit(dodge, 1000)
	if not _check(dodge.outcome() == "playing" and spawned >= 3, "hazards keep dropping during a dodging run"):
		return
	if not _check(despawned, "spawned hazards despawn"):
		return
	await _tap(KEY_M)
	await _frames(2)
	if not _check(scene.screen_name() == "developer" and not is_instance_valid(dodge) and _dodge() == null, "M unmounts the experiment back to the developer page"):
		return
	await _tap(KEY_M)
	await _tap(KEY_1)
	if not _check(scene.screen_name() == "ascent", "main menu still plays the city climb"):
		return
	await _tap(KEY_M)
	if not _check(scene.screen_name() == "main", "the city climb returns to the main menu"):
		return
	change_scene_to_file(DIRECT_SCENE)
	await _frames(4)
	dodge = current_scene as PanDodgeScene
	if not _check(dodge != null and dodge.run_tick() > 0, "the experiment scene runs directly"):
		return
	start = dodge.player_position()
	_key(KEY_W, true)
	await _frames(30)
	_key(KEY_W, false)
	if not _check(dodge.player_position().y < start.y - 25.0, "direct launch takes the same controls"):
		return
	print("Experiments: ", checks, " live checks passed (menus, mount, move, sway, loss, win, spawn, despawn, direct launch).")
	quit(0)

func _dodge() -> PanDodgeScene:
	return scene.get_node_or_null("Experiment") as PanDodgeScene

func _chase_exit(dodge: PanDodgeScene) -> void:
	for tick in range(600):
		if dodge.outcome() != "playing":
			break
		var angle: float = dodge.exit_angle()
		_drive(Vector2(cos(angle), sin(angle)) * 290.0 - dodge.player_position())
		await process_frame
	_release()

func _orbit(dodge: PanDodgeScene, ticks: int) -> int:
	var most: int = 0
	for tick in range(ticks):
		if dodge.outcome() != "playing":
			break
		var position: Vector2 = dodge.player_position()
		var radial: float = 0.0 if position.length() < 1.0 else 150.0 - position.length()
		_drive(Vector2(-position.y, position.x) + position / 150.0 * radial)
		var count: int = dodge.hazard_count()
		await process_frame
		despawned = despawned or dodge.hazard_count() < count
		most = maxi(most, count)
	_release()
	return most

func _drive(force: Vector2) -> void:
	_key(KEY_A, force.x < -3.0)
	_key(KEY_D, force.x > 3.0)
	_key(KEY_W, force.y < -3.0)
	_key(KEY_S, force.y > 3.0)

func _until_outcome(dodge: PanDodgeScene, ticks: int) -> void:
	for tick in range(ticks):
		if dodge.outcome() != "playing":
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
