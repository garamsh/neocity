#pragma once

#include <cstdint>
#include <vector>

namespace neocity::ascent {

/// A location in the engine's world coordinate system, with positive Y
/// downward.
struct Point {
  float x = 0.0F;
  float y = 0.0F;

  friend bool operator==(const Point &, const Point &) = default;
};

/// The two selectable members of the expedition.
enum class Character : std::uint8_t { Chef, Detective };

/// A one-way landing surface. The top edge is its collision boundary.
struct Surface {
  float x = 0.0F;
  float y = 0.0F;
  float width = 0.0F;
  bool moving = false;
};

/// A character pose supplied to a presentation adapter.
struct Actor {
  Point feet;
  Character character = Character::Chef;
  bool airborne = false;
  bool facing_right = true;
};

/// The visible obstacle pattern, independent of sprites and rendering
/// resources.
enum class HazardKind : std::uint8_t { Patrol, Flyer, Projectile };

/// An obstacle's current collision volume and telegraph state.
struct Hazard {
  Point center;
  float radius = 0.0F;
  HazardKind kind = HazardKind::Patrol;
  bool warning = false;
  bool active = false;
};

/// Mutually exclusive expedition states.
enum class RunPhase : std::uint8_t { Playing, Paused, Complete };

/// Values exposed by the engine, without renderer objects or platform input
/// codes.
struct WorldSnapshot {
  Actor leader;
  Actor companion;
  std::vector<Surface> surfaces;
  std::vector<Hazard> hazards;
  Point checkpoint;
  bool checkpoint_reached = false;
  Point ingredient;
  RunPhase phase = RunPhase::Playing;
  float elapsed = 0.0F;
  std::uint32_t retries = 0;
  float progress = 0.0F;
  bool invulnerable = false;
};

} // namespace neocity::ascent
