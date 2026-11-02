class_name GodotStartup
extends SceneTree
## Checks the application name and the configured main scene after Godot's
## normal startup.

## The game's name, which Godot shows as the window title and names the user
## data directory after.
const APPLICATION_NAME: String = "Monster Mouth Escape"


func _process(_delta: float) -> bool:
	var application_name: String = ProjectSettings.get_setting("application/config/name")
	if application_name != APPLICATION_NAME:
		push_error("Application name is \"%s\", not \"%s\"." % [application_name, APPLICATION_NAME])
		quit(1)
		return true

	if current_scene == null or not current_scene.is_node_ready():
		push_error("Configured main scene did not become ready.")
		quit(1)
		return true

	print("Application: ", application_name)
	print("Scene: ", current_scene.scene_file_path)
	print("Startup: 1 configured main scene ready.")
	quit(0)
	return true
