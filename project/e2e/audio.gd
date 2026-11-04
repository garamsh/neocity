class_name AudioFeedbackCheck
extends SceneTree
## Plays with real key events and reads what the speakers were asked to play and
## which flourishes are on screen: the menus' cues, the title, workshop and city
## music and its tension, the cues of walking, jumping, landing, collecting,
## travelling, appraising, cooking, feeding, a new day, rising acid, a burn, a
## repair and a fall into the acid, and the pause screen's volumes, where an effects volume of 0 plays no effect
## and a music volume of 0 plays no music. Runs with no audio device.

const SEED: int = 11
## Two stocked sources whose finds make a dish, as `KeyCoverageCheck` walks
## them: the tomato machine and the shop shelf.
const FIRST_SOURCE: Vector2i = Vector2i(10, 9)
const SECOND_SOURCE: Array[Vector2i] = [Vector2i(3, 8), Vector2i(3, 6), Vector2i(5, 6)]
## The acid drip stands on the upper walkway at 1, 8, west of the kitchen
## ladder.
const DRIP: Array[Vector2i] = [Vector2i(1, 8), Vector2i(2, 8), Vector2i(1, 8)]
## Down the step and over the rubble to the tool bag and its wrench.
const TOOL_BAG_ROUTE: Array[Vector2i] = [Vector2i(13, 9)]
## Back over the rubble, up the step and the kitchen ladder to the foot of the
## roof ladder on the kitchen loft.
const ROOF_LADDER_ROUTE: Array[Vector2i] = [Vector2i(3, 8), Vector2i(3, 6), Vector2i(1, 6)]
const APPRAISE_ATTEMPTS: int = 12
## Frames of a collection's hold on the collecting actor, with a step to spare.
const COLLECT_FRAMES: int = 102
const COOK_ATTEMPTS: int = 6
const PLAYER_TWO: Dictionary[String, Key] = {
	"left": KEY_J, "right": KEY_L, "up": KEY_I, "down": KEY_K, "jump": KEY_SEMICOLON,
}
const EFFECTS_DEFAULT: float = 0.8
const MUSIC_DEFAULT: float = 0.6

var scene: MainScene
var held: Dictionary[Key, bool] = {}
var checks: int = 0

func _initialize() -> void:
	_run.call_deferred()

func _run() -> void:
	await _frames(2)
	scene = current_scene as MainScene
	if not _check(scene != null and scene.screen_name() == "main", "the game opens on the title screen"):
		return
	scene.pin_survival_seed(SEED)
	if not await _title_plays_music_and_menu_cues():
		return
	if not await _a_day_plays_its_cues():
		return
	if not await _effects_off_play_nothing():
		return
	if not await _music_off_plays_nothing():
		return
	if not await _acid_raises_the_tension_and_burns():
		return
	if not await _repair_and_fall():
		return
	print("Audio: ", checks, " live checks passed (menus, music, tension, walking, collecting, travel, workshop, feeding, new day, acid, burn, repair, fall, flourishes, effects off, music off).")
	quit(0)

func _title_plays_music_and_menu_cues() -> bool:
	await _frames(4)
	if not _check(_track() == "title" and _flag("music_playing"), "the title screen plays the title music"):
		return false
	if not _check(is_equal_approx(_level("effects"), EFFECTS_DEFAULT) and is_equal_approx(_level("music"), MUSIC_DEFAULT) and not _flag("effects_muted") and not _flag("music_muted"), "both buses start at their default volume, unmuted"):
		return false
	var started: int = _started()
	await _tap(KEY_S)
	if not _check(_started() == started + 1 and _last() == "menu_move", "a cursor step plays the move cue"):
		return false
	await _tap(KEY_W)
	await _tap(KEY_ENTER)
	if not _check(scene.screen_name() == "survival_setup" and _last() == "menu_select", "taking an entry plays the select cue"):
		return false
	await _tap(KEY_ESCAPE)
	return _check(scene.screen_name() == "main" and _last() == "menu_back", "going back plays the back cue")

## Two players, as `KeyCoverageCheck` plays them: player two gathers, both
## cook and feed at base.
func _a_day_plays_its_cues() -> bool:
	await _tap(KEY_1)
	await _tap(KEY_3)
	if not _check(_str("cinematic") != "" and _track() == "silence", "a cinematic stops the music"):
		return false
	await _tap(KEY_E)
	if not await _until("the run opens in the workshop with its music", func() -> bool: return _track() == "workshop" and _flag("music_playing"), 30):
		return false
	await _tap(KEY_U)
	if not await _until("player two sets out", func() -> bool: return _where("appraiser") == "city", 30):
		return false
	if not _check(_heard("travel") and _track() == "city" and _flag("drone_playing"), "setting out plays the travel cue and the city music over the drone"):
		return false
	var started: int = _started()
	if not await _walk_to(FIRST_SOURCE):
		return false
	if not _check(_heard("step") and _started() > started, "walking plays footsteps"):
		return false
	_key(KEY_SEMICOLON, true)
	var jumped: bool = await _until("player two jumps", func() -> bool: return _stance() == "airborne", 30)
	_key(KEY_SEMICOLON, false)
	if not jumped or not _check(_heard("jump"), "a jump plays the jump cue"):
		return false
	if not await _until("player two lands", func() -> bool: return _stance() == "standing", 60):
		return false
	if not _check(_heard("land"), "landing plays the landing cue"):
		return false
	if not await _walk_to(FIRST_SOURCE):
		return false
	await _tap(KEY_O)
	if not _check(_actor("ingredients") == 1 and _heard("collect") and _shown("pop"), "a collection plays its cue and pops on screen"):
		return false
	for waypoint: Vector2i in SECOND_SOURCE:
		if not await _walk_to(waypoint):
			return false
	await _tap(KEY_O)
	if not _check(_actor("ingredients") == 2, "a second collection fills the pack"):
		return false
	if not await _until("the second collection ends", func() -> bool: return _actor("collecting_ms") == 0, COLLECT_FRAMES):
		return false
	await _tap(KEY_U)
	if not await _until("player two returns", func() -> bool: return _str("phase") == "workshop", 60):
		return false
	if not _check(_heard("travel") and _track() == "workshop", "returning plays the travel cue and the workshop music"):
		return false
	var attempts: int = 0
	while _int("identified") < 2 and attempts < APPRAISE_ATTEMPTS:
		attempts += 1
		await _tap(KEY_O)
	if not _check(_int("identified") == 2 and _heard("identified"), "an appraisal plays its cue"):
		return false
	attempts = 0
	while _int("foods") == 0 and attempts < COOK_ATTEMPTS:
		attempts += 1
		await _tap(KEY_E)
	if not _check(_int("foods") == 1 and (_heard("cooked") or _heard("burned")) and _shown("steam"), "cooking plays its cue and steams"):
		return false
	await _tap(KEY_BRACKETLEFT)
	if not await _until("feeding starts day two", func() -> bool: return _int("day") == 2, 30):
		return false
	var reaction: String = _str("reaction")
	var pleased: bool = reaction == "satisfied"
	if not _check(_heard("satisfied" if pleased else "refused") and _shown("delight" if pleased else "disgust"), "feeding plays the monster's %s reaction" % reaction):
		return false
	return _check(_heard("new_day") and _shown("new_day"), "a new day plays its cue and fades")

## Effects at 0 play nothing while the screen still reacts; back at their level
## the same kind of event plays again.
func _effects_off_play_nothing() -> bool:
	await _tap(KEY_ESCAPE)
	if not _check(_str("interruption") == "paused", "Esc pauses the run"):
		return false
	for press: int in range(8):
		await _tap(KEY_A)
	if not _check(is_zero_approx(_level("effects")) and _flag("effects_muted") and not _flag("music_muted"), "A lowers the effects to 0 and mutes only their bus"):
		return false
	var started: int = _started()
	var recent: Array = _list("recent")
	await _tap(KEY_ESCAPE)
	if not await _until("the run continues", func() -> bool: return _str("interruption") == "", 30):
		return false
	var day: int = _int("day")
	await _tap(KEY_G)
	if not await _until("the day ends unfed", func() -> bool: return _int("day") == day + 1, 30):
		return false
	if not _check(_shown("new_day") and _int("day") == day + 1, "with the effects off the day still turns and fades"):
		return false
	if not _check(_started() == started and _list("recent") == recent, "with the effects off no effect plays"):
		return false
	await _tap(KEY_ESCAPE)
	for press: int in range(8):
		await _tap(KEY_D)
	if not _check(is_equal_approx(_level("effects"), EFFECTS_DEFAULT) and not _flag("effects_muted") and _last() == "menu_move" and _started() > started, "D raises the effects again and they play"):
		return false
	await _tap(KEY_ESCAPE)
	return await _until("the run continues", func() -> bool: return _str("interruption") == "", 30)

## Music at 0 plays none; one step up plays it again.
func _music_off_plays_nothing() -> bool:
	await _tap(KEY_ESCAPE)
	await _tap(KEY_S)
	for press: int in range(6):
		await _tap(KEY_A)
	if not _check(is_zero_approx(_level("music")) and _flag("music_muted") and _track() == "silence" and not _flag("music_playing") and not _flag("drone_playing"), "music at 0 stops the music and mutes its bus"):
		return false
	if not _check(is_equal_approx(_level("effects"), EFFECTS_DEFAULT), "S marked the music row, so the effects kept their level"):
		return false
	for press: int in range(6):
		await _tap(KEY_D)
	if not _check(is_equal_approx(_level("music"), MUSIC_DEFAULT) and _track() == "workshop" and _flag("music_playing"), "D brings the music back"):
		return false
	await _tap(KEY_ESCAPE)
	return await _until("the run continues", func() -> bool: return _str("interruption") == "", 30)

## The unfed day raised the acid: the city plays tenser, and the drip burns.
func _acid_raises_the_tension_and_burns() -> bool:
	if not _check(_heard("acid_rise") or _int("acid") > 0, "the acid has risen"):
		return false
	await _tap(KEY_U)
	if not await _until("player two sets out again", func() -> bool: return _where("appraiser") == "city" and _track() == "city", 30):
		return false
	var tension: float = _number("tension")
	var pitch: float = _number("pitch")
	var drone: float = _number("drone_db")
	if not _check(tension > 0.0 and pitch > 1.0 and drone > -80.0, "risen acid raises the music's pitch and the drone"):
		return false
	var health: int = _actor("health")
	for waypoint: Vector2i in DRIP:
		if not await _walk_to(waypoint):
			return false
		if _actor("health") < health:
			break
	return _check(_actor("health") < health and _heard("hurt") and _shown("hit"), "the drip burns with the hurt cue, a flash and a shake")

## The unfed day broke the roof ladder: player two fetches the wrench, repairs
## the ladder, and walks off the kitchen loft's west end into the acid.
func _repair_and_fall() -> bool:
	if not _check(_object("Roof Ladder") == "destroyed", "the unfed day broke the roof ladder"):
		return false
	for waypoint: Vector2i in TOOL_BAG_ROUTE:
		if not await _walk_to(waypoint):
			return false
	await _tap(KEY_O)
	if not _check(_actor("tools") == 1, "player two takes the wrench from the tool bag"):
		return false
	for waypoint: Vector2i in ROOF_LADDER_ROUTE:
		if not await _walk_to(waypoint):
			return false
	await _tap(KEY_O)
	if not _check(_object("Roof Ladder") == "intact" and _heard("repair") and _shown("spark"), "a repair plays its cue and sparks"):
		return false
	_key(PLAYER_TWO["left"], true)
	var off: bool = await _until("player two walks off the kitchen loft's west end", func() -> bool: return _stance() == "airborne", 120)
	_key(PLAYER_TWO["left"], false)
	if not off:
		return false
	if not await _until("player two falls into the acid", func() -> bool: return _where("appraiser") == "base", 120):
		return false
	return _check(_heard("fall") and _shown("splash"), "a fall into the acid plays its cue and splashes")

func _object(object: String) -> String:
	var objects: Dictionary = scene.survival_status()["objects"]
	var state: String = objects[object]
	return state

func _track() -> String:
	var value: String = scene.sound_status()["track"]
	return value

func _flag(field: String) -> bool:
	var value: bool = scene.sound_status()[field]
	return value

func _number(field: String) -> float:
	var value: float = scene.sound_status()[field]
	return value

func _list(field: String) -> Array:
	var value: Array = scene.sound_status()[field]
	return value

func _level(bus: String) -> float:
	var value: float = scene.sound_status()["%s_level" % bus]
	return value

func _started() -> int:
	var value: int = scene.sound_status()["started"]
	return value

func _last() -> String:
	var recent: Array = scene.sound_status()["recent"]
	if recent.is_empty():
		return ""
	var last: String = recent.back()
	return last

## Whether `cue` is among the last cues played.
func _heard(cue: String) -> bool:
	var recent: Array = scene.sound_status()["recent"]
	return recent.has(cue)

func _shown(flourish: String) -> bool:
	var shown: Array = scene.survival_status().get("flourishes", [])
	return shown.has(flourish)

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

func _actor(field: String) -> int:
	var actor: Dictionary = scene.survival_status()["appraiser"]
	var value: int = actor[field]
	return value

func _stance() -> String:
	var actor: Dictionary = scene.survival_status()["appraiser"]
	var stance: String = actor["stance"]
	return stance

## Walks player two sideways along its floor to column x, jumping whatever low
## block stops it, then climbs to row y.
func _walk_to(target: Vector2i) -> bool:
	var stalled: int = 0
	var last: int = _actor("sub_x")
	for frame: int in range(480):
		var at_x: int = _actor("x")
		var at_y: int = _actor("y")
		if at_x == target.x and at_y == target.y and _stance() != "airborne":
			_release()
			return true
		var sideways: bool = at_x != target.x
		_key(PLAYER_TWO["left"], sideways and target.x < at_x)
		_key(PLAYER_TWO["right"], sideways and target.x > at_x)
		_key(PLAYER_TWO["up"], not sideways and target.y < at_y)
		_key(PLAYER_TWO["down"], not sideways and target.y > at_y)
		var sub_x: int = _actor("sub_x")
		stalled = stalled + 1 if sideways and sub_x == last and _stance() == "standing" else 0
		last = sub_x
		_key(PLAYER_TWO["jump"], stalled > 12)
		await process_frame
	_release()
	return _check(false, "player two walks to %d, %d" % [target.x, target.y])

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
	push_error("Audio check failed: ", name, " ", scene.sound_status() if scene != null else {}, " ", scene.survival_status() if scene != null else {})
	quit(1)
	return false
