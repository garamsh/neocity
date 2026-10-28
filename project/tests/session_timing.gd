class_name SessionTimingCheck
extends SceneTree
## Exercises sub-tick input and transition timing through the shipped composition.

var frames: int = 0
var scene: AscentScene
var ascent_start: Vector2

func _process(_delta: float) -> bool:
	frames += 1
	match frames:
		2:
			scene = current_scene as AscentScene
			if scene == null:
				return _fail("configured scene missing")
			Input.action_press("option_1")
		3:
			Input.action_release("option_1")
		5:
			if scene.screen_name() != "ascent":
				return _fail("ascent not selected")
			ascent_start = scene.actor_position()
			Input.action_press("move_right")
		6:
			Input.action_press("menu")
		7:
			Input.action_release("move_right")
			Input.action_release("menu")
		10:
			Input.action_press("option_1")
		11:
			Input.action_release("option_1")
		13:
			if scene.screen_name() != "ascent" or scene.actor_position() != ascent_start:
				return _fail("second ascent run not fresh")
			Input.action_press("move_right")
		15:
			if scene.actor_position() != ascent_start:
				return _fail("old partial tick leaked across menu selection")
		20:
			if scene.actor_position().x <= ascent_start.x:
				return _fail("fresh partial ticks never accumulated into movement")
			Input.action_release("move_right")
		26:
			ascent_start = scene.actor_position()
			Input.action_press("jump")
		27:
			Input.action_release("jump")
	if frames >= 28 and scene.actor_position().y < ascent_start.y - 1.0:
		print("Session: 3 integration checks passed (discard time, accumulate time, retain edge).")
		quit(0)
		return true
	if frames >= 33:
		return _fail("jump edge between rule ticks was lost")
	return false

func _fail(message: String) -> bool:
	push_error("Session timing check failed: " + message)
	quit(1)
	return true
