#include "pan.hpp"

#include "drawing.hpp"

#include <godot_cpp/variant/vector2.hpp>

namespace neocity::rendering {
namespace {

constexpr godot::Vector2 CENTER{425.0F, 390.0F};

void pan(godot::Node2D &canvas, float radius, float exit_width) {
  canvas.draw_rect(rect(0.0F, 0.0F, 1280.0F, 720.0F), color(19, 23, 30, 1.0F));
  canvas.draw_rect(
      rect(CENTER.x - 22.0F, CENTER.y + radius - 10.0F, 44.0F, 72.0F),
      color(105, 111, 125, 1.0F));
  canvas.draw_circle(CENTER, radius + 7.0F, color(143, 153, 172, 1.0F));
  canvas.draw_circle(CENTER, radius, color(42, 48, 60, 1.0F));
  canvas.draw_rect(rect(CENTER.x - exit_width, CENTER.y - radius - 16.0F,
                        exit_width * 2.0F, 32.0F),
                   color(79, 210, 165, 1.0F));
}

void disc(godot::Node2D &canvas, float x, float y, float radius, bool player) {
  const godot::Vector2 center = CENTER + godot::Vector2(x, y);
  canvas.draw_circle(center, radius + 2.0F, color(235, 240, 248, 1.0F));
  canvas.draw_circle(center, radius,
                     player ? color(92, 201, 255, 1.0F)
                            : color(246, 111, 91, 1.0F));
  if (!player) {
    canvas.draw_line(center - godot::Vector2(6.0F, 6.0F),
                     center + godot::Vector2(6.0F, 6.0F),
                     color(42, 48, 60, 1.0F), 3.0F);
    canvas.draw_line(center + godot::Vector2(-6.0F, 6.0F),
                     center + godot::Vector2(6.0F, -6.0F),
                     color(42, 48, 60, 1.0F), 3.0F);
  }
}

} // namespace

void paint_pan(godot::Node2D &canvas, const pan_dodge::Snapshot &world) {
  pan(canvas, world.pan_radius, world.exit_half_width);
  for (const auto &hazard : world.hazards) {
    disc(canvas, hazard.center.x, hazard.center.y, hazard.radius, false);
  }
  disc(canvas, world.player.center.x, world.player.center.y,
       world.player.radius, true);
}

void paint_pan(godot::Node2D &canvas, const pan_tilt::Snapshot &world) {
  pan(canvas, world.pan_radius, world.exit_half_width);
  for (const auto &hazard : world.hazards) {
    disc(canvas, hazard.center.x, hazard.center.y, hazard.radius, false);
  }
  disc(canvas, world.player.center.x, world.player.center.y,
       world.player.radius, true);
  const godot::Vector2 origin{1030.0F, 495.0F};
  canvas.draw_circle(origin, 58.0F, color(42, 48, 60, 1.0F));
  canvas.draw_line(origin,
                   origin + godot::Vector2(world.tilt.x, world.tilt.y) * 50.0F,
                   color(255, 210, 92, 1.0F), 5.0F);
  canvas.draw_circle(origin, 5.0F, color(255, 210, 92, 1.0F));
}

} // namespace neocity::rendering
