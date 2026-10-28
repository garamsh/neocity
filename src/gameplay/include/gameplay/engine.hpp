#pragma once

#include <gameplay/world_snapshot.hpp>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>

namespace neocity::gameplay {

/// Platform-independent input sampled once per fixed simulation tick.
struct PlayerCommand {
  float horizontal = 0.0F;
  bool jump_pressed = false;
  bool jump_held = false;
  bool switch_character = false;
  bool pause_pressed = false;
  bool retry_pressed = false;
};

/// Owns the expedition rules; no Godot, clock, filesystem, or rendering
/// dependency.
class AscentEngine {
public:
  /// Starts a new course at the market's lower service platform.
  AscentEngine();

  /// Advances exactly one 120 Hz tick; adapters accumulate wall time outside
  /// the core.
  void step(PlayerCommand command);

  /// Returns immutable value data; the adapter chooses sprites, camera, UI,
  /// and light.
  [[nodiscard]] WorldSnapshot snapshot() const;

private:
  /// Gives the co-located unit tests access to the run state they arrange.
  friend struct EngineProbe;

  [[nodiscard]] Actor actor() const;
  void respawn();

  Point feet;
  Point velocity;
  Character leader = Character::Chef;
  bool facing_right = true;
  std::optional<std::size_t> grounded;
  float coyote = 0.0F;
  float jump_buffer = 0.0F;
  float time = 0.0F;
  float elapsed = 0.0F;
  bool checkpoint = false;
  std::uint32_t retries = 0;
  float invulnerability = 0.0F;
  float highest = 0.0F;
  RunPhase phase = RunPhase::Playing;
  std::deque<Actor> trail;
};

} // namespace neocity::gameplay
