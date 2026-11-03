#pragma once

#include "drawing.hpp"

#include <survival/companion.hpp>
#include <survival/content.hpp>

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <vector>

namespace neocity::rendering {

/// The canvas height of the acid's surface at `acid_ratio`, from below the
/// map's lowest ground row at 0 to the top of the map at 1.
[[nodiscard]] float acid_surface(float acid_ratio);
/// Where a point of the city, in sub-cells, falls on the canvas.
[[nodiscard]] godot::Vector2 map_point(survival::Position position);
/// Draws the monster's throat, the distant city, and the acid sea behind every
/// survival screen, darker and cooler than the map. The surface rises with
/// `acid_ratio`, from below the map's lowest ground row at 0 to the top of the
/// map at 1, and the acid below it sinks dark.
void paint_mouth(godot::Node2D &canvas, const Art &art, float acid_ratio);
/// Draws the city grid with its ground, scenery, objects, stock and actors.
/// Destroyed, depleted and unavailable objects each have a mark; a dormant
/// object, and a bag emptied or carried off, is not drawn, and a repaired one
/// looks intact. The sources drawn are left as the `map_sources` meta, an
/// array of their names. A source in
/// `out_of_reach` has stock no player can walk to: it loses the gold frame and
/// is marked and named as such. Every word sits on a dark plate over the art,
/// so it reads at any acid level, and no plate covers another or a source; the
/// plates drawn are left as the canvas's `map_plates` meta, an array of Rect2.
/// Each actor faces the way it last walked, alternates its walking frames
/// while it moves, and climbs on a ladder.
void paint_city(godot::Node2D &canvas, const Art &art,
                const survival::Content &content,
                const survival::GameView &view, survival::ControlMap control,
                const std::vector<survival::ObjectId> &out_of_reach);
/// Draws the base kitchen with the chef and the appraiser, and the monster's
/// feeding bucket, steaming once `dish_ready`.
void paint_workshop(godot::Node2D &canvas, const Art &art, bool dish_ready);

} // namespace neocity::rendering
