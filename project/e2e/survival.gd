class_name SurvivalPlayCheck
extends SceneTree
## Plays the survival game through the shipped scene with real key events: full
## days as the chef, a day as the appraiser, and the map and tool changes between;
## and the same commands with real mouse events, alone and for two players.

const SEED: int = 11
const DAY_FRAMES: int = 1200
## How long one collection holds the collecting actor.
const COLLECT_MS: int = 1500
## Frames of a collection's hold, with a step to spare; a wait for AI to come
## home adds one, since AI finishes its collection first.
const COLLECT_FRAMES: int = 102
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
## Half the height a panel rises from as it comes in, in pixels, and a wait on
## the real clock longer than its rise.
const ENTRANCE_RISE: float = 9.0
const SETTLE_MSEC: int = 400
const ROUTE_KEYS: Dictionary[String, Key] = {"left": KEY_A, "right": KEY_D, "up": KEY_W, "down": KEY_S}
## Left, right, up, down and jump of the first key set, then of the second.
const WALK_KEYS: Array[Key] = [KEY_A, KEY_D, KEY_W, KEY_S, KEY_SPACE, KEY_J, KEY_L, KEY_I, KEY_K, KEY_SEMICOLON]
## Eight unfed days raise acid to 96: acid has taken every washed-up bag and
## every landed ingredient source west of the ferry, the kiosk has fallen open
## to the terrace bag, and the run is not lost.
const UNFED_DAYS: int = 8
## From the entrance down the step, over the rubble and the rope bridge to the
## supply crate in the tower shop on the gauge ledge.
const CRATE_ROUTE: Array[Vector2i] = [Vector2i(19, 8)]
## From the crate east to the east loft and up to the east roof, through the
## fallen kiosk and down to the terrace bag on the shop roof.
const TERRACE_ROUTE: Array[Vector2i] = [Vector2i(23, 8), Vector2i(23, 6), Vector2i(21, 6), Vector2i(21, 4), Vector2i(17, 5)]
## From the entrance down the step and over the rubble to the tool bag.
const MARKET_ROUTE: Array[Vector2i] = [Vector2i(13, 9)]
## From the entrance down the step to the tomato machine.
const TOMATO_ROUTE: Array[Vector2i] = [Vector2i(11, 9)]
## From the entrance up the kitchen ladder to the shop shelf on the loft.
const SHELF_ROUTE: Array[Vector2i] = [Vector2i(3, 8), Vector2i(3, 6), Vector2i(4, 6)]
## The drip stands on the upper walkway west of the kitchen ladder: in and back
## out through it burns twice, and the third burn is one step back in.
const DRIP_ROUTE: Array[Vector2i] = [Vector2i(1, 8), Vector2i(2, 8), Vector2i(1, 8), Vector2i(2, 8)]
## Time a walk from the entrance to the roof ladder's foot takes, with room.
const LADDER_WALK_MS: int = 6000
## From the entrance up the kitchen ladder to the foot of the roof ladder on
## the loft.
const LADDER_ROUTE: Array[Vector2i] = [Vector2i(3, 8), Vector2i(3, 6), Vector2i(1, 6)]
## From the entrance up the kitchen ladder and the roof ladder to the roof bag.
const ROOF_ROUTE: Array[Vector2i] = [Vector2i(3, 8), Vector2i(3, 6), Vector2i(1, 6), Vector2i(1, 4), Vector2i(6, 4)]
## The map's top-left corner and size in the scene, as the city draws it; a
## sub-cell is one scene pixel, and the scene is 720 pixels tall.
const MAP_ORIGIN: Vector2 = Vector2(64, 70)
const MAP_SIZE: Vector2 = Vector2(40 * 32, 16 * 32)
const SCENE_HEIGHT: float = 720.0
## The play area between the top bar and the cards, on screen.
const PLAY: Rect2 = Rect2(0, 62, 1280, 478)
## The camera eases on the real clock while each frame moves the game on, so
## a wait for it sleeps this long between frames, and gives up after the
## limit, well past the camera's eased motion.
const SETTLE_POLL_MSEC: int = 20
const SETTLE_LIMIT_MSEC: int = 3000
## A note shows four seconds and fades over one, on the real clock; the wait
## for it to go allows that and more.
const NOTE_FADE_LIMIT_MSEC: int = 8000
## The middle of the play area on screen, where the camera draws what it
## heads for.
const FOCUS: Vector2 = Vector2(640, 301)
## How far ahead of the way a lone human faces the camera looks, in scene
## pixels.
const LOOK_AHEAD: float = 80.0
## How far apart, in scene pixels, two humans stand when the camera must draw
## them smaller than twice the scene's size to frame both.
const APART: float = 480.0
## Where the workshop draws the ingredients waiting for appraisal, over the
## appraiser's head.
const APPRAISER_HEAD: Vector2 = Vector2(216, 166)
## The screen pixels the HUD's panels and chips covered while it named every
## key, measured by `_hud_area` at commit d72f34c55a70c3d212f0eb9b0e51dbb4b2f6712f on the
## same seeded states: day one at base, and the chef at the Vendor Cart, the
## first source the AI partner took on that map; the chef now stands at the
## Shop Shelf, the first source it takes on this one.
const BASE_HUD_AREA_WITH_KEYS: float = 226079.0
const CITY_HUD_AREA_WITH_KEYS: float = 152779.0
## How far left of a volume row's first block its name is drawn.
const VOLUME_NAME_OFFSET: float = 200.0
## The screen's middle height: the kitchen's people stand above it and the
## task card sits below it.
const SCREEN_MIDDLE: float = 360.0
## The face the screen draws every changing line in.
const CHANGING_FONT: String = "res://levels/monster_mouth_escape/fonts/Tiny5-Regular.ttf"
## The face's grid, in pixels per em, which every drawn size is a multiple of.
const FONT_GRID: int = 8
## The smallest size the interface draws a word at.
const MIN_TEXT_SIZE: int = 16
## When the escape checks begin, which the pause screen names.
const ESCAPE_GOAL: String = "Escape checks from day 30"
## How many first-play tips there are, one per concept.
const TIP_COUNT: int = 10
## The screen, where a tip must stand whole.
const SCREEN: Rect2 = Rect2(0, 0, 1280, 720)
## The words of a source's kind the map once wrote beside its supply.
const KIND_WORDS: Array[String] = ["BAG", "VEND", "CART", "SHOP"]
## The map's words that are neither a place's name nor a number, such as a
## count or the health an acid drip takes: a broken passage's mark and the
## actors' tags.
const MAP_MARKS: Array[String] = ["BROKEN", "P1", "P2", "AI"]
## What feeding does, which the recipe book on the pause screen names.
const FEED_RULES: Array[String] = ["A dish reaching the needed score holds the acid back.", "A hated dish, a lower score or no meal raises it.", "Full acid ends the run."]

var scene: MainScene
var held: Dictionary[Key, bool] = {}
var checks: int = 0
## The run's status as last read, and the frame it was read on: building it
## costs more than a frame, and nothing it reports changes before the next
## frame, which is also when the key events sent in this one arrive.
var status_read: Dictionary = {}
var status_frame: int = -1
var failed: bool = false
var walker: String = "chef"
## Plates found over a source other than its own count on any frame walked
## since the last check of them.
var covered_on_the_way: Array[String] = []
var awaiting_quit: bool = false
## The tips the open run has opened, by their line, as the screen drew them;
## the lines a run opened a second time; every line any run opened; and
## whether a run opened a line an earlier run had opened.
var run_tips: Array[String] = []
var repeated_tips: Array[String] = []
var every_tip: Dictionary[String, bool] = {}
var tip_reopened: bool = false
var last_tip: String = ""
## What a drawn tip covered or where it stood off the screen, and how many
## frames drew one at base and over the city's words.
var tip_overlaps: Array[String] = []
var tip_frames: Dictionary[String, int] = {"base": 0, "city": 0}

func _initialize() -> void:
	_run.call_deferred()

## Only the game's own quit reaches here with the pass line owed.
func _finalize() -> void:
	if awaiting_quit and not failed:
		print("Survival: ", checks, " live checks passed (menus, days, return, appraisal, cooking, dish choice, feeding, map, tools, roles, pause, leaving, hand-off, mouse, quit).")

func _run() -> void:
	await _frames(2)
	scene = current_scene as MainScene
	process_frame.connect(_watch_tips)
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
		await _two_players_share_the_mouse()
	if not failed:
		await _a_hated_dish_is_not_fed_again()
	if not failed:
		await _a_hated_dish_alone_sets_out_then_ends_the_day()
	if not failed:
		await _player_two_errand_alone_sets_out()
	if not failed:
		await _mouse_in_the_city()
	if not failed:
		await _pause_and_leave()
	if not failed:
		await _hand_off_shows_the_keys_left()
	if not failed:
		await _loss_clears_the_offer()
	if not failed:
		await _repeated_note_appears()
	if not failed:
		await _main_menu_quits()

## At base the chef's commands by mouse: the dish line chooses as S does, the
## feed line feeds as F does, the task card's step and the chef in the kitchen
## each cook as E does, a refused dish line presses nothing, and the end of the
## day ends it as G does.
func _mouse_works_the_base() -> void:
	var offered: String = _str("dish_offered")
	var dish: Dictionary = _target("dish_down")
	if not _check(not dish.is_empty() and str(dish["refusal"]) == "", "the dish line takes a click with more than one dish to cook"):
		return
	if not _check(await _click("dish_down") and _str("dish_offered") != offered and _str("dish_offered") != "", "a click on the dish line chooses the next dish, as S does"):
		return
	_point_at(_target("chef_feed"))
	await _frames(4)
	if not _check(_hovered() == "chef_feed", "the feed line lights under the mouse: %s" % [_presses()]):
		return
	var day: int = _int("day")
	if not _check(await _click("chef_feed"), "the feed line takes a click"):
		return
	if not await _until("a click on the feed line feeds and ends the day, as F does", func() -> bool: return _int("day") == day + 1 and _str("reaction") != "unfed", 30):
		return
	if not await _a_click_on_the_chef_cooks():
		return
	if not await _a_refused_click_presses_nothing():
		return
	day = _int("day")
	if not _check(await _click("chef_end_day"), "the end of the day takes a click"):
		return
	await _until("a click on the end of the day ends it unfed, as G does", func() -> bool: return _int("day") == day + 1 and _str("reaction") == "unfed", 30)

## The base as the screen offers it, for a failing check's message.
func _base_state() -> String:
	var parts: Array[String] = []
	for target: Dictionary in scene.get_meta("pointer_targets", []):
		parts.append("%s@%s[%s]" % [target["press"], target["rect"], target["refusal"]])
	return "offered=%s chosen=%s foods=%s next=%s targets=%s" % [_str("dish_offered"), _str("dish_chosen"), _stored(), _next(), parts]

## A click on the task card's step, when the step is the chef's cooking, cooks
## the dish it names, as E does.
func _a_click_on_the_step_cooks() -> bool:
	var step: Dictionary = _target_where("chef_interact", func(area: Rect2) -> bool: return area.position.y > SCREEN_MIDDLE)
	if not _check(not step.is_empty() and str(step["refusal"]) == "" and _next().contains("Cook"), "setup: the task card's step is the chef's cooking: %s" % [_base_state()]):
		return false
	var chosen: String = _str("dish_offered")
	var foods: int = _int("foods")
	return _check(await _click_on(step) and ((_int("foods") == foods + 1 and _next_event_has(chosen + " cooked.")) or _next_event_has(chosen + " burned")), "a click on the task card's step cooks the offered dish, as E does: %s" % [_base_state()])

## A click on a refused target presses nothing: with one dish left to cook the
## dish line refuses and the choice stays unset, while S, the press the line
## stands for, sets it.
func _a_refused_click_presses_nothing() -> bool:
	var attempts: int = 0
	while str(_target("dish_down").get("refusal", "")) == "" and _str("dish_offered") != "" and attempts < COOK_ATTEMPTS:
		attempts += 1
		await _tap(KEY_E)
	var line: Dictionary = _target("dish_down")
	if not _check(not line.is_empty() and str(line["refusal"]) != "" and _str("dish_offered") != "" and _str("dish_chosen") == "", "setup: the dish line refuses with one dish to cook and none chosen: %s" % [_base_state()]):
		return false
	var clicked: bool = await _click_on(line)
	await _frames(12)
	var after_click: String = _str("dish_chosen")
	await _tap(KEY_S)
	await _frames(12)
	return _check(clicked and after_click == "" and _str("dish_chosen") != "" and _str("dish_chosen") == _str("dish_offered"), "a click on a refused target presses nothing, while S, its press by key, chooses the dish: chosen by click '%s', by key '%s'" % [after_click, _str("dish_chosen")])

## A click on the chef standing in the kitchen cooks the offered dish, as E does.
func _a_click_on_the_chef_cooks() -> bool:
	var chef: Dictionary = _target_where("chef_interact", func(area: Rect2) -> bool: return area.end.y < SCREEN_MIDDLE)
	if not _check(not chef.is_empty() and str(chef["refusal"]) == "" and _str("dish_offered") != "", "setup: the chef in the kitchen can cook: %s" % [_base_state()]):
		return false
	var chosen: String = _str("dish_offered")
	var foods: int = _int("foods")
	return _check(await _click_on(chef) and ((_int("foods") == foods + 1 and _next_event_has(chosen + " cooked.")) or _next_event_has(chosen + " burned")), "a click on the chef in the kitchen cooks the offered dish, as E does: %s" % [_base_state()])

## In the city by mouse: the set-out step sets out, a button with nothing in
## reach says why, the action button collects, the pause chip pauses, the pause
## screen's blocks, leave and cancel answer clicks, and the travel key returns.
func _mouse_in_the_city() -> void:
	await _leave_run()
	await _tap(KEY_1)
	await _tap(KEY_1)
	await _tap(KEY_E)
	walker = "chef"
	if not _check(await _click("party_travel"), "the set-out step takes a click"):
		return
	if not await _until("a click on the set-out step sets out, as Q does", func() -> bool: return _where("chef") == "city", 30):
		return
	var nothing: Dictionary = _target("chef_interact")
	_point_at(nothing)
	await _frames(4)
	var refusal: String = str(nothing.get("refusal", ""))
	if not _check(refusal != "" and _drawn(refusal), "a button with nothing in reach says why under the mouse: %s" % refusal):
		return
	if not await _walk_route(CRATE_ROUTE):
		return
	var tools: int = _actor("chef", "tools")
	var ingredients: int = _actor("chef", "ingredients")
	if not _check(await _click("chef_interact") and (_actor("chef", "collecting_ms") > 0 or _actor("chef", "tools") > tools or _actor("chef", "ingredients") > ingredients), "a click on the action button collects, as E does"):
		return
	await _until("the collection ends", func() -> bool: return _actor("chef", "collecting_ms") == 0, COLLECT_FRAMES)
	if not _check(await _click("pause") and _str("interruption") == "paused", "a click on the pause chip pauses, as Esc does"):
		return
	var effects: float = scene.sound_status()["effects_level"]
	var music: float = scene.sound_status()["music_level"]
	var clicked: bool = await _click("volume_effects_3")
	var effects_now: float = scene.sound_status()["effects_level"]
	var music_now: float = scene.sound_status()["music_level"]
	if not _check(clicked and _drawn("30%") and effects_now < effects and music_now == music, "a click on an effects block sets the effects to that level alone"):
		return
	# The row's name lies left of its first block, where the row is drawn.
	var first_block: Rect2 = _target("volume_effects_1").get("rect", Rect2())
	if not _check(first_block.has_area(), "setup: the pause screen draws the effects row's first block"):
		return
	await _click_at(Vector2(first_block.position.x - VOLUME_NAME_OFFSET, first_block.get_center().y))
	var effects_after: float = scene.sound_status()["effects_level"]
	if not _check(effects_after == effects_now and _drawn("30%"), "a click on a volume row's name leaves its level as it was"):
		return
	if not _check(await _click("menu") and _str("interruption") == "confirm_leave", "a click on leave asks first, as M does"):
		return
	if not _check(await _click("pause") and _str("interruption") == "paused", "a click on cancel goes back to the pause"):
		return
	if not _check(await _click("pause") and _str("interruption") != "paused", "a click on continue resumes"):
		return
	if not await _until("the run continues", func() -> bool: return _str("interruption") == "", 30):
		return
	if not _check(await _click("chef_travel"), "the travel key takes a click"):
		return
	await _until("a click on the travel key returns to base, as Q does", func() -> bool: return _where("chef") == "base", 30)

## Two players share one mouse: the set-out step sets the party out, each human
## in the city has their own action button, and a click on one human's travel
## key moves that human alone.
func _two_players_share_the_mouse() -> void:
	await _start_two_players()
	if not _check(await _click("party_travel"), "the set-out step takes a click with two players"):
		return
	if not await _until("one click sets the whole party out", func() -> bool: return _where("chef") == "city" and _where("appraiser") == "city", 30):
		return
	if not _check(not _target("chef_interact").is_empty() and not _target("appraiser_interact").is_empty(), "each human in the city has their own action button"):
		return
	if not _check(await _click("appraiser_travel"), "player two's travel key takes a click"):
		return
	if not await _until("a click on player two's travel key moves only the appraiser", func() -> bool: return _where("appraiser") == "base" and _where("chef") == "city", 30):
		return
	await _tap(KEY_Q)
	await _until("player one comes back", func() -> bool: return _str("phase") == "workshop", 30)

## Real keys: Esc, M and Enter over a run, and over a cinematic.
func _pause_and_leave() -> void:
	await _leave_run()
	await _tap(KEY_1)
	await _tap(KEY_1)
	await _tap(KEY_ESCAPE)
	if not _check(_str("cinematic") == "intro" and _str("interruption") == "", "Esc does not pause a cinematic"):
		return
	await _tap(KEY_M)
	if not _check(scene.screen_name() == "main" and _status().is_empty(), "M during a cinematic leaves at once"):
		return
	await _tap(KEY_1)
	await _tap(KEY_1)
	await _tap(KEY_E)
	await _tap(KEY_Q)
	if not await _until("the chef sets out", func() -> bool: return _where("chef") == "city", 30):
		return
	if not _check(_drawn("Pause, keys"), "the top band offers the pause, where the keys are"):
		return
	var unpaused_rules: bool = FEED_RULES.any(func(rule: String) -> bool: return _drawn(rule))
	var rise: float = await _pause_rise()
	if not _check(_str("interruption") == "paused", "Esc pauses the run"):
		return
	if not _check(rise >= ENTRANCE_RISE, "the pause screen rises into place as it opens: %.1f pixels" % rise):
		return
	if not _check(not unpaused_rules and FEED_RULES.all(func(rule: String) -> bool: return _drawn(rule)), "the recipe book on the pause screen, and only there, says what feeding does"):
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
	rise = await _pause_rise()
	if not _check(rise >= ENTRANCE_RISE, "the pause screen, gone for a frame, rises afresh as it opens again: %.1f pixels" % rise):
		return
	await _tap(KEY_ESCAPE)
	if not _check(_str("interruption") == "", "Esc continues the run again"):
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
	if not _check(scene.screen_name() == "main" and _status().is_empty(), "Enter ends the run for the main menu"):
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
	for row: String in ["Run: Shift / H", "Jump: Space / ;", "Hand your actor to AI: X / N", "Swap actor, once one player is left: Tab / none", "Move: WASD or arrows / IJKL"]:
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
	if not _check(_str("hand_off").contains("Appraiser") and _str("hand_off").contains("WASD move") and not _str("hand_off").contains("Tab"), "the screen names the movement keys of the player left, and no other key: %s" % _str("hand_off")):
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

## A line the log repeats once the same line has dropped out of it shows as a
## new note: the chef sets out and returns with the AI appraiser, which fills
## the log, and sets out again after the first note has faded.
func _repeated_note_appears() -> void:
	await _leave_run()
	await _tap(KEY_1)
	await _tap(KEY_1)
	await _tap(KEY_E)
	await _tap(KEY_Q)
	if not await _until("the party sets out", func() -> bool: return _where("chef") == "city" and _where("appraiser") == "city", 60):
		return
	if not _check(_drawn("Chef set out."), "a new note shows: %s" % [_drawn_lines()]):
		return
	await _tap(KEY_Q)
	if not await _until("the party comes home", func() -> bool: return _str("phase") == "workshop", COLLECT_FRAMES + 120):
		return
	# The pause holds the run while the notes go on fading on the real clock.
	await _tap(KEY_ESCAPE)
	var start: int = Time.get_ticks_msec()
	while _drawn("Chef set out.") and Time.get_ticks_msec() - start < NOTE_FADE_LIMIT_MSEC:
		await _frames(1)
	var faded: bool = not _drawn("Chef set out.")
	await _tap(KEY_ESCAPE)
	if not _check(faded, "a note fades a few seconds after it appears"):
		return
	var log: Array = _events()
	if not _check(log.size() == 4 and str(log[0]) == "Chef set out." and log.count("Chef set out.") == 1, "the log is full with the first note's line oldest: %s" % [log]):
		return
	await _tap(KEY_Q)
	if not await _until("the chef sets out again", func() -> bool: return _where("chef") == "city", 30):
		return
	_check(_events().count("Chef set out.") == 1 and _drawn("Chef set out."), "the repeated line shows as a new note after the first dropped out: %s %s" % [_events(), _drawn_lines()])

func _start_two_players() -> void:
	await _leave_run()
	await _tap(KEY_1)
	await _tap(KEY_3)
	await _tap(KEY_E)

## 4 ends the application: the pass line prints from `_finalize` only.
func _main_menu_quits() -> void:
	await _leave_run()
	if not _check(scene.screen_name() == "main", "back at the main menu"):
		return
	if not _tips_opened_once_each():
		return
	awaiting_quit = true
	await _tap(KEY_4)
	await _frames(30)
	_check(false, "4 on the main menu quits")

## Reads the tip the last frame drew, every frame: which opened, and whether,
## while the run was not held, it stood whole on the screen clear of the HUD's
## panels, every target and, over the city, the map's words. A run's tips
## start afresh off its screen.
func _watch_tips() -> void:
	if scene == null or scene.screen_name() != "survival":
		run_tips.clear()
		last_tip = ""
		return
	var tip: Dictionary = scene.get_meta("hud_tip", {})
	var text: String = str(tip.get("text", ""))
	if text != "" and text != last_tip:
		if run_tips.has(text):
			repeated_tips.append(text)
		else:
			run_tips.append(text)
			tip_reopened = tip_reopened or every_tip.has(text)
			every_tip[text] = true
	last_tip = text
	# A pause or a question holds the screen under a veil, tip and all, and
	# draws its own targets over it.
	if text == "" or tip.get("held", false):
		return
	var area: Rect2 = tip["rect"]
	var panels: Array = scene.get_meta("hud_panels", [])
	var covered: Array = panels.duplicate()
	for target: Dictionary in scene.get_meta("pointer_targets", []):
		covered.append(target["rect"])
	var city: bool = tip["city"]
	var plates: Array = []
	if city:
		plates = scene.get_meta("map_plates", [])
	covered.append_array(plates)
	for other: Rect2 in covered:
		if other.intersects(area):
			tip_overlaps.append("%s over %s" % [text, other])
	if not SCREEN.encloses(area):
		tip_overlaps.append("%s off the screen at %s" % [text, area])
	if city and not plates.is_empty():
		tip_frames["city"] += 1
	elif not city:
		tip_frames["base"] += 1

## Every concept's tip opened, none twice in one run, a later run opened one
## again, and no drawn tip covered the HUD, a target or the map's words; the
## control is that tips were drawn at base and over the city's words.
func _tips_opened_once_each() -> bool:
	if not _check(every_tip.size() == TIP_COUNT, "every concept's tip opened: %s" % [every_tip.keys()]):
		return false
	if not _check(repeated_tips.is_empty() and tip_reopened, "no tip opened twice in one run, and a new run opened one again: %s" % [repeated_tips]):
		return false
	var keyed: Array[String] = []
	for line: String in every_tip:
		if line.contains("[") or line.contains("Esc") or line.contains("Enter"):
			keyed.append(line)
	if not _check(keyed.is_empty(), "the tips name actions and not keys: %s" % [keyed]):
		return false
	return _check(tip_overlaps.is_empty() and tip_frames["base"] > 0 and tip_frames["city"] > 0, "a tip covers no HUD panel, target or map word and stays on the screen: %s in %s" % [tip_overlaps.slice(0, 5), tip_frames])

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
	if not _names_no_action_key("the base"):
		return
	if not _check(_hud_area() < BASE_HUD_AREA_WITH_KEYS, "the base's HUD covers less than it did naming every key: %.0f against %.0f" % [_hud_area(), BASE_HUD_AREA_WITH_KEYS]):
		return
	if not _check(await _pause_shows_escape_goal(), "the pause screen over the base, and not the base, shows when escape checks begin"):
		return
	if not await _a_click_closes_the_tip():
		return

## The run's first tip says to set out; a click on it closes it and presses
## nothing, so the run stays at base on day one with its time.
func _a_click_closes_the_tip() -> bool:
	var tip: Dictionary = scene.get_meta("hud_tip", {})
	if not _check(_str("tip") == "set_out" and str(tip.get("text", "")).begins_with("Set out"), "the first tip at base says to set out: %s" % [tip]):
		return false
	var remaining: int = _int("remaining_ms")
	var area: Rect2 = tip["rect"]
	await _click_at(area.get_center())
	var drawn: Dictionary = scene.get_meta("hud_tip", {})
	return _check(_str("tip") == "" and drawn.is_empty() and _int("day") == 1 and _str("phase") == "workshop" and _int("remaining_ms") == remaining, "a click on the tip closes it and presses nothing")

func _first_day() -> void:
	var start: Dictionary = _status()
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
	if not _check(_out_of_reach().has("Terrace Bag") and not _out_of_reach().has("Tomato Machine"), "only stock behind the kiosk is marked out of reach"):
		return
	if not _text_fits("the first city screen"):
		return
	if not _every_place_measured():
		return
	if not _plates_apart("the first city screen"):
		return
	if not _plates_clear_of_sources("the first city screen"):
		return
	if not await _camera_holds("the first city screen"):
		return
	if not _points_to_goal("the first city screen"):
		return
	if not _check(await _pause_shows_escape_goal(), "the pause screen over the city, and not the city, shows when escape checks begin"):
		return
	if not _check(_actions() == ["USE"] and _drawn("USE") and _drawn("WASD move  Shift run  Space jump  To base"), "away from a source the action button reads use, and the move, run and jump keys show beside the way to base: %s" % [_actions()]):
		return
	if not _names_no_action_key("the first city screen"):
		return
	await _run_east()
	if failed:
		return
	if not await _walk_route(SHELF_ROUTE):
		return
	if not await _until("the AI appraiser takes the shelf's kelp", func() -> bool: return _actor("appraiser", "ingredients") >= 1, 900):
		return
	var shelf: String = "COLLECT " + _place("Shop Shelf")
	if not _check(_actions() == [shelf] and _drawn(shelf), "at a source the action button offers to collect from it by its name: %s" % [_actions()]):
		return
	if not _check(_hud_area() < CITY_HUD_AREA_WITH_KEYS, "the city's HUD at a source covers less than it did naming every key: %.0f against %.0f" % [_hud_area(), CITY_HUD_AREA_WITH_KEYS]):
		return
	if not _check(not _next().begins_with("Collect from ") and _next().contains(" to Tomato machine"), "the screen moves past a kind the party already took: %s" % _next()):
		return
	if not await _walk_route([Vector2i(3, 6), Vector2i(3, 8)] + TOMATO_ROUTE):
		return
	var stock: int = _stock("Tomato Machine")
	await _tap(KEY_E)
	if not _check(_actor("chef", "ingredients") == 1 and _stock("Tomato Machine") < stock, "E collects into the pack and spends shared stock"):
		return
	var remaining: int = _int("remaining_ms")
	if not await _until("time expires", func() -> bool: return _str("phase") == "workshop", DAY_FRAMES):
		return
	if not _check(remaining > 0 and _int("remaining_ms") == 0, "the day's clock ran out in the city"):
		return
	if not _check(_where("chef") == "base" and _where("appraiser") == "base", "expiry returns both actors"):
		return
	_check(_events().has("Time is up: everyone is back at base."), "the screen reports the timed return")

## Runs east along the upper walkway towards its step down.
func _run_east() -> void:
	var stamina: int = _actor("chef", "stamina")
	_key(KEY_SHIFT, true)
	_key(KEY_D, true)
	await _until("running reaches the step down", func() -> bool: return _actor("chef", "x") >= 8, 240)
	_key(KEY_D, false)
	_key(KEY_SHIFT, false)
	if failed:
		return
	if not _check(_actor("chef", "stamina") < stamina, "running spends stamina"):
		return
	var line: String = _next()
	var target: String = line.get_slice(" to ", 1).get_slice(".", 0)
	var places: Dictionary = _status()["places"]
	_check(line.contains(" to ") and places.values().has(target), "the screen names the object to move to: %s" % line)

## Stops before anything is collected, so the fall leaves time on the clock.
func _third_day_falls() -> void:
	await _tap(KEY_Q)
	if not await _until("chef sets out to the drip", func() -> bool: return _where("chef") == "city", 30):
		return
	if not await _walk_route([Vector2i(1, 8)]):
		return
	if not _check(_actor("chef", "health") == 2 and _next_event_has("burned"), "the acid drip costs one health slot"):
		return
	if not await _camera_settles("the drip"):
		return
	var drip: Dictionary = _backdrop()
	if not await _walk_to(24, 8):
		return
	if not await _camera_ignores_partner("the east walkway on day three"):
		return
	if not _backdrop_slides(drip, _backdrop(), "from the drip to the east walkway"):
		return
	if not _plates_clear_of_sources("the east walkway on day three"):
		return
	if not _points_to_goal("the east walkway on day three"):
		return
	if not _looks_ahead("chef", 1.0, "walking east onto the east walkway"):
		return
	await _tap(KEY_A)
	if not await _camera_settles("the east walkway facing west"):
		return
	if not _looks_ahead("chef", -1.0, "turning west on the east walkway"):
		return
	var remaining: int = _int("remaining_ms")
	var pack: int = _actor("chef", "ingredients")
	if not await _walk_to(18, 12):
		return
	_key(KEY_A, true)
	var off: bool = await _until("chef walks off the shallows' west end", func() -> bool: return _stance("chef") == "airborne", 120)
	_key(KEY_A, false)
	if not off:
		return
	if not await _until("chef falls into the acid", func() -> bool: return _where("chef") == "base", 120):
		return
	if not _check(_actor("chef", "health") == 3 and _actor("chef", "ingredients") == pack, "a fall returns to base with health restored and the pack kept"):
		return
	if not _check(_int("remaining_ms") > 0 and _int("remaining_ms") <= remaining, "a fall keeps the remaining time"):
		return
	if not _check(_next_event_has("fell"), "the screen reports the fall"):
		return
	if not await _until("AI comes back to base", func() -> bool: return _str("phase") == "workshop", COLLECT_FRAMES + 120):
		return
	var left: int = _int("remaining_ms")
	await _frames(30)
	if not _check(_int("remaining_ms") == left and left > 0, "a return with time left keeps the rest of the day"):
		return
	if not await _until("AI appraises before the chef goes out again", func() -> bool: return not _next().contains("identifying"), 300):
		return
	if not _check(_next().contains("Set out") or _names_interact(_next()), "with time left the screen offers more to do: %s" % _next()):
		return
	await _tap(KEY_Q)
	if not await _until("chef goes out again with time left", func() -> bool: return _where("chef") == "city", 30):
		return
	await _tap(KEY_Q)
	if not await _until("the party is home again", func() -> bool: return _str("phase") == "workshop", COLLECT_FRAMES + 120):
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
	if not _plates_clear_of_sources("the damaged city at acid %d" % _int("acid")):
		return
	if not await _camera_holds("the damaged city"):
		return
	if not _points_to_goal("the damaged city"):
		return
	if not _gone_bags_leave_the_map():
		return
	if not await _walk_route(DRIP_ROUTE):
		return
	if not _check(_actor("chef", "health") == 1, "two more drips leave one slot"):
		return
	_key(KEY_A, true)
	var exhausted: bool = await _until("the third drip", func() -> bool: return _actor("chef", "exhaustions") == 1, 60)
	_key(KEY_A, false)
	if not exhausted:
		return
	if not _check(_where("chef") == "base" and _actor("chef", "health") == 3, "exhaustion carries the chef to base at full health"):
		return
	if not _check(_spatula() == "damaged", "exhaustion damages the chef's spatula"):
		return
	await _until("AI comes back after the exhaustion", func() -> bool: return _str("phase") == "workshop", COLLECT_FRAMES + 120)

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
	# A collection holds AI for its time, so give it time for a dish's worth.
	if not await _until("AI collects a dish's worth", func() -> bool: return _actor("appraiser", "ingredients") >= 2, DAY_FRAMES):
		return
	if not await _walk_route(DRIP_ROUTE):
		return
	_key(KEY_A, true)
	var exhausted: bool = await _until("the chef is exhausted on day one", func() -> bool: return _actor("chef", "exhaustions") == 1, 60)
	_key(KEY_A, false)
	if not exhausted:
		return
	if not await _until("AI comes back after the chef's exhaustion", func() -> bool: return _str("phase") == "workshop", COLLECT_FRAMES + 120):
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
	_check(spatula == "damaged" and line.begins_with("Feed "), "a dish to feed comes before the AI chef replacing the spatula: %s" % line)

func _workshop_day_one() -> void:
	if not await _until("AI appraises the finds", func() -> bool: return _int("identified") >= 2 and not _next().contains("identifying"), 300):
		return
	if not _check(_int("remaining_ms") == 0 and _int("day") == 1, "the workshop runs with no clock and stays on the same day"):
		return
	await _cook()
	if failed:
		return
	if not _check(_next().begins_with("Feed "), "the screen offers feeding the stored dish: %s" % _next()):
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
	if not _check(_result_says("Day 1: " + fed) and _int("remaining_ms") == 18000, "the result stands on screen and spends no time"):
		return
	if not _text_fits("the day's result"):
		return
	if not _check(_str("reaction") == "satisfied" and _int("score") >= 100, "a new dish satisfies the monster"):
		return
	if not _check(_int("acid") == 0 and _int("eaten") == 1 and _int("foods") == 0, "a satisfied monster raises no acid and the dish is gone"):
		return
	_check(_int("remaining_ms") == 18000, "the new day restores the exploration budget")

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
	if not _check(_int("remaining_ms") == 0 and _actor("chef", "ingredients") + _actor("appraiser", "ingredients") > pack, "the day runs out with the finds in the pack"):
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
	# Pressed on the pause's own frame, so no step boundary can take the press
	# before the pause opens.
	_key(KEY_S, true)
	_key(KEY_ESCAPE, true)
	await _frames(2)
	_key(KEY_S, false)
	_key(KEY_ESCAPE, false)
	await _frames(8)
	if not _check(_str("interruption") == "paused", "Esc opens the pause over the dish press"):
		return
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
	if not _check(_stored().has(fallback), "the key cooks the dish it fell back to"):
		return
	await _mouse_works_the_base()

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
	if not await _until("the party comes back early", func() -> bool: return _str("phase") == "workshop", COLLECT_FRAMES + 120):
		return
	if not await _until("AI appraises before the chef cooks", func() -> bool: return _int("identified") >= 2 and not _next().contains("identifying"), 300):
		return
	await _cook()
	if failed:
		return
	if not _check(_int("remaining_ms") > 0 and _next().contains("Set out"), "with time and stock left the screen sets out before feeding"):
		return
	if not _check(_drawn_says("Feed " + str(_stored()[0]) + ": this ends the day."), "the task card keeps feeding the stored dish in view: %s" % [_drawn_lines()]):
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
		if _int("day") != day and line.begins_with("Feed ") and _str("reaction") == "disliked":
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
	# A second player keeps the appraiser home, so the chef alone empties the
	# roof bag before acid breaks its ladder.
	await _start_two_players()
	await _tap(KEY_Q)
	if not await _until("the chef sets out alone for the roof bag", func() -> bool: return _where("chef") == "city" and _where("appraiser") == "base", 30):
		return
	if not await _walk_route(ROOF_ROUTE):
		return
	await _tap(KEY_E)
	if not await _until("the chef's first roof bag collection ends", func() -> bool: return _actor("chef", "collecting_ms") == 0, COLLECT_FRAMES):
		return
	await _tap(KEY_E)
	if not _check(_stock("Roof Bag") == 0 and _object("Roof Ladder") == "intact", "the roof bag is emptied while its ladder stands"):
		return
	if not await _until("the chef's second roof bag collection ends", func() -> bool: return _actor("chef", "collecting_ms") == 0, COLLECT_FRAMES):
		return
	await _tap(KEY_Q)
	if not await _until("the chef comes home from the roof", func() -> bool: return _str("phase") == "workshop", COLLECT_FRAMES + 120):
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
	if not _plates_clear_of_sources("the city at acid %d" % _int("acid")):
		return
	if not await _camera_holds("the city at acid %d" % _int("acid")):
		return
	if not _points_to_goal("the city at acid %d" % _int("acid")):
		return
	if not await _walk_route(LADDER_ROUTE):
		return
	if not _check(_next().contains("Nothing is within reach"), "beside the dead passage the line names no repair"):
		return
	await _tap(KEY_E)
	_check(_object("Roof Ladder") == "intact", "the key still repairs what the line leaves out")

## Follows NEXT, but ends each day unfed where it names feeding, until it names
## ending the day with LADDER_WALK_MS left: nothing in reach is left once the
## rising acid has taken what the party did not empty. A line that sends the
## player in circles spends the budget of steps a day takes.
func _gather_unfed_until_nothing_is_left() -> bool:
	var day: int = _int("day")
	var steps: int = 0
	while not ((_next().contains("End the day unfed") or _next().begins_with("Feed ")) and _int("remaining_ms") >= LADDER_WALK_MS):
		if _str("phase") == "escaped" or _str("phase") == "lost" or _int("day") >= LONG_RUN_DAYS:
			return _check(false, "the stock runs out before the run ends")
		if _next().begins_with("Feed "):
			await _tap(KEY_G)
		else:
			await _follow_next()
		steps = 0 if _int("day") != day else steps + 1
		day = _int("day")
		if steps >= DAY_STEPS:
			return _check(false, "following NEXT keeps the days moving")
	return true

## Whether NEXT names what the interact key does: collect, repair, appraise
## or cook, as the controls page lists for it.
func _names_interact(line: String) -> bool:
	for verb: String in ["Collect from ", "Repair ", "Appraise ", "Cook "]:
		if line.begins_with(verb):
			return true
	return false

## Presses the key for the one action NEXT names, as the controls page lists
## it, or waits while it names none.
func _follow_next() -> void:
	var line: String = _next()
	if line.contains("Set out") or line.contains("Nothing is within reach"):
		await _tap(KEY_Q)
	elif line.begins_with("Feed "):
		await _tap(KEY_F)
	elif line.begins_with("Replace "):
		await _tap(KEY_R)
	elif line.begins_with("End the day"):
		await _tap(KEY_G)
	elif _names_interact(line):
		await _tap(KEY_E)
	elif line.begins_with("Walk ") or line.begins_with("Climb ") or line.begins_with("Jump "):
		await _walk_a_cell(line)
	else:
		await _frames(12)

## Moves the way NEXT names: a press and release in one frame walks or climbs
## one step at any frame length. A jump, or a step off an edge, holds its way
## until it lands, as the route it was planned on does.
func _walk_a_cell(line: String) -> void:
	var code: Key = ROUTE_KEYS.get(line.get_slice(" ", 1), KEY_NONE)
	if code == KEY_NONE:
		await _frames(STEP_FRAMES)
		return
	if line.begins_with("Jump "):
		_key(code, true)
		_key(KEY_SPACE, true)
		_key(KEY_SPACE, false)
		await _until("the jump takes off", func() -> bool: return _stance(walker) == "airborne", 30)
	else:
		_key(code, true)
		_key(code, false)
		await _frames(STEP_FRAMES)
		if _stance(walker) != "airborne":
			return
		_key(code, true)
	await _until("the move lands", func() -> bool: return _stance(walker) != "airborne", 120)
	_key(code, false)

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
	if not _check(_object("Washed-up Bag 10") == "dormant", "acid brings no bag"):
		return
	if not _check(_int("elapsed_ms") == 54000 and _object("Washed-up Bag 7") == "intact", "every finished day passes its whole time and lands the bag"):
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
	if not await _walk_route(TOMATO_ROUTE):
		return
	# Let go of E held since setting out before pressing it at the machine.
	await _frames(2 * STEP_FRAMES)
	var pack: int = _actor("chef", "ingredients")
	var left: int = _int("remaining_ms")
	await _tap(KEY_E)
	if not _check(_actor("chef", "ingredients") == pack + 1, "the chef collects from a source acid has not reached"):
		return
	if not _check(_actor("chef", "collecting_ms") > 0 and left - _int("remaining_ms") < COLLECT_MS, "a collection holds the chef and leaves the shared clock running"):
		return
	if not _check(_next().begins_with("Collecting from Tomato machine: ") and _next().ends_with("s left."), "the line shows how long the hold has left"):
		return
	await _tap(KEY_Q)
	if not _check(_where("chef") == "city", "a held chef cannot return to base"):
		return
	if not await _until("the chef's collection ends", func() -> bool: return _actor("chef", "collecting_ms") == 0, COLLECT_FRAMES):
		return
	await _tap(KEY_Q)
	await _until("the party is home from the machine", func() -> bool: return _str("phase") == "workshop", COLLECT_FRAMES + 120)

func _appraiser_run() -> void:
	await _leave_run()
	if not _check(scene.screen_name() == "main" and _status().is_empty(), "M ends the run"):
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
	if not _check(_next().contains("Wrench bag") and _next().contains("(takes 1.5s)"), "the screen offers the wrench in reach and its cost in time"):
		return
	await _tap(KEY_E)
	if not _check(_actor("appraiser", "tools") == 1, "the human collects a tool"):
		return
	if not await _until("AI collects", func() -> bool: return _actor("chef", "ingredients") >= 2, DAY_FRAMES):
		return
	if not await _until("the appraiser's tool collection ends", func() -> bool: return _actor("appraiser", "collecting_ms") == 0, COLLECT_FRAMES):
		return
	await _tap(KEY_Q)
	if not await _until("the party is back at base", func() -> bool: return _str("phase") == "workshop", COLLECT_FRAMES + 120):
		return
	if not _check(_next().contains("Appraise") and _str("appraise_job").begins_with("Appraise "), "the human appraiser is told to appraise"):
		return
	var by_mouse: int = _int("identified")
	var failed_before: bool = _next_event_has("appraisal failed")
	if not _check(await _click_on(_target_at(APPRAISER_HEAD)) and (_int("identified") == by_mouse + 1 or (not failed_before and _next_event_has("appraisal failed"))), "a click on the ingredients over the appraiser's head appraises, as E does"):
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
	if not _check(_next().contains("Repair Roof Ladder") and _actions().has("REPAIR " + _place("Roof Ladder")) and _drawn("REPAIR " + _place("Roof Ladder")), "the screen offers the repair in reach and names what it repairs: %s" % [_actions()]):
		return
	_key(KEY_W, true)
	await _frames(24)
	_key(KEY_W, false)
	await _frames(12)
	if not _check(_actor("appraiser", "y") == 6 and _stance("appraiser") == "standing", "a broken ladder blocks the way up"):
		return
	await _tap(KEY_E)
	if not _check(_object("Roof Ladder") == "intact" and _actor("appraiser", "tools") == 1, "E repairs with the carried wrench and keeps it"):
		return
	_key(KEY_W, true)
	await _until("the repaired ladder carries the human up", func() -> bool: return _actor("appraiser", "y") <= 4, 60)
	_key(KEY_W, false)

## The second key set's own keys, which the timing checks inject past.
func _two_players() -> void:
	await _start_two_players()
	if not _check(_actor("chef", "key_set") == 0 and _actor("appraiser", "key_set") == 1, "two players take one key set each"):
		return
	if not _check(_next().begins_with("Set out into the city"), "the base line tells two players to set out: %s" % _next()):
		return
	if not _text_fits("two players at base before setting out"):
		return
	await _tap(KEY_Q)
	if not await _until("player one sets out alone", func() -> bool: return _where("chef") == "city", 30):
		return
	if not _check(_next().contains("P1: Walk ") and _next().contains("P2: Set out into the city"), "the player left at base is told to set out: %s" % _next()):
		return
	if not _check(_drawn("IJKL move  H run  ; jump  Set out") and not _drawn("IJKL move  H run  ; jump  To base") and _drawn("WASD move  Shift run  Space jump  To base"), "each player's keys name their own jump and U as setting out for player two at base: %s" % [_drawn_lines()]):
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
	if not _check(_drawn("WASD move  Shift run  Space jump  To base") and _drawn("IJKL move  H run  ; jump  To base"), "each player's row names that key set's move, run and jump keys and the way to base: %s" % [_drawn_lines()]):
		return
	if not _plates_apart("two players in the city"):
		return
	if not _plates_clear_of_sources("two players in the city"):
		return
	if not await _camera_holds("two players in the city"):
		return
	# Player two climbs to the kitchen roof, rows above player one, so the
	# camera has to draw both smaller to frame them.
	var leading: String = walker
	walker = "appraiser"
	if not await _walk_route(ROOF_ROUTE):
		return
	walker = leading
	if not _check(absf(_feet("chef").y - _feet("appraiser").y) >= APART / 4.0, "the two players stand apart: %s and %s" % [_feet("chef"), _feet("appraiser")]):
		return
	if not await _camera_holds("two players apart"):
		return
	if not _plates_clear_of_sources("two players apart"):
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

## Player two stays at base, so only player one's route uses the day: two
## days give two Pickle Surprises and a Mushroom Soup's worth.
func _a_hated_dish_is_not_fed_again() -> void:
	await _start_two_players()
	walker = "chef"
	if not await _collect_along([Vector2i(18, 8), Vector2i(18, 12), Vector2i(21, 13)], 2):
		return
	if not await _collect_along([Vector2i(23, 13)], 1):
		return
	await _tap(KEY_Q)
	await _until("player one comes home", func() -> bool: return _str("phase") == "workshop", 120)
	await _tap(KEY_G)
	if not await _until("the first gathering day ends", func() -> bool: return _int("day") == 2, 30):
		return
	if not await _collect_along([Vector2i(18, 8), Vector2i(18, 12), Vector2i(23, 13)], 2):
		return
	if not await _collect_along([Vector2i(18, 12), Vector2i(18, 8), Vector2i(11, 9)], 1):
		return
	await _tap(KEY_Q)
	await _until("player one comes home again", func() -> bool: return _str("phase") == "workshop", 120)
	var attempts: int = 0
	while _next().begins_with("Appraise ") and attempts < 40:
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
	_check(_next().begins_with("Cook ") and not _next().contains("Feed"), "with only a hated dish stored the screen cooks: %s" % _next())

## Cooks, the first time by a click on the task card's step, and feeds the last
## of the pack, so only the hated dish is left and nothing can be cooked: the
## line sets out while the day has time, then ends it.
func _a_hated_dish_alone_sets_out_then_ends_the_day() -> void:
	if not await _a_click_on_the_step_cooks():
		return
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
	if not _check(_next().begins_with("Set out into the city"), "with nothing to cook or feed the line sets out while the day has time: %s" % _next()):
		return
	await _tap(KEY_Q)
	if not await _until("player one sets out", func() -> bool: return _where("chef") == "city", 30):
		return
	if not await _until("the day's clock runs out", func() -> bool: return _str("phase") == "workshop", DAY_FRAMES):
		return
	if not _check(_int("remaining_ms") == 0 and _stored() == ["Pickle Surprise"] and _str("dish_offered") == "", "the day ran out with only the hated dish and nothing to cook"):
		return
	_check(_next().begins_with("End the day unfed"), "with nothing to cook or feed and no time left the line ends the day unfed: %s" % _next())

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
	await _frames(2)
	if not _check(_actor("appraiser", "collecting_ms") > 0 and _actions().size() == 2 and _action(1).begins_with("COLLECTING ") and not _action(0).begins_with("COLLECTING "), "player two's button shows how long player two's hold has left: %s" % [_actions()]):
		return
	if not _check(_drawn("IJKL move  H run  ; jump") and _drawn("WASD move  Shift run  Space jump  Set out"), "player two's keys leave out travel while the hold keeps player two in place: %s" % [_drawn_lines()]):
		return
	if not _text_fits("player two's hold"):
		return
	if not await _until("player two's wrench collection ends", func() -> bool: return _actor("appraiser", "collecting_ms") == 0, COLLECT_FRAMES):
		return
	await _frames(2)
	if not _check(_actions().size() == 2 and _action(1) == "USE" and _drawn("IJKL move  H run  ; jump  To base"), "player two's button names its action again once the hold ends: %s" % [_actions()]):
		return
	if not _check(_bag_holds("Wrench", "P2") and not _bag_holds("Wrench", "P1"), "the bag marks the wrench as player two's: %s" % [scene.get_meta("bag", [])]):
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
	if not await _until("player two's first crate collection ends", func() -> bool: return _actor("appraiser", "collecting_ms") == 0, COLLECT_FRAMES):
		return
	await _tap(KEY_O)
	if not await _until("player two's second crate collection ends", func() -> bool: return _actor("appraiser", "collecting_ms") == 0, COLLECT_FRAMES):
		return
	if not await _walk_route(TERRACE_ROUTE):
		return
	await _tap(KEY_O)
	if not await _until("player two's first terrace bag collection ends", func() -> bool: return _actor("appraiser", "collecting_ms") == 0, COLLECT_FRAMES):
		return
	await _tap(KEY_O)
	if not await _until("player two's second terrace bag collection ends", func() -> bool: return _actor("appraiser", "collecting_ms") == 0, COLLECT_FRAMES):
		return
	await _tap(KEY_U)
	if not await _until("player two comes home", func() -> bool: return _str("phase") == "workshop", 30):
		return
	if not _check(_stock("Supply Crate") == 0 and _stock("Terrace Bag") == 0 and _actor("appraiser", "tools") == 5 and _actor("chef", "tools") == 0, "player two empties the last sources in reach and only player two holds the wrench"):
		return
	if not _check(_object("Roof Ladder") == "destroyed" and _stock("Roof Bag") > 0 and _int("remaining_ms") > 0, "stock is left behind the broken ladder and the day has time"):
		return
	if not _check(_next().begins_with("Set out into the city"), "the base line sets out for player two's errand alone: %s" % _next()):
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
		if not await _until("the hold of collection %d ends" % (time + 1), func() -> bool: return _actor(walker, "collecting_ms") == 0, COLLECT_FRAMES):
			return false
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
	return _status()["out_of_reach"]

func _changing_text() -> Array:
	return _status()["changing_text"]

func _lines(field: String) -> Array:
	return _status()[field]

func _result_says(words: String) -> bool:
	for line: String in _lines("day_result"):
		if line.contains(words):
			return true
	return false

## Pauses and continues, and tells whether the escape goal was drawn over
## the pause screen and not on the screen under it. The screen and the pause
## over it each draw every word at a whole multiple of the font's grid.
func _pause_shows_escape_goal() -> bool:
	var before: bool = _drawn_says(ESCAPE_GOAL)
	if not _text_on_grid("the screen under the pause"):
		return false
	await _tap(KEY_ESCAPE)
	var paused: bool = _str("interruption") == "paused" and _drawn_says(ESCAPE_GOAL)
	if not _text_on_grid("the pause screen"):
		return false
	await _tap(KEY_ESCAPE)
	return _str("goal") == ESCAPE_GOAL and not before and paused and _str("interruption") == ""

## The lines the screen drew in its last frame, in drawing order.
func _drawn_lines() -> PackedStringArray:
	return scene.get_meta("screen_text", PackedStringArray())

## Whether every word the last frame drew was drawn at a whole multiple of the
## font's grid, 16 pixels at the least, and the font is imported without
## antialiasing; the control is that the frame drew words at all.
func _text_on_grid(where: String) -> bool:
	var sizes: PackedInt32Array = scene.get_meta("screen_text_sizes", PackedInt32Array())
	var off: Array[int] = []
	for size: int in sizes:
		if size % FONT_GRID != 0 or size < MIN_TEXT_SIZE:
			off.append(size)
	var face: FontFile = load(CHANGING_FONT)
	return _check(not sizes.is_empty() and off.is_empty() and face.antialiasing == TextServer.FONT_ANTIALIASING_NONE, "every word on %s is drawn at a multiple of %d, %d at the least, in an unsmoothed face: off the grid %s of %s" % [where, FONT_GRID, MIN_TEXT_SIZE, off, sizes])

## Whether the last frame drew a line reading exactly `words`.
func _drawn(words: String) -> bool:
	return _drawn_lines().has(words)

## Whether the last frame drew a line holding `words`.
func _drawn_says(words: String) -> bool:
	for line: String in _drawn_lines():
		if line.contains(words):
			return true
	return false

## Whether the bag drawn in the last frame holds `item` carried by `owner`.
func _bag_holds(item: String, owner: String) -> bool:
	for slot: Dictionary in scene.get_meta("bag", []):
		if slot["name"] == item and slot["owner"] == owner:
			return true
	return false

## The name the screen gives a city object.
func _place(object: String) -> String:
	return str(_status()["places"][object])

## What each human's action button reads, the chef's first.
func _actions() -> Array:
	return _status()["actions"]

## What one human's action button reads, by its place in `_actions()`.
func _action(index: int) -> String:
	return str(_actions()[index])

## The keys of the caps drawn over the humans' heads in the last frame.
## The screen pixels the HUD's panels, chips and caps covered in the last
## frame, from the areas the HUD laid them out at.
func _hud_area() -> float:
	var area: float = 0.0
	for panel: Rect2 in scene.get_meta("hud_panels", []):
		area += panel.get_area()
	return area

## Whether the last frame drew no key cap, such as "[E]", and no Esc, while
## it drew the pause chip, as the control that it drew the HUD's words.
func _names_no_action_key(where: String) -> bool:
	var keyed: Array[String] = []
	for line: String in _drawn_lines():
		if line.contains("[") or line.contains("Esc"):
			keyed.append(line)
	return _check(keyed.is_empty() and _drawn("Pause, keys"), "%s names no key but the movement keys: %s" % [where, keyed])

## Every place the map names has its button and its log lines among the
## changing lines, so the measure covers each name and not only some.
func _every_place_measured() -> bool:
	var texts: Array[String] = []
	for line: Dictionary in _changing_text():
		texts.append(str(line["text"]))
	var places: Dictionary = _status()["places"]
	var missing: Array[String] = []
	for object: String in places:
		var place: String = str(places[object])
		var listed: bool = texts.has("COLLECT " + place) and texts.has("Chef repaired " + place + ".") and texts.has(place + " appeared.")
		if not listed and not missing.has(place):
			missing.append(place)
	return _check(not places.is_empty() and missing.is_empty(), "every place's lines are measured: %s" % [missing])

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

## A washed-up bag the acid or the tide carried off, or one emptied, is not
## drawn; the controls are a bag still holding stock, and the vendor's cart,
## drawn although its vendor is gone.
func _gone_bags_leave_the_map() -> bool:
	var drawn: Array = scene.get_meta("map_sources", [])
	var objects: Dictionary = _status()["objects"]
	var stock: Dictionary = _status()["stock"]
	var gone: Array[String] = []
	var kept: Array[String] = []
	for source: String in stock:
		if not source.begins_with("Washed-up Bag"):
			continue
		var state: String = objects[source]
		var left: int = stock[source]
		if state == "destroyed" or (state == "intact" and left == 0):
			gone.append(source)
		elif state == "intact":
			kept.append(source)
	var hidden: bool = gone.all(func(bag: String) -> bool: return not drawn.has(bag))
	var shown: bool = not kept.is_empty() and kept.all(func(bag: String) -> bool: return drawn.has(bag))
	return _check(not gone.is_empty() and hidden and shown and drawn.has("Vendor Cart") and _object("Noodle Vendor") == "destroyed", "a bag carried off or emptied leaves the map while the rest stay: gone %s, drawn %s" % [gone, drawn])

## No word plate on the map covers another; the control is that the plates
## read are the city's: more than a count for each source whose count is on
## the play area.
func _plates_apart(where: String) -> bool:
	var plates: Array = scene.get_meta("map_plates", [])
	var labelled: Array = scene.get_meta("map_labelled", [])
	var sources: int = labelled.size()
	var covered: Array[String] = []
	for first: int in range(plates.size()):
		for second: int in range(first + 1, plates.size()):
			var one: Rect2 = plates[first]
			var other: Rect2 = plates[second]
			if one.intersects(other):
				covered.append("%s over %s" % [one, other])
	return _check(sources > 0 and plates.size() > sources and covered.is_empty(), "no map plate covers another on %s: %d plates, %d sources, %s" % [where, plates.size(), sources, covered])

## Where a human's feet are in the scene.
func _feet(who: String) -> Vector2:
	return MAP_ORIGIN + Vector2(_actor(who, "sub_x"), _actor(who, "sub_y"))

## The humans out in the city.
func _humans_out() -> Array[String]:
	var out: Array[String] = []
	for who: String in ["chef", "appraiser"]:
		if _actor(who, "key_set") >= 0 and _where(who) == "city":
			out.append(who)
	return out

## Once the camera has settled: every human in the city stands on the play
## area; the view shows nothing past the map's sides or the scene's top and
## bottom, and centres on the map when wider than it; one human is drawn at
## twice the scene's size, and two between once and twice, smaller than twice
## when they stand APART or more.
func _camera_holds(where: String) -> bool:
	if not await _camera_settles(where):
		return false
	var view: Transform2D = scene.get_meta("map_view", Transform2D())
	var zoom: float = view.x.x
	var humans: Array[String] = _humans_out()
	var shown: bool = humans.all(func(who: String) -> bool: return PLAY.has_point(view * _feet(who)))
	var left: float = (view * MAP_ORIGIN).x
	var right: float = (view * (MAP_ORIGIN + MAP_SIZE)).x
	var inside: bool = (view * Vector2.ZERO).y <= 0.0 and (view * Vector2(0, SCENE_HEIGHT)).y >= SCENE_HEIGHT
	if right - left >= PLAY.size.x:
		inside = inside and left <= 0.0 and right >= PLAY.size.x
	else:
		inside = inside and absf((left + right) / 2.0 - PLAY.size.x / 2.0) <= 1.0
	var framed: bool = is_equal_approx(zoom, 2.0)
	if humans.size() == 2:
		var apart: Vector2 = (_feet("chef") - _feet("appraiser")).abs()
		framed = zoom >= 1.0 and zoom <= 2.0 and (zoom < 2.0 or (apart.x < APART and apart.y < APART / 4.0))
	return _check(not humans.is_empty() and shown and inside and framed, "the camera frames the humans on %s: zoom %.2f, humans %s at %s, map from x %.0f to %.0f" % [where, zoom, humans, humans.map(func(who: String) -> Vector2: return view * _feet(who)), left, right])

## With one human, an AI partner far off does not pull the camera: it stays
## at twice the scene's size on the human. The control is that the partner
## stands where a camera framing both would have to draw them smaller.
func _camera_ignores_partner(where: String) -> bool:
	var apart: Vector2 = (_feet("chef") - _feet("appraiser")).abs()
	if not _check(_where("appraiser") == "city" and _actor("appraiser", "key_set") == -1 and (apart.x >= APART or apart.y >= APART / 4.0), "the AI partner stands far off on %s: %s apart" % [where, apart]):
		return false
	return await _camera_holds(where)

## Waits until the camera drawn is where it heads, which the city leaves as
## map_heading, so a check reads a camera at rest.
func _camera_settles(where: String) -> bool:
	var started: int = Time.get_ticks_msec()
	while Time.get_ticks_msec() - started < SETTLE_LIMIT_MSEC:
		var view: Transform2D = scene.get_meta("map_view", Transform2D())
		var heading: Transform2D = scene.get_meta("map_heading", Transform2D(0.0, Vector2(-1, -1)))
		if view.is_equal_approx(heading):
			return true
		OS.delay_msec(SETTLE_POLL_MSEC)
		await process_frame
	return _check(false, "the camera comes to rest on %s: drawn %s, heading %s" % [where, scene.get_meta("map_view", null), scene.get_meta("map_heading", null)])

## Once the camera has settled on a lone human facing `facing`, 1 east and -1
## west, the human stands LOOK_AHEAD scene pixels behind the middle of the
## play area. The control is that the camera there is not held by the map's
## edge: the map runs past the screen on both sides.
func _looks_ahead(who: String, facing: float, where: String) -> bool:
	var view: Transform2D = scene.get_meta("map_view", Transform2D())
	var feet: Vector2 = view * _feet(who)
	var left: float = (view * MAP_ORIGIN).x
	var right: float = (view * (MAP_ORIGIN + MAP_SIZE)).x
	var expected: float = FOCUS.x - facing * LOOK_AHEAD * view.x.x
	return _check(left < 0.0 and right > PLAY.size.x and absf(feet.x - expected) <= 1.0, "the camera looks ahead of a lone human %s: feet at x %.1f, expected %.1f, map from x %.0f to %.0f" % [where, feet.x, expected, left, right])

## The camera and where the backdrop's layers were drawn, back to front.
func _backdrop() -> Dictionary:
	var world: Node = scene.get_node("World")
	return {"view": scene.get_meta("map_view", Transform2D()), "layers": world.get_meta("backdrop_layers", [])}

## Between two camera positions at one zoom, each backdrop layer slides the
## same way as the map, and less the farther back it is: the throat least,
## then the city, then the acid band, all less than the map. The control is
## that the map moved.
func _backdrop_slides(before: Dictionary, after: Dictionary, where: String) -> bool:
	var first: Transform2D = before["view"]
	var second: Transform2D = after["view"]
	var moved: float = second.origin.x - first.origin.x
	var shares: Array[float] = []
	var layers_before: Array = before["layers"]
	var layers_after: Array = after["layers"]
	if layers_before.size() == 3 and layers_after.size() == 3:
		for layer: int in range(3):
			var from: Vector2 = layers_before[layer]
			var to: Vector2 = layers_after[layer]
			shares.append((to.x - from.x) / moved if moved != 0.0 else 0.0)
	var ordered: bool = shares.size() == 3 and 0.0 < shares[0] and shares[0] < shares[1] and shares[1] < shares[2] and shares[2] < 1.0
	return _check(is_equal_approx(first.x.x, second.x.x) and absf(moved) >= 100.0 and ordered, "the backdrop's layers slide less than the map the farther back they are %s: the map moved %.0f, the layers by shares %s" % [where, moved, shares])

## No word plate on the screen but a source's count covers a source, and none
## lies under a HUD panel or note, now or on any frame walked since the last
## such check, as the camera moved and the notes came and went. The control is
## that the plates read now hold counts and other words.
func _plates_clear_of_sources(where: String) -> bool:
	var found: Dictionary = _plates_over_sources()
	var counts: int = found["counts"]
	var plates: int = found["plates"]
	var covered: Array[String] = covered_on_the_way.duplicate()
	var over: Array[String] = found["covered"]
	covered.append_array(over)
	covered_on_the_way.clear()
	return _check(counts > 0 and plates > counts and covered.is_empty(), "no map plate covers a source but its count or lies under the HUD on %s or on the way: %d plates, %d counts, %s" % [where, plates, counts, covered])

## The plates drawn, the sources' counts among them, the other plates over a
## source, and every plate under a HUD panel or note.
func _plates_over_sources() -> Dictionary:
	var plates: Array = scene.get_meta("map_plates", [])
	var areas: Array = scene.get_meta("map_source_areas", [])
	var count_plates: Array = scene.get_meta("map_counts", [])
	var panels: Array = scene.get_meta("hud_panels", [])
	var notes: Array = scene.get_meta("hud_notes", [])
	var counts: int = 0
	var covered: Array[String] = []
	for plate: Rect2 in plates:
		for area: Rect2 in panels + notes:
			if plate.intersects(area):
				covered.append("%s under the HUD at %s" % [plate, area])
		if count_plates.has(plate):
			counts += 1
			continue
		for area: Rect2 in areas:
			if area.intersects(plate):
				covered.append("%s over %s" % [plate, area])
	return {"plates": plates.size(), "counts": counts, "covered": covered}

## The goal the NEXT line names for the human gets a pointer at the screen's
## edge exactly when its count is not on the play area, and the map's words
## name it and no other place, and no source's kind; the control is that the
## map drew its sources' counts.
func _points_to_goal(where: String) -> bool:
	var line: String = _next()
	if not (line.begins_with("Walk ") or line.begins_with("Jump ") or line.begins_with("Climb ")):
		var none: Array = scene.get_meta("map_pointers", [])
		return _check(none.is_empty() and _misnamed() == "", "nothing is pointed at while no goal is named on %s, and the map names nothing else: %s, %s" % [where, line, _misnamed()])
	var goal: String = line.get_slice(" to ", 1).get_slice(".", 0)
	var drawn: Array = scene.get_meta("map_sources", [])
	var labelled: Array = scene.get_meta("map_labelled", [])
	var pointers: Array = scene.get_meta("map_pointers", [])
	# The map names sources by object name; the line by the screen's name.
	var places: Dictionary = _status()["places"]
	var shown: bool = false
	for object: String in places:
		if places[object] == goal and drawn.has(object) and pointers.has(object) != labelled.has(object):
			shown = true
	var count_plates: Array = scene.get_meta("map_counts", [])
	var misnamed: String = _misnamed()
	return _check(line.contains(" to ") and shown and not count_plates.is_empty() and misnamed == "", "the goal %s is on the play area or pointed at on %s, and named alone: labelled %s, pointers %s, %s" % [goal, where, labelled, pointers, misnamed])

## What is wrong with the map's words, or nothing: a word naming a source's
## kind, a name other than of the goals the NEXT line names, or a goal it
## names to walk to left unnamed on the map and by its pointer.
func _misnamed() -> String:
	var words: Array = scene.get_meta("map_words", [])
	var line: String = _next()
	for word: String in words:
		for part: String in word.split(" "):
			if KIND_WORDS.has(part):
				return "the map names a source's kind: %s" % word
		if not (word.is_valid_int() or MAP_MARKS.has(word) or line.contains(word)):
			return "the map names %s, which the NEXT line %s does not" % [word, line]
	if line.contains(" to ") and (line.begins_with("Walk ") or line.begins_with("Jump ") or line.begins_with("Climb ")):
		var goal: String = line.get_slice(" to ", 1).get_slice(".", 0)
		if not words.has(goal):
			return "the map leaves out the goal %s: %s" % [goal, words]
	return ""

func _next_event_has(text: String) -> bool:
	for event: String in _events():
		if event.contains(text):
			return true
	return false

func _events() -> Array:
	return _status()["events"]

func _stored() -> Array:
	return _status()["stored_foods"]

func _spatula() -> String:
	return _str("spatula")

## The run's status, built at most once a frame.
func _status() -> Dictionary:
	var frame: int = Engine.get_process_frames()
	if frame != status_frame:
		status_read = scene.survival_status()
		status_frame = frame
	return status_read

func _str(field: String) -> String:
	var value: String = _status().get(field, "")
	return value

func _int(field: String) -> int:
	var value: int = _status().get(field, 0)
	return value

func _where(who: String) -> String:
	var actor: Dictionary = _status()[who]
	var location: String = actor["location"]
	return location

func _actor(who: String, field: String) -> int:
	var actor: Dictionary = _status()[who]
	var value: int = actor[field]
	return value

func _stock(source: String) -> int:
	var stock: Dictionary = _status()["stock"]
	var remaining: int = stock[source]
	return remaining

func _object(object: String) -> String:
	var objects: Dictionary = _status()["objects"]
	var state: String = objects[object]
	return state

## Walks the human through each waypoint in turn.
func _walk_route(waypoints: Array) -> bool:
	for waypoint: Vector2i in waypoints:
		if not await _walk_to(waypoint.x, waypoint.y):
			return false
	return true

## Walks the human sideways along its floor, stairs and steps included, to
## column x, jumping whatever low block stops it, then climbs to row y. It runs
## while the column is more than two cells away: the day is short and the AI
## partner reaches the stock too.
func _walk_to(x: int, y: int) -> bool:
	var first: int = 5 * _actor(walker, "key_set")
	var run: Key = KEY_SHIFT if first == 0 else KEY_H
	var stalled: int = 0
	var last: int = _actor(walker, "sub_x")
	for frame: int in range(900):
		var at_x: int = _actor(walker, "x")
		var at_y: int = _actor(walker, "y")
		if at_x == x and at_y == y and _stance(walker) != "airborne":
			_release()
			return true
		var sideways: bool = at_x != x
		_key(WALK_KEYS[first], sideways and x < at_x)
		_key(WALK_KEYS[first + 1], sideways and x > at_x)
		_key(WALK_KEYS[first + 2], not sideways and y < at_y)
		_key(WALK_KEYS[first + 3], not sideways and y > at_y)
		_key(run, absi(x - at_x) > 2)
		var sub_x: int = _actor(walker, "sub_x")
		stalled = stalled + 1 if sideways and sub_x == last and _stance(walker) == "standing" else 0
		last = sub_x
		_key(WALK_KEYS[first + 4], stalled > 6)
		await process_frame
		var over: Array[String] = _plates_over_sources()["covered"]
		covered_on_the_way.append_array(over)
	_release()
	return _check(false, "%s walks to %d, %d" % [walker, x, y])

func _stance(who: String) -> String:
	var actor: Dictionary = _status()[who]
	var stance: String = actor["stance"]
	return stance

func _until(name: String, condition: Callable, limit: int) -> bool:
	for frame: int in range(limit):
		var done: bool = condition.call()
		if done:
			return true
		await process_frame
	return _check(false, name)

## Opens the pause screen with Esc and returns how far below its resting
## place the first frame drew its button, once the button has come to rest.
func _pause_rise() -> float:
	scene.set_meta("interruption_button", Rect2())
	_key(KEY_ESCAPE, true)
	var first: Rect2 = Rect2()
	for frame: int in range(30):
		await process_frame
		first = scene.get_meta("interruption_button", Rect2())
		if first.has_area():
			break
	if not _check(first.has_area(), "the pause screen draws its button"):
		return 0.0
	_key(KEY_ESCAPE, false)
	OS.delay_msec(SETTLE_MSEC)
	await _frames(2)
	var rest: Rect2 = scene.get_meta("interruption_button", Rect2())
	await _frames(1)
	var after: Rect2 = scene.get_meta("interruption_button", Rect2())
	if not _check(rest.is_equal_approx(after), "the pause screen's button comes to rest: %s" % rest):
		return 0.0
	return first.position.y - rest.position.y

## M asks first mid-run and Enter accepts; M leaves a cinematic at once. Enter on
## the title screen takes its marked entry, so it is pressed only to answer the
## question, and the title must show before the next menu key.
func _leave_run() -> void:
	var status: Dictionary = _status()
	if status.has("cinematic"):
		await _tap(KEY_M)
	elif not status.is_empty():
		await _tap(KEY_M)
		await _tap(KEY_ENTER)
	await _until("the title screen shows after leaving the run", func() -> bool: return scene.screen_name() == "main" and _status().is_empty(), 30)

func _tap(code: Key) -> void:
	_key(code, true)
	await _frames(2)
	_key(code, false)
	await _frames(8)

## The target the last frame drew whose click presses `press`; empty with none.
func _target(press: String) -> Dictionary:
	var targets: Array = scene.get_meta("pointer_targets", [])
	for target: Dictionary in targets:
		if target["press"] == press:
			return target
	return {}

## The first target the last frame drew whose click presses `press` and whose
## area `keep` accepts; empty with none.
func _target_where(press: String, keep: Callable) -> Dictionary:
	var targets: Array = scene.get_meta("pointer_targets", [])
	for target: Dictionary in targets:
		if target["press"] == press and keep.call(target["rect"]):
			return target
	return {}

## What every target the last frame drew presses.
func _presses() -> Array[String]:
	var names: Array[String] = []
	var targets: Array = scene.get_meta("pointer_targets", [])
	for target: Dictionary in targets:
		names.append(str(target["press"]))
	return names

## What the target under the mouse presses; empty over none.
func _hovered() -> String:
	var targets: Array = scene.get_meta("pointer_targets", [])
	for target: Dictionary in targets:
		if target["hovered"]:
			return str(target["press"])
	return ""

## Moves the mouse to the middle of `target`, a point on the screen the scene
## draws, through the window's stretch.
func _point_at(target: Dictionary) -> void:
	var area: Rect2 = target.get("rect", Rect2())
	var event: InputEventMouseMotion = InputEventMouseMotion.new()
	event.position = root.get_final_transform() * area.get_center()
	event.global_position = event.position
	Input.parse_input_event(event)

## The target drawn last under `point` on the screen; empty over none.
func _target_at(point: Vector2) -> Dictionary:
	var found: Dictionary = {}
	var targets: Array = scene.get_meta("pointer_targets", [])
	for target: Dictionary in targets:
		var area: Rect2 = target["rect"]
		if area.has_point(point):
			found = target
	return found

## Points at the target pressing `press` and clicks it with the left button;
## false when the screen shows no such target.
func _click(press: String) -> bool:
	return await _click_on(_target(press))

## Points at `target` and clicks it with the left button; false for none.
func _click_on(target: Dictionary) -> bool:
	if target.is_empty():
		return false
	var area: Rect2 = target["rect"]
	_point_at(target)
	await _frames(2)
	await _click_at(area.get_center())
	return true

## Clicks the left button at `point` on the screen the scene draws, moving the
## mouse there in the same frame.
func _click_at(point: Vector2) -> void:
	var motion: InputEventMouseMotion = InputEventMouseMotion.new()
	motion.position = root.get_final_transform() * point
	motion.global_position = motion.position
	Input.parse_input_event(motion)
	for pressed: bool in [true, false]:
		var event: InputEventMouseButton = InputEventMouseButton.new()
		event.button_index = MOUSE_BUTTON_LEFT
		event.pressed = pressed
		event.position = motion.position
		event.global_position = event.position
		Input.parse_input_event(event)
		await _frames(2 if pressed else 8)

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
