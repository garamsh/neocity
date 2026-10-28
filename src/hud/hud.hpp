#pragma once

#include "../rendering/drawing.hpp"

#include <gameplay/world_snapshot.hpp>

#include <godot_cpp/classes/node2d.hpp>

namespace neocity::hud {

/// Paints the interface and the pause/completion overlays over the world.
void paint(godot::Node2D &canvas, const rendering::AscentDrawing &drawing,
           const gameplay::WorldSnapshot &world);

} // namespace neocity::hud
