#pragma once

#include <pan_dodge/engine.hpp>
#include <pan_tilt/engine.hpp>

#include <godot_cpp/classes/node2d.hpp>

namespace neocity::rendering {

/// Draws direct-movement pan geometry without applying game rules.
void paint_pan(godot::Node2D &canvas, const pan_dodge::Snapshot &world);
/// Draws the tilt pan, moving bodies, and gravity indicator.
void paint_pan(godot::Node2D &canvas, const pan_tilt::Snapshot &world);

} // namespace neocity::rendering
