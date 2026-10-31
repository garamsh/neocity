#pragma once

#include <chrono>
#include <cstdint>
#include <vector>

namespace neocity::survival {

/// The two independently located members of the exploring party.
enum class Actor : std::uint8_t { Chef, Appraiser };

/// Workshop includes the base and untimed appraisal, cooking and feeding.
enum class Phase : std::uint8_t { Workshop, Exploration, Escaped };

/// Exhaustion relocates one actor without ending the other actor's exploration.
enum class Location : std::uint8_t { Base, City };

/// Ingredient identity shared with the future collection and workshop rules.
struct IngredientId {
  std::uint32_t value = 0;
  friend bool operator==(const IngredientId &, const IngredientId &) = default;
};

/// Exploration health and carried ingredients; workshop has no separate health.
struct ActorSnapshot {
  Location location = Location::Base;
  std::uint8_t health = 3;
  std::vector<IngredientId> inventory;
  friend bool operator==(const ActorSnapshot &,
                         const ActorSnapshot &) = default;
};

/// Caller-supplied exploration budget and seed, with no production balance
/// default.
struct Config {
  std::chrono::microseconds exploration_budget;
  std::uint32_t seed;
};

/// Value snapshot of one run; consumers cannot mutate the simulation through
/// it.
struct Snapshot {
  ActorSnapshot chef;
  ActorSnapshot appraiser;
  Phase phase = Phase::Workshop;
  std::uint32_t day = 1;
  std::chrono::microseconds remaining_exploration{};
  std::uint32_t acid = 0;
  std::uint32_t seed = 0;
  friend bool operator==(const Snapshot &, const Snapshot &) = default;
};

/// Expected command refusals leave the entire snapshot unchanged.
enum class Result : std::uint8_t {
  Applied,
  NotExploring,
  NoExplorationTime,
  InvalidElapsed
};

/// Independent survival progression with explicitly supplied simulation time.
class Simulation {
public:
  /// Starts day one at base; throws std::invalid_argument for a nonpositive
  /// budget.
  explicit Simulation(Config config);
  /// Enters or re-enters the city using the existing time, inventory and
  /// health.
  [[nodiscard]] Result enter(Actor actor);
  /// Returns one actor at full health; the last return opens the untimed
  /// workshop.
  [[nodiscard]] Result return_to_base(Actor actor);
  /// Advances only exploration; expiry returns both actors to the workshop.
  [[nodiscard]] Result advance(std::chrono::microseconds elapsed);
  /// Removes one health slot; exhaustion returns the actor at full health.
  [[nodiscard]] Result damage(Actor actor);
  /// Records collection authorized by exploration rules; no world lookup occurs
  /// here.
  [[nodiscard]] Result collect(Actor actor, IngredientId ingredient);
  /// Returns the complete current state, including the supplied replay seed.
  [[nodiscard]] Snapshot snapshot() const;

private:
  ActorSnapshot &actor_state(Actor actor);
  void relocate(ActorSnapshot &actor);

  Snapshot world;
};

} // namespace neocity::survival
