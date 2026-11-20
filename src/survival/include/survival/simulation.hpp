#pragma once

#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

namespace neocity::survival {

/// The two independently located members of the exploring party.
enum class Actor : std::uint8_t { Chef, Appraiser };

/// How a run can end well; each is judged in `ending.hpp`.
enum class Ending : std::uint8_t { MonsterSatisfied, VomitedOut };

/// Workshop includes the base and untimed appraisal, cooking and feeding.
/// Ended and Lost each end the run.
enum class Phase : std::uint8_t { Workshop, Exploration, Ended, Lost };

/// Whether `phase` has ended the run, so that no command changes state.
[[nodiscard]] constexpr bool run_over(Phase phase) {
  return phase == Phase::Ended || phase == Phase::Lost;
}

/// An actor is at base or in the city; the other actor's location is separate.
enum class Location : std::uint8_t { Base, City };

/// Ingredient identity shared with the collection and workshop rules.
struct IngredientId {
  std::uint32_t value = 0;
  friend bool operator==(const IngredientId &, const IngredientId &) = default;
};

/// Caller-defined acid units; no production rise or feeding formula is implied.
struct AcidLevel {
  std::uint32_t value = 0;
  friend bool operator==(const AcidLevel &, const AcidLevel &) = default;
};

/// One actor's place and carried ingredients, one per slot.
struct ActorSnapshot {
  Location location = Location::Base;
  std::vector<IngredientId> inventory;
  friend bool operator==(const ActorSnapshot &,
                         const ActorSnapshot &) = default;
};

/// Caller-supplied daily exploration budget and seed, with no production
/// balance default. Acid reaching `full_acid` loses the run; left out, only the
/// storage limit does. `inventory_slots` is each actor's slots and
/// `fridge_slots` the base fridge's; left out, only the storage limit holds.
struct Config {
  std::chrono::microseconds exploration_budget;
  std::uint32_t seed;
  AcidLevel full_acid{std::numeric_limits<std::uint32_t>::max()};
  std::uint32_t inventory_slots = std::numeric_limits<std::uint32_t>::max();
  std::uint32_t fridge_slots = std::numeric_limits<std::uint32_t>::max();
};

/// Value snapshot of one run; consumers cannot mutate the simulation through
/// it. `fridge` is the base fridge's ingredients, one per slot, with no
/// spoilage; `inventory_slots` and `fridge_slots` are the configured slot
/// counts, for a consumer that fills slots. `ending` is set exactly when the
/// phase is Ended. `world_time` is the run's clock:
/// the exploration time spent today plus a full budget for every finished day,
/// spent or not.
struct Snapshot {
  ActorSnapshot chef;
  ActorSnapshot appraiser;
  std::vector<IngredientId> fridge;
  std::uint32_t inventory_slots = 0;
  std::uint32_t fridge_slots = 0;
  Phase phase = Phase::Workshop;
  std::optional<Ending> ending;
  std::uint32_t day = 1;
  std::chrono::microseconds remaining_exploration{};
  AcidLevel acid;
  std::uint32_t seed = 0;
  std::chrono::microseconds world_time{};
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
  RunOver,
  Busy,
  Full
};

/// Independent survival progression with explicitly supplied simulation time.
class Simulation {
public:
  /// Starts day one at base; throws std::invalid_argument for a nonpositive
  /// budget or full acid.
  explicit Simulation(Config config);
  /// Enters or re-enters the city using the existing time and inventory.
  [[nodiscard]] Result enter(Actor actor);
  /// Returns one actor to base; the last return opens the untimed workshop.
  [[nodiscard]] Result return_to_base(Actor actor);
  /// Advances only exploration; expiry returns both actors to the workshop.
  [[nodiscard]] Result advance(std::chrono::microseconds elapsed);
  /// Records collection authorized by exploration rules; no world lookup occurs
  /// here. `Result::Full` when the actor has no free slot.
  [[nodiscard]] Result collect(Actor actor, IngredientId ingredient);
  /// Takes one `ingredient` out of a city actor's inventory, for putting it
  /// back where it came from; `Result::MissingIngredients` if not carried.
  [[nodiscard]] Result drop(Actor actor, IngredientId ingredient);
  /// Moves one `ingredient` from a base actor's inventory to the fridge:
  /// `Result::NotInWorkshop` away from base, `Result::MissingIngredients` if
  /// not carried, `Result::Full` with no free fridge slot.
  [[nodiscard]] Result store(Actor actor, IngredientId ingredient);
  /// Moves one `ingredient` from the fridge to a base actor's inventory, with
  /// the same refusals; `Result::Full` with no free inventory slot.
  [[nodiscard]] Result withdraw(Actor actor, IngredientId ingredient);
  /// Removes every listed ingredient from the two inventories, the chef's
  /// first, then the fridge, or removes none.
  [[nodiscard]] Result consume(std::span<const IngredientId> ingredients);
  /// Raises acid by `acid_rise` and lowers it by `acid_drop`, stopping at full
  /// and at zero, passes the day's unspent time on the run's clock, and starts
  /// the next day with a full exploration budget. Full acid ends the run as
  /// lost, before anything can act on the new day.
  [[nodiscard]] Result finish_day(AcidLevel acid_rise,
                                  AcidLevel acid_drop = {});
  /// Ends the run with `ending` from the workshop; every later command leaves
  /// the snapshot unchanged.
  [[nodiscard]] Result end(Ending ending);
  /// Returns the complete current state, including the supplied replay seed.
  [[nodiscard]] Snapshot snapshot() const;

private:
  friend class DaySave;
  ActorSnapshot &actor_state(Actor actor);
  void relocate(ActorSnapshot &actor);

  Snapshot world;
  std::chrono::microseconds daily_budget;
  AcidLevel full_acid;
};

} // namespace neocity::survival
