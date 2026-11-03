#pragma once

#include "drawing.hpp"

#include <survival/companion.hpp>
#include <survival/content.hpp>

#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace neocity::rendering {

/// Where the scene, in its own pixels, falls on the screen: `centre` is drawn
/// at the middle of the play area between the top bar and the cards, `zoom`
/// times its size. `rest` is the centre in the middle of where the camera may
/// go at that zoom.
struct View {
  godot::Vector2 centre;
  float zoom = 1.0F;
  godot::Vector2 rest;
};

/// The screen position of the scene's origin, on a whole pixel.
[[nodiscard]] godot::Vector2 origin(const View &view);
[[nodiscard]] godot::Vector2 to_screen(const View &view, godot::Vector2 point);
[[nodiscard]] godot::Rect2 to_screen(const View &view, godot::Rect2 area);
/// The same mapping as a transform, for the Godot checks.
[[nodiscard]] godot::Transform2D transform(const View &view);

/// The whole scene on one screen at its own size, as the workshop shows it.
[[nodiscard]] View whole_scene();

/// The camera over the city: it follows the humans in the city, never an AI
/// partner, easing toward where they are on the real clock. One human is
/// shown at twice the scene's size, a little ahead of the way it faces; two
/// are framed together between once and twice the size, smaller as they
/// part. With no human in the city it rests on the base entrance. It never
/// shows past the map's left and right edges or the scene's top and bottom,
/// and centres on the map when the view is wider than it.
[[nodiscard]] View follow(const Art &art, const survival::Content &content,
                          const survival::GameView &view,
                          survival::ControlMap control);

/// How far, in screen pixels, a backdrop layer `depth` of the way from the
/// back of the scene to the map has slid from where it rests: 0 stays and 1
/// moves with the map.
[[nodiscard]] godot::Vector2 drift(const View &view, float depth);

} // namespace neocity::rendering
