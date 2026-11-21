class_name TitleCapture
extends SceneTree
## Saves the title screen and its pages at 1920x1080 on a real window, each
## after the cursor has come to rest, and reads the saved pixels where the
## screen says it drew the UI kit: lime under the marked entry, dark cards under
## the others, the title picture's logo across the top left, and the panel's
## rim. `--out=<folder>` names where the PNGs go; the default is `user://title`.
## Run it on a window of 1920x1080 with `--fixed-fps 60`.

const CANVAS: Vector2i = Vector2i(1920, 1080)
## Layout units, which the canvas shows at one and a half pixels each.
const SCALE: float = 1.5
## Past the cursor's glide, whatever the scene clock was before.
const SETTLE_SECONDS: float = 0.5
## The logo plate in the picture, in layout units.
const LOGO: Rect2 = Rect2(92.0, 84.0, 444.0, 170.0)

var scene: MainScene
var clock: float = 100.0
var held: Dictionary[Key, bool] = {}
var checks: int = 0
var folder: String = "user://title"

func _initialize() -> void:
	for argument: String in OS.get_cmdline_user_args():
		if argument.begins_with("--out="):
			folder = argument.trim_prefix("--out=")
	_run.call_deferred()

func _run() -> void:
	await _frames(30)
	scene = current_scene as MainScene
	scene.pin_scene_clock(clock)
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(folder))
	var frame: Image = await _save("1-title-new-game")
	if frame == null or not _title_reads_as_the_kit(frame, 0):
		return
	await _tap(KEY_W)
	frame = await _save("2-title-settings-marked")
	if frame == null or not _title_reads_as_the_kit(frame, 4):
		return
	await _tap(KEY_2)
	frame = await _save("3-controls")
	if frame == null or not _page_reads_as_the_kit(frame, "controls"):
		return
	await _tap(KEY_ESCAPE)
	# The cursor still rests on Settings, which the number key left alone.
	await _tap(KEY_ENTER)
	if not _check(scene.screen_name() == "settings", "Enter on the marked Settings entry opens settings: %s" % scene.screen_name()):
		return
	frame = await _save("4-settings")
	if frame == null or not _page_reads_as_the_kit(frame, "settings"):
		return
	print("TitleCapture: finished: ", checks, " pixel checks passed.")
	quit(0)

## Moves the scene's clock past the cursor's glide, then saves what the window drew.
func _save(name: String) -> Image:
	clock += SETTLE_SECONDS
	scene.pin_scene_clock(clock)
	await process_frame
	await RenderingServer.frame_post_draw
	var frame: Image = root.get_texture().get_image()
	if not _check(frame.get_size() == CANVAS, "%s is drawn at %s: it is %s" % [name, CANVAS, frame.get_size()]):
		return null
	var destination: String = ProjectSettings.globalize_path(folder + "/" + name + ".png")
	if not _check(frame.save_png(destination) == OK, "could not save " + destination):
		return null
	print("TitleCapture: ", destination)
	return frame

## The frames of the kit the last frame noted, of one `kind`.
func _frames_of(kind: String) -> Array[Rect2]:
	var areas: Array[Rect2] = []
	var noted: Array = scene.get_meta("title_frames", [])
	for frame: Dictionary in noted:
		if frame["kind"] == kind:
			areas.append(frame["area"])
	return areas

func _title_reads_as_the_kit(frame: Image, marked: int) -> bool:
	var buttons: Array[Rect2] = _frames_of("button")
	var selected: Array[Rect2] = _frames_of("selected")
	if not _check(buttons.size() == 5 and selected.size() == 1, "the title notes five cards and one lime one: %s, %s" % [buttons, selected]):
		return false
	for index: int in range(buttons.size()):
		# Just inside the left edge, clear of the key cap and the words.
		var inside: Vector2 = (buttons[index].position + Vector2(10.0, buttons[index].size.y / 2.0 + 10.0)) * SCALE
		var pixel: Color = frame.get_pixelv(Vector2i(inside))
		if index == marked:
			if not _check(pixel.r > 0.6 and pixel.g > 0.8 and pixel.b < 0.45, "the marked entry %d is lime on the screen: %s" % [index, pixel]):
				return false
		elif not _check(pixel.r < 0.3 and pixel.g < 0.3 and pixel.b < 0.35, "entry %d is a dark card on the screen: %s" % [index, pixel]):
			return false
	if not _check(selected[0].position.y == buttons[marked].position.y, "the lime card rests on entry %d" % marked):
		return false
	return _logo_and_panel(frame)

func _page_reads_as_the_kit(frame: Image, page: String) -> bool:
	var panels: Array[Rect2] = _frames_of("panel")
	if not _check(panels.size() == 1 and _frames_of("button").size() >= 1, "the %s page notes one panel and its way back: %s" % [page, panels]):
		return false
	var rim: Color = frame.get_pixelv(Vector2i((panels[0].position + Vector2(0.4, panels[0].size.y / 2.0)) * SCALE))
	return _check(rim.b > rim.g + 0.15 and rim.r > 0.3, "the %s page's panel has a lavender outline on the screen: %s" % [page, rim])

## The picture's lettering stands where the logo is, and the menu
## panel's ink behind the cards.
func _logo_and_panel(frame: Image) -> bool:
	var golds: int = 0
	for y: int in range(int(LOGO.position.y * SCALE), int(LOGO.end.y * SCALE), 3):
		for x: int in range(int(LOGO.position.x * SCALE), int(LOGO.end.x * SCALE), 3):
			var pixel: Color = frame.get_pixel(x, y)
			if pixel.r > 0.85 and pixel.g > 0.55 and pixel.b < 0.35:
				golds += 1
	if not _check(golds > 1500, "the title picture's gold logo is on the screen: %d gold samples" % golds):
		return false
	var panels: Array[Rect2] = _frames_of("panel")
	if not _check(panels.size() == 2, "the title notes the menu panel and the foot panel: %s" % [panels]):
		return false
	var ink: Color = frame.get_pixelv(Vector2i((panels[0].position + Vector2(40.0, 40.0)) * SCALE))
	return _check(ink.r < 0.12 and ink.g < 0.1 and ink.b < 0.2 and ink.b > ink.g, "the menu panel is near-black purple ink on the screen: %s" % ink)

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
	push_error("TitleCapture failed: ", name)
	quit(1)
	return false
