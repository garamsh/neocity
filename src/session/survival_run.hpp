#pragma once

#include "../hud/survival.hpp"

#include <survival/companion.hpp>
#include <survival/content.hpp>
#include <survival/escape.hpp>

#include <godot_cpp/variant/dictionary.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <optional>
#include <vector>

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
/// exploration time from it. No step runs once the run is over.
class SurvivalRun {
public:
  /// `first_event` numbers the run's first logged event; the screen tells
  /// notes apart by these numbers, so each run continues the last one's.
  SurvivalRun(SurvivalFeatures rules, survival::ControlMap players,
              std::uint64_t first_event);
  /// Retains this frame's input and runs every step the frame time completes;
  /// returns the events those steps recorded, oldest first.
  std::vector<hud::Event>
  advance(double delta, const std::array<survival::PlayerInput, 2> &input,
          survival::Direction dish_press);
  /// Forgets every key press not yet stepped, keeping the frame time.
  void drop_input();
  /// Whether the run has ended in the escaped phase.
  [[nodiscard]] bool escaped() const;
  /// Whether the run has ended in the lost phase.
  [[nodiscard]] bool lost() const;
  /// Who drives each actor, for sampling their keys.
  [[nodiscard]] survival::ControlMap control() const;
  /// Reads one frame's values for the screen.
  [[nodiscard]] hud::SurvivalFrame frame() const;
  /// Reads every rule object's snapshot.
  [[nodiscard]] survival::GameView view() const;
  /// How full the acid is, from 0 to 1 at the rules' full value.
  [[nodiscard]] float acid_fill() const;
  /// Reports the screen's values in `current`, this run's latest `frame()`,
  /// by name for host integration assertions.
  [[nodiscard]] godot::Dictionary
  status(const hud::SurvivalFrame &current) const;
  /// The number the next logged event takes.
  [[nodiscard]] std::uint64_t next_event() const;

private:
  [[nodiscard]] bool over() const;
  /// `Companion`'s command, with a human chef's cooking set to the chosen dish.
  [[nodiscard]] survival::ActorCommand
  command_for(survival::Actor actor, const survival::PlayerInput &input,
              const survival::GameView &current) const;
  void pick_dish(const survival::GameView &current);
  void step();
  void run_action(survival::Actor actor, const survival::Action &action,
                  std::uint32_t day);
  void report_returns(const survival::Snapshot &before,
                      const std::array<bool, 2> &walked_home);
  void report_world_changes(const survival::WorldSnapshot &before);
  void record(const hud::Event &event);

  SurvivalFeatures features;
  survival::ControlMap controllers;
  bool started_together = false;
  double accumulated = 0.0;
  std::array<survival::PlayerInput, 2> pending{};
  std::array<survival::ExplorationInput, 2> held{};
  /// Game time left before each actor's interact edge acts again; a held run
  /// takes no step, so it does not run down.
  std::array<std::chrono::milliseconds, 2> interact_hold{};
  /// The chef's up or down press since the last step.
  survival::Direction dish_step = survival::Direction::Idle;
  /// Lasts for the day and applies while it can be cooked.
  std::optional<survival::FoodId> chosen_dish;
  /// City objects changed since the last finished day.
  std::uint32_t map_changes = 0;
  std::uint32_t last_day_map_changes = 0;
  std::deque<hud::Event> events;
  /// The number of the log's oldest event.
  std::uint64_t first_logged = 0;
  /// The events recorded since the current `advance` began.
  std::vector<hud::Event> fresh;
};

} // namespace neocity::session
