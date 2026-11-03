class_name SurvivalCapture
extends SceneTree
## Captures real rendered frames of a played survival run into user://survival/,
## and fails when a city frame's ground does not read over the scenery behind it,
## the stairs' walked-on part does not lead the part under it, the fallen slab's
## lit top edge does not run along its tilt, or one of its word plates covers
## another.

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
## Ground cells of the first city frame with nothing but scenery in them: the
## arcade, the rooftop and the market. Each walkway's deck runs under its
## cell's floor line, where an actor's feet are.
const LEAD_CELLS: Array[Vector2i] = [Vector2i(4, 2), Vector2i(6, 2), Vector2i(7, 2), Vector2i(8, 2), Vector2i(19, 2), Vector2i(20, 2), Vector2i(21, 2), Vector2i(22, 2), Vector2i(8, 11), Vector2i(10, 11)]
## How much brighter, in mean luminance from 0 to 1, the deck under those
## cells reads than the scenery in the cells, above the deck.
const GROUND_LEAD: float = 0.05
## The rows of a deck under its floor line, as the city draws it.
const DECK_ROWS: int = 10
## The rows of a walkway's lit top edge, as the city draws it.
const EDGE_ROWS: int = 4
## The south stairs, each rising to the left across its cell from its floor
## line down to the row below, so its walked-on part lies above that diagonal.
const STAIR_CELLS: Array[Vector2i] = [Vector2i(13, 12), Vector2i(14, 13)]
## How much brighter, in mean luminance from 0 to 1, the EDGE_ROWS just above
## a stair's diagonal read than the EDGE_ROWS just under it.
const STEP_LEAD: float = 0.09
## The fallen slab's first cell and its width in cells; it falls a row from
## the floor line of its first cell to that of the row below at its right end.
const SLAB_CELL: Vector2i = Vector2i(20, 6)
const SLAB_CELLS: int = 2
## How far one 100ms movement unit carries a running actor, in sub-cells.
const RUN_UNIT: int = 50
## Longer than the screen's longest eased motion.
const SETTLE_MSEC: int = 400

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
	await _walk(KEY_D, 7)
	var first_city: Image = await _save_city("4-city-collect-prompt")
	if not _ground_leads(first_city) or not _stairs_lit(first_city) or not _slab_tilts(first_city):
		return
	# A jump in place on the flat street comes down where it rose.
	_key(KEY_SPACE, true)
	await _until_stance("airborne")
	_key(KEY_SPACE, false)
	await _until_falling()
	await _save_city("28-city-falling")
	await _until_stance("standing")
	await _save_city("29-city-landing")
	_key(KEY_SHIFT, true)
	_key(KEY_A, true)
	# The first unit after standing still reads as a walk; the second as a run.
	await _until_moved(-2 * RUN_UNIT)
	await _save_city("30-city-running")
	_key(KEY_A, false)
	_key(KEY_SHIFT, false)
	await _frames(3)
	await _walk(KEY_D, 8)
	_key(KEY_D, true)
	_key(KEY_SPACE, true)
	await _until_stance("airborne")
	_key(KEY_SPACE, false)
	await _frames(4)
	await _save_city("27-city-jumping-the-rubble")
	await _walk(KEY_D, 12)
	await _frames(30)
	await _save_city("5-city-after-drip")
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
	# The AI appraiser's collections spend day one's clock before the chef
	# reaches the roof ladder, so the ladder frames are day two's.
	await _walk(KEY_D, 16)
	await _walk_left(15)
	await _save_city("23-city-facing-left")
	await _walk(KEY_D, 18)
	await _climb(4)
	await _save_city("24-city-on-ladder")
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
	elif line.begins_with("Walk ") or line.begins_with("Climb ") or line.begins_with("Jump "):
		var code: Key = ROUTE_KEYS.get(line.get_slice(" ", 1), KEY_NONE)
		if code != KEY_NONE:
			_key(code, true)
			if line.begins_with("Jump "):
				_key(KEY_SPACE, true)
				_key(KEY_SPACE, false)
				await _until_stance("airborne")
			else:
				_key(code, false)
				await _frames(STEP_FRAMES)
				_key(code, _stance() == "airborne")
			while _stance() == "airborne":
				await process_frame
			_key(code, false)
		else:
			await _frames(STEP_FRAMES)
	else:
		await _frames(STEP_FRAMES)

## Runs with `code` until the chef stands on column `x` or right of it, jumping
## whatever low block stops it.
func _walk(code: Key, x: int) -> void:
	_key(code, true)
	_key(KEY_SHIFT, true)
	var last: int = -1
	var stalled: int = 0
	for frame: int in range(480):
		var chef: Dictionary = scene.survival_status()["chef"]
		var at: int = chef["x"]
		if at >= x and chef["stance"] != "airborne":
			break
		var sub_x: int = chef["sub_x"]
		stalled = stalled + 1 if sub_x == last and chef["stance"] == "standing" else 0
		last = sub_x
		_key(KEY_SPACE, stalled > 6)
		await process_frame
	_key(KEY_SPACE, false)
	_key(code, false)
	_key(KEY_SHIFT, false)
	await _frames(3)

func _stance() -> String:
	var chef: Dictionary = scene.survival_status().get("chef", {})
	var stance: String = chef.get("stance", "")
	return stance

func _until_stance(stance: String) -> void:
	for frame: int in range(60):
		if _stance() == stance:
			return
		await process_frame

## Waits until the chef has moved `distance` sub-cells sideways or further,
## signed, from where it stands now.
func _until_moved(distance: int) -> void:
	var start: int = scene.survival_status()["chef"]["sub_x"]
	for frame: int in range(60):
		var now: int = scene.survival_status()["chef"]["sub_x"]
		if (now - start) * signi(distance) >= absi(distance):
			return
		await process_frame
	_fail("the chef did not move %d sub-cells" % distance)

## Waits until the chef, in the air, is lower than the highest it has been
## since: it is coming down.
func _until_falling() -> void:
	var top: int = scene.survival_status()["chef"]["sub_y"]
	for frame: int in range(60):
		var chef: Dictionary = scene.survival_status()["chef"]
		var now: int = chef["sub_y"]
		if chef["stance"] == "airborne" and now > top:
			return
		top = mini(top, now)
		await process_frame
	_fail("the chef never came down from the jump")

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

## Waits on the real clock first: --fixed-fps runs frames faster than real time,
## and the screen's eased motion runs on the real clock.
func _save(screen: String) -> Image:
	OS.delay_msec(SETTLE_MSEC)
	await process_frame
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

## Whether the deck's mean luminance leads the scenery right above it by
## GROUND_LEAD; the control is that the scenery is not black.
func _ground_leads(frame: Image) -> bool:
	var ground: float = 0.0
	var behind: float = 0.0
	for cell: Vector2i in LEAD_CELLS:
		ground += _luminance(frame, MAP_ORIGIN + (cell + Vector2i.DOWN) * CELL, DECK_ROWS)
		behind += _luminance(frame, MAP_ORIGIN + cell * CELL, CELL)
	ground /= LEAD_CELLS.size()
	behind /= LEAD_CELLS.size()
	print("Capture: the ground leads the scenery above it by %.3f (ground %.3f, scenery %.3f, at least %.3f)." % [ground - behind, ground, behind, GROUND_LEAD])
	if behind <= 0.0 or ground - behind < GROUND_LEAD:
		_fail("the ground does not read over the scenery behind it")
		return false
	return true

## Whether the EDGE_ROWS just above each stair's diagonal, where it is walked
## on, lead the EDGE_ROWS just under it by STEP_LEAD; the columns at the cell's
## ends, where a band would leave the cell, are left out.
func _stairs_lit(frame: Image) -> bool:
	var walked: float = 0.0
	var under: float = 0.0
	var columns: Vector2i = Vector2i(EDGE_ROWS - 1, CELL - EDGE_ROWS)
	for cell: Vector2i in STAIR_CELLS:
		var corner: Vector2i = MAP_ORIGIN + (cell + Vector2i.DOWN) * CELL
		walked += _band(frame, corner, CELL, columns, Vector2i(1 - EDGE_ROWS, 1))
		under += _band(frame, corner, CELL, columns, Vector2i(1, 1 + EDGE_ROWS))
	walked /= STAIR_CELLS.size()
	under /= STAIR_CELLS.size()
	print("Capture: the stairs' walked-on part leads the part under it by %.3f (walked %.3f, under %.3f, at least %.3f)." % [walked - under, walked, under, STEP_LEAD])
	if under <= 0.0 or walked - under < STEP_LEAD:
		_fail("the stairs' walked-on part does not read over the part under it")
		return false
	return true

## Whether the fallen slab's lit top edge, the EDGE_ROWS under the line it
## tilts along, leads the EDGE_ROWS of scenery just above that line by
## GROUND_LEAD.
func _slab_tilts(frame: Image) -> bool:
	var corner: Vector2i = MAP_ORIGIN + (SLAB_CELL + Vector2i.DOWN) * CELL
	var columns: Vector2i = Vector2i(0, SLAB_CELLS * CELL)
	var edge: float = _band(frame, corner, SLAB_CELLS * CELL, columns, Vector2i(0, EDGE_ROWS))
	var above: float = _band(frame, corner, SLAB_CELLS * CELL, columns, Vector2i(-EDGE_ROWS, 0))
	print("Capture: the fallen slab's top edge leads the scenery above its tilt by %.3f (edge %.3f, scenery %.3f, at least %.3f)." % [edge - above, edge, above, GROUND_LEAD])
	if above <= 0.0 or edge - above < GROUND_LEAD:
		_fail("the fallen slab's top edge does not run along its tilt")
		return false
	return true

## The mean luminance of a band along a line that falls one CELL from corner
## across width pixels: in each of the columns from columns.x to columns.y, the
## rows from rows.x to rows.y under the line, a negative row above it.
func _band(frame: Image, corner: Vector2i, width: int, columns: Vector2i, rows: Vector2i) -> float:
	var total: float = 0.0
	for x: int in range(columns.x, columns.y):
		var line: int = corner.y + floori(float(x * CELL) / width)
		for row: int in range(rows.x, rows.y):
			total += frame.get_pixel(corner.x + x, line + row).get_luminance()
	return total / ((columns.y - columns.x) * (rows.y - rows.x))

## The mean luminance of a cell-wide band of rows whose top-left is corner.
func _luminance(frame: Image, corner: Vector2i, rows: int) -> float:
	var total: float = 0.0
	for y: int in range(rows):
		for x: int in range(CELL):
			total += frame.get_pixelv(corner + Vector2i(x, y)).get_luminance()
	return total / (CELL * rows)

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
