#include <ascent/engine.hpp>

#include "collision.hpp"
#include "course.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>

namespace neocity::ascent {

namespace {

constexpr Point CHECKPOINT{
    .x = 170.0F, .y = START.y - static_cast<float>(CHECKPOINT_INDEX) * 100.0F};

Character partner_of(Character character) {
  switch (character) {
  case Character::Chef:
    return Character::Detective;
  case Character::Detective:
    return Character::Chef;
  }
  std::abort(); // unreachable: the switch covers every Character
}

} // namespace

AscentEngine::AscentEngine()
    : feet(START), grounded(0), coyote(0.1F), invulnerability(1.0F),
      highest(START.y) {}

void AscentEngine::step(PlayerCommand command) {
  if (command.retry_pressed) {
    if (phase == RunPhase::Complete) {
      *this = AscentEngine{};
    } else {
      respawn();
    }
    return;
  }
  if (command.pause_pressed && phase != RunPhase::Complete) {
    phase = phase == RunPhase::Paused ? RunPhase::Playing : RunPhase::Paused;
  }
  if (phase != RunPhase::Playing) {
    return;
  }
  if (command.switch_character) {
    leader = partner_of(leader);
  }
  elapsed += STEP;
  invulnerability = std::fmax(invulnerability - STEP, 0.0F);
  const std::vector<Surface> old_surfaces = surfaces(time);
  time += STEP;
  const std::vector<Surface> platforms = surfaces(time);
  if (grounded) {
    feet.x += platforms.at(*grounded).x - old_surfaces.at(*grounded).x;
  }
  coyote = grounded ? 0.1F : std::fmax(coyote - STEP, 0.0F);
  jump_buffer =
      command.jump_pressed ? 0.12F : std::fmax(jump_buffer - STEP, 0.0F);
  const float horizontal = std::isfinite(command.horizontal)
                               ? std::clamp(command.horizontal, -1.0F, 1.0F)
                               : 0.0F;
  if (std::abs(horizontal) > 0.01F) {
    facing_right = horizontal > 0.0F;
  }
  const float target = horizontal * 285.0F;
  const float acceleration = horizontal == 0.0F ? 2400.0F : 1900.0F;
  velocity.x += std::clamp(target - velocity.x, -acceleration * STEP,
                           acceleration * STEP);
  if (jump_buffer > 0.0F && coyote > 0.0F) {
    velocity.y = -680.0F;
    grounded.reset();
    coyote = 0.0F;
    jump_buffer = 0.0F;
  }
  const float gravity =
      velocity.y < 0.0F && !command.jump_held ? 2800.0F : 1500.0F;
  velocity.y = std::fmin(velocity.y + gravity * STEP, 950.0F);
  const Point previous = feet;
  feet.x = std::clamp(feet.x + velocity.x * STEP, 20.0F, WIDTH - 20.0F);
  feet.y += velocity.y * STEP;
  grounded = landing(previous, feet, platforms);
  if (grounded) {
    feet.y = platforms.at(*grounded).y;
    velocity.y = 0.0F;
    if (*grounded >= CHECKPOINT_INDEX) {
      checkpoint = true;
    }
  }
  highest = std::fmin(highest, feet.y);
  if (feet.y > highest + 650.0F || feet.y > START.y + 250.0F) {
    respawn();
    return;
  }
  if (invulnerability <= 0.0F) {
    const std::vector<Hazard> current = hazards(time, platforms);
    if (std::ranges::any_of(
            current, [this](const Hazard &h) { return touches(feet, h); })) {
      respawn();
      return;
    }
  }
  if (std::abs(feet.x - GOAL.x) < 45.0F && std::abs(feet.y - GOAL.y) < 75.0F) {
    phase = RunPhase::Complete;
  }
  trail.push_back(actor());
  if (trail.size() > 26) {
    trail.pop_front();
  }
}

WorldSnapshot AscentEngine::snapshot() const {
  const Actor lead = actor();
  Actor companion = lead;
  if (trail.empty()) {
    companion.feet = Point{.x = feet.x - 42.0F, .y = feet.y};
  } else {
    companion = trail.front();
  }
  companion.character = partner_of(leader);
  if (std::abs(companion.feet.x - lead.feet.x) < 10.0F &&
      std::abs(companion.feet.y - lead.feet.y) < 10.0F) {
    companion.feet.x -= facing_right ? 38.0F : -38.0F;
  }
  std::vector<Surface> platforms = surfaces(time);
  std::vector<Hazard> current = hazards(time, platforms);
  return WorldSnapshot{
      .leader = lead,
      .companion = companion,
      .surfaces = std::move(platforms),
      .hazards = std::move(current),
      .checkpoint = CHECKPOINT,
      .checkpoint_reached = checkpoint,
      .ingredient = GOAL,
      .phase = phase,
      .elapsed = elapsed,
      .retries = retries,
      .progress =
          std::clamp((START.y - highest) / (START.y - GOAL.y), 0.0F, 1.0F),
      .invulnerable = invulnerability > 0.0F,
  };
}

Actor AscentEngine::actor() const {
  return Actor{
      .feet = feet,
      .character = leader,
      .airborne = !grounded,
      .facing_right = facing_right,
  };
}

void AscentEngine::respawn() {
  feet = checkpoint ? CHECKPOINT : START;
  velocity = Point{};
  highest = feet.y;
  grounded = checkpoint ? CHECKPOINT_INDEX : 0;
  coyote = 0.1F;
  jump_buffer = 0.0F;
  invulnerability = 1.4F;
  retries += 1;
  trail.clear();
}

} // namespace neocity::ascent
