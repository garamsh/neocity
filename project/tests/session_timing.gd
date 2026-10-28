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
			Input.action_press("select_dodge")
		3:
			Input.action_release("select_dodge")
		5:
			Input.action_press("move_right")
		6:
			Input.action_press("menu")
		8:
			Input.action_release("move_right")
			Input.action_release("menu")
		10:
			Input.action_press("select_dodge")
		11:
			Input.action_release("select_dodge")
		13:
			if scene.screen_name() != "dodge" or scene.actor_position() != Vector2(-170.0, 130.0):
				return _fail("second direct run not fresh")
			Input.action_press("move_right")
		15:
			if scene.actor_position() != Vector2(-170.0, 130.0):
				return _fail("old partial tick leaked across menu selection")
		18:
			if scene.actor_position().x <= -169.0:
				return _fail("fresh partial ticks never accumulated into movement")
			Input.action_release("move_right")
		20:
			Input.action_press("menu")
		21:
			Input.action_release("menu")
		24:
			Input.action_press("select_ascent")
		25:
			Input.action_release("select_ascent")
		26:
			if scene.screen_name() != "ascent":
				return _fail("ascent not selected")
			ascent_start = scene.actor_position()
		27:
			Input.action_press("jump")
		28:
			Input.action_release("jump")
	if frames >= 29 and scene.actor_position().y < ascent_start.y - 1.0:
		print("Session: 3 integration checks passed (discard time, accumulate time, retain edge).")
		quit(0)
		return true
	if frames >= 34:
		return _fail("jump edge between rule ticks was lost")
	return false

func _fail(message: String) -> bool:
	push_error("Session timing check failed: " + message)
	quit(1)
	return true
