#pragma once

#include <cstdint>

#include <array>

namespace neocity::pan_dodge {

/// Direction of direct movement along one pan axis.
enum class Direction : std::uint8_t { Negative, Neutral, Positive };

/// A direct movement request for one 120 Hz tick.
struct Command {
  Direction horizontal = Direction::Neutral;
  Direction vertical = Direction::Neutral;
};

/// Position in pan-local units, with positive y pointing down.
struct Point {
  float x = 0.0F;
  float y = 0.0F;
};

/// Circular contact geometry.
struct Disc {
  Point center;
  float radius = 0.0F;
};

/// Terminal outcomes remain visible until reset.
enum class Outcome : std::uint8_t { Playing, Escaped, Hit };

/// Value snapshot of the direct-movement experiment.
struct Snapshot {
  Disc player{.center = {.x = -170.0F, .y = 130.0F}, .radius = 12.0F};
  std::array<Disc, 3> hazards{
      {{.center = {.x = -50.0F, .y = 100.0F}, .radius = 32.0F},
       {.center = {.x = 90.0F, .y = -55.0F}, .radius = 36.0F},
       {.center = {.x = -90.0F, .y = -70.0F}, .radius = 26.0F}}};
  float pan_radius = 250.0F;
  float exit_half_width = 48.0F;
  Outcome outcome = Outcome::Playing;
};

/// Independent direct-movement rules with no host or wall-clock dependency.
class Engine {
public:
  /// Advances movement and contact by one fixed tick.
  void step(Command command);
  /// Restores the complete initial run.
  void reset();
  /// Returns geometry and outcome without exposing mutable state.
  [[nodiscard]] Snapshot snapshot() const;

private:
  Snapshot world;
};

} // namespace neocity::pan_dodge
