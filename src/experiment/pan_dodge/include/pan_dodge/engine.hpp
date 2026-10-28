#pragma once

#include <cstdint>

#include <vector>

namespace neocity::pan_dodge {

/// Direction of direct movement along one pan axis.
enum class Direction : std::uint8_t { Negative, Neutral, Positive };

/// A direct movement request for one 120 Hz tick.
struct Command {
  Direction horizontal = Direction::Neutral;
  Direction vertical = Direction::Neutral;
};

/// Position in pan-local units: the pan centre is the origin and positive y
/// points toward the viewer.
struct Point {
  float x = 0.0F;
  float y = 0.0F;
};

/// Circular contact geometry.
struct Disc {
  Point center;
  float radius = 0.0F;
};

/// A dropped hazard follows the player while tracking, keeps its landing point
/// while falling, and only collides once it has landed.
enum class Phase : std::uint8_t { Tracking, Falling, Landed };

/// One spawned hazard; it despawns when its landed life ends or it slides off.
struct Hazard {
  Disc body;
  Point velocity;
  Phase phase = Phase::Falling;
  /// Height above the pan before landing, in pan units.
  float height = 0.0F;
  /// Seconds left before a landed hazard despawns.
  float life = 0.0F;
};

/// Terminal outcomes remain visible until reset.
enum class Outcome : std::uint8_t { Playing, Escaped, Hit };

/// Value snapshot of the moving-pan dodge experiment.
struct Snapshot {
  Disc player{.center = {}, .radius = 12.0F};
  std::vector<Hazard> hazards;
  /// The pan's own sway; each axis lies in [-1, 1].
  Point tilt;
  /// Centre of the rim opening, in radians from the positive x axis.
  float exit_angle = -1.5707964F;
  float exit_half_angle = 0.24F;
  float pan_radius = 250.0F;
  std::uint32_t tick = 0;
  Outcome outcome = Outcome::Playing;
};

/// Independent rules with no host or wall-clock dependency: the player moves
/// directly while the pan sways, its exit travels around the rim, and hazards
/// drop in and later despawn.
class Engine {
public:
  /// Advances the pan, hazards, movement and contact by one fixed tick.
  void step(Command command);
  /// Restores the complete initial run, including the drop sequence.
  void reset();
  /// Returns geometry and outcome without exposing mutable state.
  [[nodiscard]] Snapshot snapshot() const;

private:
  void spawn();
  void advance_hazards();

  Snapshot world;
  std::uint32_t random = 0x2545F491U;
  std::uint32_t drops = 0;
};

} // namespace neocity::pan_dodge
