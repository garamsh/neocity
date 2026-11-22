class_name InteriorCheck
extends SceneTree
## Walks both humans into the Cold Store's ground hall on real keys and reads what the drawing recorded
## there, headless: the hall's own picture without the street's, the stair's treads meeting its footing,
## and every badge and tag inside the building.

## The scene's origin on the layout: the map's top left corner.
const ORIGIN: Vector2 = Vector2(64, 70)
const CELL: float = 32.0
## The cell of the Cold Store's stair, whose floor line a walker's feet keep.
const STAIR: Vector2i = Vector2i(40, 7)
## The cell on the stair's lower side, one row down, and on its upper side, one row up.
const LOWER: Vector2i = Vector2i(39, 8)
const UPPER: Vector2i = Vector2i(41, 6)
const ROUTE: Array[Vector2i] = [
	Vector2i(11, 9), Vector2i(13, 9), Vector2i(16, 9), Vector2i(17, 8), Vector2i(29, 9),
	Vector2i(31, 9), Vector2i(35, 9), Vector2i(36, 8), Vector2i(38, 8),
]
## The ring colours of player one's character and of the AI partner.
const FIRST_PLAYER_OUTLINE: Color = Color(1.0, 0.62, 0.12)
const AI_OUTLINE: Color = Color(0.85, 0.88, 0.92)
## The cell of the Cold Store's hall where both humans stand overlapped.
const HALL: Vector2i = Vector2i(38, 8)
## Where the camera's centre rests on the screen, and how far from it, in layout units, a human's feet may stand.
const FOCUS: Vector2 = Vector2(640, 301)
const BAND: Vector2 = Vector2(200, 340)
## How far from the middle of the screen a human may stand where the camera is stopped by the room's side.
const EDGE_BAND: float = 580.0
## The cells of the kitchen's floor the humans stand on, from its door end through the middle to the far side.
const KITCHEN_STOPS: Array[Vector2i] = [Vector2i(0, 6), Vector2i(2, 6), Vector2i(5, 6)]
## The same for the noodle shop, whose room starts at cell 20.
const NOODLE_STOPS: Array[Vector2i] = [Vector2i(20, 6), Vector2i(22, 6), Vector2i(25, 6)]
## The least the camera's centre moves between two stops, in scene pixels.
const LEAST_TRAVEL: float = 4.0
## How far, as shares of its height on the street, a drawn human's height on the screen may stray in any frame
## while the two split up and come together again: the street's far zoom draws it at half, and no frame draws it larger.
const LEAST_HEIGHT: float = 0.45
const MOST_HEIGHT: float = 1.05
var scene: MainScene
var clock: float = 0.0
var held_keys: Dictionary[Key, bool] = {}
var failures: Array[String] = []
## The exploration day's length for this walk, in seconds.
const DAY_SECONDS: int = 120
## The street's close zoom, which the room alone is never shown at.
const STREET_ZOOM: float = 2.0
## While set, every frame the walk passes is read for each human's drawn height, by role.
var sampling: bool = false
var street_height: Dictionary[String, float] = {}
var sampled: int = 0
## With `--capture=<folder>` on a real window, each stop in the shops is saved there as a PNG, and the run ends after the noodle shop.
var capture: String = ""
## With `--capture-split=<folder>` on a real window, the split route alone runs and saves its stages and every few frames of the second human's entry there.
var capture_split: String = ""
var entering: bool = false
var entry_frames: int = 0

func _initialize() -> void:
	for argument: String in OS.get_cmdline_user_args():
		if argument.begins_with("--capture-split="):
			capture_split = argument.trim_prefix("--capture-split=")
		if argument.begins_with("--capture="):
			capture = argument.trim_prefix("--capture=")
	Engine.max_fps = 60
	_run.call_deferred()

func _run() -> void:
	await process_frame
	scene = current_scene as MainScene
	scene.get_node("World").set_meta("measure_art", true)
	scene.pin_survival_seed(11)
	scene.pin_scene_clock(clock)
	# The walk is longer than the 18 second day the Makefile gives the other checks.
	OS.set_environment("NEOCITY_EXPLORATION_SECONDS", str(DAY_SECONDS))
	OS.set_environment("NEOCITY_BASE_SECONDS", "0")
	OS.set_environment("NEOCITY_COOK_SECONDS", "0")
	await _tap(KEY_1)
	await _tap(KEY_3)
	await _tap(KEY_E)
	await _tap(KEY_Q)
	await _tap(KEY_U)
	if not capture_split.is_empty():
		capture = capture_split
		var passed: bool = await _check_split_route()
		print("Interior: split capture finished: %s; %d failures %s." % ["pass" if failures.is_empty() else "fail", failures.size(), failures])
		quit(0 if passed and failures.is_empty() else 1)
		return
	if not await _check_kitchen_camera():
		return
	if not capture.is_empty():
		await _capture_noodle()
		return
	if not await _check_split_route():
		return
	var on_street: bool = true
	for target: Vector2i in ROUTE:
		if not await _move(target, target):
			return
		if on_street:
			on_street = false
			await _settle()
			_check_outlines("street")
			_check_street_plain()
	await _settle()
	_check_hall()
	_check_visible("hall")
	_check_outlines("hall")
	if not await _move(STAIR, STAIR):
		return
	await _settle()
	_check_stair()
	_check_visible("stair")
	_check_neighbours()
	if not await _move(HALL, HALL):
		return
	await _settle()
	await _check_hand_off_outline()
	var verdict: String = "pass" if failures.is_empty() else "fail"
	print("Interior: finished: %s; %d failures %s." % [verdict, failures.size(), failures])
	quit(0 if failures.is_empty() else 1)

## Walks both humans up into the kitchen and along it, and reads where the camera stands at each stop.
func _check_kitchen_camera() -> bool:
	for target: Vector2i in [Vector2i(3, 8), Vector2i(3, 6)]:
		if not await _move(target, target):
			return false
	# Climbing in, at the top of the ladder, and then along the floor: the room's two rows are in view throughout.
	await _settle()
	_check_visible("kitchen ladder top")
	await _save("kitchen-climb")
	var centres: Array[Vector2] = []
	for stop: Vector2i in KITCHEN_STOPS:
		if not await _move(stop, stop):
			return false
		await _settle()
		centres.append(_read_camera("kitchen " + str(stop), stop == KITCHEN_STOPS[1]))
		await _save("kitchen-%d" % stop.x)
	for index: int in range(1, centres.size()):
		if centres[index].x - centres[index - 1].x < LEAST_TRAVEL:
			_fail("the camera did not follow along the kitchen: centres %s" % [centres])
	_check_open_door("kitchen")
	if _world().get_meta("vignette", 0.0) <= 0.0:
		_fail("the kitchen drew no vignette")
	return await _move(Vector2i(3, 6), Vector2i(3, 6)) and await _move(Vector2i(3, 8), Vector2i(3, 8))

## One human goes into the kitchen while the other waits on the street, then the other follows and leaves again,
## with every frame of it read: a drawn human is never larger on the screen than on the street, and never
## smaller than the street's far zoom draws it, and the room fills the screen's width once both are inside.
func _check_split_route() -> bool:
	await _settle()
	if not _measure_street():
		return false
	sampling = true
	var chef_at: Vector2i = Vector2i(3, 8)
	var appraiser_at: Vector2i = Vector2i(3, 8)
	# The chef goes in alone, along the floor and back to the ladder's top, while the appraiser stays below.
	for target: Vector2i in [Vector2i(3, 6), Vector2i(5, 6), Vector2i(3, 6)]:
		if not await _move(target, appraiser_at):
			return false
		chef_at = target
	await _settle()
	await _save("split-1-chef-in-appraiser-out")
	if _world().get_meta("inside_view", false):
		_fail("split: the view was only the inside with the appraiser outside")
	# The appraiser follows: both are inside, and the room fills the width.
	entering = true
	for target: Vector2i in [Vector2i(3, 6), Vector2i(5, 6)]:
		if not await _move(chef_at, target):
			return false
		appraiser_at = target
	entering = false
	await _settle()
	await _save("split-3-both-in")
	if not _world().get_meta("inside_view", false):
		_fail("split: both humans in the kitchen did not leave only the inside on screen")
	_check_fills("split: both inside")
	# The appraiser leaves again, then the chef after it.
	for target: Vector2i in [Vector2i(3, 6), Vector2i(3, 8)]:
		if not await _move(chef_at, target):
			return false
		appraiser_at = target
	await _settle()
	if not await _move(Vector2i(3, 8), appraiser_at):
		return false
	await _settle()
	sampling = false
	print("Interior: split route read %d frames." % sampled)
	return sampled > 0

## Reads each human's height on the screen on the street, where the later frames are measured against.
func _measure_street() -> bool:
	_sample()
	if street_height.size() != 2:
		_fail("split: both humans were not drawn on the street: %s" % [street_height])
		return false
	return true

## Each drawn human's height on the screen against its own on the street, in the frame on show.
func _sample() -> void:
	var drawn: Dictionary[String, float] = {}
	for layer: Dictionary in _world().get_meta("character_layers", []):
		var area: Rect2 = layer["area"]
		drawn[str(layer["role"])] = area.size.y
	if not sampling:
		for role: String in drawn:
			street_height[role] = drawn[role]
		return
	sampled += 1
	if _world().get_meta("inside_view", false) and _zoom() <= STREET_ZOOM:
		_fail("frame %d: the room alone was drawn at zoom %s, the street's, a small strip in a black screen" % [sampled, _zoom()])
	for role: String in drawn:
		var share: float = drawn[role] / street_height[role]
		if share > MOST_HEIGHT or share < LEAST_HEIGHT:
			_fail("frame %d: the %s is drawn %.2f times its street height, at zoom %s, inside view %s" % [sampled, role, share, _zoom(), _world().get_meta("inside_view", false)])

func _zoom() -> float:
	var view: Transform2D = scene.get_meta("map_view", Transform2D())
	return view.get_scale().x

## The room's width fills the screen, so the view shows none of the black beside it.
func _check_fills(where: String) -> void:
	var view: Transform2D = scene.get_meta("map_view", Transform2D())
	var extent: Rect2 = _world().get_meta("inside_extent", Rect2())
	var shown: Rect2 = view.affine_inverse() * Rect2(0, 0, 1280, 720)
	if shown.size.x > extent.size.x + 1.0:
		_fail("%s: the room, %s wide, does not fill the screen, which shows %s of the scene at zoom %s" % [where, extent.size.x, shown.size.x, view.get_scale().x])

## Saves the frame as `<capture>/<name>.png` when capturing.
func _save(name: String) -> void:
	if capture.is_empty():
		return
	DirAccess.make_dir_recursive_absolute(capture)
	await process_frame
	root.get_viewport().get_texture().get_image().save_png("%s/%s.png" % [capture, name])

## Walks both humans from the kitchen to the noodle shop and along it, checks the same follow and saves each stop.
func _capture_noodle() -> void:
	for target: Vector2i in [Vector2i(3, 8), Vector2i(11, 9), Vector2i(13, 9), Vector2i(16, 9), Vector2i(17, 8), Vector2i(23, 8), Vector2i(23, 6)]:
		if not await _move(target, target):
			return
	for stop: Vector2i in NOODLE_STOPS:
		if not await _move(stop, stop):
			return
		await _settle()
		_read_camera("noodle " + str(stop), stop == NOODLE_STOPS[1])
		await _save("noodle-%d" % stop.x)
	_check_open_door("noodle")
	print("Interior: capture finished: %s; %d failures %s." % ["pass" if failures.is_empty() else "fail", failures.size(), failures])
	quit(0 if failures.is_empty() else 1)

## The camera's centre in the scene; fails when the view shows past the room's sides, or when the chef
## stands outside the band around the middle of the screen: a narrow one in the middle of the room, and
## one as wide as the room's side allows where the camera is stopped there.
func _read_camera(where: String, middle: bool) -> Vector2:
	var view: Transform2D = scene.get_meta("map_view", Transform2D())
	var zoom: float = view.get_scale().x
	var centre: Vector2 = (FOCUS - view.origin) / zoom
	var extent: Rect2 = _world().get_meta("inside_extent", Rect2())
	var shown: Rect2 = view.affine_inverse() * Rect2(0, 0, 1280, 720)
	if shown.size.x < extent.size.x and (shown.position.x < extent.position.x - 1.0 or shown.end.x > extent.end.x + 1.0):
		_fail("%s: the camera looks past the room's sides: view %s, room %s" % [where, shown, extent])
	if not extent.grow(1.0).has_point(centre):
		_fail("%s: the camera's centre is outside the room: centre %s, room %s" % [where, centre, extent])
	_check_visible(where)
	var chef: Dictionary = scene.survival_status()["chef"]
	var chef_x: int = chef["sub_x"]
	var chef_y: int = chef["sub_y"]
	var feet: Vector2 = view * (ORIGIN + Vector2(chef_x, chef_y))
	var off: Vector2 = (feet - FOCUS).abs()
	var limit: float = BAND.x if middle else EDGE_BAND
	if off.x > limit or off.y > BAND.y:
		_fail("%s: the chef stands %s from the middle of the screen, outside the band %s" % [where, off, Vector2(limit, BAND.y)])
	return centre

## Both humans' feet are on the screen, and so is the whole height of the room when the room is no taller
## than the screen shows: the camera follows sideways and frames the floors upright, so no row of the room is
## cut off wherever the humans stand on it or climb to it.
func _check_visible(where: String) -> void:
	var view: Transform2D = scene.get_meta("map_view", Transform2D())
	var shown: Rect2 = view.affine_inverse() * Rect2(0, 0, 1280, 720)
	var extent: Rect2 = _world().get_meta("inside_extent", Rect2())
	var status: Dictionary = scene.survival_status()
	for role: String in ["chef", "appraiser"]:
		var body: Dictionary = status[role]
		var body_x: int = body["sub_x"]
		var body_y: int = body["sub_y"]
		var feet: Vector2 = ORIGIN + Vector2(body_x, body_y)
		if not shown.grow(1.0).has_point(feet):
			_fail("%s: the %s's feet %s are outside the view %s" % [where, role, feet, shown])
	if extent.size.y <= shown.size.y and (shown.position.y > extent.position.y + 1.0 or shown.end.y < extent.end.y - 1.0):
		_fail("%s: a row of the room is cut off: view %s, room %s" % [where, shown, extent])

## The door the humans came in by is drawn open on the street, inside the room.
func _check_open_door(where: String) -> void:
	var door: Dictionary = _world().get_meta("open_door", {})
	var extent: Rect2 = _world().get_meta("inside_extent", Rect2())
	if door.is_empty():
		_fail("%s: no open door was drawn" % where)
		return
	var opening: Rect2 = door["area"]
	if not extent.encloses(opening) or str(door["street"]) != "City":
		_fail("%s: the open door %s does not show the street inside the room %s: %s" % [where, opening, extent, door])

## The street shows neither the vignette nor an open door.
func _check_street_plain() -> void:
	var door: Dictionary = _world().get_meta("open_door", {})
	if _world().get_meta("vignette", 1.0) != 0.0 or not door.is_empty():
		_fail("the street drew a vignette or an open door")

func _world() -> Node:
	return scene.get_node("World")

func _fail(reason: String) -> void:
	failures.append(reason)

## The hall draws its own inside: no street wall or door picture over its room, and its
## badges and tags stay inside the building.
func _check_hall() -> void:
	var world: Node = _world()
	if not world.get_meta("inside_view", false):
		_fail("both humans in the hall did not leave only the building's inside on screen")
		return
	var lit: Rect2 = Rect2()
	for room: Dictionary in world.get_meta("interior_rooms", []):
		if room["lit"]:
			lit = room["area"]
	var street: Dictionary[String, int] = {}
	for layer: Dictionary in world.get_meta("foreground_layers", []):
		var texture: Texture2D = layer["texture"]
		var name: String = texture.resource_path.get_file()
		var area: Rect2 = layer["destination"]
		if name in ["door_cold_store.png", "building_wall.png"] and area.intersects(lit):
			street[name] = street.get(name, 0) + 1
	if lit.size == Vector2():
		_fail("no lit hall room was recorded")
	if not street.is_empty():
		_fail("the hall's room shows the street's front, pictures drawn over it: %s" % [street])
	var view: Transform2D = scene.get_meta("map_view", Transform2D())
	var scene_extent: Rect2 = world.get_meta("inside_extent", Rect2())
	var extent: Rect2 = view * scene_extent
	var plates: Array = scene.get_meta("map_plates", [])
	var words: Array = scene.get_meta("map_words", [])
	var stray: Array[String] = []
	for index: int in range(plates.size()):
		var box: Rect2 = plates[index]
		if not extent.encloses(box):
			stray.append("%s %s outside %s" % [words[index], box, extent])
	# The view shows about five cells of the hall's twelve, so a source's badge may be off screen.
	if plates.is_empty():
		_fail("the hall drew no badge or tag, not even a human's tag")
	var sources: Array = scene.get_meta("map_sources", [])
	var areas: Array = scene.get_meta("map_source_areas", [])
	var labelled: Array = scene.get_meta("map_labelled", [])
	var counts: Array = scene.get_meta("map_counts", [])
	var two_rows: float = 2.0 * CELL * view.get_scale().y
	var chef: Dictionary = scene.survival_status()["chef"]
	var chef_x: int = chef["sub_x"]
	var chef_y: int = chef["sub_y"]
	var feet: Vector2 = view * (ORIGIN + Vector2(chef_x, chef_y))
	for index: int in range(labelled.size()):
		var area: Rect2 = areas[sources.find(labelled[index])]
		var box: Rect2 = counts[index]
		if box.position.y < area.end.y - two_rows or box.end.y > area.end.y:
			_fail("the badge of %s floats off its source: %s over %s" % [labelled[index], box, area])
	if labelled.is_empty():
		_fail("no source of the hall carries its badge")
	for index: int in range(plates.size()):
		var box: Rect2 = plates[index]
		if words[index] in ["P1", "P2"] and (box.position.y < feet.y - two_rows or box.end.y > feet.y):
			_fail("the tag %s floats off the room of its human: %s over the feet at %s" % [words[index], box, feet])
	if not stray.is_empty():
		_fail("badges or tags float outside the building: %s" % [stray.slice(0, 3)])

## The opaque pixels of a drawn character layer, in the screen units its area is in.
func _opaque_of(layer: Dictionary) -> Rect2:
	var texture: Texture2D = layer["texture"]
	var area: Rect2 = layer["area"]
	var used: Rect2 = Rect2(texture.get_image().get_used_rect())
	var ratio: Vector2 = area.size / texture.get_size()
	var left: float = used.position.x
	if layer["mirrored"]:
		left = texture.get_size().x - used.end.x
	return Rect2(area.position + Vector2(left, used.position.y) * ratio, used.size * ratio)

## Where two humans' bodies overlap on screen, the one behind keeps an outline of its own shape and
## its player's colour drawn over the one in front: a ring with pixels, on the hidden body's frame.
func _check_outlines(where: String) -> void:
	var world: Node = _world()
	var front: Dictionary = {}
	var back: Dictionary = {}
	for layer: Dictionary in world.get_meta("character_layers", []):
		if layer["role"] == "chef":
			back = layer
		else:
			front = layer
	if back.is_empty() or front.is_empty():
		_fail("%s: both humans were not drawn: %s" % [where, world.get_meta("character_layers", [])])
		return
	var back_area: Rect2 = back["area"]
	var hidden: Rect2 = _opaque_of(back)
	if not hidden.intersects(_opaque_of(front)):
		_fail("%s: the two humans share a cell but their bodies do not overlap on screen" % where)
		return
	var found: bool = false
	for mark: Dictionary in world.get_meta("actor_outlines", []):
		var colour: Color = mark["colour"]
		var pixels: int = mark["pixels"]
		var area: Rect2 = mark["area"]
		if mark["role"] == "chef" and pixels > 0 and colour.a > 0.0 and colour.v > 0.2 and area.is_equal_approx(back_area):
			found = true
	if not found:
		_fail("%s: the chef's body is overlapped by the appraiser's with no visible outline of its own over it: %s" % [where, world.get_meta("actor_outlines", [])])

## When player one leaves, the AI takes the chef over, and the chef's ring turns to the AI's neutral
## colour: in every frame the ring's colour is the one of the key set the status reports for the chef.
func _check_hand_off_outline() -> void:
	var world: Node = _world()
	_key(KEY_X, true)
	var changed: bool = false
	for frame: int in range(30):
		await process_frame
		var driver: int = scene.survival_status()["chef"]["key_set"]
		var driven_by_ai: bool = driver < 0
		for mark: Dictionary in world.get_meta("actor_outlines", []):
			var pixels: int = mark["pixels"]
			if mark["role"] != "chef" or pixels == 0:
				continue
			var colour: Color = mark["colour"]
			var want: Color = AI_OUTLINE if driven_by_ai else FIRST_PLAYER_OUTLINE
			changed = changed or driven_by_ai
			if not colour.is_equal_approx(want):
				_fail("frame %d: the chef's ring is %s while its key set is %d" % [frame, colour, driver])
		if frame == 1:
			_key(KEY_X, false)
	_key(KEY_X, false)
	if not changed:
		_fail("player one leaving did not leave the chef to the AI with a ring drawn")

## The stair's treads sit on the floor line its walker's feet keep, and run down to the floor beside it.
func _check_stair() -> void:
	var body: Dictionary = scene.survival_status()["chef"]
	var column: int = body["x"]
	var row: int = body["y"]
	var stance: String = body["stance"]
	var sub_y: int = body["sub_y"]
	var feet: float = ORIGIN.y + float(sub_y)
	var floor_line: float = ORIGIN.y + float(STAIR.y + 1) * CELL
	if Vector2i(column, row) != STAIR or stance != "standing" or not is_equal_approx(feet, floor_line):
		_fail("the chef is not standing on the stair's floor line: %s" % [body])
		return
	var risers: Array = _world().get_meta("stair_risers", [])
	var meets: bool = false
	for riser: Dictionary in risers:
		var tread: float = riser["tread"]
		var bottom: float = riser["bottom"]
		if riser["cell"] == STAIR:
			meets = meets or (is_equal_approx(tread, feet) and is_equal_approx(bottom, ORIGIN.y + float(LOWER.y + 1) * CELL))
	if not meets:
		_fail("no drawn tread of the stair stands on the feet's line %s and reaches the floor below it: %s" % [feet, risers])

## The floor above the stair is joined down to the stair's tread, and its lower floor has none of its own to join.
func _check_neighbours() -> void:
	var above: bool = false
	for riser: Dictionary in _world().get_meta("stair_risers", []):
		var tread: float = riser["tread"]
		var bottom: float = riser["bottom"]
		if riser["cell"] == UPPER:
			above = above or (is_equal_approx(tread, ORIGIN.y + float(UPPER.y + 1) * CELL) and is_equal_approx(bottom, ORIGIN.y + float(STAIR.y + 1) * CELL))
	if not above:
		_fail("the floor above the stair is not joined down to the stair's tread")

## Lets the camera, the covers and the plates reach their rest.
func _settle() -> void:
	for frame: int in range(40):
		clock += 0.2
		scene.pin_scene_clock(clock)
		await process_frame
		if sampling:
			_sample()

## Walks the chef to `target` and the appraiser to `partner` on real keys.
func _move(target: Vector2i, partner: Vector2i) -> bool:
	var last: Dictionary[String, int] = {}
	var stalled: Dictionary[String, int] = {}
	for frame: int in range(900):
		var state: Dictionary = scene.survival_status()
		var reached: bool = true
		for role: String in ["chef", "appraiser"]:
			var body: Dictionary = state[role]
			var goal: Vector2i = target if role == "chef" else partner
			var column: int = body["x"]
			var row: int = body["y"]
			var at: Vector2i = Vector2i(column, row)
			var standing: String = body["stance"]
			var done: bool = at == goal and standing != "airborne"
			reached = reached and done
			var left: Key = KEY_A if role == "chef" else KEY_J
			var right: Key = KEY_D if role == "chef" else KEY_L
			var up: Key = KEY_W if role == "chef" else KEY_I
			var down: Key = KEY_S if role == "chef" else KEY_K
			var jump: Key = KEY_SPACE if role == "chef" else KEY_SEMICOLON
			var run: Key = KEY_SHIFT if role == "chef" else KEY_H
			var sideways: bool = not done and goal.x != at.x
			_key(left, sideways and goal.x < at.x)
			_key(right, sideways and goal.x > at.x)
			_key(up, not done and not sideways and goal.y < at.y)
			_key(down, not done and not sideways and goal.y > at.y)
			_key(run, sideways and absi(goal.x - at.x) > 3)
			var sub_x: int = body["sub_x"]
			stalled[role] = stalled.get(role, 0) + 1 if sideways and sub_x == last.get(role, -1) and standing == "standing" else 0
			last[role] = sub_x
			var gap_takeoff: bool = goal.y < at.y and goal.x - at.x == 2 and sub_x % 32 >= 16
			var climb_step: bool = goal.y < at.y and absi(goal.x - at.x) <= 1
			_key(jump, not done and standing == "standing" and (gap_takeoff or climb_step or stalled[role] > 6))
		if reached:
			_release_movement()
			await process_frame
			await process_frame
			return true
		if state["phase"] != "exploration" or state["chef"]["location"] != "city" or state["appraiser"]["location"] != "city":
			_abort("both humans left the city before %s: %s" % [target, state])
			return false
		clock += 0.1
		scene.pin_scene_clock(clock)
		await process_frame
		if sampling:
			_sample()
		if entering and not capture.is_empty():
			entry_frames += 1
			if entry_frames % 3 == 0:
				await _save("split-2-entry-%03d" % entry_frames)
	_abort("did not reach %s: %s" % [target, scene.survival_status()])
	return false

func _abort(reason: String) -> void:
	push_error(reason)
	print("Interior: finished: fail; ", reason)
	quit(1)

func _release_movement() -> void:
	for code: Key in [KEY_A, KEY_D, KEY_W, KEY_S, KEY_SPACE, KEY_SHIFT, KEY_J, KEY_L, KEY_I, KEY_K, KEY_SEMICOLON, KEY_H]:
		_key(code, false)

func _tap(code: Key) -> void:
	_key(code, true)
	await process_frame
	await process_frame
	_key(code, false)
	for frame: int in range(8):
		await process_frame

func _key(code: Key, pressed: bool) -> void:
	if held_keys.get(code, false) == pressed:
		return
	held_keys[code] = pressed
	var event: InputEventKey = InputEventKey.new()
	event.physical_keycode = code
	event.pressed = pressed
	Input.parse_input_event(event)
