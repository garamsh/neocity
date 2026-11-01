extends Node2D
## A stand-in cinematic that follows the scene contract: it runs briefly, then
## reports its end.

signal finished

const FRAMES: int = 45

var frame: int = 0

func _process(_delta: float) -> void:
	frame += 1
	if frame == FRAMES:
		finished.emit()
