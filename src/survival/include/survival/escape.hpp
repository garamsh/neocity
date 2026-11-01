#pragma once

#include <survival/simulation.hpp>

#include <cstdint>
#include <functional>
#include <random>

namespace neocity::survival {

/// When the daily escape check runs and how often it hits.
struct EscapeConfig {
  /// The first finished day that is checked.
  std::uint32_t first_day;
  /// A check hits when a roll in 0..99 falls below this value.
  std::uint32_t chance_percent;
};

/// Temporary values that make the ending reachable, not final balance.
inline constexpr EscapeConfig TEMPORARY_ESCAPE{.first_day = 30,
                                               .chance_percent = 20};

/// `rolls` counts the random draws consumed so far.
struct EscapeSnapshot {
  std::uint32_t checked_day = 0;
  std::uint64_t rolls = 0;
  friend bool operator==(const EscapeSnapshot &,
                         const EscapeSnapshot &) = default;
};

/// NotDue means no draw was consumed.
enum class EscapeResult : std::uint8_t { NotDue, Stayed, Escaped };

/// Owns the daily escape check and a random stream of its own, derived from the
/// run seed so it never repeats the workshop's. The supplied Simulation must
/// outlive this instance.
class Escape {
public:
  Escape(Simulation &progression, EscapeConfig settings);
  /// In the workshop, rolls once for the latest finished day and escapes on a
  /// hit. Repeat calls on one day draw nothing, so a caller may check after
  /// every command.
  [[nodiscard]] EscapeResult check();
  /// Returns independent values.
  [[nodiscard]] EscapeSnapshot snapshot() const;

private:
  std::reference_wrapper<Simulation> simulation;
  EscapeConfig config;
  EscapeSnapshot state;
  std::mt19937 generator;
};

} // namespace neocity::survival
