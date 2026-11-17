#pragma once

#include <survival/simulation.hpp>
#include <survival/workshop.hpp>

#include <cstdint>
#include <functional>
#include <optional>

namespace neocity::survival {

/// The values the endings are judged by.
struct EndingConfig {
  /// How many days fed to satisfaction, in a row or not, end the run as
  /// MonsterSatisfied.
  std::uint32_t satisfied_days;
  /// The completed day after which the monster vomits the party out.
  std::uint32_t vomited_out_day = 30;
};

/// Temporary satisfaction balance; the 30-day limit is the chosen rule.
inline constexpr EndingConfig TEMPORARY_ENDING{.satisfied_days = 7};

/// Owns the list of endings, each a kind and a judgment, and ends the run with
/// the first whose judgment holds. The supplied Simulation and Workshop must
/// outlive this instance.
class Endings {
public:
  Endings(Simulation &progression, const Workshop &kitchen,
          EndingConfig settings);
  /// In the workshop, ends the run with the first ending reached and returns
  /// it. Judges nothing once the run is over, so a lost run stays lost, and
  /// repeat calls change nothing, so a caller may check after every command.
  [[nodiscard]] std::optional<Ending> check();

private:
  std::reference_wrapper<Simulation> simulation;
  std::reference_wrapper<const Workshop> workshop;
  EndingConfig config;
};

} // namespace neocity::survival
