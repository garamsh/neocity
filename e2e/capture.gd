class_name AscentCapture
extends SceneTree
## Saves a frame of the configured shipped scene after it has drawn.

var frames: int = 0

func _process(_delta: float) -> bool:
	frames += 1
	if frames == 40:
		var destination: String = ProjectSettings.globalize_path("res://.tools/ascent-preview.png")
		var result: Error = root.get_texture().get_image().save_png(destination)
		if result != OK:
			push_error("Could not save gameplay frame")
			quit(1)
		else:
			print("Capture: ", destination)
			quit(0)
		return true
	return false
