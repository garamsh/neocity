#include <pan_dodge/engine.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <numbers>

namespace neocity::pan_dodge {
namespace {

constexpr float TICK = 1.0F / 120.0F;
constexpr float TAU = 2.0F * std::numbers::pi_v<float>;
constexpr float SPEED = 170.0F;
constexpr float DRIFT = 90.0F;
constexpr float SWAY = 0.6F;
constexpr float SPIN = 0.35F;
constexpr float SLIDE = 180.0F;
constexpr float DRAG = 0.65F;
constexpr std::uint32_t FIRST_DROP = 180;
constexpr std::uint32_t DROP_INTERVAL = 120;
constexpr float DROP_HEIGHT = 300.0F;
constexpr float FALL_SECONDS = 1.0F;
constexpr float LOCK_HEIGHT = DROP_HEIGHT * 0.35F;
constexpr float LANDED_SECONDS = 3.0F;

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

bool touching(Disc a, Disc b) {
  return std::hypot(a.center.x - b.center.x, a.center.y - b.center.y) <=
         a.radius + b.radius;
}

} // namespace

void Engine::step(Command command) {
  if (world.outcome != Outcome::Playing) {
    return;
  }
  ++world.tick;
  const float time = static_cast<float>(world.tick) * TICK;
  world.tilt = {.x = SWAY * std::sin(TAU * time / 5.0F),
                .y = SWAY * std::sin(TAU * time / 7.0F + 1.0F)};
  world.exit_angle =
      std::remainder(-std::numbers::pi_v<float> / 2.0F + SPIN * time, TAU);
  const float x = axis(command.horizontal);
  const float y = axis(command.vertical);
  const float length = std::max(1.0F, std::hypot(x, y));
  Point &position = world.player.center;
  position.x += (x / length * SPEED + world.tilt.x * DRIFT) * TICK;
  position.y += (y / length * SPEED + world.tilt.y * DRIFT) * TICK;

  const float distance = std::hypot(position.x, position.y);
  const float offset = std::remainder(
      std::atan2(position.y, position.x) - world.exit_angle, TAU);
  const bool at_exit =
      std::abs(offset) <=
      world.exit_half_angle - world.player.radius / world.pan_radius;
  if (at_exit && distance > world.pan_radius) {
    world.outcome = Outcome::Escaped;
    return;
  }
  const float limit = world.pan_radius - world.player.radius;
  if (!at_exit && distance > limit) {
    position.x *= limit / distance;
    position.y *= limit / distance;
  }
  advance_hazards();
  if (world.outcome == Outcome::Playing && world.tick >= FIRST_DROP &&
      (world.tick - FIRST_DROP) % DROP_INTERVAL == 0) {
    spawn();
  }
}

void Engine::spawn() {
  const auto unit = [this] {
    random = random * 1664525U + 1013904223U;
    return static_cast<float>(random >> 8U) / 16777216.0F;
  };
  Hazard hazard{
      .body = {.center = world.player.center, .radius = 18.0F + 12.0F * unit()},
      .velocity = {},
      .phase = Phase::Tracking,
      .height = DROP_HEIGHT,
      .life = LANDED_SECONDS};
  if (drops % 2 == 1) {
    const float distance = 200.0F * std::sqrt(unit());
    const float angle = TAU * unit();
    hazard.body.center = {.x = distance * std::cos(angle),
                          .y = distance * std::sin(angle)};
    hazard.phase = Phase::Falling;
  }
  ++drops;
  world.hazards.push_back(hazard);
}

void Engine::advance_hazards() {
  for (Hazard &hazard : world.hazards) {
    switch (hazard.phase) {
    case Phase::Tracking:
      hazard.body.center = world.player.center;
      [[fallthrough]];
    case Phase::Falling:
      hazard.height -= DROP_HEIGHT / FALL_SECONDS * TICK;
      if (hazard.height <= 0.0F) {
        hazard.height = 0.0F;
        hazard.phase = Phase::Landed;
      } else if (hazard.height <= LOCK_HEIGHT) {
        hazard.phase = Phase::Falling;
      }
      break;
    case Phase::Landed:
      hazard.velocity.x +=
          (world.tilt.x * SLIDE - hazard.velocity.x * DRAG) * TICK;
      hazard.velocity.y +=
          (world.tilt.y * SLIDE - hazard.velocity.y * DRAG) * TICK;
      hazard.body.center.x += hazard.velocity.x * TICK;
      hazard.body.center.y += hazard.velocity.y * TICK;
      hazard.life -= TICK;
      break;
    }
    if (hazard.phase == Phase::Landed && touching(hazard.body, world.player)) {
      world.outcome = Outcome::Hit;
    }
  }
  std::erase_if(world.hazards, [this](const Hazard &hazard) {
    return hazard.phase == Phase::Landed &&
           (hazard.life <= 0.0F ||
            std::hypot(hazard.body.center.x, hazard.body.center.y) >
                world.pan_radius);
  });
}

void Engine::reset() { *this = Engine{}; }

Snapshot Engine::snapshot() const { return world; }

} // namespace neocity::pan_dodge
