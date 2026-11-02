class_name SurvivalPlayCheck
extends SceneTree
## Plays the survival game through the shipped scene with real key events: full
## days as the chef, a day as the appraiser, and the map and tool changes between.

const SEED: int = 11
const DAY_FRAMES: int = 2900
## On this seed a party following NEXT alone escapes.
const LONG_RUN_SEED: int = 44
const LONG_RUN_DAYS: int = 60
const DAY_STEPS: int = 400
const GATHERING_DAYS: int = 3
const APPRAISER_DAYS: int = 29
## This seed's pack after GATHERING_DAYS cooks this dish twice.
const CHOSEN: String = "Fish Stew"
const REPLACEMENT_DAYS: int = 12
const MENU_PRESSES: int = 12
const COOK_ATTEMPTS: int = 12
const STEP_FRAMES: int = 6
const ROUTE_KEYS: Dictionary[String, Key] = {"left": KEY_A, "right": KEY_D, "up": KEY_W, "down": KEY_S}
## Left, right, up and down of the first key set, then of the second.
const WALK_KEYS: Array[Key] = [KEY_A, KEY_D, KEY_W, KEY_S, KEY_J, KEY_L, KEY_I, KEY_K]
## Five unfed days raise acid to 60: acid has taken every landed ingredient
## source west of the ferry, and the run is far from lost.
const UNFED_DAYS: int = 5
## From the entrance up the north stairs to the supply crate at the arcade's
## west end.
const CRATE_ROUTE: Array[Vector2i] = [Vector2i(7, 7), Vector2i(7, 2), Vector2i(3, 2)]
## From the entrance down the south stairs into the market, by the tool bag.
const MARKET_ROUTE: Array[Vector2i] = [Vector2i(8, 7), Vector2i(8, 12), Vector2i(13, 12)]
## Up and back down through the drip: two burns, with the third one step up.
const DRIP_ROUTE: Array[Vector2i] = [Vector2i(10, 7), Vector2i(10, 5), Vector2i(10, 7)]
## From the entrance up the ramp to the foot of the roof ladder on the overpass.
const LADDER_ROUTE: Array[Vector2i] = [Vector2i(10, 7), Vector2i(10, 5), Vector2i(16, 5)]
## From the entrance up the ramp, over the overpass and up the roof ladder to
## the roof bag.
const ROOF_ROUTE: Array[Vector2i] = [Vector2i(10, 7), Vector2i(10, 5), Vector2i(16, 5), Vector2i(16, 3), Vector2i(22, 3)]
## The weight the screen draws every changing line in.
const CHANGING_FONT: String = "res://levels/monster_mouth_escape/fonts/Inter-SemiBold.ttf"

var scene: MainScene
var held: Dictionary[Key, bool] = {}
var checks: int = 0
var failed: bool = false
var walker: String = "chef"
var awaiting_quit: bool = false

func _initialize() -> void:
	_run.call_deferred()

## Only the game's own quit reaches here with the pass line owed.
func _finalize() -> void:
	if awaiting_quit and not failed:
		print("Survival: ", checks, " live checks passed (menus, days, return, appraisal, cooking, dish choice, feeding, map, tools, roles, pause, leaving, hand-off, quit).")

func _run() -> void:
	await _frames(2)
	scene = current_scene as MainScene
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
		await _third_day_falls()
	if not failed:
		await _acid_changes_the_city()
	if not failed:
		await _exhaust_the_chef()
	if not failed:
		await _replace_the_spatula()
	if not failed:
		await _ai_chef_work_yields_to_a_dish_to_feed()
	if not failed:
		await _chef_chooses_a_dish()
	if not failed:
		await _follow_next_to_an_escape()
	if not failed:
		await _wrench_stops_setting_out_with_no_stock_behind()
	if not failed:
		await _appraiser_run()
	if not failed:
		await _appraiser_follows_next()
	if not failed:
		await _two_players()
	if not failed:
		await _a_hated_dish_is_not_fed_again()
	if not failed:
		await _a_hated_dish_alone_sets_out_then_ends_the_day()
	if not failed:
		await _player_two_errand_alone_sets_out()
	if not failed:
		await _pause_and_leave()
	if not failed:
		await _hand_off_shows_the_keys_left()
	if not failed:
		await _loss_clears_the_offer()
	if not failed:
		await _main_menu_quits()

## Real keys: Esc, M and Enter over a run, and over a cinematic.
func _pause_and_leave() -> void:
	await _leave_run()
	await _tap(KEY_1)
	await _tap(KEY_1)
	await _tap(KEY_ESCAPE)
	if not _check(_str("cinematic") == "intro" and _str("interruption") == "", "Esc does not pause a cinematic"):
		return
	await _tap(KEY_M)
	if not _check(scene.screen_name() == "main" and scene.survival_status().is_empty(), "M during a cinematic leaves at once"):
		return
	await _tap(KEY_1)
	await _tap(KEY_1)
	await _tap(KEY_E)
	await _tap(KEY_Q)
	if not await _until("the chef sets out", func() -> bool: return _where("chef") == "city", 30):
		return
	if not _check(_screen_shows("Pause, keys"), "the top band names Esc as the way to the keys"):
		return
	await _tap(KEY_ESCAPE)
	if not _check(_str("interruption") == "paused", "Esc pauses the run"):
		return
	var keys: String = str(_lines("pause_keys"))
	if not _check(keys.contains("Run: Shift") and keys.contains("Swap actor: Tab") and keys.contains("WASD or arrows"), "the pause screen lists one player's keys: %s" % keys):
		return
	if not _check(not keys.contains("Hand your actor") and not keys.contains("IJKL"), "the pause screen leaves out keys one player cannot use: %s" % keys):
		return
	if not _text_fits("the pause screen for one player"):
		return
	var day: int = _int("day")
	await _tap(KEY_ESCAPE)
	if not _check(_str("interruption") == "", "Esc continues the run"):
		return
	await _tap(KEY_M)
	if not _check(_str("interruption") == "confirm_leave" and scene.screen_name() == "survival", "M asks before ending a run"):
		return
	if not _check(str(_lines("prompt")).contains("not saved"), "the question says progress is not saved"):
		return
	await _tap(KEY_ESCAPE)
	if not _check(_str("interruption") == "" and _int("day") == day and _where("chef") == "city", "Esc keeps the run"):
		return
	await _tap(KEY_M)
	_key(KEY_ENTER, true)
	await _frames(4)
	if not _check(scene.screen_name() == "main" and scene.survival_status().is_empty(), "Enter ends the run for the main menu"):
		return
	await _tap(KEY_1)
	if not _check(scene.screen_name() == "main", "Enter held into the main menu holds its keys back"):
		return
	_key(KEY_ENTER, false)
	await _frames(4)
	await _tap(KEY_1)
	if not _check(scene.screen_name() == "survival_setup", "the main menu answers once Enter is up"):
		return
	await _tap(KEY_M)

## Player two's keys cross a pause, then each hand-off shows the keys left.
func _hand_off_shows_the_keys_left() -> void:
	await _start_two_players()
	if not _check(_str("hand_off") == "", "no notice while both players remain"):
		return
	await _tap(KEY_ESCAPE)
	var keys: String = str(_lines("pause_keys"))
	for row: String in ["Run: Shift / H", "Hand your actor to AI: X / N", "Swap actor, once one player is left: Tab / none", "Move: WASD or arrows / IJKL"]:
		if not _check(keys.contains(row), "the pause screen for two lists %s" % row):
			return
	if not _text_fits("the pause screen for two players"):
		return
	_key(KEY_U, true)
	await _frames(12)
	await _tap(KEY_ESCAPE)
	if not _check(_str("interruption") == "resuming", "player two's held key holds the resume"):
		return
	await _frames(12)
	_key(KEY_U, false)
	await _frames(30)
	if not _check(_str("interruption") == "" and _where("appraiser") == "base", "player two's key held across a pause does not act"):
		return
	await _tap(KEY_U)
	if not await _until("player two sets out with a fresh press", func() -> bool: return _where("appraiser") == "city", 30):
		return
	await _tap(KEY_X)
	if not await _until("player one hands off", func() -> bool: return _actor("chef", "key_set") == -1, 30):
		return
	if not _check(_actor("appraiser", "key_set") == 0, "the player left moves to the first key set"):
		return
	if not _check(_str("hand_off").contains("Appraiser") and _str("hand_off").contains("WASD move") and _str("hand_off").contains("Tab swap"), "the screen names the keys of the player left"):
		return
	if not _text_fits("the hand-off notice"):
		return
	var x: int = _actor("appraiser", "x")
	_key(KEY_L, true)
	await _frames(30)
	_key(KEY_L, false)
	if not _check(_actor("appraiser", "x") == x, "the second key set no longer moves the appraiser"):
		return
	_key(KEY_D, true)
	var walked: bool = await _until("the first key set moves the appraiser", func() -> bool: return _actor("appraiser", "x") > x, 30)
	_key(KEY_D, false)
	if not walked:
		return
	await _tap(KEY_ESCAPE)
	keys = str(_lines("pause_keys"))
	if not _check(keys.contains("Swap actor: Tab") and not keys.contains("IJKL"), "the pause screen follows the hand-off: %s" % keys):
		return
	await _tap(KEY_ESCAPE)
	await _start_two_players()
	await _tap(KEY_N)
	if not await _until("player two hands off", func() -> bool: return _actor("appraiser", "key_set") == -1, 30):
		return
	if not _check(_str("hand_off").contains("Chef") and _str("hand_off").contains("WASD move"), "the chef's player is told the keys too"):
		return
	await _tap(KEY_TAB)
	if not await _until("the player left swaps", func() -> bool: return _actor("chef", "key_set") == -1, 30):
		return
	if not _check(_str("hand_off").contains("Appraiser"), "the notice follows a swap"):
		return
	await _leave_run()
	await _tap(KEY_1)
	await _tap(KEY_1)
	await _tap(KEY_E)
	await _tap(KEY_TAB)
	if not await _until("one player swaps", func() -> bool: return _actor("chef", "key_set") == -1, 30):
		return
	_check(_str("hand_off") == "", "one player from the start gets no hand-off notice")

func _start_two_players() -> void:
	await _leave_run()
	await _tap(KEY_1)
	await _tap(KEY_3)
	await _tap(KEY_E)

## 3 ends the application: the pass line prints from `_finalize` only.
func _main_menu_quits() -> void:
	await _leave_run()
	if not _check(scene.screen_name() == "main", "back at the main menu"):
		return
	awaiting_quit = true
	await _tap(KEY_3)
	await _frames(30)
	_check(false, "3 on the main menu quits")

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
	if not _check(_goal_shown(), "the base shows when escape checks begin"):
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
	if not _check(_out_of_reach().has("Arcade Bag") and not _out_of_reach().has("Tomato Machine"), "only stock behind the kiosk is marked out of reach"):
		return
	if not _text_fits("the first city screen"):
		return
	if not _plates_apart("the first city screen"):
		return
	if not _check(_goal_shown(), "the city shows when escape checks begin"):
		return
	if not _check(_screen_shows("[E] Use") and not _screen_shows("[E] Collect") and _screen_shows("[Q] Return to base") and _screen_shows("WASD move  Shift run"), "away from a source the action card shows E unlit, Q home and the move keys"):
		return
	if not await _walk_to(5, 7):
		return
	if not _check(_screen_shows("[E] Collect") and _screen_shows("Tomato Machine") and not _screen_shows("[E] Use"), "at a source the action card offers E to collect from it"):
		return
	if not _check(_next().begins_with("Walk "), "the screen walks past a kind the party already took"):
		return
	var stock: int = _stock("Tomato Machine")
	await _tap(KEY_E)
	if not _check(_actor("chef", "ingredients") == 1 and _stock("Tomato Machine") < stock, "E collects into the pack and spends shared stock"):
		return
	await _run_east()
	if failed:
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
	await _until("running reaches the ramp", func() -> bool: return _actor("chef", "x") >= 7, 240)
	_key(KEY_D, false)
	_key(KEY_SHIFT, false)
	if failed:
		return
	if not _check(_actor("chef", "stamina") < stamina, "running spends stamina"):
		return
	if not await _walk_to(10, 7):
		return
	var line: String = _next()
	var target: String = line.get_slice(" to ", 1).get_slice(".", 0)
	var objects: Dictionary = scene.survival_status()["objects"]
	_check(line.begins_with("Walk ") and objects.has(target), "the screen names the object to walk to")

## Stops before anything is collected, so the fall leaves time on the clock.
func _third_day_falls() -> void:
	await _tap(KEY_Q)
	if not await _until("chef sets out to the drip", func() -> bool: return _where("chef") == "city", 30):
		return
	if not await _walk_route([Vector2i(10, 7), Vector2i(10, 5)]):
		return
	if not _check(_actor("chef", "health") == 2 and _next_event_has("burned"), "the acid drip costs one health slot"):
		return
	await _walk_to(11, 5)
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
	if not _check(_next_event_has("fell"), "the screen reports the fall"):
		return
	if not await _until("AI comes back to base", func() -> bool: return _str("phase") == "workshop", 120):
		return
	var left: int = _int("remaining_ms")
	await _frames(30)
	if not _check(_int("remaining_ms") == left and left > 0, "a return with time left keeps the rest of the day"):
		return
	if not await _until("AI appraises before the chef goes out again", func() -> bool: return not _next().contains("identifying"), 300):
		return
	if not _check(_next().contains("Set out") or _next().contains("[E] "), "with time left the screen offers more to do"):
		return
	await _tap(KEY_Q)
	if not await _until("chef goes out again with time left", func() -> bool: return _where("chef") == "city", 30):
		return
	await _tap(KEY_Q)
	if not await _until("the party is home again", func() -> bool: return _str("phase") == "workshop", 120):
		return
	var day: int = _int("day")
	await _tap(KEY_G)
	await _until("the fall's day ends", func() -> bool: return _int("day") == day + 1, 30)

## Starts a new day, so that drips, not the clock, end the trip.
func _exhaust_the_chef() -> void:
	await _tap(KEY_Q)
	if not await _until("chef re-enters", func() -> bool: return _where("chef") == "city", 30):
		return
	if not _check(_spatula() == "intact", "the spatula starts intact"):
		return
	await _frames(2)
	if not _plates_apart("the damaged city at acid %d" % _int("acid")):
		return
	if not await _walk_route(DRIP_ROUTE):
		return
	if not _check(_actor("chef", "health") == 1, "two more drips leave one slot"):
		return
	_key(KEY_W, true)
	var exhausted: bool = await _until("the third drip", func() -> bool: return _actor("chef", "exhaustions") == 1, 60)
	_key(KEY_W, false)
	if not exhausted:
		return
	if not _check(_where("chef") == "base" and _actor("chef", "health") == 3, "exhaustion carries the chef to base at full health"):
		return
	if not _check(_spatula() == "damaged", "exhaustion damages the chef's spatula"):
		return
	await _until("AI comes back after the exhaustion", func() -> bool: return _str("phase") == "workshop", 120)

## The damaged spatula stays damaged until the party finds its replacement.
func _replace_the_spatula() -> void:
	var day: int = _int("day")
	while not _next().contains("Replace the damaged Spatula"):
		if _int("day") >= day + REPLACEMENT_DAYS or _str("phase") == "lost" or _str("phase") == "escaped":
			_check(false, "the screen offers the found replacement")
			return
		await _follow_next()
	var carried: int = _actor("chef", "tools") + _actor("appraiser", "tools")
	await _tap(KEY_R)
	_check(_spatula() == "intact" and _actor("chef", "tools") + _actor("appraiser", "tools") == carried - 1, "R spends a found tool on the damaged spatula")

## Tab hands the chef to AI, which works on the next step, so the line read on
## the swap's first frame names that work: cooking with no dish stored, but the
## stored dish while the AI chef only replaces the spatula.
func _ai_chef_work_yields_to_a_dish_to_feed() -> void:
	await _leave_run()
	await _tap(KEY_1)
	await _tap(KEY_1)
	await _tap(KEY_E)
	await _tap(KEY_Q)
	if not await _until("the chef sets out for the drips", func() -> bool: return _where("chef") == "city", 30):
		return
	if not await _walk_route(DRIP_ROUTE):
		return
	_key(KEY_W, true)
	var exhausted: bool = await _until("the chef is exhausted on day one", func() -> bool: return _actor("chef", "exhaustions") == 1, 60)
	_key(KEY_W, false)
	if not exhausted:
		return
	if not await _until("AI comes back after the chef's exhaustion", func() -> bool: return _str("phase") == "workshop", 120):
		return
	if not await _until("AI appraises before the chef cooks", func() -> bool: return not _next().contains("identifying"), 300):
		return
	if not _check(_spatula() == "damaged" and _stored().is_empty() and _str("dish_offered") != "", "the spatula is damaged, no dish is stored and one can be cooked"):
		return
	_key(KEY_TAB, true)
	var swapped: bool = await _until("the player hands the chef to AI", func() -> bool: return _actor("chef", "key_set") == -1, 30)
	var line: String = _next()
	_key(KEY_TAB, false)
	if not swapped:
		return
	if not _check(line == "The AI chef is cooking." and _stored().is_empty(), "with no dish to feed the line waits for the AI chef: %s" % line):
		return
	if not await _until("the AI chef cooks", func() -> bool: return not _stored().is_empty(), 30):
		return
	await _tap(KEY_TAB)
	if not await _until("the player takes the chef back", func() -> bool: return _actor("chef", "key_set") == 0, 30):
		return
	await _tap(KEY_G)
	if not await _until("the first day ends with the dish kept", func() -> bool: return _int("day") == 2, 30):
		return
	await _tap(KEY_Q)
	if not await _until("the chef sets out for the spatula", func() -> bool: return _where("chef") == "city", 30):
		return
	if not await _walk_route(CRATE_ROUTE):
		return
	await _tap(KEY_E)
	if not await _until("the day's clock brings the party home", func() -> bool: return _str("phase") == "workshop", DAY_FRAMES):
		return
	if not await _until("AI appraises the second day's finds", func() -> bool: return not _next().contains("identifying"), 300):
		return
	if not _check(_spatula() == "damaged" and _actor("chef", "tools") == 1 and not _stored().is_empty(), "the chef carries a spatula home to a damaged one and a stored dish"):
		return
	_key(KEY_TAB, true)
	swapped = await _until("the player hands the chef to AI again", func() -> bool: return _actor("chef", "key_set") == -1, 30)
	line = _next()
	var spatula: String = _spatula()
	_key(KEY_TAB, false)
	if not swapped:
		return
	_check(spatula == "damaged" and line.begins_with("[F] Feed "), "a dish to feed comes before the AI chef replacing the spatula: %s" % line)

func _workshop_day_one() -> void:
	if not await _until("AI appraises the finds", func() -> bool: return _int("identified") >= 2 and not _next().contains("identifying"), 300):
		return
	if not _check(_int("remaining_ms") == 0 and _int("day") == 1, "the workshop runs with no clock and stays on the same day"):
		return
	await _cook()
	if failed:
		return
	if not _check(_next().contains("[F] Feed"), "the screen offers feeding the stored dish"):
		return
	if not _text_fits("the workshop"):
		return
	var fed: String = _stored()[0]
	if not _check(_lines("day_result").is_empty(), "the workshop shows no result until a day has ended"):
		return
	await _tap(KEY_F)
	if not await _until("feeding starts the next day", func() -> bool: return _int("day") == 2, 30):
		return
	if not _check(_result_says("Day 1: " + fed) and _result_says("Satisfied"), "the result names the dish and the reaction"):
		return
	if not _check(_result_says("New food 50 + ") and _result_says("needs 100"), "the result breaks the score down"):
		return
	if not _check(_result_says("Acid 0 to 0.") and _result_says("Map changes: "), "the result shows the acid and the map changes"):
		return
	var eaten: String = _lines("eaten_lines")[0]
	if not _check(eaten.begins_with("Eaten: "), "the eaten ingredients are listed"):
		return
	await _frames(30)
	if not _check(_result_says("Day 1: " + fed) and _int("remaining_ms") == 45000, "the result stands on screen and spends no time"):
		return
	if not _text_fits("the day's result"):
		return
	if not _check(_str("reaction") == "satisfied" and _int("score") >= 100, "a new dish satisfies the monster"):
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
	var pack: int = _actor("chef", "ingredients") + _actor("appraiser", "ingredients")
	if not await _until("day two expires", func() -> bool: return _str("phase") == "workshop", DAY_FRAMES):
		return
	if not _check(_int("remaining_ms") == 0 and _actor("chef", "ingredients") + _actor("appraiser", "ingredients") > pack, "collecting runs the day's clock out"):
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
	await _leave_run()
	await _tap(KEY_1)
	await _tap(KEY_1)
	await _tap(KEY_E)
	await _set_out_before_feeding()
	if failed:
		return
	await _gather_rest_of_day()
	for day: int in range(GATHERING_DAYS):
		await _tap(KEY_Q)
		await _gather_rest_of_day()
		if failed:
			return
	var first: String = _str("dish_offered")
	if not _check(first != "", "with no choice the key offers Companion's dish"):
		return
	if not _check(_str("forecast").begins_with("Plain ") and _str("forecast").contains(", fine ") and _str("forecast").ends_with(": needs 100"), "the panel forecasts the offered dish against the threshold"):
		return
	await _tap(KEY_TAB)
	if not _check(_actor("chef", "key_set") == -1, "the human takes the appraiser"):
		return
	_key(KEY_S, true)
	await _frames(12)
	await _tap(KEY_TAB)
	var unmoved: String = _str("dish_offered")
	_key(KEY_S, false)
	await _frames(8)
	if not _check(_actor("chef", "key_set") == 0 and unmoved == first, "a key held through the swap chooses nothing"):
		return
	_key(KEY_S, true)
	await _frames(1)
	_key(KEY_S, false)
	await _tap(KEY_ESCAPE)
	await _tap(KEY_ESCAPE)
	await _frames(12)
	if not _check(_str("dish_offered") == first, "a dish press pending when the pause opens chooses nothing"):
		return
	_key(KEY_S, true)
	await _frames(30)
	_key(KEY_S, false)
	await _frames(8)
	var second: String = _str("dish_offered")
	if not _check(second != first, "a held S moves the choice"):
		return
	_key(KEY_S, true)
	await _frames(12)
	await _tap(KEY_W)
	_key(KEY_S, false)
	await _frames(8)
	if not _check(_str("dish_offered") == second, "S held under a tapped W counts each press once"):
		return
	_key(KEY_A, true)
	await _frames(12)
	await _tap(KEY_S)
	_key(KEY_A, false)
	await _frames(8)
	var third: String = _str("dish_offered")
	if not _check(third != second and third != first, "S chooses while A is held"):
		return
	await _tap(KEY_W)
	if not _check(_str("dish_offered") == second, "W walks up the list"):
		return
	var presses: int = 0
	await _tap(KEY_W)
	while _str("dish_offered") != second and presses < MENU_PRESSES:
		presses += 1
		await _tap(KEY_W)
	if not _check(presses >= 2 and _str("dish_offered") == second, "W wraps past the first dish"):
		return
	while _str("dish_offered") != CHOSEN and presses < 2 * MENU_PRESSES:
		presses += 1
		await _tap(KEY_W)
	var attempts: int = 0
	while not _stored().has(CHOSEN) and attempts < 6:
		attempts += 1
		await _tap(KEY_E)
	if not _check(_stored().has(CHOSEN), "the chosen dish is the one cooked"):
		return
	if not _check(_str("dish_offered") == CHOSEN, "the choice stands while it can be cooked"):
		return
	attempts = 0
	while _str("dish_offered") == CHOSEN and attempts < COOK_ATTEMPTS:
		attempts += 1
		await _tap(KEY_E)
	var fallback: String = _str("dish_offered")
	if not _check(fallback != CHOSEN and fallback != "", "a choice out of ingredients falls back to Companion's dish"):
		return
	await _tap(KEY_S)
	presses = 0
	while _str("dish_offered") == fallback and presses < MENU_PRESSES:
		presses += 1
		await _tap(KEY_S)
	var late: String = _str("dish_offered")
	if not _check(late != fallback, "a choice is made late in the day"):
		return
	var day: int = _int("day")
	# An unfed day leaves the pack and the eating history as they were, so the new
	# day offers the dish this one fell back to.
	await _tap(KEY_G)
	if not await _until("the next day begins", func() -> bool: return _int("day") == day + 1, 30):
		return
	if not _check(_str("dish_offered") == fallback, "a new day clears the choice"):
		return
	attempts = 0
	while not _stored().has(fallback) and attempts < 6:
		attempts += 1
		await _tap(KEY_E)
	_check(_stored().has(fallback), "the key cooks the dish it fell back to")

## Gathers until the clock or the stock ends, and ends the day with the finds
## appraised.
func _gather_rest_of_day() -> void:
	var day: int = _int("day")
	var steps: int = 0
	while _str("phase") == "exploration" and not _next().contains("Nothing is within reach") and steps < DAY_STEPS:
		steps += 1
		await _follow_next()
	await _tap(KEY_Q)
	if not await _until("the party comes home", func() -> bool: return _str("phase") == "workshop", 300):
		return
	if not await _until("AI appraises the day's finds", func() -> bool: return not _next().contains("identifying"), 300):
		return
	await _tap(KEY_G)
	await _until("the gathering day ends", func() -> bool: return _int("day") == day + 1, 30)
	if failed:
		return
	await _until("AI appraises what was left", func() -> bool: return not _next().contains("identifying"), 300)

## A dish keeps, so the screen spends the day's time before ending the day.
func _set_out_before_feeding() -> void:
	await _tap(KEY_Q)
	if not await _until("AI gathers a dish's worth", func() -> bool: return _actor("appraiser", "ingredients") >= 2, 900):
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

## Plays on what NEXT says alone to an escape. A dish the monster hated stays
## stored, so the line may end a day unfed beside it, but never beside a dish it
## would feed.
func _follow_next_to_an_escape() -> void:
	await _leave_run()
	await _tap(KEY_1)
	scene.pin_survival_seed(LONG_RUN_SEED)
	await _tap(KEY_1)
	await _tap(KEY_E)
	var hated: Array[String] = []
	var beside: Array = []
	var day: int = _int("day")
	var steps: int = 0
	while beside.is_empty() and (_str("phase") == "workshop" or _str("phase") == "exploration") and day < LONG_RUN_DAYS:
		var line: String = _next()
		if line.contains("End the day unfed"):
			beside = _stored().filter(func(dish: String) -> bool: return not hated.has(dish))
		await _follow_next()
		if _int("day") != day and line.contains("[F] Feed") and _str("reaction") == "disliked":
			hated.append(line.get_slice("Feed ", 1).get_slice(" to the monster", 0))
		steps = 0 if _int("day") != day else steps + 1
		day = _int("day")
		if steps >= DAY_STEPS:
			_check(false, "following NEXT keeps the days moving")
			return
	if not _check(beside.is_empty(), "the line never ends a day unfed beside a dish it would feed: %s" % [beside]):
		return
	if not _check(_str("phase") == "escaped", "following NEXT alone escapes"):
		return
	_check(not hated.is_empty(), "the run met a dish the monster hated")
	scene.pin_survival_seed(SEED)

## A repair is an errand only towards stock, so a wrench in hand does not keep
## the line setting out once the broken passages lead to nothing.
func _wrench_stops_setting_out_with_no_stock_behind() -> void:
	# A second player keeps the appraiser home, so the chef alone spends the
	# shared clock on the roof bag before acid breaks its ladder.
	await _start_two_players()
	await _tap(KEY_Q)
	if not await _until("the chef sets out alone for the roof bag", func() -> bool: return _where("chef") == "city" and _where("appraiser") == "base", 30):
		return
	if not await _walk_route(ROOF_ROUTE):
		return
	await _tap(KEY_E)
	await _tap(KEY_E)
	if not _check(_stock("Roof Bag") == 0 and _object("Roof Ladder") == "intact", "the roof bag is emptied while its ladder stands"):
		return
	await _tap(KEY_Q)
	if not await _until("the chef comes home from the roof", func() -> bool: return _str("phase") == "workshop", 120):
		return
	await _tap(KEY_G)
	if not await _until("an unfed day breaks the ladder", func() -> bool: return _int("day") == 2 and _object("Roof Ladder") == "destroyed", 30):
		return
	await _tap(KEY_Q)
	if not await _until("the chef sets out alone for the wrench", func() -> bool: return _where("chef") == "city" and _where("appraiser") == "base", 30):
		return
	if not await _walk_route(MARKET_ROUTE):
		return
	await _tap(KEY_E)
	if not _check(_actor("chef", "tools") == 1, "the chef carries the wrench"):
		return
	# The rest is one-player play, with the AI appraiser.
	await _tap(KEY_N)
	if not await _until("player two hands the appraiser to AI", func() -> bool: return _actor("appraiser", "key_set") == -1 and _actor("chef", "key_set") == 0, 30):
		return
	if not await _gather_unfed_until_nothing_is_left():
		return
	if not _check(_object("Roof Ladder") == "destroyed" and _stock("Roof Bag") == 0, "a broken passage is left, with no stock behind it"):
		return
	if not _check(_int("remaining_ms") > 0 and not _next().contains("Set out"), "a wrench in hand does not set out for it"):
		return
	await _tap(KEY_Q)
	if not await _until("the chef goes to the ladder anyway", func() -> bool: return _where("chef") == "city", 30):
		return
	await _frames(2)
	if not _plates_apart("the city at acid %d" % _int("acid")):
		return
	if not await _walk_route(LADDER_ROUTE):
		return
	if not _check(_next().contains("Nothing is within reach"), "beside the dead passage the line names no repair"):
		return
	await _tap(KEY_E)
	_check(_object("Roof Ladder") == "intact", "the key still repairs what the line leaves out")

## Follows NEXT, but ends each day unfed where it names feeding, until it names
## ending the day with time left: nothing in reach is left once the rising acid
## has taken what the party did not empty. A line that sends the player in
## circles spends the budget of steps a day takes.
func _gather_unfed_until_nothing_is_left() -> bool:
	var day: int = _int("day")
	var steps: int = 0
	while not ((_next().contains("End the day unfed") or _next().contains("[F] Feed")) and _int("remaining_ms") > 0):
		if _str("phase") == "escaped" or _str("phase") == "lost" or _int("day") >= LONG_RUN_DAYS:
			return _check(false, "the stock runs out before the run ends")
		if _next().contains("[F] Feed"):
			await _tap(KEY_G)
		else:
			await _follow_next()
		steps = 0 if _int("day") != day else steps + 1
		day = _int("day")
		if steps >= DAY_STEPS:
			return _check(false, "following NEXT keeps the days moving")
	return true

## Presses the one key NEXT names, or waits while it names none.
func _follow_next() -> void:
	var line: String = _next()
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
		await _walk_a_cell(line)
	else:
		await _frames(12)

## Walks one cell the way NEXT names: a press and release in one frame walks one
## step at any frame length.
func _walk_a_cell(line: String) -> void:
	var code: Key = ROUTE_KEYS.get(line.get_slice(" ", 1), KEY_NONE)
	if code != KEY_NONE:
		_key(code, true)
		_key(code, false)
	await _frames(STEP_FRAMES)

func _acid_changes_the_city() -> void:
	if not _check(_object("Noodle Vendor") == "intact" and _object("Drifted Ferry") == "dormant", "the vendor stands and the ferry is away"):
		return
	if not _check(_int("day") == 4 and _int("acid") == 12 and _str("reaction") == "unfed", "an unfed day raises acid by the penalty"):
		return
	if not _check(_result_says("Day 3: Unfed") and _result_says("Acid 0 to 12."), "the result of an unfed day shows the acid it cost"):
		return
	if not await _until("the unfed result counts the map changes", func() -> bool: return _result_says("Map changes: ") and not _result_says("Map changes: 0"), 30):
		return
	if not await _until("the screen reports the unfed day", func() -> bool: return _next_event_has("unfed"), 30):
		return
	if not await _until("acid breaks the ladder", func() -> bool: return _object("Roof Ladder") == "destroyed", 30):
		return
	if not _check(_object("Washed-up Bag 2") == "dormant", "acid brings no bag"):
		return
	if not _check(_int("elapsed_ms") == 135000 and _object("Washed-up Bag 1") == "intact", "every finished day passes its whole time and lands the bag"):
		return
	await _collect_from_the_tomato_machine()
	if failed:
		return
	await _tap(KEY_G)
	await _until("the second unfed day ends", func() -> bool: return _int("day") == 5, 30)
	if not await _until("acid reaches the city", func() -> bool: return _object("Noodle Vendor") == "destroyed", 30):
		return
	await _tap(KEY_G)
	await _until("the third unfed day ends", func() -> bool: return _int("day") == 6, 30)
	await _until("acid brings the ferry", func() -> bool: return _object("Drifted Ferry") == "intact", 30)
	_check(_object("Drifted Ferry") == "intact" and _int("acid") == 36, "rising acid brings the ferry in")

func _collect_from_the_tomato_machine() -> void:
	await _tap(KEY_Q)
	if not await _until("the chef sets out for the machine", func() -> bool: return _where("chef") == "city", 30):
		return
	if not await _walk_to(5, 7):
		return
	# The machine is two cells in, closer than the hold on E after setting out.
	await _frames(2 * STEP_FRAMES)
	var pack: int = _actor("chef", "ingredients")
	var left: int = _int("remaining_ms")
	await _tap(KEY_E)
	if not _check(_actor("chef", "ingredients") == pack + 1, "the chef collects from a source acid has not reached"):
		return
	if not _check(_int("remaining_ms") <= left - 12000, "a collection spends the shared clock"):
		return
	await _tap(KEY_Q)
	await _until("the party is home from the machine", func() -> bool: return _str("phase") == "workshop", 120)

func _appraiser_run() -> void:
	await _leave_run()
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
	if not await _walk_route(MARKET_ROUTE):
		return
	if not _check(_next().contains("Tool Bag") and _next().contains("(spends 0:12)"), "the screen offers the wrench in reach and its cost in time"):
		return
	if not _check(_screen_says("Each collection spends 0:12"), "the pack says what every collection spends"):
		return
	await _tap(KEY_E)
	if not _check(_actor("appraiser", "tools") == 1, "the human collects a tool"):
		return
	if not await _until("AI collects", func() -> bool: return _actor("chef", "ingredients") >= 2, DAY_FRAMES):
		return
	await _tap(KEY_Q)
	if not await _until("the party is back at base", func() -> bool: return _str("phase") == "workshop", 120):
		return
	if not _check(_next().contains("Appraise") and _str("appraise_job").contains("[E] Appraise"), "the human appraiser is told to appraise"):
		return
	var identified: int = _int("identified")
	var attempts: int = 0
	while _next().contains("Appraise") and attempts < 40:
		attempts += 1
		await _tap(KEY_E)
	if not _check(_int("identified") > identified, "E appraises for the appraiser"):
		return
	if not _check(_str("appraise_job") == "Nothing to appraise.", "the panel stops asking once nothing is unknown"):
		return
	if not await _until("AI cooks once ingredients are known", func() -> bool: return _int("foods") >= 1, 120):
		return
	await _tap(KEY_F)
	await _until("the appraiser feeds the dish", func() -> bool: return _int("day") == 2, 30)
	if not _check(_int("eaten") == 1, "the appraiser can feed what AI cooked"):
		return
	await _human_repairs_the_ladder()

## An AI chef cooks a step after the last appraisal, so NEXT must not end that day.
func _appraiser_follows_next() -> void:
	await _leave_run()
	await _tap(KEY_1)
	await _tap(KEY_2)
	await _tap(KEY_E)
	var steps: int = 0
	var fed: bool = true
	while _int("day") <= APPRAISER_DAYS and steps < APPRAISER_DAYS * DAY_STEPS:
		steps += 1
		await _follow_next()
		fed = fed and _str("reaction") != "unfed"
	if not _check(fed, "following NEXT feeds what an AI chef can cook"):
		return
	_check(_int("day") > APPRAISER_DAYS, "the appraiser's days keep moving on NEXT alone")

func _human_repairs_the_ladder() -> void:
	await _tap(KEY_Q)
	if not await _until("day two runs out", func() -> bool: return _int("remaining_ms") == 0 and _str("phase") == "workshop", DAY_FRAMES):
		return
	if not await _until("AI cooks day two's finds", func() -> bool: return _int("foods") >= 1, 300):
		return
	await _tap(KEY_F)
	if not await _until("the appraiser feeds day two's dish", func() -> bool: return _int("day") == 3, 30):
		return
	await _tap(KEY_G)
	if not await _until("acid breaks the ladder", func() -> bool: return _object("Roof Ladder") == "destroyed", 30):
		return
	await _tap(KEY_Q)
	if not await _walk_route(LADDER_ROUTE):
		return
	await _until("the screen offers the repair", func() -> bool: return _next().contains("Repair Roof Ladder"), 30)
	if not _check(_next().contains("Repair Roof Ladder") and _screen_shows("[E] Repair") and _screen_shows("Roof Ladder"), "the screen offers the repair in reach"):
		return
	_key(KEY_W, true)
	await _frames(24)
	_key(KEY_W, false)
	await _frames(12)
	if not _check(_actor("appraiser", "y") == 5, "a broken ladder blocks the way up"):
		return
	await _tap(KEY_E)
	if not _check(_object("Roof Ladder") == "intact" and _actor("appraiser", "tools") == 1, "E repairs with the carried wrench and keeps it"):
		return
	_key(KEY_W, true)
	await _until("the repaired ladder carries the human up", func() -> bool: return _actor("appraiser", "y") <= 3, 60)
	_key(KEY_W, false)

## The second key set's own keys, which the timing checks inject past.
func _two_players() -> void:
	await _start_two_players()
	if not _check(_actor("chef", "key_set") == 0 and _actor("appraiser", "key_set") == 1, "two players take one key set each"):
		return
	if not _check(_next().begins_with("[Q]/[U] Set out into the city"), "the base line names both players' set-out keys: %s" % _next()):
		return
	if not _text_fits("two players at base before setting out"):
		return
	await _tap(KEY_Q)
	if not await _until("player one sets out alone", func() -> bool: return _where("chef") == "city", 30):
		return
	if not _check(_next().contains("P1: Walk ") and _next().contains("P2: [U] Set out into the city"), "the player left at base is told to set out: %s" % _next()):
		return
	if not _check(_screen_shows("[U] Set out") and not _screen_shows("[U] Return to base") and _screen_shows("[Q] Return to base"), "the action card names U as setting out for player two at base"):
		return
	if not _text_fits("one player out and one at base"):
		return
	await _tap(KEY_Q)
	if not await _until("player one comes back", func() -> bool: return _str("phase") == "workshop", 30):
		return
	await _tap(KEY_U)
	if not await _until("player two sets out alone", func() -> bool: return _where("appraiser") == "city", 30):
		return
	if not _check(_where("chef") == "base", "a human partner stays until told"):
		return
	await _tap(KEY_Q)
	if not await _until("player one joins", func() -> bool: return _where("chef") == "city", 30):
		return
	if not _check(_next().contains("P1: Walk ") and _next().contains("P2: Walk "), "the screen tells each player where to walk"):
		return
	if not _text_fits("two players in the city"):
		return
	if not _plates_apart("two players in the city"):
		return
	await _tap(KEY_Q)
	if not await _until("player one goes home", func() -> bool: return _where("chef") == "base", 30):
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
	if not _text_fits("two players at base"):
		return
	await _tap(KEY_BRACKETRIGHT)
	await _until("player two ends the day", func() -> bool: return _int("day") == 2, 30)

## Player two stays at base, so only player one's route spends the clock: two
## days give two Pickle Surprises and a Mushroom Soup's worth.
func _a_hated_dish_is_not_fed_again() -> void:
	await _start_two_players()
	walker = "chef"
	if not await _collect_along([Vector2i(8, 7), Vector2i(8, 12), Vector2i(6, 12), Vector2i(6, 13), Vector2i(4, 13)], 2):
		return
	if not await _collect_along([Vector2i(7, 13)], 1):
		return
	await _tap(KEY_Q)
	await _until("player one comes home", func() -> bool: return _str("phase") == "workshop", 120)
	await _tap(KEY_G)
	if not await _until("the first gathering day ends", func() -> bool: return _int("day") == 2, 30):
		return
	if not await _collect_along([Vector2i(8, 7), Vector2i(8, 12), Vector2i(6, 12), Vector2i(6, 13), Vector2i(7, 13)], 2):
		return
	if not await _collect_along([Vector2i(6, 13), Vector2i(6, 12), Vector2i(8, 12), Vector2i(8, 7), Vector2i(5, 7)], 1):
		return
	await _tap(KEY_Q)
	await _until("player one comes home again", func() -> bool: return _str("phase") == "workshop", 120)
	var attempts: int = 0
	while _next().contains("[O] Appraise") and attempts < 40:
		attempts += 1
		await _tap(KEY_O)
	while _str("dish_offered") != "Pickle Surprise" and attempts < 40 + MENU_PRESSES:
		attempts += 1
		await _tap(KEY_S)
	attempts = 0
	while _stored().count("Pickle Surprise") < 2 and attempts < COOK_ATTEMPTS:
		attempts += 1
		await _tap(KEY_E)
	if not _check(_stored() == ["Pickle Surprise", "Pickle Surprise"], "player one cooks the disliked dish twice"):
		return
	await _tap(KEY_F)
	if not await _until("the hated dish ends the day", func() -> bool: return _int("day") == 3, 30):
		return
	if not _check(_str("reaction") == "disliked" and _stored() == ["Pickle Surprise"], "the monster hates the dish and one is left"):
		return
	await _tap(KEY_F)
	await _frames(30)
	if not _check(_int("day") == 3 and _stored() == ["Pickle Surprise"], "F does not feed a hated dish again"):
		return
	_check(_next().begins_with("[E] Cook ") and not _next().contains("Feed"), "with only a hated dish stored the screen cooks: %s" % _next())

## Cooks and feeds the last of the pack, so only the hated dish is left and
## nothing can be cooked: the line sets out while the day has time, then ends it.
func _a_hated_dish_alone_sets_out_then_ends_the_day() -> void:
	var attempts: int = 0
	while _stored().size() < 2 and attempts < COOK_ATTEMPTS:
		attempts += 1
		await _tap(KEY_E)
	if not _check(_stored().size() == 2 and _stored().has("Pickle Surprise"), "player one cooks the last of the pack beside the hated dish"):
		return
	await _tap(KEY_F)
	if not await _until("the new dish ends the day", func() -> bool: return _int("day") == 4, 30):
		return
	if not _check(_stored() == ["Pickle Surprise"] and _str("dish_offered") == "" and _int("remaining_ms") > 0, "only the hated dish is left, nothing can be cooked and the day has time"):
		return
	if not _check(_next().begins_with("[Q]/[U] Set out into the city"), "with nothing to cook or feed the line sets out while the day has time: %s" % _next()):
		return
	await _tap(KEY_Q)
	if not await _until("player one sets out", func() -> bool: return _where("chef") == "city", 30):
		return
	if not await _until("the day's clock runs out", func() -> bool: return _str("phase") == "workshop", DAY_FRAMES):
		return
	if not _check(_int("remaining_ms") == 0 and _stored() == ["Pickle Surprise"] and _str("dish_offered") == "", "the day ran out with only the hated dish and nothing to cook"):
		return
	_check(_next().begins_with("[G] End the day unfed"), "with nothing to cook or feed and no time left the line ends the day unfed: %s" % _next())

## Player two takes the only wrench, and acid leaves stock only behind broken
## passages: player one has nothing to do, yet the base line sets out.
func _player_two_errand_alone_sets_out() -> void:
	await _start_two_players()
	walker = "appraiser"
	await _tap(KEY_U)
	if not await _until("player two sets out for the wrench", func() -> bool: return _where("appraiser") == "city", 30):
		return
	if not await _walk_route(MARKET_ROUTE):
		return
	await _tap(KEY_O)
	if not _check(_actor("appraiser", "tools") == 1, "player two carries the wrench"):
		return
	await _tap(KEY_U)
	if not await _until("player two brings the wrench home", func() -> bool: return _str("phase") == "workshop", 30):
		return
	for day: int in range(UNFED_DAYS):
		await _tap(KEY_G)
		if not await _until("an unfed day ends", func() -> bool: return _int("day") == day + 2, 30):
			return
	await _tap(KEY_U)
	if not await _until("player two sets out for the spatulas", func() -> bool: return _where("appraiser") == "city", 30):
		return
	if not await _walk_route(CRATE_ROUTE):
		return
	await _tap(KEY_O)
	await _tap(KEY_O)
	await _tap(KEY_U)
	if not await _until("player two comes home", func() -> bool: return _str("phase") == "workshop", 30):
		return
	if not _check(_stock("Supply Crate") == 0 and _actor("appraiser", "tools") == 3 and _actor("chef", "tools") == 0, "player two empties the last source in reach and only player two holds the wrench"):
		return
	if not _check(_object("Roof Ladder") == "destroyed" and _stock("Roof Bag") > 0 and _int("remaining_ms") > 0, "stock is left behind the broken ladder and the day has time"):
		return
	if not _check(_next().begins_with("[Q]/[U] Set out into the city"), "the base line sets out for player two's errand alone: %s" % _next()):
		return
	await _tap(KEY_U)
	if not await _until("player two sets out for the repair", func() -> bool: return _where("appraiser") == "city", 30):
		return
	_check(_next().begins_with("Walk ") and not _next().contains("Set out"), "only player two is given something to do: %s" % _next())

## Sets out if at base, walks the waypoints and collects `times` at the last.
func _collect_along(waypoints: Array[Vector2i], times: int) -> bool:
	if _where(walker) == "base":
		await _tap(KEY_Q)
	for waypoint: Vector2i in waypoints:
		if not await _walk_to(waypoint.x, waypoint.y):
			return false
	var pack: int = _actor(walker, "ingredients")
	for time: int in range(times):
		await _tap(KEY_E)
	return _check(_actor(walker, "ingredients") == pack + times, "player one collects %d at %s" % [times, waypoints.back()])

func _loss_clears_the_offer() -> void:
	await _leave_run()
	await _tap(KEY_1)
	await _tap(KEY_1)
	await _tap(KEY_E)
	await _tap(KEY_Q)
	if not await _until("the last run's day ends", func() -> bool: return _int("remaining_ms") == 0 and _str("phase") == "workshop", DAY_FRAMES):
		return
	if not await _until("AI appraises the last run's finds", func() -> bool: return _str("dish_offered") != "" and not _next().contains("identifying"), 300):
		return
	var days: int = 0
	while _str("phase") != "lost" and days < LONG_RUN_DAYS:
		days += 1
		await _tap(KEY_G)
	_check(_str("phase") == "lost" and _str("dish_offered") == "", "a lost party is offered no dish")

func _next() -> String:
	return _str("next")

func _out_of_reach() -> Array:
	return scene.survival_status()["out_of_reach"]

func _changing_text() -> Array:
	return scene.survival_status()["changing_text"]

func _lines(field: String) -> Array:
	return scene.survival_status()[field]

func _result_says(words: String) -> bool:
	for line: String in _lines("day_result"):
		if line.contains(words):
			return true
	return false

func _goal_shown() -> bool:
	return _str("goal") == "Escape checks from day 30" and _screen_says("Escape checks from day 30")

## Whether one changing line reads exactly `words`, as a button's label does.
func _screen_shows(words: String) -> bool:
	for line: Dictionary in _changing_text():
		if line["text"] == words:
			return true
	return false

func _screen_says(words: String) -> bool:
	for line: Dictionary in _changing_text():
		var text: String = line["text"]
		if text.contains(words):
			return true
	return false

## Measures every changing line in the font and weight the screen draws it with.
func _text_fits(where: String) -> bool:
	var font: Font = load(CHANGING_FONT)
	var overflowing: Array[String] = []
	for line: Dictionary in _changing_text():
		var text: String = line["text"]
		var size: int = line["size"]
		var width: float = line["width"]
		if font.get_string_size(text, HORIZONTAL_ALIGNMENT_LEFT, -1, size).x > width:
			overflowing.append(text)
	var measured: bool = font.get_string_size(_next(), HORIZONTAL_ALIGNMENT_LEFT, -1, 20).x > 0.0
	return _check(measured and overflowing.is_empty(), "text fits on %s: %s" % [where, overflowing])

## No word plate on the map covers another; the control is that the plates
## read are the city's: more than a count and a name for each source shown.
func _plates_apart(where: String) -> bool:
	var plates: Array = scene.get_meta("map_plates", [])
	var objects: Dictionary = scene.survival_status()["objects"]
	var sources: int = 0
	for source: String in scene.survival_status()["stock"]:
		if objects.get(source, "dormant") != "dormant":
			sources += 1
	var covered: Array[String] = []
	for first: int in range(plates.size()):
		for second: int in range(first + 1, plates.size()):
			var one: Rect2 = plates[first]
			var other: Rect2 = plates[second]
			if one.intersects(other):
				covered.append("%s over %s" % [one, other])
	return _check(sources > 0 and plates.size() > 2 * sources and covered.is_empty(), "no map plate covers another on %s: %d plates, %d sources, %s" % [where, plates.size(), sources, covered])

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

## Walks the human through each waypoint in turn.
func _walk_route(waypoints: Array) -> bool:
	for waypoint: Vector2i in waypoints:
		if not await _walk_to(waypoint.x, waypoint.y):
			return false
	return true

## Walks the human in a straight line: every leg here runs along one axis.
func _walk_to(x: int, y: int) -> bool:
	var first: int = 4 * _actor(walker, "key_set")
	for frame: int in range(240):
		var at_x: int = _actor(walker, "x")
		var at_y: int = _actor(walker, "y")
		if at_x == x and at_y == y:
			_release()
			return true
		_key(WALK_KEYS[first], x < at_x)
		_key(WALK_KEYS[first + 1], x > at_x)
		_key(WALK_KEYS[first + 2], x == at_x and y < at_y)
		_key(WALK_KEYS[first + 3], x == at_x and y > at_y)
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

## M asks first mid-run and Enter accepts; M leaves a cinematic at once. Enter on
## the title screen takes its marked entry, so it is pressed only to answer the
## question, and the title must show before the next menu key.
func _leave_run() -> void:
	var status: Dictionary = scene.survival_status()
	if status.has("cinematic"):
		await _tap(KEY_M)
	elif not status.is_empty():
		await _tap(KEY_M)
		await _tap(KEY_ENTER)
	await _until("the title screen shows after leaving the run", func() -> bool: return scene.screen_name() == "main" and scene.survival_status().is_empty(), 30)

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
