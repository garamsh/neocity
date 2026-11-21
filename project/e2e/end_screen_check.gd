class_name EndScreenCheck
extends RefCounted
## What every end screen must show: its picture at the screen's width with its
## aspect kept, a title, a description and a Main menu button in the band under
## it, and nothing else: no HUD, no skip, no development text.

const ART_DIR: String = "res://levels/monster_mouth_escape/art_hd/endings/"
const SCREEN_WIDTH: float = 1280.0
const BAND_Y: float = 534.0
## Words that mean the card is unfinished, whatever their case.
const DEVELOPMENT_WORDS: PackedStringArray = ["placeholder", "no cinematic", "todo", "debug", "lorem"]

## An empty string when the last frame drew the end screen for picture `art`
## with `title` and a description starting `description`; otherwise what is wrong.
static func problem(scene: MainScene, art: String, title: String, description: String) -> String:
	var path: String = ART_DIR + art + ".png"
	var texture: Texture2D = scene.get_meta("ending_art", null)
	if texture == null or texture.resource_path != path or scene.get_meta("ending_image", "") != path:
		return "the picture %s is missing" % path
	var area: Rect2 = scene.get_meta("ending_picture", Rect2())
	var wide: float = SCREEN_WIDTH * float(texture.get_height()) / float(texture.get_width())
	if area.position != Vector2.ZERO or not is_equal_approx(area.size.x, SCREEN_WIDTH) or absf(area.size.y - wide) > 0.5:
		return "the picture is %s, not the screen's width from the top at its aspect" % [area]
	var lines: PackedStringArray = scene.get_meta("screen_text", PackedStringArray())
	for line: String in lines:
		for word: String in DEVELOPMENT_WORDS:
			if line.to_lower().contains(word):
				return "development text shows: %s" % line
	if lines.size() != 3 or lines[0] != title or not lines[1].begins_with(description) or lines[2] != "Main menu":
		return "the screen's words are %s, not the title %s, a description starting %s and Main menu" % [lines, title, description]
	var targets: Array = scene.get_meta("pointer_targets", [])
	if targets.size() != 1 or targets[0]["press"] != "menu":
		return "the screen's targets are not the Main menu alone: %s" % [targets]
	var button: Rect2 = targets[0]["rect"]
	if button.position.y < BAND_Y or button.end.x > SCREEN_WIDTH or button.position.x < SCREEN_WIDTH / 2.0:
		return "the Main menu button is not at the band's right: %s" % [button]
	return ""
