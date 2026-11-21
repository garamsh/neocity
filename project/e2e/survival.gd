class_name SurvivalPlayCheck
extends SceneTree
## Plays the survival game through the shipped scene with real key events: full
## days as the chef, a day as the appraiser, and the map changes between;
## and the same commands with real mouse events, alone and for two players.

const SEED: int = 11
const DAY_FRAMES: int = 1200
## Frames a search takes to close, with a step to spare; a wait for AI to come
## home adds one, since AI finishes its search first.
const COLLECT_FRAMES: int = 102
## Presses of the search key before a container another actor holds is given up.
const SEARCH_ATTEMPTS: int = 12
## Frames the AI partner stands still before the camera is read: a search of a
## full container holds it about this long.
const REST_FRAMES: int = 30
## On this seed a party following NEXT alone meets a hated dish, then ends the run satisfied.
const LONG_RUN_SEED: int = 4
const LONG_RUN_DAYS: int = 60
const DAY_STEPS: int = 400
const GATHERING_DAYS: int = 3
## Six days: a run ends at seven days fed to satisfaction, so six cannot end it.
const APPRAISER_DAYS: int = 6
## This seed's pack after GATHERING_DAYS cooks this dish twice.
const CHOSEN: String = "Fish Stew"
const MENU_PRESSES: int = 12
const COOK_ATTEMPTS: int = 12
const STEP_FRAMES: int = 6
## Waits of six frames a walker gives a patrol, and an absent walker is given.
const MUTANT_WAIT_STEPS: int = 40
const ABSENCE_WAIT_STEPS: int = 400
## Sub-cells behind the walker a mutant still counts as in its way.
const MUTANT_CLEARANCE: int = 16
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
## From the entrance east to the east loft and the vendor cart on it.
const VENDOR_ROUTE: Array[Vector2i] = [Vector2i(23, 8), Vector2i(23, 6), Vector2i(25, 6)]
## From the entrance down the step to the tomato machine.
const TOMATO_ROUTE: Array[Vector2i] = [Vector2i(11, 9)]
## From the entrance up the kitchen ladder to the shop shelf on the loft.
const SHELF_ROUTE: Array[Vector2i] = [Vector2i(3, 8), Vector2i(3, 6), Vector2i(4, 6)]
## The obstacle once drawn as the acid drip stands on the upper walkway west of
## the kitchen ladder, at 1, 8; the walkway cell beside it is 2, 8.
const OBSTACLE_CELL: Vector2i = Vector2i(1, 8)
const BESIDE_OBSTACLE: Vector2i = Vector2i(2, 8)
## The exploration time a touched actor is away: ten seconds, in milliseconds.
const ABSENCE_MSEC: int = 10000
## From the entrance up the kitchen ladder to the foot of the roof ladder on
## the loft.
const LADDER_ROUTE: Array[Vector2i] = [Vector2i(3, 8), Vector2i(3, 6), Vector2i(1, 6)]
## From the entrance up the kitchen ladder and the roof ladder to the roof bag.
const ROOF_ROUTE: Array[Vector2i] = [Vector2i(3, 8), Vector2i(3, 6), Vector2i(1, 6), Vector2i(1, 4), Vector2i(6, 4)]
## The map's top-left corner and size in the scene, as the city draws it; a
## sub-cell is one scene pixel, and the scene is 720 pixels tall.
const MAP_ORIGIN: Vector2 = Vector2(64, 70)
const SCENE_HEIGHT: float = 720.0
## The play area between the top bar and the cards, on screen.
const PLAY: Rect2 = Rect2(0, 62, 1280, 478)
## The camera eases on the real clock while each frame moves the game on, so
## a wait for it sleeps this long between frames, and gives up after the
## limit, well past the camera's eased motion.
const SETTLE_POLL_MSEC: int = 20
const SETTLE_LIMIT_MSEC: int = 3000
## How far, in screen pixels, a camera may still lag a walking AI partner's
## heading and count as at rest. Only the rest test uses it; the checks that
## read the settled camera stay exact.
const SETTLE_SLACK: float = 24.0
## The middle of the play area on screen, where the camera draws what it
## heads for.
const FOCUS: Vector2 = Vector2(640, 301)
## How far apart, in scene pixels, two actors stand when the camera must draw
## them smaller than twice the scene's size to frame both.
const APART: float = 480.0
## The screen lock, in sub-cells, and how near it a walker stops pressing on
## so its partner can come up and a jump is never mistaken for a snag.
const SCREEN_LOCK: int = 384
const LOCK_WAIT: int = 320
## Where the workshop draws the first ingredient waiting for appraisal, in the
## appraise panel's first slot.
const APPRAISER_HEAD: Vector2 = Vector2(62, 622)
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
## deck's panels sit below it.
const SCREEN_MIDDLE: float = 360.0
## What the chef says at base while the inventory is open: the screen takes the
## interact key, so cooking waits for it to close.
const CLOSE_FIRST: String = "Close the inventory first."
## The face the screen draws every changing line in.
const CHANGING_FONT: String = "res://levels/monster_mouth_escape/fonts/Tiny5-Regular.ttf"
## The face's grid, in layout units per em, which every drawn size is a multiple of.
## A layout unit is 1.5 canvas pixels, so this keeps the face's pixels square on
## the layout grid and not on the screen's, as ADR 0024 records.
const FONT_GRID: int = 8
## The smallest size, in layout units, the interface draws a word at.
const MIN_TEXT_SIZE: int = 16
## What ends the run well, which the pause screen names.
const ENDING_GOAL: String = "Satisfy the monster on 7 days"
## How many first-play tips there are, one per concept that base shows; the
## controls page lists the move and collect tips, which no screen opens.
const TIP_COUNT: int = 7
## The screen, where a tip must stand whole.
const SCREEN: Rect2 = Rect2(0, 0, 1280, 720)
## The key caps the action prompts draw: the search and travel keys of each key
## set.
const PROMPT_CAPS: Array[String] = ["[R]", "[P]", "[Q]", "[U]"]
## The set-out tip's words, which the tip draws its key after.
const SET_OUT_TIP: String = "Set out into the city to gather food. The day's clock runs only there."
## The top band's lower edge, which holds the day, acid and clock plates alone.
const TOP_BAND_END: float = 56.0
## The words of a source's kind the map once wrote beside its supply.
const KIND_WORDS: Array[String] = ["BAG", "VEND", "CART", "SHOP"]
## The map's words that are neither a place's name nor a number, such as a
## count: a broken passage's mark and the actors' tags.
const MAP_MARKS: Array[String] = ["BROKEN", "P1", "P2", "AI"]
## The words of the actors' tags, and the space a tag keeps from any other plate.
const ACTOR_TAGS: Array[String] = ["P1", "P2", "AI"]
const TAG_CLEARANCE: float = 7.5
## How far past either end of the map the camera may look, in scene pixels.
const EDGE_ROOM: float = 64.0
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
## frames drew one, at base only.
var tip_overlaps: Array[String] = []
var tip_frames: int = 0

func _initialize() -> void:
	_run.call_deferred()

## Only the game's own quit reaches here with the pass line owed.
func _finalize() -> void:
	if awaiting_quit and not failed:
		print("Survival: ", checks, " live checks passed (menus, days, return, appraisal, cooking, dish choice, feeding, map, obstacle, roles, pause, leaving, hand-off, mouse, quit).")

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
		await _the_days_change_the_city()
	if not failed:
		await _touching_an_obstacle_sends_the_actor_away()
	if not failed:
		await _ai_chef_work_yields_to_a_dish_to_feed()
	if not failed:
		await _chef_chooses_a_dish()
	if not failed:
		await _follow_next_to_the_ending()
	if not failed:
		await _a_broken_passage_with_no_stock_behind_stops_setting_out()
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
		await _player_two_searches_and_holds_in_place()
	if not failed:
		await _search_screen_and_fridge()
	if not failed:
		await _chef_reappears_in_the_city_beside_a_partner_there()
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
		await _a_timed_cook_draws_its_bar()
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

## A click on the cook panel's step, when the step is the chef's cooking, cooks
## the dish it names, as E does.
func _a_click_on_the_step_cooks() -> bool:
	var step: Dictionary = _target_where("chef_interact", func(area: Rect2) -> bool: return area.position.y > SCREEN_MIDDLE)
	if not _check(not step.is_empty() and str(step["refusal"]) == "" and _next().contains("Cook"), "setup: the cook panel's step is the chef's cooking: %s" % [_base_state()]):
		return false
	var chosen: String = _str("dish_offered")
	var foods: int = _int("foods")
	return _check(await _click_on(step) and ((_int("foods") == foods + 1 and _next_event_has(chosen + " cooked.")) or _next_event_has(chosen + " burned")), "a click on the cook panel's step cooks the offered dish, as E does: %s" % [_base_state()])

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
	var chef: Dictionary = _target_where("chef_interact", func(area: Rect2) -> bool: return area.position.y < SCREEN_MIDDLE)
	if not _check(not chef.is_empty() and str(chef["refusal"]) == "" and _str("dish_offered") != "", "setup: the chef in the kitchen can cook: %s" % [_base_state()]):
		return false
	var chosen: String = _str("dish_offered")
	var foods: int = _int("foods")
	return _check(await _click_on(chef) and ((_int("foods") == foods + 1 and _next_event_has(chosen + " cooked.")) or _next_event_has(chosen + " burned")), "a click on the chef in the kitchen cooks the offered dish, as E does: %s" % [_base_state()])

## In the city by mouse: the set-out step sets out, nothing in reach shows no
## prompt, the prompt searches and closes, the pause screen's blocks, leave and
## cancel answer clicks, and the keys alone pause and return.
func _mouse_in_the_city() -> void:
	await _leave_run()
	await _new_game()
	await _tap(KEY_1)
	await _tap(KEY_E)
	walker = "chef"
	if not _check(await _click("party_travel"), "the set-out step takes a click"):
		return
	if not await _until("a click on the set-out step sets out, as Q does", func() -> bool: return _where("chef") == "city", 30):
		return
	# With no container in reach the prompt offers the way to base, and no search
	# or pause chip; R opens the inventory alone, and the prompt then closes it.
	if not _check(_target("chef_search").is_empty() and _target("pause").is_empty() and not _target("chef_travel").is_empty() and _actions() == ["TO BASE"] and _drawn("[Q]"), "with nothing in reach the prompt offers the way to base by its key and no search or pause chip stands: %s" % [_actions()]):
		return
	await _tap(KEY_R)
	if not _check(_screen_open(0) and _actor("chef", "searching") == -1 and _actions() == ["CLOSE"], "R with nothing in reach opens the inventory alone, and its prompt reads close: %s" % [_actions()]):
		return
	if not _check(await _click("chef_search") and not _screen_open(0), "a click on the close prompt closes the inventory"):
		return
	if not await _walk_route(TOMATO_ROUTE):
		return
	var left: int = _int("remaining_ms")
	if not _check(await _click("chef_search") and _actor("chef", "searching") >= 0, "a click on the action button opens the search, as R does"):
		return
	# A search holds the chef: the shared clock runs on, the line says it is open,
	# and the travel key cannot return it to base.
	if not _check(_int("remaining_ms") <= left and _next().begins_with("Searching Tomato machine: "), "the line says the search is open: %s" % _next()):
		return
	await _tap(KEY_Q)
	if not _check(_where("chef") == "city" and _actor("chef", "searching") >= 0, "a searching chef cannot return to base"):
		return
	if not _check(await _click("chef_other_0") and _actor("chef", "ingredients") == 1, "a click on a container slot takes the item into the pack"):
		return
	if not _check(await _click("chef_search") and _actor("chef", "searching") == -1, "a click on the action button closes the search"):
		return
	# Away from a container the inventory moves nothing: the pack keeps its item.
	var carried: int = _actor("chef", "ingredients")
	if not await _walk_route([BESIDE_OBSTACLE]):
		return
	await _tap(KEY_R)
	if not _check(_screen_open(0), "setup: the inventory opens alone with an item in the pack"):
		return
	await _tap(KEY_E)
	if not _check(carried == 1 and _actor("chef", "ingredients") == carried, "the interact key on the inventory alone moves nothing out of the pack"):
		return
	if not _check(await _click("chef_search") and not _screen_open(0), "a click on the close prompt closes the inventory again"):
		return
	await _tap(KEY_ESCAPE)
	if not _check(_str("interruption") == "paused", "Esc pauses the run"):
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
	await _tap(KEY_Q)
	await _until("Q returns to base", func() -> bool: return _where("chef") == "base", 30)

## Two players share one mouse: the set-out step sets the party out, a human
## in the city has a prompt only with an action, and the travel key moves
## that human alone.
func _two_players_share_the_mouse() -> void:
	await _start_two_players()
	if not _check(await _click("party_travel"), "the set-out step takes a click with two players"):
		return
	if not await _until("one click sets the whole party out", func() -> bool: return _where("chef") == "city" and _where("appraiser") == "city", 30):
		return
	if not _check(_target("chef_search").is_empty() and _target("appraiser_search").is_empty() and _actions() == ["TO BASE", "TO BASE"] and _drawn("[U]"), "with nothing in reach each human's prompt offers the way to base: %s" % [_actions()]):
		return
	if not _check(await _click("appraiser_travel"), "player two's travel prompt takes a click"):
		return
	if not await _until("a click on player two's travel prompt moves only the appraiser", func() -> bool: return _where("appraiser") == "base" and _where("chef") == "city", 30):
		return
	await _tap(KEY_Q)
	await _until("player one comes back", func() -> bool: return _str("phase") == "workshop", 30)

## Real keys: Esc, M and Enter over a run, and over a cinematic.
func _pause_and_leave() -> void:
	await _leave_run()
	await _new_game()
	await _tap(KEY_1)
	await _tap(KEY_ESCAPE)
	if not _check(_str("cinematic") == "intro" and _str("interruption") == "", "Esc does not pause a cinematic"):
		return
	await _tap(KEY_M)
	if not _check(scene.screen_name() == "main" and _status().is_empty(), "M during a cinematic leaves at once"):
		return
	await _new_game()
	await _tap(KEY_1)
	await _tap(KEY_E)
	await _tap(KEY_Q)
	if not await _until("the chef sets out", func() -> bool: return _where("chef") == "city", 30):
		return
	if not _check(_top_bar_drawn(), "the top bar draws the day, the acid and the clock"):
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
	await _new_game()
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
	await _new_game()
	await _tap(KEY_1)
	await _tap(KEY_E)
	await _tap(KEY_TAB)
	if not await _until("one player swaps", func() -> bool: return _actor("chef", "key_set") == -1, 30):
		return
	_check(_str("hand_off") == "", "one player from the start gets no hand-off notice")

## A line the log repeats once the same line has dropped out of it is logged
## again, and the HUD draws no note of it: the chef sets out and returns with
## the AI appraiser, which fills the log, and sets out again.
func _repeated_note_appears() -> void:
	await _leave_run()
	await _new_game()
	await _tap(KEY_1)
	await _tap(KEY_E)
	await _tap(KEY_Q)
	if not await _until("the party sets out", func() -> bool: return _where("chef") == "city" and _where("appraiser") == "city", 60):
		return
	if not _check(_events().has("Chef set out.") and not _drawn_says("Chef set out."), "the log records the line and the HUD draws no note: %s %s" % [_events(), _drawn_lines()]):
		return
	await _tap(KEY_Q)
	if not await _until("the party comes home", func() -> bool: return _str("phase") == "workshop", COLLECT_FRAMES + 120):
		return
	var log: Array = _events()
	if not _check(log.size() == 4 and str(log[0]) == "Chef set out." and log.count("Chef set out.") == 1, "the log is full with the first note's line oldest: %s" % [log]):
		return
	await _tap(KEY_Q)
	if not await _until("the chef sets out again", func() -> bool: return _where("chef") == "city", 30):
		return
	_check(_events().count("Chef set out.") == 1 and not _drawn_says("Chef set out."), "the repeated line is logged again after the first dropped out, and no note is drawn: %s %s" % [_events(), _drawn_lines()])

func _start_two_players() -> void:
	await _leave_run()
	await _new_game()
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
## panels and every target. A run's tips start afresh off its screen.
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
	for other: Rect2 in covered:
		if other.intersects(area):
			tip_overlaps.append("%s over %s" % [text, other])
	if not SCREEN.encloses(area):
		tip_overlaps.append("%s off the screen at %s" % [text, area])
	tip_frames += 1

## The set-out key of each key set, from the controls table: its travel row.
func _set_out_keys() -> Array[String]:
	var keys: Array[String] = []
	for row: String in MainScene.controls_keys():
		if row.begins_with("Set out or return to base: "):
			for key: String in row.trim_prefix("Set out or return to base: ").split(" / "):
				if key != "":
					keys.append(key)
	return keys

## The endings the set-out tip may draw: the set-out key of one human, or of two.
func _set_out_endings() -> Array[String]:
	var keys: Array[String] = _set_out_keys()
	var endings: Array[String] = []
	for key: String in keys:
		endings.append(" Set out with [%s]." % key)
	if keys.size() == 2:
		endings.append(" Set out with [%s] (P1) or [%s] (P2)." % [keys[0], keys[1]])
	return endings

## `line` without the set-out key the set-out tip ends in; any other line, and a
## set-out tip ending in anything else, stays as drawn.
func _without_set_out_key(line: String) -> String:
	if line.begins_with(SET_OUT_TIP) and _set_out_endings().has(line.trim_prefix(SET_OUT_TIP)):
		return SET_OUT_TIP
	return line

## Every concept's tip opened, none twice in one run, a later run opened one
## again, and no drawn tip covered the HUD or a target; the control is that
## tips were drawn.
func _tips_opened_once_each() -> bool:
	var opened: Dictionary[String, bool] = {}
	for line: String in every_tip:
		opened[_without_set_out_key(line)] = true
	if not _check(opened.size() == TIP_COUNT, "every concept's tip opened: %s" % [every_tip.keys()]):
		return false
	if not _check(repeated_tips.is_empty() and tip_reopened, "no tip opened twice in one run, and a new run opened one again: %s" % [repeated_tips]):
		return false
	var keyed: Array[String] = []
	var lines: Dictionary[String, bool] = {}
	for line: String in every_tip:
		var bare: String = _without_set_out_key(line)
		lines[bare] = true
		if bare.contains("[") or bare.contains("Esc") or bare.contains("Enter"):
			keyed.append(line)
	if not _check(keyed.is_empty(), "the tips name actions and not keys, but the set-out key on the set-out tip: %s" % [keyed]):
		return false
	return _check(tip_overlaps.is_empty() and tip_frames > 0, "a tip covers no HUD panel or target and stays on the screen: %s in %s" % [tip_overlaps.slice(0, 5), tip_frames])

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
	if not _names_only_prompt_keys("the base"):
		return
	if not _check(_hud_area() < BASE_HUD_AREA_WITH_KEYS, "the base's HUD covers less than it did naming every key: %.0f against %.0f" % [_hud_area(), BASE_HUD_AREA_WITH_KEYS]):
		return
	if not _check(await _pause_shows_ending_goal(), "the pause screen over the base, and not the base, shows what ends the run"):
		return
	if not await _a_click_closes_the_tip():
		return

## The run's first tip says to set out; a click on it closes it and presses
## nothing, so the run stays at base on day one with its time.
func _a_click_closes_the_tip() -> bool:
	var tip: Dictionary = scene.get_meta("hud_tip", {})
	if not _check(_str("tip") == "set_out" and str(tip.get("text", "")) == SET_OUT_TIP + " Set out with [%s]." % _set_out_keys()[0], "the first tip at base says to set out and names the set-out key: %s" % [tip]):
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
	if not _check(_out_of_reach().is_empty(), "no stock starts out of reach"):
		return
	if not _text_fits("the first city screen"):
		return
	if not _every_place_measured():
		return
	if not _plates_apart("the first city screen"):
		return
	if not _tags_clear_of_names("the first city screen"):
		return
	if not _plates_clear_of_sources("the first city screen"):
		return
	if not await _camera_holds("the first city screen"):
		return
	var view: Transform2D = scene.get_meta("map_view", Transform2D())
	var upper_walkway: float = (view * Vector2(0, MAP_ORIGIN.y + 9 * 32)).y
	if not _check(absf(upper_walkway - 329.0) <= 8.0, "the upper walkway stands within 8px of mockup row 329: %.1f" % upper_walkway):
		return
	if not _points_to_goal("the first city screen"):
		return
	if not _check(await _pause_shows_ending_goal(), "the pause screen over the city, and not the city, shows what ends the run"):
		return
	if not _check(_actions() == ["TO BASE"] and _drawn("TO BASE") and _drawn("[Q]") and not _drawn_says("INVENTORY") and not _drawn_says("WASD") and not _drawn_says("To base"), "away from a source the prompt offers the way to base, and no inventory button or key line shows: %s %s" % [_actions(), _drawn_lines()]):
		return
	if not _hud_is_top_bar("the first city screen"):
		return
	if not _names_only_prompt_keys("the first city screen"):
		return
	if not _hud_shows_no_health_or_stamina("the first city screen"):
		return
	await _run_east()
	if failed:
		return
	if not await _walk_route(SHELF_ROUTE):
		return
	if not await _until("the AI appraiser takes the shelf's kelp", func() -> bool: return _actor("appraiser", "ingredients") >= 1, 900):
		return
	if not await _emptied_source_is_not_offered("Shop Shelf"):
		return
	if not _hud_is_top_bar("a source"):
		return
	if not _check(_hud_area() < CITY_HUD_AREA_WITH_KEYS, "the city's HUD at a source covers less than it did naming every key: %.0f against %.0f" % [_hud_area(), CITY_HUD_AREA_WITH_KEYS]):
		return
	if not _check(not _next().begins_with("Search ") and _next().contains(" to Tomato machine"), "the screen moves past a kind the party already took: %s" % _next()):
		return
	var stock: int = _stock("Tomato Machine")
	if not await _walk_route([Vector2i(3, 6), Vector2i(3, 8)] + TOMATO_ROUTE):
		return
	walker = "chef"
	if not await _emptied_source_is_not_offered("Tomato Machine"):
		return
	if not _check(_stock("Tomato Machine") < stock, "a take spends the shared container's stock"):
		return
	var remaining: int = _int("remaining_ms")
	if not await _until("time expires", func() -> bool: return _str("phase") == "workshop", DAY_FRAMES):
		return
	if not _check(remaining > 0 and _int("remaining_ms") == 0, "the day's clock ran out in the city"):
		return
	if not _check(_where("chef") == "base" and _where("appraiser") == "base", "expiry returns both actors"):
		return
	_check(_events().has("Time is up: everyone is back at base."), "the screen reports the timed return")

## Beside a source the AI partner shares, waits until the partner has emptied
## it, which it does while it stays within reach of the chef, so the run takes
## one path whichever machine's timing; then an emptied source is not offered
## for a search, only the way to base. The control is that the source still
## stands, so it is the emptiness that withholds the offer.
func _emptied_source_is_not_offered(source: String) -> bool:
	if not await _until("the AI partner empties %s" % source, func() -> bool: return _stock(source) == 0, 900):
		return false
	return _check(_object(source) == "intact" and _actions() == ["TO BASE"] and _target("chef_search").is_empty() and _actor("chef", "searching") == -1, "an emptied source that still stands is not offered for a search, only the way to base: %s" % [_actions()])

## Runs east along the upper walkway towards its step down.
func _run_east() -> void:
	_key(KEY_SHIFT, true)
	_key(KEY_D, true)
	await _until("running reaches the step down", func() -> bool: return _actor("chef", "x") >= 8, 240)
	_key(KEY_D, false)
	_key(KEY_SHIFT, false)
	if failed:
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
	if not await _walk_to(BESIDE_OBSTACLE.x, BESIDE_OBSTACLE.y):
		return
	if not await _camera_settles("the west walkway"):
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
	if not _check(_actor("chef", "ingredients") == pack, "a fall returns to base with the pack kept"):
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
	# The AI partner's trips home to empty its hands take much of the day, so the
	# camera on the east walkway is read once the fall is over.
	if not await _walk_to(24, 8):
		return
	if not await _camera_frames_partner("the east walkway on day three"):
		return
	if not _plates_clear_of_sources("the east walkway on day three"):
		return
	if not _points_to_goal("the east walkway on day three"):
		return
	if not await _centres_pair("walking east onto the east walkway"):
		return
	await _tap(KEY_A)
	if not await _camera_settles("the east walkway facing west"):
		return
	if not await _centres_pair("turning west on the east walkway"):
		return
	if not await _backdrop_slides_along("the east walkway"):
		return
	await _tap(KEY_Q)
	if not await _until("the party is home again", func() -> bool: return _str("phase") == "workshop", 900):
		return
	var day: int = _int("day")
	await _tap(KEY_G)
	await _until("the fall's day ends", func() -> bool: return _int("day") == day + 1, 30)

## Starts a new day: a mutant on its patrol changes position over time, and
## the chef touching the obstacle disappears from the map and, ten seconds of
## exploration time later, reappears beside its partner.
func _touching_an_obstacle_sends_the_actor_away() -> void:
	await _tap(KEY_Q)
	if not await _until("chef re-enters", func() -> bool: return _where("chef") == "city", 30):
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
	var mutants: Array = _status()["mutants"]
	if not _check(mutants.size() >= 3, "the street has patrolling mutants: %d" % mutants.size()):
		return
	var patrolled: Array[int] = []
	for mutant: Dictionary in mutants:
		var sub_x: int = mutant["sub_x"]
		patrolled.append(sub_x)
	await _frames(30)
	var moved: bool = false
	var after: Array = _status()["mutants"]
	for index: int in range(patrolled.size()):
		var now: int = after[index]["sub_x"]
		moved = moved or now != patrolled[index]
	if not _check(moved, "a mutant's position changes over time"):
		return
	if not await _until("the AI partner joins the chef in the city", func() -> bool: return _where("appraiser") == "city", 1200):
		return
	if not await _walk_route([BESIDE_OBSTACLE]):
		return
	_key(WALK_KEYS[5 * _actor(walker, "key_set")], true)
	var gone: bool = await _until("touching the obstacle takes the chef off the map", func() -> bool: return _actor("chef", "absent_ms") >= 0, 120)
	_release()
	if not gone:
		return
	if not _check(_where("chef") == "city" and _where("appraiser") == "city", "the touched chef is away from the map, still in the city"):
		return
	var away_ms: int = _actor("chef", "absent_ms")
	if not _check(away_ms <= ABSENCE_MSEC and away_ms > ABSENCE_MSEC - 1000, "the chef is away for ten seconds: %d ms left" % away_ms):
		return
	if not await _until("the chef reappears beside its partner", func() -> bool: return _actor("chef", "absent_ms") < 0, 900):
		return
	var beside: int = absi(_actor("chef", "sub_x") - _actor("appraiser", "sub_x"))
	# The AI partner may have gone home meanwhile; the chef then reappears
	# beside it at base. `_chef_reappears_in_the_city_beside_a_partner_there`
	# holds the partner in the city and checks the city.
	_check(_where("chef") == _where("appraiser") and beside <= 32, "the chef is back beside its partner, wherever it is: %d sub-cells apart" % beside)

## Tab hands the chef to AI, which works on the next step, so the line read on
## the swap's first frame names that work: cooking with no dish stored, but the
## stored dish once there is one to feed.
func _ai_chef_work_yields_to_a_dish_to_feed() -> void:
	await _leave_run()
	await _new_game()
	await _tap(KEY_1)
	await _tap(KEY_E)
	await _tap(KEY_Q)
	if not await _until("the chef sets out", func() -> bool: return _where("chef") == "city", 30):
		return
	# A collection holds AI for its time, so give it time for a dish's worth.
	if not await _until("AI collects a dish's worth", func() -> bool: return _actor("appraiser", "ingredients") >= 6, DAY_FRAMES):
		return
	await _tap(KEY_Q)
	if not await _until("the party is home", func() -> bool: return _str("phase") == "workshop", COLLECT_FRAMES + 120):
		return
	if not await _until("AI appraises before the chef cooks", func() -> bool: return not _next().contains("identifying"), 300):
		return
	if not _check(_stored().is_empty() and _str("dish_offered") != "", "no dish is stored and one can be cooked"):
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
	if not await _until("the chef sets out again", func() -> bool: return _where("chef") == "city", 30):
		return
	if not await _walk_route(TOMATO_ROUTE):
		return
	await _tap(KEY_E)
	if not await _until("the day's clock brings the party home", func() -> bool: return _str("phase") == "workshop", DAY_FRAMES):
		return
	if not await _until("AI appraises the second day's finds", func() -> bool: return not _next().contains("identifying"), 300):
		return
	if not _check(not _stored().is_empty(), "a dish is stored from the first day"):
		return
	_key(KEY_TAB, true)
	swapped = await _until("the player hands the chef to AI again", func() -> bool: return _actor("chef", "key_set") == -1, 30)
	line = _next()
	_key(KEY_TAB, false)
	if not swapped:
		return
	_check(line.begins_with("Feed "), "a dish to feed comes before the AI chef cooking another: %s" % line)

## The chef and the appraiser stand on the kitchen floor and the feed crane
## hangs over the monster's mouth with the stored dish on it: the base draws
## both over the kitchen picture.
func _base_figures_drawn() -> bool:
	var figures: Array = scene.get_meta("workshop_figures", [])
	var crane: Rect2 = scene.get_meta("feed_crane", Rect2())
	if not _check(figures.size() == 2, "the base draws the appraiser and the chef: %d figures" % figures.size()):
		return false
	for figure: Rect2 in figures:
		if not _check(figure.end.y > 440.0 and figure.end.y < 560.0 and figure.size.y > 100.0, "a worker stands on the kitchen floor: %s" % figure):
			return false
	return _check(crane.size.x > 100.0 and crane.size.y > 40.0 and crane.position.x > 700.0, "the feed crane is drawn over the monster's mouth: %s" % crane)


## How far down the feed crane's hook and dish hang.
func _crane_height() -> float:
	var crane: Rect2 = scene.get_meta("feed_crane", Rect2())
	return crane.size.y


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
	if not _base_figures_drawn():
		return
	var fed: String = _stored()[0]
	if not _check(_lines("day_result").is_empty(), "the workshop shows no result until a day has ended"):
		return
	var resting: float = _crane_height()
	await _tap(KEY_F)
	if not await _until("the feed crane lowers its dish", func() -> bool: return _crane_height() > resting + 20.0, 30):
		return
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
	await _new_game()
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
	if not await _until("the party comes home", func() -> bool: return _str("phase") == "workshop", 900):
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
	if not await _until("AI gathers a dish's worth", func() -> bool: return _actor("appraiser", "ingredients") >= 6, 900):
		return
	await _tap(KEY_Q)
	if not await _until("the party comes back early", func() -> bool: return _str("phase") == "workshop", COLLECT_FRAMES + 120):
		return
	if not await _until("AI appraises before the chef cooks", func() -> bool: return _int("identified") >= 2 and not _next().contains("identifying"), 900):
		return
	await _cook()
	if failed:
		return
	if not _check(_int("remaining_ms") > 0 and _next().contains("Set out"), "with time and stock left the screen sets out before feeding"):
		return
	if not _check(_drawn_says("SERVE") and _drawn_says("NEEDS 100") and not _target("chef_feed").is_empty(), "the feed panel keeps the stored dish and its serve button in view: %s" % [_drawn_lines()]):
		return
	await _tap(KEY_Q)

## Plays on what NEXT says alone to the ending. A dish the monster hated stays
## stored, so the line may end a day unfed beside it, but never beside a dish it
## would feed.
func _follow_next_to_the_ending() -> void:
	await _leave_run()
	await _new_game()
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
	if not _check(_str("phase") == "ended", "following NEXT alone ends the run satisfied"):
		return
	_check(not hated.is_empty(), "the run met a dish the monster hated")
	scene.pin_survival_seed(SEED)

## A broken passage is never an errand, so once the broken passages lead to
## nothing the line stops setting out.
func _a_broken_passage_with_no_stock_behind_stops_setting_out() -> void:
	# A second player keeps the appraiser home, so the chef alone empties the
	# roof bag before acid breaks its ladder.
	await _start_two_players()
	await _tap(KEY_Q)
	if not await _until("the chef sets out alone for the roof bag", func() -> bool: return _where("chef") == "city" and _where("appraiser") == "base", 30):
		return
	if not await _walk_route(ROOF_ROUTE):
		return
	walker = "chef"
	var bag: int = _stock("Roof Bag")
	if not await _search_and_take(mini(bag, 6)):
		return
	if not _check(_stock("Roof Bag") == 0 and _object("Roof Ladder") == "intact", "the roof bag is emptied while its ladder stands"):
		return
	await _tap(KEY_Q)
	if not await _until("the chef comes home from the roof", func() -> bool: return _str("phase") == "workshop", COLLECT_FRAMES + 120):
		return
	# Four unfed days bring the cracked stage, which breaks the ladder.
	for day: int in range(4):
		await _tap(KEY_G)
		if not await _until("an unfed day ends", func() -> bool: return _int("day") == day + 2 and _str("phase") == "workshop", 30):
			return
	if not await _until("the cracked stage breaks the ladder", func() -> bool: return _object("Roof Ladder") == "destroyed", 30):
		return
	# The rest is one-player play, with the AI appraiser.
	await _tap(KEY_N)
	if not await _until("player two hands the appraiser to AI", func() -> bool: return _actor("appraiser", "key_set") == -1 and _actor("chef", "key_set") == 0, 30):
		return
	if not _check(_object("Roof Ladder") == "destroyed" and _stock("Roof Bag") == 0, "a broken passage is left, with no stock behind it"):
		return
	if not _check(_int("remaining_ms") > 0 and not _next().contains(_place("Roof Bag")), "a broken passage with nothing behind it is not an errand"):
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
	if not _check(not _next().contains(_place("Roof Bag")) and not _next().contains("Repair"), "beside the dead passage the line sends no one up it: %s" % _next()):
		return
	await _tap(KEY_E)
	_check(_object("Roof Ladder") == "destroyed", "no key repairs the broken ladder")

## Whether NEXT names what the interact key does: appraise or cook, as the controls page lists for it.
func _names_interact(line: String) -> bool:
	for verb: String in ["Appraise ", "Cook "]:
		if line.begins_with(verb):
			return true
	return false

## Presses the key for the one action NEXT names, as the controls page lists
## it, or waits while it names none.
func _follow_next() -> void:
	await _wait_out_absence()
	var line: String = _next()
	if line.begins_with("Inventory open"):
		await _tap(KEY_R)
	elif line.contains("Set out") or line.contains("Nothing is within reach"):
		await _tap(KEY_Q)
	elif line.begins_with("Feed "):
		await _tap(KEY_F)
	elif line.begins_with("End the day"):
		await _tap(KEY_G)
	elif line.begins_with("Search "):
		await _tap(KEY_R)
	elif line.begins_with("Searching "):
		await _tap(KEY_E)
		await _tap(KEY_R)
	elif _names_interact(line):
		await _tap(KEY_E)
	elif line.begins_with("Walk ") or line.begins_with("Climb ") or line.begins_with("Jump "):
		await _walk_a_cell(line)
	else:
		await _frames(12)

## Waits, like the AI, while a mutant of the walker's row is or soon will be in
## the cells the next step crosses: within three cells ahead, which a mutant
## covers in under a second. A patrol that never leaves the way is left after
## `MUTANT_WAIT_STEPS` waits, and a touch is then waited out.
func _let_mutants_pass(sign: int) -> void:
	for waited in MUTANT_WAIT_STEPS:
		var here: int = _actor(walker, "sub_x")
		var row: int = _actor(walker, "sub_y")
		var near: bool = false
		for mutant: Dictionary in _mutants():
			var mutant_x: int = mutant["sub_x"]
			var mutant_y: int = mutant["sub_y"]
			var ahead: int = (mutant_x - here) * sign
			if mutant_y == row and ahead >= -MUTANT_CLEARANCE and ahead <= 3 * 32:
				near = true
		if not near:
			return
		await _frames(6)

## Waits until a walker that touched a mutant or an obstacle is back.
func _wait_out_absence() -> void:
	for waited in ABSENCE_WAIT_STEPS:
		if _actor(walker, "absent_ms") == -1:
			return
		await _frames(6)

## Moves the way NEXT names: a press and release in one frame walks or climbs
## one step at any frame length. A jump, or a step off an edge, holds its way
## until it lands, as the route it was planned on does.
func _walk_a_cell(line: String) -> void:
	var code: Key = ROUTE_KEYS.get(line.get_slice(" ", 1), KEY_NONE)
	if line.begins_with("Walk ") and (code == KEY_A or code == KEY_D):
		await _let_mutants_pass(1 if code == KEY_D else -1)
	if code == KEY_NONE:
		await _frames(STEP_FRAMES)
		return
	if line.begins_with("Jump "):
		_key(code, true)
		_key(KEY_SPACE, true)
		_key(KEY_SPACE, false)
		# The clock can end the trip before the jump leaves the ground.
		await _until("the jump takes off", func() -> bool: return _stance(walker) == "airborne" or _str("phase") != "exploration", 30)
	else:
		_key(code, true)
		_key(code, false)
		await _frames(STEP_FRAMES)
		if _stance(walker) != "airborne":
			return
		_key(code, true)
	await _until("the move lands", func() -> bool: return _stance(walker) != "airborne", 120)
	_key(code, false)

func _the_days_change_the_city() -> void:
	if not _check(_object("Noodle Vendor") == "intact" and _object("Drifted Ferry") == "dormant", "the vendor stands and the ferry is away"):
		return
	if not _check(_int("day") == 4 and _int("acid") == 12 and _str("reaction") == "unfed", "an unfed day raises acid by the penalty"):
		return
	if not _check(_result_says("Day 3: Unfed") and _result_says("Acid 0 to 12."), "the result of an unfed day shows the acid it cost"):
		return
	if not await _until("the unfed result counts no map change on the standing days", func() -> bool: return _result_says("Map changes: 0"), 30):
		return
	if not await _until("the screen reports the unfed day", func() -> bool: return _next_event_has("unfed"), 30):
		return
	if not await _until("the standing stage keeps the ladder", func() -> bool: return _object("Roof Ladder") == "intact", 30):
		return
	if not _check(_object("Washed-up Bag 10") == "dormant", "a stage brings no bag before its day"):
		return
	if not _check(_int("elapsed_ms") == 54000 and _object("Washed-up Bag 7") == "intact", "every finished day passes its whole time and lands the bag"):
		return
	await _search_the_vendor_cart()
	if failed:
		return
	await _tap(KEY_G)
	await _until("the second unfed day ends", func() -> bool: return _int("day") == 5, 30)
	if not await _until("the cracked stage breaks the ladder and acid destroys nothing", func() -> bool: return _object("Roof Ladder") == "destroyed" and _object("Noodle Vendor") == "intact", 30):
		return
	await _tap(KEY_G)
	await _until("the third unfed day ends", func() -> bool: return _int("day") == 6, 30)
	await _until("the cracked stage brings the ferry", func() -> bool: return _object("Drifted Ferry") == "intact", 30)
	_check(_object("Drifted Ferry") == "intact" and _int("acid") == 36, "the cracked stage brings the ferry in while acid rises")

func _search_the_vendor_cart() -> void:
	await _tap(KEY_Q)
	if not await _until("the chef sets out for the cart", func() -> bool: return _where("chef") == "city", 30):
		return
	if not await _walk_route(VENDOR_ROUTE):
		return
	# Let go of E held since setting out before pressing it at the machine.
	await _frames(2 * STEP_FRAMES)
	if not await _emptied_source_is_not_offered("Vendor Cart"):
		return
	await _tap(KEY_R)
	if not _check(_screen_open(0) and _actor("chef", "searching") == -1 and _actions() == ["CLOSE"], "R at an emptied source acid never destroys opens the inventory alone: %s" % [_actions()]):
		return
	await _tap(KEY_R)
	await _tap(KEY_Q)
	await _until("the party is home from the cart", func() -> bool: return _str("phase") == "workshop", COLLECT_FRAMES + 120)

func _appraiser_run() -> void:
	await _leave_run()
	if not _check(scene.screen_name() == "main" and _status().is_empty(), "M ends the run"):
		return
	await _new_game()
	await _tap(KEY_2)
	await _tap(KEY_E)
	if not _check(_actor("appraiser", "key_set") == 0 and _actor("chef", "key_set") == -1, "choosing the appraiser gives AI the chef"):
		return
	if not _check(_int("day") == 1 and _int("acid") == 0 and _object("Noodle Vendor") == "intact", "a new run starts fresh"):
		return
	walker = "appraiser"
	await _tap(KEY_Q)
	if not await _walk_route(TOMATO_ROUTE):
		return
	if not _check(_next().begins_with("Search ") and _next().contains("Tomato machine"), "the screen offers the tomato machine in reach: %s" % _next()):
		return
	if not await _search_and_take(1):
		return
	if not _check(_actor("appraiser", "ingredients") == 1, "the human takes an ingredient"):
		return
	if not await _until("AI collects", func() -> bool: return _actor("chef", "ingredients") >= 2, DAY_FRAMES):
		return
	await _tap(KEY_Q)
	if not await _until("the party is back at base", func() -> bool: return _str("phase") == "workshop", COLLECT_FRAMES + 120):
		return
	if not _check(_next().contains("Appraise") and _str("appraise_job").begins_with("Appraise "), "the human appraiser is told to appraise"):
		return
	var by_mouse: int = _int("identified")
	# An appraisal can fail and leave the ingredient: the click is made again.
	var clicks: int = 0
	var clicked: bool = true
	while clicked and _int("identified") == by_mouse and clicks < 6:
		clicked = await _click_on(_target_at(APPRAISER_HEAD))
		clicks += 1
	if not _check(clicked and _int("identified") == by_mouse + 1, "a click on the ingredients over the appraiser's head appraises, as E does"):
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
	await _a_broken_ladder_stays_broken()

## An AI chef cooks a step after the last appraisal, so NEXT must not end that day.
func _appraiser_follows_next() -> void:
	await _leave_run()
	await _new_game()
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

func _a_broken_ladder_stays_broken() -> void:
	await _tap(KEY_Q)
	if not await _until("day two runs out", func() -> bool: return _int("remaining_ms") == 0 and _str("phase") == "workshop", DAY_FRAMES):
		return
	if not await _until("AI cooks day two's finds", func() -> bool: return _int("foods") >= 1, 300):
		return
	await _tap(KEY_F)
	if not await _until("the appraiser feeds day two's dish", func() -> bool: return _int("day") == 3, 30):
		return
	# Two unfed days more bring the cracked stage, which breaks the ladder.
	for day: int in range(2):
		await _tap(KEY_G)
		if not await _until("an unfed day ends", func() -> bool: return _int("day") == day + 4 and _str("phase") == "workshop", 30):
			return
	if not await _until("the cracked stage breaks the ladder", func() -> bool: return _object("Roof Ladder") == "destroyed", 30):
		return
	await _tap(KEY_Q)
	if not await _walk_route(LADDER_ROUTE):
		return
	if not _check(not _next().contains("Repair") and not _actions().has("REPAIR " + _place("Roof Ladder")) and not _drawn("REPAIR " + _place("Roof Ladder")), "the screen offers no repair beside the broken ladder: %s" % [_actions()]):
		return
	_key(KEY_W, true)
	await _frames(24)
	_key(KEY_W, false)
	await _frames(12)
	if not _check(_actor("appraiser", "y") == 6 and _stance("appraiser") == "standing", "a broken ladder blocks the way up"):
		return
	await _tap(KEY_E)
	_check(_object("Roof Ladder") == "destroyed", "E does not mend the broken ladder")

## The second key set's own keys, which the timing checks inject past.
func _two_players() -> void:
	await _start_two_players()
	if not _check(_actor("chef", "key_set") == 0 and _actor("appraiser", "key_set") == 1, "two players take one key set each"):
		return
	var keys: Array[String] = _set_out_keys()
	var drawn_tip: Dictionary = scene.get_meta("hud_tip", {})
	var two_tip: String = str(drawn_tip.get("text", ""))
	if not _check(_next().begins_with("Set out into the city") and two_tip == SET_OUT_TIP + " Set out with [%s] (P1) or [%s] (P2)." % [keys[0], keys[1]], "the base line tells two players to set out, and the set-out tip names both keys: %s %s" % [_next(), two_tip]):
		return
	if not _text_fits("two players at base before setting out"):
		return
	await _tap(KEY_Q)
	if not await _until("player one sets out alone", func() -> bool: return _where("chef") == "city", 30):
		return
	if not _check(_next().contains("P1: Walk ") and _next().contains("P2: Set out into the city"), "the player left at base is told to set out: %s" % _next()):
		return
	if not _check(not _drawn_says("IJKL") and not _drawn_says("WASD"), "exploration draws no key line: %s" % [_drawn_lines()]):
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
	if not _check(not _drawn_says("IJKL") and not _drawn_says("WASD") and _hud_is_top_bar("two players in the city"), "two players in the city draw no key line, only the top bar: %s" % [_drawn_lines()]):
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

## Real keys on the transfer screens: a search opens the container beside the
## inventory and a take lands in the pack; at base the same screen stores into
## the fridge and withdraws from it, and what the fridge holds is still there
## on the next day.
func _search_screen_and_fridge() -> void:
	await _start_two_players()
	walker = "chef"
	await _tap(KEY_Q)
	if not await _until("player one sets out", func() -> bool: return _where("chef") == "city", 30):
		return
	if not await _walk_route(TOMATO_ROUTE):
		return
	await _frames(2 * STEP_FRAMES)
	var stock: int = _stock("Tomato Machine")
	await _tap(KEY_R)
	if not _check(_screen_open(0) and _actor("chef", "searching") >= 0 and _actor("chef", "searching_items") == stock, "the search key opens the container beside the inventory"):
		return
	await _tap(KEY_E)
	if not _check(_actor("chef", "ingredients") == 1 and _stock("Tomato Machine") == stock - 1, "the interact key moves the item under the cursor into the inventory"):
		return
	await _tap(KEY_R)
	if not _check(not _screen_open(0) and _actor("chef", "searching") == -1, "the search key closes the screen"):
		return
	await _tap(KEY_Q)
	if not await _until("player one is home", func() -> bool: return _where("chef") == "base", COLLECT_FRAMES + 600):
		return
	var fridge: int = _int("fridge")
	await _tap(KEY_R)
	if not _check(_screen_open(0), "the search key opens the inventory and the fridge at base"):
		return
	await _tap(KEY_S)
	await _tap(KEY_E)
	if not _check(_int("fridge") == fridge + 1 and _actor("chef", "ingredients") == 0, "an item moves from the inventory into the fridge"):
		return
	await _tap(KEY_W)
	await _tap(KEY_E)
	if not _check(_int("fridge") == fridge and _actor("chef", "ingredients") == 1, "an item moves from the fridge back into the inventory"):
		return
	await _tap(KEY_S)
	await _tap(KEY_E)
	await _tap(KEY_R)
	if not _check(_int("fridge") == fridge + 1 and not _screen_open(0), "the fridge holds what was stored once the screen is closed"):
		return
	# By mouse: the fridge button on the chef's block opens the same screen, and
	# while it is open the chef refuses a click until it is closed.
	if not _check(await _click("chef_search") and _screen_open(0), "a click on the chef's fridge button opens the inventory and the fridge at base"):
		return
	var kitchen_chef: Dictionary = _target_where("chef_interact", func(area: Rect2) -> bool: return area.position.y < SCREEN_MIDDLE)
	if not _check(not kitchen_chef.is_empty() and str(kitchen_chef["refusal"]) == CLOSE_FIRST, "with the fridge open a click on the chef is refused with the close step: %s" % [_base_state()]):
		return
	if not _check(await _click("chef_other_%d" % fridge) and _int("fridge") == fridge and _actor("chef", "ingredients") == 1, "a click on a fridge slot withdraws the item into the pack"):
		return
	if not _check(await _click("chef_mine_0") and _int("fridge") == fridge + 1 and _actor("chef", "ingredients") == 0, "a click on a pack slot stores the item in the fridge"):
		return
	if not _check(await _click("chef_search") and not _screen_open(0) and str(_target_where("chef_interact", func(area: Rect2) -> bool: return area.position.y < SCREEN_MIDDLE).get("refusal", "")) != CLOSE_FIRST, "a click on the fridge button closes the screen and the chef stops refusing"):
		return
	var day: int = _int("day")
	await _tap(KEY_G)
	if not await _until("the day ends unfed", func() -> bool: return _int("day") == day + 1, 30):
		return
	_check(_int("fridge") == fridge + 1, "the fridge keeps its contents into the next day")

## Two humans: player two stays where it is in the city while player one
## touches the obstacle, so the chef must reappear in the city beside it.
func _chef_reappears_in_the_city_beside_a_partner_there() -> void:
	await _start_two_players()
	walker = "chef"
	await _tap(KEY_Q)
	if not await _until("player one sets out", func() -> bool: return _where("chef") == "city", 30):
		return
	await _tap(KEY_U)
	if not await _until("player two sets out", func() -> bool: return _where("appraiser") == "city", 30):
		return
	if not await _walk_route([BESIDE_OBSTACLE]):
		return
	if not _check(_where("appraiser") == "city" and _actor("appraiser", "absent_ms") < 0, "setup: the second player stays in the city while the chef touches the obstacle"):
		return
	_key(WALK_KEYS[5 * _actor(walker, "key_set")], true)
	var gone: bool = await _until("touching the obstacle takes the chef off the map", func() -> bool: return _actor("chef", "absent_ms") >= 0, 120)
	_release()
	if not gone:
		return
	if not await _until("the chef reappears", func() -> bool: return _actor("chef", "absent_ms") < 0, 900):
		return
	var beside: int = absi(_actor("chef", "sub_x") - _actor("appraiser", "sub_x"))
	_check(_where("chef") == "city" and _where("appraiser") == "city" and beside <= 32, "the chef is back in the city beside its partner: %d sub-cells apart" % beside)

## Player two searches while player one stays home: the search keeps player
## two in place and its button reads close until the search ends.
func _player_two_searches_and_holds_in_place() -> void:
	await _start_two_players()
	walker = "appraiser"
	await _tap(KEY_U)
	if not await _until("player two sets out for the tomato machine", func() -> bool: return _where("appraiser") == "city", 30):
		return
	if not await _walk_route(TOMATO_ROUTE):
		return
	await _tap(KEY_P)
	await _tap(KEY_O)
	if not _check(_actor("appraiser", "ingredients") == 1, "player two carries the ingredient"):
		return
	await _frames(2)
	if not _check(_actor("appraiser", "searching") >= 0 and _actions().size() == 2 and _action(1) == "CLOSE" and _action(0) != "CLOSE", "player two's button reads close while player two searches: %s" % [_actions()]):
		return
	if not _check(_drawn("CLOSE") and _drawn("[P]") and _drawn("P2"), "player two's prompt tags the player and names the search key: %s" % [_drawn_lines()]):
		return
	if not _text_fits("player two's search"):
		return
	await _tap(KEY_P)
	if not await _until("player two's search closes", func() -> bool: return _actor("appraiser", "searching") == -1, COLLECT_FRAMES):
		return
	await _frames(2)
	_check(_actions().size() == 2 and _action(1) == "SEARCH Tomato machine" and _drawn("SEARCH Tomato machine"), "player two's prompt names its action again once the search ends: %s" % [_actions()])

## Sets out if at base, walks the waypoints and takes `times` items at the last.
func _collect_along(waypoints: Array[Vector2i], times: int) -> bool:
	if _where(walker) == "base":
		await _tap(KEY_Q)
	for waypoint: Vector2i in waypoints:
		if not await _walk_to(waypoint.x, waypoint.y):
			return false
	return await _search_and_take(times)

## The search key of the walker's key set.
func _search_key() -> Key:
	return KEY_P if _actor(walker, "key_set") == 1 else KEY_R

## The interact key of the walker's key set.
func _interact_key() -> Key:
	return KEY_O if _actor(walker, "key_set") == 1 else KEY_E

## Opens the container in reach with the walker's search key, takes the first
## `times` items with its interact key and closes the search. A `shared`
## container, named by its source, may hold fewer by then, and fewer are taken.
func _search_and_take(times: int, shared: String = "") -> bool:
	var pack: int = _actor(walker, "ingredients")
	# A container another actor has open is refused, so the key is pressed again
	# once that actor has closed it.
	for attempt: int in range(SEARCH_ATTEMPTS):
		# A container the partner emptied is not offered, and nothing is left to take.
		if not shared.is_empty() and _stock(shared) == 0:
			return true
		await _tap(_search_key())
		if _actor(walker, "searching") >= 0:
			break
		await _frames(30)
	if not _check(_actor(walker, "searching") >= 0, "the search key opens the container in reach"):
		return false
	# With the container shared with AI, what is left may be less than wanted.
	var want: int = mini(times, _actor(walker, "searching_items")) if not shared.is_empty() else times
	for time: int in range(want):
		await _tap(_interact_key())
	var took: bool = _check(_actor(walker, "ingredients") == pack + want, "%s takes %d from the container" % [walker, want])
	await _tap(_search_key())
	if not await _until("the search closes", func() -> bool: return _actor(walker, "searching") == -1, 30):
		return false
	return took

func _loss_clears_the_offer() -> void:
	await _leave_run()
	await _new_game()
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

## With a clock on the base and a cook that takes time, the top bar shows the
## base's time and the cook panel draws the running cook: its name, COOKING
## with a percentage that follows the snapshot, the bar's panel, and a second
## cook refused with the cooking message and not the fridge's.
func _a_timed_cook_draws_its_bar() -> void:
	await _leave_run()
	OS.set_environment("NEOCITY_BASE_SECONDS", "300")
	OS.set_environment("NEOCITY_COOK_SECONDS", "20")
	await _new_game()
	await _tap(KEY_1)
	await _tap(KEY_E)
	await _tap(KEY_Q)
	if not await _until("the timed run's day ends", func() -> bool: return _int("remaining_ms") == 0 and _str("phase") == "workshop", DAY_FRAMES):
		_restore_clocks()
		return
	if not await _until("AI appraises the timed run's finds", func() -> bool: return _str("dish_offered") != "" and not _next().contains("identifying"), 300):
		_restore_clocks()
		return
	var shown: String = _clock_drawn()
	var remaining: int = _int("base_remaining_ms")
	var left: int = roundi(remaining / 1000.0)
	var seen: int = _clock_seconds(shown)
	if not _check(_flag("base_timed") and seen >= 0 and absi(seen - left) <= 2, "the top bar shows the base's own time: %s against %s s" % [shown, left]):
		_restore_clocks()
		return
	var attempts: int = 0
	while not _flag("cooking") and attempts < 6:
		attempts += 1
		await _tap(KEY_E)
	if not _check(_flag("cooking") and _drawn_says("COOKING"), "a timed cook is drawn as COOKING on the cook panel: %s" % [_drawn_lines()]):
		_restore_clocks()
		return
	await _frames(120)
	var progress: float = _float("cooking_progress")
	var percent: int = roundi(progress * 100.0)
	var drawn_percent: int = _cooking_percent()
	if not _check(progress > 0.0 and progress < 1.0 and absi(drawn_percent - percent) <= 3, "the cook panel's percentage follows the snapshot: %s against %s" % [drawn_percent, percent]):
		_restore_clocks()
		return
	if not _check(_drawn_says(str(_status()["cooking_food"])), "the cook panel names the dish being cooked: %s" % [_drawn_lines()]):
		_restore_clocks()
		return
	var foods: int = _int("foods")
	await _tap(KEY_E)
	_check(_int("foods") == foods and _flag("cooking"), "a second cook is refused while one runs")
	_restore_clocks()

func _flag(field: String) -> bool:
	var value: bool = _status()[field]
	return value

func _float(field: String) -> float:
	var value: float = _status()[field]
	return value

func _restore_clocks() -> void:
	OS.set_environment("NEOCITY_BASE_SECONDS", "0")
	OS.set_environment("NEOCITY_COOK_SECONDS", "0")

## The top bar's clock as drawn, the one line of two digits, a colon and two
## digits; empty when none was drawn.
func _clock_drawn() -> String:
	for line: String in _drawn_lines():
		if line.length() == 5 and line[2] == ":":
			return line
	return ""

func _clock_seconds(clock: String) -> int:
	if clock.is_empty():
		return -1
	return int(clock.substr(0, 2)) * 60 + int(clock.substr(3, 2))

## The percentage the cook panel's COOKING line drew, -1 when none.
func _cooking_percent() -> int:
	for line: String in _drawn_lines():
		if line.begins_with("COOKING ") and line.ends_with("%"):
			return int(line.substr(8, line.length() - 9))
	return -1

func _mutants() -> Array:
	return _status()["mutants"]

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

## Pauses and continues, and tells whether the ending goal was drawn over
## the pause screen and not on the screen under it. The screen and the pause
## over it each draw every word at a whole multiple of the font's grid.
func _pause_shows_ending_goal() -> bool:
	var before: bool = _drawn_says(ENDING_GOAL)
	if not _text_on_grid("the screen under the pause"):
		return false
	await _tap(KEY_ESCAPE)
	var paused: bool = _str("interruption") == "paused" and _drawn_says(ENDING_GOAL)
	if not _text_on_grid("the pause screen"):
		return false
	await _tap(KEY_ESCAPE)
	return _str("goal") == ENDING_GOAL and not before and paused and _str("interruption") == ""

## The lines the screen drew in its last frame, in drawing order.
func _drawn_lines() -> PackedStringArray:
	return scene.get_meta("screen_text", PackedStringArray())

## Whether every word the last frame drew was drawn at a whole multiple of the
## font's grid, 16 layout units at the least, and the font is imported without
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

## The name the screen gives a city object.
func _place(object: String) -> String:
	return str(_status()["places"][object])

## What each human's action prompt reads, the chef's first; empty while hidden.
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

## Whether the last frame drew no Esc and no key cap but the keys of
## the action prompts, such as "[R]", and the set-out key of the set-out tip,
## while it drew the top bar's words.
func _names_only_prompt_keys(where: String) -> bool:
	var keyed: Array[String] = []
	for line: String in _drawn_lines():
		var bare: String = _without_set_out_key(line)
		if bare.contains("Esc") or (bare.contains("[") and not PROMPT_CAPS.has(bare)):
			keyed.append(line)
	return _check(keyed.is_empty() and _top_bar_drawn(), "%s names no key but the prompts' keys and the set-out tip's: %s" % [where, keyed])

## Whether the last frame drew the day, the acid and the clock.
func _top_bar_drawn() -> bool:
	var day: bool = false
	var acid: bool = false
	var clock: bool = false
	for line: String in _drawn_lines():
		day = day or line.begins_with("DAY ")
		acid = acid or line.begins_with("ACID ")
		clock = clock or (line.length() == 5 and line[2] == ":")
	return day and acid and clock

## Whether the city's HUD, with no screen open, is the three plates in the top
## band and one prompt below it for each human with an action, with no goal
## panel; a notice or any other bar or cell fails it.
func _hud_is_top_bar(where: String) -> bool:
	var panels: Array = scene.get_meta("hud_panels", [])
	var top: int = 0
	var lower: int = 0
	for panel: Rect2 in panels:
		if panel.end.y <= TOP_BAND_END:
			top += 1
		elif panel.position.y >= SCREEN.size.y / 2.0:
			lower += 1
	var prompts: int = _actions().filter(func(action: String) -> bool: return action != "").size()
	return _check(top == 3 and lower == prompts and panels.size() == 3 + prompts and _top_bar_drawn(), "%s draws the three plates and %d prompts and no other bar or cell: %s" % [where, prompts, panels])

## Every place the map names has its prompt among the changing lines, so the
## measure covers each name and not only some; the log is not drawn.
func _every_place_measured() -> bool:
	var texts: Array[String] = []
	for line: Dictionary in _changing_text():
		texts.append(str(line["text"]))
	var places: Dictionary = _status()["places"]
	var missing: Array[String] = []
	for object: String in places:
		var place: String = str(places[object])
		var listed: bool = texts.has("SEARCH " + place)
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

## A washed-up bag the tide carried off, or one emptied, is not drawn; the
## controls are a bag still holding stock, and the vendor's cart, which the
## day stages never take.
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
	return _check(not gone.is_empty() and hidden and shown and drawn.has("Vendor Cart") and _object("Noodle Vendor") == "intact", "a bag carried off or emptied leaves the map while the rest stay: gone %s, drawn %s" % [gone, drawn])

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

## No actor's tag stands within TAG_CLEARANCE of another plate, a place's name
## among them, so a tag is not read as part of a name; the control is that the
## map drew a tag and a plate that is not one.
func _tags_clear_of_names(where: String) -> bool:
	var plates: Array = scene.get_meta("map_plates", [])
	var words: Array = scene.get_meta("map_words", [])
	var tags: int = 0
	var crowded: Array[String] = []
	for tag: int in range(plates.size()):
		if not ACTOR_TAGS.has(words[tag]):
			continue
		tags += 1
		var tag_box: Rect2 = plates[tag]
		for other: int in range(plates.size()):
			var other_box: Rect2 = plates[other]
			if other != tag and tag_box.grow(TAG_CLEARANCE).intersects(other_box):
				crowded.append("%s %s near %s %s" % [words[tag], tag_box, words[other], other_box])
	return _check(tags > 0 and plates.size() > tags and crowded.is_empty(), "an actor's tag keeps clear of every other plate on %s: %d tags, %d plates, %s" % [where, tags, plates.size(), crowded])

## Where a human's feet are in the scene.
func _feet(who: String) -> Vector2:
	return MAP_ORIGIN + Vector2(_actor(who, "sub_x"), _actor(who, "sub_y"))

## The actors out in the city, human or AI.
func _humans_out() -> Array[String]:
	var out: Array[String] = []
	for who: String in ["chef", "appraiser"]:
		if _where(who) == "city":
			out.append(who)
	return out

## Once the camera has settled: every actor in the city stands on the play
## area; the view shows nothing past the map's sides, beyond EDGE_ROOM, or the
## scene's top and bottom, and centres on the map when wider than it; one human is drawn at
## twice the scene's size, and two at twice or once, never twice when they
## stand APART or more.
func _camera_holds(where: String) -> bool:
	if not await _camera_settles(where):
		return false
	var view: Transform2D = scene.get_meta("map_view", Transform2D())
	var zoom: float = view.x.x
	var humans: Array[String] = _humans_out()
	var shown: bool = humans.all(func(who: String) -> bool: return PLAY.has_point(view * _feet(who)))
	var left: float = (view * MAP_ORIGIN).x
	var map_size: Vector2 = scene.get_node("World").get_meta("map_size")
	var right: float = (view * (MAP_ORIGIN + map_size)).x
	var inside: bool = (view * Vector2.ZERO).y <= 0.0 and (view * Vector2(0, SCENE_HEIGHT)).y >= SCENE_HEIGHT
	if right - left >= PLAY.size.x:
		inside = inside and left <= EDGE_ROOM * zoom and right >= PLAY.size.x - EDGE_ROOM * zoom
	else:
		inside = inside and absf((left + right) / 2.0 - PLAY.size.x / 2.0) <= 1.0
	var framed: bool = is_equal_approx(zoom, 2.0)
	if humans.size() == 2:
		var apart: Vector2 = (_feet("chef") - _feet("appraiser")).abs()
		framed = zoom >= 1.0 and zoom <= 2.0 and (zoom < 2.0 or (apart.x < APART and apart.y < APART / 4.0))
	return _check(not humans.is_empty() and shown and inside and framed, "the camera frames the humans on %s: zoom %.2f, humans %s at %s, map from x %.0f to %.0f" % [where, zoom, humans, humans.map(func(who: String) -> Vector2: return view * _feet(who)), left, right])

## The camera frames an AI partner as it does a human: both stand on the play
## area. The control is that the AI partner is out in the city.
func _camera_frames_partner(where: String) -> bool:
	if not _check(_where("appraiser") == "city" and _actor("appraiser", "key_set") == -1, "the AI partner is out on %s" % where):
		return false
	# The camera follows the AI partner, so it rests only once the partner does.
	var rest: Array[int] = [0, -1, -1]
	var at_rest: Callable = func() -> bool:
		var sub_x: int = _actor("appraiser", "sub_x")
		var sub_y: int = _actor("appraiser", "sub_y")
		rest[0] = rest[0] + 1 if sub_x == rest[1] and sub_y == rest[2] else 0
		rest[1] = sub_x
		rest[2] = sub_y
		return rest[0] >= REST_FRAMES
	if not await _until("the AI partner comes to rest on %s" % where, at_rest, 900):
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
		# An AI partner in the city keeps walking and the camera frames it too, so
		# after a while a camera within SETTLE_SLACK of its heading is at rest.
		# Measured on the east walkway on day three: 12 px across, 18 px down
		# (about 22 px) while the appraiser jumped.
		if Time.get_ticks_msec() - started > SETTLE_LIMIT_MSEC / 2 and is_equal_approx(view.x.x, heading.x.x) and view.origin.distance_to(heading.origin) <= SETTLE_SLACK:
			return true
		OS.delay_msec(SETTLE_POLL_MSEC)
		await process_frame
	return _check(false, "the camera comes to rest on %s: drawn %s, heading %s" % [where, scene.get_meta("map_view", null), scene.get_meta("map_heading", null)])

## Once the camera has settled on two actors in the city, it centres on the
## middle of the two rather than looking ahead of one.
func _centres_pair(where: String) -> bool:
	if not await _camera_settles(where):
		return false
	var view: Transform2D = scene.get_meta("map_view", Transform2D())
	var middle: float = (view * _feet("chef")).x / 2.0 + (view * _feet("appraiser")).x / 2.0
	# An AI partner walking between containers keeps the camera a step behind;
	# the middle is read on a frame the camera has caught up.
	for frame: int in range(120):
		if absf(middle - FOCUS.x) <= 1.0:
			break
		await process_frame
		view = scene.get_meta("map_view", Transform2D())
		middle = (view * _feet("chef")).x / 2.0 + (view * _feet("appraiser")).x / 2.0
	return _check(absf(middle - FOCUS.x) <= 1.0, "the camera centres the two actors %s: middle at x %.1f, expected %.1f" % [where, middle, FOCUS.x])

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

## The camera frames both actors, so an AI partner at another height changes
## the zoom between two walks. This walks the chef along the walkway between
## its two ends until two settled cameras share a zoom, then checks the slide.
func _backdrop_slides_along(where: String) -> bool:
	for attempt: int in range(4):
		if not await _walk_to(20, 8) or not await _camera_settles(where):
			return false
		var west: Dictionary = _backdrop()
		if not await _walk_to(28, 8) or not await _camera_settles(where):
			return false
		var east: Dictionary = _backdrop()
		var west_view: Transform2D = west["view"]
		var east_view: Transform2D = east["view"]
		if is_equal_approx(west_view.x.x, east_view.x.x) and absf(east_view.origin.x - west_view.origin.x) >= 100.0:
			return _backdrop_slides(west, east, "along " + where)
	return _check(false, "two settled cameras share a zoom along %s" % where)

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
## source, and every plate under a HUD panel.
func _plates_over_sources() -> Dictionary:
	var plates: Array = scene.get_meta("map_plates", [])
	var areas: Array = scene.get_meta("map_source_areas", [])
	var count_plates: Array = scene.get_meta("map_counts", [])
	var panels: Array = scene.get_meta("hud_panels", [])
	var counts: int = 0
	var covered: Array[String] = []
	for plate: Rect2 in plates:
		for area: Rect2 in panels:
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

## Whether the city's HUD reports no health and no stamina: the run's status
## names neither for an actor, and no word the frame drew speaks of either.
func _hud_shows_no_health_or_stamina(where: String) -> bool:
	var spoken: Array[String] = []
	for line: String in _drawn_lines():
		var lower: String = line.to_lower()
		if lower.contains("health") or lower.contains("stamina") or lower.contains("heart") or lower.begins_with("hp"):
			spoken.append(line)
	var named: Array[String] = []
	for who: String in ["chef", "appraiser"]:
		var actor: Dictionary = _status()[who]
		for field: String in ["health", "stamina"]:
			if actor.has(field):
				named.append(who + "." + field)
	return _check(not _drawn_lines().is_empty() and spoken.is_empty() and named.is_empty(), "%s shows no health or stamina: %s %s" % [where, spoken, named])

## Whether any line of the run's log says `words`.
func _events_mention(words: String) -> bool:
	for line: String in _events():
		if line.contains(words):
			return true
	return false

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

## Whether the transfer screen of the actor with the index is open.
func _screen_open(index: int) -> bool:
	var screens: Array = _status()["screens"]
	var open: bool = screens[index]["open"]
	return open

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
		var heading: int = signi(x - at_x)
		var other: String = "appraiser" if walker == "chef" else "chef"
		var ahead: bool = at_x != x and signi(_actor(walker, "sub_x") - _actor(other, "sub_x")) == heading
		if ahead and _lock_gap(walker) >= LOCK_WAIT:
			_release()
			await process_frame
			continue
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

## How far across, in sub-cells, `who` stands from its partner while both are
## in the city; 0 otherwise.
func _lock_gap(who: String) -> int:
	var other: String = "appraiser" if who == "chef" else "chef"
	if _where(who) != "city" or _where(other) != "city":
		return 0
	return absi(_actor(who, "sub_x") - _actor(other, "sub_x"))

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

## Chooses New game on the title screen: a saved day puts Continue first, so
## New game is then the second entry.
func _new_game() -> void:
	var lines: PackedStringArray = scene.get_meta("screen_text", PackedStringArray())
	await _tap(KEY_2 if lines.find("CONTINUE") >= 0 else KEY_1)

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
