class_name SaveSlotCheck
extends SceneTree
## Starts a new game with real key events and reads that the run wrote its day
## slot. Run it with the system libstdc++ preloaded, as a display's libraries
## leave it in the process: the extension keeps its own copy of the library,
## and its iostream facets must not bind to the other one.

const WAIT_FRAMES: int = 4
const HOLD_FRAMES: int = 8

var scene: MainScene

func _initialize() -> void:
	_run.call_deferred()

func _run() -> void:
	for frame: int in range(3):
		await process_frame
	scene = current_scene as MainScene
	if scene == null or scene.screen_name() != "main":
		_finish("the title did not open")
		return
	# A fresh data directory has no slot: New game is the first entry, then Chef.
	await _tap(KEY_1)
	await _tap(KEY_1)
	await _tap(KEY_E)
	if scene.screen_name() != "survival":
		_finish("a new game did not start a run")
		return
	if not FileAccess.file_exists("user://day.save"):
		_finish("the run did not write its day slot")
		return
	print("Save slot: 2 live checks passed (a new game's day one is written).")
	quit(0)

func _finish(reason: String) -> void:
	push_error("Save slot check failed: ", reason)
	quit(1)

func _tap(code: Key) -> void:
	for pressed: bool in [true, false]:
		var event: InputEventKey = InputEventKey.new()
		event.physical_keycode = code
		event.pressed = pressed
		Input.parse_input_event(event)
		for frame: int in range(WAIT_FRAMES if pressed else HOLD_FRAMES):
			await process_frame
