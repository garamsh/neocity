#pragma once

#include <survival/simulation.hpp>

#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

namespace neocity::survival {

/// Stable caller-assigned identity, retained through destruction and exposure.
struct ObjectId {
  std::uint32_t value = 0;
  friend bool operator==(const ObjectId &, const ObjectId &) = default;
};

/// Tool identity; the caller proves possession before requesting a repair.
struct ToolId {
  std::uint32_t value = 0;
  friend bool operator==(const ToolId &, const ToolId &) = default;
};

/// Caller-defined acid units; no production rise or feeding formula is implied.
struct AcidLevel {
  std::uint32_t value = 0;
  friend bool operator==(const AcidLevel &, const AcidLevel &) = default;
};

/// Axis-aligned area in integer world units, independent of a renderer.
struct WorldArea {
  std::int32_t x = 0;
  std::int32_t y = 0;
  std::int32_t width = 1;
  std::int32_t height = 1;
  friend bool operator==(const WorldArea &, const WorldArea &) = default;
};

/// Concrete city roles exposed to exploration and presentation.
enum class ObjectKind : std::uint8_t {
  Npc,
  Terrain,
  Building,
  Ladder,
  Entrance,
  MaterialSource
};

/// Dormant objects retain their IDs before becoming visible.
enum class ObjectState : std::uint8_t { Dormant, Intact, Destroyed };

/// Either enabled threshold triggers; supplied thresholds must be positive.
struct WorldThreshold {
  std::optional<std::chrono::microseconds> elapsed = std::nullopt;
  std::optional<AcidLevel> acid = std::nullopt;
};

/// Concrete object configuration; destruction measures from run start or
/// repair.
struct ObjectDefinition {
  ObjectId id{};
  ObjectKind kind = ObjectKind::Terrain;
  WorldArea area{};
  std::optional<WorldThreshold> exposed_at = std::nullopt;
  std::optional<WorldThreshold> destroyed_after = std::nullopt;
  std::optional<ToolId> repair_with = std::nullopt;
  std::optional<IngredientId> ingredient = std::nullopt;
  std::optional<ObjectId> supported_by = std::nullopt;
};

/// Cumulative exploration time and current acid, supplied explicitly by the
/// host.
struct WorldProgression {
  std::chrono::microseconds elapsed{};
  AcidLevel acid{};
  friend bool operator==(const WorldProgression &,
                         const WorldProgression &) = default;
};

/// Derived access and supply values prevent consumers from duplicating rules.
struct ObjectSnapshot {
  ObjectId id;
  ObjectKind kind = ObjectKind::Terrain;
  WorldArea area;
  ObjectState state = ObjectState::Intact;
  bool traversable = false;
  bool material_available = false;
  std::optional<IngredientId> ingredient;
  std::optional<ToolId> repair_with;
  friend bool operator==(const ObjectSnapshot &,
                         const ObjectSnapshot &) = default;
};

/// Value snapshot; cumulative time and historical peak acid never rewind.
struct WorldSnapshot {
  WorldProgression progression;
  AcidLevel peak_acid;
  std::vector<ObjectSnapshot> objects;
  friend bool operator==(const WorldSnapshot &,
                         const WorldSnapshot &) = default;
};

/// Decreasing cumulative time is refused without changing any world state.
enum class WorldUpdateResult : std::uint8_t { Applied, DecreasingTime };

/// Expected repair refusals never change the world.
enum class RepairResult : std::uint8_t {
  Applied,
  UnknownObject,
  NotDamaged,
  NotRepairable,
  WrongTool
};

/// Persistent city deterioration and repair, without exploration
/// implementation.
class World {
public:
  /// Throws std::invalid_argument for invalid geometry, IDs or object
  /// contracts.
  explicit World(const std::vector<ObjectDefinition> &definitions);
  /// Applies cumulative exploration time and peak-acid deterioration once.
  [[nodiscard]] WorldUpdateResult update(WorldProgression progression);
  /// Repairs a destroyed passage and starts its next deterioration interval.
  [[nodiscard]] RepairResult repair(ObjectId id, ToolId tool);
  /// Returns stable identities, areas, visibility, access and material supply.
  [[nodiscard]] WorldSnapshot snapshot() const;

private:
  struct Object {
    ObjectDefinition definition;
    ObjectState state = ObjectState::Intact;
    WorldProgression repaired_at;
  };

  std::vector<Object> objects;
  WorldProgression current;
  AcidLevel peak_acid;
};

} // namespace neocity::survival
