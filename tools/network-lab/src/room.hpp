#pragma once

#include <survival/content.hpp>
#include <survival/escape.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

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
  /// Runs one step; an escaped run changes nothing, as no step runs after it.
  void step(const StepInput &input);
  [[nodiscard]] survival::GameView view() const;
  [[nodiscard]] survival::EscapeSnapshot escape() const;
  [[nodiscard]] const survival::CompanionConfig &companion_config() const;

private:
  survival::Content data;
  survival::Simulation shared;
  survival::World map;
  survival::Exploration explorers;
  survival::Workshop kitchen;
  survival::Escape escape_check;
  survival::Companion controllers;
};

/// Compares every value of a view.
[[nodiscard]] bool same_view(const survival::GameView &left,
                             const survival::GameView &right);

/// Items an actor carries, which no departure may take away.
[[nodiscard]] std::size_t carried(const survival::GameView &view,
                                  survival::Actor actor);

/// Whether an actor itself moved or changed what it carries, which a day's
/// rollover alone does not do.
[[nodiscard]] bool actor_progressed(const survival::GameView &before,
                                    const survival::GameView &after,
                                    survival::Actor actor);

/// The inputs of every step since tick 1, or the knowledge that one is missing.
class StepLog {
public:
  /// Takes the inputs of `tick`; a tick out of order marks the log incomplete.
  void record(std::uint64_t tick, const StepInput &input);
  [[nodiscard]] bool complete() const { return unbroken; }
  [[nodiscard]] std::span<const StepInput> steps() const { return entries; }

private:
  std::vector<StepInput> entries;
  bool unbroken = true;
};

/// Rebuilds a room from its seed and log; refuses a log with a step missing,
/// which would reach a different state.
[[nodiscard]] std::unique_ptr<Room> replay(const StepLog &log,
                                           std::uint32_t seed);

} // namespace neocity::lab
