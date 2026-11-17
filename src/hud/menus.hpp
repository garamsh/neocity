#pragma once

#include "../rendering/drawing.hpp"
#include "display_rows.hpp"
#include "pointer.hpp"
#include "title.hpp"

#include <godot_cpp/classes/node2d.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace neocity::hud {

/// Draws every key of both players and the way back to the title.
void paint_controls(godot::Node2D &canvas, const rendering::Art &drawing);
/// Draws the How to play page: what the game asks, each first-play tip's
/// line, and the way back to the title.
void paint_how_to_play(godot::Node2D &canvas, const rendering::Art &drawing);
/// Draws the settings page: the display `rows` and the way back to the title.
void paint_settings(godot::Node2D &canvas, const rendering::Art &drawing,
                    const std::vector<DisplayRow> &rows);
/// The settings page's arrows and its way back to the title.
[[nodiscard]] std::vector<Target>
settings_targets(const rendering::Art &drawing,
                 const std::vector<DisplayRow> &rows);
/// The way back to the title from the controls page or the How to play page,
/// which share its place.
[[nodiscard]] std::vector<Target>
controls_targets(const rendering::Art &drawing);

} // namespace neocity::hud
