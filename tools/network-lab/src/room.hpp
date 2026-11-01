#pragma once

#include <survival/content.hpp>

#include <array>
#include <cstdint>

namespace neocity::lab {

/// What one step consumed: the whole input of a deterministic room.
struct StepInput {
  std::array<survival::PlayerInput, 2> inputs{};
  survival::ControlMap control{};
};

/// One run of the real rules, stepped as `session::SurvivalRun::step` steps
/// them. That class links Godot, so this keeps its order of rule calls.
class Room {
public:
  /// Builds the run for `seed`, then plays an AI-only party until `start_day`.
  Room(std::uint32_t seed, std::uint32_t start_day);
  void step(const StepInput &input);
  [[nodiscard]] survival::GameView view() const;
  [[nodiscard]] const survival::CompanionConfig &companion_config() const;

private:
  survival::Content data;
  survival::Simulation shared;
  survival::World map;
  survival::Exploration explorers;
  survival::Workshop kitchen;
  survival::Companion controllers;
};

/// Compares every value a client receives.
[[nodiscard]] bool same_view(const survival::GameView &left,
                             const survival::GameView &right);

} // namespace neocity::lab
