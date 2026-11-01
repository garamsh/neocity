#include <survival/escape.hpp>

#include <cstdint>
#include <cstdlib>

namespace neocity::survival {
namespace {

/// Distinguishes this stream from any other seeded from the run seed.
constexpr std::uint32_t ESCAPE_STREAM = 0x45534350U;

std::mt19937 escape_stream(std::uint32_t seed) {
  std::seed_seq sequence{seed, ESCAPE_STREAM};
  return std::mt19937(sequence);
}

} // namespace

Escape::Escape(Simulation &progression, EscapeConfig settings)
    : simulation(progression), config(settings),
      generator(escape_stream(progression.snapshot().seed)) {}

EscapeResult Escape::check() {
  const auto now = simulation.get().snapshot();
  const auto finished_day = now.day - 1;
  if (now.phase != Phase::Workshop || finished_day < config.first_day ||
      finished_day == state.checked_day) {
    return EscapeResult::NotDue;
  }
  state.checked_day = finished_day;
  ++state.rolls;
  if (generator() % 100U >= config.chance_percent) {
    return EscapeResult::Stayed;
  }
  if (simulation.get().escape() != Result::Applied) {
    std::abort(); // unreachable: the workshop phase was read above
  }
  return EscapeResult::Escaped;
}

EscapeSnapshot Escape::snapshot() const { return state; }

} // namespace neocity::survival
