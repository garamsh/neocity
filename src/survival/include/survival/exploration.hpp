#pragma once

#include <survival/world.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <vector>

namespace neocity::survival {

/// Temporary four-direction grid coordinates, independent of presentation.
struct GridPosition {
  std::int32_t x = 0;
  std::int32_t y = 0;
  friend bool operator==(const GridPosition &, const GridPosition &) = default;
};

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
struct ExplorationConfig {
  GridPosition base;
  GridPosition entry;
  std::uint32_t maximum_stamina = 10;
  std::uint32_t collection_distance = 1;
  std::vector<SourceStock> sources;
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
  Exploration(Simulation &simulation, World &world, ExplorationConfig config);
  /// Starts at the configured entry with fresh stamina and no partial movement.
  [[nodiscard]] Result enter(Actor actor);
  /// Synchronizes the detailed base position while preserving run state.
  [[nodiscard]] Result return_to_base(Actor actor);
  /// Applies one health-slot hit and synchronizes any exhaustion return.
  [[nodiscard]] Result damage(Actor actor);
  /// Consumes shared time once, immediately; movement retains sub-tick time.
  /// World time is advanced here only; acid is explicit until feeding is wired.
  [[nodiscard]] Result advance(std::chrono::microseconds elapsed,
                               const std::array<ExplorationInput, 2> &inputs,
                               AcidLevel acid);
  /// Checks identity, supply, distance and shared stock before recording
  /// material.
  [[nodiscard]] CollectionResult collect(Actor actor, ObjectId source);
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

  Simulation &simulation;
  World &world;
  ExplorationConfig config;
  Explorer chef;
  Explorer appraiser;
};

} // namespace neocity::survival
