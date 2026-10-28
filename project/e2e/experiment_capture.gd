class_name PanExperimentsCapture
extends SceneTree
## Captures real rendered menu and experiment frames from the shipped scene.

func _initialize() -> void:
	_run.call_deferred()

func _run() -> void:
	await process_frame
	await process_frame
	if not await _save("menu"):
		return
	_key(KEY_1, true)
	await process_frame
	await process_frame
	_key(KEY_1, false)
	await process_frame
	await process_frame
	if not await _save("direct"):
		return
	_key(KEY_M, true)
	await process_frame
	await process_frame
	_key(KEY_M, false)
	await process_frame
	await process_frame
	_key(KEY_2, true)
	await process_frame
	await process_frame
	_key(KEY_2, false)
	await process_frame
	await process_frame
	_key(KEY_W, true)
	for frame in range(30):
		await process_frame
	if not await _save("tilt"):
		return
	_key(KEY_W, false)
	print("Capture: 3 rendered experiment frames saved.")
	quit(0)

func _save(screen: String) -> bool:
	await RenderingServer.frame_post_draw
	var destination: String = ProjectSettings.globalize_path("user://pan-" + screen + ".png")
	var result: Error = root.get_texture().get_image().save_png(destination)
	if result != OK:
		push_error("Could not save " + destination)
		quit(1)
		return false
	print("Capture: ", destination)
	return true

func _key(code: Key, pressed: bool) -> void:
	var event: InputEventKey = InputEventKey.new()
	event.physical_keycode = code
	event.pressed = pressed
	Input.parse_input_event(event)
