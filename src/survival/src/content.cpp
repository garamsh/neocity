#include <survival/content.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <iterator>
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

constexpr ObjectId UPPER_WALKWAY{1};
constexpr ObjectId LOWER_WALKWAY{2};
constexpr ObjectId ROTTEN_BRIDGE{3};
constexpr ObjectId EAST_WALKWAY{4};
constexpr ObjectId KITCHEN_LADDER{5};
constexpr ObjectId KITCHEN_LOFT{6};
constexpr ObjectId GAUGE_LADDER{7};
constexpr ObjectId GAUGE_PIER{8};
constexpr ObjectId KITCHEN_ROOF{9};
constexpr ObjectId DRIFTED_FERRY{10};
constexpr ObjectId ROOF_LADDER{11};
constexpr ObjectId COLD_STORE_DOOR{12};
constexpr ObjectId TOWER_SHOP{13};
constexpr ObjectId COLD_STORE{14};
constexpr ObjectId KIOSK_WRECK{15};
constexpr ObjectId NOODLE_VENDOR{16};
constexpr ObjectId KITCHEN_SHOP{17};
constexpr ObjectId NOODLE_SHOP{18};
constexpr ObjectId TOMATO_MACHINE{21};
constexpr ObjectId VENDOR_CART{22};
constexpr ObjectId SHOP_SHELF{23};
constexpr ObjectId LOST_BASKET{24};
constexpr ObjectId PICKLE_MACHINE{26};
constexpr ObjectId COLD_LOCKER{29};
constexpr ObjectId ROOF_BAG{30};
constexpr ObjectId FERRY_BAG{31};
constexpr ObjectId ROPE_BRIDGE{51};
constexpr ObjectId GAUGE_LEDGE{52};
constexpr ObjectId SHOP_ROOF{53};
constexpr ObjectId EAST_ROOF{54};
constexpr ObjectId EAST_LOFT{55};
constexpr ObjectId EAST_LOWER_WALKWAY{56};
constexpr ObjectId COLD_STORE_FLOOR{57};
constexpr ObjectId SHALLOWS{58};
constexpr ObjectId SUNKEN_TRAIN{59};
constexpr ObjectId PIER{60};
constexpr ObjectId PIER_LADDER{61};
constexpr ObjectId WEST_RUBBLE{62};
constexpr ObjectId EAST_RUBBLE{63};
constexpr ObjectId EAST_ROOF_LADDER{64};
constexpr ObjectId EAST_LOFT_LADDER{65};
constexpr ObjectId FIRE_ESCAPE{66};
constexpr ObjectId TERMINAL_ROAD{67};
constexpr ObjectId TERMINAL_STAIR{68};
constexpr ObjectId TERMINAL_STAIR_TOP{69};
constexpr ObjectId UPPER_DECK{70};
constexpr ObjectId DECK_LADDER{71};
constexpr ObjectId TOP_CATWALK{72};
constexpr ObjectId DOCK_LADDER{73};
constexpr ObjectId DOCK{74};
constexpr ObjectId TERMINAL_MACHINE{27};
constexpr ObjectId DECK_BAG{28};
constexpr ObjectId DOCK_CRATE{25};
constexpr ObjectId LOBBY_FLOOR{80};
constexpr ObjectId LOBBY_STAIR{81};
constexpr ObjectId WEST_CATWALK{82};
constexpr ObjectId UPPER_SPAN{83};
constexpr ObjectId EAST_CATWALK{84};
constexpr ObjectId WEST_LADDER{85};
constexpr ObjectId WEST_GALLERY{86};
constexpr ObjectId LOWER_SPAN{87};
constexpr ObjectId EAST_GALLERY{88};
constexpr ObjectId EAST_GALLERY_LADDER{89};
constexpr ObjectId SKYWALK{90};
constexpr ObjectId SKY_LADDER_WEST{91};
constexpr ObjectId SKY_LADDER_EAST{92};
constexpr ObjectId TANK_STAIR{93};
constexpr ObjectId TANK_STAIR_LOW{94};
constexpr ObjectId TANK_FLOOR{95};
constexpr ObjectId KEEPER{96};
constexpr ObjectId KEEPER_CART{97};
constexpr ObjectId GIFT_SHELF{98};
constexpr ObjectId JELLY_MACHINE{99};
constexpr ObjectId TANK_LOCKER{100};

/// One exploration day's length.
constexpr auto DAY = 18s;

/// The days the damage stages after the first begin on.
constexpr std::uint32_t CRACKED_DAY = STAGE_FIRST_DAY.at(1);
constexpr std::uint32_t RUINED_DAY = STAGE_FIRST_DAY.at(2);
constexpr std::uint32_t BAG_STOCK = 2;

/// Landings on the shallows and the gauge pier at the foot of the gauge
/// ladder, which acid drowns first.
constexpr std::array LOW_WEST{
    GridPosition{.x = 16, .y = 12}, GridPosition{.x = 18, .y = 12},
    GridPosition{.x = 14, .y = 12}, GridPosition{.x = 19, .y = 12},
    GridPosition{.x = 15, .y = 12}, GridPosition{.x = 17, .y = 12}};
/// Landings on the shop roof by the fallen kiosk, the gauge ledge and the rope
/// bridge, which the west lane takes once the low west has drowned.
constexpr std::array HIGH_WEST{
    GridPosition{.x = 18, .y = 5}, GridPosition{.x = 15, .y = 9},
    GridPosition{.x = 17, .y = 8}, GridPosition{.x = 19, .y = 5},
    GridPosition{.x = 14, .y = 9}, GridPosition{.x = 18, .y = 8},
    GridPosition{.x = 16, .y = 9}};
/// Landings on the kitchen loft.
constexpr std::array MIDDLE{
    GridPosition{.x = 7, .y = 6}, GridPosition{.x = 4, .y = 6},
    GridPosition{.x = 6, .y = 6}, GridPosition{.x = 0, .y = 6},
    GridPosition{.x = 2, .y = 6}, GridPosition{.x = 1, .y = 6}};
/// Landings on the east half: its roof, loft, walkways and pier.
constexpr std::array EAST{
    GridPosition{.x = 26, .y = 8}, GridPosition{.x = 36, .y = 12},
    GridPosition{.x = 22, .y = 4}, GridPosition{.x = 27, .y = 6},
    GridPosition{.x = 31, .y = 9}, GridPosition{.x = 24, .y = 4},
    GridPosition{.x = 21, .y = 8}, GridPosition{.x = 38, .y = 12},
    GridPosition{.x = 20, .y = 6}, GridPosition{.x = 26, .y = 4},
    GridPosition{.x = 28, .y = 8}, GridPosition{.x = 22, .y = 6}};

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
/// kitchen loft, and the east, so a day's errands cross the map.
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
  return {.cell = *std::ranges::next(
              landings.begin(),
              static_cast<std::ptrdiff_t>(turn % landings.size())),
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
    list.reserve(WASHED_BAG_COUNT);
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
    Named<ObjectId>{.id = UPPER_WALKWAY, .name = "Upper Walkway"},
    Named<ObjectId>{.id = LOWER_WALKWAY, .name = "Lower Walkway"},
    Named<ObjectId>{.id = ROPE_BRIDGE, .name = "Rope Bridge"},
    Named<ObjectId>{.id = GAUGE_LEDGE, .name = "Gauge Ledge"},
    Named<ObjectId>{.id = SHOP_ROOF, .name = "Shop Roof"},
    Named<ObjectId>{.id = GAUGE_LADDER, .name = "Gauge Ladder"},
    Named<ObjectId>{.id = GAUGE_PIER, .name = "Gauge Pier"},
    Named<ObjectId>{.id = SHALLOWS, .name = "Shallows"},
    Named<ObjectId>{.id = KITCHEN_LADDER, .name = "Kitchen Ladder"},
    Named<ObjectId>{.id = KITCHEN_LOFT, .name = "Kitchen Loft"},
    Named<ObjectId>{.id = ROOF_LADDER, .name = "Roof Ladder"},
    Named<ObjectId>{.id = KITCHEN_ROOF, .name = "Kitchen Roof"},
    Named<ObjectId>{.id = KITCHEN_SHOP, .name = "Kitchen Shop"},
    Named<ObjectId>{.id = TOWER_SHOP, .name = "Tower Shop"},
    Named<ObjectId>{.id = WEST_RUBBLE, .name = "West Rubble"},
    Named<ObjectId>{.id = EAST_WALKWAY, .name = "East Walkway"},
    Named<ObjectId>{.id = EAST_LOFT_LADDER, .name = "East Loft Ladder"},
    Named<ObjectId>{.id = EAST_LOFT, .name = "East Loft"},
    Named<ObjectId>{.id = EAST_ROOF_LADDER, .name = "East Roof Ladder"},
    Named<ObjectId>{.id = EAST_ROOF, .name = "East Roof"},
    Named<ObjectId>{.id = NOODLE_SHOP, .name = "Noodle Shop"},
    Named<ObjectId>{.id = KIOSK_WRECK, .name = "Old Kiosk"},
    Named<ObjectId>{.id = NOODLE_VENDOR, .name = "Noodle Vendor"},
    Named<ObjectId>{.id = EAST_LOWER_WALKWAY, .name = "East Lower Walkway"},
    Named<ObjectId>{.id = EAST_RUBBLE, .name = "East Rubble"},
    Named<ObjectId>{.id = ROTTEN_BRIDGE, .name = "Rotten Bridge"},
    Named<ObjectId>{.id = COLD_STORE_FLOOR, .name = "Cold Store Floor"},
    Named<ObjectId>{.id = COLD_STORE, .name = "Cold Store"},
    Named<ObjectId>{.id = COLD_STORE_DOOR, .name = "Cold Store Door"},
    Named<ObjectId>{.id = PIER_LADDER, .name = "Pier Ladder"},
    Named<ObjectId>{.id = PIER, .name = "Pier"},
    Named<ObjectId>{.id = SUNKEN_TRAIN, .name = "Sunken Train"},
    Named<ObjectId>{.id = DRIFTED_FERRY, .name = "Drifted Ferry"},
    Named<ObjectId>{.id = TOMATO_MACHINE, .name = "Tomato Machine"},
    Named<ObjectId>{.id = VENDOR_CART, .name = "Vendor Cart"},
    Named<ObjectId>{.id = SHOP_SHELF, .name = "Shop Shelf"},
    Named<ObjectId>{.id = LOST_BASKET, .name = "Lost Basket"},
    Named<ObjectId>{.id = PICKLE_MACHINE, .name = "Pickle Machine"},
    Named<ObjectId>{.id = COLD_LOCKER, .name = "Cold Locker"},
    Named<ObjectId>{.id = ROOF_BAG, .name = "Roof Bag"},
    Named<ObjectId>{.id = FERRY_BAG, .name = "Ferry Bag"},
    Named<ObjectId>{.id = FIRE_ESCAPE, .name = "Fire Escape"},
    Named<ObjectId>{.id = TERMINAL_ROAD, .name = "Terminal Road"},
    Named<ObjectId>{.id = TERMINAL_STAIR, .name = "Terminal Stair"},
    Named<ObjectId>{.id = TERMINAL_STAIR_TOP, .name = "Terminal Stair Top"},
    Named<ObjectId>{.id = UPPER_DECK, .name = "Upper Deck"},
    Named<ObjectId>{.id = DECK_LADDER, .name = "Deck Ladder"},
    Named<ObjectId>{.id = TOP_CATWALK, .name = "Top Catwalk"},
    Named<ObjectId>{.id = DOCK_LADDER, .name = "Dock Ladder"},
    Named<ObjectId>{.id = DOCK, .name = "Dock"},
    Named<ObjectId>{.id = TERMINAL_MACHINE, .name = "Terminal Machine"},
    Named<ObjectId>{.id = DECK_BAG, .name = "Deck Bag"},
    Named<ObjectId>{.id = DOCK_CRATE, .name = "Dock Crate"},
    Named<ObjectId>{.id = LOBBY_FLOOR, .name = "Lobby Floor"},
    Named<ObjectId>{.id = LOBBY_STAIR, .name = "Lobby Stair"},
    Named<ObjectId>{.id = WEST_CATWALK, .name = "West Catwalk"},
    Named<ObjectId>{.id = UPPER_SPAN, .name = "Upper Span"},
    Named<ObjectId>{.id = EAST_CATWALK, .name = "East Catwalk"},
    Named<ObjectId>{.id = WEST_LADDER, .name = "West Ladder"},
    Named<ObjectId>{.id = WEST_GALLERY, .name = "West Gallery"},
    Named<ObjectId>{.id = LOWER_SPAN, .name = "Lower Span"},
    Named<ObjectId>{.id = EAST_GALLERY, .name = "East Gallery"},
    Named<ObjectId>{.id = EAST_GALLERY_LADDER, .name = "East Gallery Ladder"},
    Named<ObjectId>{.id = SKYWALK, .name = "Skywalk"},
    Named<ObjectId>{.id = SKY_LADDER_WEST, .name = "West Skywalk Ladder"},
    Named<ObjectId>{.id = SKY_LADDER_EAST, .name = "East Skywalk Ladder"},
    Named<ObjectId>{.id = TANK_STAIR, .name = "Tank Stair"},
    Named<ObjectId>{.id = TANK_STAIR_LOW, .name = "Tank Stair Foot"},
    Named<ObjectId>{.id = TANK_FLOOR, .name = "Tank Floor"},
    Named<ObjectId>{.id = KEEPER, .name = "Tank Keeper"},
    Named<ObjectId>{.id = KEEPER_CART, .name = "Keeper Cart"},
    Named<ObjectId>{.id = GIFT_SHELF, .name = "Gift Shelf"},
    Named<ObjectId>{.id = JELLY_MACHINE, .name = "Jelly Machine"},
    Named<ObjectId>{.id = TANK_LOCKER, .name = "Tank Locker"},
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

ObjectDefinition owned(ObjectDefinition source, ObjectId owner) {
  source.supported_by = owner;
  return source;
}

ObjectDefinition from_day(ObjectDefinition object, std::uint32_t day) {
  object.exposed_from_day = day;
  return object;
}

ObjectDefinition gone_from(ObjectDefinition object, std::uint32_t day) {
  object.destroyed_from_day = day;
  return object;
}

ObjectDefinition aquatic(ObjectDefinition object) {
  object.map = MapId::Aquarium;
  return object;
}

/// A bag the tide brings in at the start of its day and takes back `BAG_DAYS`
/// days later. Acid neither brings nor takes one, and the day alone decides.
ObjectDefinition washed_up(std::size_t index) {
  const auto bag = washed_bag_at(index);
  return gone_from(
      from_day(ingredient_source(washed_bag(index), bag.cell, bag.ingredient),
               bag.day),
      bag.day + BAG_DAYS);
}

ObjectDefinition building(ObjectId id, WorldArea area) {
  return {.id = id, .kind = ObjectKind::Building, .area = area};
}

ObjectDefinition ladder(ObjectId id, WorldArea area) {
  return {.id = id, .kind = ObjectKind::Ladder, .area = area};
}

ObjectDefinition entrance(ObjectId id, WorldArea area) {
  return {.id = id, .kind = ObjectKind::Entrance, .area = area};
}

/// The street of the first mockup, seen from the side, laid out twice: in each
/// half a kitchen shop on stilts with a ladder to its loft and one to its roof,
/// an upper walkway that steps down to a lower one, a rope bridge, and a ledge
/// on a gauge pillar with a ladder down and a shop above. Every walkway is one
/// row, and floors meet only by ladder, a one-row step, or a jump. East of the
/// cold store a terminal climbs by stairs to a deck and by ladders to a top
/// catwalk and down to a dock, where the way into the aquarium lies.
///
/// Damage follows the day alone, in three stages: from `CRACKED_DAY` the
/// kitchen roof ladder is gone, a fire escape stands in its place and the
/// drifted ferry arrives; from `RUINED_DAY` the rotten bridge and the kiosk are
/// gone. Every stage leaves a way to every source, as the path check in the
/// tests proves.
std::vector<ObjectDefinition> street() {
  std::vector<ObjectDefinition> objects{
      terrain(UPPER_WALKWAY, {.x = 0, .y = 8, .width = 9}),
      terrain(LOWER_WALKWAY, {.x = 9, .y = 9, .width = 5}),
      terrain(ROPE_BRIDGE, {.x = 14, .y = 9, .width = 3}),
      terrain(GAUGE_LEDGE, {.x = 17, .y = 8, .width = 3}),
      terrain(SHOP_ROOF, {.x = 17, .y = 5, .width = 3}),
      ladder(GAUGE_LADDER, {.x = 18, .y = 8, .height = 5}),
      terrain(GAUGE_PIER, {.x = 17, .y = 12, .width = 3}),
      terrain(SHALLOWS, {.x = 14, .y = 12, .width = 3}),
      ladder(KITCHEN_LADDER, {.x = 3, .y = 6, .height = 3}),
      terrain(KITCHEN_LOFT, {.x = 0, .y = 6, .width = 8}),
      terrain(KITCHEN_ROOF, {.x = 0, .y = 4, .width = 8}),
      gone_from(ladder(ROOF_LADDER, {.x = 1, .y = 4, .height = 3}),
                CRACKED_DAY),
      from_day(ladder(FIRE_ESCAPE, {.x = 7, .y = 4, .height = 3}), CRACKED_DAY),
      building(KITCHEN_SHOP, {.x = 0, .y = 5, .width = 6}),
      building(TOWER_SHOP, {.x = 18, .y = 6, .width = 2, .height = 2}),
      building(WEST_RUBBLE, {.x = 12, .y = 9}),
      terrain(EAST_WALKWAY, {.x = 20, .y = 8, .width = 9}),
      ladder(EAST_LOFT_LADDER, {.x = 23, .y = 6, .height = 3}),
      terrain(EAST_LOFT, {.x = 20, .y = 6, .width = 8}),
      ladder(EAST_ROOF_LADDER, {.x = 21, .y = 4, .height = 3}),
      terrain(EAST_ROOF, {.x = 21, .y = 4, .width = 7}),
      building(NOODLE_SHOP, {.x = 20, .y = 5, .width = 6}),
      gone_from(building(KIOSK_WRECK, {.x = 20, .y = 3, .height = 2}),
                RUINED_DAY),
      terrain(EAST_LOWER_WALKWAY, {.x = 29, .y = 9, .width = 3}),
      building(EAST_RUBBLE, {.x = 30, .y = 9}),
      gone_from(terrain(ROTTEN_BRIDGE, {.x = 32, .y = 9, .width = 4}),
                RUINED_DAY),
      terrain(COLD_STORE_FLOOR, {.x = 36, .y = 8, .width = 4}),
      building(COLD_STORE, {.x = 37, .y = 6, .width = 3, .height = 3}),
      entrance(COLD_STORE_DOOR, {.x = 37, .y = 8, .width = 3}),
      ladder(PIER_LADDER, {.x = 36, .y = 8, .height = 5}),
      terrain(PIER, {.x = 36, .y = 12, .width = 4}),
      terrain(SUNKEN_TRAIN, {.x = 20, .y = 13, .width = 5}),
      from_day(terrain(DRIFTED_FERRY, {.x = 27, .y = 13, .width = 8}),
               CRACKED_DAY),
      {.id = NOODLE_VENDOR, .kind = ObjectKind::Npc, .area = {.x = 25, .y = 6}},
      ingredient_source(TOMATO_MACHINE, {.x = 11, .y = 9}, TOMATO),
      owned(ingredient_source(VENDOR_CART, {.x = 25, .y = 6}, FISH),
            NOODLE_VENDOR),
      owned(ingredient_source(SHOP_SHELF, {.x = 5, .y = 6}, KELP),
            KITCHEN_SHOP),
      ingredient_source(LOST_BASKET, {.x = 23, .y = 13}, MUSHROOM),
      ingredient_source(PICKLE_MACHINE, {.x = 21, .y = 13}, PICKLES),
      owned(ingredient_source(COLD_LOCKER, {.x = 39, .y = 8}, MEAT),
            COLD_STORE),
      ingredient_source(ROOF_BAG, {.x = 6, .y = 4}, TRUFFLE),
      from_day(owned(ingredient_source(FERRY_BAG, {.x = 30, .y = 13}, KELP),
                     DRIFTED_FERRY),
               CRACKED_DAY),
      terrain(TERMINAL_ROAD, {.x = 40, .y = 8, .width = 10}),
      terrain(TERMINAL_STAIR, {.x = 50, .y = 7}),
      terrain(TERMINAL_STAIR_TOP, {.x = 51, .y = 6}),
      terrain(UPPER_DECK, {.x = 52, .y = 5, .width = 9}),
      ladder(DECK_LADDER, {.x = 56, .y = 2, .height = 4}),
      terrain(TOP_CATWALK, {.x = 53, .y = 2, .width = 8}),
      ladder(DOCK_LADDER, {.x = 60, .y = 5, .height = 7}),
      terrain(DOCK, {.x = 56, .y = 11, .width = 8}),
      ingredient_source(TERMINAL_MACHINE, {.x = 44, .y = 8}, PICKLES),
      ingredient_source(DECK_BAG, {.x = 58, .y = 5}, MUSHROOM),
      ingredient_source(DOCK_CRATE, {.x = 57, .y = 11}, TOMATO),
  };
  for (std::size_t index = 0; index < WASHED_BAG_COUNT; ++index) {
    objects.push_back(washed_up(index));
  }
  return objects;
}

/// The flooded aquarium, teal inside: a lobby by the way back to the street,
/// stairs up to an upper catwalk and a ladder down to a lower gallery, and the
/// two halves of the aquarium joined by one span on each level. Damage follows
/// the day: from `CRACKED_DAY` the upper span is gone, and from `RUINED_DAY`
/// the lower span is gone too and a skywalk with two ladders carries the way
/// over.
std::vector<ObjectDefinition> aquarium() {
  std::vector<ObjectDefinition> objects{
      terrain(LOBBY_FLOOR, {.x = 0, .y = 10, .width = 12}),
      terrain(LOBBY_STAIR, {.x = 12, .y = 9}),
      terrain(WEST_CATWALK, {.x = 13, .y = 8, .width = 5}),
      gone_from(terrain(UPPER_SPAN, {.x = 18, .y = 8, .width = 4}),
                CRACKED_DAY),
      terrain(EAST_CATWALK, {.x = 22, .y = 8, .width = 13}),
      ladder(WEST_LADDER, {.x = 13, .y = 8, .height = 4}),
      terrain(WEST_GALLERY, {.x = 13, .y = 11, .width = 9}),
      gone_from(terrain(LOWER_SPAN, {.x = 22, .y = 11, .width = 4}),
                RUINED_DAY),
      terrain(EAST_GALLERY, {.x = 26, .y = 11, .width = 6}),
      ladder(EAST_GALLERY_LADDER, {.x = 30, .y = 8, .height = 4}),
      from_day(terrain(SKYWALK, {.x = 17, .y = 5, .width = 10}), RUINED_DAY),
      from_day(ladder(SKY_LADDER_WEST, {.x = 17, .y = 5, .height = 4}),
               RUINED_DAY),
      from_day(ladder(SKY_LADDER_EAST, {.x = 26, .y = 5, .height = 4}),
               RUINED_DAY),
      terrain(TANK_STAIR, {.x = 35, .y = 9}),
      terrain(TANK_STAIR_LOW, {.x = 36, .y = 10}),
      terrain(TANK_FLOOR, {.x = 37, .y = 11, .width = 7}),
      {.id = KEEPER, .kind = ObjectKind::Npc, .area = {.x = 7, .y = 10}},
      owned(ingredient_source(KEEPER_CART, {.x = 7, .y = 10}, FISH), KEEPER),
      owned(ingredient_source(GIFT_SHELF, {.x = 27, .y = 8}, KELP),
            EAST_CATWALK),
      ingredient_source(JELLY_MACHINE, {.x = 15, .y = 11}, MUSHROOM),
      owned(ingredient_source(TANK_LOCKER, {.x = 41, .y = 11}, MEAT),
            TANK_FLOOR),
  };
  std::ranges::transform(objects, objects.begin(), aquatic);
  return objects;
}

std::vector<ObjectDefinition> city() {
  auto objects = street();
  const auto flooded = aquarium();
  objects.insert(objects.end(), flooded.begin(), flooded.end());
  return objects;
}

/// The road surface by row, one character a cell: = road, ~ cracked, o
/// pothole, < and > the left and right end of a broken road, / tilted slab,
/// % rubble, _ planks, c buried car, and . no ground.
constexpr std::array<std::string_view, 16> GROUND_ROWS{
    "........................................",
    "........................................",
    "........................................",
    "........................................",
    "<=~==o=>.............<=~=o=>............",
    ".................<=>....................",
    "<=~=o==>............<=~==o=>............",
    "........................................",
    "=~=c~o=~>........<=~==~o==~=>.......<=~=",
    "........./=~%=___............/%=____....",
    "........................................",
    "........................................",
    "..............<~o==>................<=~=",
    "..................../ccc>..________.....",
    "........................................",
    "........................................",
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

/// The ground cells of `objects`' terrain on `map`, one road cell for each
/// cell a terrain object covers, so a map without a hand-drawn road shows its
/// floors.
std::vector<GroundCell>
ground_of_terrain(const std::vector<ObjectDefinition> &objects, MapId map,
                  std::int32_t from_column) {
  std::vector<GroundCell> cells;
  for (const auto &object : objects) {
    if (object.kind != ObjectKind::Terrain || object.map != map) {
      continue;
    }
    for (auto column = std::max(object.area.x, from_column);
         column < object.area.x + object.area.width; ++column) {
      for (auto row = object.area.y; row < object.area.y + object.area.height;
           ++row) {
        const bool plank = object.id == UPPER_SPAN || object.id == LOWER_SPAN ||
                           object.id == SKYWALK;
        cells.push_back(
            {.cell = {.x = column, .y = row},
             .kind = plank ? GroundKind::PlankBridge : GroundKind::Road});
      }
    }
  }
  return cells;
}

/// The street's hand-drawn road of the first columns, then a road under every
/// floor of the terminal beyond it.
std::vector<GroundCell>
street_ground(const std::vector<ObjectDefinition> &objects) {
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
  const auto terminal =
      ground_of_terrain(objects, MapId::Street,
                        static_cast<std::int32_t>(GROUND_ROWS.front().size()));
  cells.insert(cells.end(), terminal.begin(), terminal.end());
  return cells;
}

std::vector<Decoration> street_decorations() {
  using enum DecorationKind;
  return {
      {.kind = TiltedTrain, .area = {.x = 1, .y = 11, .width = 4}},
      {.kind = TiltedTrain, .area = {.x = 9, .y = 12, .width = 4}},
      {.kind = TiltedTrain, .area = {.x = 26, .y = 10, .width = 3}},
      {.kind = BuildingShell,
       .area = {.x = 10, .y = 1, .width = 3, .height = 4}},
      {.kind = BuildingShell,
       .area = {.x = 30, .y = 2, .width = 3, .height = 4}},
      {.kind = HangingCables, .area = {.x = 3, .y = 9, .width = 4}},
      {.kind = HangingCables, .area = {.x = 22, .y = 9, .width = 4}},
      {.kind = DeadNeonSign, .area = {.x = 13, .y = 4, .width = 2}},
      {.kind = DeadNeonSign, .area = {.x = 33, .y = 4, .width = 2}},
      {.kind = FleshGrowth, .area = {.x = 0, .y = 13, .width = 3, .height = 2}},
      {.kind = FleshGrowth,
       .area = {.x = 36, .y = 13, .width = 3, .height = 2}},
      {.kind = RubblePile, .area = {.x = 14, .y = 8}},
      {.kind = RubblePile, .area = {.x = 31, .y = 8}},
      {.kind = BrokenPillar, .area = {.x = 17, .y = 9, .height = 3}},
      {.kind = BrokenPillar, .area = {.x = 8, .y = 9, .height = 3}},
      {.kind = BrokenPillar, .area = {.x = 28, .y = 9, .height = 3}},
      {.kind = StreetLamp, .area = {.x = 8, .y = 7}},
      {.kind = StreetLamp, .area = {.x = 28, .y = 7}},
      {.kind = BuildingShell,
       .area = {.x = 57, .y = 6, .width = 5, .height = 5}},
      {.kind = HangingCables, .area = {.x = 42, .y = 9, .width = 4}},
      {.kind = StreetLamp, .area = {.x = 48, .y = 7}},
      {.kind = FleshGrowth,
       .area = {.x = 44, .y = 13, .width = 4, .height = 2}},
  };
}

std::vector<Decoration> aquarium_decorations() {
  using enum DecorationKind;
  return {
      {.kind = BuildingShell,
       .area = {.x = 14, .y = 1, .width = 6, .height = 6}},
      {.kind = HangingCables, .area = {.x = 3, .y = 6, .width = 5}},
      {.kind = HangingCables, .area = {.x = 31, .y = 5, .width = 4}},
      {.kind = FleshGrowth,
       .area = {.x = 22, .y = 13, .width = 4, .height = 2}},
      {.kind = FleshGrowth,
       .area = {.x = 38, .y = 13, .width = 4, .height = 2}},
      {.kind = DeadNeonSign, .area = {.x = 8, .y = 7, .width = 2}},
  };
}

std::vector<Landmark> street_landmarks() {
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
      {.id = PICKLE_MACHINE,
       .kind = SourceKind::VendingMachine,
       .remaining = 3},
      {.id = COLD_LOCKER, .kind = SourceKind::Shop, .remaining = 3},
      {.id = ROOF_BAG, .kind = SourceKind::Bag, .remaining = 2},
      {.id = FERRY_BAG, .kind = SourceKind::Bag, .remaining = 3},
      {.id = TERMINAL_MACHINE,
       .kind = SourceKind::VendingMachine,
       .remaining = 4},
      {.id = DECK_BAG, .kind = SourceKind::Bag, .remaining = 2},
      {.id = DOCK_CRATE, .kind = SourceKind::Bag, .remaining = 3},
      {.id = KEEPER_CART, .kind = SourceKind::Npc, .remaining = 4},
      {.id = GIFT_SHELF, .kind = SourceKind::Shop, .remaining = 3},
      {.id = JELLY_MACHINE, .kind = SourceKind::VendingMachine, .remaining = 3},
      {.id = TANK_LOCKER, .kind = SourceKind::Shop, .remaining = 3},
  };
  for (std::size_t index = 0; index < WASHED_BAG_COUNT; ++index) {
    sources.push_back({.id = washed_bag(index),
                       .kind = SourceKind::Bag,
                       .remaining = BAG_STOCK});
  }
  return sources;
}

/// The way to the aquarium at the end of the dock and the way back by its
/// lobby. Each arrival lies outside the zone of the map it arrives on.
std::vector<MapExit> exits() {
  return {{.from = MapId::Street,
           .zone = {.x = 62, .y = 10, .width = 2, .height = 2},
           .to = MapId::Aquarium,
           .arrival = {.x = 4, .y = 10}},
          {.from = MapId::Aquarium,
           .zone = {.x = 0, .y = 9, .width = 2, .height = 2},
           .to = MapId::Street,
           .arrival = {.x = 58, .y = 11}}};
}

/// The floors inside the street's three shops: the kitchen loft, the noodle
/// loft and the cold store.
std::vector<Interior> interiors() {
  return {{.building = KITCHEN_SHOP, .area = {.x = 0, .y = 6, .width = 6}},
          {.building = NOODLE_SHOP, .area = {.x = 20, .y = 6, .width = 6}},
          {.building = COLD_STORE,
           .area = {.x = 37, .y = 6, .width = 3, .height = 3}}};
}

/// The fixed obstacles, each at the end of a spur no route needs: the old acid
/// drip beside the base, a catwalk's end, a pier's end and the dead ends of
/// two aquarium floors.
std::vector<Obstacle> obstacles() {
  return {{.map = MapId::Street, .cell = {.x = 1, .y = 8}},
          {.map = MapId::Street, .cell = {.x = 54, .y = 2}},
          {.map = MapId::Street, .cell = {.x = 39, .y = 12}},
          {.map = MapId::Aquarium, .cell = {.x = 31, .y = 11}},
          {.map = MapId::Aquarium, .cell = {.x = 43, .y = 11}}};
}

/// One mutant for each patrol, none on a source's cell or the exit's zone: the
/// pier, the top catwalk and the east roof on the street, the east gallery
/// and the tank floor in the aquarium.
std::vector<Patrol> patrols() {
  return {{.map = MapId::Street,
           .from = {.x = 37, .y = 12},
           .to = {.x = 38, .y = 12}},
          {.map = MapId::Street,
           .from = {.x = 57, .y = 2},
           .to = {.x = 59, .y = 2}},
          {.map = MapId::Street,
           .from = {.x = 23, .y = 4},
           .to = {.x = 26, .y = 4}},
          {.map = MapId::Aquarium,
           .from = {.x = 27, .y = 11},
           .to = {.x = 29, .y = 11}},
          {.map = MapId::Aquarium,
           .from = {.x = 38, .y = 11},
           .to = {.x = 39, .y = 11}}};
}

/// The obstacle cells of `map`, which the screen still draws as acid drips.
std::vector<WorldArea> hazard_areas(MapId map) {
  std::vector<WorldArea> areas;
  for (const auto &obstacle : obstacles()) {
    if (obstacle.map == map) {
      areas.push_back({.x = obstacle.cell.x, .y = obstacle.cell.y});
    }
  }
  return areas;
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
  auto objects = city();
  auto street_cells = street_ground(objects);
  auto aquarium_cells = ground_of_terrain(objects, MapId::Aquarium, 0);
  return {
      .run = {.exploration_budget = DAY,
              .seed = seed,
              .full_acid = AcidLevel{100}},
      .objects = std::move(objects),
      .companion = {.exploration = {.base = {.x = 1, .y = 8},
                                    .entry = {.x = 5, .y = 8},
                                    .exits = exits(),
                                    .interiors = interiors(),
                                    .collection_distance = 1,
                                    .sources = stock(),
                                    .collection_time = 1500ms,
                                    .obstacles = obstacles(),
                                    .patrols = patrols(),
                                    .absence = 10s},
                    .workshop = {.recipes = recipes(),
                                 .new_food_score = 50,
                                 .new_ingredient_score = 5,
                                 .satisfying_score = 100,
                                 .disliked_acid_rise = AcidLevel{15},
                                 .unsatisfied_acid_rise = AcidLevel{10},
                                 .flavors = flavors(),
                                 .fine_dish_score = 30},
                    .unfed_penalty = AcidLevel{12}},
      .maps = {{.id = MapId::Street,
                .name = "Street",
                .size = {.width = 64, .height = 16},
                .hazards = hazard_areas(MapId::Street),
                .ground = std::move(street_cells),
                .decorations = street_decorations(),
                .landmarks = street_landmarks()},
               {.id = MapId::Aquarium,
                .name = "Aquarium",
                .size = {.width = 44, .height = 16},
                .hazards = hazard_areas(MapId::Aquarium),
                .ground = std::move(aquarium_cells),
                .decorations = aquarium_decorations(),
                .landmarks = {}}},
  };
}

const MapContent &map_of(const Content &content, MapId id) {
  const auto found = std::ranges::find(content.maps, id, &MapContent::id);
  if (found == content.maps.end()) {
    throw std::out_of_range("the content has no such map");
  }
  return *found;
}

namespace {

/// What a source is, as its place name ends.
const char *source_word(SourceKind kind) {
  switch (kind) {
  case SourceKind::VendingMachine:
    return "machine";
  case SourceKind::Npc:
    return "cart";
  case SourceKind::Shop:
    return "shop";
  case SourceKind::Bag:
    return "bag";
  }
  std::abort();
}

} // namespace

std::string_view ingredient_name(IngredientId ingredient) {
  return name_of(INGREDIENT_NAMES, ingredient);
}

std::string_view food_name(FoodId food) { return name_of(FOOD_NAMES, food); }

std::string_view object_name(ObjectId object) {
  return name_of(OBJECT_NAMES, object);
}

std::string place_display_name(const GameView &view, ObjectId object) {
  const auto &objects = view.city.objects;
  const auto &sources = view.exploration.sources;
  const auto found = std::ranges::find(objects, object, &ObjectSnapshot::id);
  const auto stock = std::ranges::find(sources, object, &SourceStock::id);
  if (found == objects.end() || stock == sources.end()) {
    return std::string(object_name(object));
  }
  if (const auto &ingredient = found->ingredient) {
    return std::string(ingredient_name(*ingredient)) + " " +
           source_word(stock->kind);
  }
  return std::string(object_name(object));
}

} // namespace neocity::survival
