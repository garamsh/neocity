#pragma once

#include <cstdint>

#include <array>

namespace neocity::pan_tilt {

/// Target tilt direction; neutral levels the pan without stopping motion.
enum class Direction : std::uint8_t { Negative, Neutral, Positive };

/// Requests pan tilt rather than player displacement for one 120 Hz tick.
struct Command {
  Direction horizontal = Direction::Neutral;
  Direction vertical = Direction::Neutral;
};

/// Two-dimensional pan-local position, velocity, or normalized tilt.
struct Point {
  float x = 0.0F;
  float y = 0.0F;
};

/// A sliding circular body, in pan units and units per second.
struct Body {
  Point center;
  Point velocity;
  float radius = 0.0F;
};

/// Terminal outcomes freeze the entire pan until reset.
enum class Outcome : std::uint8_t { Playing, Escaped, Hit };

/// Independent values read by presentation and tests.
struct Snapshot {
  Body player{
      .center = {.x = -150.0F, .y = 140.0F}, .velocity = {}, .radius = 12.0F};
  std::array<Body, 2> hazards{
      {{.center = {.x = -30.0F, .y = 45.0F}, .velocity = {}, .radius = 28.0F},
       {.center = {.x = 95.0F, .y = -55.0F}, .velocity = {}, .radius = 34.0F}}};
  Point tilt;
  float pan_radius = 250.0F;
  float exit_half_width = 48.0F;
  Outcome outcome = Outcome::Playing;
};

/// Tilt-driven rules; all bodies receive the same gravity and drag.
class Engine {
public:
  /// Advances tilt, inertia, wall response, and contact by one fixed tick.
  void step(Command command);
  /// Clears velocity, tilt, contact outcome, and positions.
  void reset();
  /// Returns the complete observable experiment state.
  [[nodiscard]] Snapshot snapshot() const;

private:
  Snapshot world;
};

} // namespace neocity::pan_tilt
