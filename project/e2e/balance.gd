class_name SurvivalBalanceCheck
extends SceneTree
## Plays the survival game from the main menu on three fixed seeds with real key events, pressing only
## the key the NEXT line names, and reads the ending each run reaches. The seeds are the ones whose runs
## reach the monster satisfied, the run vomited out and the acid filled, so the three endings stay reachable.

## The seed of each run, the phase and ending it reaches, the day the run is on at its end and the satisfied
## days it counted, as `docs/architecture/survival.md` §Ending values records. An acid failure has no ending.
const RUNS: Array[Dictionary] = [
	{"seed": 1, "phase": "ended", "ending": "monster_satisfied", "day": 10, "satisfied_days": 7},
	{"seed": 26, "phase": "ended", "ending": "vomited_out", "day": 31, "satisfied_days": 6},
	{"seed": 81, "phase": "lost", "ending": "none", "day": 31, "satisfied_days": 5},
]
const DAY_LIMIT: int = 40
const ROUTE_KEYS: Dictionary[String, Key] = {"left": KEY_A, "right": KEY_D, "up": KEY_W, "down": KEY_S}

var scene: MainScene
var held: Dictionary[Key, bool] = {}
var reached: PackedStringArray = []

func _initialize() -> void:
	_run.call_deferred()

func _run() -> void:
	for _frame: int in range(2):
		await process_frame
	scene = current_scene as MainScene
	for run: Dictionary in RUNS:
		if not await _play(run):
			return
	print("Balance: finished: pass; %s." % ", ".join(reached))
	quit(0)

## Plays one seed to its end and reads where it ended against `run`.
func _play(run: Dictionary) -> bool:
	var seed_value: int = run["seed"]
	scene.pin_survival_seed(seed_value)
	await _tap(KEY_1)
	await _tap(KEY_1)
	if _str("cinematic") != "":
		await _tap(KEY_E)
	while (_str("phase") == "workshop" or _str("phase") == "exploration") and _int("day") < DAY_LIMIT:
		await _follow_next()
	var state: Dictionary = scene.survival_status()
	var seen: Dictionary = {"seed": seed_value, "phase": state["phase"], "ending": state.get("ending", "none"), "day": state["day"], "satisfied_days": state["satisfied_days"]}
	if seen != run:
		push_error("seed %d ended differently than recorded" % seed_value)
		print("Balance: finished: fail; seed %d ended as %s, recorded as %s." % [seed_value, seen, run])
		quit(1)
		return false
	reached.append("seed %d %s on day %d" % [seed_value, seen["ending"] if seen["ending"] != "none" else "acid loss", seen["day"]])
	await _tap(KEY_M)
	if scene.screen_name() != "main":
		await _tap(KEY_ENTER)
	for _frame: int in range(30):
		await process_frame
	return true

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
	elif line.begins_with("Search "):
		await _tap(KEY_R)
	elif line.begins_with("Searching "):
		await _tap(KEY_E)
		await _tap(KEY_R)
	elif line.begins_with("Appraise ") or line.begins_with("Cook "):
		await _tap(KEY_E)
	elif line.begins_with("Walk "):
		var code: Key = ROUTE_KEYS.get(line.get_slice(" ", 1), KEY_NONE)
		if code != KEY_NONE:
			_key(code, true)
			_key(code, false)
		await _frames(6)
	else:
		await _frames(6)

func _str(field: String) -> String:
	var value: String = scene.survival_status().get(field, "")
	return value

func _int(field: String) -> int:
	var value: int = scene.survival_status().get(field, 0)
	return value

func _tap(code: Key) -> void:
	_key(code, true)
	await _frames(2)
	_key(code, false)
	await _frames(8)

func _frames(count: int) -> void:
	for _frame: int in range(count):
		await process_frame

func _key(code: Key, pressed: bool) -> void:
	if held.get(code, false) == pressed:
		return
	held[code] = pressed
	var event: InputEventKey = InputEventKey.new()
	event.physical_keycode = code
	event.pressed = pressed
	Input.parse_input_event(event)
