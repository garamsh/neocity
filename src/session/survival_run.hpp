#pragma once

#include "../hud/survival.hpp"

#include <survival/companion.hpp>
#include <survival/content.hpp>
#include <survival/escape.hpp>

#include <godot_cpp/variant/dictionary.hpp>

#include <array>
#include <cstdint>
#include <deque>
#include <functional>
#include <optional>

namespace neocity::session {

/// One survival run's rule objects, owned by the composition root.
struct SurvivalFeatures {
  std::reference_wrapper<const survival::Content> content;
  std::reference_wrapper<survival::Simulation> simulation;
  std::reference_wrapper<survival::World> world;
  std::reference_wrapper<survival::Exploration> exploration;
  std::reference_wrapper<survival::Workshop> workshop;
  std::reference_wrapper<survival::Escape> escape;
  std::reference_wrapper<const survival::Companion> companion;
};

/// Drives one survival run: accumulates frame time into fixed 100ms steps,
/// retains input between them, and runs both actors' commands through the
/// rules. The step clock runs in every phase; `Exploration` spends only
/// exploration time from it. No step runs once the party has escaped.
class SurvivalRun {
public:
  SurvivalRun(SurvivalFeatures rules, survival::ControlMap players);
  /// Retains this frame's input and runs every step the frame time completes.
  void advance(double delta, const std::array<survival::PlayerInput, 2> &input);
  /// Whether the run has ended in the escaped phase.
  [[nodiscard]] bool escaped() const;
  /// Who drives each actor, for sampling their keys.
  [[nodiscard]] survival::ControlMap control() const;
  /// Reads one frame's values for the screen.
  [[nodiscard]] hud::SurvivalFrame frame() const;
  /// Reports the screen's values by name for host integration assertions.
  [[nodiscard]] godot::Dictionary status() const;

private:
  [[nodiscard]] survival::GameView view() const;
  /// `Companion`'s command, with a human chef's cooking set to the chosen dish.
  [[nodiscard]] survival::ActorCommand
  command_for(survival::Actor actor, const survival::PlayerInput &input,
              const survival::GameView &current) const;
  void pick_dish(const survival::GameView &current);
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
  /// The chef's direction press since the last step; a held key is one press.
  survival::Direction dish_step = survival::Direction::Idle;
  /// Lasts until another is chosen and applies while it can be cooked.
  std::optional<survival::FoodId> chosen_dish;
  std::deque<hud::Event> events;
};

} // namespace neocity::session
