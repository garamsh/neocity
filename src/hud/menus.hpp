#pragma once

#include "../experiment/catalog.hpp"
#include "../rendering/drawing.hpp"

#include <span>

namespace neocity::hud {

/// Draws the start screen: the survival game, the developer page, and the
/// ascent prototype.
void paint_main_menu(godot::Node2D &canvas,
                     const rendering::AscentDrawing &drawing);
/// Draws the developer page listing every registered experiment.
void paint_developer_menu(godot::Node2D &canvas,
                          const rendering::AscentDrawing &drawing,
                          std::span<const experiment::Entry> experiments);

} // namespace neocity::hud
