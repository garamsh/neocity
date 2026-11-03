#pragma once

#include "camera.hpp"
#include "drawing.hpp"

#include <survival/companion.hpp>
#include <survival/content.hpp>

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <span>
#include <vector>

namespace neocity::rendering {

/// The canvas height of the acid's surface at `acid_ratio`, from below the
/// map's lowest ground row at 0 to the top of the map at 1.
[[nodiscard]] float acid_surface(float acid_ratio);
/// Where a point of the city, in sub-cells, falls on the canvas.
[[nodiscard]] godot::Vector2 map_point(survival::Position position);
/// Draws the backdrop behind every survival screen as `view` shows the
/// scene, untinted, in layers that move less with the camera the farther back
/// they are: the monster's throat, the swallowed city, and an acid band of
/// sunken trains and broken pillars pinned to the bottom of the play area,
/// which rises with `acid_ratio`. Then the acid sea in the scene, whose
/// surface rises with `acid_ratio`, from below the map's lowest ground row at
/// 0 to the top of the map at 1, and sinks dark below it. Leaves the canvas
/// drawing the scene as `view` shows it.
void paint_mouth(godot::Node2D &canvas, const Art &art, float acid_ratio,
                 const View &view);
/// Draws the light over both acid surfaces `paint_mouth` draws, for a canvas
/// that blends by adding.
void paint_glow(godot::Node2D &canvas, const Art &art, float acid_ratio,
                const View &view);
/// Draws the light of the city's lamps and acid drips as `view` shows them,
/// for a canvas that blends by adding.
void paint_city_glow(godot::Node2D &canvas, const Art &art,
                     const survival::Content &content, const View &view);
/// Draws the city grid with its ground, scenery, objects, stock and actors,
/// in the scene's own pixels. Destroyed, depleted and unavailable objects
/// each have a mark; a dormant object, and a bag emptied or carried off, is
/// not drawn, and a repaired one looks intact. A source in `out_of_reach`
/// has stock no player can walk to: it loses the gold frame. Each actor faces
/// the way it last walked, alternates its walking frames while it moves, and
/// climbs on a ladder.
void paint_city(godot::Node2D &canvas, const Art &art,
                const survival::Content &content,
                const survival::GameView &view,
                const std::vector<survival::ObjectId> &out_of_reach);
/// Draws the city's words on the screen where `camera` shows what they name,
/// each on a dark plate over the art so it reads at any acid level, none
/// covering another or a source, and none for what is off the play area. A
/// source in `out_of_reach` is named as having no way there. Each of `goals`
/// whose area is off the play area gets a pointer at the area's edge, aimed
/// at it, with its name. Leaves on the canvas the plates drawn as the
/// `map_plates` meta, an array of Rect2; the names of the sources the city
/// draws as `map_sources`, and of those whose count is on the play area as
/// `map_labelled`; the names pointed at as `map_pointers`; and the camera as
/// `map_view`, a Transform2D from the scene's pixels to the screen's.
void paint_city_labels(godot::Node2D &canvas, const Art &art,
                       const survival::Content &content,
                       const survival::GameView &view,
                       survival::ControlMap control,
                       const std::vector<survival::ObjectId> &out_of_reach,
                       std::span<const survival::ObjectId> goals,
                       const View &camera);
/// Draws the base kitchen with the chef and the appraiser, and the monster's
/// feeding bucket, steaming once `dish_ready`.
void paint_workshop(godot::Node2D &canvas, const Art &art, bool dish_ready);

} // namespace neocity::rendering
