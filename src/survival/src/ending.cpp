#include <survival/ending.hpp>

#include <array>
#include <cstdlib>

namespace neocity::survival {
namespace {

using Judgment = bool (*)(const WorkshopSnapshot &, const EndingConfig &);

struct Rule {
  Ending ending;
  Judgment reached;
};

bool monster_satisfied(const WorkshopSnapshot &workshop,
                       const EndingConfig &config) {
  return workshop.satisfied_days >= config.satisfied_days;
}

/// A new ending adds its kind and its judgment here.
constexpr std::array RULES{
    Rule{.ending = Ending::MonsterSatisfied, .reached = monster_satisfied}};

} // namespace

Endings::Endings(Simulation &progression, const Workshop &kitchen,
                 EndingConfig settings)
    : simulation(progression), workshop(kitchen), config(settings) {}

std::optional<Ending> Endings::check() {
  if (simulation.get().snapshot().phase != Phase::Workshop) {
    return std::nullopt;
  }
  const auto base = workshop.get().snapshot();
  for (const auto &rule : RULES) {
    if (rule.reached(base, config)) {
      if (simulation.get().end(rule.ending) != Result::Applied) {
        std::abort(); // unreachable: the workshop phase was read above
      }
      return rule.ending;
    }
  }
  return std::nullopt;
}

} // namespace neocity::survival
