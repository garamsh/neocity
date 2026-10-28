#include "drawing.hpp"

#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <array>
#include <cmath>
#include <cstdlib>
#include <utility>

namespace neocity::ascent {

namespace {

using godot::Vector2;

/// Euclidean remainder, non-negative for a positive divisor.
float rem_euclid(float value, float divisor) {
  const float r = std::fmod(value, divisor);
  return r < 0.0F ? r + std::abs(divisor) : r;
}

bool within(float value, float start, float end) {
  return start <= value && value < end;
}

godot::Ref<godot::Texture2D> load_texture(const godot::String &path) {
  const godot::Ref<godot::Texture2D> texture =
      godot::ResourceLoader::get_singleton()->load(path);
  CRASH_COND_MSG(texture.is_null(), "Ascent texture must load: " + path);
  return texture;
}

std::size_t sprite_of(Character character) {
  switch (character) {
  case Character::Chef:
    return 0;
  case Character::Detective:
    return 2;
  }
  std::abort(); // unreachable: the switch covers every Character
}

} // namespace

godot::Color color(std::uint8_t r, std::uint8_t g, std::uint8_t b, float a) {
  return {static_cast<float>(r) / 255.0F, static_cast<float>(g) / 255.0F,
          static_cast<float>(b) / 255.0F, a};
}

godot::Rect2 rect(float x, float y, float w, float h) { return {x, y, w, h}; }

AscentDrawing::AscentDrawing(godot::Ref<godot::Texture2D> city,
                             godot::Ref<godot::Texture2D> sprites,
                             godot::Ref<godot::Font> fallback_font)
    : background(std::move(city)), atlas(std::move(sprites)),
      font(std::move(fallback_font)) {}

AscentDrawing AscentDrawing::load() {
  godot::Ref<godot::Font> font =
      godot::ThemeDB::get_singleton()->get_fallback_font();
  CRASH_COND_MSG(font.is_null(), "Godot fallback font must be available");
  return {load_texture("res://ascent/neon_city.png"),
          load_texture("res://ascent/ascent_atlas.png"), std::move(font)};
}

void AscentDrawing::text(godot::Node2D &canvas, float x, float y,
                         const godot::String &text, int size,
                         godot::Color tint) const {
  canvas.draw_string(font, Vector2(x, y), text,
                     godot::HORIZONTAL_ALIGNMENT_LEFT, -1, size, tint);
}

void AscentDrawing::sprite(godot::Node2D &canvas, std::size_t index,
                           godot::Rect2 destination, bool flip) const {
  // Source rectangles are measured from the untouched atlas, which is not a
  // uniform grid.
  constexpr std::array<std::array<float, 4>, 8> bounds{{
      {88.0F, 90.0F, 354.0F, 408.0F},
      {494.0F, 64.0F, 397.0F, 364.0F},
      {990.0F, 156.0F, 256.0F, 342.0F},
      {1396.0F, 129.0F, 275.0F, 324.0F},
      {58.0F, 600.0F, 360.0F, 228.0F},
      {453.0F, 567.0F, 422.0F, 253.0F},
      {890.0F, 655.0F, 437.0F, 127.0F},
      {1458.0F, 598.0F, 224.0F, 238.0F},
  }};
  const auto &[x, y, w, h] = bounds.at(index);
  const godot::Rect2 source = rect(x, y, w, h);
  godot::Rect2 target = destination;
  if (flip) {
    target.position.x += target.size.x;
    target.size.x = -target.size.x;
  }
  canvas.draw_texture_rect_region(atlas, target, source);
}

void AscentDrawing::actor(godot::Node2D &canvas, Actor actor, float camera,
                          bool leader, float time) const {
  const std::size_t index =
      sprite_of(actor.character) + (actor.airborne ? 1U : 0U);
  const float bob =
      actor.airborne ? 0.0F : std::abs(std::sin(time * 12.0F)) * 1.5F;
  const float x = actor.feet.x;
  const float y = actor.feet.y - camera;
  canvas.draw_circle(Vector2(x, y - 3.0F), 18.0F, color(4, 8, 25, 0.5F));
  sprite(canvas, index, rect(x - 31.0F, y - 74.0F - bob, 62.0F, 74.0F),
         !actor.facing_right);
  if (leader) {
    canvas.draw_circle(Vector2(x, y - 88.0F - bob), 2.5F,
                       color(105, 251, 231, 1.0F));
  }
}

void AscentDrawing::paint(godot::Node2D &canvas, const WorldSnapshot &world,
                          float camera) const {
  const godot::Color white = color(228, 234, 255, 1.0F);
  const godot::Color muted = color(144, 159, 195, 1.0F);
  const godot::Color cyan = color(91, 244, 219, 1.0F);
  const godot::Color gold = color(255, 192, 83, 1.0F);
  canvas.draw_rect(rect(0.0F, 0.0F, 1280.0F, 720.0F), color(7, 10, 30, 1.0F));
  canvas.draw_texture_rect(
      background,
      rect(-55.0F, -90.0F - world.progress * 70.0F, 1390.0F, 920.0F), false);
  canvas.draw_rect(rect(0.0F, 0.0F, 1280.0F, 720.0F), color(8, 10, 32, 0.17F));
  // Near structures scroll independently from the distant city texture.
  constexpr std::array structures{28.0F, 1228.0F};
  for (std::size_t i = 0; i < structures.size(); ++i) {
    const float x = structures.at(i);
    canvas.draw_rect(rect(x, 0.0F, 25.0F, 720.0F), color(9, 17, 34, 0.94F));
    canvas.draw_rect(rect(x + 4.0F, 0.0F, 3.0F, 720.0F),
                     color(43, 65, 85, 0.8F));
    for (std::size_t n = 0; n < 10; ++n) {
      const float y =
          rem_euclid(static_cast<float>(n) * 110.0F - camera * 0.7F, 820.0F) -
          60.0F;
      canvas.draw_line(Vector2(x, y), Vector2(x + 25.0F, y + 70.0F),
                       color(32, 47, 69, 1.0F));
      if ((n + i) % 3 == 0) {
        canvas.draw_rect(rect(x - 3.0F, y + 12.0F, 9.0F, 18.0F),
                         color(249, 64, 100, 0.9F));
      }
    }
  }
  for (std::size_t i = 0; i < world.surfaces.size(); ++i) {
    const Surface &p = world.surfaces.at(i);
    const float y = p.y - camera;
    if (!within(y, -100.0F, 820.0F)) {
      continue;
    }
    const float x = p.x;
    canvas.draw_rect(rect(x + 5.0F, y + 22.0F, p.width - 10.0F, 8.0F),
                     color(3, 7, 17, 0.9F));
    canvas.draw_rect(rect(x + 18.0F, y + 27.0F, 9.0F, 110.0F),
                     color(13, 24, 43, 1.0F));
    canvas.draw_rect(rect(x + p.width - 27.0F, y + 27.0F, 9.0F, 80.0F),
                     color(13, 24, 43, 1.0F));
    canvas.draw_line(Vector2(x + 22.0F, y + 105.0F),
                     Vector2(x + p.width - 22.0F, y + 30.0F),
                     color(34, 49, 68, 1.0F), 5.0F);
    // The atlas module is decorative; its bright flat rim coincides with the
    // collision top.
    sprite(canvas, 6, rect(x, y - 1.0F, p.width, 36.0F), false);
    canvas.draw_rect(rect(x, y, p.width, 3.0F), p.moving ? cyan : gold);
    if (p.moving) {
      canvas.draw_line(Vector2(x + 30.0F, 0.0F), Vector2(x + 30.0F, y),
                       color(57, 65, 88, 0.8F));
      canvas.draw_line(Vector2(x + p.width - 30.0F, 0.0F),
                       Vector2(x + p.width - 30.0F, y),
                       color(57, 65, 88, 0.8F));
    }
    if (i == 0) {
      text(canvas, x + 24.0F, y + 45.0F, "MARKET SERVICE / 01", 12, muted);
    }
  }
  const Point cp = world.checkpoint;
  const float cy = cp.y - camera;
  if (within(cy, -80.0F, 800.0F)) {
    canvas.draw_rect(rect(cp.x - 5.0F, cy - 42.0F, 10.0F, 42.0F),
                     color(21, 42, 62, 1.0F));
    canvas.draw_rect(rect(cp.x - 10.0F, cy - 50.0F, 20.0F, 18.0F), cyan);
    canvas.draw_circle(Vector2(cp.x, cy - 42.0F), 30.0F,
                       color(57, 240, 229, 0.09F));
    text(canvas, cp.x + 24.0F, cy - 29.0F,
         world.checkpoint_reached ? "CHECKPOINT SAVED" : "SAFE LANDING", 12,
         cyan);
  }
  for (const Hazard &h : world.hazards) {
    const float y = h.center.y - camera;
    if (!within(y, -80.0F, 800.0F)) {
      continue;
    }
    switch (h.kind) {
    case HazardKind::Patrol:
      sprite(canvas, 4, rect(h.center.x - 29.0F, y - 20.0F, 58.0F, 40.0F),
             false);
      break;
    case HazardKind::Flyer: {
      const float flutter = std::sin(world.elapsed * 16.0F) * 6.0F;
      sprite(canvas, 5,
             rect(h.center.x - 40.0F, y - 25.0F + flutter, 80.0F, 50.0F),
             false);
      break;
    }
    case HazardKind::Projectile:
      if (h.warning) {
        canvas.draw_circle(Vector2(1130.0F, y), 18.0F,
                           color(251, 115, 117, 0.35F));
        text(canvas, 1124.0F, y + 5.0F, "!", 20, gold);
      } else if (h.active) {
        canvas.draw_line(Vector2(h.center.x + 20.0F, y), Vector2(h.center.x, y),
                         color(165, 255, 106, 0.5F), 5.0F);
        canvas.draw_circle(Vector2(h.center.x, y), 7.0F,
                           color(177, 255, 107, 1.0F));
      }
      break;
    }
  }
  const Point goal = world.ingredient;
  const float gy = goal.y - camera;
  if (within(gy, -100.0F, 820.0F)) {
    canvas.draw_circle(Vector2(goal.x, gy - 30.0F), 45.0F,
                       color(255, 176, 63, 0.08F));
    sprite(canvas, 7,
           rect(goal.x - 25.0F,
                gy - 58.0F + std::sin(world.elapsed * 2.0F) * 4.0F, 50.0F,
                53.0F),
           false);
    text(canvas, goal.x - 65.0F, gy - 85.0F, "THE LAST INGREDIENT", 12, gold);
  }
  actor(canvas, world.companion, camera, false, world.elapsed);
  actor(canvas, world.leader, camera, true, world.elapsed);
  if (world.invulnerable) {
    canvas.draw_circle(
        Vector2(world.leader.feet.x, world.leader.feet.y - camera - 28.0F),
        38.0F, color(91, 244, 219, 0.3F), false, 1.5F);
  }
  hud(canvas, world, white, muted, cyan, gold);
}

} // namespace neocity::ascent
