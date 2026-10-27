class_name GodotStartup
extends SceneTree
## Checks the configured main scene after Godot's normal startup.


func _process(_delta: float) -> bool:
	if current_scene == null or not current_scene.is_node_ready():
		push_error("Configured main scene did not become ready.")
		quit(1)
		return true

	print("Scene: ", current_scene.scene_file_path)
	print("Startup: 1 configured main scene ready.")
	quit(0)
	return true
