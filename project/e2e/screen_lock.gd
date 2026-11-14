class_name SurvivalScreenLockCheck
extends SceneTree
## Plays two humans through the shipped composition with real input actions and
## checks the screen lock: a lone actor walking on stops at the range and the
## map does not change; a jump still lifts an actor stopped there; both actors
## walking to the street's exit zone change the map together, through a fade
## that darkens the screen and changes the drawn map behind it, and back through
## the aquarium's exit zone to the street; an actor walking
## into a building darkens the outside and out again restores it.

const SEED: int = 3
## No AI acts in this party, so only these checks move the actors.
const TWO_PLAYERS: int = 3
## The screen lock in sub-cells, and one running movement unit of it.
const SCREEN_LOCK: int = 384
const RUN_UNIT: int = 50
## One walking physics tick, in sub-cells.
const WALK_TICK: int = 5
## How near the lock a walker stops pressing on, so its partner can come up.
const LOCK_WAIT: int = 320
## The street's exit zone and the map beyond it.
const FAR_MAP: String = "Aquarium"
const HOME_MAP: String = "Street"
## A cell inside the aquarium's exit zone, which the arrival cell (4,10) lies outside.
const AQUARIUM_EXIT: Array[Vector2i] = [Vector2i(1, 10)]
## The way along the street from the entrance to the exit zone, by cells.
const ROUTE: Array[Vector2i] = [
	Vector2i(8, 8), Vector2i(13, 9), Vector2i(16, 9), Vector2i(19, 8),
	Vector2i(28, 8), Vector2i(31, 9), Vector2i(35, 9), Vector2i(49, 8),
	Vector2i(60, 5), Vector2i(60, 8), Vector2i(63, 11),
]
const NAMES: Array[String] = ["chef", "appraiser"]
## The kitchen shop's floor in cells: its door, and a cell inside it.
const KITCHEN_DOOR: Array[Vector2i] = [Vector2i(3, 8)]
const KITCHEN_FLOOR: Array[Vector2i] = [Vector2i(3, 8), Vector2i(3, 6)]
## The darkest the outside goes in a building, and the least counted as dark.
const DARK_OUTSIDE: float = 0.5
## How long the drawn veil is awaited, in real milliseconds, since it eases on
## the real clock.
const VEIL_LIMIT_MSEC: int = 3000
## How many frames a veil that is not to appear is watched for; the day's
## time is short, and the veil takes a fraction of a second to show.
const VEIL_HOLD_FRAMES: int = 10
## The frames a map fade is watched for: it lasts 0.6 seconds of frame time (36
## frames at 60 a second), and the day has no time to spare.
const FADE_FRAMES: int = 40

var scene: MainScene
var checks: int = 0
var failed: bool = false

func _initialize() -> void:
	_run.call_deferred()

func _run() -> void:
	await _frames(2)
	scene = current_scene as MainScene
	scene.pin_survival_seed(SEED)
	await _start()
	if failed:
		return
	scene.get_node("World").set_meta("measure_art", true)
	scene.set_meta("measure_art", true)
	await _building_entry_darkens_the_outside()
	if failed:
		return
	await _lone_actor_stops_at_the_range()
	if failed:
		return
	await _jump_lifts_a_stopped_actor()
	if failed:
		return
	await _both_actors_change_the_map()
	if failed:
		return
	print("Screen lock: ", checks, " live checks passed (building entry, lone actor, jump, joint travel and fade there and back).")
	quit(0)

func _start() -> void:
	await _tap("option_1")
	await _frames(4)
	await _tap("option_" + str(TWO_PLAYERS))
	await _frames(4)
	await _tap("survival_1_interact")
	await _tap("survival_1_travel")
	await _tap("survival_2_travel")
	await _frames(10)
	_check(_where("chef") == "city" and _where("appraiser") == "city", "both players set out into the city")

## Both walk up into the kitchen shop: the outside darkens while the interior
## background is drawn; both walk out and it clears.
func _building_entry_darkens_the_outside() -> void:
	await _frames(2)
	_check(_veil() == 0.0, "the outside is not dark before any building is entered: %f" % _veil())
	# One actor in the shop while the other stays out leaves the street lit.
	if not await _drive([KITCHEN_FLOOR, []], 1200):
		return
	await _frames(VEIL_HOLD_FRAMES)
	_check(_veil() < 0.01, "the street stays lit while one actor is in the shop and the other is out: %f" % _veil())
	if not await _drive([KITCHEN_FLOOR, KITCHEN_FLOOR], 1200):
		return
	await _veil_settles(true)
	_check(_veil() >= DARK_OUTSIDE, "the outside is dark with both actors in the kitchen shop: %f" % _veil())
	_check(_draws_interior("interior_kitchen.png"), "the occupied kitchen draws its interior background")
	if not await _drive([KITCHEN_DOOR, KITCHEN_DOOR], 1200):
		return
	await _veil_settles(false)
	_check(_veil() < 0.01, "the outside is clear again after both walk out: %f" % _veil())
	_check(not _draws_interior("interior_kitchen.png"), "the kitchen's interior background is gone after both walk out")

## Waits, in real time, for the veil to ease to dark or to clear.
func _veil_settles(dark: bool) -> void:
	var until: int = Time.get_ticks_msec() + VEIL_LIMIT_MSEC
	while Time.get_ticks_msec() < until:
		await process_frame
		if (_veil() >= DARK_OUTSIDE) == dark and (dark or _veil() < 0.01):
			return

func _veil() -> float:
	var level: float = scene.get_node("World").get_meta("interior_veil", -1.0)
	return level

func _draws_interior(file: String) -> bool:
	for layer: Dictionary in scene.get_node("World").get_meta("foreground_layers", []):
		var texture: Texture2D = layer["texture"]
		if texture.resource_path.ends_with("world/" + file):
			return true
	return false

## The appraiser waits on the lower walkway while the chef goes on to the east
## walkway and runs east along it. The chef is stopped within one running unit
## of the lock, on level floor and still on the street.
func _lone_actor_stops_at_the_range() -> void:
	if not await _drive([ROUTE.slice(0, 4) + [Vector2i(21, 8)], ROUTE.slice(0, 2)], 1200):
		return
	Input.action_press("survival_1_right")
	Input.action_press("survival_1_run")
	var widest: int = 0
	for frame: int in range(150):
		await process_frame
		widest = maxi(widest, _gap())
	Input.action_release("survival_1_right")
	Input.action_release("survival_1_run")
	var gap: int = _gap()
	_check(widest <= SCREEN_LOCK, "the gap never passes the lock: widest %d of %d" % [widest, SCREEN_LOCK])
	_check(gap > SCREEN_LOCK - RUN_UNIT and gap <= SCREEN_LOCK, "the chef is stopped at the lock, not by the terrain: gap %d" % gap)
	_check(_actor("chef", "x") < 28 and _stance("chef") == "standing", "the chef stands on the east walkway short of its end: cell %d" % _actor("chef", "x"))
	_check(_str("map") == HOME_MAP and _where("chef") == "city", "the map does not change while one actor walks on: %s" % _str("map"))

## At the lock, a jump lifts the chef and it lands, with the gap held.
func _jump_lifts_a_stopped_actor() -> void:
	Input.action_press("survival_1_right")
	await _frames(30)
	Input.action_release("survival_1_right")
	await _frames(6)
	_check(_gap() > SCREEN_LOCK - WALK_TICK and _gap() <= SCREEN_LOCK, "the chef stands hard against the lock, so no step is left to walk: gap %d" % _gap())
	var floor_y: int = _actor("chef", "sub_y")
	var highest: int = floor_y
	var widest: int = 0
	Input.action_press("survival_1_right")
	for frame: int in range(90):
		if frame == 6:
			Input.action_press("survival_1_jump")
		if frame == 8:
			Input.action_release("survival_1_jump")
		await process_frame
		highest = mini(highest, _actor("chef", "sub_y"))
		widest = maxi(widest, _gap())
	Input.action_release("survival_1_right")
	_check(highest <= floor_y - 16, "the jump lifts the stopped chef: from %d up to %d" % [floor_y, highest])
	_check(_stance("chef") == "standing" and _actor("chef", "sub_y") == floor_y, "the chef lands where it jumped from: from %d to %d, %s" % [floor_y, _actor("chef", "sub_y"), _stance("chef")])
	_check(widest <= SCREEN_LOCK, "the gap holds through the jump: widest %d" % widest)

## Both walk the street to its exit zone, the one ahead waiting at the lock, and
## the map changes once both stand in the zone.
func _both_actors_change_the_map() -> void:
	var street_size: Vector2 = _map_size()
	var moved: bool = await _drive([ROUTE.slice(4), ROUTE.slice(2)], 2400, true)
	if failed:
		return
	_check(moved, "both actors walking to the exit zone change the map")
	await _fade_through(street_size)
	_check(_str("map") == FAR_MAP and _where("chef") == "city" and _where("appraiser") == "city", "both arrive on the far map")
	_check(_actor("chef", "x") == _actor("appraiser", "x") and _gap() <= SCREEN_LOCK, "both stand at the arrival cell together")
	# And back: both walk into the aquarium's exit zone and the street returns.
	var aquarium_size: Vector2 = _map_size()
	_check(aquarium_size != street_size, "the aquarium is drawn before the way back")
	moved = await _drive([AQUARIUM_EXIT, AQUARIUM_EXIT], 600, true, FAR_MAP)
	if failed:
		return
	_check(moved, "both actors walking to the aquarium's exit zone change the map")
	await _fade_through(aquarium_size)
	_check(_str("map") == HOME_MAP and _where("chef") == "city" and _where("appraiser") == "city", "both arrive back on the street")

## Watches the frames after the rules changed the map. The screen keeps drawing
## the map left (of size `left_size`) while it darkens, draws the new one only
## once it is black, and clears afterwards.
func _fade_through(left_size: Vector2) -> void:
	var darkest: float = 0.0
	var left_after_dark: bool = false
	var new_before_dark: bool = false
	var new_drawn: bool = false
	for frame: int in range(FADE_FRAMES):
		await process_frame
		var veil: float = _screen_veil()
		darkest = maxf(darkest, veil)
		var size: Vector2 = _map_size()
		if size != left_size:
			new_drawn = true
			new_before_dark = new_before_dark or darkest < 0.9
		elif darkest >= 0.99:
			left_after_dark = true
	_check(darkest >= 0.99, "the screen goes black between the maps: darkest %f" % darkest)
	_check(new_drawn, "the new map is drawn after the fade")
	_check(not new_before_dark, "the new map is not drawn before the screen is black")
	_check(not left_after_dark, "the map left is not drawn once the screen has been black")
	_check(_screen_veil() < 0.01, "the screen is clear again after the fade: %f" % _screen_veil())

func _map_size() -> Vector2:
	var size: Vector2 = scene.get_node("World").get_meta("map_size", Vector2.ZERO)
	return size

func _screen_veil() -> float:
	var veil: Color = scene.get_meta("screen_veil", Color(0, 0, 0, 0))
	return veil.a

## Walks both players along their routes by cells, the one ahead waiting where
## the lock would stop it. Ends when both have finished, or, with `until_moved`,
## when the map changes from `from`; true when it did.
func _drive(routes: Array, limit: int, until_moved: bool = false, from: String = HOME_MAP) -> bool:
	var targets: Array[int] = [0, 0]
	var stalled: Array[int] = [0, 0]
	var last: Array[int] = [_actor("chef", "sub_x"), _actor("appraiser", "sub_x")]
	for frame: int in range(limit):
		if until_moved and _str("map") != from:
			_release()
			return true
		var finished: bool = true
		for who: int in range(2):
			var route: Array = routes[who]
			finished = finished and targets[who] >= route.size()
			_steer(who, route, targets, stalled, last)
		if finished and not until_moved:
			_release()
			return true
		await process_frame
	_release()
	if not until_moved:
		_check(false, "both players walk their routes")
	return false

## Presses one walker's keys toward its next waypoint; it holds still while the
## one ahead of it has run to the lock.
func _steer(who: int, route: Array, targets: Array[int], stalled: Array[int], last: Array[int]) -> void:
	var name: String = NAMES[who]
	var prefix: String = "survival_%d_" % (who + 1)
	if targets[who] >= route.size():
		_hold(prefix, false, false, false, false, false, false)
		return
	var goal: Vector2i = route[targets[who]]
	var at_x: int = _actor(name, "x")
	var at_y: int = _actor(name, "y")
	if at_x == goal.x and at_y == goal.y and _stance(name) != "airborne":
		targets[who] += 1
		return
	var sideways: bool = at_x != goal.x
	var direction: int = signi(goal.x - at_x)
	var ahead: bool = sideways and signi(_actor(name, "sub_x") - _actor(NAMES[1 - who], "sub_x")) == direction
	if ahead and _gap() >= LOCK_WAIT:
		_hold(prefix, false, false, false, false, false, false)
		stalled[who] = 0
		return
	var sub_x: int = _actor(name, "sub_x")
	stalled[who] = stalled[who] + 1 if sideways and sub_x == last[who] and _stance(name) == "standing" else 0
	last[who] = sub_x
	_hold(prefix, sideways and goal.x < at_x, sideways and goal.x > at_x, not sideways and goal.y < at_y, not sideways and goal.y > at_y, absi(goal.x - at_x) > 2, stalled[who] > 6)

func _hold(prefix: String, left: bool, right: bool, up: bool, down: bool, run: bool, jump: bool) -> void:
	_action(prefix + "left", left)
	_action(prefix + "right", right)
	_action(prefix + "up", up)
	_action(prefix + "down", down)
	_action(prefix + "run", run)
	_action(prefix + "jump", jump)

func _action(name: String, pressed: bool) -> void:
	if pressed and not Input.is_action_pressed(name):
		Input.action_press(name)
	elif not pressed and Input.is_action_pressed(name):
		Input.action_release(name)

func _release() -> void:
	for prefix: String in ["survival_1_", "survival_2_"]:
		_hold(prefix, false, false, false, false, false, false)

func _gap() -> int:
	return absi(_actor("chef", "sub_x") - _actor("appraiser", "sub_x"))

func _tap(action: String) -> void:
	Input.action_press(action)
	await _frames(1)
	Input.action_release(action)
	await _frames(1)

func _where(who: String) -> String:
	var actor: Dictionary = scene.survival_status()[who]
	var location: String = actor["location"]
	return location

func _stance(who: String) -> String:
	var actor: Dictionary = scene.survival_status()[who]
	var stance: String = actor["stance"]
	return stance

func _actor(who: String, field: String) -> int:
	var actor: Dictionary = scene.survival_status()[who]
	var value: int = actor[field]
	return value

func _str(field: String) -> String:
	var value: String = scene.survival_status().get(field, "")
	return value

func _frames(count: int) -> void:
	for frame: int in range(count):
		await process_frame

func _check(condition: bool, name: String) -> bool:
	if condition:
		checks += 1
		return true
	if not failed:
		push_error("Screen lock check failed: ", name, " ", scene.survival_status())
		failed = true
		quit(1)
	return false
