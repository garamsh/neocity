class_name SurvivalCapture
extends SceneTree
## Captures real rendered frames of a played survival run into user://survival/,
## and fails when a city frame's ground does not read over the scenery behind it,
## its walkways, backdrop and acid band do not read dark to bright, the acid
## band behind the map does not show green at no acid, the stairs' walked-on part does not
## lead the part under it or reads darker than the deck's top edge, the fallen slab's
## lit top edge does not run along its tilt, the acid's light does not rise
## above its surface, the acid stands still between two
## frames a moment apart on the scene's clock, the finishing pass
## breaks the scene's pixels below the sea's surface into smaller ones, the
## city draws fewer than 60 frames a second,
## one of its word plates covers another, a source other than the source's
## own count, or lies under a HUD panel or note, its words name a source's kind or a place other than the goals
## NEXT names, or leave out the goal NEXT names to walk to, the monster's teeth do not show along the top of the play area,
## a support reaches down from a walkway too high for it, the shops, stall
## and vending machines do not read warm and lit, or a window of another size,
## or full screen, does not show the 1280x720 screen at the largest whole
## multiple of it that fits. The camera shows a part of
## the city, so each look is measured on every city frame that shows what it
## looks at, and fails when none does.
## The scene's clock, which runs its ambient motion, its finishing pass and
## every eased value of the screen, is pinned and moves only when the capture
## moves it: by each frame's delta, and past the longest eased motion before
## each saved frame, so every frame shows the same moment on every run.
## Run it where the screen never locks, such as kwin_wayland --virtual
## --no-lockscreen: a locked screen stops the window drawing, and the capture
## then waits on a frame that never comes.
## On Wayland, Godot 4.7.2 can hang after quit() (godotengine/godot#123059,
## docs/architecture/godot-runtime.md), so run it under a watchdog that ends
## Godot 30 seconds after it prints "Capture: finished: pass" or
## "Capture: finished: fail: <reason>", and read the result from that line.

## One unfed day short of the acid that ends the run.
const HIGH_ACID: int = 84
## Half the acid that ends the run, to the nearest unfed day's rise.
const MIDDLE_ACID: int = 48
## Following NEXT on this seed escapes; `G` alone only ever loses.
const ESCAPE_SEED: int = 7
const DAY_LIMIT: int = 120
const STEP_FRAMES: int = 6
const ROUTE_KEYS: Dictionary[String, Key] = {"left": KEY_A, "right": KEY_D, "up": KEY_W, "down": KEY_S}
## The map's top-left corner in the scene and its cell size, as the city draws
## it; the canvas's map_view meta takes the scene to the screen.
const MAP_ORIGIN: Vector2i = Vector2i(64, 70)
## The play area between the top bar and the cards, on screen.
const PLAY: Rect2 = Rect2(0, 62, 1280, 478)
const CELL: int = 32
## Ground cells of the first city frame with nothing but scenery in them: the
## arcade, the rooftop and the market. Each walkway's deck runs under its
## cell's floor line, where an actor's feet are.
const LEAD_CELLS: Array[Vector2i] = [Vector2i(4, 2), Vector2i(6, 2), Vector2i(7, 2), Vector2i(8, 2), Vector2i(19, 2), Vector2i(20, 2), Vector2i(21, 2), Vector2i(22, 2), Vector2i(8, 11), Vector2i(10, 11)]
## How much brighter, in mean luminance from 0 to 1, the walkway's lit top
## edge under those cells reads than the scenery in the cells, above the deck.
const GROUND_LEAD: float = 0.12
## The rows of a walkway's lit top edge, as the city draws it: the ground a
## player walks on, read over the scenery above it.
const EDGE_ROWS: int = 4
## The rows of a walkway's face, under its lit top edge, that the value layers
## read as the foreground, clear of the edge and the deck's bottom.
const FACE_ROWS: Vector2i = Vector2i(8, 24)
## The screen row of the acid band's surface at no acid and at the acid that
## ends the run, as the city draws it; it rises evenly between them.
const BAND_LOW: int = 470
const BAND_HIGH: int = 240
## The screen rows under the band's surface that the value layers read as the
## acid.
const BAND_ROWS: Vector2i = Vector2i(6, 40)
## How much darker, in mean luminance from 0 to 1, the walkways' faces read
## than the backdrop above them, and how much brighter the acid band's glowing
## pixels read than that backdrop.
const FACE_GAP: float = 0.1
const ACID_GAP: float = 0.05
## How much a pixel's green channel leads its red and blue, from 0 to 1, for
## it to read as glowing acid, and the least share of the band's pixels that
## read so, with its sunken trains, the pillars and the walkways in front.
const ACID_GREEN: float = 0.1
const ACID_SHARE: float = 0.13
## The south stairs, each rising to the left across its cell from its floor
## line down to the row below, so its walked-on part lies above that diagonal.
const STAIR_CELLS: Array[Vector2i] = [Vector2i(13, 12), Vector2i(14, 13)]
## The scene row of the acid's surface at no acid, under the map's lowest
## ground row; it rises evenly to the map's top at the acid that ends the run,
## as the city draws it.
const ACID_LOW: int = 518
## How far above that surface the sea in the scene, with the light over it,
## covers the acid band behind the map.
const SEA_REACH: int = 32
## How much brighter, in mean luminance from 0 to 1, the EDGE_ROWS just above
## a stair's diagonal read than the EDGE_ROWS just under it.
const STEP_LEAD: float = 0.15
## Plain market deck beside the south stairs, whose lit top edge the stairs'
## walked-on part is held to; the camera that shows the stairs shows it too.
const STAIR_DECK_CELLS: Array[Vector2i] = [Vector2i(11, 11)]
## The least share of that deck's top edge, in mean luminance, that the
## stairs' walked-on part reads at, so stairs read as walkway like the deck.
const STAIR_PARITY: float = 0.9
## From the entrance down the market ladder into the market.
const MARKET_ROUTE: Array[Vector2i] = [Vector2i(7, 8), Vector2i(7, 11), Vector2i(11, 11)]
## The fallen slab's first cell and its width in cells; it falls a row from
## the floor line of its first cell to that of the row below at its right end.
const SLAB_CELL: Vector2i = Vector2i(20, 6)
const SLAB_CELLS: int = 2
## Where the scene's clock is pinned, in seconds; how far apart on
## that clock the two frames of the acid's motion are taken; how much a pixel's
## colour must change in a channel, from 0 to 1, to count as moved; and the
## least share of the acid's pixels that move.
const SCENE_CLOCK: float = 0.0
const MOTION_SECONDS: float = 0.25
const MOTION_STEP: float = 0.04
const MOTION_SHARE: float = 0.05
## The screen rows just above the acid's surface that its light falls on, and
## the rows of backdrop higher up that they are held against; how much more,
## in mean green lead from 0 to 1, the lit rows read.
const LIT_ROWS: Vector2i = Vector2i(-16, 0)
const UNLIT_ROWS: Vector2i = Vector2i(-96, -64)
const LIGHT_GAP: float = 0.03
## The screen rows under the acid's surface where a column shows the acid, and
## the least share of the columns that must show it for the light to be read.
const ACID_UNDER: Array[int] = [6, 12, 24]
const LIGHT_COLUMNS: float = 0.1
## The screen rows under the sea's surface that the motion is read over.
const SEA_ROWS: Vector2i = Vector2i(2, 40)
## The least share of the scene's pixels below the sea's surface that the
## finished frame draws as one colour, and how far apart, from 0 to 1 in a
## channel, two screen pixels of one may be. A scene pixel is a square of
## screen pixels as wide as the camera's zoom.
const GRID_SHARE: float = 0.97
const GRID_TOLERANCE: float = 0.01
## The least share of those scene pixels whose green leads by ACID_GREEN, so
## the pixels read hold the acid.
const GRID_GREEN: float = 0.05
## The least frames a second the city draws, and over how many frames.
const FRAME_RATE: float = 60.0
const RATE_FRAMES: int = 600
## How far one 100ms movement unit carries a running actor, in sub-cells.
const RUN_UNIT: int = 50
## Longer than the screen's longest eased motion, in seconds of the scene's
## clock.
const SETTLE_SECONDS: float = 0.4
## The rows at the top of the play area the teeth are read over; the least
## share of the pixels their picture covers there that must show it, within
## TEETH_TOLERANCE in each channel, low because a high walkway hides them
## where it crosses the top of the screen; and the least share of the pixels
## read the picture must cover.
const TEETH_ROWS: int = 48
const TEETH_SHARE: float = 0.04
const TEETH_TOLERANCE: float = 0.06
const TEETH_COVER: float = 0.3
## The teeth picture's gum, as the World layer draws it unbroken along the top
## over the fangs: its left edge in the picture, the width of one run of the
## fangs it repeats at, and its rows.
const GUM_LEFT: int = 28
const JAW_RUN: int = 392
const GUM_ROWS: int = 24
## How many fangs one run of them holds: no run of that many on the screen may
## recur elsewhere on it in the same order of pictures, whatever their drops.
const RUN_FANGS: int = 8
## The most of the throat's pixels, drawn behind the teeth from under their
## gum down to their lowest tip, that may be in the colours the teeth picture
## draws its fangs in and not its gum, so no second row of fangs hangs there;
## and the least of the teeth's own pixels over the same rows that must be, so
## those colours are the fangs'.
const SECOND_ROW: float = 0.05
const TEETH_FANGED: float = 0.5
## How dark, in luminance, a pixel of a support reads; how many scene pixels
## tall a dark run must be to read as a long support, longer than a brace;
## and the most of the play area's columns such runs may cross.
const SUPPORT_DARK: float = 0.15
const SUPPORT_RUN: int = 120
const SUPPORT_SHARE: float = 0.07
## The scene rectangles of the lit structures: the corner shops, the vending
## machine and the stall; how bright a pixel's red must be and how far it must
## lead blue to read warm and lit; and the least share of their pixels that do.
const LIT_AREAS: Array[Rect2] = [Rect2(468, 290, 120, 114), Rect2(52, 52, 120, 114), Rect2(308, 392, 56, 62), Rect2(305, 274, 62, 84)]
const LIT_RED: float = 0.6
const LIT_WARMTH: float = 0.2
const LIT_SHARE: float = 0.15

## The size of the screen every check above measures, and the window sizes it
## is shown at afterwards: twice and three times it, and a 1280x720 window at
## a display scale of 1.7, which holds it once with a border.
const SCREEN: Vector2i = Vector2i(1280, 720)
const WINDOW_SIZES: Array[Vector2i] = [Vector2i(2560, 1440), Vector2i(3840, 2160), Vector2i(2176, 1224)]
## How many frames a window may take to reach the size or mode asked of it.
const RESIZE_FRAMES: int = 120

## The words of a source's kind the map once wrote beside its supply.
const KIND_WORDS: Array[String] = ["BAG", "VEND", "CART", "SHOP"]
## The map's words that are neither a place's name nor a number, such as a
## count or the health an acid drip takes: a broken passage's mark and the
## actors' tags.
const MAP_MARKS: Array[String] = ["BROKEN", "P1", "P2", "AI"]

var scene: MainScene
var held: Dictionary[Key, bool] = {}
var city_frames: int = 0
var ground_frames: int = 0
var stairs_frames: int = 0
var parity_frames: int = 0
var slab_frames: int = 0
var layer_frames: int = 0
var acid_frames: int = 0
var band_frames: int = 0
var grid_frames: int = 0
var light_frames: int = 0
var teeth_frames: int = 0
var support_frames: int = 0
var lit_frames: int = 0
## The teeth picture, whose gum and fangs the World layer draws where its
## `jaw_gum` and `jaw_fangs` metas say.
var teeth: Image = Image.load_from_file(ProjectSettings.globalize_path("res://levels/monster_mouth_escape/art/backgrounds/jaw.png"))
## The lowest row the teeth picture covers in each of its columns.
var teeth_bottom: Array[int] = _bottoms(teeth)
## The colours the teeth picture draws its fangs in and not its gum.
var fang_colours: Dictionary = _fang_colours(teeth)
## The throat picture, which the World layer draws where its `throat_at` meta
## says, beside mirrored copies.
var throat: Image = Image.load_from_file(ProjectSettings.globalize_path("res://levels/monster_mouth_escape/art/backgrounds/throat.png"))
## Where the scene's clock stands.
var clock: float = SCENE_CLOCK
## Set once the run has printed its result line.
var finished: bool = false

func _initialize() -> void:
	_run.call_deferred()

func _run() -> void:
	await _frames(2)
	scene = current_scene as MainScene
	scene.pin_survival_seed(11)
	scene.pin_scene_clock(clock)
	process_frame.connect(_tick)
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("user://survival"))
	await _save("1-main-menu")
	await _tap(KEY_2)
	await _save("25-controls")
	await _tap(KEY_ESCAPE)
	await _tap(KEY_1)
	await _save("2-choose-players")
	await _tap(KEY_1)
	await _save("12-intro-card")
	await _tap(KEY_E)
	await _save("3-base-day-1")
	await _tap(KEY_Q)
	await _walk(KEY_D, 7)
	await _save_city("4-city-collect-prompt")
	if not await _acid_moves("4-city-collect-prompt") or not await _draws_fast("4-city-collect-prompt"):
		return
	# A jump in place on the flat street comes down where it rose.
	_key(KEY_SPACE, true)
	await _until_stance("airborne")
	_key(KEY_SPACE, false)
	await _until_falling()
	await _save_city("28-city-falling")
	await _until_stance("standing")
	await _save_city("29-city-landing")
	_key(KEY_SHIFT, true)
	_key(KEY_A, true)
	# The first unit after standing still reads as a walk; the second as a run.
	await _until_moved(-2 * RUN_UNIT)
	await _save_city("30-city-running")
	_key(KEY_A, false)
	_key(KEY_SHIFT, false)
	await _frames(3)
	await _walk(KEY_D, 8)
	_key(KEY_D, true)
	_key(KEY_SPACE, true)
	await _until_stance("airborne")
	_key(KEY_SPACE, false)
	await _frames(4)
	await _save_city("27-city-jumping-the-rubble")
	await _walk(KEY_D, 12)
	await _frames(30)
	await _save_city("5-city-after-drip")
	await _until_phase("workshop")
	await _save("6-workshop-after-return")
	await _until_next("Cook", 300)
	await _save("7-workshop-cook-offered")
	await _tap(KEY_S)
	await _save("14-workshop-dish-chosen")
	for attempt: int in range(4):
		await _tap(KEY_E)
		if _int("foods") > 0:
			break
	await _save("8-workshop-dish-ready")
	await _tap(KEY_F)
	await _frames(20)
	await _save("9-next-day")
	await _tap(KEY_Q)
	# Feeding started day two, so the ladder frames are day two's.
	await _walk(KEY_D, 16)
	await _walk_left(15)
	await _save_city("23-city-facing-left")
	await _walk(KEY_D, 18)
	await _climb(4)
	await _save_city("24-city-on-ladder")
	await _until_phase("workshop")
	await _frames(60)
	await _tap(KEY_E)
	await _tap(KEY_E)
	await _tap(KEY_F)
	await _frames(12)
	# Before the acid rises: down the market ladder, along the market and down
	# the south stairs, so the camera, low on the map, shows the stairs behind
	# the chef.
	await _tap(KEY_Q)
	await _frames(40)
	for waypoint: Vector2i in MARKET_ROUTE:
		await _walk_to(waypoint.x, waypoint.y)
	await _walk(KEY_D, 17)
	await _frames(30)
	await _save_city("31-city-market")
	await _tap(KEY_Q)
	await _until_phase("workshop")
	await _frames(12)
	await _tap(KEY_G)
	await _frames(12)
	await _tap(KEY_G)
	await _frames(12)
	await _tap(KEY_G)
	await _frames(12)
	await _save("10-workshop-acid-rose")
	await _tap(KEY_Q)
	await _frames(40)
	await _walk(KEY_D, 8)
	await _frames(60)
	await _save_city("11-city-damaged")
	await _tap(KEY_Q)
	await _until_phase("workshop")
	while _int("acid") < MIDDLE_ACID:
		await _tap(KEY_G)
		await _frames(12)
	await _tap(KEY_Q)
	await _frames(40)
	await _walk(KEY_D, 6)
	await _save_city("26-city-middle-acid")
	if not await _acid_moves("26-city-middle-acid"):
		return
	await _tap(KEY_Q)
	await _until_phase("workshop")
	while _int("acid") < HIGH_ACID:
		await _tap(KEY_G)
		await _frames(12)
	await _tap(KEY_Q)
	await _frames(40)
	await _walk(KEY_D, 6)
	await _save_city("17-city-high-acid")
	await _tap(KEY_ESCAPE)
	await _save("18-pause-one-player")
	await _tap(KEY_M)
	await _save("19-confirm-leave")
	await _tap(KEY_ESCAPE)
	await _tap(KEY_ESCAPE)
	_key(KEY_D, true)
	await _tap(KEY_ESCAPE)
	await _tap(KEY_ESCAPE)
	await _save("20-resuming")
	_key(KEY_D, false)
	await _frames(4)
	await _tap(KEY_M)
	await _tap(KEY_ENTER)
	await _tap(KEY_1)
	await _tap(KEY_3)
	await _tap(KEY_E)
	await _save("15-two-players-workshop")
	await _tap(KEY_Q)
	await _tap(KEY_U)
	await _frames(12)
	await _save_city("16-two-players-city")
	await _tap(KEY_ESCAPE)
	await _save("21-pause-two-players")
	await _tap(KEY_ESCAPE)
	await _tap(KEY_X)
	await _frames(12)
	await _save("22-hand-off-notice")
	# A human appraiser leaves the AI chef's finds unidentified, so the second
	# day's base shows the day's result beside several of them.
	await _tap(KEY_M)
	await _tap(KEY_ENTER)
	await _tap(KEY_1)
	await _tap(KEY_2)
	await _tap(KEY_E)
	await _tap(KEY_Q)
	await _frames(12)
	await _until_phase("workshop")
	await _tap(KEY_G)
	# Past the new day's flourish, which darkens the kitchen as it opens.
	await _frames(120)
	var bag: Array = scene.get_meta("bag", [])
	var unknown: int = bag.filter(func(item: Dictionary) -> bool: return not item["known"]).size()
	if _int("day") != 2 or unknown < 2:
		_fail("the second day's base holds %d unidentified kinds on day %d" % [unknown, _int("day")])
		return
	await _save("32-workshop-appraisal-and-result")
	await _tap(KEY_M)
	await _tap(KEY_ENTER)
	await _tap(KEY_1)
	scene.pin_survival_seed(ESCAPE_SEED)
	await _tap(KEY_1)
	await _tap(KEY_E)
	while scene.survival_status().get("phase", "") in ["workshop", "exploration"] and _int("day") < DAY_LIMIT:
		await _follow_next()
	await _frames(2)
	await _save("13-ending-card")
	if not await _scales_whole():
		return
	if ground_frames == 0 or layer_frames == 0 or acid_frames == 0 or band_frames == 0 or stairs_frames == 0 or parity_frames == 0 or slab_frames == 0 or grid_frames == 0 or light_frames == 0 or teeth_frames == 0 or support_frames == 0 or lit_frames == 0:
		_fail("a look was never measured: ground %d, layers %d, acid over the backdrop %d, acid band %d, stairs %d, stairs beside the deck %d, slab %d, pixel grid %d, acid light %d, teeth %d, supports %d, lit structures %d frames" % [ground_frames, layer_frames, acid_frames, band_frames, stairs_frames, parity_frames, slab_frames, grid_frames, light_frames, teeth_frames, support_frames, lit_frames])
		return
	print("Capture: the ground was measured in %d, the layers in %d, the acid over the backdrop in %d, the acid band in %d, the stairs in %d, the stairs beside the deck in %d, the slab in %d, the pixel grid in %d, the acid light in %d, the teeth in %d, the supports in %d and the lit structures in %d city frames." % [ground_frames, layer_frames, acid_frames, band_frames, stairs_frames, parity_frames, slab_frames, grid_frames, light_frames, teeth_frames, support_frames, lit_frames])
	print("Capture: no map plate covers another or a source but its count in ", city_frames, " city frames.")
	print("Capture: ", ProjectSettings.globalize_path("user://survival"))
	_finish("pass")
	quit(0)

## Whether the window, taken out of the full screen it starts in and set to
## each of WINDOW_SIZES, and then back to full screen,
## shows the screen, still SCREEN in size, at the largest whole multiple of it
## that fits, centred on whole pixels. Godot 4.7.2's compatibility renderer
## copies the screen to the window with nearest filtering, so each of its
## pixels is then a square of whole window pixels. A window that does not take
## the size or mode asked of it fails, and so does a game that did not start
## in full screen.
func _scales_whole() -> bool:
	if DisplayServer.window_get_mode() != DisplayServer.WINDOW_MODE_FULLSCREEN:
		_fail("the game did not start in full screen: mode %d" % DisplayServer.window_get_mode())
		return false
	if not _shown_whole("the full screen the game starts in"):
		return false
	DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_WINDOWED)
	for frame: int in range(RESIZE_FRAMES):
		if DisplayServer.window_get_mode() == DisplayServer.WINDOW_MODE_WINDOWED:
			break
		await process_frame
	if DisplayServer.window_get_mode() != DisplayServer.WINDOW_MODE_WINDOWED:
		_fail("the window did not leave full screen: mode %d" % DisplayServer.window_get_mode())
		return false
	for size: Vector2i in WINDOW_SIZES:
		DisplayServer.window_set_size(size)
		for frame: int in range(RESIZE_FRAMES):
			if DisplayServer.window_get_size() == size:
				break
			await process_frame
		await _frames(2)
		if DisplayServer.window_get_size() != size:
			_fail("the window did not take the size %s: %s" % [size, DisplayServer.window_get_size()])
			return false
		if not _shown_whole("a %dx%d window" % [size.x, size.y]):
			return false
	var screen_size: Vector2i = DisplayServer.screen_get_size()
	DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_FULLSCREEN)
	for frame: int in range(RESIZE_FRAMES):
		if DisplayServer.window_get_mode() == DisplayServer.WINDOW_MODE_FULLSCREEN and DisplayServer.window_get_size() == screen_size:
			break
		await process_frame
	await _frames(2)
	if DisplayServer.window_get_mode() != DisplayServer.WINDOW_MODE_FULLSCREEN or DisplayServer.window_get_size() != screen_size:
		_fail("the window did not fill the %s screen: mode %d, %s" % [screen_size, DisplayServer.window_get_mode(), DisplayServer.window_get_size()])
		return false
	return _shown_whole("full screen at %dx%d" % [screen_size.x, screen_size.y])

## Whether the screen is still SCREEN in size and the window, at its current
## size, shows it at the largest whole multiple of it that fits, centred on
## whole pixels.
func _shown_whole(window: String) -> bool:
	var size: Vector2i = DisplayServer.window_get_size()
	var scale: int = maxi(mini(size.x / SCREEN.x, size.y / SCREEN.y), 1)
	var origin: Vector2 = Vector2((size - SCREEN * scale) / 2)
	var expected: Transform2D = Transform2D(Vector2(scale, 0), Vector2(0, scale), origin)
	var shown: Transform2D = root.get_final_transform()
	var drawn: Vector2i = root.get_texture().get_image().get_size()
	print("Capture: %s shows the %s screen at %s, origin %s (expected %d times at %s)." % [window, drawn, shown.get_scale(), shown.origin, scale, origin])
	if drawn != SCREEN or shown != expected:
		_fail("%s does not show the %s screen at a whole multiple: %s screen, transform %s" % [window, SCREEN, drawn, shown])
		return false
	return true

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
	var line: String = scene.survival_status().get("next", "")
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
		var code: Key = ROUTE_KEYS.get(line.get_slice(" ", 1), KEY_NONE)
		if code != KEY_NONE:
			_key(code, true)
			if line.begins_with("Jump "):
				_key(KEY_SPACE, true)
				_key(KEY_SPACE, false)
				await _until_stance("airborne")
			else:
				_key(code, false)
				await _frames(STEP_FRAMES)
				_key(code, _stance() == "airborne")
			while _stance() == "airborne":
				await process_frame
			_key(code, false)
		else:
			await _frames(STEP_FRAMES)
	else:
		await _frames(STEP_FRAMES)

## Runs with `code` until the chef stands on column `x` or right of it, jumping
## whatever low block stops it.
func _walk(code: Key, x: int) -> void:
	_key(code, true)
	_key(KEY_SHIFT, true)
	var last: int = -1
	var stalled: int = 0
	for frame: int in range(480):
		var chef: Dictionary = scene.survival_status()["chef"]
		var at: int = chef["x"]
		if at >= x and chef["stance"] != "airborne":
			break
		var sub_x: int = chef["sub_x"]
		stalled = stalled + 1 if sub_x == last and chef["stance"] == "standing" else 0
		last = sub_x
		_key(KEY_SPACE, stalled > 6)
		await process_frame
	_key(KEY_SPACE, false)
	_key(code, false)
	_key(KEY_SHIFT, false)
	await _frames(3)

func _stance() -> String:
	var chef: Dictionary = scene.survival_status().get("chef", {})
	var stance: String = chef.get("stance", "")
	return stance

func _until_stance(stance: String) -> void:
	for frame: int in range(60):
		if _stance() == stance:
			return
		await process_frame

## Waits until the chef has moved `distance` sub-cells sideways or further,
## signed, from where it stands now.
func _until_moved(distance: int) -> void:
	var start: int = scene.survival_status()["chef"]["sub_x"]
	for frame: int in range(60):
		var now: int = scene.survival_status()["chef"]["sub_x"]
		if (now - start) * signi(distance) >= absi(distance):
			return
		await process_frame
	_fail("the chef did not move %d sub-cells" % distance)

## Waits until the chef, in the air, is lower than the highest it has been
## since: it is coming down.
func _until_falling() -> void:
	var top: int = scene.survival_status()["chef"]["sub_y"]
	for frame: int in range(60):
		var chef: Dictionary = scene.survival_status()["chef"]
		var now: int = chef["sub_y"]
		if chef["stance"] == "airborne" and now > top:
			return
		top = mini(top, now)
		await process_frame
	_fail("the chef never came down from the jump")

## Holds A until the chef stands on column `x` or left of it.
func _walk_left(x: int) -> void:
	_key(KEY_A, true)
	for frame: int in range(240):
		var chef: Dictionary = scene.survival_status()["chef"]
		var at: int = chef["x"]
		if at <= x:
			break
		await process_frame
	_key(KEY_A, false)
	await _frames(3)

## Holds W until the chef stands on row `y` or above.
func _climb(y: int) -> void:
	_key(KEY_W, true)
	for frame: int in range(240):
		var chef: Dictionary = scene.survival_status()["chef"]
		var at: int = chef["y"]
		if at <= y:
			break
		await process_frame
	_key(KEY_W, false)
	await _frames(3)

## Walks the chef sideways to column x, jumping whatever low block stops it,
## then climbs to row y.
func _walk_to(x: int, y: int) -> void:
	var stalled: int = 0
	var last: int = -1
	for frame: int in range(900):
		var chef: Dictionary = scene.survival_status()["chef"]
		var at_x: int = chef["x"]
		var at_y: int = chef["y"]
		if at_x == x and at_y == y and chef["stance"] != "airborne":
			break
		var sideways: bool = at_x != x
		_key(KEY_A, sideways and x < at_x)
		_key(KEY_D, sideways and x > at_x)
		_key(KEY_W, not sideways and y < at_y)
		_key(KEY_S, not sideways and y > at_y)
		var sub_x: int = chef["sub_x"]
		stalled = stalled + 1 if sideways and sub_x == last and chef["stance"] == "standing" else 0
		last = sub_x
		_key(KEY_SPACE, stalled > 6)
		await process_frame
	for code: Key in [KEY_A, KEY_D, KEY_W, KEY_S, KEY_SPACE]:
		_key(code, false)
	await _frames(3)

func _until_phase(phase: String) -> void:
	for frame: int in range(3000):
		var now: String = scene.survival_status()["phase"]
		if now == phase:
			return
		await process_frame

func _until_next(text: String, limit: int) -> void:
	for frame: int in range(limit):
		var now: String = scene.survival_status()["next"]
		if now.contains(text):
			return
		await process_frame

func _int(field: String) -> int:
	var value: int = scene.survival_status().get(field, 0)
	return value

## Moves the scene's clock past the screen's longest eased motion first.
func _save(screen: String) -> Image:
	clock += SETTLE_SECONDS
	scene.pin_scene_clock(clock)
	await process_frame
	await RenderingServer.frame_post_draw
	var destination: String = ProjectSettings.globalize_path("user://survival/" + screen + ".png")
	var frame: Image = root.get_texture().get_image()
	if frame.save_png(destination) != OK:
		_fail("Could not save " + destination)
		return frame
	print("Capture: ", destination)
	return frame

## Saves a city frame and fails it when one word plate covers another, or a
## plate other than a source's count covers a source, when a plate lies under
## a HUD panel or note, when it
## drew no plate beyond a count for each source labelled, when its words name
## a source's kind or anything but the goals NEXT names, or leave out the goal
## NEXT names to walk to, or when a look it shows is wrong.
func _save_city(screen: String) -> Image:
	var frame: Image = await _save(screen)
	var plates: Array = scene.get_meta("map_plates", [])
	var labelled: Array = scene.get_meta("map_labelled", [])
	var view: Transform2D = scene.get_meta("map_view", Transform2D())
	var sources: int = labelled.size()
	for first: int in range(plates.size()):
		for second: int in range(first + 1, plates.size()):
			var one: Rect2 = plates[first]
			var other: Rect2 = plates[second]
			if one.intersects(other):
				_fail("%s: the plate at %s covers the plate at %s" % [screen, one, other])
				return frame
	if sources == 0 or plates.size() <= sources:
		_fail("%s: %d plates for %d sources" % [screen, plates.size(), sources])
		return frame
	var areas: Array = scene.get_meta("map_source_areas", [])
	var counts: Array = scene.get_meta("map_counts", [])
	for plate: Rect2 in plates:
		if counts.has(plate):
			continue
		for area: Rect2 in areas:
			if area.intersects(plate):
				_fail("%s: the plate at %s covers the source at %s" % [screen, plate, area])
				return frame
	var panels: Array = scene.get_meta("hud_panels", [])
	var notes: Array = scene.get_meta("hud_notes", [])
	var hud: Array = panels + notes
	for plate: Rect2 in plates:
		for area: Rect2 in hud:
			if plate.intersects(area):
				_fail("%s: the plate at %s lies under the HUD at %s" % [screen, plate, area])
				return frame
	var misnamed: String = _misnamed()
	if misnamed != "":
		_fail("%s: %s" % [screen, misnamed])
		return frame
	print("Capture: %s at zoom %.2f." % [screen, view.x.x])
	if not _ground_leads(frame, view, screen) or not _layers(frame, view, screen) or not _band_shows(frame, view, screen) or not _stairs_lit(frame, view, screen) or not _slab_tilts(frame, view, screen) or not _on_grid(frame, view, screen, plates) or not _acid_lights(frame, view, screen) or not _teeth_show(frame, screen) or not _fangs_vary(screen) or not _one_row_of_teeth(screen) or not _supports_open(frame, view, screen) or not _shops_lit(frame, view, screen):
		return frame
	city_frames += 1
	return frame

## What is wrong with the map's words, or nothing: a word naming a source's
## kind, a name other than of the goals NEXT names, or a goal NEXT names to
## walk to left unnamed on the map and by its pointer.
func _misnamed() -> String:
	var words: Array = scene.get_meta("map_words", [])
	var line: String = scene.survival_status().get("next", "")
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

## Whether the frame shows the scene's rectangle at corner of size whole, inside
## the play area.
func _shows(view: Transform2D, corner: Vector2, size: Vector2) -> bool:
	return PLAY.encloses(Rect2(view * corner, size * view.x.x))

## Whether the walkway's lit top edge leads the scenery right above it by
## GROUND_LEAD, over the LEAD_CELLS the frame shows, when it shows two or more;
## the control is that the scenery is not black.
func _ground_leads(frame: Image, view: Transform2D, screen: String) -> bool:
	var ground: float = 0.0
	var behind: float = 0.0
	var shown: int = 0
	for cell: Vector2i in LEAD_CELLS:
		var corner: Vector2 = Vector2(MAP_ORIGIN + cell * CELL)
		if not _shows(view, corner, Vector2(CELL, CELL + EDGE_ROWS)):
			continue
		ground += _luminance(frame, view, corner + Vector2(0, CELL), EDGE_ROWS)
		behind += _luminance(frame, view, corner, CELL)
		shown += 1
	if shown < 2:
		return true
	ground /= shown
	behind /= shown
	ground_frames += 1
	print("Capture: %s: the ground leads the scenery above it by %.3f over %d cells (ground %.3f, scenery %.3f, at least %.3f)." % [screen, ground - behind, shown, ground, behind, GROUND_LEAD])
	if behind <= 0.0 or ground - behind < GROUND_LEAD:
		_fail("%s: the ground does not read over the scenery behind it" % screen)
		return false
	return true

## Whether the frame's values fall in three layers, over the LEAD_CELLS it
## shows, when it shows two or more: the walkways' faces darkest, the backdrop
## in the cells above them brighter by FACE_GAP, and, where the frame shows the
## acid band clear of the sea, the band glowing green over ACID_SHARE of it,
## those pixels brighter than the backdrop by ACID_GAP. The control is that
## the faces are not black.
func _layers(frame: Image, view: Transform2D, screen: String) -> bool:
	var face: float = 0.0
	var behind: float = 0.0
	var shown: int = 0
	for cell: Vector2i in LEAD_CELLS:
		var corner: Vector2 = Vector2(MAP_ORIGIN + cell * CELL)
		if not _shows(view, corner, Vector2(CELL, CELL + FACE_ROWS.y)):
			continue
		face += _luminance(frame, view, corner + Vector2(0, CELL + FACE_ROWS.x), FACE_ROWS.y - FACE_ROWS.x)
		behind += _luminance(frame, view, corner, CELL)
		shown += 1
	if shown < 2:
		return true
	face /= shown
	behind /= shown
	layer_frames += 1
	print("Capture: %s: the walkways' faces read %.3f under the backdrop's %.3f over %d cells (at least %.3f darker)." % [screen, face, behind, shown, FACE_GAP])
	if face <= 0.0 or behind - face < FACE_GAP:
		_fail("%s: the walkways do not read darker than the backdrop" % screen)
		return false
	if not _band_clear(view):
		return true
	var band: Vector2 = _acid_band(frame)
	acid_frames += 1
	print("Capture: %s: the acid band glows green over %.3f of it, at %.3f over the backdrop's %.3f (at least %.3f of it, %.3f brighter)." % [screen, band.x, band.y, behind, ACID_SHARE, ACID_GAP])
	if band.x < ACID_SHARE or band.y - behind < ACID_GAP:
		_fail("%s: the acid band does not read brighter than the backdrop" % screen)
		return false
	return true

## Whether a frame at no acid that shows the acid band clear of the sea shows
## it glowing green over ACID_SHARE of it.
func _band_shows(frame: Image, view: Transform2D, screen: String) -> bool:
	if _int("acid") != 0 or not _band_clear(view):
		return true
	var share: float = _acid_band(frame).x
	band_frames += 1
	print("Capture: %s: the acid band at row %d glows green over %.3f of it (at least %.3f)." % [screen, _band_top(), share, ACID_SHARE])
	if share < ACID_SHARE:
		_fail("%s: the acid band does not show green at no acid" % screen)
		return false
	return true

## The screen row of the acid band's surface in the frame the status describes.
func _band_top() -> int:
	var status: Dictionary = scene.survival_status()
	var acid: int = status["acid"]
	var full: int = status["full_acid"]
	return floori(BAND_LOW - clampf(float(acid) / full, 0.0, 1.0) * (BAND_LOW - BAND_HIGH))

## Whether the sea in the scene, drawn over the acid band, reaches none of the
## BAND_ROWS under the band's surface.
func _band_clear(view: Transform2D) -> bool:
	return (view * Vector2(0, _acid_surface() - SEA_REACH)).y >= _band_top() + BAND_ROWS.y

## Over the BAND_ROWS under the acid band's surface across the screen, every
## other column: the share of pixels whose green leads their red and blue by
## ACID_GREEN, as x, and their mean luminance, as y.
func _acid_band(frame: Image) -> Vector2:
	var top: int = _band_top()
	var glow: float = 0.0
	var green: int = 0
	var count: int = 0
	for y: int in range(top + BAND_ROWS.x, top + BAND_ROWS.y):
		for x: int in range(0, int(PLAY.size.x), 2):
			var pixel: Color = frame.get_pixel(x, y)
			count += 1
			if pixel.g - maxf(pixel.r, pixel.b) >= ACID_GREEN:
				green += 1
				glow += pixel.get_luminance()
	return Vector2(float(green) / count, glow / maxi(green, 1))

## Whether the EDGE_ROWS just above each stair's diagonal, where it is walked
## on, lead the EDGE_ROWS just under it by STEP_LEAD, when the frame shows both
## stairs, and read at STAIR_PARITY of the STAIR_DECK_CELLS' top EDGE_ROWS,
## when it shows that deck too; the columns at the cell's ends, where a band
## would leave the cell, are left out.
func _stairs_lit(frame: Image, view: Transform2D, screen: String) -> bool:
	var walked: float = 0.0
	var under: float = 0.0
	var columns: Vector2i = Vector2i(EDGE_ROWS - 1, CELL - EDGE_ROWS)
	for cell: Vector2i in STAIR_CELLS:
		var corner: Vector2 = Vector2(MAP_ORIGIN + (cell + Vector2i.DOWN) * CELL)
		if not _shows(view, corner - Vector2(0, EDGE_ROWS), Vector2(CELL, CELL + 2 * EDGE_ROWS)):
			return true
	for cell: Vector2i in STAIR_CELLS:
		var corner: Vector2 = Vector2(MAP_ORIGIN + (cell + Vector2i.DOWN) * CELL)
		walked += _band(frame, view, corner, CELL, columns, Vector2i(1 - EDGE_ROWS, 1))
		under += _band(frame, view, corner, CELL, columns, Vector2i(1, 1 + EDGE_ROWS))
	walked /= STAIR_CELLS.size()
	under /= STAIR_CELLS.size()
	stairs_frames += 1
	print("Capture: %s: the stairs' walked-on part leads the part under it by %.3f (walked %.3f, under %.3f, at least %.3f)." % [screen, walked - under, walked, under, STEP_LEAD])
	if under <= 0.0 or walked - under < STEP_LEAD:
		_fail("%s: the stairs' walked-on part does not read over the part under it" % screen)
		return false
	var deck: float = 0.0
	for cell: Vector2i in STAIR_DECK_CELLS:
		var corner: Vector2 = Vector2(MAP_ORIGIN + (cell + Vector2i.DOWN) * CELL)
		if not _shows(view, corner, Vector2(CELL, EDGE_ROWS)):
			return true
		deck += _luminance(frame, view, corner, EDGE_ROWS)
	deck /= STAIR_DECK_CELLS.size()
	parity_frames += 1
	print("Capture: %s: the stairs' walked-on part reads at %.3f of the deck's top edge (walked %.3f, deck %.3f, at least %.3f)." % [screen, walked / deck, walked, deck, STAIR_PARITY])
	if deck <= 0.0 or walked < STAIR_PARITY * deck:
		_fail("%s: the stairs' walked-on part reads darker than the deck's top edge" % screen)
		return false
	return true

## Whether the fallen slab's lit top edge, the EDGE_ROWS under the line it
## tilts along, leads the EDGE_ROWS of scenery just above that line by
## GROUND_LEAD, when the frame shows the slab.
func _slab_tilts(frame: Image, view: Transform2D, screen: String) -> bool:
	var corner: Vector2 = Vector2(MAP_ORIGIN + (SLAB_CELL + Vector2i.DOWN) * CELL)
	if not _shows(view, corner - Vector2(0, EDGE_ROWS), Vector2(SLAB_CELLS * CELL, CELL + 2 * EDGE_ROWS)):
		return true
	var columns: Vector2i = Vector2i(0, SLAB_CELLS * CELL)
	var edge: float = _band(frame, view, corner, SLAB_CELLS * CELL, columns, Vector2i(0, EDGE_ROWS))
	var above: float = _band(frame, view, corner, SLAB_CELLS * CELL, columns, Vector2i(-EDGE_ROWS, 0))
	slab_frames += 1
	print("Capture: %s: the fallen slab's top edge leads the scenery above its tilt by %.3f (edge %.3f, scenery %.3f, at least %.3f)." % [screen, edge - above, edge, above, GROUND_LEAD])
	if above <= 0.0 or edge - above < GROUND_LEAD:
		_fail("%s: the fallen slab's top edge does not run along its tilt" % screen)
		return false
	return true

## Whether, while the camera rests, MOTION_SHARE of the acid's pixels change
## between two frames MOTION_SECONDS apart on the scene's clock: of
## the pixels whose green leads by ACID_GREEN in the first, in the BAND_ROWS
## under the acid band's surface where the sea does not cover them, else in
## the SEA_ROWS under the sea's surface. The control is that the camera did not
## move between the frames and that the rows read are on the play area.
func _acid_moves(screen: String) -> bool:
	var view: Transform2D = scene.get_meta("map_view", Transform2D())
	var top: int = _band_top() + BAND_ROWS.x
	var bottom: int = _band_top() + BAND_ROWS.y
	var which: String = "acid band"
	if not _band_clear(view):
		var sea: int = floori((view * Vector2(0, _acid_surface())).y)
		top = sea + SEA_ROWS.x
		bottom = sea + SEA_ROWS.y
		which = "sea"
	var first: Image = await _frame()
	clock += MOTION_SECONDS
	scene.pin_scene_clock(clock)
	var second: Image = await _frame()
	var still: Transform2D = scene.get_meta("map_view", Transform2D())
	if not still.is_equal_approx(view) or top < PLAY.position.y or bottom > PLAY.end.y:
		_fail("%s: the acid's motion cannot be read: the camera moved from %s to %s, or rows %d to %d leave the play area" % [screen, view, still, top, bottom])
		return false
	var moved: int = 0
	var count: int = 0
	for y: int in range(top, bottom):
		for x: int in range(0, int(PLAY.size.x), 2):
			var before: Color = first.get_pixel(x, y)
			var after: Color = second.get_pixel(x, y)
			if before.g - maxf(before.r, before.b) < ACID_GREEN:
				continue
			count += 1
			if maxf(absf(before.r - after.r), maxf(absf(before.g - after.g), absf(before.b - after.b))) >= MOTION_STEP:
				moved += 1
	var share: float = float(moved) / maxi(count, 1)
	print("Capture: %s: %.3f of the %d green pixels of the %s in rows %d to %d moved in %.2fs of the scene's clock (at least %.3f)." % [screen, share, count, which, top, bottom, MOTION_SECONDS, MOTION_SHARE])
	if share < MOTION_SHARE:
		_fail("%s: the %s stands still" % [screen, which])
		return false
	return true

## Whether the city draws FRAME_RATE frames a second or more over RATE_FRAMES
## frames while the camera rests; run with vsync off, so frames are not held to
## the display's rate.
func _draws_fast(screen: String) -> bool:
	await _frames(2)
	var start: int = Time.get_ticks_usec()
	await _frames(RATE_FRAMES)
	var rate: float = RATE_FRAMES / ((Time.get_ticks_usec() - start) / 1000000.0)
	print("Capture: %s: the city drew %.1f frames a second over %d frames at %s (at least %.0f)." % [screen, rate, RATE_FRAMES, root.get_texture().get_size(), FRAME_RATE])
	if rate < FRAME_RATE:
		_fail("%s: the city draws fewer than %.0f frames a second" % [screen, FRAME_RATE])
		return false
	return true

## Whether, at a zoom of two or more, GRID_SHARE of the scene pixels the frame
## shows below the sea's surface, clear of the word plates, are each one colour,
## so the finishing pass keeps to the scene's pixels. The control is that
## GRID_GREEN of the pixels read hold the acid's green.
func _on_grid(frame: Image, view: Transform2D, screen: String, plates: Array) -> bool:
	var zoom: int = roundi(view.x.x)
	var sea: int = ceili((view * Vector2(0, _acid_surface())).y)
	if zoom < 2 or sea >= PLAY.end.y - 8 * zoom:
		return true
	var origin: Vector2i = Vector2i(view.origin.round())
	var first_x: int = posmod(origin.x, zoom)
	var first_y: int = maxi(sea, int(PLAY.position.y))
	first_y += posmod(origin.y - first_y, zoom)
	var whole: int = 0
	var count: int = 0
	var green: int = 0
	for y: int in range(first_y, int(PLAY.end.y) - zoom + 1, zoom):
		for x: int in range(first_x, int(PLAY.size.x) - zoom + 1, zoom):
			var block: Rect2 = Rect2(x, y, zoom, zoom)
			if plates.any(func(plate: Rect2) -> bool: return plate.intersects(block)):
				continue
			var corner: Color = frame.get_pixel(x, y)
			var one: bool = true
			for dy: int in range(zoom):
				for dx: int in range(zoom):
					var other: Color = frame.get_pixel(x + dx, y + dy)
					if maxf(absf(corner.r - other.r), maxf(absf(corner.g - other.g), absf(corner.b - other.b))) > GRID_TOLERANCE:
						one = false
			count += 1
			whole += 1 if one else 0
			green += 1 if corner.g - maxf(corner.r, corner.b) >= ACID_GREEN else 0
	if count == 0:
		return true
	var share: float = float(whole) / count
	grid_frames += 1
	print("Capture: %s: %.3f of %d scene pixels below the sea's surface are one colour, %.3f of them green (at least %.3f one colour, %.3f green)." % [screen, share, count, float(green) / count, GRID_SHARE, GRID_GREEN])
	if float(green) / count < GRID_GREEN or share < GRID_SHARE:
		_fail("%s: the finished sea does not keep to the scene's pixels" % screen)
		return false
	return true

## Whether, over the columns where the acid shows under its surface, the band's
## where the sea does not cover it and the sea's otherwise, the LIT_ROWS above
## that surface lead the UNLIT_ROWS of backdrop higher up in green by
## LIGHT_GAP, when the play area shows both. The control is that the acid shows
## under the surface in LIGHT_COLUMNS of the columns read.
func _acid_lights(frame: Image, view: Transform2D, screen: String) -> bool:
	var surface: int = _band_top() if _band_clear(view) else floori((view * Vector2(0, _acid_surface())).y)
	if surface + UNLIT_ROWS.x < PLAY.position.y or surface + ACID_UNDER.back() >= PLAY.end.y:
		return true
	var lit: float = 0.0
	var unlit: float = 0.0
	var columns: int = 0
	var read: int = 0
	for x: int in range(0, int(PLAY.size.x), 2):
		read += 1
		if not ACID_UNDER.any(func(row: int) -> bool: return _green(frame.get_pixel(x, surface + row)) >= ACID_GREEN):
			continue
		columns += 1
		for y: int in range(LIT_ROWS.x, LIT_ROWS.y):
			lit += maxf(_green(frame.get_pixel(x, surface + y)), 0.0)
		for y: int in range(UNLIT_ROWS.x, UNLIT_ROWS.y):
			unlit += maxf(_green(frame.get_pixel(x, surface + y)), 0.0)
	var share: float = float(columns) / read
	lit /= maxi(columns, 1) * (LIT_ROWS.y - LIT_ROWS.x)
	unlit /= maxi(columns, 1) * (UNLIT_ROWS.y - UNLIT_ROWS.x)
	light_frames += 1
	print("Capture: %s: over %.3f of the columns, where the acid shows under its surface at row %d, the rows above it lead the backdrop above them in green by %.3f (lit %.3f, backdrop %.3f, at least %.3f)." % [screen, share, surface, lit - unlit, lit, unlit, LIGHT_GAP])
	if share < LIGHT_COLUMNS or lit - unlit < LIGHT_GAP:
		_fail("%s: the acid's light does not rise above its surface" % screen)
		return false
	return true

## Whether, over the TEETH_ROWS at the top of the play area, every other
## column, TEETH_SHARE of the pixels the teeth cover where the World layer's
## `jaw_gum` and `jaw_fangs` metas put them read within TEETH_TOLERANCE of the
## picture's colour in each channel. The control is that the teeth cover at
## least TEETH_COVER of the pixels read, so the rows are the teeth's.
func _teeth_show(frame: Image, screen: String) -> bool:
	var covered: int = 0
	var matched: int = 0
	var count: int = 0
	for y: int in range(int(PLAY.position.y), int(PLAY.position.y) + TEETH_ROWS):
		for x: int in range(0, int(PLAY.size.x), 2):
			count += 1
			var tooth: Color = _tooth(x, y)
			if tooth.a < 0.5:
				continue
			covered += 1
			var pixel: Color = frame.get_pixel(x, y)
			if maxf(absf(pixel.r - tooth.r), maxf(absf(pixel.g - tooth.g), absf(pixel.b - tooth.b))) <= TEETH_TOLERANCE:
				matched += 1
	var cover: float = float(covered) / count
	var share: float = float(matched) / maxi(covered, 1)
	teeth_frames += 1
	print("Capture: %s: %.3f of the %.3f of the top %d rows of the play area the teeth cover show them (at least %.3f of at least %.3f)." % [screen, share, cover, TEETH_ROWS, TEETH_SHARE, TEETH_COVER])
	if cover < TEETH_COVER or share < TEETH_SHARE:
		_fail("%s: the teeth do not show along the top of the play area" % screen)
		return false
	return true

## The teeth picture's pixel the World layer drew at screen pixel (x, y), from
## its `jaw_gum` and `jaw_fangs` metas; clear where it drew none.
func _tooth(x: int, y: int) -> Color:
	var world: Node = scene.get_node("World")
	var gum: Vector2 = world.get_meta("jaw_gum", Vector2(-INF, -INF))
	var row: int = y - int(gum.y)
	if row >= 0 and row < GUM_ROWS:
		return teeth.get_pixel(GUM_LEFT + posmod(x - int(gum.x), JAW_RUN), row)
	for fang: Dictionary in world.get_meta("jaw_fangs", []):
		var at: Vector2 = fang["at"]
		var width: float = fang["width"]
		var left: float = fang["left"]
		if x < int(at.x) or x >= int(at.x + width):
			continue
		row = y - int(at.y)
		if row < 0 or row >= teeth.get_height():
			return Color(0, 0, 0, 0)
		return teeth.get_pixel(int(left) + x - int(at.x), row)
	return Color(0, 0, 0, 0)

## Whether the fangs the World layer drew across the screen, by its
## `jaw_fangs` meta, differ from each neighbour in picture, and no RUN_FANGS of
## them in a row recur elsewhere on the screen in the same order of pictures,
## whatever their drops, so the row repeats nowhere within the screen's width.
## The control is that the screen holds more than twice RUN_FANGS fangs, so a
## repeat within it could be seen.
func _fangs_vary(screen: String) -> bool:
	var row: Array[String] = []
	var pictures: Array[int] = []
	var gum: Vector2 = scene.get_node("World").get_meta("jaw_gum", Vector2(-INF, -INF))
	for fang: Dictionary in scene.get_node("World").get_meta("jaw_fangs", []):
		var at: Vector2 = fang["at"]
		var width: float = fang["width"]
		var left: float = fang["left"]
		if at.x + width <= 0.0 or at.x >= PLAY.size.x:
			continue
		pictures.append(int(left))
		row.append("%d@%d" % [int(left), int(at.y - gum.y)])
	if row.size() <= 2 * RUN_FANGS:
		_fail("%s: the fangs' variety cannot be read: %d fangs on the screen" % [screen, row.size()])
		return false
	for i: int in range(1, pictures.size()):
		if pictures[i] == pictures[i - 1]:
			_fail("%s: two neighbouring fangs are the same picture: %s" % [screen, row])
			return false
	var runs: Dictionary = {}
	for i: int in range(row.size() - RUN_FANGS + 1):
		var run: String = str(pictures.slice(i, i + RUN_FANGS))
		if runs.has(run):
			_fail("%s: the fangs repeat within the screen: pictures %s at %d and %d in %s" % [screen, run, runs[run], i, row])
			return false
		runs[run] = i
	print("Capture: %s: %d fangs across the screen, no neighbours alike and no %d in a row recurring." % [screen, row.size(), RUN_FANGS])
	return true

## Whether, over the rows from under the teeth's gum down to their lowest
## tip, every other row and column, at most SECOND_ROW of the throat's pixels
## the World layer drew there, by its `throat_at` meta, are in the colours the
## teeth picture draws its fangs in and not its gum, so the teeth hang in one
## row. The control is that at least TEETH_FANGED of the teeth's own pixels
## over the same rows, by the `jaw_fangs` meta, are in those colours.
func _one_row_of_teeth(screen: String) -> bool:
	var world: Node = scene.get_node("World")
	var at: Vector2 = world.get_meta("throat_at", Vector2(-INF, -INF))
	var gum: Vector2 = world.get_meta("jaw_gum", Vector2(-INF, -INF))
	var lowest: int = int(gum.y)
	for fang: Dictionary in world.get_meta("jaw_fangs", []):
		var place: Vector2 = fang["at"]
		var width: float = fang["width"]
		var left: float = fang["left"]
		for column: int in range(int(left), int(left + width)):
			lowest = maxi(lowest, int(place.y) + teeth_bottom[column])
	var width: int = throat.get_width()
	var read: int = 0
	var fanged: int = 0
	var teeth_read: int = 0
	var teeth_fanged: int = 0
	for y: int in range(maxi(int(gum.y) + GUM_ROWS, 0), lowest + 1, 2):
		var row: int = y - floori(at.y)
		if row < 0 or row >= throat.get_height():
			continue
		for x: int in range(0, int(PLAY.size.x), 2):
			var offset: int = x - floori(at.x)
			var copy: int = floori(float(offset) / width)
			var column: int = offset - copy * width
			if copy != 0:
				column = width - 1 - column
			read += 1
			if fang_colours.has(throat.get_pixel(column, row).to_html(false)):
				fanged += 1
			var tooth: Color = _tooth(x, y)
			if tooth.a >= 0.5:
				teeth_read += 1
				if fang_colours.has(tooth.to_html(false)):
					teeth_fanged += 1
	var share: float = float(fanged) / maxi(read, 1)
	var control: float = float(teeth_fanged) / maxi(teeth_read, 1)
	print("Capture: %s: %.3f of the throat behind the teeth, rows %d to %d, is in the fangs' colours, against %.3f of the teeth (at most %.3f, at least %.3f)." % [screen, share, int(gum.y) + GUM_ROWS, lowest, control, SECOND_ROW, TEETH_FANGED])
	if read == 0 or control < TEETH_FANGED:
		_fail("%s: the fangs' colours cannot be read: %d throat pixels, %.3f of the teeth" % [screen, read, control])
		return false
	if share > SECOND_ROW:
		_fail("%s: a second row of teeth hangs behind the first" % screen)
		return false
	return true

func _fang_colours(picture: Image) -> Dictionary:
	var gum: Dictionary = {}
	for y: int in range(GUM_ROWS):
		for x: int in range(picture.get_width()):
			gum[picture.get_pixel(x, y).to_html(false)] = true
	var colours: Dictionary = {}
	for y: int in range(GUM_ROWS, picture.get_height()):
		for x: int in range(picture.get_width()):
			var pixel: Color = picture.get_pixel(x, y)
			if pixel.a >= 0.5 and not gum.has(pixel.to_html(false)):
				colours[pixel.to_html(false)] = true
	return colours

func _bottoms(picture: Image) -> Array[int]:
	var bottoms: Array[int] = []
	for x: int in range(picture.get_width()):
		var lowest: int = -1
		for y: int in range(picture.get_height()):
			if picture.get_pixel(x, y).a >= 0.5:
				lowest = y
		bottoms.append(lowest)
	return bottoms


## Whether at most SUPPORT_SHARE of the play area's columns, every other one,
## hold a run of SUPPORT_DARK pixels SUPPORT_RUN scene pixels tall or more, so
## the supports do not stand in dark lines across the backdrop. The control is
## that a dark run a cell tall or more is found, so dark structures are read.
func _supports_open(frame: Image, view: Transform2D, screen: String) -> bool:
	var run_rows: int = SUPPORT_RUN * roundi(view.x.x)
	var long: int = 0
	var count: int = 0
	var longest: int = 0
	for x: int in range(0, int(PLAY.size.x), 2):
		count += 1
		var run: int = 0
		var best: int = 0
		for y: int in range(int(PLAY.position.y), int(PLAY.end.y)):
			run = run + 1 if frame.get_pixel(x, y).get_luminance() < SUPPORT_DARK else 0
			best = maxi(best, run)
		longest = maxi(longest, best)
		long += 1 if best >= run_rows else 0
	var share: float = float(long) / count
	support_frames += 1
	print("Capture: %s: %.3f of the columns hold a dark run %d rows tall or more, the longest %d rows (at most %.3f)." % [screen, share, run_rows, longest, SUPPORT_SHARE])
	if longest < CELL * roundi(view.x.x):
		_fail("%s: no dark structure is read in the play area" % screen)
		return false
	if share > SUPPORT_SHARE:
		_fail("%s: the supports stand in dark lines across the backdrop" % screen)
		return false
	return true

## Whether, at no acid, before anything lit is destroyed, LIT_SHARE of the
## pixels of the lit structures the frame shows whole read warm and bright:
## red over LIT_RED and leading blue by LIT_WARMTH. The control is that the
## frame shows at least one.
func _shops_lit(frame: Image, view: Transform2D, screen: String) -> bool:
	if _int("acid") != 0:
		return true
	var warm: int = 0
	var count: int = 0
	for area: Rect2 in LIT_AREAS:
		if not _shows(view, area.position, area.size):
			continue
		for y: int in range(int(area.size.y)):
			for x: int in range(int(area.size.x)):
				var at: Vector2 = view * (area.position + Vector2(x + 0.5, y + 0.5))
				var pixel: Color = frame.get_pixelv(Vector2i(at.floor()))
				count += 1
				if pixel.r >= LIT_RED and pixel.r - pixel.b >= LIT_WARMTH:
					warm += 1
	if count == 0:
		return true
	var share: float = float(warm) / count
	lit_frames += 1
	print("Capture: %s: %.3f of the lit structures' %d scene pixels read warm and bright (at least %.3f)." % [screen, share, count, LIT_SHARE])
	if share < LIT_SHARE:
		_fail("%s: the shops, stalls and vending machines do not read lit" % screen)
		return false
	return true

## How far a colour's green leads its red and blue, from -1 to 1.
func _green(pixel: Color) -> float:
	return pixel.g - maxf(pixel.r, pixel.b)

## The next drawn frame.
func _frame() -> Image:
	await process_frame
	await RenderingServer.frame_post_draw
	return root.get_texture().get_image()

## The scene row of the acid's surface in the frame the status describes.
func _acid_surface() -> float:
	var status: Dictionary = scene.survival_status()
	var acid: int = status["acid"]
	var full: int = status["full_acid"]
	return ACID_LOW - clampf(float(acid) / full, 0.0, 1.0) * (ACID_LOW - MAP_ORIGIN.y)

## The mean luminance of a band along a line that falls one CELL from corner
## across width scene pixels: in each of the columns from columns.x to
## columns.y, the rows from rows.x to rows.y under the line, a negative row
## above it. Each scene pixel is read where the camera puts it.
func _band(frame: Image, view: Transform2D, corner: Vector2, width: int, columns: Vector2i, rows: Vector2i) -> float:
	var total: float = 0.0
	for x: int in range(columns.x, columns.y):
		var line: int = floori(float(x * CELL) / width)
		for row: int in range(rows.x, rows.y):
			total += _pixel(frame, view, corner + Vector2(x, line + row))
	return total / ((columns.y - columns.x) * (rows.y - rows.x))

## The mean luminance of a cell-wide band of scene rows whose top-left is
## corner.
func _luminance(frame: Image, view: Transform2D, corner: Vector2, rows: int) -> float:
	var total: float = 0.0
	for y: int in range(rows):
		for x: int in range(CELL):
			total += _pixel(frame, view, corner + Vector2(x, y))
	return total / (CELL * rows)

## The luminance of the screen pixel at the middle of the scene pixel at point.
func _pixel(frame: Image, view: Transform2D, point: Vector2) -> float:
	var at: Vector2 = view * (point + Vector2(0.5, 0.5))
	return frame.get_pixelv(Vector2i(at.floor())).get_luminance()

func _fail(message: String) -> void:
	push_error(message)
	_finish("fail: " + message)
	quit(1)

## Prints the run's result line once: a failure can be followed by another
## before quit() takes effect.
func _finish(result: String) -> void:
	if not finished:
		finished = true
		print("Capture: finished: ", result)

func _tap(code: Key) -> void:
	_key(code, true)
	await _frames(2)
	_key(code, false)
	await _frames(8)

## Moves the scene's clock by the frame's delta, as the real clock would.
func _tick() -> void:
	clock += root.get_process_delta_time()
	scene.pin_scene_clock(clock)

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
