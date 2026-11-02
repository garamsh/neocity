extends Node2D
## A stand-in cinematic that follows the scene contract and reports its end
## before the host has finished adding it.

signal finished

@export var in_enter_tree: bool = false

func _enter_tree() -> void:
	if in_enter_tree:
		finished.emit()

func _ready() -> void:
	if not in_enter_tree:
		finished.emit()
