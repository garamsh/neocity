#pragma once

#include "drawing.hpp"

#include <survival/companion.hpp>
#include <survival/content.hpp>

#include <godot_cpp/classes/node2d.hpp>

#include <vector>

namespace neocity::rendering {

/// Draws the monster's throat, the distant city, and the acid sea behind every
/// survival screen. The surface rises with `acid_ratio`, from below the map's
/// lowest ground row at 0 to the top of the map at 1.
void paint_mouth(godot::Node2D &canvas, const Art &art, float acid_ratio);
/// Draws the city grid with its ground, scenery, objects, stock and actors.
/// Destroyed, depleted and unavailable objects each have a mark; a dormant
/// object is not drawn, and a repaired one looks intact. A source in
/// `out_of_reach` has stock no player can walk to: it loses the gold frame and
/// is marked and named as such. Every word sits on a dark plate over the
/// actors, so it reads at any acid level. Each actor faces the way it last
/// walked, alternates its walking frames while it moves, and climbs on a
/// ladder.
void paint_city(godot::Node2D &canvas, const Art &art,
                const survival::Content &content,
                const survival::GameView &view, survival::ControlMap control,
                const std::vector<survival::ObjectId> &out_of_reach);
/// Draws the base kitchen with the chef and the appraiser, and the monster's
/// feeding bucket, steaming once `dish_ready`.
void paint_workshop(godot::Node2D &canvas, const Art &art, bool dish_ready);

} // namespace neocity::rendering
