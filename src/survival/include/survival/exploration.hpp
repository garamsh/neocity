#pragma once

#include <survival/world.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <vector>

namespace neocity::survival {

/// Temporary four-direction grid coordinates, independent of presentation.
struct GridPosition {
  std::int32_t x = 0;
  std::int32_t y = 0;
  friend bool operator==(const GridPosition &, const GridPosition &) = default;
};

/// What one cell means to an actor: stand on it, stopped by it, or fall.
enum class Cell : std::uint8_t { Supported, Blocked, Falling };

/// Grid steps from a cell to the nearest cell of an area; zero inside it.
[[nodiscard]] std::int64_t distance(GridPosition position, WorldArea area);

/// The one cell rule behind movement, entry and route planning.
[[nodiscard]] Cell cell_at(const WorldSnapshot &snapshot,
                           GridPosition position);

/// The same movement choices apply to both actors.
enum class Direction : std::uint8_t { Idle, Left, Right, Up, Down };

/// Presentation categories share the world's MaterialSource identity.
enum class SourceKind : std::uint8_t { VendingMachine, Npc, Shop, Bag };

/// Caller-supplied finite stock, shared by both actors for the entire run.
struct SourceStock {
  ObjectId id;
  SourceKind kind = SourceKind::Bag;
  std::uint32_t remaining = 1;
  friend bool operator==(const SourceStock &, const SourceStock &) = default;
};

/// Temporary tuning: walking/running move one/two cells per 100ms tick.
/// `collection_distance` also bounds repairs. `collection_time` is the shared
/// exploration time one collection spends, ingredient or tool.
struct ExplorationConfig {
  GridPosition base;
  GridPosition entry;
  std::uint32_t maximum_stamina = 10;
  std::uint32_t collection_distance = 1;
  std::vector<SourceStock> sources;
  std::chrono::microseconds collection_time{};
};

/// Current input only; no movement or collection requests are queued.
struct ExplorationInput {
  Direction direction = Direction::Idle;
  bool running = false;
};

/// Detailed position and tick-based stamina; base has no movement or stamina
/// use.
struct ExplorerSnapshot {
  GridPosition position;
  std::uint32_t stamina = 0;
  friend bool operator==(const ExplorerSnapshot &,
                         const ExplorerSnapshot &) = default;
};

/// Read-only values for a renderer or companion controller.
struct ExplorationSnapshot {
  ExplorerSnapshot chef;
  ExplorerSnapshot appraiser;
  std::vector<SourceStock> sources;
  friend bool operator==(const ExplorationSnapshot &,
                         const ExplorationSnapshot &) = default;
};

/// Collection refusals leave inventories, stock and progression unchanged.
enum class CollectionResult : std::uint8_t {
  Applied,
  NotExploring,
  UnknownSource,
  Unavailable,
  OutOfRange,
  Depleted
};

/// Coordinates existing progression/world commands; owns neither dependency.
/// Route actor entry, return, damage and time through this instance during
/// play.
class Exploration {
public:
  /// Throws std::invalid_argument for invalid stock, tuning or an unsafe entry.
  Exploration(Simulation &progression, World &city, ExplorationConfig settings);
  /// Starts at the configured entry with fresh stamina and no partial movement.
  [[nodiscard]] Result enter(Actor actor);
  /// Synchronizes the detailed base position while preserving run state.
  [[nodiscard]] Result return_to_base(Actor actor);
  /// Applies one health-slot hit and synchronizes any exhaustion return.
  [[nodiscard]] Result damage(Actor actor);
  /// Consumes shared time once, immediately; movement retains sub-tick time.
  /// The world follows the run's clock here and on collection only; acid is
  /// explicit until feeding is wired.
  /// Answers `Result::RunOver` with no change once the run is over.
  [[nodiscard]] Result advance(std::chrono::microseconds elapsed,
                               const std::array<ExplorationInput, 2> &inputs,
                               AcidLevel acid);
  /// Checks identity, supply, distance and shared stock before recording the
  /// source's ingredient or tool, then spends `collection_time` of the shared
  /// clock, or what is left of it: a collection that runs the clock out keeps
  /// its item and returns everyone.
  [[nodiscard]] CollectionResult collect(Actor actor, ObjectId source);
  /// Repairs from within interaction distance with a tool this actor carries;
  /// the tool is kept.
  [[nodiscard]] RepairResult repair(Actor actor, ObjectId target, ToolId tool);
  /// Returns independent values; requests are never retained in this snapshot.
  [[nodiscard]] ExplorationSnapshot snapshot() const;

private:
  struct Explorer {
    ExplorerSnapshot state;
    std::chrono::microseconds remainder{};
  };

  Explorer &explorer(Actor actor);
  void synchronize();
  void tick(Actor actor, ExplorationInput input);
  void check_support();
  void advance_world(AcidLevel acid);

  std::reference_wrapper<Simulation> simulation;
  std::reference_wrapper<World> world;
  ExplorationConfig config;
  Explorer chef;
  Explorer appraiser;
};

} // namespace neocity::survival
