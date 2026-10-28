#include <pan_tilt/engine.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace neocity::pan_tilt {
namespace {

constexpr float TICK = 1.0F / 120.0F;

float axis(Direction direction) {
  switch (direction) {
  case Direction::Negative:
    return -1.0F;
  case Direction::Neutral:
    return 0.0F;
  case Direction::Positive:
    return 1.0F;
  }
  std::abort();
}

void slide(Body &body, Point tilt) {
  body.velocity.x += (tilt.x * 260.0F - body.velocity.x * 0.65F) * TICK;
  body.velocity.y += (tilt.y * 260.0F - body.velocity.y * 0.65F) * TICK;
  body.center.x += body.velocity.x * TICK;
  body.center.y += body.velocity.y * TICK;
}

void rim(Body &body, float radius, float rebound) {
  const float distance = std::hypot(body.center.x, body.center.y);
  const float limit = radius - body.radius;
  if (distance <= limit) {
    return;
  }
  const Point normal{.x = body.center.x / distance,
                     .y = body.center.y / distance};
  body.center = {.x = normal.x * limit, .y = normal.y * limit};
  const float outward = body.velocity.x * normal.x + body.velocity.y * normal.y;
  if (outward > 0.0F) {
    body.velocity.x -= (1.0F + rebound) * outward * normal.x;
    body.velocity.y -= (1.0F + rebound) * outward * normal.y;
  }
}

} // namespace

void Engine::step(Command command) {
  if (world.outcome != Outcome::Playing) {
    return;
  }
  Point target{.x = axis(command.horizontal), .y = axis(command.vertical)};
  const float length = std::max(1.0F, std::hypot(target.x, target.y));
  target.x /= length;
  target.y /= length;
  world.tilt.x +=
      std::clamp(target.x - world.tilt.x, -2.5F * TICK, 2.5F * TICK);
  world.tilt.y +=
      std::clamp(target.y - world.tilt.y, -2.5F * TICK, 2.5F * TICK);
  slide(world.player, world.tilt);
  const Point position = world.player.center;
  const bool at_exit =
      std::abs(position.x) + world.player.radius <= world.exit_half_width &&
      position.y < 0.0F;
  if (!at_exit) {
    rim(world.player, world.pan_radius, 0.0F);
  }
  for (Body &hazard : world.hazards) {
    slide(hazard, world.tilt);
    rim(hazard, world.pan_radius, 0.65F);
    if (std::hypot(world.player.center.x - hazard.center.x,
                   world.player.center.y - hazard.center.y) <=
        world.player.radius + hazard.radius) {
      world.outcome = Outcome::Hit;
    }
  }
  if (world.outcome == Outcome::Playing && at_exit &&
      position.y < -world.pan_radius) {
    world.outcome = Outcome::Escaped;
  }
}

void Engine::reset() { world = Snapshot{}; }

Snapshot Engine::snapshot() const { return world; }

} // namespace neocity::pan_tilt
