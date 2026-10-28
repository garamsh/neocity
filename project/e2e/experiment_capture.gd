class_name PanExperimentsCapture
extends SceneTree
## Captures real rendered main-menu, developer-page, and pan dodge frames.

func _initialize() -> void:
	_run.call_deferred()

func _run() -> void:
	await _frames(2)
	if not await _save("main-menu"):
		return
	await _tap(KEY_2)
	if not await _save("developer"):
		return
	await _tap(KEY_1)
	_key(KEY_A, true)
	await _frames(150)
	_key(KEY_A, false)
	await _frames(110)
	if not await _save("pan-dodge"):
		return
	print("Capture: 3 rendered frames saved.")
	quit(0)

func _save(screen: String) -> bool:
	await RenderingServer.frame_post_draw
	var destination: String = ProjectSettings.globalize_path("user://" + screen + ".png")
	var result: Error = root.get_texture().get_image().save_png(destination)
	if result != OK:
		push_error("Could not save " + destination)
		quit(1)
		return false
	print("Capture: ", destination)
	return true

func _tap(code: Key) -> void:
	_key(code, true)
	await _frames(2)
	_key(code, false)
	await _frames(3)

func _frames(count: int) -> void:
	for frame in range(count):
		await process_frame

func _key(code: Key, pressed: bool) -> void:
	var event: InputEventKey = InputEventKey.new()
	event.physical_keycode = code
	event.pressed = pressed
	Input.parse_input_event(event)
