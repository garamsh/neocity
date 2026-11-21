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
/// go at that zoom. `inside` is set while the view shows only the inside of
/// one building, whose whole extent in the scene is `room`.
struct View {
  godot::Vector2 centre;
  float zoom = 1.0F;
  godot::Vector2 rest;
  bool inside = false;
  godot::Rect2 room = godot::Rect2();
};

/// The screen position of the scene's origin, on a whole pixel.
[[nodiscard]] godot::Vector2 origin(const View &camera);
[[nodiscard]] godot::Vector2 to_screen(const View &camera,
                                       godot::Vector2 point);
[[nodiscard]] godot::Rect2 to_screen(const View &camera, godot::Rect2 area);
/// The same mapping as a transform, for the Godot checks.
[[nodiscard]] godot::Transform2D transform(const View &camera);

/// The whole scene on one screen at its own size, as the workshop shows it.
[[nodiscard]] View whole_scene();

/// Where the camera over the city heads: the actors in the city, human or AI.
/// One is shown at twice the scene's size, a little ahead of the way it faces;
/// two are framed together at twice the
/// size while they fit and at the scene's own size once they do not, which the
/// screen lock keeps from happening sideways. With nobody in the city it rests
/// on the base entrance. It never shows past the map's left and right edges or
/// the scene's top and bottom, and centres on the map when the view is wider
/// than it. Inside a building, which every actor in the city is in, it follows
/// the same way at the same zooms, its centre kept within the building's rooms,
/// so the room scrolls past as the actors walk and the view past its sides is
/// darkness.
[[nodiscard]] View aim(const survival::Content &content,
                       const survival::GameView &view);

/// The camera easing from where it last was toward `heading` on the real
/// clock, inside the same bounds as `aim`; its zoom is always a whole number,
/// stepping once the eased zoom passes halfway between two.
[[nodiscard]] View follow(const Art &art, const survival::MapContent &content,
                          const View &heading);

/// How much of its size an actor is drawn at under `camera`: whole on the
/// street, and inside a building the street's close zoom over the camera's, so
/// an actor stands as tall on the screen as on the street while the rooms,
/// drawn two cells tall, are four times its height.
[[nodiscard]] float actor_scale(bool inside, float zoom);

/// How far, in screen pixels, a backdrop layer `depth` of the way from the
/// back of the scene to the map has slid from where it rests: 0 stays and 1
/// moves with the map.
[[nodiscard]] godot::Vector2 drift(const View &camera, float depth);

} // namespace neocity::rendering
