#pragma once

#include <survival/content.hpp>

#include <optional>
#include <random>
#include <string>
#include <string_view>

namespace neocity::survival {

/// A complete start-of-day checkpoint. Only capture and parsing create one;
/// parsing rejects incompatible content, unknown identities and invalid state.
class DaySave {
public:
  /// Captures at base with fresh daily clocks, after the ending judgment.
  [[nodiscard]] static std::optional<DaySave>
  capture(const Simulation &simulation, const Workshop &workshop,
          const Exploration &exploration, ControlMap control);
  /// Parses the versioned file against the run's current content and balance.
  [[nodiscard]] static std::optional<DaySave> parse(std::string_view bytes,
                                                    const Content &content);
  /// Encodes the complete checkpoint, including the workshop random sequence.
  [[nodiscard]] std::string encode() const;
  /// Restores freshly constructed rules using the content supplied to parse.
  void restore(Simulation &simulation, Workshop &workshop,
               Exploration &exploration) const;
  /// The local roles at the checkpoint.
  [[nodiscard]] ControlMap control() const;
  /// The run seed to use when constructing the restored content.
  [[nodiscard]] std::uint32_t seed() const;

private:
  explicit DaySave(const std::mt19937 &generator) : random(generator) {}
  [[nodiscard]] bool valid(const Content &content) const;

  Snapshot progression;
  WorkshopSnapshot kitchen;
  std::vector<SourceStock> sources;
  std::array<std::uint32_t, 2> touches{};
  std::mt19937 random;
  ControlMap players;
  std::chrono::microseconds exploration_budget{};
  std::chrono::microseconds cook_time{};
};

} // namespace neocity::survival
