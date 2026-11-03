#include "camera.hpp"

#include "survival.hpp"

#include <algorithm>
#include <array>
#include <vector>

namespace neocity::rendering {
namespace {

using godot::Vector2;
using survival::Actor;

constexpr float SCREEN_WIDTH = 1280.0F;
constexpr float SCREEN_HEIGHT = 720.0F;
/// The middle of the play area, between the top bar and the cards.
constexpr Vector2 FOCUS(640.0F, 301.0F);
/// The play area's height on screen.
constexpr float PLAY_HEIGHT = 478.0F;
constexpr float CLOSE = 2.0F;
constexpr float FAR = 1.0F;
/// How far ahead of a lone human the camera looks, in scene pixels.
constexpr float LOOK_AHEAD = 80.0F;
/// How far above its feet the camera aims at a human: about its waist.
constexpr float WAIST = 32.0F;
/// Room kept around two humans, in scene pixels: sideways, and above and
/// below their feet.
constexpr float ROOM_ACROSS = 256.0F;
constexpr float ROOM_UP = 128.0F;

constexpr std::array ACTORS{Actor::Chef, Actor::Appraiser};

/// Keeps `centre` where a view at `zoom` stays inside `bounds`, or centres it
/// on an axis the view is wider than.
Vector2 clamp_centre(Vector2 centre, float zoom, godot::Rect2 bounds) {
  const auto axis = [](float at, float low, float high, float before,
                       float after) {
    if (high - low <= before + after) {
      return low + before + (high - low - before - after) / 2.0F;
    }
    return std::clamp(at, low + before, high - after);
  };
  const Vector2 end = bounds.position + bounds.size;
  return {axis(centre.x, bounds.position.x, end.x, FOCUS.x / zoom,
               (SCREEN_WIDTH - FOCUS.x) / zoom),
          axis(centre.y, bounds.position.y, end.y, FOCUS.y / zoom,
               (SCREEN_HEIGHT - FOCUS.y) / zoom)};
}

/// A human the camera follows: its feet and which way it faces.
struct Subject {
  Vector2 feet;
  float facing = 1.0F;
};

} // namespace

Vector2 origin(const View &view) {
  return (FOCUS - view.centre * view.zoom).round();
}

Vector2 to_screen(const View &view, Vector2 point) {
  return origin(view) + point * view.zoom;
}

godot::Rect2 to_screen(const View &view, godot::Rect2 area) {
  return {to_screen(view, area.position), area.size * view.zoom};
}

godot::Transform2D transform(const View &view) {
  return {0.0F, Vector2(view.zoom, view.zoom), 0.0F, origin(view)};
}

View whole_scene() { return {.centre = FOCUS, .zoom = 1.0F, .rest = FOCUS}; }

View follow(const Art &art, const survival::Content &content,
            const survival::GameView &view, survival::ControlMap control) {
  const Vector2 corner = map_point({.x = 0, .y = 0});
  const Vector2 far_corner =
      map_point({.x = content.map.width * survival::SUBCELLS, .y = 0});
  const godot::Rect2 bounds(corner.x, 0.0F, far_corner.x - corner.x,
                            SCREEN_HEIGHT);
  std::vector<Subject> subjects;
  for (const auto actor : ACTORS) {
    const auto &state = actor == Actor::Chef ? view.progression.chef
                                             : view.progression.appraiser;
    if (!survival::key_set(control, actor) ||
        state.location != survival::Location::City) {
      continue;
    }
    const auto &body = actor == Actor::Chef ? view.exploration.chef.body
                                            : view.exploration.appraiser.body;
    subjects.push_back(
        {.feet = map_point(body.position),
         .facing = body.heading == survival::Direction::Left ? -1.0F : 1.0F});
  }
  Vector2 target;
  float zoom = CLOSE;
  if (subjects.empty()) {
    const auto &entrance = content.objects.front().area;
    target = map_point({.x = entrance.x * survival::SUBCELLS,
                        .y = entrance.y * survival::SUBCELLS}) -
             Vector2(0.0F, WAIST);
  } else if (subjects.size() == 1) {
    const auto &one = subjects.front();
    target = one.feet + Vector2(one.facing * LOOK_AHEAD, -WAIST);
  } else {
    const Vector2 first = subjects.front().feet;
    const Vector2 second = subjects.back().feet;
    const Vector2 apart = (first - second).abs();
    zoom = std::clamp(std::min(SCREEN_WIDTH / (apart.x + ROOM_ACROSS),
                               PLAY_HEIGHT / (apart.y + ROOM_UP)),
                      FAR, CLOSE);
    target = (first + second) / 2.0F - Vector2(0.0F, WAIST);
  }
  View shown{.centre = {}, .zoom = art.glide("camera_zoom", zoom), .rest = {}};
  shown.rest = clamp_centre(bounds.get_center(), shown.zoom, bounds);
  target = clamp_centre(target, shown.zoom, bounds);
  shown.centre = clamp_centre(
      Vector2(art.glide("camera_x", target.x), art.glide("camera_y", target.y)),
      shown.zoom, bounds);
  return shown;
}

Vector2 drift(const View &view, float depth) {
  return ((view.rest - view.centre) * view.zoom * depth).round();
}

} // namespace neocity::rendering
