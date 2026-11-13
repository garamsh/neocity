class_name StairsCheck
extends SceneTree
## Checks drawn floor contact against real walking in both directions, including the adjacent flat deck.

const ORIGIN: Vector2 = Vector2(64, 70)
const CELL: float = 32.0
const CELLS: Array[Vector2i] = [Vector2i(9, 9), Vector2i(10, 9)]
var scene: MainScene
var clock: float = 0.0
var pixels: Dictionary = {}
var measured: Dictionary = {}
var failures: int = 0
var held_keys: Dictionary[Key, bool] = {}

func _initialize() -> void:
	Engine.max_fps = 60
	_run.call_deferred()

func _run() -> void:
	await process_frame
	scene = current_scene as MainScene
	scene.pin_survival_seed(11)
	scene.pin_scene_clock(clock)
	if OS.get_cmdline_user_args().has("--map-art-tour"):
		await _map_art_tour()
		return
	await _tap(KEY_1)
	await _tap(KEY_1)
	await _tap(KEY_E)
	await _tap(KEY_Q)
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
	var enough: bool = measured.size() == 4 and check_floor_lines(scene, pixels)
	print("Stairs: finished: %s; %d/4 direction/cell readings, %d failures." % ["pass" if enough and failures == 0 else "fail", measured.size(), failures])
	quit(0 if enough and failures == 0 else 1)

## Plays both actors through the fixed street into the aquarium and saves native evidence.
func _map_art_tour() -> void:
	await _tap(KEY_1)
	await _tap(KEY_3)
	await _tap(KEY_E)
	await _tap(KEY_Q)
	await _tap(KEY_U)
	if not await _tour_shot("230-street-start"):
		return
	for target: Vector2i in [Vector2i(3, 8), Vector2i(3, 6)]:
		if not await _tour_move(target):
			return
	if not await _tour_shot("230-interior-kitchen"):
		return
	var inside: bool = false
	for layer: Dictionary in scene.get_node("World").get_meta("foreground_layers", []):
		var texture: Texture2D = layer["texture"]
		inside = inside or texture.resource_path.ends_with("world/interior_kitchen.png")
	if not inside:
		_tour_fail("the occupied kitchen did not draw its interior background")
		return
	for target: Vector2i in [Vector2i(3, 8), Vector2i(11, 9), Vector2i(13, 9), Vector2i(16, 9), Vector2i(17, 8), Vector2i(29, 9), Vector2i(31, 9), Vector2i(35, 9), Vector2i(36, 8), Vector2i(49, 8), Vector2i(50, 7), Vector2i(51, 6), Vector2i(52, 5), Vector2i(60, 5), Vector2i(60, 11)]:
		var body: Dictionary = scene.survival_status()["chef"]
		var row: int = body["y"]
		var jump: bool = target.y < row or target.x in [13, 31]
		if not await _tour_move(target, jump):
			return
	if not await _tour_shot("230-street-terminal"):
		return
	if not await _tour_move(Vector2i(62, 11)):
		return
	if scene.survival_status()["map"] != "Aquarium":
		_tour_fail("the paired street exit did not enter the aquarium")
		return
	if not await _tour_shot("230-aquarium-lobby"):
		return
	print("Stairs: finished: pass; fixed street and aquarium floor lines, paired travel and native map evidence.")
	quit(0)

func _tour_move(target: Vector2i, jump: bool = false) -> bool:
	_key(KEY_SPACE, jump)
	_key(KEY_SEMICOLON, jump)
	for frame: int in range(600):
		var state: Dictionary = scene.survival_status()
		var body: Dictionary = state["chef"]
		var column: int = body["x"]
		var row: int = body["y"]
		var at: Vector2i = Vector2i(column, row)
		if state["map"] == "Aquarium" or (at == target and body["stance"] != "airborne"):
			for code: Key in [KEY_A, KEY_D, KEY_W, KEY_S, KEY_SPACE, KEY_SHIFT, KEY_J, KEY_L, KEY_I, KEY_K, KEY_SEMICOLON, KEY_H]:
				_key(code, false)
			print("Stairs: tour reached %s on %s; chef %s, appraiser (%s,%s)." % [target, state["map"], at, state["appraiser"]["x"], state["appraiser"]["y"]])
			return true
		if state["phase"] != "exploration":
			_tour_fail("the day ended before reaching %s; last cell %s" % [target, at])
			return false
		var sideways: bool = target.x != at.x
		_key(KEY_A, sideways and target.x < at.x)
		_key(KEY_J, sideways and target.x < at.x)
		_key(KEY_D, sideways and target.x > at.x)
		_key(KEY_L, sideways and target.x > at.x)
		_key(KEY_W, not sideways and target.y < at.y)
		_key(KEY_I, not sideways and target.y < at.y)
		_key(KEY_S, not sideways and target.y > at.y)
		_key(KEY_K, not sideways and target.y > at.y)
		_key(KEY_SHIFT, absi(target.x - at.x) > 3)
		_key(KEY_H, absi(target.x - at.x) > 3)
		if frame == 3:
			_key(KEY_SPACE, false)
			_key(KEY_SEMICOLON, false)
		clock += 0.1
		scene.pin_scene_clock(clock)
		await process_frame
	_tour_fail("did not reach %s; status %s" % [target, scene.survival_status()])
	return false

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
	var world: Node = subject.get_node("World")
	var floors: Array = world.get_meta("collision_floors", [])
	var layers: Array = world.get_meta("deck_layers", [])
	var cells: int = 0
	var controls: int = 0
	var voids: int = 0
	for floor_record: Dictionary in floors:
		var area: Rect2 = floor_record["area"]
		var floor_y: float = area.end.y
		var standing: bool = floor_record["standing"]
		for column: int in range(roundi(area.position.x), roundi(area.end.x), 32):
			if not standing:
				for offset: float in [0.5, 16.0, 31.5]:
					var view: Transform2D = subject.get_meta("map_view", Transform2D())
					if _stone_layers(view * Vector2(column + offset, floor_y + 0.25), layers, cache):
						push_error("Stairs: absent terrain %s still draws a floor at x=%s." % [floor_record["name"], column + offset])
						return false
				voids += 1
				continue
			var top_layers: Array = []
			for layer: Dictionary in layers:
				var destination: Rect2 = layer["destination"]
				if is_equal_approx(destination.position.y, floor_y) and destination.position.x < column + CELL and destination.end.x > column:
					top_layers.append(layer)
			if top_layers.is_empty():
				push_error("Stairs: no deck begins at collision floor %s x=%d." % [floor_record["name"], column])
				return false
			for offset: float in [0.5, 16.0, 31.5]:
				var view: Transform2D = subject.get_meta("map_view", Transform2D())
				var hit: Vector2 = view * Vector2(column + offset, floor_y + 0.25)
				var miss: Vector2 = view * Vector2(column + offset, floor_y - 0.25)
				if not _stone_layers(hit, top_layers, cache) or _stone_layers(miss, top_layers, cache):
					push_error("Stairs: floor %s x=%s top=%s lost contact or confused the nearest row above it." % [floor_record["name"], column + offset, floor_y])
					return false
				controls += 1
			cells += 1
	print("Stairs: %s day %s: %d terrain cells have opaque contact at x=0.5,16,31.5; %d nearest-row-above controls returned nothing." % [subject.survival_status()["map"], subject.survival_status()["day"], cells, controls])
	print("Stairs: %d dormant or destroyed terrain cells correctly draw no collision deck." % voids)
	return cells > 0

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
