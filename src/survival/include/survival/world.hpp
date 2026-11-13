#pragma once

#include <survival/simulation.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace neocity::survival {

/// Stable caller-assigned identity, retained through destruction and exposure.
struct ObjectId {
  std::uint32_t value = 0;
  friend bool operator==(const ObjectId &, const ObjectId &) = default;
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

/// The two playable maps. Each has its own cell grid, with its origin at its
/// own top left corner.
enum class MapId : std::uint8_t { Street, Aquarium };

/// How far a map has fallen apart. The stage depends on the day alone.
enum class DamageStage : std::uint8_t { Standing, Cracked, Ruined };

/// The first day of each damage stage, in stage order.
inline constexpr std::array<std::uint32_t, 3> STAGE_FIRST_DAY{1, 5, 10};

/// The stage `day` falls in; day 0 counts as the first.
[[nodiscard]] DamageStage damage_stage(std::uint32_t day);

/// Concrete object configuration. From `exposed_from_day` on, a dormant object
/// is intact; from `destroyed_from_day` on, it is destroyed. A material source
/// supplies exactly one ingredient.
struct ObjectDefinition {
  ObjectId id{};
  ObjectKind kind = ObjectKind::Terrain;
  WorldArea area{};
  MapId map = MapId::Street;
  std::optional<std::uint32_t> exposed_from_day = std::nullopt;
  std::optional<std::uint32_t> destroyed_from_day = std::nullopt;
  std::optional<IngredientId> ingredient = std::nullopt;
  std::optional<ObjectId> supported_by = std::nullopt;
};

/// Derived access and supply values prevent consumers from duplicating rules.
struct ObjectSnapshot {
  ObjectId id;
  ObjectKind kind = ObjectKind::Terrain;
  WorldArea area;
  MapId map = MapId::Street;
  ObjectState state = ObjectState::Intact;
  bool traversable = false;
  bool material_available = false;
  std::optional<IngredientId> ingredient;
  friend bool operator==(const ObjectSnapshot &,
                         const ObjectSnapshot &) = default;
};

/// Value snapshot of every map's objects on the world's day.
struct WorldSnapshot {
  std::uint32_t day = 1;
  std::vector<ObjectSnapshot> objects;
  friend bool operator==(const WorldSnapshot &,
                         const WorldSnapshot &) = default;
};

/// The snapshot with only the objects of `map`, so a `CellMap` built from it
/// holds that map alone.
[[nodiscard]] WorldSnapshot on_map(const WorldSnapshot &snapshot, MapId map);

/// A day before the current one is refused without changing any world state.
enum class WorldUpdateResult : std::uint8_t { Applied, DecreasingDay };

/// Persistent city deterioration, without exploration implementation.
class World {
public:
  /// Throws std::invalid_argument for invalid geometry, IDs or object
  /// contracts.
  explicit World(const std::vector<ObjectDefinition> &definitions);
  /// Moves the world to `day`; damage follows the day alone, so acid never
  /// changes the map.
  [[nodiscard]] WorldUpdateResult update(std::uint32_t next);
  /// Returns stable identities, areas, visibility, access and material supply.
  [[nodiscard]] WorldSnapshot snapshot() const;

private:
  struct Object {
    ObjectDefinition definition;
    ObjectState state = ObjectState::Intact;
  };

  std::vector<Object> objects;
  std::uint32_t day = 1;
};

} // namespace neocity::survival
