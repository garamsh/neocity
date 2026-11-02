#include <survival/content.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <stdexcept>

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
constexpr ObjectId NORTH_STAIRS{5};
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
constexpr ObjectId WASHED_BAG_1{41};
constexpr ObjectId WASHED_BAG_2{42};
constexpr ObjectId WASHED_BAG_3{43};
constexpr ObjectId WASHED_BAG_4{44};
constexpr ObjectId WASHED_BAG_5{45};
constexpr ObjectId WASHED_BAG_6{46};
constexpr ObjectId WASHED_BAG_7{47};
constexpr ObjectId WASHED_BAG_8{48};

constexpr std::uint32_t BAG_STOCK = 14;

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
    Named<ObjectId>{.id = NORTH_STAIRS, .name = "North Stairs"},
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
    Named<ObjectId>{.id = WASHED_BAG_1, .name = "Washed-up Bag 1"},
    Named<ObjectId>{.id = WASHED_BAG_2, .name = "Washed-up Bag 2"},
    Named<ObjectId>{.id = WASHED_BAG_3, .name = "Washed-up Bag 3"},
    Named<ObjectId>{.id = WASHED_BAG_4, .name = "Washed-up Bag 4"},
    Named<ObjectId>{.id = WASHED_BAG_5, .name = "Washed-up Bag 5"},
    Named<ObjectId>{.id = WASHED_BAG_6, .name = "Washed-up Bag 6"},
    Named<ObjectId>{.id = WASHED_BAG_7, .name = "Washed-up Bag 7"},
    Named<ObjectId>{.id = WASHED_BAG_8, .name = "Washed-up Bag 8"},
};

template <typename Id, std::size_t Size>
std::string_view name_of(const std::array<Named<Id>, Size> &names, Id id) {
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

/// A bag the tide brings in once world time reaches `arrival`. Acid
/// never brings one: it carries the bag off at `gone`, or sinks it before it
/// lands, so rising acid only takes supply away.
ObjectDefinition washed_up(ObjectId id, GridPosition cell,
                           IngredientId ingredient,
                           std::chrono::seconds arrival, std::uint32_t gone) {
  return destroyed(
      exposed(ingredient_source(id, cell, ingredient), {.elapsed = arrival}),
      {.acid = AcidLevel{gone}});
}

ObjectDefinition passage(ObjectId id, ObjectKind kind, WorldArea area) {
  return {.id = id, .kind = kind, .area = area, .repair_with = WRENCH};
}

std::vector<ObjectDefinition> city() {
  constexpr auto ferry_time = 800s;
  const WorldThreshold ferry_arrives{.elapsed = ferry_time,
                                     .acid = AcidLevel{30}};
  return {
      terrain(BASE_DOCK, {.x = 0, .y = 5, .width = 4, .height = 5}),
      terrain(MAIN_STREET, {.x = 4, .y = 6, .width = 17, .height = 3}),
      destroyed(terrain(ROTTEN_BRIDGE, {.x = 21, .y = 7, .width = 4}),
                {.elapsed = 1080s, .acid = AcidLevel{40}}),
      terrain(EAST_PLAZA, {.x = 25, .y = 4, .width = 10, .height = 9}),
      terrain(NORTH_STAIRS, {.x = 8, .y = 3, .width = 2, .height = 3}),
      terrain(ARCADE, {.x = 4, .y = 1, .width = 16, .height = 2}),
      terrain(SOUTH_STAIRS, {.x = 12, .y = 9, .width = 2, .height = 3}),
      terrain(MARKET, {.x = 6, .y = 12, .width = 15, .height = 3}),
      terrain(ROOFTOP, {.x = 28, .y = 1, .width = 7, .height = 2}),
      exposed(
          terrain(DRIFTED_FERRY, {.x = 21, .y = 12, .width = 4, .height = 2}),
          ferry_arrives),
      destroyed(passage(ROOF_LADDER, ObjectKind::Ladder, {.x = 31, .y = 3}),
                {.elapsed = 240s, .acid = AcidLevel{10}}),
      destroyed(passage(COLD_STORE_DOOR, ObjectKind::Entrance,
                        {.x = 27, .y = 7, .width = 4}),
                {.elapsed = 400s, .acid = AcidLevel{20}}),
      destroyed({.id = CORNER_SHOP,
                 .kind = ObjectKind::Building,
                 .area = {.x = 14, .y = 4, .width = 4, .height = 2}},
                {.elapsed = 600s, .acid = AcidLevel{25}}),
      {.id = COLD_STORE,
       .kind = ObjectKind::Building,
       .area = {.x = 27, .y = 6, .width = 6, .height = 4}},
      destroyed({.id = KIOSK_WRECK,
                 .kind = ObjectKind::Building,
                 .area = {.x = 12, .y = 1, .width = 2, .height = 2}},
                {.elapsed = 360s}),
      destroyed({.id = NOODLE_VENDOR,
                 .kind = ObjectKind::Npc,
                 .area = {.x = 10, .y = 6}},
                {.elapsed = 480s, .acid = AcidLevel{20}}),
      destroyed(ingredient_source(TOMATO_MACHINE, {.x = 6, .y = 6}, TOMATO),
                {.acid = AcidLevel{50}}),
      owned(ingredient_source(VENDOR_CART, {.x = 12, .y = 6}, FISH),
            NOODLE_VENDOR),
      owned(ingredient_source(SHOP_SHELF, {.x = 15, .y = 5}, KELP),
            CORNER_SHOP),
      destroyed(ingredient_source(LOST_BASKET, {.x = 9, .y = 13}, MUSHROOM),
                {.acid = AcidLevel{40}}),
      tool_source(TOOL_BAG, {.x = 16, .y = 13}, WRENCH),
      destroyed(ingredient_source(PICKLE_MACHINE, {.x = 6, .y = 2}, PICKLES),
                {.acid = AcidLevel{35}}),
      tool_source(ARCADE_BAG, {.x = 17, .y = 1}, MAGNIFIER),
      tool_source(SUPPLY_CRATE, {.x = 33, .y = 5}, SPATULA),
      owned(ingredient_source(COLD_LOCKER, {.x = 31, .y = 7}, MEAT),
            COLD_STORE),
      ingredient_source(ROOF_BAG, {.x = 33, .y = 1}, TRUFFLE),
      exposed(owned(ingredient_source(FERRY_BAG, {.x = 23, .y = 12}, KELP),
                    DRIFTED_FERRY),
              {.elapsed = ferry_time}),
      washed_up(WASHED_BAG_1, {.x = 5, .y = 8}, FISH, 120s, 40),
      washed_up(WASHED_BAG_2, {.x = 12, .y = 13}, MUSHROOM, 250s, 50),
      washed_up(WASHED_BAG_3, {.x = 4, .y = 1}, TOMATO, 380s, 55),
      washed_up(WASHED_BAG_4, {.x = 26, .y = 11}, PICKLES, 510s, 60),
      washed_up(WASHED_BAG_5, {.x = 7, .y = 7}, KELP, 640s, 65),
      washed_up(WASHED_BAG_6, {.x = 16, .y = 8}, MEAT, 980s, 75),
      washed_up(WASHED_BAG_7, {.x = 12, .y = 10}, TRUFFLE, 1160s, 85),
      washed_up(WASHED_BAG_8, {.x = 30, .y = 12}, TOMATO, 770s, 70),
  };
}

std::vector<SourceStock> stock() {
  return {
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
      {.id = WASHED_BAG_1, .kind = SourceKind::Bag, .remaining = BAG_STOCK},
      {.id = WASHED_BAG_2, .kind = SourceKind::Bag, .remaining = BAG_STOCK},
      {.id = WASHED_BAG_3, .kind = SourceKind::Bag, .remaining = BAG_STOCK},
      {.id = WASHED_BAG_4, .kind = SourceKind::Bag, .remaining = BAG_STOCK},
      {.id = WASHED_BAG_5, .kind = SourceKind::Bag, .remaining = BAG_STOCK},
      {.id = WASHED_BAG_6, .kind = SourceKind::Bag, .remaining = BAG_STOCK},
      {.id = WASHED_BAG_7, .kind = SourceKind::Bag, .remaining = BAG_STOCK},
      {.id = WASHED_BAG_8, .kind = SourceKind::Bag, .remaining = BAG_STOCK},
  };
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
      .run = {.exploration_budget = 45s,
              .seed = seed,
              .full_acid = AcidLevel{100}},
      .objects = city(),
      .companion = {.exploration = {.base = {.x = 1, .y = 7},
                                    .entry = {.x = 3, .y = 7},
                                    .maximum_stamina = 20,
                                    .collection_distance = 1,
                                    .sources = stock(),
                                    .collection_time = 12s},
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
      .hazards = {{.x = 18, .y = 6, .width = 1, .height = 3}},
      .map = {.width = 36, .height = 16},
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
