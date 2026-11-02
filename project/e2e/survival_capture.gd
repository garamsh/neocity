class_name SurvivalCapture
extends SceneTree
## Captures real rendered frames of a played survival run into user://survival/,
## and fails when a city frame's ground does not read over the scenery behind it
## or one of its word plates covers another.

## One unfed day short of the acid that ends the run.
const HIGH_ACID: int = 84
## Half the acid that ends the run, to the nearest unfed day's rise.
const MIDDLE_ACID: int = 48
## Following NEXT on this seed escapes; `G` alone only ever loses.
const ESCAPE_SEED: int = 7
const DAY_LIMIT: int = 120
const STEP_FRAMES: int = 6
const ROUTE_KEYS: Dictionary[String, Key] = {"left": KEY_A, "right": KEY_D, "up": KEY_W, "down": KEY_S}
## The map's top-left corner on screen and its cell size, as the city draws it.
const MAP_ORIGIN: Vector2i = Vector2i(64, 70)
const CELL: int = 32
## Ground cells of the first city frame with nothing but scenery in the cell
## above: the arcade, the rooftop and the market.
const LEAD_CELLS: Array[Vector2i] = [Vector2i(5, 1), Vector2i(6, 1), Vector2i(7, 1), Vector2i(8, 1), Vector2i(17, 2), Vector2i(18, 2), Vector2i(19, 2), Vector2i(20, 2), Vector2i(6, 11), Vector2i(7, 11)]
## How much brighter, in mean luminance from 0 to 1, those ground cells read
## than the scenery in the cells above them.
const GROUND_LEAD: float = 0.05

var scene: MainScene
var held: Dictionary[Key, bool] = {}
var city_frames: int = 0

func _initialize() -> void:
	_run.call_deferred()

func _run() -> void:
	await _frames(2)
	scene = current_scene as MainScene
	scene.pin_survival_seed(11)
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("user://survival"))
	await _save("1-main-menu")
	await _tap(KEY_2)
	await _save("25-controls")
	await _tap(KEY_ESCAPE)
	await _tap(KEY_1)
	await _save("2-choose-players")
	await _tap(KEY_1)
	await _save("12-intro-card")
	await _tap(KEY_E)
	await _save("3-base-day-1")
	await _tap(KEY_Q)
	await _walk(KEY_D, 6)
	if not _ground_leads(await _save_city("4-city-collect-prompt")):
		return
	await _walk(KEY_D, 10)
	await _climb(5)
	await _frames(30)
	await _save_city("5-city-after-drip")
	await _walk(KEY_D, 16)
	await _walk_left(15)
	await _save_city("23-city-facing-left")
	await _walk(KEY_D, 16)
	await _climb(4)
	await _save_city("24-city-on-ladder")
	await _until_phase("workshop")
	await _save("6-workshop-after-return")
	await _until_next("Cook", 300)
	await _save("7-workshop-cook-offered")
	await _tap(KEY_S)
	await _save("14-workshop-dish-chosen")
	for attempt: int in range(4):
		await _tap(KEY_E)
		if _int("foods") > 0:
			break
	await _save("8-workshop-dish-ready")
	await _tap(KEY_F)
	await _frames(20)
	await _save("9-next-day")
	await _tap(KEY_Q)
	await _until_phase("workshop")
	await _frames(60)
	await _tap(KEY_E)
	await _tap(KEY_E)
	await _tap(KEY_F)
	await _frames(12)
	await _tap(KEY_G)
	await _frames(12)
	await _tap(KEY_G)
	await _frames(12)
	await _tap(KEY_G)
	await _frames(12)
	await _save("10-workshop-acid-rose")
	await _tap(KEY_Q)
	await _frames(40)
	await _walk(KEY_D, 8)
	await _frames(60)
	await _save_city("11-city-damaged")
	await _tap(KEY_Q)
	await _until_phase("workshop")
	while _int("acid") < MIDDLE_ACID:
		await _tap(KEY_G)
		await _frames(12)
	await _tap(KEY_Q)
	await _frames(40)
	await _walk(KEY_D, 6)
	await _save_city("26-city-middle-acid")
	await _tap(KEY_Q)
	await _until_phase("workshop")
	while _int("acid") < HIGH_ACID:
		await _tap(KEY_G)
		await _frames(12)
	await _tap(KEY_Q)
	await _frames(40)
	await _walk(KEY_D, 6)
	await _save_city("17-city-high-acid")
	await _tap(KEY_ESCAPE)
	await _save("18-pause-one-player")
	await _tap(KEY_M)
	await _save("19-confirm-leave")
	await _tap(KEY_ESCAPE)
	await _tap(KEY_ESCAPE)
	_key(KEY_D, true)
	await _tap(KEY_ESCAPE)
	await _tap(KEY_ESCAPE)
	await _save("20-resuming")
	_key(KEY_D, false)
	await _frames(4)
	await _tap(KEY_M)
	await _tap(KEY_ENTER)
	await _tap(KEY_1)
	await _tap(KEY_3)
	await _tap(KEY_E)
	await _save("15-two-players-workshop")
	await _tap(KEY_Q)
	await _tap(KEY_U)
	await _frames(12)
	await _save_city("16-two-players-city")
	await _tap(KEY_ESCAPE)
	await _save("21-pause-two-players")
	await _tap(KEY_ESCAPE)
	await _tap(KEY_X)
	await _frames(12)
	await _save("22-hand-off-notice")
	await _tap(KEY_M)
	await _tap(KEY_ENTER)
	await _tap(KEY_1)
	scene.pin_survival_seed(ESCAPE_SEED)
	await _tap(KEY_1)
	await _tap(KEY_E)
	while scene.survival_status().get("phase", "") in ["workshop", "exploration"] and _int("day") < DAY_LIMIT:
		await _follow_next()
	await _frames(2)
	await _save("13-ending-card")
	print("Capture: no map plate covers another in ", city_frames, " city frames.")
	print("Capture: ", ProjectSettings.globalize_path("user://survival"))
	quit(0)

## Presses the one key NEXT names, or lets the day run while it names none.
func _follow_next() -> void:
	var line: String = scene.survival_status().get("next", "")
	if line.contains("Set out") or line.contains("Nothing is within reach"):
		await _tap(KEY_Q)
	elif line.contains("[F] Feed"):
		await _tap(KEY_F)
	elif line.contains("[R] "):
		await _tap(KEY_R)
	elif line.contains("[G] "):
		await _tap(KEY_G)
	elif line.contains("[E] "):
		await _tap(KEY_E)
	elif line.begins_with("Walk "):
		var code: Key = ROUTE_KEYS.get(line.get_slice(" ", 1), KEY_NONE)
		if code != KEY_NONE:
			_key(code, true)
			_key(code, false)
		await _frames(STEP_FRAMES)
	else:
		await _frames(STEP_FRAMES)

func _walk(code: Key, x: int) -> void:
	_key(code, true)
	for frame: int in range(240):
		var chef: Dictionary = scene.survival_status()["chef"]
		var at: int = chef["x"]
		if at >= x:
			break
		await process_frame
	_key(code, false)
	await _frames(3)

## Holds A until the chef stands on column `x` or left of it.
func _walk_left(x: int) -> void:
	_key(KEY_A, true)
	for frame: int in range(240):
		var chef: Dictionary = scene.survival_status()["chef"]
		var at: int = chef["x"]
		if at <= x:
			break
		await process_frame
	_key(KEY_A, false)
	await _frames(3)

## Holds W until the chef stands on row `y` or above.
func _climb(y: int) -> void:
	_key(KEY_W, true)
	for frame: int in range(240):
		var chef: Dictionary = scene.survival_status()["chef"]
		var at: int = chef["y"]
		if at <= y:
			break
		await process_frame
	_key(KEY_W, false)
	await _frames(3)

func _until_phase(phase: String) -> void:
	for frame: int in range(3000):
		var now: String = scene.survival_status()["phase"]
		if now == phase:
			return
		await process_frame

func _until_next(text: String, limit: int) -> void:
	for frame: int in range(limit):
		var now: String = scene.survival_status()["next"]
		if now.contains(text):
			return
		await process_frame

func _int(field: String) -> int:
	var value: int = scene.survival_status().get(field, 0)
	return value

func _save(screen: String) -> Image:
	await RenderingServer.frame_post_draw
	var destination: String = ProjectSettings.globalize_path("user://survival/" + screen + ".png")
	var frame: Image = root.get_texture().get_image()
	if frame.save_png(destination) != OK:
		_fail("Could not save " + destination)
		return frame
	print("Capture: ", destination)
	return frame

## Saves a city frame and fails it when one word plate covers another, or when
## it drew fewer plates than a count and a name for each source shown.
func _save_city(screen: String) -> Image:
	var frame: Image = await _save(screen)
	var plates: Array = scene.get_meta("map_plates", [])
	var objects: Dictionary = scene.survival_status()["objects"]
	var sources: int = 0
	for source: String in scene.survival_status()["stock"]:
		if objects.get(source, "dormant") != "dormant":
			sources += 1
	for first: int in range(plates.size()):
		for second: int in range(first + 1, plates.size()):
			var one: Rect2 = plates[first]
			var other: Rect2 = plates[second]
			if one.intersects(other):
				_fail("%s: the plate at %s covers the plate at %s" % [screen, one, other])
				return frame
	if sources == 0 or plates.size() <= 2 * sources:
		_fail("%s: %d plates for %d sources" % [screen, plates.size(), sources])
		return frame
	city_frames += 1
	return frame

## Whether the ground's mean luminance leads the scenery right above it by
## GROUND_LEAD; the control is that the scenery is not black.
func _ground_leads(frame: Image) -> bool:
	var ground: float = 0.0
	var behind: float = 0.0
	for cell: Vector2i in LEAD_CELLS:
		ground += _luminance(frame, cell)
		behind += _luminance(frame, cell + Vector2i.UP)
	ground /= LEAD_CELLS.size()
	behind /= LEAD_CELLS.size()
	print("Capture: the ground leads the scenery above it by %.3f (ground %.3f, scenery %.3f, at least %.3f)." % [ground - behind, ground, behind, GROUND_LEAD])
	if behind <= 0.0 or ground - behind < GROUND_LEAD:
		_fail("the ground does not read over the scenery behind it")
		return false
	return true

func _luminance(frame: Image, cell: Vector2i) -> float:
	var corner: Vector2i = MAP_ORIGIN + cell * CELL
	var total: float = 0.0
	for y: int in range(CELL):
		for x: int in range(CELL):
			total += frame.get_pixelv(corner + Vector2i(x, y)).get_luminance()
	return total / (CELL * CELL)

func _fail(message: String) -> void:
	push_error(message)
	quit(1)

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
