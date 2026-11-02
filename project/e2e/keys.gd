class_name KeyCoverageCheck
extends SceneTree
## Presses each survival key the README gives that no other harness presses, as a
## real key event, and asserts the action's result. KEYS is the one table of those
## keys: a key change edits one row here and its binding in project.godot.

const SEED: int = 11
const DAY_FRAMES: int = 2900
## The shared clock one collection spends.
const COLLECT_MS: int = 12000
const KIOSK_DAYS: int = 10
const APPRAISE_ATTEMPTS: int = 12
const COOK_ATTEMPTS: int = 6
## The acid drip stands on the ramp's first step at 10, 7, past the rubble; up
## and back down through it burns the walker twice, and the third burn is one
## step back down.
const BURN_WAYPOINTS: Array[Vector2i] = [Vector2i(10, 7), Vector2i(11, 6), Vector2i(10, 7), Vector2i(11, 6)]
## Up the north ladder, through the fallen kiosk and down to the neon ledge.
const ARCADE_ROUTE: Array[Vector2i] = [Vector2i(2, 8), Vector2i(2, 2), Vector2i(10, 2), Vector2i(13, 3)]
## The foot of the north ladder on the dock, beside the entrance.
const LADDER_FOOT: Vector2i = Vector2i(2, 8)

## Action name to the key the README names for it.
const KEYS: Dictionary[String, Key] = {
	"survival_1_left": KEY_LEFT,
	"survival_1_up": KEY_UP,
	"survival_1_down": KEY_DOWN,
	"survival_2_left": KEY_J,
	"survival_2_up": KEY_I,
	"survival_2_down": KEY_K,
	"survival_2_run": KEY_H,
	"survival_2_interact": KEY_O,
	"survival_2_replace_tool": KEY_P,
	"survival_2_feed": KEY_BRACKETLEFT,
	"survival_1_jump": KEY_SPACE,
	"survival_2_jump": KEY_SEMICOLON,
}

var scene: MainScene
var held: Dictionary[Key, bool] = {}
var checks: int = 0

func _initialize() -> void:
	_run.call_deferred()

func _run() -> void:
	await _frames(2)
	scene = current_scene as MainScene
	if not _check(scene != null and scene.screen_name() == "main", "configured startup main menu"):
		return
	scene.pin_survival_seed(SEED)
	if not await _one_player():
		return
	if not await _two_players():
		return
	print("Keys: ", checks, " live checks passed (survival player one, survival player two).")
	quit(0)

func _one_player() -> bool:
	await _tap(KEY_1)
	await _tap(KEY_1)
	await _tap(KEY_E)
	if not _check(scene.screen_name() == "survival" and _actor("chef", "key_set") == 0, "choosing the chef starts a run on key set one"):
		return false
	await _tap(KEY_Q)
	if not await _until("chef sets out", func() -> bool: return _where("chef") == "city", 30):
		return false
	var keys: Dictionary[String, Key] = _walk_keys(1)
	if not await _walk_to("chef", keys, LADDER_FOOT.x, LADDER_FOOT.y):
		return false
	var x: int = _actor("chef", "x")
	if not await _held_result("survival_1_left", func() -> bool: return _actor("chef", "x") < x, 30, "%s walks player one left" % _name("survival_1_left")):
		return false
	if not await _walk_to("chef", keys, LADDER_FOOT.x, LADDER_FOOT.y):
		return false
	if not await _held_result("survival_1_up", func() -> bool: return _actor("chef", "y") < LADDER_FOOT.y - 1, 60, "%s climbs player one up" % _name("survival_1_up")):
		return false
	if not await _held_result("survival_1_down", func() -> bool: return _actor("chef", "y") == LADDER_FOOT.y, 90, "%s climbs player one down" % _name("survival_1_down")):
		return false
	if not await _held_result("survival_1_jump", func() -> bool: return _stance("chef") == "airborne", 30, "%s jumps player one" % _name("survival_1_jump")):
		return false
	if not await _until("player one lands", func() -> bool: return _stance("chef") == "standing", 60):
		return false
	await _tap(KEY_M)
	await _tap(KEY_ENTER)
	return _check(scene.screen_name() == "main", "M then Enter ends the run")

## Two humans: player two plays the appraiser on the second key set.
func _two_players() -> bool:
	await _tap(KEY_1)
	await _tap(KEY_3)
	await _tap(KEY_E)
	if not _check(_actor("chef", "key_set") == 0 and _actor("appraiser", "key_set") == 1, "two players take one key set each"):
		return false
	var keys: Dictionary[String, Key] = _walk_keys(2)
	await _tap(KEY_U)
	if not await _until("player two sets out alone", func() -> bool: return _where("appraiser") == "city", 30):
		return false
	if not await _walk_to("appraiser", keys, LADDER_FOOT.x, LADDER_FOOT.y):
		return false
	var x: int = _actor("appraiser", "x")
	if not await _held_result("survival_2_left", func() -> bool: return _actor("appraiser", "x") < x, 30, "%s walks player two left" % _name("survival_2_left")):
		return false
	if not await _walk_to("appraiser", keys, LADDER_FOOT.x, LADDER_FOOT.y):
		return false
	if not await _held_result("survival_2_up", func() -> bool: return _actor("appraiser", "y") < LADDER_FOOT.y - 1, 60, "%s climbs player two up" % _name("survival_2_up")):
		return false
	if not await _held_result("survival_2_down", func() -> bool: return _actor("appraiser", "y") == LADDER_FOOT.y, 90, "%s climbs player two down" % _name("survival_2_down")):
		return false
	if not await _held_result("survival_2_jump", func() -> bool: return _stance("appraiser") == "airborne", 30, "%s jumps player two" % _name("survival_2_jump")):
		return false
	if not await _until("player two lands", func() -> bool: return _stance("appraiser") == "standing", 60):
		return false
	if not await _walk_to("appraiser", keys, 4, 8):
		return false
	var stamina: int = _actor("appraiser", "stamina")
	_key(KEYS["survival_2_run"], true)
	_key(keys["right"], true)
	var ran: bool = await _until("%s runs player two" % _name("survival_2_run"), func() -> bool: return _actor("appraiser", "stamina") < stamina, 60)
	_release()
	if not ran:
		return false
	if not await _walk_to("appraiser", keys, 7, 8):
		return false
	var remaining: int = _int("remaining_ms")
	await _tap(KEYS["survival_2_interact"])
	if not _check(_actor("appraiser", "ingredients") == 1 and remaining - _int("remaining_ms") >= COLLECT_MS, "%s collects for player two and spends the clock" % _name("survival_2_interact")):
		return false
	if not await _walk_route("appraiser", keys, [Vector2i(7, 11), Vector2i(6, 11)]):
		return false
	await _tap(KEYS["survival_2_interact"])
	if not _check(_actor("appraiser", "ingredients") == 2, "%s collects a second ingredient" % _name("survival_2_interact")):
		return false
	await _tap(KEY_U)
	if not await _until("player two returns", func() -> bool: return _str("phase") == "workshop", 60):
		return false
	var attempts: int = 0
	while _int("identified") < 2 and attempts < APPRAISE_ATTEMPTS:
		attempts += 1
		await _tap(KEYS["survival_2_interact"])
	if not _check(_int("identified") == 2, "%s appraises for player two" % _name("survival_2_interact")):
		return false
	attempts = 0
	while _int("foods") == 0 and attempts < COOK_ATTEMPTS:
		attempts += 1
		await _tap(KEY_E)
	if not _check(_int("foods") == 1, "player one cooks the dish player two found"):
		return false
	await _tap(KEYS["survival_2_feed"])
	if not _check(await _until("%s feeds the stored dish" % _name("survival_2_feed"), func() -> bool: return _int("day") == 2, 30) and _int("eaten") == 1, "%s feeds a dish to the monster" % _name("survival_2_feed")):
		return false
	return await _replace_the_magnifier(keys)

## Player two burns out on the acid drip, finds a magnifier in the arcade, and
## swaps it in. The arcade bag stands behind the kiosk wreck, which falls only
## after the run has explored for KIOSK_SECONDS.
func _replace_the_magnifier(keys: Dictionary[String, Key]) -> bool:
	if not await _wait_for_the_kiosk_to_fall():
		return false
	await _tap(KEY_U)
	if not await _until("player two sets out on day two", func() -> bool: return _where("appraiser") == "city", 30):
		return false
	if not _check(_str("magnifier") == "intact", "the magnifier starts intact"):
		return false
	if not await _walk_route("appraiser", keys, BURN_WAYPOINTS):
		return false
	_key(keys["left"], true)
	var exhausted: bool = await _until("the third drip", func() -> bool: return _actor("appraiser", "exhaustions") == 1, 60)
	_key(keys["left"], false)
	if not exhausted:
		return false
	if not _check(_where("appraiser") == "base" and _str("magnifier") == "damaged", "exhaustion sends player two home and damages the magnifier"):
		return false
	await _tap(KEY_U)
	if not await _until("player two sets out again", func() -> bool: return _where("appraiser") == "city", 30):
		return false
	if not await _walk_route("appraiser", keys, ARCADE_ROUTE):
		return false
	await _tap(KEYS["survival_2_interact"])
	if not _check(_actor("appraiser", "tools") == 1, "%s collects the magnifier from the arcade" % _name("survival_2_interact")):
		return false
	await _tap(KEY_U)
	if not await _until("player two is home with the magnifier", func() -> bool: return _str("phase") == "workshop", 120):
		return false
	await _tap(KEYS["survival_2_replace_tool"])
	return _check(_str("magnifier") == "intact" and _actor("appraiser", "tools") == 0, "%s spends the found tool on the damaged magnifier" % _name("survival_2_replace_tool"))

## Lets whole days run out unfed, with player two out for each, until the kiosk
## that blocks the arcade has fallen.
func _wait_for_the_kiosk_to_fall() -> bool:
	var days: int = 0
	while _object("Old Kiosk") != "destroyed" and days < KIOSK_DAYS:
		days += 1
		await _tap(KEY_U)
		if not await _until("player two spends the day out", func() -> bool: return _str("phase") == "workshop" and _int("remaining_ms") == 0, DAY_FRAMES):
			return false
		var day: int = _int("day")
		await _tap(KEY_G)
		if not await _until("the day ends unfed", func() -> bool: return _int("day") == day + 1, 30):
			return false
	return _check(_object("Old Kiosk") == "destroyed" and _str("phase") == "workshop", "exploring time brings the kiosk down")

## Names the action with its key, so a failure says which row broke.
func _name(action: String) -> String:
	return "%s (%s)" % [OS.get_keycode_string(KEYS[action]), action]

## The walking keys of key set `player`; its left, up, down and jump come from
## KEYS.
func _walk_keys(player: int) -> Dictionary[String, Key]:
	var right: Key = KEY_D if player == 1 else KEY_L
	return {
		"left": KEYS["survival_%d_left" % player],
		"right": right,
		"up": KEYS["survival_%d_up" % player],
		"down": KEYS["survival_%d_down" % player],
		"jump": KEYS["survival_%d_jump" % player],
	}

func _stance(who: String) -> String:
	var actor: Dictionary = scene.survival_status()[who]
	var stance: String = actor["stance"]
	return stance

func _object(object: String) -> String:
	var objects: Dictionary = scene.survival_status()["objects"]
	var state: String = objects[object]
	return state

func _str(field: String) -> String:
	var value: String = scene.survival_status().get(field, "")
	return value

func _int(field: String) -> int:
	var value: int = scene.survival_status().get(field, 0)
	return value

func _where(who: String) -> String:
	var actor: Dictionary = scene.survival_status()[who]
	var location: String = actor["location"]
	return location

func _actor(who: String, field: String) -> int:
	var actor: Dictionary = scene.survival_status()[who]
	var value: int = actor[field]
	return value

func _walk_route(who: String, keys: Dictionary[String, Key], waypoints: Array) -> bool:
	for waypoint: Vector2i in waypoints:
		if not await _walk_to(who, keys, waypoint.x, waypoint.y):
			return false
	return true

## Walks `who` sideways along its floor, stairs and steps included, to column
## x, jumping whatever low block stops it, then climbs to row y.
func _walk_to(who: String, keys: Dictionary[String, Key], x: int, y: int) -> bool:
	var stalled: int = 0
	var last: int = _actor(who, "sub_x")
	for frame: int in range(480):
		var at_x: int = _actor(who, "x")
		var at_y: int = _actor(who, "y")
		if at_x == x and at_y == y and _stance(who) != "airborne":
			_release()
			return true
		var sideways: bool = at_x != x
		_key(keys["left"], sideways and x < at_x)
		_key(keys["right"], sideways and x > at_x)
		_key(keys["up"], not sideways and y < at_y)
		_key(keys["down"], not sideways and y > at_y)
		var sub_x: int = _actor(who, "sub_x")
		stalled = stalled + 1 if sideways and sub_x == last and _stance(who) == "standing" else 0
		last = sub_x
		_key(keys["jump"], stalled > 12)
		await process_frame
	_release()
	return _check(false, "%s walks to %d, %d" % [who, x, y])

## Holds `code` until `condition` holds or `limit` frames pass, then releases it.
func _hold_until(code: Key, condition: Callable, limit: int) -> bool:
	_key(code, true)
	var reached: bool = false
	for frame: int in range(limit):
		var done: bool = condition.call()
		if done:
			reached = true
			break
		await process_frame
	_key(code, false)
	return reached

## Holds the key of `action` until `condition` holds, as the result it asserts.
func _held_result(action: String, condition: Callable, limit: int, name: String) -> bool:
	return _check(await _hold_until(KEYS[action], condition, limit), name)

func _until(name: String, condition: Callable, limit: int) -> bool:
	for frame: int in range(limit):
		var done: bool = condition.call()
		if done:
			return true
		await process_frame
	return _check(false, name)

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

func _release() -> void:
	for code: Key in held.keys():
		_key(code, false)

func _check(condition: bool, name: String) -> bool:
	if condition:
		checks += 1
		return true
	push_error("Key check failed: ", name, " ", scene.survival_status() if scene != null else {})
	quit(1)
	return false
