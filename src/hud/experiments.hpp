#pragma once

#include "../rendering/drawing.hpp"

namespace neocity::hud {

/// Draws the common selection screen and available keyboard shortcuts.
void paint_menu(godot::Node2D &canvas, const rendering::AscentDrawing &drawing);
/// Draws instructions and the active pan's observable outcome.
void paint_experiment(godot::Node2D &canvas,
                      const rendering::AscentDrawing &drawing, bool tilt,
                      const godot::String &outcome);

} // namespace neocity::hud
