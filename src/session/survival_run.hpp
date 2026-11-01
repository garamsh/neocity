#pragma once

#include "../hud/survival.hpp"

#include <survival/companion.hpp>
#include <survival/content.hpp>

#include <godot_cpp/variant/dictionary.hpp>

#include <array>
#include <cstdint>
#include <deque>
#include <functional>

namespace neocity::session {

/// One survival run's rule objects, owned by the composition root.
struct SurvivalFeatures {
  std::reference_wrapper<const survival::Content> content;
  std::reference_wrapper<survival::Simulation> simulation;
  std::reference_wrapper<survival::World> world;
  std::reference_wrapper<survival::Exploration> exploration;
  std::reference_wrapper<survival::Workshop> workshop;
  std::reference_wrapper<const survival::Companion> companion;
};

/// Drives one survival run: accumulates frame time into fixed 100ms steps,
/// retains input between them, and runs both actors' commands through the
/// rules. The step clock runs in every phase; `Exploration` spends only
/// exploration time from it.
class SurvivalRun {
public:
  SurvivalRun(SurvivalFeatures rules, survival::ControlMap players);
  /// Retains this frame's input and runs every step the frame time completes.
  void advance(double delta, const std::array<survival::PlayerInput, 2> &input);
  /// Who drives each actor, for sampling their keys.
  [[nodiscard]] survival::ControlMap control() const;
  /// Reads one frame's values for the screen.
  [[nodiscard]] hud::SurvivalFrame frame() const;
  /// Reports the screen's values by name for host integration assertions.
  [[nodiscard]] godot::Dictionary status() const;

private:
  [[nodiscard]] survival::GameView view() const;
  void step();
  void run_action(survival::Actor actor, const survival::Action &action,
                  std::uint32_t day);
  void hurt_in_hazards(const survival::ExplorationSnapshot &before);
  void report_returns(const survival::Snapshot &before,
                      const std::array<bool, 2> &walked_home);
  void report_world_changes(const survival::WorldSnapshot &before);
  void record(const hud::Event &event);

  SurvivalFeatures features;
  survival::ControlMap controllers;
  double accumulated = 0.0;
  std::array<survival::PlayerInput, 2> pending{};
  std::array<survival::ExplorationInput, 2> held{};
  std::deque<hud::Event> events;
};

} // namespace neocity::session
