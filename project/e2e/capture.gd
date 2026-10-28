class_name AscentCapture
extends SceneTree
## Saves a frame of the configured shipped scene after it has drawn.

var frames: int = 0

func _process(_delta: float) -> bool:
	frames += 1
	if frames == 2:
		_key(KEY_1, true)
	elif frames == 3:
		_key(KEY_1, false)
	elif frames == 40:
		var destination: String = ProjectSettings.globalize_path("user://ascent-preview.png")
		var result: Error = root.get_texture().get_image().save_png(destination)
		if result != OK:
			push_error("Could not save gameplay frame")
			quit(1)
		else:
			print("Capture: ", destination)
			quit(0)
		return true
	return false

func _key(code: Key, pressed: bool) -> void:
	var event: InputEventKey = InputEventKey.new()
	event.physical_keycode = code
	event.pressed = pressed
	Input.parse_input_event(event)
