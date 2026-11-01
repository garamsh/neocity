#pragma once

#include <chrono>
#include <cstdint>
#include <span>
#include <vector>

namespace neocity::survival {

/// The two independently located members of the exploring party.
enum class Actor : std::uint8_t { Chef, Appraiser };

/// Workshop includes the base and untimed appraisal, cooking and feeding.
enum class Phase : std::uint8_t { Workshop, Exploration, Escaped };

/// Exhaustion relocates one actor without ending the other actor's exploration.
enum class Location : std::uint8_t { Base, City };

/// Ingredient identity shared with the collection and workshop rules.
struct IngredientId {
  std::uint32_t value = 0;
  friend bool operator==(const IngredientId &, const IngredientId &) = default;
};

/// Tool identity, never an ingredient; what a tool does is set by its consumer.
struct ToolId {
  std::uint32_t value = 0;
  friend bool operator==(const ToolId &, const ToolId &) = default;
};

/// Caller-defined acid units; no production rise or feeding formula is implied.
struct AcidLevel {
  std::uint32_t value = 0;
  friend bool operator==(const AcidLevel &, const AcidLevel &) = default;
};

/// Exploration health and carried items; workshop has no separate health.
/// `exhaustions` counts the times this actor's health ran out.
struct ActorSnapshot {
  Location location = Location::Base;
  std::uint8_t health = 3;
  std::vector<IngredientId> inventory;
  std::vector<ToolId> tools;
  std::uint32_t exhaustions = 0;
  friend bool operator==(const ActorSnapshot &,
                         const ActorSnapshot &) = default;
};

/// Caller-supplied daily exploration budget and seed, with no production
/// balance default.
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
  AcidLevel acid;
  std::uint32_t seed = 0;
  friend bool operator==(const Snapshot &, const Snapshot &) = default;
};

/// Expected command refusals leave the entire snapshot unchanged.
enum class Result : std::uint8_t {
  Applied,
  NotExploring,
  NoExplorationTime,
  InvalidElapsed,
  NotInWorkshop,
  MissingIngredients,
  MissingTool,
  Escaped
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
  /// Records a tool pickup authorized by exploration rules.
  [[nodiscard]] Result collect_tool(Actor actor, ToolId tool);
  /// Removes every listed ingredient from the two inventories, the chef's
  /// first, or removes none.
  [[nodiscard]] Result consume(std::span<const IngredientId> ingredients);
  /// Removes one tool from the two inventories, the chef's first.
  [[nodiscard]] Result consume_tool(ToolId tool);
  /// Raises acid, saturating at its limit, and starts the next day with a full
  /// exploration budget.
  [[nodiscard]] Result finish_day(AcidLevel acid_rise);
  /// Ends the run from the workshop; every later command leaves the snapshot
  /// unchanged.
  [[nodiscard]] Result escape();
  /// Returns the complete current state, including the supplied replay seed.
  [[nodiscard]] Snapshot snapshot() const;

private:
  ActorSnapshot &actor_state(Actor actor);
  void relocate(ActorSnapshot &actor);

  Snapshot world;
  std::chrono::microseconds daily_budget;
};

} // namespace neocity::survival
