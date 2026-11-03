#include <survival/content.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace neocity::survival {
namespace {

using namespace std::chrono_literals;

constexpr IngredientId TOMATO{1};
constexpr IngredientId FISH{2};
constexpr IngredientId KELP{3};
constexpr IngredientId MUSHROOM{4};
constexpr IngredientId PICKLES{5};
constexpr IngredientId MEAT{6};
constexpr IngredientId TRUFFLE{7};

constexpr ToolId MAGNIFIER{1};
constexpr ToolId SPATULA{2};
constexpr ToolId WRENCH{3};

constexpr FoodId MUSHROOM_SOUP{1};
constexpr FoodId FISH_STEW{2};
constexpr FoodId KELP_WRAP{3};
constexpr FoodId MEAT_SKEWER{4};
constexpr FoodId TRUFFLE_ROAST{5};
constexpr FoodId PICKLE_SURPRISE{6};
constexpr FoodId SEA_SALAD{7};
constexpr FoodId FISH_PIE{8};
constexpr FoodId MEAT_STEW{9};
constexpr FoodId TRUFFLE_SOUP{10};
constexpr FoodId SURF_AND_TURF{11};

constexpr ObjectId BASE_DOCK{1};
constexpr ObjectId MAIN_STREET{2};
constexpr ObjectId ROTTEN_BRIDGE{3};
constexpr ObjectId EAST_PLAZA{4};
constexpr ObjectId NORTH_LADDER{5};
constexpr ObjectId ARCADE{6};
constexpr ObjectId SOUTH_STAIRS{7};
constexpr ObjectId MARKET{8};
constexpr ObjectId ROOFTOP{9};
constexpr ObjectId DRIFTED_FERRY{10};
constexpr ObjectId ROOF_LADDER{11};
constexpr ObjectId COLD_STORE_DOOR{12};
constexpr ObjectId CORNER_SHOP{13};
constexpr ObjectId COLD_STORE{14};
constexpr ObjectId KIOSK_WRECK{15};
constexpr ObjectId NOODLE_VENDOR{16};
constexpr ObjectId TOMATO_MACHINE{21};
constexpr ObjectId VENDOR_CART{22};
constexpr ObjectId SHOP_SHELF{23};
constexpr ObjectId LOST_BASKET{24};
constexpr ObjectId TOOL_BAG{25};
constexpr ObjectId PICKLE_MACHINE{26};
constexpr ObjectId ARCADE_BAG{27};
constexpr ObjectId SUPPLY_CRATE{28};
constexpr ObjectId COLD_LOCKER{29};
constexpr ObjectId ROOF_BAG{30};
constexpr ObjectId FERRY_BAG{31};
constexpr ObjectId NEON_LEDGE{51};
constexpr ObjectId CRACKED_RAMP{52};
constexpr ObjectId OVERPASS_STUB{53};
constexpr ObjectId FALLEN_SLAB{54};
constexpr ObjectId EAST_AVENUE{55};
constexpr ObjectId SUNKEN_ASPHALT{56};
constexpr ObjectId COLD_STORE_FLOOR{57};
constexpr ObjectId MARKET_SHALLOWS{58};
constexpr ObjectId TILTED_CAR{59};
constexpr ObjectId PIER{60};
constexpr ObjectId PLAZA_LADDER{61};
constexpr ObjectId WEST_RUBBLE{62};
constexpr ObjectId EAST_RUBBLE{63};
constexpr ObjectId UPPER_RAMP{64};
constexpr ObjectId MARKET_LADDER{65};

/// One exploration day, which every world time threshold counts in.
constexpr auto DAY = 18s;
constexpr std::uint32_t BAG_STOCK = 2;

/// Landings on the low west, the market and the shallows, which acid drowns
/// first.
constexpr std::array LOW_WEST{
    GridPosition{.x = 16, .y = 13}, GridPosition{.x = 1, .y = 12},
    GridPosition{.x = 17, .y = 13}, GridPosition{.x = 14, .y = 13},
    GridPosition{.x = 10, .y = 11}, GridPosition{.x = 13, .y = 12},
    GridPosition{.x = 9, .y = 11},  GridPosition{.x = 8, .y = 11},
    GridPosition{.x = 3, .y = 12},  GridPosition{.x = 5, .y = 11},
    GridPosition{.x = 4, .y = 11},  GridPosition{.x = 12, .y = 11}};
/// Landings by the fallen kiosk and on the neon ledge, which the west lane
/// takes once the low west has drowned.
constexpr std::array HIGH_WEST{
    GridPosition{.x = 11, .y = 6}, GridPosition{.x = 13, .y = 3},
    GridPosition{.x = 14, .y = 5}, GridPosition{.x = 9, .y = 2},
    GridPosition{.x = 11, .y = 3}, GridPosition{.x = 10, .y = 2},
    GridPosition{.x = 12, .y = 3}};
/// Landings on the arcade and the overpass.
constexpr std::array MIDDLE{
    GridPosition{.x = 3, .y = 2},  GridPosition{.x = 8, .y = 2},
    GridPosition{.x = 15, .y = 5}, GridPosition{.x = 4, .y = 2},
    GridPosition{.x = 6, .y = 2},  GridPosition{.x = 12, .y = 5},
    GridPosition{.x = 5, .y = 2},  GridPosition{.x = 13, .y = 5},
    GridPosition{.x = 17, .y = 5}, GridPosition{.x = 16, .y = 5},
    GridPosition{.x = 18, .y = 5}, GridPosition{.x = 7, .y = 2}};
/// Landings on the east avenue, the plaza and the pier.
constexpr std::array EAST{
    GridPosition{.x = 32, .y = 7},  GridPosition{.x = 35, .y = 12},
    GridPosition{.x = 26, .y = 8},  GridPosition{.x = 24, .y = 7},
    GridPosition{.x = 21, .y = 6},  GridPosition{.x = 34, .y = 12},
    GridPosition{.x = 23, .y = 7},  GridPosition{.x = 25, .y = 8},
    GridPosition{.x = 22, .y = 7},  GridPosition{.x = 32, .y = 12},
    GridPosition{.x = 33, .y = 12}, GridPosition{.x = 20, .y = 6}};

/// One way the tide washes bags up: the cells they land on in turn and what
/// they hold in turn, before `LATE_BAG_DAY` and from then on, when meat and
/// truffle start to wash up.
struct Lane {
  std::span<const GridPosition> landings;
  std::span<const GridPosition> late_landings;
  std::array<IngredientId, 5> cargo;
  std::array<IngredientId, 5> late_cargo;
};

/// Three bags land at the start of every day, one in each lane: the west, the
/// middle, and the east, so a day's errands cross the map.
constexpr std::array LANES{
    Lane{.landings = LOW_WEST,
         .late_landings = HIGH_WEST,
         .cargo = {FISH, TOMATO, PICKLES, MUSHROOM, TOMATO},
         .late_cargo = {PICKLES, TOMATO, MUSHROOM, TOMATO, TOMATO}},
    Lane{.landings = MIDDLE,
         .late_landings = MIDDLE,
         .cargo = {MUSHROOM, KELP, TOMATO, PICKLES, KELP},
         .late_cargo = {PICKLES, TOMATO, TRUFFLE, TOMATO, PICKLES}},
    Lane{.landings = EAST,
         .late_landings = EAST,
         .cargo = {KELP, FISH, PICKLES, TOMATO, PICKLES},
         .late_cargo = {TOMATO, MEAT, PICKLES, KELP, FISH}},
};
constexpr std::uint32_t LATE_BAG_DAY = 19;

constexpr std::uint32_t FIRST_BAG_DAY = 2;
constexpr std::uint32_t LAST_BAG_DAY = 50;
constexpr std::size_t WASHED_BAG_COUNT =
    LANES.size() * (LAST_BAG_DAY - FIRST_BAG_DAY + 1);
/// Days a bag lies before the tide takes it back; fewer than any lane has
/// landings, so a bag is gone before the next one lands on its cell.
constexpr std::uint32_t BAG_DAYS = 5;
static_assert(std::ranges::all_of(LANES, [](const Lane &lane) {
  return BAG_DAYS < lane.landings.size() &&
         BAG_DAYS < lane.late_landings.size();
}));

/// One washed-up bag: where it lands, what it holds, and the day it lands
/// with.
struct WashedBag {
  GridPosition cell;
  IngredientId ingredient;
  std::uint32_t day = 0;
};

WashedBag washed_bag_at(std::size_t index) {
  const auto turn = index / LANES.size();
  const auto &lane = LANES.at(index % LANES.size());
  const auto day = FIRST_BAG_DAY + static_cast<std::uint32_t>(turn);
  const bool late = day >= LATE_BAG_DAY;
  const auto &cargo = late ? lane.late_cargo : lane.cargo;
  const auto landings = late ? lane.late_landings : lane.landings;
  return {.cell = landings[turn % landings.size()],
          .ingredient = cargo.at(turn % cargo.size()),
          .day = day};
}

/// The identity of the washed-up bag at `index`, from the first one.
constexpr ObjectId FIRST_WASHED_BAG{101};
ObjectId washed_bag(std::size_t index) {
  return ObjectId{FIRST_WASHED_BAG.value + static_cast<std::uint32_t>(index)};
}

std::string_view washed_bag_name(std::size_t index) {
  static const auto names = [] {
    std::vector<std::string> list;
    for (std::size_t bag = 0; bag < WASHED_BAG_COUNT; ++bag) {
      list.push_back("Washed-up Bag " + std::to_string(bag + 1));
    }
    return list;
  }();
  return names.at(index);
}

template <typename Id> struct Named {
  Id id;
  std::string_view name;
};

constexpr std::array INGREDIENT_NAMES{
    Named<IngredientId>{.id = TOMATO, .name = "Tomato"},
    Named<IngredientId>{.id = FISH, .name = "Fish"},
    Named<IngredientId>{.id = KELP, .name = "Kelp"},
    Named<IngredientId>{.id = MUSHROOM, .name = "Mushroom"},
    Named<IngredientId>{.id = PICKLES, .name = "Pickles"},
    Named<IngredientId>{.id = MEAT, .name = "Meat"},
    Named<IngredientId>{.id = TRUFFLE, .name = "Truffle"},
};

constexpr std::array TOOL_NAMES{
    Named<ToolId>{.id = MAGNIFIER, .name = "Magnifier"},
    Named<ToolId>{.id = SPATULA, .name = "Spatula"},
    Named<ToolId>{.id = WRENCH, .name = "Wrench"},
};

constexpr std::array FOOD_NAMES{
    Named<FoodId>{.id = MUSHROOM_SOUP, .name = "Mushroom Soup"},
    Named<FoodId>{.id = FISH_STEW, .name = "Fish Stew"},
    Named<FoodId>{.id = KELP_WRAP, .name = "Kelp Wrap"},
    Named<FoodId>{.id = MEAT_SKEWER, .name = "Meat Skewer"},
    Named<FoodId>{.id = TRUFFLE_ROAST, .name = "Truffle Roast"},
    Named<FoodId>{.id = PICKLE_SURPRISE, .name = "Pickle Surprise"},
    Named<FoodId>{.id = SEA_SALAD, .name = "Sea Salad"},
    Named<FoodId>{.id = FISH_PIE, .name = "Fish Pie"},
    Named<FoodId>{.id = MEAT_STEW, .name = "Meat Stew"},
    Named<FoodId>{.id = TRUFFLE_SOUP, .name = "Truffle Soup"},
    Named<FoodId>{.id = SURF_AND_TURF, .name = "Surf and Turf"},
};

constexpr std::array OBJECT_NAMES{
    Named<ObjectId>{.id = BASE_DOCK, .name = "Base Dock"},
    Named<ObjectId>{.id = MAIN_STREET, .name = "Main Street"},
    Named<ObjectId>{.id = ROTTEN_BRIDGE, .name = "Rotten Bridge"},
    Named<ObjectId>{.id = EAST_PLAZA, .name = "East Plaza"},
    Named<ObjectId>{.id = NORTH_LADDER, .name = "North Ladder"},
    Named<ObjectId>{.id = ARCADE, .name = "Arcade"},
    Named<ObjectId>{.id = SOUTH_STAIRS, .name = "South Stairs"},
    Named<ObjectId>{.id = MARKET, .name = "Market"},
    Named<ObjectId>{.id = ROOFTOP, .name = "Rooftop"},
    Named<ObjectId>{.id = DRIFTED_FERRY, .name = "Drifted Ferry"},
    Named<ObjectId>{.id = ROOF_LADDER, .name = "Roof Ladder"},
    Named<ObjectId>{.id = COLD_STORE_DOOR, .name = "Cold Store Door"},
    Named<ObjectId>{.id = CORNER_SHOP, .name = "Corner Shop"},
    Named<ObjectId>{.id = COLD_STORE, .name = "Cold Store"},
    Named<ObjectId>{.id = KIOSK_WRECK, .name = "Old Kiosk"},
    Named<ObjectId>{.id = NOODLE_VENDOR, .name = "Noodle Vendor"},
    Named<ObjectId>{.id = TOMATO_MACHINE, .name = "Tomato Machine"},
    Named<ObjectId>{.id = VENDOR_CART, .name = "Vendor Cart"},
    Named<ObjectId>{.id = SHOP_SHELF, .name = "Shop Shelf"},
    Named<ObjectId>{.id = LOST_BASKET, .name = "Lost Basket"},
    Named<ObjectId>{.id = TOOL_BAG, .name = "Tool Bag"},
    Named<ObjectId>{.id = PICKLE_MACHINE, .name = "Pickle Machine"},
    Named<ObjectId>{.id = ARCADE_BAG, .name = "Arcade Bag"},
    Named<ObjectId>{.id = SUPPLY_CRATE, .name = "Supply Crate"},
    Named<ObjectId>{.id = COLD_LOCKER, .name = "Cold Locker"},
    Named<ObjectId>{.id = ROOF_BAG, .name = "Roof Bag"},
    Named<ObjectId>{.id = FERRY_BAG, .name = "Ferry Bag"},
    Named<ObjectId>{.id = NEON_LEDGE, .name = "Neon Ledge"},
    Named<ObjectId>{.id = CRACKED_RAMP, .name = "Cracked Ramp"},
    Named<ObjectId>{.id = OVERPASS_STUB, .name = "Overpass Stub"},
    Named<ObjectId>{.id = FALLEN_SLAB, .name = "Fallen Slab"},
    Named<ObjectId>{.id = EAST_AVENUE, .name = "East Avenue"},
    Named<ObjectId>{.id = SUNKEN_ASPHALT, .name = "Sunken Asphalt"},
    Named<ObjectId>{.id = COLD_STORE_FLOOR, .name = "Cold Store Floor"},
    Named<ObjectId>{.id = MARKET_SHALLOWS, .name = "Market Shallows"},
    Named<ObjectId>{.id = TILTED_CAR, .name = "Tilted Car"},
    Named<ObjectId>{.id = PIER, .name = "Pier"},
    Named<ObjectId>{.id = PLAZA_LADDER, .name = "Plaza Ladder"},
    Named<ObjectId>{.id = WEST_RUBBLE, .name = "West Rubble"},
    Named<ObjectId>{.id = EAST_RUBBLE, .name = "East Rubble"},
    Named<ObjectId>{.id = UPPER_RAMP, .name = "Upper Ramp"},
    Named<ObjectId>{.id = MARKET_LADDER, .name = "Market Ladder"},
};

template <typename Id, std::size_t Size>
std::string_view name_of(const std::array<Named<Id>, Size> &names, Id id) {
  if constexpr (std::is_same_v<Id, ObjectId>) {
    const auto bag = id.value - FIRST_WASHED_BAG.value;
    if (id.value >= FIRST_WASHED_BAG.value && bag < WASHED_BAG_COUNT) {
      return washed_bag_name(bag);
    }
  }
  const auto found = std::ranges::find(names, id, &Named<Id>::id);
  if (found == names.end()) {
    throw std::out_of_range("identity is not in the temporary content");
  }
  return found->name;
}

ObjectDefinition terrain(ObjectId id, WorldArea area) {
  return {.id = id, .kind = ObjectKind::Terrain, .area = area};
}

ObjectDefinition ingredient_source(ObjectId id, GridPosition cell,
                                   IngredientId ingredient) {
  return {.id = id,
          .kind = ObjectKind::MaterialSource,
          .area = {.x = cell.x, .y = cell.y},
          .ingredient = ingredient};
}

ObjectDefinition tool_source(ObjectId id, GridPosition cell, ToolId tool) {
  return {.id = id,
          .kind = ObjectKind::MaterialSource,
          .area = {.x = cell.x, .y = cell.y},
          .tool = tool};
}

ObjectDefinition owned(ObjectDefinition source, ObjectId owner) {
  source.supported_by = owner;
  return source;
}

ObjectDefinition exposed(ObjectDefinition object, WorldThreshold threshold) {
  object.exposed_at = threshold;
  return object;
}

ObjectDefinition destroyed(ObjectDefinition object, WorldThreshold threshold) {
  object.destroyed_after = threshold;
  return object;
}

/// The acid that carries off a bag on `row`: the lower the row, the sooner, so
/// a bag's limit keeps the order every acid-only threshold keeps.
AcidLevel bag_limit(std::int32_t row) {
  if (row >= 13) {
    return AcidLevel{35};
  }
  if (row >= 12) {
    return AcidLevel{45};
  }
  if (row >= 11) {
    return AcidLevel{50};
  }
  if (row >= 8) {
    return AcidLevel{55};
  }
  if (row >= 7) {
    return AcidLevel{65};
  }
  if (row >= 6) {
    return AcidLevel{70};
  }
  if (row >= 5) {
    return AcidLevel{75};
  }
  if (row >= 3) {
    return AcidLevel{80};
  }
  return AcidLevel{85};
}

/// A bag the tide brings in once world time reaches the start of its day and
/// takes back `BAG_DAYS` days later. Acid never brings one: it carries the bag
/// off at its row's limit, or sinks it before it lands, so rising acid only
/// takes supply away.
ObjectDefinition washed_up(std::size_t index) {
  const auto bag = washed_bag_at(index);
  const auto landing = (bag.day - 1) * DAY;
  return destroyed(
      exposed(ingredient_source(washed_bag(index), bag.cell, bag.ingredient),
              {.elapsed = landing}),
      {.elapsed = landing + BAG_DAYS * DAY, .acid = bag_limit(bag.cell.y)});
}

ObjectDefinition passage(ObjectId id, ObjectKind kind, WorldArea area) {
  return {.id = id, .kind = kind, .area = area, .repair_with = WRENCH};
}

ObjectDefinition building(ObjectId id, WorldArea area) {
  return {.id = id, .kind = ObjectKind::Building, .area = area};
}

/// A ladder nothing breaks.
ObjectDefinition ladder(ObjectId id, WorldArea area) {
  return {.id = id, .kind = ObjectKind::Ladder, .area = area};
}

/// A broken road across the gullet, seen from the side: every walkway is one
/// row, and floors meet only by ladder, stairs, ramp, or a jump. Low ground
/// drowns first, so every acid-only threshold rises with height.
std::vector<ObjectDefinition> city() {
  constexpr auto ferry_time = DAY * 160 / 9;
  const WorldThreshold ferry_arrives{.elapsed = ferry_time,
                                     .acid = AcidLevel{30}};
  std::vector<ObjectDefinition> objects{
      terrain(BASE_DOCK, {.x = 0, .y = 8, .width = 4}),
      terrain(MAIN_STREET, {.x = 4, .y = 8, .width = 6}),
      terrain(CRACKED_RAMP, {.x = 10, .y = 7}),
      terrain(UPPER_RAMP, {.x = 11, .y = 6}),
      terrain(OVERPASS_STUB, {.x = 12, .y = 5, .width = 7}),
      terrain(FALLEN_SLAB, {.x = 20, .y = 6, .width = 2}),
      terrain(EAST_AVENUE, {.x = 22, .y = 7, .width = 3}),
      destroyed(terrain(SUNKEN_ASPHALT, {.x = 25, .y = 8, .width = 2}),
                {.acid = AcidLevel{55}}),
      destroyed(terrain(ROTTEN_BRIDGE, {.x = 27, .y = 8, .width = 2}),
                {.elapsed = 24 * DAY, .acid = AcidLevel{40}}),
      terrain(EAST_PLAZA, {.x = 29, .y = 7, .width = 4}),
      terrain(COLD_STORE_FLOOR, {.x = 33, .y = 7, .width = 3}),
      ladder(NORTH_LADDER, {.x = 2, .y = 2, .height = 7}),
      terrain(ARCADE, {.x = 1, .y = 2, .width = 10}),
      terrain(NEON_LEDGE, {.x = 11, .y = 3, .width = 4}),
      ladder(MARKET_LADDER, {.x = 7, .y = 8, .height = 4}),
      terrain(MARKET, {.x = 4, .y = 11, .width = 9}),
      destroyed(terrain(MARKET_SHALLOWS, {.x = 0, .y = 12, .width = 4}),
                {.acid = AcidLevel{45}}),
      terrain(SOUTH_STAIRS, {.x = 13, .y = 12}),
      terrain(TILTED_CAR, {.x = 14, .y = 13, .width = 4}),
      exposed(terrain(DRIFTED_FERRY, {.x = 20, .y = 13, .width = 10}),
              ferry_arrives),
      terrain(PIER, {.x = 31, .y = 12, .width = 5}),
      ladder(PLAZA_LADDER, {.x = 31, .y = 7, .height = 6}),
      terrain(ROOFTOP, {.x = 18, .y = 2, .width = 6}),
      destroyed(passage(ROOF_LADDER, ObjectKind::Ladder,
                        {.x = 18, .y = 2, .height = 4}),
                {.elapsed = DAY * 16 / 3, .acid = AcidLevel{10}}),
      destroyed(passage(COLD_STORE_DOOR, ObjectKind::Entrance,
                        {.x = 33, .y = 7, .width = 3}),
                {.elapsed = DAY * 80 / 9, .acid = AcidLevel{20}}),
      destroyed(
          building(CORNER_SHOP, {.x = 13, .y = 6, .width = 3, .height = 2}),
          {.elapsed = DAY * 40 / 3, .acid = AcidLevel{25}}),
      building(COLD_STORE, {.x = 33, .y = 5, .width = 3, .height = 3}),
      destroyed(
          building(KIOSK_WRECK, {.x = 9, .y = 1, .width = 2, .height = 2}),
          {.elapsed = DAY * 8}),
      building(WEST_RUBBLE, {.x = 9, .y = 8}),
      building(EAST_RUBBLE, {.x = 30, .y = 7}),
      destroyed({.id = NOODLE_VENDOR,
                 .kind = ObjectKind::Npc,
                 .area = {.x = 8, .y = 8}},
                {.elapsed = DAY * 32 / 3, .acid = AcidLevel{20}}),
      destroyed(ingredient_source(TOMATO_MACHINE, {.x = 6, .y = 11}, TOMATO),
                {.acid = AcidLevel{50}}),
      owned(ingredient_source(VENDOR_CART, {.x = 8, .y = 8}, FISH),
            NOODLE_VENDOR),
      owned(ingredient_source(SHOP_SHELF, {.x = 14, .y = 6}, KELP),
            CORNER_SHOP),
      destroyed(ingredient_source(LOST_BASKET, {.x = 2, .y = 12}, MUSHROOM),
                {.acid = AcidLevel{40}}),
      tool_source(TOOL_BAG, {.x = 11, .y = 11}, WRENCH),
      destroyed(ingredient_source(PICKLE_MACHINE, {.x = 0, .y = 12}, PICKLES),
                {.acid = AcidLevel{35}}),
      tool_source(ARCADE_BAG, {.x = 14, .y = 3}, MAGNIFIER),
      tool_source(SUPPLY_CRATE, {.x = 1, .y = 2}, SPATULA),
      owned(ingredient_source(COLD_LOCKER, {.x = 35, .y = 7}, MEAT),
            COLD_STORE),
      ingredient_source(ROOF_BAG, {.x = 23, .y = 2}, TRUFFLE),
      exposed(owned(ingredient_source(FERRY_BAG, {.x = 25, .y = 13}, KELP),
                    DRIFTED_FERRY),
              {.elapsed = ferry_time}),
  };
  for (std::size_t index = 0; index < WASHED_BAG_COUNT; ++index) {
    objects.push_back(washed_up(index));
  }
  return objects;
}

/// The road surface by row, one character a cell: = road, ~ cracked, o
/// pothole, < and > the left and right end of a broken road, / tilted slab,
/// % rubble, _ planks, c buried car, and . no ground.
constexpr std::array<std::string_view, 16> GROUND_ROWS{
    "....................................",
    "....................................",
    ".<=~=c~=o=>.......<=~o=>............",
    "...........<~o>.....................",
    "....................................",
    "............=~=o~=>.................",
    ".........../........//..............",
    "........../...........=~>....<%=~=~=",
    "=~=c~o=~=%...............o~__.......",
    "....................................",
    "....................................",
    "....<=~o=c~=>.......................",
    "=~o>........./.................=~o=~",
    "............../ccc..__________......",
    "....................................",
    "....................................",
};

std::optional<GroundKind> ground_kind(char mark) {
  switch (mark) {
  case '=':
    return GroundKind::Road;
  case '~':
    return GroundKind::CrackedRoad;
  case 'o':
    return GroundKind::Pothole;
  case '<':
    return GroundKind::RoadEndLeft;
  case '>':
    return GroundKind::RoadEndRight;
  case '/':
    return GroundKind::TiltedSlab;
  case '%':
    return GroundKind::Rubble;
  case '_':
    return GroundKind::PlankBridge;
  case 'c':
    return GroundKind::BuriedCar;
  case '.':
    return std::nullopt;
  default:
    throw std::logic_error("unknown ground mark");
  }
}

std::vector<GroundCell> ground() {
  std::vector<GroundCell> cells;
  for (std::size_t row = 0; row < GROUND_ROWS.size(); ++row) {
    const auto marks = GROUND_ROWS.at(row);
    for (std::size_t column = 0; column < marks.size(); ++column) {
      if (const auto kind = ground_kind(marks.at(column))) {
        cells.push_back({.cell = {.x = static_cast<std::int32_t>(column),
                                  .y = static_cast<std::int32_t>(row)},
                         .kind = *kind});
      }
    }
  }
  return cells;
}

std::vector<Decoration> decorations() {
  using enum DecorationKind;
  return {
      {.kind = TiltedTrain, .area = {.x = 25, .y = 3, .width = 3}},
      {.kind = TiltedTrain, .area = {.x = 29, .y = 2, .width = 4}},
      {.kind = TiltedTrain, .area = {.x = 27, .y = 14, .width = 4}},
      {.kind = BuildingShell,
       .area = {.x = 0, .y = 3, .width = 2, .height = 4}},
      {.kind = BuildingShell,
       .area = {.x = 9, .y = 9, .width = 4, .height = 2}},
      {.kind = BuildingShell,
       .area = {.x = 31, .y = 9, .width = 4, .height = 3}},
      {.kind = HangingCables, .area = {.x = 12, .y = 4, .width = 5}},
      {.kind = HangingCables, .area = {.x = 22, .y = 5, .width = 4}},
      {.kind = DeadNeonSign, .area = {.x = 5, .y = 4, .width = 2, .height = 2}},
      {.kind = DeadNeonSign, .area = {.x = 30, .y = 4, .width = 2}},
      {.kind = FleshGrowth, .area = {.x = 0, .y = 13, .width = 3, .height = 2}},
      {.kind = FleshGrowth,
       .area = {.x = 33, .y = 13, .width = 3, .height = 2}},
      {.kind = FleshGrowth, .area = {.x = 15, .y = 9, .width = 3, .height = 2}},
      {.kind = RubblePile, .area = {.x = 9, .y = 13, .width = 4}},
      {.kind = RubblePile, .area = {.x = 19, .y = 9, .width = 3, .height = 2}},
      {.kind = BrokenPillar, .area = {.x = 17, .y = 6, .height = 3}},
      {.kind = BrokenPillar, .area = {.x = 24, .y = 8, .height = 4}},
      {.kind = BrokenPillar, .area = {.x = 28, .y = 9, .height = 3}},
      {.kind = StreetLamp, .area = {.x = 3, .y = 7}},
      {.kind = StreetLamp, .area = {.x = 24, .y = 6}},
      {.kind = StreetLamp, .area = {.x = 32, .y = 6}},
  };
}

std::vector<Landmark> landmarks() {
  using enum LandmarkKind;
  return {
      {.object = KIOSK_WRECK, .kind = Kiosk},
      {.object = COLD_STORE, .kind = ColdStore},
      {.object = DRIFTED_FERRY, .kind = Ferry},
      {.object = PIER, .kind = Platform},
      {.object = ROTTEN_BRIDGE, .kind = Bridge},
  };
}

std::vector<SourceStock> stock() {
  std::vector<SourceStock> sources{
      {.id = TOMATO_MACHINE,
       .kind = SourceKind::VendingMachine,
       .remaining = 6},
      {.id = VENDOR_CART, .kind = SourceKind::Npc, .remaining = 4},
      {.id = SHOP_SHELF, .kind = SourceKind::Shop, .remaining = 3},
      {.id = LOST_BASKET, .kind = SourceKind::Bag, .remaining = 4},
      {.id = TOOL_BAG, .kind = SourceKind::Bag, .remaining = 1},
      {.id = PICKLE_MACHINE,
       .kind = SourceKind::VendingMachine,
       .remaining = 3},
      {.id = ARCADE_BAG, .kind = SourceKind::Bag, .remaining = 2},
      {.id = SUPPLY_CRATE, .kind = SourceKind::Shop, .remaining = 2},
      {.id = COLD_LOCKER, .kind = SourceKind::Shop, .remaining = 3},
      {.id = ROOF_BAG, .kind = SourceKind::Bag, .remaining = 2},
      {.id = FERRY_BAG, .kind = SourceKind::Bag, .remaining = 3},
  };
  for (std::size_t index = 0; index < WASHED_BAG_COUNT; ++index) {
    sources.push_back({.id = washed_bag(index),
                       .kind = SourceKind::Bag,
                       .remaining = BAG_STOCK});
  }
  return sources;
}

std::vector<Recipe> recipes() {
  return {
      {.food = MUSHROOM_SOUP, .ingredients = {MUSHROOM, TOMATO}},
      {.food = FISH_STEW, .ingredients = {FISH, TOMATO}},
      {.food = KELP_WRAP, .ingredients = {KELP, FISH}},
      {.food = MEAT_SKEWER, .ingredients = {MEAT, PICKLES}},
      {.food = TRUFFLE_ROAST, .ingredients = {TRUFFLE, MEAT}},
      {.food = PICKLE_SURPRISE,
       .ingredients = {PICKLES, MUSHROOM},
       .disliked = true},
      {.food = SEA_SALAD, .ingredients = {KELP, TOMATO}},
      {.food = FISH_PIE, .ingredients = {FISH, MUSHROOM}},
      {.food = MEAT_STEW, .ingredients = {MEAT, TOMATO}},
      {.food = TRUFFLE_SOUP, .ingredients = {TRUFFLE, MUSHROOM}},
      {.food = SURF_AND_TURF, .ingredients = {MEAT, FISH}},
  };
}

std::vector<Flavor> flavors() {
  return {{.ingredient = TOMATO, .taste = 20},
          {.ingredient = FISH, .taste = 35},
          {.ingredient = KELP, .taste = 25},
          {.ingredient = MUSHROOM, .taste = 25},
          {.ingredient = PICKLES, .taste = 10},
          {.ingredient = MEAT, .taste = 40},
          {.ingredient = TRUFFLE, .taste = 55}};
}

} // namespace

Content temporary_content(std::uint32_t seed) {
  return {
      .run = {.exploration_budget = DAY,
              .seed = seed,
              .full_acid = AcidLevel{100}},
      .objects = city(),
      .companion = {.exploration = {.base = {.x = 1, .y = 8},
                                    .entry = {.x = 3, .y = 8},
                                    .maximum_stamina = 20,
                                    .collection_distance = 1,
                                    .sources = stock(),
                                    .collection_time = 1500ms},
                    .workshop = {.recipes = recipes(),
                                 .magnifier = MAGNIFIER,
                                 .spatula = SPATULA,
                                 .new_food_score = 50,
                                 .new_ingredient_score = 5,
                                 .satisfying_score = 100,
                                 .disliked_acid_rise = AcidLevel{15},
                                 .unsatisfied_acid_rise = AcidLevel{10},
                                 .flavors = flavors(),
                                 .fine_dish_score = 30},
                    .unfed_penalty = AcidLevel{12}},
      .hazards = {{.x = 10, .y = 7}},
      .map = {.width = 36, .height = 16},
      .ground = ground(),
      .decorations = decorations(),
      .landmarks = landmarks(),
  };
}

std::string_view ingredient_name(IngredientId ingredient) {
  return name_of(INGREDIENT_NAMES, ingredient);
}

std::string_view food_name(FoodId food) { return name_of(FOOD_NAMES, food); }

std::string_view tool_name(ToolId tool) { return name_of(TOOL_NAMES, tool); }

std::string_view object_name(ObjectId object) {
  return name_of(OBJECT_NAMES, object);
}

} // namespace neocity::survival
