class_name SurvivalCapture
extends SceneTree
## Captures real rendered frames of a played survival run into user://survival/.

var scene: AscentScene
var held: Dictionary[Key, bool] = {}

func _initialize() -> void:
	_run.call_deferred()

func _run() -> void:
	await _frames(2)
	scene = current_scene as AscentScene
	scene.pin_survival_seed(11)
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("user://survival"))
	await _save("1-main-menu")
	await _tap(KEY_1)
	await _save("2-choose-players")
	await _tap(KEY_1)
	await _save("3-base-day-1")
	await _tap(KEY_Q)
	await _walk(KEY_D, 6)
	await _save("4-city-collect-prompt")
	await _tap(KEY_E)
	await _walk(KEY_D, 19)
	await _frames(30)
	await _save("5-city-after-drip")
	await _until_phase("workshop")
	await _frames(60)
	await _save("6-workshop-after-return")
	await _until_next("Cook", 300)
	await _save("7-workshop-cook-offered")
	for attempt: int in range(4):
		await _tap(KEY_E)
		if _int("foods") > 0:
			break
	await _save("8-workshop-dish-ready")
	await _tap(KEY_F)
	await _frames(20)
	await _save("9-next-day")
	await _tap(KEY_Q)
	await _until_phase("workshop")
	await _frames(60)
	await _tap(KEY_E)
	await _tap(KEY_E)
	await _tap(KEY_F)
	await _frames(12)
	await _tap(KEY_G)
	await _frames(12)
	await _tap(KEY_G)
	await _frames(12)
	await _tap(KEY_G)
	await _frames(12)
	await _save("10-workshop-acid-rose")
	await _tap(KEY_Q)
	await _frames(40)
	await _walk(KEY_D, 10)
	await _frames(12)
	await _save("11-city-damaged")
	print("Capture: ", ProjectSettings.globalize_path("user://survival"))
	quit(0)

func _walk(code: Key, x: int) -> void:
	_key(code, true)
	for frame: int in range(240):
		var chef: Dictionary = scene.survival_status()["chef"]
		var at: int = chef["x"]
		if at >= x:
			break
		await process_frame
	_key(code, false)
	await _frames(3)

func _until_phase(phase: String) -> void:
	for frame: int in range(3000):
		var now: String = scene.survival_status()["phase"]
		if now == phase:
			return
		await process_frame

func _until_next(text: String, limit: int) -> void:
	for frame: int in range(limit):
		var now: String = scene.survival_status()["next"]
		if now.contains(text):
			return
		await process_frame

func _int(field: String) -> int:
	var value: int = scene.survival_status().get(field, 0)
	return value

func _save(screen: String) -> void:
	await RenderingServer.frame_post_draw
	var destination: String = ProjectSettings.globalize_path("user://survival/" + screen + ".png")
	if root.get_texture().get_image().save_png(destination) != OK:
		push_error("Could not save " + destination)
		quit(1)
		return
	print("Capture: ", destination)

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
