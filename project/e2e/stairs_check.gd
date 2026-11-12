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

func _initialize() -> void:
	_run.call_deferred()

func _run() -> void:
	await process_frame
	scene = current_scene as MainScene
	scene.pin_survival_seed(11)
	scene.pin_scene_clock(clock)
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
	var enough: bool = measured.size() == 4
	print("Stairs: finished: %s; %d/4 direction/cell readings, %d failures." % ["pass" if enough and failures == 0 else "fail", measured.size(), failures])
	quit(0 if enough and failures == 0 else 1)

## Reads alpha through the actual texture, crop and draw transform, excluding all scenery and actors.
func _stone(point: Vector2) -> bool:
	for layer: Dictionary in scene.get_node("World").get_meta("deck_layers", []):
		var transform: Transform2D = layer["transform"]
		var destination: Rect2 = layer["destination"]
		var local: Vector2 = transform.affine_inverse() * point
		if not destination.has_point(local):
			continue
		var texture: Texture2D = layer["texture"]
		var id: int = texture.get_instance_id()
		if not pixels.has(id):
			pixels[id] = texture.get_image()
		var source: Rect2 = layer["source"]
		var at: Vector2i = Vector2i((source.position + (local - destination.position) / destination.size * source.size).floor())
		var image: Image = pixels[id]
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
	var event: InputEventKey = InputEventKey.new()
	event.physical_keycode = code
	event.pressed = pressed
	Input.parse_input_event(event)
