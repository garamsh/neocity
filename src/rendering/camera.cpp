#include "camera.hpp"

#include "survival.hpp"

#include <algorithm>
#include <array>
#include <vector>

namespace neocity::rendering {
namespace {

using godot::Vector2;
using survival::Actor;

using rendering::SCREEN_HEIGHT;
using rendering::SCREEN_WIDTH;
/// The middle of the play area, between the top bar and the cards.
constexpr Vector2 FOCUS(SCREEN_WIDTH / 2.0F, 301.0F);
/// The play area's height on screen.
constexpr float PLAY_HEIGHT = 478.0F;
/// The camera's two zooms. Nothing between them is ever drawn, so each
/// picture pixel stays a whole square of screen pixels.
constexpr float CLOSE = 2.0F;
constexpr float FAR = 1.0F;
/// How far ahead of the actors the camera looks, in layout pixels on the
/// screen, so the actors stand that far behind the middle whatever the zoom.
constexpr float LOOK_AHEAD = 160.0F;
/// The largest zoom inside a building, and how much of the screen's height the
/// rooms the actors stand on fill: the zoom is the whole number that comes
/// nearest to it. A shop's room, 64 units tall, then fills about 71% of the
/// height at zoom 8 and is wider than the screen, so it scrolls past.
constexpr float MOST = 8.0F;
constexpr float ROOM_SHARE = 0.71F;
/// How far above its feet the camera aims at an actor, so one on the upper
/// walkway stands on it where the mockup draws that walkway, at screen row 325.
constexpr float WAIST = 12.0F;
/// Room kept around two actors, in scene pixels: sideways, and above and
/// below their feet.
constexpr float ROOM_ACROSS = 256.0F;
constexpr float ROOM_UP = 128.0F;
/// How far past either end of the map the camera may look, in scene pixels, so
/// a character at the end stands whole in the frame, not half off its edge.
constexpr float EDGE_ROOM = 64.0F;

/// The screen lock keeps two actors in the city no further apart sideways than
/// the close zoom still fits with their room, one scene pixel to a sub-cell.
static_assert((static_cast<float>(survival::SCREEN_LOCK) + ROOM_ACROSS) *
                  CLOSE <=
              SCREEN_WIDTH);

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

/// An actor the camera follows: its feet and which way it faces.
struct Subject {
  Vector2 feet;
  float facing = 1.0F;
};

/// Where the camera may look: the map's width with EDGE_ROOM beyond each end,
/// and the scene's height.
godot::Rect2 bounds_of(const survival::MapContent &map) {
  const Vector2 corner = map_point({.x = 0, .y = 0});
  const Vector2 far_corner =
      map_point({.x = map.size.width * survival::SUBCELLS, .y = 0});
  return {corner.x - EDGE_ROOM, 0.0F,
          far_corner.x - corner.x + 2.0F * EDGE_ROOM, SCREEN_HEIGHT};
}

/// `centre` and `zoom` kept inside `bounds`, with where they rest.
View placed(Vector2 centre, float zoom, godot::Rect2 bounds) {
  return {.centre = clamp_centre(centre, zoom, bounds),
          .zoom = zoom,
          .rest = clamp_centre(bounds.get_center(), zoom, bounds)};
}

} // namespace

Vector2 origin(const View &camera) {
  return (FOCUS - camera.centre * camera.zoom).round();
}

Vector2 to_screen(const View &camera, Vector2 point) {
  return origin(camera) + point * camera.zoom;
}

godot::Rect2 to_screen(const View &camera, godot::Rect2 area) {
  return {to_screen(camera, area.position), area.size * camera.zoom};
}

godot::Transform2D transform(const View &camera) {
  return {0.0F, Vector2(camera.zoom, camera.zoom), 0.0F, origin(camera)};
}

View whole_scene() { return {.centre = FOCUS, .zoom = 1.0F, .rest = FOCUS}; }

View aim(const survival::Content &content, const survival::GameView &view) {
  std::vector<Subject> subjects;
  for (const auto actor : ACTORS) {
    const auto &state = actor == Actor::Chef ? view.progression.chef
                                             : view.progression.appraiser;
    const auto &explorer = actor == Actor::Chef ? view.exploration.chef
                                                : view.exploration.appraiser;
    // An absent actor is not drawn, so the camera does not frame it.
    if (state.location != survival::Location::City || explorer.absent) {
      continue;
    }
    const auto &body = explorer.body;
    subjects.push_back(
        {.feet = map_point(body.position),
         .facing = body.heading == survival::Direction::Left ? -1.0F : 1.0F});
  }
  const auto inside = inside_of(content, view);
  // The zoom: the street's close zoom, its far one while two actors do not fit
  // it, and inside a building the one that fills the screen's height with the
  // rooms the actors stand on.
  float zoom = CLOSE;
  if (inside) {
    zoom = std::clamp(
        std::round(ROOM_SHARE * SCREEN_HEIGHT / inside->frame.size.y), FAR,
        MOST);
  } else if (subjects.size() > 1) {
    const Vector2 apart = (subjects.at(0).feet - subjects.at(1).feet).abs();
    const bool fit = (apart.x + ROOM_ACROSS) * CLOSE <= SCREEN_WIDTH &&
                     (apart.y + ROOM_UP) * CLOSE <= PLAY_HEIGHT;
    zoom = fit ? CLOSE : FAR;
  }
  Vector2 target;
  if (subjects.empty()) {
    const auto &entrance = content.objects.at(0).area;
    target = map_point({.x = entrance.x * survival::SUBCELLS,
                        .y = entrance.y * survival::SUBCELLS}) -
             Vector2(0.0F, WAIST);
  } else {
    // A lone actor, or the actors inside a building, look ahead together as
    // far as they face the same way; two on the street are framed centred.
    Vector2 middle;
    float facing = 0.0F;
    for (const auto &subject : subjects) {
      middle += subject.feet;
      facing += subject.facing;
    }
    const auto count = static_cast<float>(subjects.size());
    const bool ahead = inside || subjects.size() == 1;
    target = middle / count +
             Vector2(ahead ? facing / count * LOOK_AHEAD / zoom : 0.0F, -WAIST);
  }
  // The same follow in a building and on the street; only the bounds differ.
  if (inside) {
    auto camera = placed(target, zoom, inside->extent);
    camera.inside = true;
    camera.room = inside->extent;
    return camera;
  }
  return placed(target, zoom,
                bounds_of(survival::map_of(content, view.exploration.map)));
}

View follow(const Art &art, const survival::MapContent &content,
            const View &heading) {
  // The zoom steps once the eased value passes halfway, so a step wanted for
  // only a moment is never drawn.
  const float easing = art.glide("camera_zoom", heading.zoom);
  const float zoom = std::clamp(std::floor(easing + 0.5F), FAR, MOST);
  auto camera =
      placed(Vector2(art.glide("camera_x", heading.centre.x),
                     art.glide("camera_y", heading.centre.y)),
             zoom, heading.inside ? heading.room : bounds_of(content));
  camera.inside = heading.inside;
  camera.room = heading.room;
  return camera;
}

float actor_scale(bool inside, float zoom) {
  return inside ? CLOSE / zoom : 1.0F;
}

Vector2 drift(const View &camera, float depth) {
  return ((camera.rest - camera.centre) * camera.zoom * depth).round();
}

} // namespace neocity::rendering
