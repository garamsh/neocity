class_name StairsCheck
extends SceneTree
## Checks drawn floor contact against real walking in both directions, including the adjacent flat deck.

const ORIGIN: Vector2 = Vector2(64, 70)
const CELL: float = 32.0
## Canvas pixels to a layout unit: the 1920x1080 canvas over the 1280x720 layout.
const CANVAS_SCALE: float = 1.5
## Half a drawn body's width, in layout pixels, the chef's feet must stay inside the screen by.
const CHEF_HALF: float = 48.0
const CELLS: Array[Vector2i] = [Vector2i(9, 9), Vector2i(10, 9)]
## How far, in scene pixels, the control reads the drawn contact line from the real floor line, up and down.
const OFFSETS: Array[float] = [-8.0, -1.0, 1.0, 8.0]
var scene: MainScene
var edge_logged: int = 0
var clock: float = 0.0
var pixels: Dictionary = {}
var measured: Dictionary = {}
var failures: int = 0
var held_keys: Dictionary[Key, bool] = {}

func _initialize() -> void:
	Engine.max_fps = 60
	_run.call_deferred()

## Whether Godot runs at --fixed-fps 60, which the engine keeps from the
## command line the script reads: five frames in a row each last exactly a
## sixtieth of a second, as no real clock keeps them.
func _fixed_rate() -> bool:
	for _frame: int in range(5):
		await process_frame
		if not is_equal_approx(root.get_process_delta_time(), 1.0 / 60.0):
			return false
	return true

func _run() -> void:
	await process_frame
	scene = current_scene as MainScene
	scene.get_node("World").set_meta("measure_art", true)
	scene.pin_survival_seed(11)
	scene.pin_scene_clock(clock)
	if OS.get_cmdline_user_args().has("--map-art-tour") or OS.get_cmdline_user_args().has("--kit-interiors"):
		if not await _fixed_rate():
			_tour_fail("the tour steers by frames and a day ends by the frame's delta, so run it with --fixed-fps 60: a loaded machine's longer frames make a day end before the humans arrive")
			return
		if OS.get_cmdline_user_args().has("--kit-interiors"):
			await _kit_interiors()
		else:
			await _map_art_tour()
		return
	if OS.get_cmdline_user_args().has("--map-density"):
		await _map_density()
		return
	await _enter_city()
	for direction: Key in [KEY_D, KEY_A, KEY_D]:
		_key(direction, true)
		for frame: int in range(600):
			clock += 0.1
			scene.pin_scene_clock(clock)
			await process_frame
			await RenderingServer.frame_post_draw
			var body: Dictionary = scene.survival_status()["chef"]
			var column: int = body["x"]
			var absolute: int = body["sub_x"]
			if (direction == KEY_D and column >= 11):
				break
			var row: int = body["y"]
			var cell: Vector2i = Vector2i(column, row)
			if not CELLS.has(cell) or body["stance"] != "standing":
				continue
			var name: String = "%s %s" % [cell, "right" if direction == KEY_D else "left"]
			if measured.has(name):
				if direction == KEY_A and column == 9 and absolute % 32 <= 16:
					break
				continue
			var floor_y: int = (cell.y + 1) * 32
			var body_y: int = body["sub_y"]
			var valid: bool = body_y == floor_y
			var view: Transform2D = scene.get_meta("map_view", Transform2D())
			var top: Vector2 = ORIGIN + Vector2(cell + Vector2i.DOWN) * CELL
			for x: float in [0.5, 16.0, 31.5]:
				valid = _stone(view * (top + Vector2(x, 0.25))) and valid
			# Both sides of the same-row join must retain their contact edge.
			var join: Vector2 = ORIGIN + Vector2(cell.x + 1, 10) * CELL
			valid = _stone(view * (join + Vector2(-0.1, 0.25))) and _stone(view * (join + Vector2(0.1, 0.25))) and valid
			measured[name] = valid
			failures += 0 if valid else 1
			print("Stairs: %s: %s; body sub=(%s,%s), floor_line=%d; contact x=0.5,16,31.5 and right join." % [name, "PASS" if valid else "FAIL", body["sub_x"], body["sub_y"], floor_y])
			if direction == KEY_A and column == 9 and absolute % 32 <= 16:
				break
		_key(direction, false)
		await process_frame
	var enough: bool = measured.size() == 4 and check_floor_lines(scene, pixels) and floor_line_offsets_fail(scene, pixels)
	print("Stairs: finished: %s; %d/4 direction/cell readings, %d failures." % ["pass" if enough and failures == 0 else "fail", measured.size(), failures])
	quit(0 if enough and failures == 0 else 1)

## Starts a run with two humans and sends the chef out onto the street.
func _enter_city() -> void:
	await _tap(KEY_1)
	await _tap(KEY_1)
	await _tap(KEY_E)
	await _tap(KEY_Q)

## The street's map art as the last frame drew it: each picture's texture path
## and the area of the scene it covered.
func _map_layers() -> Array[Dictionary]:
	var layers: Array[Dictionary] = []
	for layer: Dictionary in scene.get_node("World").get_meta("foreground_layers", []):
		var texture: Texture2D = layer["texture"]
		var source: Rect2 = layer["source"]
		layers.append({"path": texture.resource_path, "area": layer["destination"], "pixels": source.size, "texture": texture.get_size(), "kind": texture.get_class()})
	return layers

## The characters the last frame drew whose picture is not drawn at one texture
## pixel to a canvas pixel at the close camera, which is the density the drawing
## derives for art that no table sizes by world area. Fails to read none, so a
## frame that drew no character is reported too.
func _characters_off_native_density() -> Array[String]:
	var found: Array[String] = []
	var layers: Array = scene.get_node("World").get_meta("character_layers", [])
	if layers.is_empty():
		found.append("no character was drawn")
	for layer: Dictionary in layers:
		var texture: Texture2D = layer["texture"]
		var area: Rect2 = layer["area"]
		var drawn: Vector2 = area.size * CANVAS_SCALE
		if not drawn.is_equal_approx(texture.get_size()):
			found.append("%s drawn %s for %s pixels" % [layer["role"], drawn, texture.get_size()])
	return found

## Checks that map art is drawn by the world area it covers: the street is
## drawn, then again after every world, ground and walkway picture is replaced
## by itself at twice the pixels, and no picture may change its area.
func _map_density() -> void:
	await _enter_city()
	var before: Array[Dictionary] = _map_layers()
	var unsized: Array[String] = _characters_off_native_density()
	var folders: Array[String] = ["world", "ground", "walkway"]
	var finer: int = 0
	var kept: Array[Texture2D] = []
	for folder: String in folders:
		var directory: String = "res://levels/monster_mouth_escape/art_hd/" + folder
		for file: String in DirAccess.get_files_at(directory):
			if not file.ends_with(".png"):
				continue
			var texture: Texture2D = load(directory.path_join(file))
			var image: Image = texture.get_image()
			image.resize(image.get_width() * 2, image.get_height() * 2, Image.INTERPOLATE_NEAREST)
			var replacement: ImageTexture = ImageTexture.create_from_image(image)
			var where: String = texture.resource_path
			replacement.take_over_path(where)
			kept.append(replacement)
			finer += 1
	change_scene_to_file("res://main.tscn")
	for frame: int in range(4):
		await process_frame
	scene = current_scene as MainScene
	scene.get_node("World").set_meta("measure_art", true)
	scene.pin_survival_seed(11)
	scene.pin_scene_clock(clock)
	await _enter_city()
	var after: Array[Dictionary] = _map_layers()
	var moved: Array[String] = []
	var denser: bool = false
	if before.is_empty() or before.size() != after.size():
		moved.append("the street drew %d pictures, then %d" % [before.size(), after.size()])
	else:
		for index: int in range(before.size()):
			var was: Dictionary = before[index]
			var now: Dictionary = after[index]
			var was_area: Rect2 = was["area"]
			var now_area: Rect2 = now["area"]
			if not was_area.position.is_equal_approx(now_area.position) or not was_area.size.is_equal_approx(now_area.size):
				moved.append("%s covered %s, then %s" % [was["path"], was_area, now_area])
			var was_pixels: Vector2 = was["pixels"]
			var now_pixels: Vector2 = now["pixels"]
			denser = denser or now_pixels.is_equal_approx(was_pixels * 2.0)
	var passed: bool = moved.is_empty() and denser and finer > 0 and unsized.is_empty()
	print("Stairs: finished: %s; %d pictures replaced at twice the pixels, %d drawn, %d moved or resized %s, finer pixels read: %s, %d characters off native density %s." % ["pass" if passed else "fail", finer, before.size(), moved.size(), moved.slice(0, 3), denser, unsized.size(), unsized])
	quit(0 if passed else 1)

## Captures both single-floor shops through their actual ladder routes.
func _kit_interiors() -> void:
	OS.set_environment("NEOCITY_BASE_SECONDS", "0")
	await _tap(KEY_1)
	await _tap(KEY_3)
	await _tap(KEY_E)
	await _tap(KEY_Q)
	await _tap(KEY_U)
	for target: Vector2i in [Vector2i(3, 8), Vector2i(3, 6)]:
		if not await _tour_move(target):
			return
	if not await _shop_interior("276-interior-kitchen", "world/interior_kitchen.png"):
		return
	for target: Vector2i in [Vector2i(3, 8), Vector2i(11, 9), Vector2i(13, 9), Vector2i(16, 9), Vector2i(17, 8), Vector2i(23, 8), Vector2i(23, 6)]:
		if not await _tour_move(target):
			return
	if not await _shop_interior("276-interior-noodle", "world/interior_noodle.png"):
		return
	print("Stairs: finished: pass; kitchen and noodle shop use their own pictures after paired ladder travel.")
	quit(0)

## Plays two humans through both maps at every drawn damage stage.
func _map_art_tour() -> void:
	# An untimed base and an instant cook: the first meal is cooked by hand at
	# once, as before the cook took base time, while the day length stays default.
	OS.set_environment("NEOCITY_BASE_SECONDS", "0")
	OS.set_environment("NEOCITY_COOK_SECONDS", "0")
	await _tap(KEY_1)
	await _tap(KEY_3)
	await _tap(KEY_E)
	for day: int in [1, 5, 10]:
		var current_day: int = scene.survival_status()["day"]
		while current_day < day:
			await _tap(KEY_G)
			current_day = scene.survival_status()["day"]
		var state: Dictionary = scene.survival_status()
		if state["day"] != day or state["phase"] != "workshop" or state["chef"]["key_set"] != 0 or state["appraiser"]["key_set"] != 1:
			_tour_fail("requested day/control not established: " + _tour_status(state))
			return
		await _tap(KEY_Q)
		await _tap(KEY_U)
		if not await _tour_shot("230-street-day-%d" % day):
			return
		var route: Array[Vector2i] = [Vector2i(11, 9), Vector2i(13, 9), Vector2i(16, 9), Vector2i(17, 8), Vector2i(29, 9), Vector2i(31, 9)]
		if day < 10:
			route.append(Vector2i(35, 9))
			route.append(Vector2i(36, 8))
		else:
			route.append(Vector2i(34, 13))
			route.append(Vector2i(36, 12))
			route.append(Vector2i(36, 8))
		route.append_array([
			Vector2i(38, 8), Vector2i(40, 7), Vector2i(44, 6),
			Vector2i(47, 6), Vector2i(47, 4), Vector2i(39, 4),
			Vector2i(42, 4), Vector2i(42, 2), Vector2i(46, 2),
			Vector2i(48, 2), Vector2i(49, 3), Vector2i(50, 4),
			Vector2i(51, 3), Vector2i(52, 2), Vector2i(53, 2),
			Vector2i(52, 2), Vector2i(51, 3), Vector2i(50, 4),
			Vector2i(49, 3), Vector2i(48, 2), Vector2i(42, 2), Vector2i(42, 4), Vector2i(47, 4), Vector2i(47, 6),
			Vector2i(49, 8), Vector2i(50, 7), Vector2i(51, 6), Vector2i(52, 5), Vector2i(60, 5), Vector2i(60, 11)])
		var regrouped: bool = false
		for target: Vector2i in route:
			if regrouped and target in [Vector2i(40, 7), Vector2i(44, 6)]:
				continue
			if not await _tour_move(target):
				return
			if day == 1 and target == Vector2i(38, 8):
				if not await _tour_party_in_cold_store():
					return
				regrouped = true
			if target == Vector2i(39, 4):
				if not await _tour_interior("271-cold-store-day-%d" % day, 3, "world/interior_cold_store.png"):
					return
			if day == 1 and target == Vector2i(11, 9):
				if not await _tour_collect():
					return
		if not await _tour_move(Vector2i(62, 11)):
			return
		state = scene.survival_status()
		if state["map"] != "Aquarium" or state["chef"]["x"] != 4 or state["appraiser"]["x"] != 4 or state["chef"]["y"] != 10 or state["appraiser"]["y"] != 10:
			_tour_fail("paired exit did not place both humans at aquarium arrival (4,10): " + _tour_status(state))
			return
		print("Stairs: paired travel day %d: Aquarium arrival (4,10), two human key sets 0/1." % day)
		for target: Vector2i in [Vector2i(9, 10), Vector2i(9, 7), Vector2i(6, 7), Vector2i(4, 7), Vector2i(4, 4), Vector2i(8, 4), Vector2i(4, 4), Vector2i(4, 7), Vector2i(9, 7), Vector2i(9, 10), Vector2i(7, 10), Vector2i(11, 10), Vector2i(12, 9), Vector2i(13, 8), Vector2i(17, 8)]:
			if not await _tour_move(target):
				return
			if target == Vector2i(6, 7):
				if not await _tour_interior("271-service-room-day-%d" % day, 2, "world/interior_service.png"):
					return
			if day == 1 and target == Vector2i(7, 10):
				if not await _tour_collect():
					return
		if not await _tour_shot("230-aquarium-day-%d" % day):
			return
		await _tap(KEY_Q)
		await _tap(KEY_U)
		if day == 1:
			for attempt: int in range(8):
				await _tap(KEY_O)
				await _tap(KEY_E)
				var foods: int = scene.survival_status()["foods"]
				if foods > 0:
					break
			await _tap(KEY_F)
			state = scene.survival_status()
			print("Stairs: human meal: day=%s acid=%s reaction=%s score=%s." % [state["day"], state["acid"], state.get("reaction", ""), state.get("score", 0)])
			if state["day"] != 2 or state["acid"] != 0:
				_tour_fail("the human-cooked first meal did not preserve the day-ten run: " + _tour_status(state))
				return
	await _tap(KEY_Q)
	await _tap(KEY_U)
	for target: Vector2i in [Vector2i(3, 8), Vector2i(3, 6)]:
		if not await _tour_move(target):
			return
	if not await _shop_interior("230-interior-kitchen", "world/interior_kitchen.png"):
		return
	print("Stairs: finished: pass; both maps on days 1,5,10, paired human travel, native evidence and floor lines.")
	quit(0)

## Checks the occupied shop's actual draw commands select its own picture.
func _shop_interior(name: String, picture: String) -> bool:
	if not await _tour_shot(name):
		return false
	var inside: bool = false
	for layer: Dictionary in scene.get_node("World").get_meta("foreground_layers", []):
		var texture: Texture2D = layer["texture"]
		inside = inside or texture.resource_path.ends_with(picture)
	if not inside:
		_tour_fail("the occupied shop did not draw " + picture)
	return inside

## Checks the Cold Store display when the two humans are not on one floor of
## it: with one outside, only the floor the other stands on is lit and every
## other floor is covered solid; with both inside on different floors, both
## floors are lit and the camera holds both. Ends with both on the middle floor.
func _tour_party_in_cold_store() -> bool:
	if not await _tour_move(Vector2i(38, 8), Vector2i(36, 8)):
		return false
	if not await _tour_shot("281-cold-store-one-out"):
		return false
	var world: Node = scene.get_node("World")
	var rooms: Array = world.get_meta("interior_rooms", [])
	if rooms.size() != 3 or world.get_meta("inside_view", false):
		_tour_fail("one human out of the Cold Store still leaves the outside on screen with its three floors: " + str(rooms.size()))
		return false
	for room: Dictionary in rooms:
		var floor_y: float = room["floor_y"]
		var ground: bool = is_equal_approx(floor_y, _lowest_floor(rooms))
		var covered: float = room["cover"]
		if room["lit"] != ground or (ground and covered > 0.01) or (not ground and covered < 0.99):
			_tour_fail("with one human out, the floor of the other must be alone lit and the rest covered solid: " + str(room))
			return false
	if not await _tour_move(Vector2i(40, 7), Vector2i(44, 6)):
		return false
	if not await _tour_shot("281-cold-store-two-floors"):
		return false
	rooms = world.get_meta("interior_rooms", [])
	var view: Transform2D = scene.get_meta("map_view", Transform2D())
	var lit: int = 0
	for room: Dictionary in rooms:
		var area: Rect2 = room["area"]
		var covered: float = room["cover"]
		var floor_y: float = room["floor_y"]
		var upper: bool = is_equal_approx(floor_y, _highest_floor(rooms))
		if room["lit"] == upper or (room["lit"] and covered > 0.01) or (not room["lit"] and covered < 0.8):
			_tour_fail("with the humans on two floors, both of their floors must be lit and the top one covered: " + str(room))
			return false
		if room["lit"]:
			lit += 1
			if not Rect2(0, 0, 1280, 720).encloses(view * area):
				_tour_fail("the camera does not hold the lit floor " + str(room["name"]))
				return false
	if rooms.size() != 3 or lit != 2 or not world.get_meta("inside_view", false):
		_tour_fail("the two floors are not the only inside on screen: " + str(lit))
		return false
	print("Stairs: Cold Store with one human out, then on two floors: lit floors and covers as the display rules say.")
	return await _tour_move(Vector2i(44, 6))

func _lowest_floor(rooms: Array) -> float:
	var lowest: float = -INF
	for room: Dictionary in rooms:
		var floor_y: float = room["floor_y"]
		lowest = maxf(lowest, floor_y)
	return lowest

func _highest_floor(rooms: Array) -> float:
	var highest: float = INF
	for room: Dictionary in rooms:
		var floor_y: float = room["floor_y"]
		highest = minf(highest, floor_y)
	return highest

## Checks that all room bands end at their collision floors and the outside darkens.
func _tour_interior(name: String, count: int, picture: String) -> bool:
	if not await _tour_shot(name):
		return false
	var world: Node = scene.get_node("World")
	var rooms: Array = world.get_meta("interior_rooms", [])
	if rooms.size() != count or not world.get_meta("inside_view", false):
		_tour_fail("the occupied building lacks its rooms or shows more than its inside: " + name)
		return false
	if not _inside_is_framed(world, rooms):
		_tour_fail("the building's lit floor is not alone bright and framed to fill the screen: " + name)
		return false
	var fragments: Array = []
	for fragment: Dictionary in world.get_meta("foreground_layers", []):
		var texture: Texture2D = fragment["texture"]
		if texture.resource_path.ends_with(picture):
			fragments.append(fragment)
	var view: Transform2D = scene.get_meta("map_view", Transform2D())
	for room: Dictionary in rooms:
		var area: Rect2 = room["area"]
		var floor_y: float = room["floor_y"]
		if not _stone_layers(view * Vector2(area.position.x + 16.0, area.end.y - 16.0), fragments, pixels):
			_tour_fail("the room has no opaque interior artwork above its floor: " + str(room["name"]))
			return false
		if not is_equal_approx(area.end.y, floor_y):
			_tour_fail("the room background ends away from its floor: " + str(room["name"]))
			return false
	print("Stairs: interior %s: %d floor-aligned rooms, only the inside on screen." % [name, rooms.size()])
	return true

## Whether the floors nobody stands on are covered, the lit floor is not, and
## the lit floor fills the play area in one dimension at a whole zoom.
func _inside_is_framed(world: Node, rooms: Array) -> bool:
	var view: Transform2D = scene.get_meta("map_view", Transform2D())
	var lit: int = 0
	for room: Dictionary in rooms:
		var covered: float = room["cover"]
		if room["lit"]:
			lit += 1
			var area: Rect2 = room["area"]
			var shown: Rect2 = view * area
			if covered > 0.01 or maxf(shown.size.x / 1280.0, shown.size.y / 478.0) < 0.7 or not is_equal_approx(view.get_scale().x, roundf(view.get_scale().x)):
				return false
		elif covered < 0.8:
			return false
	var layers: Array = world.get_meta("backdrop_layers", [null])
	return lit >= 1 and layers.is_empty()

## Whether the chef, once on the street's far columns, is drawn with room to
## spare either side of its feet: its feet's screen position a body's half width
## inside the screen, where the camera clamps at the end of the map.
func _chef_whole(state: Dictionary) -> bool:
	var body: Dictionary = state["chef"]
	if body["location"] != "city" or body["x"] < 58:
		return true
	var view: Transform2D = scene.get_meta("map_view", Transform2D())
	var sub_x: int = body["sub_x"]
	var feet: Vector2 = view * (ORIGIN + Vector2(sub_x, 0))
	var shown: bool = feet.x >= CHEF_HALF and feet.x <= 1280.0 - CHEF_HALF
	if edge_logged < 3:
		edge_logged += 1
		print("Stairs: chef at column %d is drawn %.1f of 1280 across (needs %.0f to %.0f)." % [body["x"], feet.x, CHEF_HALF, 1280.0 - CHEF_HALF])
	return shown

## Walks the chef to `target` and the appraiser to `partner`, or to `target` too.
func _tour_move(target: Vector2i, other: Vector2i = Vector2i(-1, -1)) -> bool:
	var partner: Vector2i = target if other == Vector2i(-1, -1) else other
	var last: Dictionary[String, int] = {}
	var stalled: Dictionary[String, int] = {}
	for frame: int in range(600):
		var state: Dictionary = scene.survival_status()
		var reached: bool = true
		for role: String in ["chef", "appraiser"]:
			var body: Dictionary = state[role]
			var goal: Vector2i = target if role == "chef" else partner
			var column: int = body["x"]
			var row: int = body["y"]
			var at: Vector2i = Vector2i(column, row)
			var done: bool = state["map"] == "Aquarium" and target == Vector2i(62, 11) or at == goal and body["stance"] != "airborne"
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
			stalled[role] = stalled.get(role, 0) + 1 if sideways and sub_x == last.get(role, -1) and body["stance"] == "standing" else 0
			last[role] = sub_x
			var climb_step: bool = goal.y < at.y and absi(goal.x - at.x) <= 1
			var gap_takeoff: bool = goal.y < row and goal.x - column == 2 and sub_x % 32 >= 16
			var jump_pressed: bool = not done and body["stance"] == "standing" and (gap_takeoff or climb_step or stalled[role] > 6)
			_key(jump, jump_pressed)
		if not _chef_whole(state):
			_tour_fail("the camera cut the chef off at the map's edge: " + _tour_status(state))
			return false
		if reached:
			_release_movement()
			await process_frame
			await process_frame
			print("Stairs: tour day %s reached %s on %s; chef %s, appraiser %s." % [state["day"], target, state["map"], state["chef"], state["appraiser"]])
			return true
		if state["phase"] != "exploration" or state["chef"]["location"] != "city" or state["appraiser"]["location"] != "city":
			_tour_fail("both humans left the city before %s: %s" % [target, _tour_status(state)])
			return false
		if target == Vector2i(62, 11) and frame % 12 == 0:
			print("Stairs: exit zone x=62..63 y=10..11 after movement; chef %s; appraiser %s; map %s." % [state["chef"], state["appraiser"], state["map"]])
		clock += 0.1
		scene.pin_scene_clock(clock)
		await process_frame
	_tour_fail("did not reach %s; %s" % [target, _tour_status(scene.survival_status())])
	return false

func _tour_collect() -> bool:
	var before: int = scene.survival_status()["chef"]["ingredients"]
	await _tap(KEY_R)
	await _tap(KEY_E)
	await _tap(KEY_R)
	for frame: int in range(180):
		var body: Dictionary = scene.survival_status()["chef"]
		var ingredients: int = body["ingredients"]
		if ingredients > before:
			return true
		await process_frame
	_tour_fail("human search did not add an ingredient: " + _tour_status(scene.survival_status()))
	return false

func _tour_status(state: Dictionary) -> String:
	return "day=%s phase=%s map=%s acid=%s remaining_ms=%s chef=%s appraiser=%s" % [state["day"], state["phase"], state["map"], state["acid"], state["remaining_ms"], state["chef"], state["appraiser"]]

func _release_movement() -> void:
	for code: Key in [KEY_A, KEY_D, KEY_W, KEY_S, KEY_SPACE, KEY_SHIFT, KEY_J, KEY_L, KEY_I, KEY_K, KEY_SEMICOLON, KEY_H]:
		_key(code, false)

func _tour_shot(name: String) -> bool:
	var previous_scale: float = Engine.time_scale
	Engine.time_scale = 0.0
	for frame: int in range(3):
		clock += 1.0
		scene.pin_scene_clock(clock)
		await process_frame
		await RenderingServer.frame_post_draw
	var frame: Image = root.get_texture().get_image()
	var directory: String = ProjectSettings.globalize_path("user://survival")
	DirAccess.make_dir_recursive_absolute(directory)
	var valid: bool = frame.get_size() == Vector2i(1920, 1080) and check_floor_lines(scene, pixels)
	var result: Error = frame.save_png(directory.path_join(name + ".png"))
	await process_frame
	Engine.time_scale = previous_scale
	if not valid or result != OK:
		_tour_fail("invalid native capture or floor line: " + name)
		return false
	print("Stairs: saved %s/%s.png at native 1920x1080." % [directory, name])
	return true

func _tour_fail(reason: String) -> void:
	push_error(reason)
	print("Stairs: finished: fail; ", reason)
	quit(1)

## Reads the collision snapshot's floors through the actual deck commands, including offscreen cells.
static func check_floor_lines(subject: MainScene, cache: Dictionary = {}) -> bool:
	var fault: String = _floor_line_fault(subject, cache, 0.0)
	if not fault.is_empty():
		push_error(fault)
		return false
	return true

## Whether the drawn contact line sits `offset` scene pixels away from the real
## floor line on every intact terrain floor, which the check must tell from one
## that sits on it: probes every floor as check_floor_lines does with the line
## read at floor + offset, and returns what it found wrong, or an empty string
## where it found nothing.
static func _floor_line_fault(subject: MainScene, cache: Dictionary, offset: float) -> String:
	var world: Node = subject.get_node("World")
	var floors: Array = world.get_meta("collision_floors", [])
	var layers: Array = world.get_meta("deck_layers", [])
	var view: Transform2D = subject.get_meta("map_view", Transform2D())
	var cells: int = 0
	var controls: int = 0
	var voids: int = 0
	for floor_record: Dictionary in floors:
		var area: Rect2 = floor_record["area"]
		var floor_y: float = area.end.y
		var standing: bool = floor_record["standing"]
		for column: int in range(roundi(area.position.x), roundi(area.end.x), 32):
			if not standing:
				for x: float in [0.5, 16.0, 31.5]:
					if offset == 0.0 and _stone_layers(view * Vector2(column + x, floor_y + 0.25), layers, cache):
						return "Stairs: absent terrain %s still draws a floor at x=%s." % [floor_record["name"], column + x]
				voids += 1
				continue
			var top_layers: Array = []
			for layer: Dictionary in layers:
				var destination: Rect2 = layer["destination"]
				if is_equal_approx(destination.position.y, floor_y) and destination.position.x < column + CELL and destination.end.x > column:
					top_layers.append(layer)
			if top_layers.is_empty():
				return "Stairs: no deck begins at collision floor %s x=%d." % [floor_record["name"], column]
			for x: float in [0.5, 16.0, 31.5]:
				var hit: Vector2 = view * Vector2(column + x, floor_y + offset + 0.25)
				var miss: Vector2 = view * Vector2(column + x, floor_y + offset - 0.25)
				if not _stone_layers(hit, top_layers, cache) or _stone_layers(miss, top_layers, cache):
					return "Stairs: floor %s x=%s top=%s lost contact or confused the nearest row above it." % [floor_record["name"], column + x, floor_y]
				controls += 1
			cells += 1
	if offset == 0.0:
		print("Stairs: %s day %s: %d terrain cells have opaque contact at x=0.5,16,31.5; %d nearest-row-above controls returned nothing." % [subject.survival_status()["map"], subject.survival_status()["day"], cells, controls])
		print("Stairs: %d dormant or destroyed terrain cells correctly draw no collision deck." % voids)
	return "" if cells > 0 else "Stairs: no terrain floor was read."

## Whether the floor-contact reading fails for a contact line drawn OFFSETS
## scene pixels above or below the real floor line, in each direction, so a
## drawn line that drifts from where the feet stand is not read as matching.
static func floor_line_offsets_fail(subject: MainScene, cache: Dictionary = {}) -> bool:
	for offset: float in OFFSETS:
		var fault: String = _floor_line_fault(subject, cache, offset)
		print("Stairs: control, the contact line read %+.1f scene pixels from the floor line: %s." % [offset, "rejected" if not fault.is_empty() else "ACCEPTED"])
		if fault.is_empty():
			return false
	return true

## Reads alpha through the actual texture, crop and draw transform, excluding all scenery and actors.
func _stone(point: Vector2) -> bool:
	var layers: Array = scene.get_node("World").get_meta("deck_layers", [])
	return _stone_layers(point, layers, pixels)

static func _stone_layers(point: Vector2, layers: Array, cache: Dictionary) -> bool:
	for layer: Dictionary in layers:
		var transform: Transform2D = layer["transform"]
		var destination: Rect2 = layer["destination"]
		var local: Vector2 = transform.affine_inverse() * point
		if not destination.has_point(local):
			continue
		var texture: Texture2D = layer["texture"]
		var id: int = texture.get_instance_id()
		if not cache.has(id):
			cache[id] = texture.get_image()
		var source: Rect2 = layer["source"]
		var at: Vector2i = Vector2i((source.position + (local - destination.position) / destination.size * source.size).floor())
		var image: Image = cache[id]
		if Rect2i(Vector2i.ZERO, image.get_size()).has_point(at) and image.get_pixelv(at).a >= 0.99:
			return true
	return false

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
