class_name SurvivalEscapeCheck
extends SceneTree
## Plays the survival game from the main menu to the ending with real key events,
## and drives the cinematic contract with no asset, a working scene, and broken ones.

## This seed misses on days 30 and 31 and hits on day 32.
const SEED: int = 7
const INTRO: String = "res://levels/monster_mouth_escape/cinematics/intro/intro.tscn"
const ENDING: String = "res://levels/monster_mouth_escape/cinematics/ending/ending.tscn"
const WORKING: String = "res://e2e/cinematic_fixture.tscn"
const UNSIGNED: String = "res://e2e/cinematic_unsigned_fixture.tscn"
const NODE3D: String = "res://e2e/cinematic_node3d_fixture.tscn"
const FINISHES_IN_ENTER_TREE: String = "res://e2e/cinematic_enter_tree_fixture.tscn"
const FINISHES_IN_READY: String = "res://e2e/cinematic_ready_fixture.tscn"
const CARD_FRAME_MARGIN: int = 12
const INTRO_CARD_FRAMES: int = 240
const ENDING_CARD_FRAMES: int = 360
const DAY_LIMIT: int = 120
const ROUTE_KEYS: Dictionary[String, Key] = {"left": KEY_A, "right": KEY_D, "up": KEY_W, "down": KEY_S}
const SLOW_FRAME_MS: int = 500
## The intro card's 4 seconds at SLOW_FRAME_MS, and 4 frames of margin.
const INTRO_CARD_SLOW_FRAMES: int = 12

var scene: MainScene
var held: Dictionary[Key, bool] = {}
var checks: int = 0
## The run's status as last read, and the frame it was read on: building it
## costs more than a frame, and nothing it reports changes before the next
## frame, which is also when the key events sent in this one arrive.
var status_read: Dictionary = {}
var status_frame: int = -1
var failed: bool = false
## Frames one survival step lasts: one on the 5 frames per second run.
var step_frames: int = 6
## Keeps each taken-over resource alive; the cache forgets a path when it is freed.
var stand_in: Resource
var frame_seconds: float = 0.0
## The last roll count read while a run was open: a closed run reports none.
var rolls_seen: int = 0

func _initialize() -> void:
	_run.call_deferred()

func _process(delta: float) -> bool:
	frame_seconds = delta
	return false

func _run() -> void:
	await _frames(2)
	scene = current_scene as MainScene
	if not _check(scene != null and scene.screen_name() == "main", "the game opens on the main menu"):
		return
	scene.pin_survival_seed(SEED)
	if OS.get_cmdline_user_args().has("real-clock"):
		await _real_frames()
		if not failed:
			print("Escape real frames: ", checks, " live checks passed.")
			quit(0)
		return
	if frame_seconds > 0.1:
		step_frames = 1
		await _slow_frames()
		if not failed:
			print("Escape slow frames: ", checks, " live checks passed.")
			quit(0)
		return
	for part: Callable in [_fallback_intro, _play_to_the_ending, _fallback_ending, _scene_intro, _skipped_scene, _broken_scenes, _menu_leaves_a_cinematic, _scene_ending, _instant_intro.bind(FINISHES_IN_ENTER_TREE), _instant_ending.bind(FINISHES_IN_ENTER_TREE), _instant_intro.bind(FINISHES_IN_READY), _instant_ending.bind(FINISHES_IN_READY)]:
		await part.call()
		if failed:
			return
	print("Escape: ", checks, " live checks passed (fallback, input block, days, escape, ending, scene, skip, broken assets, menu, instant finish).")
	quit(0)

## No intro asset exists, so a real run opens on the card, blocked, until it times out.
func _fallback_intro() -> void:
	if not _check(not ResourceLoader.exists(INTRO) and not ResourceLoader.exists(ENDING), "no cinematic asset exists at the contract paths"):
		return
	await _start()
	if not _check(_str("cinematic") == "intro" and _str("cinematic_source") == "fallback", "a missing intro takes the fallback card"):
		return
	await _tap(KEY_Q)
	await _tap(KEY_G)
	if not _check(_where("chef") == "base" and _int("day") == 1, "the card blocks setting out and ending the day"):
		return
	if not await _until("the card times out", func() -> bool: return _str("cinematic") == "", INTRO_CARD_FRAMES + CARD_FRAME_MARGIN):
		return
	await _tap(KEY_Q)
	if not await _until("the game takes input after the card", func() -> bool: return _where("chef") == "city", 30):
		return
	await _tap(KEY_Q)
	await _until("the party is back at base", func() -> bool: return _str("phase") == "workshop", 60)

func _play_to_the_ending() -> void:
	while _int("day") < 30 and _str("phase") != "lost":
		await _follow_next()
	if not _check(_str("phase") == "workshop" or _str("phase") == "exploration", "following NEXT reaches day thirty with the acid below full"):
		return
	if not _check(_int("escape_rolls") == 0, "no escape roll before day thirty finishes"):
		return
	if not _check(not _events_say("stayed shut"), "no miss is reported before the first check"):
		return
	while _int("day") == 30:
		await _follow_next()
	if not _check(_int("escape_rolls") == 1, "finishing day thirty rolls once"):
		return
	if not _check(_events_say("Day 30: the way out stayed shut."), "the missed check is reported"):
		return
	await _follow_to_the_end()
	if not _check(_str("phase") == "escaped", "a later day escapes"):
		return
	_check(_int("day") > 31 and _int("escape_rolls") == _int("day") - 30, "each finished day from thirty rolled once, with a miss before the hit")

func _fallback_ending() -> void:
	if not _check(_str("cinematic") == "ending" and _str("cinematic_source") == "fallback", "a missing ending takes the fallback card"):
		return
	if not _check(_str("dish_offered") == "", "an escaped party is offered no dish"):
		return
	var before: Dictionary = _status()
	await _tap(KEY_Q)
	await _tap(KEY_G)
	await _tap(KEY_F)
	await _tap(KEY_R)
	await _frames(30)
	if not _check(_same_state(before), "no key changes an escaped run"):
		return
	if not await _until("the ending card times out", func() -> bool: return scene.screen_name() == "main", ENDING_CARD_FRAMES + CARD_FRAME_MARGIN):
		return
	_check(_status().is_empty(), "the ending closes the run")

func _scene_intro() -> void:
	_stand_in(load(WORKING), INTRO)
	await _start()
	if not _check(_str("cinematic") == "intro" and _str("cinematic_source") == "scene" and _mounted(), "an intro asset plays as a scene"):
		return
	await _tap(KEY_Q)
	await _tap(KEY_G)
	if not _check(_where("chef") == "base" and _int("day") == 1, "a playing scene blocks the game's keys"):
		return
	if not await _until("the scene reports its end", func() -> bool: return _str("cinematic") == "", 120):
		return
	if not _check(not _mounted(), "the host closes a finished scene"):
		return
	await _tap(KEY_Q)
	await _until("the game takes input after the scene", func() -> bool: return _where("chef") == "city", 30)

func _skipped_scene() -> void:
	await _leave()
	await _start()
	if not _check(_mounted(), "a new run plays the scene again"):
		return
	await _tap(KEY_E)
	_check(_str("cinematic") == "" and not _mounted() and _int("day") == 1, "E skips the scene and returns to the game")

## A resource that is no scene and a scene without the signal both fall back.
func _broken_scenes() -> void:
	await _leave()
	_stand_in(Resource.new(), INTRO)
	await _start()
	if not _check(_str("cinematic_source") == "fallback" and not _mounted(), "a load that yields no scene takes the fallback"):
		return
	await _leave()
	_stand_in(load(UNSIGNED), INTRO)
	await _start()
	if not _check(_str("cinematic_source") == "fallback" and not _mounted(), "a scene without the finished signal takes the fallback"):
		return
	await _leave()
	_stand_in(load(NODE3D), INTRO)
	await _start()
	if not _check(_str("cinematic_source") == "fallback" and not _mounted(), "a root that is no canvas item takes the fallback"):
		return
	await _tap(KEY_E)
	_check(_str("cinematic") == "", "E skips the fallback card")

func _menu_leaves_a_cinematic() -> void:
	await _leave()
	_stand_in(load(WORKING), INTRO)
	await _start()
	await _tap(KEY_M)
	_check(scene.screen_name() == "main" and _status().is_empty() and not _mounted(), "M leaves a playing scene and ends the run")

## The working scene's end returns from the ending to the main menu.
func _scene_ending() -> void:
	stand_in = null
	var ending: Resource = load(WORKING)
	stand_in = ending
	ending.take_over_path(ENDING)
	await _start()
	await _tap(KEY_E)
	await _follow_to_the_end()
	if not _check(_str("cinematic") == "ending" and _str("cinematic_source") == "scene", "an ending asset plays as a scene"):
		return
	if not await _until("the ending scene reports its end", func() -> bool: return scene.screen_name() == "main", 120):
		return
	_check(not _mounted() and _status().is_empty(), "the ending scene returns to the main menu")

## A scene reporting its end while the host adds it closes. A crash here ends
## the process, which `make test` reads as a failure.
func _instant_intro(fixture: String) -> void:
	_stand_in(load(fixture), INTRO)
	await _start()
	if not await _until("an instant intro closes unskipped", func() -> bool: return _str("cinematic") == "", 30):
		return
	if not _check(not _mounted() and scene.screen_name() == "survival" and _int("day") == 1, "an instant intro hands over to day one"):
		return
	await _tap(KEY_Q)
	if not await _until("the game takes input after an instant intro", func() -> bool: return _where("chef") == "city", 30):
		return
	await _leave()

func _instant_ending(fixture: String) -> void:
	_stand_in(load(fixture), ENDING)
	await _start()
	await _tap(KEY_E)
	rolls_seen = 0
	await _follow_to_the_end()
	_check(rolls_seen > 0 and scene.screen_name() == "main" and _status().is_empty() and not _mounted(), "an instant ending closes the run and returns to the main menu")

## Run with --fixed-fps 5: a frame spans two steps, so the step after the hit
## would run in the frame that escaped.
func _slow_frames() -> void:
	await _start()
	await _tap(KEY_E)
	if not _check(_str("cinematic") == "", "E skips the card on slow frames"):
		return
	await _follow_to_the_end()
	if not _check(_str("phase") == "escaped" and _str("cinematic") == "ending", "a slow frame opens the ending on the hit"):
		return
	_check(_int("day") == 33 and _int("escape_rolls") == 3, "a slow frame takes the same rolls to the same day")

## Run with no --fixed-fps, which bypasses the engine's cap on `delta`: the card
## stands its 4 seconds of wall clock on frames longer than that cap.
func _real_frames() -> void:
	await _start()
	if not _check(_str("cinematic") == "intro", "a run opens on the intro card on real slow frames"):
		return
	var started: int = Time.get_ticks_msec()
	for frame: int in range(INTRO_CARD_SLOW_FRAMES):
		OS.delay_msec(SLOW_FRAME_MS)
		await process_frame
		if _str("cinematic") == "":
			break
	var stood_ms: int = Time.get_ticks_msec() - started
	if not _check(_str("cinematic") == "", "the card closes after its seconds on real slow frames, open after %dms" % stood_ms):
		return
	_check(stood_ms >= 3500, "the card stands its seconds on real slow frames, closed after %dms" % stood_ms)

## Serves `resource` from `path` without writing to the art team's folder.
func _stand_in(resource: Resource, path: String) -> void:
	stand_in = resource
	resource.take_over_path(path)

func _start() -> void:
	await _tap(KEY_1)
	await _tap(KEY_1)

## M asks first mid-run and Enter accepts; M leaves a cinematic at once. Enter on
## the title screen takes its marked entry, so it is pressed only to answer the
## question, and the title must show before the next menu key.
func _leave() -> void:
	var status: Dictionary = _status()
	if status.has("cinematic"):
		await _tap(KEY_M)
	elif not status.is_empty():
		await _tap(KEY_M)
		await _tap(KEY_ENTER)
	await _until("the title screen shows after leaving the run", func() -> bool: return scene.screen_name() == "main" and _status().is_empty(), 30)

## Plays on what NEXT says until the run ends: only following it holds the acid.
func _follow_to_the_end() -> void:
	while (_str("phase") == "workshop" or _str("phase") == "exploration") and _int("day") < DAY_LIMIT:
		rolls_seen = _int("escape_rolls")
		await _follow_next()

## Whether NEXT names what the interact key does: collect, repair, appraise
## or cook, as the controls page lists for it.
func _names_interact(line: String) -> bool:
	for verb: String in ["Collect from ", "Repair ", "Appraise ", "Cook "]:
		if line.begins_with(verb):
			return true
	return false

## Presses the key for the one action NEXT names, as the controls page lists
## it, or lets the day run while it names none.
func _follow_next() -> void:
	var line: String = _str("next")
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
	elif line.begins_with("Walk "):
		await _walk_a_cell(line)
	else:
		await _frames(step_frames)

## Walks one cell the way NEXT names: a press and release in one frame walks one
## step at any frame length.
func _walk_a_cell(line: String) -> void:
	var code: Key = ROUTE_KEYS.get(line.get_slice(" ", 1), KEY_NONE)
	if code != KEY_NONE:
		_key(code, true)
		_key(code, false)
	await _frames(step_frames)

func _mounted() -> bool:
	return scene.get_node_or_null("Cinematic") != null

## Compares the run's state, leaving out the event log's order of arrival.
func _same_state(before: Dictionary) -> bool:
	var now: Dictionary = _status()
	for field: String in ["day", "phase", "acid", "remaining_ms", "elapsed_ms", "rolls", "escape_rolls", "foods", "chef", "appraiser", "stock", "objects"]:
		if now[field] != before[field]:
			return false
	return true

func _events_say(words: String) -> bool:
	var events: Array = _status()["events"]
	for event: String in events:
		if event.contains(words):
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

func _check(condition: bool, name: String) -> bool:
	if condition:
		checks += 1
		return true
	push_error("Escape check failed: ", name, " ", scene.survival_status() if scene != null else {})
	failed = true
	quit(1)
	return false
