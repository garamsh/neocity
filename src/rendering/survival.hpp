#pragma once

#include "drawing.hpp"

#include <survival/companion.hpp>
#include <survival/content.hpp>

#include <godot_cpp/classes/node2d.hpp>

#include <vector>

namespace neocity::rendering {

/// Draws the monster's mouth behind every survival screen; the acid rises with
/// `acid_ratio`, from 0 to 1.
void paint_mouth(godot::Node2D &canvas, float acid_ratio);
/// Draws the city grid with its objects, stock and actors. Destroyed, depleted
/// and unavailable objects each have a mark; a dormant object is not drawn, and
/// a repaired one looks intact. A source in `out_of_reach` has stock no player
/// can walk to: it loses the gold frame and is marked and named as such. Every
/// word sits on a dark plate over the actors, so it reads at any acid level.
void paint_city(godot::Node2D &canvas, const AscentDrawing &drawing,
                const survival::Content &content,
                const survival::GameView &view, survival::ControlMap control,
                const std::vector<survival::ObjectId> &out_of_reach);
/// Draws the base with the chef and the appraiser at their stations, and the
/// monster's feeding bowl, steaming once `dish_ready`.
void paint_workshop(godot::Node2D &canvas, const AscentDrawing &drawing,
                    bool dish_ready);

} // namespace neocity::rendering
