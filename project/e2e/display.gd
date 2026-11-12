class_name DisplayCheck
extends SceneTree
## Changes the display setting through the game's own screens with real key
## and mouse events, on the Settings page and on the pause screen, and reads
## each change back from the session and from the file under `user://` that
## keeps it, then from a scene opened afterwards. The run owns its `user://`:
## the Makefile points the engine's data folder at a fresh one.

const FILE: String = "user://display.cfg"
## What a player who never chose gets: 1920x1080 in full screen.
const DEFAULT_RESOLUTION: String = "1920x1080"
const SIZES: Array[String] = ["1280x720", "1600x900", "1920x1080", "2560x1440", "3840x2160"]

var scene: MainScene
var held: Dictionary[Key, bool] = {}
var checks: int = 0
var failed: bool = false

func _initialize() -> void:
	_run.call_deferred()

func _run() -> void:
	await _frames(2)
	scene = current_scene as MainScene
	if not _check(scene != null and scene.screen_name() == "main", "the game opens on the title screen"):
		return
	if not _check(not FileAccess.file_exists(FILE), "setup: no display file exists before a choice"):
		return
	if not await _defaults_hold_without_a_file():
		return
	if not await _keys_change_the_settings_page():
		return
	if not await _mouse_changes_the_settings_page():
		return
	if not await _pause_screen_changes_the_display():
		return
	if not _a_new_scene_reads_the_choice_back():
		return
	if not _a_bad_file_gives_the_defaults():
		return
	print("Display: ", checks, " live checks passed (defaults, settings keys, ends held, settings mouse, pause screen, file, reopened scene, bad file).")
	quit(0)

func _defaults_hold_without_a_file() -> bool:
	var status: Dictionary = scene.display_status()
	return _check(_resolution(status) == DEFAULT_RESOLUTION and _mode(status) == "fullscreen" and _size(status) == Vector2i(1920, 1080), "with no file the display is 1920x1080 in full screen: %s" % status)

## The Settings entry is the title's last, reached by up from the first and
## chosen with Enter; keys change the marked row and write the file.
func _keys_change_the_settings_page() -> bool:
	await _tap(KEY_UP)
	await _tap(KEY_ENTER)
	if not _check(scene.screen_name() == "settings", "up from New game then Enter opens the Settings page"):
		return false
	await _tap(KEY_D)
	if not _check(_resolution(scene.display_status()) == "2560x1440", "D on the marked resolution row moves it on: %s" % scene.display_status()):
		return false
	if not _file_says("2560x1440", "fullscreen", "D"):
		return false
	await _tap(KEY_LEFT)
	await _tap(KEY_A)
	await _tap(KEY_LEFT)
	if not _check(_resolution(scene.display_status()) == "1280x720", "A and left step the resolution back to the smallest: %s" % scene.display_status()):
		return false
	# The smallest is held: another step back changes nothing and writes nothing.
	var written: int = FileAccess.get_modified_time(FILE)
	OS.delay_msec(1100)
	await _tap(KEY_A)
	if not _check(_resolution(scene.display_status()) == "1280x720" and FileAccess.get_modified_time(FILE) == written, "a step back past the smallest changes nothing and writes nothing"):
		return false
	for step: int in range(SIZES.size() + 1):
		await _tap(KEY_RIGHT)
	if not _check(_resolution(scene.display_status()) == "3840x2160", "steps on stop at the largest: %s" % scene.display_status()):
		return false
	await _tap(KEY_S)
	await _tap(KEY_D)
	if not _check(_mode(scene.display_status()) == "windowed", "S marks the window row and D makes it a window: %s" % scene.display_status()):
		return false
	if not _file_says("3840x2160", "windowed", "the window row"):
		return false
	await _tap(KEY_A)
	if not _check(_mode(scene.display_status()) == "fullscreen", "A on the window row returns to full screen"):
		return false
	await _tap(KEY_ESCAPE)
	return _check(scene.screen_name() == "main", "Esc goes back from the Settings page")

## A click on the title's Settings entry, then on each arrow.
func _mouse_changes_the_settings_page() -> bool:
	if not _check(await _click("option_5") and scene.screen_name() == "settings", "a click on Settings opens its page"):
		return false
	if not _check(await _click("display_resolution_prev") and _resolution(scene.display_status()) == "2560x1440", "a click on the resolution's back arrow steps it back: %s" % scene.display_status()):
		return false
	if not _file_says("2560x1440", "fullscreen", "the back arrow"):
		return false
	if not _check(await _click("display_window_next") and _mode(scene.display_status()) == "windowed", "a click on the window row's arrow makes it a window"):
		return false
	if not _file_says("2560x1440", "windowed", "the window arrow"):
		return false
	if not _check(await _click("menu") and scene.screen_name() == "main", "a click on the way back returns to the title"):
		return false
	_point_at({"rect": Rect2(0, 0, 1, 1)})
	return true

## The pause screen holds the same two rows after the volumes: down twice marks
## the resolution, and its arrows work by mouse.
func _pause_screen_changes_the_display() -> bool:
	await _tap(KEY_1)
	await _tap(KEY_1)
	await _tap(KEY_E)
	await _tap(KEY_ESCAPE)
	if not _check(_interruption() == "paused", "setup: Esc pauses the run"):
		return false
	var volume: float = _effects()
	await _tap(KEY_DOWN)
	await _tap(KEY_S)
	await _tap(KEY_D)
	if not _check(_resolution(scene.display_status()) == "3840x2160" and _effects() == volume, "down twice marks the resolution row, and D moves it on and no volume: %s" % scene.display_status()):
		return false
	if not _file_says("3840x2160", "windowed", "the pause screen"):
		return false
	await _tap(KEY_S)
	await _tap(KEY_A)
	if not _check(_mode(scene.display_status()) == "fullscreen", "down marks the window row, and A returns to full screen"):
		return false
	if not _check(await _click("display_resolution_prev") and _resolution(scene.display_status()) == "2560x1440", "a click on the pause screen's resolution arrow steps it back"):
		return false
	if not _file_says("2560x1440", "fullscreen", "the pause screen's arrow"):
		return false
	await _tap(KEY_UP)
	await _tap(KEY_UP)
	await _tap(KEY_A)
	if not _check(_effects() != volume and _resolution(scene.display_status()) == "2560x1440", "up twice from the clicked resolution row marks the effects, and A lowers the effects alone"):
		return false
	await _tap(KEY_ESCAPE)
	await _tap(KEY_M)
	await _tap(KEY_ENTER)
	return _check(scene.screen_name() == "main", "setup: the run is left")

## A scene made afterwards reads the choice the file keeps.
func _a_new_scene_reads_the_choice_back() -> bool:
	var fresh: MainScene = MainScene.new()
	var status: Dictionary = fresh.display_status()
	fresh.free()
	return _check(_resolution(status) == "2560x1440" and _mode(status) == "fullscreen", "a scene made afterwards starts on the kept choice: %s" % status)

## A file holding no choice of the list gives the defaults for that part.
func _a_bad_file_gives_the_defaults() -> bool:
	var config: ConfigFile = ConfigFile.new()
	config.set_value("display", "width", 1234)
	config.set_value("display", "height", 777)
	config.set_value("display", "mode", "sideways")
	if not _check(config.save(FILE) == OK, "setup: the display file is written"):
		return false
	var fresh: MainScene = MainScene.new()
	var status: Dictionary = fresh.display_status()
	fresh.free()
	return _check(_resolution(status) == DEFAULT_RESOLUTION and _mode(status) == "fullscreen", "a file with sizes and a mode outside the choices gives the defaults: %s" % status)

func _resolution(status: Dictionary) -> String:
	var label: String = status["resolution"]
	return label

func _mode(status: Dictionary) -> String:
	var mode: String = status["mode"]
	return mode

func _interruption() -> String:
	var open: String = scene.survival_status().get("interruption", "")
	return open

func _size(status: Dictionary) -> Vector2i:
	var width: int = status["width"]
	var height: int = status["height"]
	return Vector2i(width, height)

func _effects() -> float:
	var level: float = scene.sound_status()["effects_level"]
	return level

## Whether the file under `user://` holds `resolution` and `mode`.
func _file_says(resolution: String, mode: String, after: String) -> bool:
	var config: ConfigFile = ConfigFile.new()
	var loaded: bool = config.load(FILE) == OK
	var size: String = "%sx%s" % [config.get_value("display", "width", 0), config.get_value("display", "height", 0)]
	var kept: String = config.get_value("display", "mode", "")
	return _check(loaded and size == resolution and kept == mode, "after %s the file keeps %s %s: it holds %s %s" % [after, resolution, mode, size, kept])

## The target the last frame drew whose click presses `press`; empty with none.
func _target(press: String) -> Dictionary:
	var targets: Array = scene.get_meta("pointer_targets", [])
	for target: Dictionary in targets:
		if target["press"] == press:
			return target
	return {}

## Moves the mouse to the middle of `target`, a point on the screen the scene
## draws, through the window's stretch.
func _point_at(target: Dictionary) -> void:
	var area: Rect2 = target["rect"]
	var event: InputEventMouseMotion = InputEventMouseMotion.new()
	event.position = root.get_final_transform() * area.get_center()
	event.global_position = event.position
	Input.parse_input_event(event)

## Points at the target pressing `press` and clicks it with the left button;
## false when the screen shows no such target.
func _click(press: String) -> bool:
	var target: Dictionary = _target(press)
	if target.is_empty():
		return false
	_point_at(target)
	await _frames(2)
	var area: Rect2 = target["rect"]
	var motion: InputEventMouseMotion = InputEventMouseMotion.new()
	motion.position = root.get_final_transform() * area.get_center()
	motion.global_position = motion.position
	Input.parse_input_event(motion)
	for pressed: bool in [true, false]:
		var event: InputEventMouseButton = InputEventMouseButton.new()
		event.button_index = MOUSE_BUTTON_LEFT
		event.pressed = pressed
		event.position = motion.position
		event.global_position = event.position
		Input.parse_input_event(event)
		await _frames(2 if pressed else 8)
	return true

func _tap(code: Key) -> void:
	_key(code, true)
	await _frames(2)
	_key(code, false)
	await _frames(8)

func _frames(count: int) -> void:
	for frame: int in range(count):
		await process_frame

func _key(code: Key, pressed: bool) -> void:
	if held.get(code, false) == pressed:
		return
	held[code] = pressed
	var event: InputEventKey = InputEventKey.new()
	event.physical_keycode = code
	event.pressed = pressed
	Input.parse_input_event(event)

func _check(condition: bool, name: String) -> bool:
	if condition:
		checks += 1
		return true
	push_error("Display check failed: ", name, " ", scene.screen_name() if scene != null else "")
	failed = true
	quit(1)
	return false
