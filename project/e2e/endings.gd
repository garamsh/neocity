class_name SurvivalEndingsCheck
extends SurvivalEscapeCheck
## Keeps meals below satisfaction between occasional new dishes to survive
## thirty days through real inputs, then checks the vomited-out screen.

## The most frames a climb or a jump is given to land.
const AIRBORNE_FRAMES: int = 240

func _run() -> void:
	await _frames(2)
	scene = current_scene as MainScene
	if not _check(scene != null and scene.screen_name() == "main", "the title opens"):
		return
	scene.pin_survival_seed(SEED)
	await _start()
	await _tap(KEY_E)
	var previous_day: int = 0
	for action: int in range(30000):
		if _str("phase") == "ended" or _str("phase") == "lost":
			break
		if _int("day") != previous_day:
			previous_day = _int("day")
			print("Endings route: day ", previous_day, " acid ", _int("acid"), " satisfied ", _int("satisfied_days"))
		if _str("phase") == "workshop" and _str("next").begins_with("Cook "):
			await _choose_meal()
			if failed:
				return
		else:
			await _follow_next()
	if not _check(_str("phase") == "ended" and _str("ending") == "vomited_out" and _int("day") == 31, "the thirtieth completed day opens vomited out"):
		return
	await _frames(2)
	var shown: String = EndScreenCheck.problem(scene, "vomited_out", "Vomited out", "After 30 days, the monster spat")
	if not _check(shown.is_empty(), "the vomited-out screen is complete: " + shown):
		return
	var before: Dictionary = _status()
	await _tap(KEY_Q)
	await _tap(KEY_G)
	if not _check(_same_state(before), "vomited out blocks play"):
		return
	await _tap(KEY_E)
	if not _check(scene.screen_name() == "main", "the skip key leaves vomited out for the title"):
		return
	print("Endings: ", checks, " live checks passed.")
	quit(0)

## Prefers repeated meals on most days; one new satisfying meal every five
## days reduces acid without reaching seven satisfied days before day thirty.
func _choose_meal() -> void:
	var want_satisfied: bool = _int("satisfied_days") < mini(6, 1 + (_int("day") - 1) / 5)
	var first: String = _str("dish_offered")
	var chosen: String = ""
	var best: int = -1
	var mildest: String = ""
	var mildest_fine: int = 1 << 30
	for option: int in range(12):
		var forecast: String = _str("forecast")
		var fine: int = forecast.get_slice(", fine ", 1).get_slice(" ", 0).to_int()
		var name: String = _str("dish_offered")
		if name != "Pickle Surprise" and (fine >= 100) == want_satisfied and fine > best:
			chosen = name
			best = fine
		if name != "Pickle Surprise" and fine < mildest_fine:
			mildest = name
			mildest_fine = fine
		await _tap(KEY_S)
		if _str("dish_offered") == first:
			break
	if chosen.is_empty():
		# Every dish on offer would fall on the wrong side of satisfaction, so take the one that
		# comes nearest to the wanted side rather than whatever was offered first.
		chosen = first if mildest.is_empty() or want_satisfied else mildest
	for option: int in range(12):
		if _str("dish_offered") == chosen:
			await _tap(KEY_E)
			return
		await _tap(KEY_S)
	_check(false, "the selected meal remains cookable")

## NEXT names a climb or a jump as well as a walk, and says an open transfer screen is open instead
## of naming the search; the base check handles neither, so the chef stands still and the days run
## out. An open screen takes an item and closes. A climb holds its key for a step and a
## jump presses the jump key with the direction, each until the chef lands.
func _follow_next() -> void:
	var line: String = _str("next")
	var code: Key = ROUTE_KEYS.get(line.get_slice(" ", 1), KEY_NONE)
	if (line.begins_with("Climb ") or line.begins_with("Jump ")) and code != KEY_NONE:
		_key(code, true)
		if line.begins_with("Jump "):
			_key(KEY_SPACE, true)
			_key(KEY_SPACE, false)
		await _frames(step_frames)
		for frame: int in range(AIRBORNE_FRAMES):
			if _status()["chef"]["stance"] != "airborne":
				break
			await process_frame
		_key(code, false)
		return
	if line.begins_with("Inventory open"):
		await _tap(KEY_E)
		await _tap(KEY_R)
		return
	await super._follow_next()
