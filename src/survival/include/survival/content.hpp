#pragma once

#include <survival/companion.hpp>

#include <cstdint>
#include <string_view>
#include <vector>

namespace neocity::survival {

/// The playable grid's size in cells; every object lies inside it.
struct MapSize {
  std::int32_t width = 0;
  std::int32_t height = 0;
  friend bool operator==(const MapSize &, const MapSize &) = default;
};

/// The road surface a terrain cell shows while its terrain stands. Rubble
/// marks a cell an intact building blocks; every other kind is drawing only.
enum class GroundKind : std::uint8_t {
  Road,
  CrackedRoad,
  Pothole,
  RoadEndLeft,
  RoadEndRight,
  TiltedSlab,
  Rubble,
  PlankBridge,
  BuriedCar
};

struct GroundCell {
  GridPosition cell;
  GroundKind kind = GroundKind::Road;
};

/// Scenery of the swallowed city; no rule reads it.
enum class DecorationKind : std::uint8_t {
  TiltedTrain,
  BuildingShell,
  HangingCables,
  DeadNeonSign,
  FleshGrowth,
  RubblePile,
  BrokenPillar,
  StreetLamp
};

struct Decoration {
  DecorationKind kind = DecorationKind::RubblePile;
  WorldArea area;
};

/// One run's temporary data: the map, ingredients, recipes, tools and tuning.
/// Every value makes the prototype playable and none is final balance.
struct Content {
  Config run;
  std::vector<ObjectDefinition> objects;
  CompanionConfig companion;
  /// Cells that cost one health slot to step into.
  std::vector<WorldArea> hazards;
  MapSize map;
  /// One entry for each cell any terrain object covers.
  std::vector<GroundCell> ground;
  std::vector<Decoration> decorations;
};

/// Builds the temporary game for one run seed.
[[nodiscard]] Content temporary_content(std::uint32_t seed);

/// Display names of the temporary data; an unknown identity throws
/// std::out_of_range, since only this module issues them.
[[nodiscard]] std::string_view ingredient_name(IngredientId ingredient);
[[nodiscard]] std::string_view food_name(FoodId food);
[[nodiscard]] std::string_view tool_name(ToolId tool);
[[nodiscard]] std::string_view object_name(ObjectId object);

} // namespace neocity::survival
