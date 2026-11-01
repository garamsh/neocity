class_name AscentGameplayCheck
extends SceneTree
## Exercises live input and the actual C++ scene through Godot's normal startup.

var frames: int = 0
var start: Vector2
var moved: Vector2
var scene: AscentScene

func _process(_delta: float) -> bool:
	frames += 1
	if frames == 2:
		_key(KEY_3, true)
	elif frames == 3:
		_key(KEY_3, false)
	elif frames == 5:
		scene = current_scene as AscentScene
		if scene == null or scene.screen_name() != "ascent":
			push_error("Configured C++ scene is missing")
			quit(1)
			return true
		start = scene.actor_position()
		_key(KEY_D, true)
	elif frames == 35:
		_key(KEY_D, false)
		moved = scene.actor_position()
		if moved.x <= start.x + 30.0:
			push_error("Live horizontal input did not move the actor")
			quit(1)
			return true
		_key(KEY_SPACE, true)
	elif frames == 36:
		_key(KEY_SPACE, false)
	elif frames == 46:
		var jumping: Vector2 = scene.actor_position()
		if jumping.y >= moved.y - 10.0:
			push_error("Live jump input did not lift the actor")
			quit(1)
			return true
		_key(KEY_R, true)
	elif frames == 47:
		_key(KEY_R, false)
	elif frames == 51:
		var recovered: Vector2 = scene.actor_position()
		if recovered.distance_to(start) > 2.0:
			push_error("Retry did not restore the start/checkpoint")
			quit(1)
			return true
		print("Gameplay: 3 live input checks passed (move, jump, retry).")
		quit(0)
		return true
	return false

func _key(code: Key, pressed: bool) -> void:
	var event: InputEventKey = InputEventKey.new()
	event.physical_keycode = code
	event.pressed = pressed
	Input.parse_input_event(event)
