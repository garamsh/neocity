#pragma once

#include "../rendering/drawing.hpp"
#include "pointer.hpp"

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/string.hpp>

#include <vector>

namespace neocity::hud {

/// One display setting on the pause screen or the settings page: its name, the
/// choice it shows now, and whether the left and right keys change it.
struct DisplayRow {
  DisplaySetting setting = DisplaySetting::Resolution;
  godot::String name;
  godot::String value;
  bool marked = false;
};

/// Draws `rows` one under another from `left`, the first row's baseline at
/// `top`, `pitch` apart: each a name, a back arrow, its value and a forward
/// arrow, the marked row in the accent.
void paint_display_rows(godot::Node2D &canvas, const rendering::Art &drawing,
                        const std::vector<DisplayRow> &rows, float left,
                        float top, float pitch);
/// The arrows of `rows` laid out as `paint_display_rows` draws them, each
/// pressing its setting one place back or on.
[[nodiscard]] std::vector<Target>
display_row_targets(const std::vector<DisplayRow> &rows, float left, float top,
                    float pitch);

} // namespace neocity::hud
