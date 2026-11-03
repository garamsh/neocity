#pragma once

#include "camera.hpp"
#include "drawing.hpp"

#include <survival/companion.hpp>
#include <survival/content.hpp>

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <vector>

namespace neocity::rendering {

/// What one frame of the city shows, worked out once for every layer that
/// draws it: the camera as it eases, where it heads, the stocked sources no
/// human can walk to, and each human's goal.
struct Shot {
  View camera;
  View heading;
  std::vector<survival::ObjectId> out_of_reach;
  std::vector<survival::ObjectId> goals;
};

/// The canvas height of the acid's surface at `acid_ratio`, from below the
/// map's lowest ground row at 0 to the top of the map at 1.
[[nodiscard]] float acid_surface(float acid_ratio);
/// Where a point of the city, in sub-cells, falls on the canvas.
[[nodiscard]] godot::Vector2 map_point(survival::Position position);
/// Draws the backdrop behind every survival screen as `camera` shows the
/// scene, in layers that move less with the camera the farther back they
/// are: the monster's throat, breathing on `art`'s clock with saliva dripping
/// from its fangs, the swallowed city, and an acid band of sunken trains and
/// broken pillars glowing along the bottom of the play area, which rises with
/// `acid_ratio`. Then the acid sea in the scene, whose surface rises with
/// `acid_ratio`, from below the map's lowest ground row at 0 to the top of the
/// map at 1, and fades below it to a deep green. Leaves on the canvas where
/// each layer slid to as the `backdrop_layers` meta, an array of Vector2
/// offsets from where it rests, back to front, notes on `art` where both acid
/// surfaces lie on the screen, and leaves the canvas drawing the scene as
/// `camera` shows it.
void paint_mouth(godot::Node2D &canvas, const Art &art, float acid_ratio,
                 const View &camera);
/// Draws the light and the steam over both acid surfaces `paint_mouth` draws,
/// and spores drifting far behind the map and near in front of it, for a
/// canvas that blends by adding.
void paint_glow(godot::Node2D &canvas, const Art &art, float acid_ratio,
                const View &camera);
/// Draws the light of the city's lamps and acid drips as `camera` shows them,
/// for a canvas that blends by adding.
void paint_city_glow(godot::Node2D &canvas, const Art &art,
                     const survival::Content &content, const View &camera);
/// Draws the city grid with its ground, scenery, objects, stock and actors,
/// in the scene's own pixels. Destroyed, depleted and unavailable objects
/// each have a mark; a dormant object, and a bag emptied or carried off, is
/// not drawn, and a repaired one looks intact. A source in `out_of_reach`
/// has stock no player can walk to: it loses the gold frame. Each actor faces
/// the way it last walked, alternates its walking frames while it moves, and
/// climbs on a ladder. The monster's upper teeth hang from the top of the
/// screen whatever the camera shows, in front of the backdrop scenery and the
/// supports and behind the buildings, the walkways and all on them.
void paint_city(godot::Node2D &canvas, const Art &art,
                const survival::Content &content,
                const survival::GameView &view,
                const std::vector<survival::ObjectId> &out_of_reach);
/// Draws the city's words on the screen where the shot's camera shows what
/// they name, each on a dark plate over the art so it reads at any acid
/// level, none covering another or a source other than its own count, and
/// none for what is off the play area. A source out of reach is named as
/// having no way there. Each goal whose area is off the play area gets a
/// pointer at the area's edge, aimed at it, with its name. Leaves on the
/// canvas the plates drawn as the `map_plates` meta, an array of Rect2; the
/// names of the sources the city draws as `map_sources`, their areas on the
/// screen as `map_source_areas`, an array of Rect2, and the names of those
/// whose count is on the play area as `map_labelled`, with those counts'
/// plates as `map_counts`; the names pointed at as
/// `map_pointers`; the camera as `map_view` and where it heads as
/// `map_heading`, each a Transform2D from the scene's pixels to the screen's.
void paint_city_labels(godot::Node2D &canvas, const Art &art,
                       const survival::Content &content,
                       const survival::GameView &view,
                       survival::ControlMap control, const Shot &shot);
/// Draws the base kitchen with the chef and the appraiser, and the monster's
/// feeding bucket, steaming once `dish_ready`.
void paint_workshop(godot::Node2D &canvas, const Art &art, bool dish_ready);

} // namespace neocity::rendering
