#pragma once

#include <survival/companion.hpp>

#include <cstdint>
#include <string>
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

/// The ground kind one terrain cell shows.
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

/// One piece of scenery and the area it covers.
struct Decoration {
  DecorationKind kind = DecorationKind::RubblePile;
  WorldArea area;
};

/// A picture of its own that one building or terrain object shows; no rule
/// reads it.
enum class LandmarkKind : std::uint8_t {
  Kiosk,
  ColdStore,
  Ferry,
  Platform,
  Bridge
};

/// The object that shows one landmark picture.
struct Landmark {
  ObjectId object;
  LandmarkKind kind = LandmarkKind::Kiosk;
};

/// One map's drawing data: its size, hazards, ground, scenery and landmark
/// pictures. The objects of every map are in `Content::objects`, each tagged
/// with its map.
struct MapContent {
  MapId id = MapId::Street;
  std::string_view name;
  /// The playable grid's size in cells; every object of the map lies inside.
  MapSize size;
  /// Cells where acid drips are drawn; no rule reads them.
  std::vector<WorldArea> hazards;
  /// One entry for each cell any terrain object of the map covers.
  std::vector<GroundCell> ground;
  /// Scenery placed on the map; no rule reads it.
  std::vector<Decoration> decorations;
  /// Objects drawn with a picture of their own; no rule reads it.
  std::vector<Landmark> landmarks;
};

/// One run's temporary data: the maps, ingredients, recipes and tuning.
/// Every value makes the prototype playable and none is final balance. The
/// maps are fixed: the seed moves neither a cell nor a supply.
struct Content {
  Config run;
  std::vector<ObjectDefinition> objects;
  CompanionConfig companion;
  /// The street first, then the aquarium.
  std::vector<MapContent> maps;
};

/// The data of `id`; throws std::out_of_range for a map the content lacks.
[[nodiscard]] const MapContent &map_of(const Content &content, MapId id);

/// Builds the temporary game for one run seed.
[[nodiscard]] Content temporary_content(std::uint32_t seed);

/// Display names of the temporary data; an unknown identity throws
/// std::out_of_range, since only this module issues them.
[[nodiscard]] std::string_view ingredient_name(IngredientId ingredient);
[[nodiscard]] std::string_view food_name(FoodId food);
[[nodiscard]] std::string_view object_name(ObjectId object);

/// The one name a city object has on every map, so the map, the goal line,
/// the action button and the log agree: a source by what it holds and what it
/// is, "Fish cart", anything else by `object_name`. It reads the objects of
/// every map, never only the current one.
[[nodiscard]] std::string place_display_name(const GameView &view,
                                             ObjectId object);

} // namespace neocity::survival
