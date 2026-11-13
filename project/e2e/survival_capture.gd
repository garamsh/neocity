class_name SurvivalCapture
extends SceneTree
## Captures real rendered frames of a played survival run into user://survival/,
## and fails when a city frame's ground does not read over the scenery behind it,
## its walkways, backdrop and acid band do not read dark to bright, the acid
## band behind the map does not show green at no acid, a deck misses its flat
## collision floor, the acid's light does not rise
## above its surface, the acid stands still between two
## frames a moment apart on the scene's clock, the camera draws the scene at a
## zoom other than once or twice its size, the teeth, the backdrop or the acid
## are drawn in pixels larger than a screen pixel at twice the scene's size, the
## city draws fewer than 60 frames a second,
## one of its word plates covers another, a source other than the source's
## own count, or lies under a HUD panel or note, its words name a source's kind or a place other than the goals
## NEXT names, or leave out the goal NEXT names to walk to, the throat's fangs do not hang from the top of the play area,
## a support reaches down from a walkway too high for it, the shops, stall
## and vending machines do not read warm and lit, or a window of another size,
## or full screen, does not show the 1920x1080 canvas at fractional scale
## with centered letterboxing. The camera shows a part of
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
## The acid reached while advancing to the fixed map's first damaged stage.
const DAMAGE_ACID: int = 45
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
## kitchen loft, the upper walkway and the shop roof. Each walkway's deck runs
## under its cell's floor line, where an actor's feet are.
const LEAD_CELLS: Array[Vector2i] = [Vector2i(2, 6), Vector2i(4, 8), Vector2i(8, 8), Vector2i(18, 5), Vector2i(19, 5)]
## How much brighter, in mean luminance from 0 to 1, the walkway's lit top
## edge under those cells reads than the scenery in the cells, above the deck:
## the least on the captured city frames is 0.068559, against 0.038126
## over the same cells and bands in the screen-sized mockup.
const GROUND_LEAD: float = 0.06
## The rows of a walkway's lit top edge, as the city draws it: the ground a
## player walks on, read over the scenery above it.
const EDGE_ROWS: int = 4
## The rows of a walkway's face, under its lit top edge, that the value layers
## read as the foreground, clear of the edge and the deck's bottom.
const FACE_ROWS: Vector2i = Vector2i(8, 24)
## The screen row of the acid band's surface at no acid and at the acid that
## ends the run, as the city draws it; it rises evenly between them.
const BAND_LOW: int = 489
const BAND_HIGH: int = 240
## The screen rows under the band's surface that the value layers read as the
## acid.
const BAND_ROWS: Vector2i = Vector2i(6, 40)
## How much darker, in mean luminance from 0 to 1, the walkways' faces read
## than the backdrop above them, and how much brighter the acid band's glowing
## pixels read than that backdrop.
const FACE_GAP: float = 0.077
## The fewest pixels clear of the plates and the HUD that a cell's face, or
## the scenery above it, is read over.
const CLEAR_PIXELS: int = 32
## How far over the acid band's surface its light reaches what stands there:
## the finish shader's six glow bands of 8 screen pixels.
const ACID_LIGHT_REACH: int = 48
const ACID_GAP: float = 0.05
## How much a pixel's green channel leads its red and blue, from 0 to 1, for
## it to read as glowing acid, and the least share of the band's pixels that
## read so, with its sunken trains, the pillars and the walkways in front:
## the least share on the captured city frames is 0.355331.
const ACID_GREEN: float = 0.1
const ACID_SHARE: float = 0.355
## The formerly sloped cell, whose drawn top must now match its flat collision floor.
const STAIR_CELLS: Array[Vector2i] = [Vector2i(9, 9)]
## The scene row of the acid's surface at no acid, under the map's lowest
## ground row; it rises evenly to the map's top at the acid that ends the run,
## as the city draws it.
const ACID_LOW: int = 518
## How far above that surface, in screen pixels, the sea in the scene covers
## the acid band behind the map: the acid picture's 21 rows over its surface,
## its heave of 3 and a margin of 8. Its light over it never reaches the band's
## rows, which take the band's own light.
const SEA_REACH: int = 32
## The adjacent normal walkway supplies the flat-floor control.
const STAIR_DECK_CELLS: Array[Vector2i] = [Vector2i(10, 9)]
## From the entrance down the stairs onto the lower walkway.
const MARKET_ROUTE: Array[Vector2i] = [Vector2i(11, 9)]
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
## The camera's two zooms, the only ones it draws the scene at.
const ZOOMS: Array[float] = [1.0, 2.0]
## Frames the camera settles after the two actors separate vertically.
const PARTED_SETTLE: int = 60
## The least share, by layer, of its edge pixels, those that differ from a
## neighbour along a row or a column, that differ from both neighbours along
## it, so stand one screen pixel wide or tall; and how far apart, from 0 to 1
## in a channel, two pixels must be to differ. Art drawn in blocks of two or
## more screen pixels holds none but where something else crosses it, such as
## the finishing pass's ripples, which run per screen pixel over the acid
## whatever its picture. Measured on 4-city-collect-prompt with the pictures
## of 17b4d09, drawn in blocks: teeth 0.05, backdrop 0.01, acid 0.15; with
## the layers cut from the mockup, on every city frame that reads them:
## teeth 0.79 to 0.88, backdrop 0.45 to 0.61, acid 0.68 to 0.87.
const DENSE_SHARE: Dictionary[String, float] = {"teeth": 0.4, "backdrop": 0.25, "acid": 0.5, "walkways": 0.5, "shops": 0.65}
const DENSE_TOLERANCE: float = 0.01
## The least edge pixels, along the rows and along the columns, a layer must
## show for its density to be read.
const DENSE_EDGES: int = 400
## The least frames a second the city draws, and over how many frames.
const FRAME_RATE: float = 60.0
const RATE_FRAMES: int = 600
## How far one 100ms movement unit carries a running actor, in sub-cells.
const RUN_UNIT: int = 50
## Longer than the screen's longest eased motion, in seconds of the scene's
## clock.
const SETTLE_SECONDS: float = 0.4
## The rows at the top of the play area the fangs are read over, which hang
## there from the throat; the least share of the pixels the throat's fangs
## cover there that must show them, within TEETH_TOLERANCE in each channel,
## low because a high walkway hides them where it crosses the top of the
## screen; and the least share of the pixels read the fangs must cover.
## The captured city frames show at least 0.313433 of the fangs, covering
## at least 0.035531 of the sample; the same throat cut placed over the
## screen-sized mockup covers 0.044 of it.
const TEETH_ROWS: int = 96
const TEETH_SHARE: float = 0.313
## The largest measured 31.3% colour-error quantile was 0.121569; 0.14 retains a margin for the finishing pass.
const TEETH_TOLERANCE: float = 0.14
const TEETH_COVER: float = 0.035
## How a fang's cream reads in the throat picture: its red and green at least
## these, and its green no less than its blue, which the pink flesh's is.
const FANG_RED: float = 0.75
const FANG_GREEN: float = 0.68
## How dark, in luminance, a pixel of a support reads; how many scene pixels
## tall a dark run must be to read as a long support, longer than a brace;
## and the most of the play area's columns such runs may cross.
const SUPPORT_DARK: float = 0.15
const SUPPORT_RUN: int = 120
const SUPPORT_SHARE: float = 0.07
## The scene rectangles of the lit structures: the kitchen shop's shelf, the
## supply crate's shop, the tomato machine and the noodle stall; how bright a
## pixel's red must be and how far it must lead blue to read warm and lit; and
## the least share of their pixels that do.
const LIT_AREAS: Array[Rect2] = [Rect2(180, 180, 120, 114), Rect2(628, 244, 120, 114), Rect2(404, 328, 56, 62), Rect2(849, 210, 62, 84)]
const LIT_RED: float = 0.6
const LIT_WARMTH: float = 0.2
const LIT_SHARE: float = 0.200

## The size of the screen every check above measures, and the window sizes it
## is shown at afterwards. Saved evidence and density checks use the native
## 1920x1080 output; value and placement checks use this layout grid.
const SCREEN: Vector2i = Vector2i(1280, 720)
const CANVAS: Vector2i = Vector2i(1920, 1080)
const CANVAS_SCALE: float = float(CANVAS.x) / SCREEN.x
const WINDOW_SIZES: Array[Vector2i] = [Vector2i(1280, 720), Vector2i(1600, 900), Vector2i(1920, 1080), Vector2i(2560, 1440), Vector2i(3840, 2160), Vector2i(2176, 1224), Vector2i(1280, 1024)]
## How many frames a window may take to reach the size or mode asked of it.
const RESIZE_FRAMES: int = 120

## The words of a source's kind the map once wrote beside its supply.
const KIND_WORDS: Array[String] = ["BAG", "VEND", "CART", "SHOP"]
## The map's words that are neither a place's name nor a number, such as a
## count: a broken passage's mark and the
## actors' tags.
const MAP_MARKS: Array[String] = ["BROKEN", "P1", "P2", "AI"]

var native_frame: Image
var scene: MainScene
var held: Dictionary[Key, bool] = {}
var city_frames: int = 0
var ground_frames: int = 0
var stairs_frames: int = 0
var parity_frames: int = 0
var layer_frames: int = 0
var acid_frames: int = 0
var band_frames: int = 0
var dense_frames: int = 0
var dense_city_frames: Dictionary[String, int] = {"walkways": 0, "shops": 0}
var character_frames: Dictionary[String, int] = {"chef": 0, "appraiser": 0}
var character_masks: Array[Dictionary] = []
var deck_footprint: Image = Image.create(1280, 720, false, Image.FORMAT_RGBA8)
var deck_alpha_checked: bool = false
var foreground_alpha_checked: bool = false
var foreground_footprint: Image = Image.create(1920, 1080, false, Image.FORMAT_RGBA8)
var finished_ends: int = 0
var destroyed_end: bool = false
var light_frames: int = 0
var teeth_frames: int = 0
var teeth_control_checked: bool = false
var support_frames: int = 0
var lit_frames: int = 0
## The throat picture, which the World layer draws where its `throat_at` meta
## says, beside mirrored copies.
var throat: Image = Image.load_from_file(ProjectSettings.globalize_path("res://levels/monster_mouth_escape/art_hd/backgrounds/throat.png"))
## Where the scene's clock stands.
var clock: float = SCENE_CLOCK
## Set once the run has printed its result line.
var finished: bool = false
var diagnostic_findings: Array[String] = []

func _initialize() -> void:
	Engine.max_fps = roundi(FRAME_RATE)
	_drive.call_deferred()

## A diagnostic run records all reachable findings before emitting its failing verdict.
func _drive() -> void:
	await _run()
	if OS.get_cmdline_user_args().has("--measure-all") and not finished:
		_finish("fail: diagnostic run ended before all checks completed")
		quit(1)

func _run() -> void:
	if not _character_mask_control():
		return
	await _frames(2)
	scene = current_scene as MainScene
	scene.pin_survival_seed(11)
	scene.pin_scene_clock(clock)
	process_frame.connect(_tick)
	if not await _scales_canvas():
		return
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
	await _walk(KEY_D, 11)
	await _save_city("4-city-collect-prompt")
	if not await _acid_moves("4-city-collect-prompt") or not await _draws_fast("4-city-collect-prompt"):
		return
	# A jump in place on the flat lower walkway comes down where it rose.
	_key(KEY_SPACE, true)
	await _until_stance("airborne")
	_key(KEY_SPACE, false)
	await _until_falling()
	await _save_city("28-city-falling")
	await _until_stance("standing")
	await _save_city("29-city-landing")
	await _walk_to(9, 9)
	await _walk_to(7, 8, "chef", true)
	_key(KEY_SHIFT, true)
	_key(KEY_A, true)
	# The first unit after standing still reads as a walk; the second as a run.
	await _until_moved(-2 * RUN_UNIT)
	await _save_city("30-city-running")
	_key(KEY_A, false)
	_key(KEY_SHIFT, false)
	await _frames(3)
	await _walk(KEY_D, 11)
	_key(KEY_D, true)
	_key(KEY_SPACE, true)
	await _until_stance("airborne")
	_key(KEY_SPACE, false)
	await _frames(4)
	await _save_city("27-city-jumping-the-rubble")
	await _walk(KEY_D, 13)
	# The fixed map's one-row rise needs a jump from the lower walkway.
	await _walk_to(9, 9)
	await _walk_to(7, 8, "chef", true)
	await _walk_to(1, 8)
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
	await _walk(KEY_D, 8)
	await _walk_left(7)
	await _save_city("23-city-facing-left")
	await _walk_to(3, 8)
	await _climb(7)
	await _save_city("24-city-on-ladder")
	await _until_phase("workshop")
	await _frames(60)
	await _tap(KEY_E)
	await _tap(KEY_E)
	await _tap(KEY_F)
	await _frames(12)
	# Before the acid rises: down the stairs onto the lower walkway, so the
	# camera shows the stairs behind the chef.
	await _tap(KEY_Q)
	await _frames(40)
	for waypoint: Vector2i in MARKET_ROUTE:
		await _walk_to(waypoint.x, waypoint.y)
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
	# Advance through days until the first stage changes are visible.
	while _int("acid") < DAMAGE_ACID:
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
	if OS.get_cmdline_user_args().has("--fang-diagnostic"):
		_finish("fail: diagnostic measurement only; not an acceptance run")
		quit(0)
		return
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
	if not await _zoom_steps():
		return
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
	if ground_frames == 0 or layer_frames == 0 or acid_frames == 0 or band_frames == 0 or stairs_frames == 0 or parity_frames == 0 or dense_frames == 0 or light_frames == 0 or teeth_frames == 0 or support_frames == 0 or lit_frames == 0:
		_fail("a look was never measured: ground %d, layers %d, acid over the backdrop %d, acid band %d, stairs %d, stairs beside the deck %d, density %d, acid light %d, teeth %d, supports %d, lit structures %d frames" % [ground_frames, layer_frames, acid_frames, band_frames, stairs_frames, parity_frames, dense_frames, light_frames, teeth_frames, support_frames, lit_frames])
		return
	print("Capture: the ground was measured in %d, the layers in %d, the acid over the backdrop in %d, the acid band in %d, the stairs in %d, the stairs beside the deck in %d, the density in %d, the acid light in %d, the teeth in %d, the supports in %d and the lit structures in %d city frames." % [ground_frames, layer_frames, acid_frames, band_frames, stairs_frames, parity_frames, dense_frames, light_frames, teeth_frames, support_frames, lit_frames])
	for layer: String in dense_city_frames:
		if dense_city_frames[layer] == 0:
			_fail("the %s density was never measured" % layer)
			return
	print("Capture: city layer density measured in ", dense_city_frames, " city frames.")
	for role: String in character_frames:
		if character_frames[role] == 0:
			_fail("the %s character was never measured" % role)
			return
	print("Capture: character size and interior density measured in ", character_frames, " city frames.")
	if finished_ends < 4 or not destroyed_end:
		_fail("exposed deck ends were not measured before and after day-five passage replacement")
		return
	print("Capture: finished ends measured on %d representative sides, including the retained Shallows end after day-five passage replacement." % finished_ends)
	print("Capture: no map plate covers another or a source but its count in ", city_frames, " city frames.")
	print("Capture: ", ProjectSettings.globalize_path("user://survival"))
	_finish("pass")
	quit(0)

## Checks full screen and several window sizes against fractional canvas scaling.
func _scales_canvas() -> bool:
	if DisplayServer.window_get_mode() != DisplayServer.WINDOW_MODE_FULLSCREEN:
		_fail("the game did not start in full screen: mode %d" % DisplayServer.window_get_mode())
		return false
	if not _shown_canvas("the full screen the game starts in"):
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
		if not _shown_canvas("a %dx%d window" % [size.x, size.y]):
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
	return _shown_canvas("full screen at %dx%d" % [screen_size.x, screen_size.y])

## Checks the authored canvas contract and the live output size, scale and letterbox.
func _shown_canvas(window: String) -> bool:
	var size: Vector2i = DisplayServer.window_get_size()
	var scale: float = minf(float(size.x) / CANVAS.x, float(size.y) / CANVAS.y)
	var content: Vector2 = Vector2(CANVAS) * scale
	var origin: Vector2 = (Vector2(size) - content) / 2.0
	var shown: Transform2D = root.get_final_transform()
	var drawn: Vector2i = root.get_texture().get_image().get_size()
	print("Capture: %s: canvas %s, output %s, content %s, letterbox %s, final %s." % [window, CANVAS, size, drawn, origin, shown])
	if ProjectSettings.get_setting("display/window/size/viewport_width") != CANVAS.x or ProjectSettings.get_setting("display/window/size/viewport_height") != CANVAS.y or ProjectSettings.get_setting("display/window/stretch/mode") != "canvas_items":
		_fail("authored canvas settings differ from 1920x1080 canvas_items")
		return false
	if not Vector2(drawn).is_equal_approx(content) or not shown.origin.is_equal_approx(origin) or not shown.get_scale().is_equal_approx(Vector2.ONE * scale * CANVAS_SCALE):
		_fail("%s: output scale or centered letterbox differs" % window)
		return false
	return true

## Whether NEXT names what the interact key does: collect, appraise
## or cook, as the controls page lists for it.
func _names_interact(line: String) -> bool:
	for verb: String in ["Collect from ", "Appraise ", "Cook "]:
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
func _walk_to(x: int, y: int, role: String = "chef", jump: bool = false) -> void:
	var left: Key = KEY_A if role == "chef" else KEY_J
	var right: Key = KEY_D if role == "chef" else KEY_L
	var up: Key = KEY_W if role == "chef" else KEY_I
	var down: Key = KEY_S if role == "chef" else KEY_K
	var jump_key: Key = KEY_SPACE if role == "chef" else KEY_SEMICOLON
	var stalled: int = 0
	var last: int = -1
	for frame: int in range(900):
		var body: Dictionary = scene.survival_status()[role]
		var at_x: int = body["x"]
		var at_y: int = body["y"]
		if at_x == x and at_y == y and body["stance"] != "airborne":
			break
		var sideways: bool = at_x != x
		_key(left, sideways and x < at_x)
		_key(right, sideways and x > at_x)
		_key(up, not sideways and y < at_y)
		_key(down, not sideways and y > at_y)
		var sub_x: int = body["sub_x"]
		stalled = stalled + 1 if sideways and sub_x == last and body["stance"] == "standing" else 0
		last = sub_x
		_key(jump_key, (jump and frame < 3) or stalled > 6)
		await process_frame
	for code: Key in [left, right, up, down, jump_key]:
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
	native_frame = frame.duplicate() as Image
	if frame.save_png(destination) != OK:
		_fail("Could not save " + destination)
		return frame
	print("Capture: ", destination)
	# Value and placement checks sample the existing layout grid; density reads native_frame.
	frame.resize(SCREEN.x, SCREEN.y, Image.INTERPOLATE_NEAREST)
	return frame

## Reads actual frame textures; the first city has no screen fade over its native corners.
func _interface_corners(frame: Image, screen: String) -> bool:
	var expected: Dictionary = {"frame_panel.png": Vector2i(368, 46), "frame_button.png": Vector2i(163, 54), "frame_slot.png": Vector2i(73, 79)}
	var drawn: Dictionary = scene.get_meta("interface_frames", {})
	if drawn.is_empty():
		_fail("%s: no actual interface frame textures were observed" % screen)
		return false
	for path: String in drawn:
		var observation: Dictionary = drawn[path]
		var texture: Texture2D = observation["texture"]
		var image: Image = texture.get_image()
		if image.get_size() != expected.get(path.get_file(), Vector2i.ZERO):
			_fail("%s: interface texture %s loaded at %s" % [screen, path, image.get_size()])
			return false
		if screen != "4-city-collect-prompt":
			continue
		var area: Rect2 = observation["area"]
		var compared: int = 0
		for y: int in range(9):
			for x: int in range(9):
				var original: Color = image.get_pixel(x, y)
				var at: Vector2i = Vector2i(area.position) + Vector2i(x, y)
				if original.a < 1.0 or not Rect2i(Vector2i.ZERO, frame.get_size()).has_point(at):
					continue
				if not frame.get_pixelv(at).is_equal_approx(original):
					_fail("%s: %s native corner differs at %s" % [screen, path.get_file(), at])
					return false
				compared += 1
		if compared < 8:
			_fail("%s: fewer than eight opaque native corner pixels for %s" % [screen, path])
			return false
	return true

## The nearest transparent screen pixel beside an opaque character pixel must stay free.
func _character_plate_control(layer: Dictionary, screen: String) -> bool:
	var alpha: Image = layer["alpha"]
	var area: Rect2 = layer["area"]
	if alpha.get_size() != Vector2i(256, 224) or area.size.x < 64.0:
		_fail("%s: source-plate control did not load the actual HD character" % screen)
		return false
	for y: int in range(ceili(area.position.y), floori(area.end.y)):
		for x: int in range(ceili(area.position.x), floori(area.end.x) - 1):
			var at: Vector2 = Vector2(x, y)
			if not _character_opaque(alpha, layer, at + Vector2(0.5, 0.5)) or _character_opaque(alpha, layer, at + Vector2(1.5, 0.5)):
				continue
			if not _plate_covers_character(Rect2(at, Vector2.ONE), layer) or _plate_covers_character(Rect2(at + Vector2.RIGHT, Vector2.ONE), layer):
				_fail("%s: source-plate alpha control confused the nearest opaque and transparent pixels" % screen)
				return false
			return true
	_fail("%s: no opaque character pixel beside a transparent one for the plate control" % screen)
	return false

## Whether a plate intersects the actual drawn character's opaque pixels.
func _plate_covers_character(plate: Rect2, layer: Dictionary) -> bool:
	var character_area: Rect2 = layer["area"]
	var area: Rect2 = plate.intersection(character_area)
	var alpha: Image = layer["alpha"]
	for y: int in range(floori(area.position.y), ceili(area.end.y)):
		for x: int in range(floori(area.position.x), ceili(area.end.x)):
			if _character_opaque(alpha, layer, Vector2(x, y) + Vector2(0.5, 0.5)):
				return true
	return false

## Saves a city frame and fails it when one word plate covers another, or a
## plate other than a source's count covers a source, when a plate lies under
## a HUD panel or note, when it
## drew no plate beyond a count for each source labelled, when its words name
## a source's kind or anything but the goals NEXT names, or leave out the goal
## NEXT names to walk to, or when a look it shows is wrong.
func _save_city(screen: String) -> Image:
	var previous_scale: float = Engine.time_scale
	Engine.time_scale = 0.0
	var frame: Image = await _measure_city(screen)
	# Consume the analysis interval while physics is held, before resuming play.
	await process_frame
	Engine.time_scale = previous_scale
	return frame

func _measure_city(screen: String) -> Image:
	var frame: Image = await _save(screen)
	if not _interface_corners(frame, screen):
		return frame
	var plates: Array = scene.get_meta("map_plates", [])
	var labelled: Array = scene.get_meta("map_labelled", [])
	var view: Transform2D = scene.get_meta("map_view", Transform2D())
	if not _read_decks(screen):
		return frame
	if not StairsCheck.check_floor_lines(scene):
		_fail("%s: a drawn deck missed its collision floor" % screen)
		return frame
	var drawn_sea: float = scene.get_node("World").get_meta("sea_surface", -1.0)
	if drawn_sea != float(_sea_top(view)):
		_fail("%s: drawn near sea %.1f differs from the projected sea below the far shore at %d" % [screen, drawn_sea, _sea_top(view)])
		return frame
	character_masks.clear()
	for layer: Dictionary in scene.get_node("World").get_meta("character_layers", []):
		var sampled: Dictionary = layer.duplicate()
		var texture: Texture2D = layer["texture"]
		sampled["alpha"] = texture.get_image()
		character_masks.append(sampled)
	for layer: Dictionary in character_masks:
		if not _character_plate_control(layer, screen):
			return frame
		for plate: Rect2 in scene.get_meta("map_counts", []):
			if _plate_covers_character(plate, layer):
				_fail("%s: source icon plate covers the actual character silhouette" % screen)
				return frame
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
	print("Capture: %s at zoom %.2f, the scene's origin at %s, the backdrop's layers slid by %s from where they rest." % [screen, view.x.x, view.origin, scene.get_node("World").get_meta("backdrop_layers", [])])
	if not _ground_leads(frame, view, screen, plates) or not _layers(frame, view, screen, plates + hud) or not _band_shows(frame, view, screen) or not _stairs_lit(frame, view, screen) or not _dense(frame, view, screen, plates) or not _acid_lights(frame, view, screen) or not _teeth_show(frame, screen) or not _supports_open(frame, view, screen) or not _shops_lit(frame, view, screen) or not _characters_dense(frame, view, screen, plates + hud):
		return frame
	if not _finished_decks(frame, view, screen, plates + hud):
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

## Whether the projected rectangle lies inside the play area.
func _shows(view: Transform2D, corner: Vector2, size: Vector2) -> bool:
	return PLAY.encloses(Rect2(view * corner, size * view.x.x))

## Stamps the loaded source alpha through the fragment and transform actually drawn.
func _stamp_deck(target: Image, source: Image, cut: Rect2, destination: Rect2, transform: Transform2D, mirrored: bool = false) -> void:
	var area: Rect2 = (transform * destination).abs()
	var fragment: Image = source.get_region(Rect2i(cut))
	if mirrored or destination.size.x < 0:
		fragment.flip_x()
	if destination.size.y < 0:
		fragment.flip_y()
	fragment.resize(maxi(1, roundi(area.size.x)), maxi(1, roundi(area.size.y)), Image.INTERPOLATE_NEAREST)
	target.blend_rect(fragment, Rect2i(Vector2i.ZERO, fragment.get_size()), Vector2i(area.position.round()))

## The nearest opaque/transparent source pixels inside one fragment stay distinct.
func _fragment_alpha_control(source: Image) -> bool:
	for y: int in range(source.get_height()):
		for x: int in range(source.get_width() - 1):
			if source.get_pixel(x, y).a < 0.99 or source.get_pixel(x + 1, y).a > 0.01:
				continue
			var probe: Image = Image.create(2, 1, false, Image.FORMAT_RGBA8)
			_stamp_deck(probe, source, Rect2(x, y, 2, 1), Rect2(0, 0, 2, 1), Transform2D())
			if not _deck_opaque(probe, Vector2(0.5, 0.5)) or _deck_opaque(probe, Vector2(1.5, 0.5)):
				_fail("deck fragment alpha control confused nearest opaque and transparent source pixels")
				return false
			print("Capture: actual deck alpha control retained opaque %s and excluded its nearest transparent neighbour inside the same fragment." % Vector2i(x, y))
			return true
	# HD deck bands are opaque; make the nearest transparent neighbour in a two-pixel control.
	var control: Image = source.get_region(Rect2i(0, 0, 2, 1))
	control.set_pixel(1, 0, Color.TRANSPARENT)
	var probe: Image = Image.create(2, 1, false, Image.FORMAT_RGBA8)
	_stamp_deck(probe, control, Rect2(0, 0, 2, 1), Rect2(0, 0, 2, 1), Transform2D())
	if not _deck_opaque(probe, Vector2(0.5, 0.5)) or _deck_opaque(probe, Vector2(1.5, 0.5)):
		_fail("opaque HD deck control confused its cleared nearest neighbour")
		return false
	print("Capture: opaque HD source control retained the first pixel and excluded its cleared nearest neighbour.")
	return true

## Reconstructs only the deck's drawn footprint, leaving all scenery samples alone.
func _read_decks(screen: String) -> bool:
	deck_footprint.fill(Color.TRANSPARENT)
	var layers: Array = scene.get_node("World").get_meta("deck_layers", [])
	if layers.is_empty():
		_fail("%s: no actual deck fragments were observed" % screen)
		return false
	var loaded: Dictionary = {}
	for layer: Dictionary in layers:
		var texture: Texture2D = layer["texture"]
		var id: int = texture.get_instance_id()
		if not loaded.has(id):
			var pixels: Image = texture.get_image()
			if texture.resource_path != "res://levels/monster_mouth_escape/art_hd/walkway/deck.png" or pixels.get_size() != Vector2i(1920, 504):
				_fail("%s: actual deck texture identity/size differs: %s %s" % [screen, texture.resource_path, pixels.get_size()])
				return false
			pixels.convert(Image.FORMAT_RGBA8)
			loaded[id] = pixels
			if not deck_alpha_checked:
				if not _fragment_alpha_control(pixels):
					return false
				deck_alpha_checked = true
		var pixels: Image = loaded[id]
		var cut: Rect2 = layer["source"]
		var destination: Rect2 = layer["destination"]
		var transform: Transform2D = layer["transform"]
		_stamp_deck(deck_footprint, pixels, cut, destination, transform)
	print("Capture: %s: actual deck footprint from %d fragments of %d loaded textures." % [screen, layers.size(), loaded.size()])
	return true

## Reconstructs opaque foreground pixels at native resolution so hidden fangs are not compared to scenery.
func _read_foreground() -> bool:
	foreground_footprint.fill(Color.TRANSPARENT)
	var layers: Array = scene.get_node("World").get_meta("foreground_layers", [])
	var loaded: Dictionary = {}
	var sample: Rect2 = Rect2(0, PLAY.position.y * CANVAS_SCALE, CANVAS.x, TEETH_ROWS * CANVAS_SCALE)
	for layer: Dictionary in layers:
		var view: Transform2D = layer["transform"]
		var transform: Transform2D = Transform2D(0.0, Vector2.ONE * CANVAS_SCALE, 0.0, Vector2.ZERO) * view
		var destination: Rect2 = layer["destination"]
		if not (transform * destination).abs().intersects(sample):
			continue
		var texture: Texture2D = layer["texture"]
		var id: int = texture.get_instance_id()
		if not loaded.has(id):
			var source: Image = texture.get_image()
			source.convert(Image.FORMAT_RGBA8)
			loaded[id] = source
			if not foreground_alpha_checked:
				if not _fragment_alpha_control(source):
					return false
				foreground_alpha_checked = true
		var source: Image = loaded[id]
		var cut: Rect2 = layer["source"]
		var mirrored: bool = layer["mirrored"]
		_stamp_deck(foreground_footprint, source, cut, destination, transform, mirrored)
	return true

func _deck_opaque(mask: Image, at: Vector2) -> bool:
	var point: Vector2i = Vector2i(at.floor())
	return Rect2i(Vector2i.ZERO, mask.get_size()).has_point(point) and mask.get_pixelv(point).a >= 0.99

## A real exposed side has a full contact edge, recessed courses, and a solid interior control.
func _finished_end(frame: Image, view: Transform2D, screen: String, cell: Vector2i, right: bool, interior: Vector2i, covered: Array) -> bool:
	var top: Vector2 = Vector2(MAP_ORIGIN + (cell + Vector2i.DOWN) * CELL)
	var control: Vector2 = Vector2(MAP_ORIGIN + (interior + Vector2i.DOWN) * CELL)
	var edge: float = CELL - 0.5 if right else 0.5
	if not _deck_opaque(deck_footprint, view * (top + Vector2(edge, 2.5))):
		_fail("%s: exposed side %s lost its walked-on edge" % [screen, cell])
		return false
	var pairs: int = 0
	for depth: float in [8.5, 14.5, 20.5, 25.5]:
		var solid: Vector2 = view * (top + Vector2(CELL - 10.5 if right else 10.5, depth))
		var step: Vector2 = Vector2.RIGHT if right else Vector2.LEFT
		for offset: int in range(1, roundi(11.0 * view.x.x)):
			var outside: Vector2 = solid + step * offset
			var inside: Vector2 = outside - step
			if _deck_opaque(deck_footprint, outside):
				continue
			var local: Vector2 = view.affine_inverse() * outside - top
			if local.x < 0.0 or local.x >= CELL:
				break
			var uninterrupted: Vector2 = view * (control + Vector2(local.x, depth))
			if not _deck_opaque(deck_footprint, inside) or not _deck_opaque(deck_footprint, uninterrupted):
				_fail("%s: an end notch also cut the adjacent uninterrupted deck" % screen)
				return false
			if not Rect2(Vector2.ZERO, Vector2(frame.get_size())).has_point(outside) or covered.any(func(area: Rect2) -> bool: return area.has_point(outside) or area.has_point(inside)) or _character_covers(inside):
				break
			if frame.get_pixelv(Vector2i(inside)).is_equal_approx(frame.get_pixelv(Vector2i(outside))):
				_fail("%s: exposed-side stone and its neighbouring void have no visible difference" % screen)
				return false
			pairs += 1
			break
	if pairs < 3:
		_fail("%s: fewer than three visible stone/void pairs on exposed side %s; rectangle finishing or obscured setup" % [screen, cell])
		return false
	finished_ends += 1
	print("Capture: %s: finished %s side of %s retained contact and %d nearest stone/void pairs; adjacent interior %s stayed opaque." % [screen, "right" if right else "left", cell, pairs, interior])
	return true

func _finished_decks(frame: Image, view: Transform2D, screen: String, covered: Array) -> bool:
	var objects: Dictionary = scene.survival_status()["objects"]
	if screen == "4-city-collect-prompt":
		if objects.get("Shallows", "") != "intact":
			_fail("first-city exposed-end setup did not retain Shallows")
			return false
		return _finished_end(frame, view, screen, Vector2i(8, 8), true, Vector2i(7, 8), covered) and _finished_end(frame, view, screen, Vector2i(14, 12), false, Vector2i(15, 12), covered) and _finished_end(frame, view, screen, Vector2i(19, 12), true, Vector2i(18, 12), covered)
	if screen == "11-city-damaged":
		if objects.get("Shallows", "") != "intact" or objects.get("Roof Ladder", "") != "destroyed" or objects.get("Fire Escape", "") != "intact" or objects.get("Drifted Ferry", "") != "intact":
			_fail("damaged-city setup differs from the fixed day-five map states")
			return false
		if not _finished_end(frame, view, screen, Vector2i(14, 12), false, Vector2i(15, 12), covered):
			return false
		destroyed_end = true
	return true

## Whether the walkway's lit top edge leads the scenery right above it by
## GROUND_LEAD, over the LEAD_CELLS the frame shows, when it shows two or more,
## of the pixels clear of the plates covered; the control is that the scenery
## is not black.
func _ground_leads(frame: Image, view: Transform2D, screen: String, covered: Array) -> bool:
	var ground: Vector2 = Vector2.ZERO
	var behind: Vector2 = Vector2.ZERO
	var including_actors: Vector2 = Vector2.ZERO
	var ground_including_actors: Vector2 = Vector2.ZERO
	var shown: int = 0
	for cell: Vector2i in LEAD_CELLS:
		var corner: Vector2 = Vector2(MAP_ORIGIN + cell * CELL)
		if not _shows(view, corner, Vector2(CELL, CELL + EDGE_ROWS)):
			continue
		ground += _clear_luminance(frame, view, corner + Vector2(0, CELL), EDGE_ROWS, covered, true)
		ground_including_actors += _clear_luminance(frame, view, corner + Vector2(0, CELL), EDGE_ROWS, covered)
		behind += _clear_luminance(frame, view, corner, CELL, covered, true)
		including_actors += _clear_luminance(frame, view, corner, CELL, covered)
		shown += 1
	if shown < 2 or ground.y == 0.0 or behind.y == 0.0:
		return true
	ground.x /= ground.y
	behind.x /= behind.y
	ground_frames += 1
	print("Capture: %s: contrast including opaque actors %.6f (ground %.6f, scenery %.6f), excluding them %.6f; sampled %.0f ground and %.0f scenery pixels." % [screen, ground_including_actors.x / ground_including_actors.y - including_actors.x / including_actors.y, ground_including_actors.x / ground_including_actors.y, including_actors.x / including_actors.y, ground.x - behind.x, ground.y, behind.y])
	print("Capture: %s: the ground leads the scenery above it by %.6f over %d cells (ground %.6f, scenery %.6f, at least %.6f)." % [screen, ground.x - behind.x, shown, ground.x, behind.x, GROUND_LEAD])
	if behind.x <= 0.0 or ground.x - behind.x < GROUND_LEAD:
		_fail("%s: the ground does not read over the scenery behind it" % screen)
		return false
	return true

## Whether the frame's values fall in three layers, over the LEAD_CELLS it
## shows, when it shows two or more: the walkways' faces darkest, the backdrop
## in the cells above them brighter by FACE_GAP, and, where the frame shows the
## acid band clear of the sea, the band glowing green over ACID_SHARE of it,
## those pixels brighter than the backdrop by ACID_GAP. A face the band's
## light reaches, within ACID_LIGHT_REACH over its surface, is lit green as the
## mockup lights its posts over the lake, and is not read. Only the pixels
## clear of the rectangles covered are read, and a cell with fewer than
## CLEAR_PIXELS of them in its face or above it is not read. The control is
## that the faces are not black. Only loaded-source alpha inside actual deck
## fragments contributes to a face; new end notches remain scenery.
func _layers(frame: Image, view: Transform2D, screen: String, covered: Array) -> bool:
	var face: float = 0.0
	var behind: float = 0.0
	var shown: int = 0
	for cell: Vector2i in LEAD_CELLS:
		var corner: Vector2 = Vector2(MAP_ORIGIN + cell * CELL)
		if not _shows(view, corner, Vector2(CELL, CELL + FACE_ROWS.y)) or (view * (corner + Vector2(0, CELL + FACE_ROWS.y))).y > _band_top() - ACID_LIGHT_REACH:
			continue
		var cell_face: Vector2 = _clear_luminance(frame, view, corner + Vector2(0, CELL + FACE_ROWS.x), FACE_ROWS.y - FACE_ROWS.x, covered, true, true)
		var cell_behind: Vector2 = _clear_luminance(frame, view, corner, CELL, covered, true)
		if cell_face.y < CLEAR_PIXELS or cell_behind.y < CLEAR_PIXELS:
			continue
		face += cell_face.x / cell_face.y
		behind += cell_behind.x / cell_behind.y
		shown += 1
	if shown < 2:
		return true
	face /= shown
	behind /= shown
	layer_frames += 1
	print("Capture: %s: the walkways' faces read %.6f under the backdrop's %.6f over %d cells (at least %.6f darker)." % [screen, face, behind, shown, FACE_GAP])
	if face <= 0.0 or behind - face < FACE_GAP:
		_fail("%s: the walkways do not read darker than the backdrop" % screen)
		return false
	if not _band_clear(view):
		return true
	var band: Vector2 = _acid_band(frame)
	acid_frames += 1
	print("Capture: %s: the acid band glows green over %.6f of it, at %.6f over the backdrop's %.6f (at least %.6f of it, %.6f brighter)." % [screen, band.x, band.y, behind, ACID_SHARE, ACID_GAP])
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
	print("Capture: %s: the acid band at row %d glows green over %.6f of it (at least %.6f)." % [screen, _band_top(), share, ACID_SHARE])
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
	return _sea_top(view) - SEA_REACH >= _band_top() + BAND_ROWS.y

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

## Actual deck fragments must cover the flat collision floor at the cell's start, middle and end.
func _stairs_lit(_frame_image: Image, view: Transform2D, screen: String) -> bool:
	for cell: Vector2i in STAIR_CELLS + STAIR_DECK_CELLS:
		var top: Vector2 = Vector2(MAP_ORIGIN + (cell + Vector2i.DOWN) * CELL)
		if not _shows(view, top, Vector2(CELL, EDGE_ROWS)):
			return true
		for x: float in [0.5, 16.0, 31.5]:
			if not _deck_opaque(deck_footprint, view * (top + Vector2(x, 0.5))):
				_fail("%s: deck at %s x %.1f misses flat floor_line" % [screen, cell, x])
				return false
	stairs_frames += 1
	parity_frames += 1
	print("Capture: %s: flat stair floor and neighbouring walkway meet at start, middle and end." % screen)
	return true

## Whether, while the camera rests, MOTION_SHARE of the acid's pixels change
## between two frames MOTION_SECONDS apart on the scene's clock: of
## the pixels whose green leads by ACID_GREEN in the first, in the BAND_ROWS
## under the acid band's surface where the sea does not cover them, else in
## the SEA_ROWS under the sea's surface. The control is that the camera did not
## move between the frames and that the rows read are on the play area.
func _acid_moves(screen: String) -> bool:
	var previous_scale: float = Engine.time_scale
	Engine.time_scale = 0.0
	var valid: bool = await _measure_acid_moves(screen)
	await process_frame
	Engine.time_scale = previous_scale
	return valid

func _measure_acid_moves(screen: String) -> bool:
	var view: Transform2D = scene.get_meta("map_view", Transform2D())
	var top: int = _band_top() + BAND_ROWS.x
	var bottom: int = _band_top() + BAND_ROWS.y
	var which: String = "acid band"
	if not _band_clear(view):
		var sea: int = _sea_top(view)
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
	var previous_scale: float = Engine.time_scale
	var previous_rate: int = Engine.max_fps
	Engine.time_scale = 0.0
	Engine.max_fps = 0
	await _frames(2)
	var start: int = Time.get_ticks_usec()
	await _frames(RATE_FRAMES)
	var rate: float = RATE_FRAMES / ((Time.get_ticks_usec() - start) / 1000000.0)
	await process_frame
	Engine.time_scale = previous_scale
	Engine.max_fps = previous_rate
	print("Capture: %s: the city drew %.1f frames a second over %d frames at %s (at least %.0f)." % [screen, rate, RATE_FRAMES, root.get_texture().get_size(), FRAME_RATE])
	if rate < FRAME_RATE and OS.get_cmdline_user_args().has("--allow-software-fps"):
		print("Capture: FPS verdict excepted by --allow-software-fps; measured %.1f, required %.0f." % [rate, FRAME_RATE])
		return true
	if rate < FRAME_RATE:
		_fail("%s: the city draws fewer than %.0f frames a second" % [screen, FRAME_RATE])
		return false
	return true

## Checks both discrete zooms as player two climbs four rows above player one.
func _zoom_steps() -> bool:
	var zooms: Array[float] = []
	var headings: Array[float] = []
	var observe: Callable = func() -> void:
		var view: Transform2D = scene.get_meta("map_view", Transform2D())
		var heading: Transform2D = scene.get_meta("map_heading", Transform2D())
		zooms.append(view.x.x)
		headings.append(heading.x.x)
	process_frame.connect(observe)
	for target: Vector2i in [Vector2i(3, 8), Vector2i(3, 6), Vector2i(1, 6), Vector2i(1, 4)]:
		await _walk_to(target.x, target.y, "appraiser")
	await _frames(PARTED_SETTLE)
	process_frame.disconnect(observe)
	var between: Array[float] = []
	var drawn: Array[float] = []
	for zoom: float in zooms:
		if not drawn.has(zoom):
			drawn.append(zoom)
		if not ZOOMS.any(func(step: float) -> bool: return is_equal_approx(zoom, step)):
			between.append(zoom)
	var status: Dictionary = scene.survival_status()
	var appraiser: Dictionary = status["appraiser"]
	var chef: Dictionary = status["chef"]
	print("Capture: player two climbed the kitchen ladders to %s, player one on %s; camera zooms %s." % [_cell(appraiser), _cell(chef), drawn])
	if not headings.any(func(zoom: float) -> bool: return is_equal_approx(zoom, 1.0)):
		_fail("the camera's zoom cannot be read: it never headed for the scene's own size")
		return false
	if not between.is_empty():
		_fail("the camera draws the scene at zooms between its steps: %s" % [between.slice(0, 5)])
		return false
	if not drawn.any(func(zoom: float) -> bool: return is_equal_approx(zoom, 1.0)):
		_fail("the camera never drew the scene at its own size: %s" % [drawn])
		return false
	return true

## The cell an actor's status dictionary puts it on.
func _cell(actor: Dictionary) -> Vector2i:
	var x: int = actor["x"]
	var y: int = actor["y"]
	return Vector2i(x, y)

## Whether the camera draws the scene at one of its ZOOMS and, at twice the
## scene's size, the teeth, backdrop, acid, walkways and shops each stand in
## pixels one screen pixel wide and tall: of each one's edge pixels clear of
## the word plates and the HUD, its DENSE_SHARE along the rows and along the
## columns differ from both their neighbours. The teeth are read where the
## frame shows them over the top TEETH_ROWS of the play area, the backdrop
## from under those rows down to the acid's surface, and the acid's green
## pixels over the BAND_ROWS under the band's surface where the sea does not
## cover them, else the SEA_ROWS under the sea's surface. A layer is read when
## the frame shows DENSE_EDGES of its edge pixels along each, and the frame
## counts as measured when all five are. Walkways use the faces of
## LEAD_CELLS and shops use LIT_AREAS, excluding opaque characters. The
## actual drawn deck footprint also excludes the new end notches.
func _dense(frame: Image, view: Transform2D, screen: String, plates: Array) -> bool:
	var zoom: float = view.x.x
	if not ZOOMS.any(func(step: float) -> bool: return is_equal_approx(zoom, step)):
		_fail("%s: the camera draws the scene at zoom %.3f, between its steps" % [screen, zoom])
		return false
	if not is_equal_approx(zoom, 2.0):
		return true
	var covered: Array = plates + scene.get_meta("hud_panels", []) + scene.get_meta("hud_notes", [])
	var acid_rows: Vector2i = Vector2i(_band_top() + BAND_ROWS.x, _band_top() + BAND_ROWS.y)
	if not _band_clear(view):
		var sea: int = _sea_top(view)
		acid_rows = Vector2i(sea + SEA_ROWS.x, sea + SEA_ROWS.y)
	var top: int = int(PLAY.position.y)
	acid_rows = Vector2i(clampi(acid_rows.x, top + TEETH_ROWS, int(PLAY.end.y) - 1), clampi(acid_rows.y, top + TEETH_ROWS, int(PLAY.end.y) - 1))
	var walkways: Array[Rect2] = []
	for cell: Vector2i in LEAD_CELLS:
		var corner: Vector2 = Vector2(MAP_ORIGIN + cell * CELL)
		walkways.append(view * Rect2(corner + Vector2(0, CELL + FACE_ROWS.x), Vector2(CELL, FACE_ROWS.y - FACE_ROWS.x)))
	var shops: Array[Rect2] = []
	for area: Rect2 in LIT_AREAS:
		shops.append(view * area)
	var layers: Dictionary = {
		"teeth": _thin(frame, Vector2i(top, top + TEETH_ROWS), covered, "teeth"),
		"backdrop": _thin(frame, Vector2i(top + TEETH_ROWS, acid_rows.x), covered, ""),
		"acid": _thin(frame, acid_rows, covered, "acid"),
		"walkways": _thin(frame, Vector2i(top, int(PLAY.end.y)), covered, "walkways", walkways),
		"shops": _thin(frame, Vector2i(top, int(PLAY.end.y)), covered, "city", shops),
	}
	print("Capture: %s: the share of edge pixels one screen pixel wide or tall, and the edge pixels read, are %s (at least %s, read over %d)." % [screen, layers, DENSE_SHARE, DENSE_EDGES])
	var read: int = 0
	for layer: String in layers:
		var found: Vector2 = layers[layer]
		if found.y < DENSE_EDGES:
			continue
		read += 1
		if dense_city_frames.has(layer):
			dense_city_frames[layer] += 1
		if found.x < DENSE_SHARE[layer]:
			_fail("%s: the %s is drawn in pixels larger than a screen pixel" % [screen, layer])
			return false
	if read == layers.size():
		dense_frames += 1
	return true

## Whether a sampled screen pixel is painted by a character, retaining the
## transparent gaps inside its bounding rectangle as scenery samples.
func _character_covers(point: Vector2) -> bool:
	for layer: Dictionary in character_masks:
		var mask: Image = layer["alpha"]
		if _character_opaque(mask, layer, point):
			return true
	return false

## An opaque foreground pixel is excluded while its nearest transparent
## neighbour inside the same bounds remains in the scenery measurement.
func _character_mask_control() -> bool:
	var frame: Image = Image.create(CELL, CELL, false, Image.FORMAT_RGBA8)
	frame.fill(Color(0.2, 0.2, 0.2))
	frame.set_pixel(16, 16, Color.WHITE)
	frame.set_pixel(16, 17, Color.GREEN)
	var mask: Image = Image.create(CELL, CELL, false, Image.FORMAT_RGBA8)
	mask.fill(Color.TRANSPARENT)
	mask.set_pixel(16, 16, Color.WHITE)
	character_masks.assign([{ "area": Rect2(0, 0, CELL, CELL), "alpha": mask, "mirrored": false }])
	var all_pixels: Vector2 = _clear_luminance(frame, Transform2D(), Vector2.ZERO, CELL, [])
	var scenery: Vector2 = _clear_luminance(frame, Transform2D(), Vector2.ZERO, CELL, [], true)
	var valid: bool = _character_covers(Vector2(16.5, 16.5)) and not _character_covers(Vector2(16.5, 17.5)) and all_pixels.y == CELL * CELL and scenery.y == CELL * CELL - 1 and is_equal_approx(scenery.x, all_pixels.x - 1.0)
	character_masks.clear()
	if not valid:
		_fail("character alpha control excluded a transparent gap or retained an opaque pixel")
		return false
	print("Capture: character alpha control excluded one opaque pixel, retained its transparent neighbour and the other 1023 scenery pixels.")
	return true


## Checks the actual drawn texture's silhouette at the close camera, then reads
## the rendered frame only inside its opaque interior, excluding plates and HUD.
## Background and item detail cannot contribute to this character measurement.
func _characters_dense(frame: Image, view: Transform2D, screen: String, covered: Array) -> bool:
	if not is_equal_approx(view.x.x, 2.0):
		return true
	frame = native_frame
	var layers: Array = scene.get_node("World").get_meta("character_layers", [])
	for index: int in range(layers.size()):
		var layer: Dictionary = layers[index]
		var role: String = layer["role"]
		var layout_area: Rect2 = layer["area"]
		var area: Rect2 = Rect2(layout_area.position * CANVAS_SCALE, layout_area.size * CANVAS_SCALE)
		if not PLAY.encloses(layout_area):
			continue
		var texture: Texture2D = layer["texture"]
		var mask: Image = texture.get_image()
		print("Capture: %s: %s loaded texture %s, actual draw area %s." % [screen, role, mask.get_size(), area])
		var height: float = mask.get_used_rect().size.y * area.size.y / mask.get_height()
		var target: float = mask.get_used_rect().size.y
		if not area.size.is_equal_approx(Vector2(mask.get_size())) or absf(height - target) > 0.01:
			_fail("%s: %s silhouette is %.1f screen pixels tall, native pose %.0f" % [screen, role, height, target])
			return false
		var edges: Vector2i = Vector2i.ZERO
		var thin: Vector2i = Vector2i.ZERO
		for y: int in range(maxi(1, int(area.position.y)), mini(frame.get_height() - 1, int(ceilf(area.end.y)))):
			for x: int in range(maxi(1, int(area.position.x)), mini(frame.get_width() - 1, int(ceilf(area.end.x)))):
				var point: Vector2 = Vector2(x + 0.5, y + 0.5)
				if covered.any(func(box: Rect2) -> bool: return box.grow(1).has_point(point / CANVAS_SCALE)):
					continue
				var hidden: bool = false
				for later: int in range(index + 1, layers.size()):
					var later_layer: Dictionary = layers[later]
					var later_area: Rect2 = later_layer["area"]
					if later_area.has_point(point / CANVAS_SCALE):
						hidden = true
				if hidden or not _character_opaque(mask, layer, point / CANVAS_SCALE) or not _character_opaque(mask, layer, (point + Vector2.LEFT) / CANVAS_SCALE) or not _character_opaque(mask, layer, (point + Vector2.RIGHT) / CANVAS_SCALE) or not _character_opaque(mask, layer, (point + Vector2.UP) / CANVAS_SCALE) or not _character_opaque(mask, layer, (point + Vector2.DOWN) / CANVAS_SCALE):
					continue
				var pixel: Color = frame.get_pixel(x, y)
				var left: bool = _differ(pixel, frame.get_pixel(x - 1, y))
				var right: bool = _differ(pixel, frame.get_pixel(x + 1, y))
				var up: bool = _differ(pixel, frame.get_pixel(x, y - 1))
				var down: bool = _differ(pixel, frame.get_pixel(x, y + 1))
				edges += Vector2i(1 if left or right else 0, 1 if up or down else 0)
				thin += Vector2i(1 if left and right else 0, 1 if up and down else 0)
		if mini(edges.x, edges.y) < 64:
			continue
		var share: float = minf(float(thin.x) / edges.x, float(thin.y) / edges.y)
		print("Capture: %s: %s height %.0f, character-only thin share %.3f over %s edges." % [screen, role, height, share, edges])
		if share < 0.1:
			_fail("%s: %s character pixels are drawn in blocks" % [screen, role])
			return false
		character_frames[role] += 1
	return true

## Samples alpha from the texture the draw used, through that draw's area and reflection.
func _character_opaque(mask: Image, layer: Dictionary, point: Vector2) -> bool:
	var area: Rect2 = layer["area"]
	var at: Vector2i = Vector2i(((point - area.position) / area.size * Vector2(mask.get_size())).floor())
	if not Rect2i(Vector2i.ZERO, mask.get_size()).has_point(at):
		return false
	if layer["mirrored"]:
		at.x = mask.get_width() - 1 - at.x
	return mask.get_pixelv(at).a > 0.99


## Over the screen rows from rows.x to rows.y, every other row and column,
## each pixel clear of the rectangles covered, inside an area for city
## layers and clear of opaque characters there, and, for the teeth, only where
## the frame shows the throat's fangs, within TEETH_TOLERANCE in
## each channel, and for the acid, only where its green leads by ACID_GREEN:
## of the pixels that differ from a neighbour along the row, the share that
## differ from both, and the same along the column; the smaller of the two
## shares, as x, and the fewer of the two counts of such pixels, as y.
func _thin(frame: Image, rows: Vector2i, covered: Array, only: String, areas: Array[Rect2] = []) -> Vector2:
	var edges: Vector2i = Vector2i.ZERO
	var thin: Vector2i = Vector2i.ZERO
	frame = native_frame
	var width: int = frame.get_width()
	for y: int in range(maxi(roundi(rows.x * CANVAS_SCALE), 1), mini(roundi(rows.y * CANVAS_SCALE), frame.get_height() - 1), 2):
		for x: int in range(1, width - 1, 2):
			if covered.any(func(area: Rect2) -> bool: return area.has_point(Vector2(x, y) / CANVAS_SCALE)):
				continue
			if only in ["city", "walkways"] and (_character_covers(Vector2(x + 0.5, y + 0.5) / CANVAS_SCALE) or not areas.any(func(area: Rect2) -> bool: return area.has_point(Vector2(x, y) / CANVAS_SCALE))):
				continue
			if only == "walkways" and not [Vector2.ZERO, Vector2.LEFT, Vector2.RIGHT, Vector2.UP, Vector2.DOWN].all(func(offset: Vector2) -> bool: return _deck_opaque(deck_footprint, Vector2(x + 0.5, y + 0.5) / CANVAS_SCALE + offset / CANVAS_SCALE)):
				continue
			var pixel: Color = frame.get_pixel(x, y)
			if only == "acid" and _green(pixel) < ACID_GREEN:
				continue
			if only == "teeth":
				var tooth: Color = _fang(x, y)
				if tooth.a < 0.5 or maxf(absf(pixel.r - tooth.r), maxf(absf(pixel.g - tooth.g), absf(pixel.b - tooth.b))) > TEETH_TOLERANCE:
					continue
			var left: bool = _differ(pixel, frame.get_pixel(x - 1, y))
			var right: bool = _differ(pixel, frame.get_pixel(x + 1, y))
			var up: bool = _differ(pixel, frame.get_pixel(x, y - 1))
			var down: bool = _differ(pixel, frame.get_pixel(x, y + 1))
			if left or right:
				edges.x += 1
				thin.x += 1 if left and right else 0
			if up or down:
				edges.y += 1
				thin.y += 1 if up and down else 0
	if edges.x == 0 or edges.y == 0:
		return Vector2(0.0, 0.0)
	return Vector2(minf(float(thin.x) / edges.x, float(thin.y) / edges.y), mini(edges.x, edges.y))

## Whether two pixels differ by more than DENSE_TOLERANCE in a channel.
func _differ(one: Color, other: Color) -> bool:
	return maxf(absf(one.r - other.r), maxf(absf(one.g - other.g), absf(one.b - other.b))) > DENSE_TOLERANCE

## Whether, over the columns where the acid shows under its surface, the band's
## where the sea does not cover it or stands below it and the sea's otherwise,
## the LIT_ROWS above that surface lead the UNLIT_ROWS of backdrop higher up in
## green by LIGHT_GAP, when the play area shows both. Under the band's surface
## the rows above the sea are the band's acid, not backdrop. The control is that
## the acid shows under the surface in LIGHT_COLUMNS of the columns read.
func _acid_lights(frame: Image, view: Transform2D, screen: String) -> bool:
	var sea: int = _sea_top(view)
	var surface: int = _band_top() if _band_clear(view) or _band_top() < sea else sea
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
## column, TEETH_SHARE of the pixels the throat's fangs cover where the World
## layer's `throat_at` meta puts them read within TEETH_TOLERANCE of the
## picture's colour in each channel, so the fangs hang from the top of the
## screen. The control is that the fangs cover at least TEETH_COVER of the
## pixels read, so the rows are the fangs'. Native colour comparisons exclude
## observed overlays, actual opaque foreground fragments and actors; source coverage includes hidden fangs.
func _teeth_show(_frame_image: Image, screen: String) -> bool:
	if not _read_foreground():
		return false
	var occluders: Array = scene.get_meta("hud_panels", []) + scene.get_meta("hud_notes", []) + scene.get_meta("map_plates", [])
	var reading: Vector2 = _teeth_sample(native_frame, occluders)
	teeth_frames += 1
	print("Capture: %s: %.6f of the %.6f of the top %d rows of the play area the fangs cover show them (at least %.6f of at least %.6f)." % [screen, reading.x, reading.y, TEETH_ROWS, TEETH_SHARE, TEETH_COVER])
	if not _teeth_visible(reading):
		_fail("%s: the fangs do not hang from the top of the play area" % screen)
		return false
	if not teeth_control_checked:
		var broken: Image = native_frame.duplicate() as Image
		for y: int in range(roundi(PLAY.position.y * CANVAS_SCALE), roundi((PLAY.position.y + TEETH_ROWS) * CANVAS_SCALE)):
			for x: int in range(broken.get_width()):
				if _fang(x, y).a >= 0.5:
					broken.set_pixel(x, y, Color(0.03, 0.04, 0.07))
		var absent: Vector2 = _teeth_sample(broken, occluders)
		if _teeth_visible(absent) or not is_equal_approx(reading.y, absent.y):
			_fail("the same fang verdict accepted removed fang colours or changed its sampled subject")
			return false
		print("Capture: fang control rejected removed fang colours: share %.6f, unchanged source coverage %.6f; original share %.6f passed." % [absent.x, absent.y, reading.x])
		var displaced: Image = native_frame.duplicate() as Image
		var band_end: int = roundi((PLAY.position.y + TEETH_ROWS) * CANVAS_SCALE)
		for y: int in range(band_end):
			for x: int in range(displaced.get_width()):
				if _fang(x, y).a >= 0.5:
					displaced.set_pixel(x, y, Color(0.03, 0.04, 0.07))
					displaced.set_pixel(x, y + band_end, native_frame.get_pixel(x, y))
		var lowered: Vector2 = _teeth_sample(displaced, occluders)
		if _teeth_visible(lowered):
			_fail("the fang verdict accepted fangs displaced below the top band")
			return false
		print("Capture: fang placement control rejected fangs displaced below the top band: share %.6f, unchanged source coverage %.6f." % [lowered.x, lowered.y])
		teeth_control_checked = true
	return true

func _teeth_visible(reading: Vector2) -> bool:
	return reading.y >= TEETH_COVER and reading.x >= TEETH_SHARE

func _teeth_sample(frame: Image, occluders: Array) -> Vector2:
	var candidates: int = 0
	var covered: int = 0
	var matched: int = 0
	var count: int = 0
	var errors: Array[float] = []
	for y: int in range(roundi(PLAY.position.y * CANVAS_SCALE), roundi((PLAY.position.y + TEETH_ROWS) * CANVAS_SCALE)):
		for x: int in range(0, frame.get_width(), 2):
			count += 1
			var tooth: Color = _fang(x, y)
			if tooth.a < 0.5:
				continue
			candidates += 1
			var layout_point: Vector2 = Vector2(x + 0.5, y + 0.5) / CANVAS_SCALE
			if occluders.any(func(area: Rect2) -> bool: return area.has_point(layout_point)) or _deck_opaque(foreground_footprint, Vector2(x + 0.5, y + 0.5)) or _character_covers(layout_point):
				continue
			covered += 1
			var pixel: Color = frame.get_pixel(x, y)
			var error: float = maxf(absf(pixel.r - tooth.r), maxf(absf(pixel.g - tooth.g), absf(pixel.b - tooth.b)))
			if OS.get_cmdline_user_args().has("--fang-diagnostic"):
				errors.append(error)
			if error <= TEETH_TOLERANCE:
				matched += 1
	if not errors.is_empty():
		errors.sort()
		var quantiles: Dictionary = {}
		for fraction: float in [0.313, 0.5, 0.75, 0.9]:
			quantiles[str(fraction)] = errors[mini(errors.size() - 1, ceili(errors.size() * fraction) - 1)]
		print("Fang diagnostic: pinned clock %.6f, throat_at %s, visible samples %d, colour-error quantiles %s." % [clock, scene.get_node("World").get_meta("throat_at", Vector2.ZERO), errors.size(), quantiles])
	return Vector2(float(matched) / maxi(covered, 1), float(candidates) / count)

## The throat picture's pixel the World layer drew at screen pixel (x, y), by
## its `throat_at` meta, beside mirrored copies of itself, where it is a
## fang's cream; clear elsewhere.
func _fang(x: int, y: int) -> Color:
	var at: Vector2 = scene.get_node("World").get_meta("throat_at", Vector2(-INF, -INF))
	at = at.floor() * CANVAS_SCALE
	var row: int = floori(y + 0.5 - at.y)
	if row < 0 or row >= throat.get_height():
		return Color(0, 0, 0, 0)
	var width: int = throat.get_width()
	var offset: int = floori(x + 0.5 - at.x)
	var copy: int = floori(float(offset) / width)
	var column: int = offset - copy * width
	if copy != 0:
		column = width - 1 - column
	var pixel: Color = throat.get_pixel(column, row)
	if pixel.r >= FANG_RED and pixel.g >= FANG_GREEN and pixel.g >= pixel.b:
		return pixel
	return Color(0, 0, 0, 0)


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
	print("Capture: %s: %.6f of the lit structures' %d scene pixels read warm and bright (at least %.6f)." % [screen, share, count, LIT_SHARE])
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
	var image: Image = root.get_texture().get_image()
	image.resize(SCREEN.x, SCREEN.y, Image.INTERPOLATE_NEAREST)
	return image

## The near sea cannot stand above the far shore in screen perspective.
func _sea_top(view: Transform2D) -> int:
	return maxi(floori((view * Vector2(0, _acid_surface())).y), _band_top())

## The scene row of the acid's surface in the frame the status describes.
func _acid_surface() -> float:
	var status: Dictionary = scene.survival_status()
	var acid: int = status["acid"]
	var full: int = status["full_acid"]
	return ACID_LOW - clampf(float(acid) / full, 0.0, 1.0) * (ACID_LOW - MAP_ORIGIN.y)

## The summed luminance, as x, and the count, as y, of the screen pixels of a
## cell-wide band of scene rows whose top-left is corner that lie clear of the
## rectangles covered, optionally excluding opaque characters or retaining only
## the actually drawn deck footprint.
func _clear_luminance(frame: Image, view: Transform2D, corner: Vector2, rows: int, covered: Array, exclude_characters: bool = false, only_deck: bool = false) -> Vector2:
	var found: Vector2 = Vector2.ZERO
	for y: int in range(rows):
		for x: int in range(CELL):
			var at: Vector2 = view * (corner + Vector2(x + 0.5, y + 0.5))
			if only_deck and not _deck_opaque(deck_footprint, at.floor() + Vector2(0.5, 0.5)):
				continue
			if exclude_characters and _character_covers(at.floor() + Vector2(0.5, 0.5)):
				continue
			if covered.any(func(area: Rect2) -> bool: return area.has_point(at)):
				continue
			found += Vector2(_pixel(frame, view, corner + Vector2(x, y)), 1.0)
	return found

## The luminance of the screen pixel at the middle of the scene pixel at point.
func _pixel(frame: Image, view: Transform2D, point: Vector2) -> float:
	var at: Vector2 = view * (point + Vector2(0.5, 0.5))
	return frame.get_pixelv(Vector2i(at.floor())).get_luminance()

func _fail(message: String) -> void:
	push_error(message)
	if OS.get_cmdline_user_args().has("--measure-all"):
		diagnostic_findings.append(message)
		print("Capture: diagnostic finding: ", message)
		return
	_finish("fail: " + message)
	quit(1)

## Prints the run's result line once: a failure can be followed by another
## before quit() takes effect.
func _finish(result: String) -> void:
	if not finished:
		finished = true
		if not diagnostic_findings.is_empty():
			result = "fail: %d diagnostic findings: %s" % [diagnostic_findings.size(), diagnostic_findings]
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
