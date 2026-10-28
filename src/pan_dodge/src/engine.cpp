#include <pan_dodge/engine.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace neocity::pan_dodge {
namespace {

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

} // namespace

void Engine::step(Command command) {
  if (world.outcome != Outcome::Playing) {
    return;
  }
  const float x = axis(command.horizontal);
  const float y = axis(command.vertical);
  const float length = std::max(1.0F, std::hypot(x, y));
  Point &position = world.player.center;
  position.x += x / length * (200.0F / 120.0F);
  position.y += y / length * (200.0F / 120.0F);

  const bool at_exit =
      std::abs(position.x) + world.player.radius <= world.exit_half_width &&
      position.y < 0.0F;
  if (at_exit && position.y < -world.pan_radius) {
    world.outcome = Outcome::Escaped;
    return;
  }
  const float distance = std::hypot(position.x, position.y);
  const float limit = world.pan_radius - world.player.radius;
  if (!at_exit && distance > limit) {
    position.x *= limit / distance;
    position.y *= limit / distance;
  }
  for (const Disc hazard : world.hazards) {
    if (std::hypot(position.x - hazard.center.x,
                   position.y - hazard.center.y) <=
        world.player.radius + hazard.radius) {
      world.outcome = Outcome::Hit;
    }
  }
}

void Engine::reset() { world = Snapshot{}; }

Snapshot Engine::snapshot() const { return world; }

} // namespace neocity::pan_dodge
