class_name SurvivalPlayCheck
extends SceneTree
## Plays the survival game through the shipped scene with real key events: full
## days as the chef, a day as the appraiser, and the map and tool changes between.

const SEED: int = 11
const DAY_FRAMES: int = 2900

var scene: AscentScene
var held: Dictionary[Key, bool] = {}
var checks: int = 0
var failed: bool = false
var walker: String = "chef"

func _initialize() -> void:
	_run.call_deferred()

func _run() -> void:
	await _frames(2)
	scene = current_scene as AscentScene
	if not _check(scene != null and scene.screen_name() == "main", "configured startup main menu"):
		return
	scene.pin_survival_seed(SEED)
	await _menus()
	if not failed:
		await _first_day()
	if not failed:
		await _workshop_day_one()
	if not failed:
		await _second_day()
	if not failed:
		await _acid_changes_the_city()
	if not failed:
		await _chef_chooses_a_dish()
	if not failed:
		await _appraiser_run()
	if not failed:
		await _two_players()
	if failed:
		return
	print("Survival: ", checks, " live checks passed (menus, days, return, appraisal, cooking, dish choice, feeding, map, tools, roles).")
	quit(0)

func _menus() -> void:
	await _tap(KEY_1)
	if not _check(scene.screen_name() == "survival_setup", "1 opens the choice of players"):
		return
	await _tap(KEY_M)
	if not _check(scene.screen_name() == "main", "M leaves the choice of players"):
		return
	await _tap(KEY_1)
	await _tap(KEY_1)
	if not _check(scene.screen_name() == "survival", "choosing the chef starts a run"):
		return
	await _tap(KEY_E)
	if not _check(_actor("chef", "key_set") == 0 and _actor("appraiser", "key_set") == -1, "the chosen actor is human and the other is AI"):
		return
	if not _check(_int("day") == 1 and _str("phase") == "workshop", "the run opens on day one at base"):
		return
	if not _check(_next().contains("Set out"), "the screen tells a new player to set out"):
		return

func _first_day() -> void:
	var start: Dictionary = scene.survival_status()
	await _frames(30)
	var budget: int = start["remaining_ms"]
	if not _check(_int("remaining_ms") == budget, "exploration time stands still at base"):
		return
	await _tap(KEY_Q)
	if not await _until("chef sets out", func() -> bool: return _where("chef") == "city", 30):
		return
	if not _check(_int("remaining_ms") < budget and _str("phase") == "exploration", "setting out starts the clock"):
		return
	if not await _until("AI follows into the city", func() -> bool: return _where("appraiser") == "city", 30):
		return
	if not await _walk_to(6, 7):
		return
	if not _check(_next().contains("Tomato Machine"), "the screen names the source in reach"):
		return
	var stock: int = _stock("Tomato Machine")
	await _tap(KEY_E)
	if not _check(_actor("chef", "ingredients") == 1 and _stock("Tomato Machine") < stock, "E collects into the pack and spends shared stock"):
		return
	await _run_east()
	if failed:
		return
	await _burn_and_fall()
	if failed:
		return
	await _exhaust_the_chef()
	if failed:
		return
	await _tap(KEY_Q)
	if not await _until("chef sets out again", func() -> bool: return _where("chef") == "city", 30):
		return
	var remaining: int = _int("remaining_ms")
	if not await _until("time expires", func() -> bool: return _str("phase") == "workshop", DAY_FRAMES):
		return
	if not _check(remaining > 0 and _int("remaining_ms") == 0, "the day's clock ran out in the city"):
		return
	if not _check(_where("chef") == "base" and _where("appraiser") == "base", "expiry returns both actors"):
		return
	_check(_events().has("Time is up: everyone is back at base."), "the screen reports the timed return")

func _run_east() -> void:
	var stamina: int = _actor("chef", "stamina")
	_key(KEY_SHIFT, true)
	_key(KEY_D, true)
	await _until("running reaches the drip's edge", func() -> bool: return _actor("chef", "x") >= 14, 240)
	_key(KEY_D, false)
	_key(KEY_SHIFT, false)
	if failed:
		return
	if not _check(_actor("chef", "stamina") < stamina, "running spends stamina"):
		return
	if not await _walk_to(17, 7):
		return
	_check(_next().contains("gold-framed"), "the screen points at stock the chef can walk to")

func _burn_and_fall() -> void:
	if not await _walk_to(19, 7):
		return
	if not _check(_actor("chef", "health") == 2 and _next_event_has("burned"), "the acid drip costs one health slot"):
		return
	await _walk_to(20, 7)
	var remaining: int = _int("remaining_ms")
	var pack: int = _actor("chef", "ingredients")
	_key(KEY_W, true)
	var fell: bool = await _until("chef reaches the edge", func() -> bool: return _where("chef") == "base", 60)
	_key(KEY_W, false)
	if not fell:
		return
	if not _check(_actor("chef", "health") == 3 and _actor("chef", "ingredients") == pack, "a fall returns to base with health restored and the pack kept"):
		return
	if not _check(_int("remaining_ms") > 0 and _int("remaining_ms") <= remaining, "a fall keeps the remaining time"):
		return
	_check(_next_event_has("fell"), "the screen reports the fall")

func _exhaust_the_chef() -> void:
	if not await _until("AI comes back to base", func() -> bool: return _str("phase") == "workshop", 120):
		return
	var left: int = _int("remaining_ms")
	await _frames(30)
	if not _check(_int("remaining_ms") == left and left > 0, "a return with time left keeps the rest of the day"):
		return
	await _tap(KEY_Q)
	if not await _until("chef re-enters", func() -> bool: return _where("chef") == "city", 30):
		return
	if not _check(_spatula() == "intact", "the spatula starts intact"):
		return
	for waypoint: int in [17, 19, 17]:
		if not await _walk_to(waypoint, 7):
			return
	if not _check(_actor("chef", "health") == 1, "two more drips leave one slot"):
		return
	_key(KEY_D, true)
	var exhausted: bool = await _until("the third drip", func() -> bool: return _actor("chef", "exhaustions") == 1, 60)
	_key(KEY_D, false)
	if not exhausted:
		return
	if not _check(_where("chef") == "base" and _actor("chef", "health") == 3, "exhaustion carries the chef to base at full health"):
		return
	if not _check(_spatula() == "damaged", "exhaustion damages the chef's spatula"):
		return
	await _until("AI comes back after the exhaustion", func() -> bool: return _str("phase") == "workshop", 120)

func _workshop_day_one() -> void:
	if not await _until("AI appraises the finds", func() -> bool: return _int("identified") >= 2 and not _next().contains("identifying"), 300):
		return
	if not _check(_int("remaining_ms") == 0 and _int("day") == 1, "the workshop runs with no clock and stays on the same day"):
		return
	if not _check(_next().contains("Replace the damaged Spatula"), "the screen offers the found replacement"):
		return
	var carried: int = _actor("chef", "tools") + _actor("appraiser", "tools")
	await _tap(KEY_R)
	if not _check(_spatula() == "intact" and _actor("chef", "tools") + _actor("appraiser", "tools") == carried - 1, "R spends a found tool on the damaged spatula"):
		return
	await _cook()
	if failed:
		return
	if not _check(_next().contains("[F] Feed"), "the screen offers feeding the stored dish"):
		return
	await _tap(KEY_F)
	if not await _until("feeding starts the next day", func() -> bool: return _int("day") == 2, 30):
		return
	if not _check(_str("reaction") == "satisfied" and _int("score") >= 60, "a new dish satisfies the monster"):
		return
	if not _check(_int("acid") == 0 and _int("eaten") == 1 and _int("foods") == 0, "a satisfied monster raises no acid and the dish is gone"):
		return
	_check(_int("remaining_ms") == 45000, "the new day restores the exploration budget")

func _cook() -> void:
	var pack: int = _actor("chef", "ingredients") + _actor("appraiser", "ingredients")
	var attempts: int = 0
	while _int("foods") == 0 and attempts < 6:
		attempts += 1
		await _tap(KEY_E)
	if not _check(_int("foods") == 1, "E cooks a dish at base"):
		return
	_check(_actor("chef", "ingredients") + _actor("appraiser", "ingredients") < pack, "cooking spends ingredients")

func _second_day() -> void:
	if not _check(_object("Roof Ladder") == "intact", "the ladder stands before the second day"):
		return
	var spent: int = _stock("Tomato Machine")
	await _tap(KEY_Q)
	if not await _until("chef sets out on day two", func() -> bool: return _where("chef") == "city", 30):
		return
	if not _check(_stock("Tomato Machine") == spent, "a new day does not refill stock"):
		return
	if not await _walk_to(5, 7):
		return
	if not await _until("a washed-up bag appears", func() -> bool: return _object("Washed-up Bag 1") == "intact", DAY_FRAMES):
		return
	await _until("the chef has a source in reach", func() -> bool: return _next().contains("Washed-up Bag 1"), 30)
	var pack: int = _actor("chef", "ingredients")
	await _tap(KEY_E)
	if not _check(_actor("chef", "ingredients") == pack + 1, "day two collects from a source that time exposed"):
		return
	if not await _until("AI empties all it can walk to", func() -> bool: return _next().contains("Nothing is within reach"), 300):
		return
	if not _check(_stock("Arcade Bag") > 0 and _object("Old Kiosk") == "intact", "stock behind the kiosk is not called reachable"):
		return
	if not await _until("exploration time breaks the ladder", func() -> bool: return _object("Roof Ladder") == "destroyed", DAY_FRAMES):
		return
	if not await _until("AI repairs the ladder with its found wrench", func() -> bool: return _object("Roof Ladder") == "intact" and _next_event_has("repaired Roof Ladder"), 600):
		return
	if not await _until("day two expires", func() -> bool: return _str("phase") == "workshop", DAY_FRAMES):
		return
	if not _check(_object("Old Kiosk") == "destroyed", "exploration time collapsed the kiosk"):
		return
	await _until("AI appraises day two's finds", func() -> bool: return not _next().contains("identifying"), 300)
	await _cook()
	if failed:
		return
	await _tap(KEY_F)
	if not await _until("feeding starts the third day", func() -> bool: return _int("day") == 3, 30):
		return
	_check(_int("eaten") == 2 and _str("reaction") == "satisfied", "a second new dish satisfies the monster")

func _chef_chooses_a_dish() -> void:
	await _tap(KEY_M)
	await _tap(KEY_1)
	await _tap(KEY_1)
	await _tap(KEY_E)
	await _set_out_before_feeding()
	if failed:
		return
	if not await _until("the rest of day one runs out", func() -> bool: return _int("remaining_ms") == 0 and _str("phase") == "workshop", DAY_FRAMES):
		return
	if not await _until("AI appraises the day's finds", func() -> bool: return not _next().contains("identifying"), 300):
		return
	if not _check(_str("dish_offered") == "Mushroom Soup", "with no choice the key offers Companion's dish"):
		return
	_key(KEY_S, true)
	await _frames(30)
	_key(KEY_S, false)
	await _frames(8)
	if not _check(_str("dish_offered") == "Fish Stew", "a held S moves the choice one dish"):
		return
	await _tap(KEY_W)
	await _tap(KEY_W)
	if not _check(_str("dish_offered") == "Pickle Surprise", "W wraps past the first dish"):
		return
	await _tap(KEY_W)
	if not _check(_str("dish_offered") == "Truffle Roast", "W walks up the list"):
		return
	var attempts: int = 0
	while not _stored().has("Truffle Roast") and attempts < 6:
		attempts += 1
		await _tap(KEY_E)
	if not _check(_stored().has("Truffle Roast") and not _stored().has("Mushroom Soup"), "the chosen dish is the one cooked"):
		return
	await _tap(KEY_F)
	if not await _until("day two begins", func() -> bool: return _int("day") == 2, 30):
		return
	await _feed_with_nothing_in_reach()

## A dish keeps, so the screen spends the day's time before ending the day.
func _set_out_before_feeding() -> void:
	await _tap(KEY_Q)
	if not await _until("AI gathers a dish's worth", func() -> bool: return _actor("appraiser", "ingredients") >= 8, 900):
		return
	await _tap(KEY_Q)
	if not await _until("the party comes back early", func() -> bool: return _str("phase") == "workshop", 120):
		return
	if not await _until("AI appraises before the chef cooks", func() -> bool: return _int("identified") >= 2 and not _next().contains("identifying"), 300):
		return
	await _cook()
	if failed:
		return
	if not _check(_int("remaining_ms") > 0 and _next().contains("Set out"), "with time and stock left the screen sets out before feeding"):
		return
	await _tap(KEY_Q)

func _feed_with_nothing_in_reach() -> void:
	await _tap(KEY_Q)
	if not await _until("the emptied city offers nothing", func() -> bool: return _next().contains("Nothing is within reach"), 60):
		return
	await _tap(KEY_Q)
	if not await _until("the party comes home", func() -> bool: return _str("phase") == "workshop", 120):
		return
	_check(_int("remaining_ms") > 0 and _next().contains("[F] Feed"), "with nothing in reach the screen feeds despite the time left")

func _acid_changes_the_city() -> void:
	if not _check(_object("Noodle Vendor") == "intact" and _object("Drifted Ferry") == "dormant", "the vendor stands and the ferry is away"):
		return
	await _tap(KEY_G)
	await _until("the first unfed day ends", func() -> bool: return _int("day") == 4, 30)
	if not _check(_int("acid") == 15 and _str("reaction") == "unfed", "an unfed day raises acid by the penalty"):
		return
	if not await _until("the screen reports the unfed day", func() -> bool: return _next_event_has("unfed"), 30):
		return
	await _tap(KEY_G)
	await _until("the second unfed day ends", func() -> bool: return _int("day") == 5, 30)
	if not await _until("acid reaches the city", func() -> bool: return _object("Noodle Vendor") == "destroyed", 30):
		return
	_check(_object("Drifted Ferry") == "intact" and _int("acid") == 30, "rising acid brings the ferry in")

func _appraiser_run() -> void:
	await _tap(KEY_M)
	if not _check(scene.screen_name() == "main" and scene.survival_status().is_empty(), "M ends the run"):
		return
	await _tap(KEY_1)
	await _tap(KEY_2)
	await _tap(KEY_E)
	if not _check(_actor("appraiser", "key_set") == 0 and _actor("chef", "key_set") == -1, "choosing the appraiser gives AI the chef"):
		return
	if not _check(_int("day") == 1 and _int("acid") == 0 and _object("Noodle Vendor") == "intact", "a new run starts fresh"):
		return
	walker = "appraiser"
	await _tap(KEY_Q)
	for waypoint: Vector2i in [Vector2i(13, 7), Vector2i(13, 12), Vector2i(16, 12)]:
		if not await _walk_to(waypoint.x, waypoint.y):
			return
	if not _check(_next().contains("Tool Bag"), "the screen offers the wrench in reach"):
		return
	await _tap(KEY_E)
	if not _check(_actor("appraiser", "tools") == 1, "the human collects a tool"):
		return
	if not await _until("AI collects mushrooms and tomatoes", func() -> bool: return _stock("Lost Basket") == 0 and _stock("Tomato Machine") == 0 and _int("elapsed_ms") >= 20000, DAY_FRAMES):
		return
	await _tap(KEY_Q)
	if not await _until("the party is back at base", func() -> bool: return _str("phase") == "workshop", 120):
		return
	if not _check(_next().contains("Appraise"), "the human appraiser is told to appraise"):
		return
	var identified: int = _int("identified")
	var attempts: int = 0
	while _next().contains("Appraise") and attempts < 40:
		attempts += 1
		await _tap(KEY_E)
	if not _check(_int("identified") > identified, "E appraises for the appraiser"):
		return
	if not await _until("AI cooks once ingredients are known", func() -> bool: return _int("foods") >= 1, 120):
		return
	await _tap(KEY_F)
	await _until("the appraiser feeds the dish", func() -> bool: return _int("day") == 2, 30)
	if not _check(_int("eaten") == 1, "the appraiser can feed what AI cooked"):
		return
	await _human_repairs_the_ladder()

func _human_repairs_the_ladder() -> void:
	await _tap(KEY_Q)
	for waypoint: Vector2i in [Vector2i(26, 7), Vector2i(26, 4), Vector2i(31, 4)]:
		if not await _walk_to(waypoint.x, waypoint.y):
			return
	if not await _until("the ladder breaks over the human", func() -> bool: return _object("Roof Ladder") == "destroyed", DAY_FRAMES):
		return
	await _until("the screen offers the repair", func() -> bool: return _next().contains("Repair Roof Ladder"), 30)
	if not _check(_next().contains("Repair Roof Ladder"), "the screen offers the repair in reach"):
		return
	_key(KEY_W, true)
	await _frames(24)
	_key(KEY_W, false)
	await _frames(12)
	if not _check(_actor("appraiser", "y") == 4, "a broken ladder blocks the way up"):
		return
	await _tap(KEY_E)
	if not _check(_object("Roof Ladder") == "intact" and _actor("appraiser", "tools") == 1, "E repairs with the carried wrench and keeps it"):
		return
	_key(KEY_W, true)
	await _until("the repaired ladder carries the human up", func() -> bool: return _actor("appraiser", "y") <= 3, 60)
	_key(KEY_W, false)

## The second key set's own keys, which the timing checks inject past.
func _two_players() -> void:
	await _tap(KEY_M)
	await _tap(KEY_1)
	await _tap(KEY_3)
	await _tap(KEY_E)
	if not _check(_actor("chef", "key_set") == 0 and _actor("appraiser", "key_set") == 1, "two players take one key set each"):
		return
	await _tap(KEY_U)
	if not await _until("player two sets out alone", func() -> bool: return _where("appraiser") == "city", 30):
		return
	if not _check(_where("chef") == "base", "a human partner stays until told"):
		return
	var x: int = _actor("appraiser", "x")
	_key(KEY_L, true)
	var walked: bool = await _until("player two walks", func() -> bool: return _actor("appraiser", "x") > x, 30)
	_key(KEY_L, false)
	if not walked:
		return
	await _tap(KEY_U)
	if not await _until("player two returns", func() -> bool: return _str("phase") == "workshop", 30):
		return
	await _tap(KEY_BRACKETRIGHT)
	await _until("player two ends the day", func() -> bool: return _int("day") == 2, 30)

func _next() -> String:
	return _str("next")

func _next_event_has(text: String) -> bool:
	for event: String in _events():
		if event.contains(text):
			return true
	return false

func _events() -> Array:
	return scene.survival_status()["events"]

func _stored() -> Array:
	return scene.survival_status()["stored_foods"]

func _spatula() -> String:
	return _str("spatula")

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

func _stock(source: String) -> int:
	var stock: Dictionary = scene.survival_status()["stock"]
	var remaining: int = stock[source]
	return remaining

func _object(object: String) -> String:
	var objects: Dictionary = scene.survival_status()["objects"]
	var state: String = objects[object]
	return state

## Walks the human in a straight line: every leg here runs along one axis.
func _walk_to(x: int, y: int) -> bool:
	for frame: int in range(240):
		var at_x: int = _actor(walker, "x")
		var at_y: int = _actor(walker, "y")
		if at_x == x and at_y == y:
			_release()
			return true
		_key(KEY_A, x < at_x)
		_key(KEY_D, x > at_x)
		_key(KEY_W, x == at_x and y < at_y)
		_key(KEY_S, x == at_x and y > at_y)
		await process_frame
	_release()
	return _check(false, "%s walks to %d, %d" % [walker, x, y])

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
	push_error("Survival check failed: ", name, " ", scene.survival_status() if scene != null else {})
	failed = true
	quit(1)
	return false
