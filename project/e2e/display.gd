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
## The window's size and the display's, neither 16:9 at the window.
const WINDOW: Vector2i = Vector2i(1280, 1024)
const DISPLAY: Vector2i = Vector2i(2560, 1440)
const CANVAS: Vector2 = Vector2(1920.0, 1080.0)
const ART_SCALE: float = 1.5
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
	# A window whose aspect ratio is not 16:9, so the canvas is letterboxed and
	# every click below is placed by the arithmetic of `_window_point`.
	root.size = Vector2i(WINDOW.x, WINDOW.y)
	scene.pin_display_size(DISPLAY)
	await _frames(2)
	if not _check(root.size == WINDOW, "setup: the window is %s: it is %s" % [WINDOW, root.size]):
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
	if not _a_saved_size_too_big_for_the_display_opens_smaller():
		return
	if not await _a_failed_save_is_reported():
		return
	if not _a_bad_file_gives_the_defaults():
		return
	print("Display: ", checks, " live checks passed (defaults, full screen refusal, settings keys, display size limit, settings mouse at ", WINDOW, ", pause screen, file, reopened scene, oversize file, failed save, bad file).")
	quit(0)

func _defaults_hold_without_a_file() -> bool:
	var status: Dictionary = scene.display_status()
	return _check(_resolution(status) == DEFAULT_RESOLUTION and _mode(status) == "fullscreen" and _size(status) == Vector2i(1920, 1080), "with no file the display is 1920x1080 in full screen: %s" % status)

## The Settings entry is the title's last, reached by up from the first and
## chosen with Enter. In full screen the resolution row shows the display's
## size and refuses a change; as a window it steps up to the display's size.
func _keys_change_the_settings_page() -> bool:
	await _tap(KEY_UP)
	await _tap(KEY_ENTER)
	if not _check(scene.screen_name() == "settings", "up from New game then Enter opens the Settings page"):
		return false
	if not _check(_shows("2560x1440 (display)"), "in full screen the resolution row shows the display's size"):
		return false
	if not _check(not _refusal("display_resolution_next").is_empty() and not _refusal("display_resolution_prev").is_empty(), "in full screen both resolution arrows carry a refusal"):
		return false
	await _tap(KEY_D)
	if not _check(_resolution(scene.display_status()) == DEFAULT_RESOLUTION and not FileAccess.file_exists(FILE), "D on the resolution row in full screen changes and writes nothing: %s" % scene.display_status()):
		return false
	await _tap(KEY_S)
	await _tap(KEY_D)
	if not _check(_mode(scene.display_status()) == "windowed" and _shows("1920x1080") and not _shows("(display)"), "S marks the window row and D makes it a window showing its own size: %s" % scene.display_status()):
		return false
	if not _check(not _target("display_resolution_next").is_empty() and _refusal("display_resolution_next").is_empty(), "as a window the resolution arrows are there and carry no refusal"):
		return false
	if not _file_says("1920x1080", "windowed", "the window row"):
		return false
	await _tap(KEY_UP)
	await _tap(KEY_D)
	if not _check(_resolution(scene.display_status()) == "2560x1440", "D on the resolution row of a window moves it on: %s" % scene.display_status()):
		return false
	if not _file_says("2560x1440", "windowed", "D"):
		return false
	# The display's size is the end of the list: the next size is refused and nothing is written.
	var written: int = FileAccess.get_modified_time(FILE)
	OS.delay_msec(1100)
	await _tap(KEY_D)
	if not _check(_resolution(scene.display_status()) == "2560x1440" and FileAccess.get_modified_time(FILE) == written, "a step past the display's size changes nothing and writes nothing: %s" % scene.display_status()):
		return false
	for step: int in range(SIZES.size()):
		await _tap(KEY_LEFT)
	if not _check(_resolution(scene.display_status()) == "1280x720", "left steps the resolution back to the smallest: %s" % scene.display_status()):
		return false
	written = FileAccess.get_modified_time(FILE)
	OS.delay_msec(1100)
	await _tap(KEY_A)
	if not _check(_resolution(scene.display_status()) == "1280x720" and FileAccess.get_modified_time(FILE) == written, "a step back past the smallest changes nothing and writes nothing"):
		return false
	for step: int in range(SIZES.size() + 1):
		await _tap(KEY_RIGHT)
	if not _check(_resolution(scene.display_status()) == "2560x1440", "steps on stop at the display's size, not the largest choice: %s" % scene.display_status()):
		return false
	await _tap(KEY_S)
	await _tap(KEY_A)
	if not _check(_mode(scene.display_status()) == "fullscreen", "A on the window row returns to full screen"):
		return false
	if not _file_says("2560x1440", "fullscreen", "returning to full screen"):
		return false
	await _tap(KEY_ESCAPE)
	return _check(scene.screen_name() == "main", "Esc goes back from the Settings page")

## A click on the title's Settings entry, then on each arrow.
func _mouse_changes_the_settings_page() -> bool:
	if not _check(await _click("option_5") and scene.screen_name() == "settings", "a click on Settings opens its page"):
		return false
	var written: int = FileAccess.get_modified_time(FILE)
	OS.delay_msec(1100)
	if not _check(await _click("display_resolution_prev") and _resolution(scene.display_status()) == "2560x1440" and FileAccess.get_modified_time(FILE) == written, "a click on the resolution arrow in full screen is refused and writes nothing: %s" % scene.display_status()):
		return false
	if not _check(await _click("display_window_next") and _mode(scene.display_status()) == "windowed", "a click on the window row's arrow makes it a window"):
		return false
	if not _file_says("2560x1440", "windowed", "the window arrow"):
		return false
	if not _check(await _click("display_resolution_prev") and _resolution(scene.display_status()) == "1920x1080", "a click on the resolution's back arrow steps a window back: %s" % scene.display_status()):
		return false
	if not _file_says("1920x1080", "windowed", "the back arrow"):
		return false
	if not _check(await _click("menu") and scene.screen_name() == "main", "a click on the way back returns to the title"):
		return false
	_move_mouse(Vector2.ZERO)
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
	if not _check(_resolution(scene.display_status()) == "2560x1440" and _effects() == volume, "down twice marks the resolution row, and D moves the window on and no volume: %s" % scene.display_status()):
		return false
	if not _file_says("2560x1440", "windowed", "the pause screen"):
		return false
	await _tap(KEY_S)
	await _tap(KEY_A)
	if not _check(_mode(scene.display_status()) == "fullscreen" and _shows("2560x1440 (display)"), "down marks the window row, and A returns to full screen showing the display's size"):
		return false
	var written: int = FileAccess.get_modified_time(FILE)
	OS.delay_msec(1100)
	if not _check(await _click("display_resolution_prev") and _resolution(scene.display_status()) == "2560x1440" and FileAccess.get_modified_time(FILE) == written, "a click on the pause screen's resolution arrow in full screen is refused"):
		return false
	if not _check(await _click("display_window_next") and _mode(scene.display_status()) == "windowed", "a click on the pause screen's window arrow makes a window"):
		return false
	if not _check(await _click("display_resolution_prev") and _resolution(scene.display_status()) == "1920x1080", "a click on the pause screen's resolution arrow steps the window back"):
		return false
	if not _file_says("1920x1080", "windowed", "the pause screen's arrow"):
		return false
	await _tap(KEY_UP)
	await _tap(KEY_UP)
	await _tap(KEY_A)
	if not _check(_effects() != volume and _resolution(scene.display_status()) == "1920x1080", "up twice from the clicked resolution row marks the effects, and A lowers the effects alone"):
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
	return _check(_resolution(status) == "1920x1080" and _mode(status) == "windowed", "a scene made afterwards starts on the kept choice: %s" % status)

## A size the display cannot hold opens as the largest that fits, and the file
## keeps what was saved.
func _a_saved_size_too_big_for_the_display_opens_smaller() -> bool:
	if not _write_file(3840, 2160, "windowed"):
		return false
	var fresh: MainScene = MainScene.new()
	fresh.pin_display_size(DISPLAY)
	var status: Dictionary = fresh.display_status()
	fresh.free()
	if not _check(_resolution(status) == "2560x1440" and _mode(status) == "windowed", "a saved 3840x2160 opens as 2560x1440 on a 2560x1440 display: %s" % status):
		return false
	return _file_says("3840x2160", "windowed", "opening smaller")

## A file that cannot be written leaves the choice in force, reports it in the
## status, and a later change that can be written clears it.
func _a_failed_save_is_reported() -> bool:
	var global: String = ProjectSettings.globalize_path(FILE)
	DirAccess.remove_absolute(global)
	if not _check(DirAccess.make_dir_absolute(global) == OK, "setup: a folder stands where the display file goes"):
		return false
	if not _check(await _click("option_5") and scene.screen_name() == "settings", "setup: a click on Settings opens its page"):
		return false
	await _tap(KEY_S)
	await _tap(KEY_D)
	if not _check(_mode(scene.display_status()) == "fullscreen" and _saved(scene.display_status()) == false, "a change that cannot be saved is still in force and reports saved false: %s" % scene.display_status()):
		return false
	DirAccess.remove_absolute(global)
	await _tap(KEY_A)
	if not _check(_mode(scene.display_status()) == "windowed" and _saved(scene.display_status()) == true, "a later change that can be saved reports saved true: %s" % scene.display_status()):
		return false
	await _tap(KEY_ESCAPE)
	return true

## A file holding no choice of the list gives the defaults for that part.
func _a_bad_file_gives_the_defaults() -> bool:
	if not _write_file(1234, 777, "sideways"):
		return false
	var fresh: MainScene = MainScene.new()
	var status: Dictionary = fresh.display_status()
	fresh.free()
	return _check(_resolution(status) == DEFAULT_RESOLUTION and _mode(status) == "fullscreen", "a file with sizes and a mode outside the choices gives the defaults: %s" % status)

func _write_file(width: int, height: int, mode: String) -> bool:
	var config: ConfigFile = ConfigFile.new()
	config.set_value("display", "width", width)
	config.set_value("display", "height", height)
	config.set_value("display", "mode", mode)
	return _check(config.save(FILE) == OK, "setup: the display file is written")

## Whether a line the scene drew reads `text`, or holds it.
func _shows(text: String) -> bool:
	var lines: Array = scene.get_meta("screen_text", [])
	for line: String in lines:
		if line.contains(text):
			return true
	return false

func _resolution(status: Dictionary) -> String:
	var label: String = status["resolution"]
	return label

func _mode(status: Dictionary) -> String:
	var mode: String = status["mode"]
	return mode

## Why the target pressing `press` does nothing now; empty when it acts or is not there.
func _refusal(press: String) -> String:
	var reason: String = _target(press).get("refusal", "")
	return reason

func _interruption() -> String:
	var open: String = scene.survival_status().get("interruption", "")
	return open

func _saved(status: Dictionary) -> bool:
	var saved: bool = status["saved"]
	return saved

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

## Where the window's pixel is for `point` in the units the scene lays out in:
## the canvas fits the window at its smaller ratio and is centered, and the
## art's layout covers `ART_SCALE` canvas pixels a unit. The engine's own
## transform is not asked.
func _window_point(point: Vector2) -> Vector2:
	var fit: float = minf(float(root.size.x) / CANVAS.x, float(root.size.y) / CANVAS.y)
	var margin: Vector2 = (Vector2(root.size) - CANVAS * fit) / 2.0
	return margin + point * ART_SCALE * fit

func _move_mouse(point: Vector2) -> void:
	var event: InputEventMouseMotion = InputEventMouseMotion.new()
	event.position = _window_point(point)
	event.global_position = event.position
	Input.parse_input_event(event)

## Points at the target pressing `press` and clicks it with the left button;
## false when the screen shows no such target, or when the scene does not
## light that target and no other under the mouse.
func _click(press: String) -> bool:
	var target: Dictionary = _target(press)
	if target.is_empty():
		return false
	var area: Rect2 = target["rect"]
	_move_mouse(area.get_center())
	await _frames(2)
	var lit: Array[String] = []
	var targets: Array = scene.get_meta("pointer_targets", [])
	for shown: Dictionary in targets:
		if shown["hovered"]:
			var name: String = shown["press"]
			lit.append(name)
	if not _check(lit == [press], "the mouse over %s lights that target alone: %s" % [press, lit]):
		return false
	for pressed: bool in [true, false]:
		var event: InputEventMouseButton = InputEventMouseButton.new()
		event.button_index = MOUSE_BUTTON_LEFT
		event.pressed = pressed
		event.position = _window_point(area.get_center())
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
