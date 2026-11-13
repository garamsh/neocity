#include <survival/content.hpp>
#include <survival/escape.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <iterator>
#include <map>
#include <numeric>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace neocity::survival {
namespace {

using namespace std::chrono_literals;

constexpr ControlMap ALL_AI{.chef = Controller::Ai,
                            .appraiser = Controller::Ai};
constexpr std::array ACTORS{Actor::Chef, Actor::Appraiser};
constexpr auto STEP = 100ms;
constexpr std::uint32_t STEP_LIMIT = 400'000;
/// The last day the path check and the supply scans look at: past it every
/// washed-up bag has gone and the stages no longer change.
constexpr std::uint32_t LAST_SCANNED_DAY = 60;
/// The share of cooking attempts that turn into a dish.
constexpr std::uint32_t SUCCESS_PERCENT = 90;

/// The temporary game's features, advanced with a fresh view per actor.
class Game {
public:
  explicit Game(Content settings)
      : data(std::move(settings)), shared(data.run), map(data.objects),
        explorers(shared, map, data.companion.exploration),
        kitchen_work(shared, data.companion.workshop),
        controllers(data.companion) {}

  [[nodiscard]] const Content &content() const { return data; }
  [[nodiscard]] Simulation &simulation() { return shared; }
  [[nodiscard]] World &world() { return map; }
  [[nodiscard]] Exploration &exploration() { return explorers; }
  [[nodiscard]] Workshop &workshop() { return kitchen_work; }

  [[nodiscard]] GameView view() const {
    return {.progression = shared.snapshot(),
            .world = on_map(map.snapshot(), explorers.snapshot().map),
            .city = map.snapshot(),
            .exploration = explorers.snapshot(),
            .workshop = kitchen_work.snapshot()};
  }

  void step(ControlMap control, const std::array<PlayerInput, 2> &inputs = {}) {
    std::array<ExplorationInput, 2> movement;
    for (std::size_t index = 0; index < ACTORS.size(); ++index) {
      const auto command = controllers.command(ACTORS.at(index), control,
                                               inputs.at(index), view());
      movement.at(index) = command.movement;
      if (command.action) {
        (void)apply(explorers, kitchen_work, ACTORS.at(index), *command.action);
      }
      if (run_over(shared.snapshot().phase)) {
        return;
      }
    }
    ASSERT_EQ(explorers.advance(STEP, movement), Result::Applied);
  }

private:
  Content data;
  Simulation shared;
  World map;
  Exploration explorers;
  Workshop kitchen_work;
  Companion controllers;
};

/// Runs AI-only days until `last_day` begins or the run ends, or gives up at
/// the step limit.
std::uint32_t play_days(Game &game, std::uint32_t last_day) {
  for (std::uint32_t step = 0;
       step < STEP_LIMIT && game.simulation().snapshot().day < last_day &&
       !run_over(game.simulation().snapshot().phase);
       ++step) {
    game.step(ALL_AI);
  }
  return game.simulation().snapshot().day;
}

/// Counts the days before `last_day` that ended with nothing fed.
std::uint32_t unfed_days(Game &game, std::uint32_t last_day) {
  std::uint32_t unfed = 0;
  std::uint32_t counted = 0;
  for (std::uint32_t step = 0;
       step < STEP_LIMIT && game.simulation().snapshot().day < last_day &&
       !run_over(game.simulation().snapshot().phase);
       ++step) {
    game.step(ALL_AI);
    const auto last = game.workshop().snapshot().last_day;
    if (last && last->completed_day != counted) {
      counted = last->completed_day;
      unfed += last->reaction == MonsterReaction::Unfed ? 1U : 0U;
    }
  }
  return unfed;
}

std::set<Move> every_move() {
  return {Move::Left,      Move::Right,    Move::ClimbUp,
          Move::ClimbDown, Move::JumpLeft, Move::JumpRight};
}

const MapContent &street_of(const Content &content) {
  return map_of(content, MapId::Street);
}

/// The obstacle cells of `map`.
std::vector<GridPosition> obstacle_cells(const Content &content, MapId map) {
  std::vector<GridPosition> cells;
  for (const auto &obstacle : content.companion.exploration.obstacles) {
    if (obstacle.map == map) {
      cells.push_back(obstacle.cell);
    }
  }
  return cells;
}

/// Where a party starts on `map`: the street entrance, or the cell the way in
/// puts it on.
GridPosition start_of(const Content &content, MapId map) {
  const auto &exploration = content.companion.exploration;
  if (map == MapId::Street) {
    return exploration.entry;
  }
  const auto way_in = std::ranges::find(exploration.exits, map, &MapExit::to);
  if (way_in == exploration.exits.end()) {
    throw std::out_of_range("no way into the map");
  }
  return way_in->arrival;
}

/// The cells a party can reach on `map` from where it starts there by
/// `allowed` moves.
std::vector<GridPosition>
cells_reached(const Content &content, const WorldSnapshot &world,
              const std::set<Move> &allowed = every_move(),
              MapId on = MapId::Street) {
  const auto start = start_of(content, on);
  const CellMap map(on_map(world, on), obstacle_cells(content, on));
  std::vector<GridPosition> seen{start};
  std::deque<GridPosition> frontier{start};
  while (!frontier.empty()) {
    const auto cell = frontier.front();
    frontier.pop_front();
    for (const auto &link : links(map, cell)) {
      if (allowed.contains(link.move) &&
          std::ranges::find(seen, link.to) == seen.end()) {
        seen.push_back(link.to);
        frontier.push_back(link.to);
      }
    }
  }
  return seen;
}

/// Whether a party can stand within `range` of `area` on `map`.
bool within_reach(const std::vector<GridPosition> &reached,
                  const WorldArea &area, std::uint32_t range) {
  return std::ranges::any_of(reached, [&](GridPosition cell) {
    return distance(cell, area) <= static_cast<std::int32_t>(range);
  });
}

bool reachable(const Content &content, const WorldSnapshot &world,
               const ObjectSnapshot &object) {
  return within_reach(cells_reached(content, world, every_move(), object.map),
                      object.area,
                      content.companion.exploration.collection_distance);
}

/// The source's world object, as a snapshot.
ObjectSnapshot object_of(const WorldSnapshot &world, ObjectId id) {
  const auto found = std::ranges::find(world.objects, id, &ObjectSnapshot::id);
  if (found == world.objects.end()) {
    throw std::out_of_range("no such object");
  }
  return *found;
}

/// What the path check finds wrong with `world`, in words: each map is walked
/// from where a party starts on it with the moves the game allows, over the
/// same cells and links movement uses, and every source on offer, every exit
/// and every interior zone must be reached.
std::vector<std::string> path_problems(const Content &content,
                                       const WorldSnapshot &world) {
  std::vector<std::string> problems;
  const auto &config = content.companion.exploration;
  for (const auto &map : content.maps) {
    if (!stands_on(
            CellMap(on_map(world, map.id), obstacle_cells(content, map.id)),
            start_of(content, map.id))) {
      problems.push_back(std::string(map.name) + ": no footing at the start");
    }
    const auto reached = cells_reached(content, world, every_move(), map.id);
    for (const auto &stock : config.sources) {
      const auto object = object_of(world, stock.id);
      if (object.map == map.id && object.material_available &&
          !within_reach(reached, object.area, config.collection_distance)) {
        problems.push_back(std::string(map.name) + ": " +
                           std::string(object_name(stock.id)) +
                           " is out of reach");
      }
    }
    for (const auto &exit : config.exits) {
      if (exit.from == map.id && !within_reach(reached, exit.zone, 0)) {
        problems.push_back(std::string(map.name) + ": the exit to " +
                           std::string(map_of(content, exit.to).name) +
                           " is out of reach");
      }
    }
    if (map.id == MapId::Street) {
      for (const auto &interior : config.interiors) {
        if (!within_reach(reached, interior.area, 0)) {
          problems.push_back(std::string(map.name) + ": the inside of " +
                             std::string(object_name(interior.building)) +
                             " is out of reach");
        }
      }
    }
  }
  return problems;
}

/// The sources a party can stand in reach of on `day`, without any passage
/// repaired.
std::set<std::uint32_t> collectable_sources(const Content &content,
                                            const WorldSnapshot &world) {
  std::set<std::uint32_t> found;
  for (const auto &stock : content.companion.exploration.sources) {
    const auto object = object_of(world, stock.id);
    if (object.material_available && reachable(content, world, object)) {
      found.insert(stock.id.value);
    }
  }
  return found;
}

/// The sources reached on any day of a run.
std::set<std::uint32_t> sources_ever_reached(const Content &content) {
  std::set<std::uint32_t> found;
  World world(content.objects);
  for (std::uint32_t day = 1; day <= LAST_SCANNED_DAY; ++day) {
    if (world.update(day) != WorldUpdateResult::Applied) {
      throw std::logic_error("the day went backwards");
    }
    found.merge(collectable_sources(content, world.snapshot()));
  }
  return found;
}

/// The world on `day`.
WorldSnapshot on_day(const Content &content, std::uint32_t day) {
  World world(content.objects);
  if (world.update(day) != WorldUpdateResult::Applied) {
    throw std::logic_error("the day went backwards");
  }
  return world.snapshot();
}

/// Ingredient stock held by the sources in `sources`, by identity value.
std::map<std::uint32_t, std::uint32_t>
stock_in(const Content &content, const std::set<std::uint32_t> &sources) {
  std::map<std::uint32_t, std::uint32_t> totals;
  for (const auto &stock : content.companion.exploration.sources) {
    const auto object =
        std::ranges::find(content.objects, stock.id, &ObjectDefinition::id);
    if (object == content.objects.end() || !sources.contains(stock.id.value)) {
      continue;
    }
    if (const auto ingredient = object->ingredient) {
      totals[ingredient->value] += stock.remaining;
    }
  }
  return totals;
}

/// Dishes `stock` makes, none disliked, always spending the most plentiful
/// kinds first; a lower bound on what the best mix makes.
std::uint32_t wanted_dishes(const std::vector<Recipe> &recipes,
                            std::map<std::uint32_t, std::uint32_t> stock) {
  std::uint32_t dishes = 0;
  while (true) {
    const Recipe *best = nullptr;
    std::uint32_t most = 0;
    for (const auto &recipe : recipes) {
      std::uint32_t least = STEP_LIMIT;
      for (const auto ingredient : recipe.ingredients) {
        least = std::min(least, stock[ingredient.value]);
      }
      if (!recipe.disliked && least > most) {
        most = least;
        best = &recipe;
      }
    }
    if (best == nullptr) {
      return dishes;
    }
    for (const auto ingredient : best->ingredients) {
      --stock[ingredient.value];
    }
    ++dishes;
  }
}

ObjectSnapshot object_named(const WorldSnapshot &world, std::string_view name) {
  const auto found =
      std::ranges::find_if(world.objects, [name](const ObjectSnapshot &object) {
        return object_name(object.id) == name;
      });
  if (found == world.objects.end()) {
    throw std::out_of_range("no such object");
  }
  return *found;
}

TEST(TemporaryContent, APlaceHasTheSameNameOnEveryMap) {
  const Game game(temporary_content(7));
  const auto street = game.view();
  ASSERT_EQ(street.exploration.map, MapId::Street);
  auto aquarium = street;
  aquarium.exploration.map = MapId::Aquarium;
  aquarium.world = on_map(street.city, MapId::Aquarium);
  ASSERT_NE(aquarium.world.objects.size(), street.world.objects.size());
  std::size_t named_by_ingredient = 0;
  for (const auto &object : street.city.objects) {
    const auto name = place_display_name(street, object.id);
    EXPECT_EQ(name, place_display_name(aquarium, object.id))
        << object_name(object.id);
    if (name != object_name(object.id)) {
      ++named_by_ingredient;
    }
  }
  // Control: some places are named by what they hold, so the comparison
  // could tell a short fallback name from the printed one.
  EXPECT_GT(named_by_ingredient, 0U);
  const auto on_aquarium = std::ranges::find_if(
      street.city.objects, [](const ObjectSnapshot &object) {
        return object.map == MapId::Aquarium && object.ingredient;
      });
  ASSERT_NE(on_aquarium, street.city.objects.end());
  EXPECT_NE(place_display_name(street, on_aquarium->id),
            object_name(on_aquarium->id));
}

TEST(TemporaryContent, ConstructsEveryFeatureWithinItsMap) {
  Game game(temporary_content(7));
  EXPECT_EQ(game.simulation().snapshot().seed, 7U);
  for (const auto &object : game.content().objects) {
    const auto &map = map_of(game.content(), object.map);
    EXPECT_GE(object.area.x, 0);
    EXPECT_GE(object.area.y, 0);
    EXPECT_LE(object.area.x + object.area.width, map.size.width);
    EXPECT_LE(object.area.y + object.area.height, map.size.height);
    EXPECT_FALSE(object_name(object.id).empty());
  }
}

/// Every cell `area` covers.
std::set<std::pair<std::int32_t, std::int32_t>>
cells_of(const WorldArea &area) {
  std::set<std::pair<std::int32_t, std::int32_t>> cells;
  for (auto x = area.x; x < area.x + area.width; ++x) {
    for (auto y = area.y; y < area.y + area.height; ++y) {
      cells.insert({x, y});
    }
  }
  return cells;
}

std::map<std::pair<std::int32_t, std::int32_t>, GroundKind>
ground_of(const MapContent &map) {
  std::map<std::pair<std::int32_t, std::int32_t>, GroundKind> ground;
  for (const auto &cell : map.ground) {
    EXPECT_TRUE(ground.insert({{cell.cell.x, cell.cell.y}, cell.kind}).second)
        << cell.cell.x << ", " << cell.cell.y;
  }
  return ground;
}

TEST(TemporaryContent, GroundNamesTheSurfaceOfEveryTerrainCellAndNoOther) {
  const auto content = temporary_content(1);
  for (const auto &map : content.maps) {
    std::set<std::pair<std::int32_t, std::int32_t>> terrain;
    for (const auto &object : content.objects) {
      if (object.kind == ObjectKind::Terrain && object.map == map.id) {
        terrain.merge(cells_of(object.area));
      }
    }
    std::set<std::pair<std::int32_t, std::int32_t>> ground;
    for (const auto &[cell, kind] : ground_of(map)) {
      ground.insert(cell);
    }
    EXPECT_EQ(ground, terrain) << map.name;
  }
}

TEST(TemporaryContent, RubbleBlocksItsCellForTheWholeRun) {
  const auto content = temporary_content(1);
  const auto ground = ground_of(street_of(content));
  World world(content.objects);
  const auto rubble_blocks = [&] {
    std::size_t rubble = 0;
    for (const auto &[cell, kind] : ground) {
      if (kind == GroundKind::Rubble) {
        ++rubble;
        EXPECT_EQ(CellMap(on_map(world.snapshot(), MapId::Street))
                      .at({.x = cell.first, .y = cell.second}),
                  Cell::Blocked)
            << cell.first << ", " << cell.second;
      }
    }
    return rubble;
  };
  EXPECT_GT(rubble_blocks(), 0U);
  ASSERT_EQ(world.update(LAST_SCANNED_DAY), WorldUpdateResult::Applied);
  EXPECT_GT(rubble_blocks(), 0U);
}

TEST(TemporaryContent, ABrokenRoadEndsWhereItsGroundStops) {
  const auto ground = ground_of(street_of(temporary_content(1)));
  std::size_t ends = 0;
  for (const auto &[cell, kind] : ground) {
    const auto [x, y] = cell;
    if (kind == GroundKind::RoadEndLeft) {
      ++ends;
      EXPECT_FALSE(ground.contains({x - 1, y})) << x << ", " << y;
    }
    if (kind == GroundKind::RoadEndRight) {
      ++ends;
      EXPECT_FALSE(ground.contains({x + 1, y})) << x << ", " << y;
    }
  }
  EXPECT_GT(ends, 0U);
}

TEST(TemporaryContent, EveryWalkwayIsOneRowWithOpenAirAboveIt) {
  const auto content = temporary_content(1);
  std::size_t walkways = 0;
  for (const auto &object : content.objects) {
    if (object.kind == ObjectKind::Terrain) {
      ++walkways;
      EXPECT_EQ(object.area.height, 1) << object_name(object.id);
    }
  }
  EXPECT_GT(walkways, 0U);
  for (const auto &map : content.maps) {
    const auto ground = ground_of(map);
    for (const auto &[cell, kind] : ground) {
      EXPECT_FALSE(ground.contains({cell.first, cell.second - 1}))
          << map.name << " " << cell.first << ", " << cell.second;
    }
  }
}

TEST(TemporaryContent, FloorsMeetOnlyByLadderStairsRampOrJump) {
  const auto content = temporary_content(1);
  const World world(content.objects);
  const auto every_way = cells_reached(content, world.snapshot());
  const auto walking =
      cells_reached(content, world.snapshot(), {Move::Left, Move::Right});
  const auto no_jump =
      cells_reached(content, world.snapshot(),
                    {Move::Left, Move::Right, Move::ClimbUp, Move::ClimbDown});
  const auto no_climb =
      cells_reached(content, world.snapshot(),
                    {Move::Left, Move::Right, Move::JumpLeft, Move::JumpRight});
  EXPECT_LT(walking.size(), no_jump.size());
  EXPECT_LT(no_jump.size(), every_way.size());
  EXPECT_LT(no_climb.size(), every_way.size());
  std::set<std::int32_t> rows;
  for (const auto cell : every_way) {
    rows.insert(cell.y);
  }
  EXPECT_GE(rows.size(), 6U);
}

TEST(TemporaryContent,
     NoStraightRunOfGroundIsLongerThanAThirdOfTheStreetOrHalfTheAquarium) {
  const auto content = temporary_content(1);
  for (const auto &map : content.maps) {
    const auto ground = ground_of(map);
    // The aquarium is wider per floor, so its floors may run half its width.
    const auto longest = map.size.width / (map.id == MapId::Street ? 3 : 2);
    for (std::int32_t y = 0; y < map.size.height; ++y) {
      std::int32_t run = 0;
      for (std::int32_t x = 0; x < map.size.width; ++x) {
        run = ground.contains({x, y}) ? run + 1 : 0;
        EXPECT_LE(run, longest) << map.name << " row " << y << " at " << x;
      }
    }
  }
}

TEST(TemporaryContent, EveryDecorationKindStandsInsideItsMap) {
  const auto content = temporary_content(1);
  std::set<DecorationKind> kinds;
  for (const auto &map : content.maps) {
    for (const auto &decoration : map.decorations) {
      kinds.insert(decoration.kind);
      EXPECT_GE(decoration.area.x, 0);
      EXPECT_GE(decoration.area.y, 0);
      EXPECT_LE(decoration.area.x + decoration.area.width, map.size.width);
      EXPECT_LE(decoration.area.y + decoration.area.height, map.size.height);
    }
  }
  EXPECT_EQ(kinds.size(), 8U);
}

TEST(TemporaryContent, EveryLandmarkPictureMarksOneObjectThatCanShowIt) {
  const auto content = temporary_content(1);
  std::map<LandmarkKind, std::uint32_t> pictures;
  std::set<std::uint32_t> marked;
  for (const auto &map : content.maps) {
    for (const auto &landmark : map.landmarks) {
      const auto object = std::ranges::find(content.objects, landmark.object,
                                            &ObjectDefinition::id);
      ASSERT_NE(object, content.objects.end());
      EXPECT_EQ(object->map, map.id) << object_name(landmark.object);
      const bool building = landmark.kind == LandmarkKind::Kiosk ||
                            landmark.kind == LandmarkKind::ColdStore;
      EXPECT_EQ(object->kind,
                building ? ObjectKind::Building : ObjectKind::Terrain)
          << object_name(landmark.object);
      EXPECT_TRUE(marked.insert(landmark.object.value).second)
          << object_name(landmark.object);
      ++pictures[landmark.kind];
    }
  }
  EXPECT_EQ(pictures.size(), 5U);
  for (const auto &[kind, count] : pictures) {
    EXPECT_EQ(count, 1U);
  }
}

TEST(TemporaryContent, EveryNameLookupRefusesAnIdentityItNeverIssued) {
  EXPECT_EQ(ingredient_name(IngredientId{1}), "Tomato");
  EXPECT_THROW((void)ingredient_name(IngredientId{999}), std::out_of_range);
  EXPECT_THROW((void)food_name(FoodId{999}), std::out_of_range);
  EXPECT_THROW((void)object_name(ObjectId{999}), std::out_of_range);
}

TEST(TemporaryContent, EveryRecipeIngredientHasASource) {
  const auto content = temporary_content(1);
  const auto supplies = [&](auto member, auto id) {
    return std::ranges::any_of(content.objects, [&](const auto &object) {
      return std::invoke(member, object) == std::optional{id};
    });
  };
  for (const auto &recipe : content.companion.workshop.recipes) {
    EXPECT_FALSE(food_name(recipe.food).empty());
    for (const auto ingredient : recipe.ingredients) {
      EXPECT_TRUE(supplies(&ObjectDefinition::ingredient, ingredient));
    }
  }
}

TEST(TemporaryContent, EveryStockedSourceCanBeReachedWhileItSupplies) {
  const auto content = temporary_content(1);
  const auto reached = sources_ever_reached(content);
  for (const auto &stock : content.companion.exploration.sources) {
    EXPECT_TRUE(reached.contains(stock.id.value)) << object_name(stock.id);
  }
}

TEST(TemporaryContent, EveryDayBagsLandOnBothSidesOfTheCity) {
  const auto content = temporary_content(1);
  World world(content.objects);
  std::set<std::uint32_t> seen;
  for (std::uint32_t day = 2; day <= 40; ++day) {
    ASSERT_EQ(world.update(day), WorldUpdateResult::Applied);
    std::vector<GridPosition> landed;
    for (const auto &object : world.snapshot().objects) {
      if (object_name(object.id).starts_with("Washed-up Bag") &&
          object.material_available && !seen.contains(object.id.value)) {
        seen.insert(object.id.value);
        landed.push_back({.x = object.area.x, .y = object.area.y});
      }
    }
    EXPECT_EQ(landed.size(), 3U) << "day " << day;
    const auto east = std::ranges::count_if(
        landed, [](GridPosition cell) { return cell.x >= 20; });
    EXPECT_EQ(east, 1) << "day " << day;
  }
}

TEST(TemporaryContent, NoTwoSourcesShareACellOfAMapWhileEitherIsOnIt) {
  const auto content = temporary_content(1);
  World world(content.objects);
  for (std::uint32_t day = 1; day <= LAST_SCANNED_DAY; ++day) {
    ASSERT_EQ(world.update(day), WorldUpdateResult::Applied);
    std::set<std::tuple<MapId, std::int32_t, std::int32_t>> cells;
    std::size_t standing = 0;
    for (const auto &stock : content.companion.exploration.sources) {
      const auto object = object_of(world.snapshot(), stock.id);
      if (object.state == ObjectState::Intact) {
        ++standing;
        EXPECT_TRUE(
            cells.insert({object.map, object.area.x, object.area.y}).second)
            << object_name(object.id) << " on day " << day;
      }
    }
    EXPECT_GT(standing, 0U);
  }
}

TEST(TemporaryContent, TheCrackedStageBringsTheFerryAndItsStock) {
  const auto content = temporary_content(1);
  const auto before = on_day(content, STAGE_FIRST_DAY.at(1) - 1);
  EXPECT_EQ(object_named(before, "Drifted Ferry").state, ObjectState::Dormant);
  EXPECT_FALSE(object_named(before, "Ferry Bag").material_available);
  const auto after = on_day(content, STAGE_FIRST_DAY.at(1));
  EXPECT_EQ(object_named(after, "Drifted Ferry").state, ObjectState::Intact);
  EXPECT_TRUE(object_named(after, "Ferry Bag").material_available);
}

TEST(TemporaryContent, EveryRecipeCanBeCookedFromReachableStock) {
  const auto content = temporary_content(1);
  const auto stock = stock_in(content, sources_ever_reached(content));
  for (const auto &recipe : content.companion.workshop.recipes) {
    std::map<std::uint32_t, std::uint32_t> needed;
    for (const auto ingredient : recipe.ingredients) {
      ++needed[ingredient.value];
    }
    for (const auto &[ingredient, count] : needed) {
      const auto held = stock.find(ingredient);
      ASSERT_NE(held, stock.end()) << food_name(recipe.food);
      EXPECT_GE(held->second, count) << food_name(recipe.food);
    }
  }
}

TEST(TemporaryContent,
     ReachableStockCoversBurnedDishesUpToTheFirstEscapeCheck) {
  const auto content = temporary_content(1);
  const auto stock = stock_in(content, sources_ever_reached(content));
  const auto dishes = wanted_dishes(content.companion.workshop.recipes, stock);
  EXPECT_GE(dishes * SUCCESS_PERCENT, TEMPORARY_ESCAPE.first_day * 100U)
      << dishes << " dishes";
}

TEST(TemporaryContent, TheFirstDayReachesASourceOfEveryKind) {
  Game game(temporary_content(1));
  const auto world = game.world().snapshot();
  std::set<SourceKind> kinds;
  for (const auto &stock : game.exploration().snapshot().sources) {
    const auto object =
        std::ranges::find(world.objects, stock.id, &ObjectSnapshot::id);
    ASSERT_NE(object, world.objects.end());
    if (object->material_available &&
        reachable(game.content(), world, *object)) {
      kinds.insert(stock.kind);
    }
  }
  EXPECT_EQ(kinds.size(), 4U);
}

TEST(TemporaryContent, OnFiveSeedsAnAiPartyReachesDayThirtyWithAcidLeft) {
  for (const std::uint32_t seed : {1U, 2U, 3U, 4U, 5U}) {
    Game game(temporary_content(seed));
    EXPECT_EQ(play_days(game, 30), 30U) << "seed " << seed;
    const auto state = game.simulation().snapshot();
    EXPECT_EQ(state.phase == Phase::Lost, false) << "seed " << seed;
    EXPECT_GT(state.acid.value, 0U) << "seed " << seed;
  }
}

TEST(TemporaryContent, AnAiPartyStillCollectsAfterDayFourteen) {
  for (const std::uint32_t seed : {1U, 2U, 3U}) {
    Game game(temporary_content(seed));
    ASSERT_EQ(play_days(game, 15), 15U) << "seed " << seed;
    const auto before = game.exploration().snapshot().sources;
    ASSERT_EQ(play_days(game, 25), 25U) << "seed " << seed;
    const auto after = game.exploration().snapshot().sources;
    const auto total = [](const std::vector<SourceStock> &sources) {
      return std::accumulate(sources.begin(), sources.end(), 0U,
                             [](std::uint32_t sum, const SourceStock &source) {
                               return sum + source.remaining;
                             });
    };
    EXPECT_LT(total(after), total(before)) << "seed " << seed;
  }
}

TEST(TemporaryContent, TheOldAcidDripIsAnObstacleOnTheStreet) {
  const auto content = temporary_content(1);
  const auto &hazards = street_of(content).hazards;
  ASSERT_EQ(hazards.size(), 3U);
  const auto cells = obstacle_cells(content, MapId::Street);
  ASSERT_EQ(cells.size(), 3U);
  for (const auto &hazard : hazards) {
    EXPECT_EQ(std::ranges::count(cells, GridPosition{hazard.x, hazard.y}), 1)
        << hazard.x << ", " << hazard.y;
  }
}

TEST(TemporaryContent, EveryMapHasEnoughMutantsAndObstacles) {
  const auto content = temporary_content(1);
  const auto &config = content.companion.exploration;
  const auto count = [](const auto &list, MapId map) {
    return std::ranges::count(
        list, map, &std::remove_cvref_t<decltype(list)>::value_type::map);
  };
  EXPECT_GE(count(config.patrols, MapId::Street), 3);
  EXPECT_GE(count(config.obstacles, MapId::Street), 3);
  EXPECT_GE(count(config.patrols, MapId::Aquarium), 2);
  EXPECT_GE(count(config.obstacles, MapId::Aquarium), 2);
}

TEST(TemporaryContent, EveryPatrolAndObstacleStandsOnAFloorOnEveryDay) {
  const auto content = temporary_content(1);
  const auto &config = content.companion.exploration;
  for (std::uint32_t day = 1; day <= LAST_SCANNED_DAY; ++day) {
    const auto world = on_day(content, day);
    for (const auto &obstacle : config.obstacles) {
      EXPECT_TRUE(
          stands_on(CellMap(on_map(world, obstacle.map)), obstacle.cell))
          << "obstacle " << obstacle.cell.x << ", " << obstacle.cell.y
          << " on day " << day;
    }
    for (const auto &patrol : config.patrols) {
      const CellMap cells(on_map(world, patrol.map));
      for (auto x = patrol.from.x; x <= patrol.to.x; ++x) {
        EXPECT_TRUE(stands_on(cells, {.x = x, .y = patrol.from.y}))
            << "patrol cell " << x << ", " << patrol.from.y << " on day "
            << day;
      }
    }
  }
}

TEST(TemporaryContent, TheAcidGaugeFillsAtAValueOfTheRules) {
  EXPECT_EQ(temporary_content(1).run.full_acid, AcidLevel{100});
}

TEST(TemporaryContent,
     OnlyAVeryTastyFineRepeatSatisfiesAndEveryWantedNewDishCanWhenCookedFine) {
  const auto content = temporary_content(1);
  const auto &work = content.companion.workshop;
  std::uint32_t fine_repeats = 0;
  std::uint32_t short_repeats = 0;
  for (const auto &recipe : work.recipes) {
    const auto taste = taste_of(recipe, work);
    EXPECT_LT(taste, work.satisfying_score) << food_name(recipe.food);
    const bool satisfies =
        taste + work.fine_dish_score >= work.satisfying_score;
    fine_repeats += satisfies && !recipe.disliked ? 1U : 0U;
    short_repeats += satisfies ? 0U : 1U;
    if (!recipe.disliked) {
      EXPECT_GE(work.new_food_score + taste + work.fine_dish_score,
                work.satisfying_score)
          << food_name(recipe.food);
    }
  }
  EXPECT_GT(fine_repeats, 0U);
  EXPECT_GT(short_repeats, 0U);
}

TEST(TemporaryContent, TheFirstDayEndsByTimeWhileStockRemains) {
  const auto first_trip = [](Content content) {
    Game game(std::move(content));
    game.step(ALL_AI);
    for (std::uint32_t step = 0;
         step < STEP_LIMIT &&
         game.simulation().snapshot().phase != Phase::Workshop;
         ++step) {
      game.step(ALL_AI);
    }
    const auto sources = game.exploration().snapshot().sources;
    return std::pair{
        game.simulation().snapshot().remaining_exploration,
        std::accumulate(sources.begin(), sources.end(), 0U,
                        [](std::uint32_t sum, const SourceStock &source) {
                          return sum + source.remaining;
                        })};
  };
  auto content = temporary_content(1);
  const auto [left, stock] = first_trip(content);
  EXPECT_EQ(left, 0us);
  EXPECT_GT(stock, 0U);
  content.run.exploration_budget *= 10;
  const auto [long_left, long_stock] = first_trip(content);
  EXPECT_GT(long_left, 0us);
  EXPECT_LT(long_stock, stock);
}

TEST(TemporaryContent, NeverFeedingLosesLongBeforeTheFirstEscapeCheck) {
  Game game(temporary_content(1));
  const auto penalty = game.content().companion.unfed_penalty;
  std::uint32_t days = 0;
  while (game.simulation().snapshot().phase == Phase::Workshop &&
         days < TEMPORARY_ESCAPE.first_day) {
    ASSERT_EQ(game.workshop().end_day_unfed(penalty), WorkshopResult::Applied);
    ++days;
  }
  EXPECT_EQ(game.simulation().snapshot().phase, Phase::Lost);
  EXPECT_LT(days, TEMPORARY_ESCAPE.first_day / 2);
}

TEST(TemporaryContent, TheDayBringsAWashedUpBagAndNothingElseTakesItAway) {
  const auto content = temporary_content(1);
  EXPECT_EQ(object_named(on_day(content, 1), "Washed-up Bag 1").state,
            ObjectState::Dormant);
  EXPECT_TRUE(
      object_named(on_day(content, 2), "Washed-up Bag 1").material_available);
  EXPECT_TRUE(
      object_named(on_day(content, 6), "Washed-up Bag 1").material_available);
}

TEST(TemporaryContent, TheTideTakesAnUncollectedBagBackAfterFiveDays) {
  const auto content = temporary_content(1);
  const auto before = on_day(content, 6);
  EXPECT_TRUE(object_named(before, "Washed-up Bag 1").material_available);
  const auto after = on_day(content, 7);
  EXPECT_EQ(object_named(after, "Washed-up Bag 1").state,
            ObjectState::Destroyed);
  EXPECT_TRUE(object_named(after, "Washed-up Bag 4").material_available);
}

TEST(TemporaryContent, ThreeBagsLandWithEachDayFromTheSecond) {
  const auto content = temporary_content(1);
  const auto bags = [](const WorldSnapshot &world) {
    return std::ranges::count_if(
        world.objects, [](const ObjectSnapshot &object) {
          return object.state != ObjectState::Dormant &&
                 object_name(object.id).starts_with("Washed-up Bag");
        });
  };
  EXPECT_EQ(bags(on_day(content, 1)), 0);
  EXPECT_EQ(bags(on_day(content, 2)), 3);
  EXPECT_EQ(bags(on_day(content, 30)), 87);
}

TEST(TemporaryContent, BagsLandOnScheduleForAPartyThatNeverSetsOut) {
  Game game(temporary_content(1));
  const std::array<ExplorationInput, 2> idle{};
  const auto end_days = [&](std::uint32_t days) {
    for (std::uint32_t day = 0; day < days; ++day) {
      ASSERT_EQ(game.workshop().end_day_unfed(AcidLevel{}),
                WorkshopResult::Applied);
    }
    ASSERT_EQ(game.exploration().advance(0us, idle), Result::Applied);
  };
  end_days(0);
  EXPECT_EQ(object_named(game.world().snapshot(), "Washed-up Bag 1").state,
            ObjectState::Dormant);
  end_days(1);
  ASSERT_EQ(game.world().snapshot().day, 2U);
  EXPECT_TRUE(object_named(game.world().snapshot(), "Washed-up Bag 1")
                  .material_available);
  end_days(28);
  ASSERT_EQ(game.world().snapshot().day, 30U);
  EXPECT_TRUE(object_named(game.world().snapshot(), "Washed-up Bag 87")
                  .material_available);
  EXPECT_EQ(game.simulation().snapshot().chef.location, Location::Base);
  EXPECT_EQ(game.simulation().snapshot().appraiser.location, Location::Base);
}

TEST(TemporaryContent, TheSourcesAcidOnceDestroyedNowSupplyForTheWholeRun) {
  const auto content = temporary_content(1);
  for (const auto *name :
       {"Shallows", "Tomato Machine", "Lost Basket", "Pickle Machine",
        "Noodle Vendor", "Kitchen Shop", "Cold Store Door"}) {
    for (const auto day :
         {1U, STAGE_FIRST_DAY.at(1), STAGE_FIRST_DAY.at(2), LAST_SCANNED_DAY}) {
      EXPECT_EQ(object_named(on_day(content, day), name).state,
                ObjectState::Intact)
          << name << " on day " << day;
    }
  }
  for (const auto *name : {"Vendor Cart", "Shop Shelf", "Cold Locker"}) {
    const auto last = on_day(content, LAST_SCANNED_DAY);
    EXPECT_TRUE(object_named(last, name).material_available) << name;
    EXPECT_TRUE(reachable(content, last, object_named(last, name))) << name;
  }
}

TEST(TemporaryContent, OnFiveSeedsAnAiPartyFeedsEveryDayToTheFirstEscapeCheck) {
  for (const std::uint32_t seed : {1U, 2U, 3U, 4U, 5U}) {
    Game game(temporary_content(seed));
    EXPECT_EQ(unfed_days(game, TEMPORARY_ESCAPE.first_day + 1), 0U)
        << "seed " << seed;
    EXPECT_EQ(game.simulation().snapshot().day, TEMPORARY_ESCAPE.first_day + 1)
        << "seed " << seed;
  }
}

TEST(TemporaryContent, AnAiPartyFeedsTheMonsterAndSatisfiesIt) {
  Game game(temporary_content(1));
  bool satisfied = false;
  for (std::uint32_t step = 0;
       step < STEP_LIMIT && game.simulation().snapshot().day < 6; ++step) {
    game.step(ALL_AI);
    const auto last = game.workshop().snapshot().last_day;
    satisfied =
        satisfied || (last && last->reaction == MonsterReaction::Satisfied);
  }
  EXPECT_TRUE(satisfied);
  EXPECT_GE(game.workshop().snapshot().eaten_foods.size(), 2U);
}

TEST(TemporaryContent, TheDayDamagesTheCityAndBringsTheFerryInThreeStages) {
  const auto content = temporary_content(1);
  const auto standing = on_day(content, 1);
  ASSERT_EQ(object_named(standing, "Noodle Vendor").state, ObjectState::Intact);
  ASSERT_EQ(object_named(standing, "Roof Ladder").state, ObjectState::Intact);
  ASSERT_EQ(object_named(standing, "Fire Escape").state, ObjectState::Dormant);
  ASSERT_EQ(object_named(standing, "Drifted Ferry").state,
            ObjectState::Dormant);
  ASSERT_EQ(object_named(standing, "Rotten Bridge").state, ObjectState::Intact);
  ASSERT_EQ(object_named(standing, "Old Kiosk").state, ObjectState::Intact);
  EXPECT_EQ(damage_stage(standing.day), DamageStage::Standing);

  const auto cracked = on_day(content, STAGE_FIRST_DAY.at(1));
  EXPECT_EQ(damage_stage(cracked.day), DamageStage::Cracked);
  EXPECT_EQ(object_named(cracked, "Roof Ladder").state, ObjectState::Destroyed);
  EXPECT_EQ(object_named(cracked, "Fire Escape").state, ObjectState::Intact);
  EXPECT_EQ(object_named(cracked, "Drifted Ferry").state, ObjectState::Intact);
  EXPECT_EQ(object_named(cracked, "Rotten Bridge").state, ObjectState::Intact);
  EXPECT_EQ(object_named(cracked, "Noodle Vendor").state, ObjectState::Intact);

  const auto ruined = on_day(content, STAGE_FIRST_DAY.at(2));
  EXPECT_EQ(damage_stage(ruined.day), DamageStage::Ruined);
  EXPECT_EQ(object_named(ruined, "Rotten Bridge").state,
            ObjectState::Destroyed);
  EXPECT_EQ(object_named(ruined, "Old Kiosk").state, ObjectState::Destroyed);
  EXPECT_EQ(object_named(ruined, "Drifted Ferry").state, ObjectState::Intact);
}

TEST(TemporaryContent, AnAiRunsWorldFollowsItsDays) {
  Game game(temporary_content(1));
  ASSERT_EQ(play_days(game, STAGE_FIRST_DAY.at(1)), STAGE_FIRST_DAY.at(1));
  ASSERT_EQ(game.exploration().advance(0us, {}), Result::Applied);
  EXPECT_EQ(game.world().snapshot().day, STAGE_FIRST_DAY.at(1));
  EXPECT_EQ(object_named(game.world().snapshot(), "Roof Ladder").state,
            ObjectState::Destroyed);
}

TEST(TemporaryContent, TheFerryReopensTheEastAfterTheBridgeFalls) {
  const auto content = temporary_content(1);
  const auto after = on_day(content, STAGE_FIRST_DAY.at(2));
  ASSERT_EQ(object_named(after, "Rotten Bridge").state, ObjectState::Destroyed);
  const auto floor = object_named(after, "Cold Store Floor");
  EXPECT_TRUE(reachable(content, after, floor));

  auto without_ferry = after;
  std::erase_if(without_ferry.objects, [](const ObjectSnapshot &object) {
    return object_name(object.id) == "Drifted Ferry";
  });
  EXPECT_FALSE(reachable(content, without_ferry, floor));
}

TEST(TemporaryContent, TheFireEscapeCarriesTheRoofOnceTheLadderBreaks) {
  const auto content = temporary_content(1);
  const auto before = on_day(content, 1);
  ASSERT_TRUE(reachable(content, before, object_named(before, "Roof Bag")));

  const auto after = on_day(content, STAGE_FIRST_DAY.at(1));
  ASSERT_EQ(object_named(after, "Roof Ladder").state, ObjectState::Destroyed);
  EXPECT_TRUE(reachable(content, after, object_named(after, "Roof Bag")));

  auto without_escape = after;
  std::erase_if(without_escape.objects, [](const ObjectSnapshot &object) {
    return object_name(object.id) == "Fire Escape";
  });
  EXPECT_FALSE(
      reachable(content, without_escape, object_named(after, "Roof Bag")));
}

/// The input of a human chef walking to `zone` along the links of the street,
/// idle while the partner is `WAIT` or more cells behind, and falling straight
/// down once above the zone.
PlayerInput walk_to_zone(const Game &game, const WorldArea &zone) {
  constexpr std::int32_t WAIT = 6;
  const auto view = game.view();
  const auto &body = view.exploration.chef.body;
  const auto chef = cell_of(body.position);
  const auto behind =
      chef.x - cell_of(view.exploration.appraiser.body.position).x;
  if (body.stance == Stance::Airborne) {
    // Above the zone the chef lets itself fall straight in.
    const bool above = chef.x >= zone.x && chef.x < zone.x + zone.width;
    return {.movement = {.direction = above ? Direction::Idle : body.heading}};
  }
  if (behind >= WAIT) {
    return {};
  }
  const CellMap cells(view.world, view.exploration.obstacles);
  std::map<std::pair<std::int32_t, std::int32_t>, std::optional<Move>> first;
  std::deque<GridPosition> frontier{chef};
  first[{chef.x, chef.y}] = std::nullopt;
  while (!frontier.empty()) {
    const auto at = frontier.front();
    frontier.pop_front();
    const auto step = first.at({at.x, at.y});
    if (distance(at, zone) == 0) {
      return step ? PlayerInput{.movement = steer(*step, body)} : PlayerInput{};
    }
    for (const auto &link : links(cells, at)) {
      if (first.try_emplace({link.to.x, link.to.y}, step ? step : link.move)
              .second) {
        frontier.push_back(link.to);
      }
    }
  }
  return {};
}

TEST(TemporaryContent,
     AnAiPartnerFollowsAHumanThroughTheStreetExitToTheAquarium) {
  auto content = temporary_content(7);
  // The walk waits for the AI's collections, which a day's budget may not.
  content.run.exploration_budget = 120s;
  // The walk steers around obstacles only, so no mutant is on its way.
  content.companion.exploration.patrols.clear();
  const auto &exploration = content.companion.exploration;
  const auto exit =
      std::ranges::find(exploration.exits, MapId::Street, &MapExit::from);
  ASSERT_NE(exit, exploration.exits.end());
  ASSERT_GT((exit->zone.x - exploration.entry.x) * SUBCELLS, SCREEN_LOCK);
  Game game(content);
  const auto control = single_player(Actor::Chef);
  game.step(control, {PlayerInput{.travel = true}, PlayerInput{}});
  ASSERT_EQ(game.simulation().snapshot().chef.location, Location::City);
  for (std::uint32_t step = 0;
       step < STEP_LIMIT &&
       game.exploration().snapshot().map == MapId::Street &&
       game.simulation().snapshot().phase == Phase::Exploration;
       ++step) {
    game.step(control, {walk_to_zone(game, exit->zone), PlayerInput{}});
  }
  EXPECT_EQ(game.exploration().snapshot().map, MapId::Aquarium);
  EXPECT_EQ(game.simulation().snapshot().appraiser.location, Location::City);
}

TEST(TemporaryContent, TwoSeedsGiveTheSameMapsAndTheSameSupply) {
  const auto first = temporary_content(1);
  const auto second = temporary_content(98765);
  ASSERT_NE(first.run.seed, second.run.seed);
  EXPECT_EQ(World(first.objects).snapshot(), World(second.objects).snapshot());
  EXPECT_EQ(on_day(first, 20), on_day(second, 20));
  EXPECT_EQ(first.companion.exploration.sources,
            second.companion.exploration.sources);
  ASSERT_EQ(first.maps.size(), second.maps.size());
  for (std::size_t index = 0; index < first.maps.size(); ++index) {
    const auto &one = first.maps.at(index);
    const auto &other = second.maps.at(index);
    EXPECT_EQ(one.size.width, other.size.width);
    EXPECT_EQ(one.size.height, other.size.height);
    ASSERT_EQ(one.ground.size(), other.ground.size());
    for (std::size_t cell = 0; cell < one.ground.size(); ++cell) {
      EXPECT_EQ(one.ground.at(cell).cell, other.ground.at(cell).cell);
      EXPECT_EQ(one.ground.at(cell).kind, other.ground.at(cell).kind);
    }
    EXPECT_EQ(one.decorations.size(), other.decorations.size());
    EXPECT_EQ(one.landmarks.size(), other.landmarks.size());
  }
}

TEST(TemporaryContent, TheStreetIsThreeScreensWideAndTheAquariumTwo) {
  const auto content = temporary_content(1);
  constexpr std::int32_t SCREEN = 20;
  ASSERT_EQ(content.maps.size(), 2U);
  EXPECT_GE(street_of(content).size.width, 3 * SCREEN);
  EXPECT_GE(map_of(content, MapId::Aquarium).size.width, 2 * SCREEN);
}

TEST(TemporaryContent,
     TheStreetHasTwoEntrancesTwoShopsWithAnInsideAndThreeFloors) {
  const auto content = temporary_content(1);
  const auto &config = content.companion.exploration;
  const auto world = World(content.objects).snapshot();
  // The base entrance is the street entry; the aquarium entrance is the exit
  // out of the street, and the aquarium has its own way back.
  EXPECT_TRUE(stands_on(CellMap(on_map(world, MapId::Street)), config.entry));
  const auto way_in =
      std::ranges::find(config.exits, MapId::Aquarium, &MapExit::to);
  const auto way_out =
      std::ranges::find(config.exits, MapId::Street, &MapExit::to);
  ASSERT_NE(way_in, config.exits.end());
  ASSERT_NE(way_out, config.exits.end());
  EXPECT_EQ(way_in->from, MapId::Street);
  EXPECT_EQ(way_out->from, MapId::Aquarium);
  EXPECT_GE(config.interiors.size(), 2U);
  for (const auto &interior : config.interiors) {
    const auto building = object_of(world, interior.building);
    EXPECT_EQ(building.kind, ObjectKind::Building);
    EXPECT_EQ(building.map, MapId::Street);
  }
  for (const auto map : {MapId::Street, MapId::Aquarium}) {
    std::set<std::int32_t> rows;
    bool stairs = false;
    bool ladder = false;
    const CellMap cells(on_map(world, map));
    for (const auto cell : cells_reached(content, world, every_move(), map)) {
      rows.insert(cell.y);
      for (const auto &link : links(cells, cell)) {
        stairs =
            stairs || ((link.move == Move::Left || link.move == Move::Right) &&
                       link.to.y != cell.y);
        ladder = ladder || link.move == Move::ClimbUp;
      }
    }
    EXPECT_GE(rows.size(), 3U) << static_cast<int>(map);
    EXPECT_TRUE(stairs) << static_cast<int>(map);
    EXPECT_TRUE(ladder) << static_cast<int>(map);
  }
}

/// Every cell of `map` on `day`, row by row.
std::vector<Cell> cells_on(const Content &content, MapId map,
                           std::uint32_t day) {
  const CellMap cells(on_map(on_day(content, day), map));
  const auto &size = map_of(content, map).size;
  std::vector<Cell> found;
  for (std::int32_t y = 0; y < size.height; ++y) {
    for (std::int32_t x = 0; x < size.width; ++x) {
      found.push_back(cells.at({.x = x, .y = y}));
    }
  }
  return found;
}

TEST(TemporaryContent, EachDamageStageChangesCellsOnEveryMapAndOnlyAtItsDay) {
  const auto content = temporary_content(1);
  for (const auto map : {MapId::Street, MapId::Aquarium}) {
    const auto standing = cells_on(content, map, 1);
    const auto cracked = cells_on(content, map, STAGE_FIRST_DAY.at(1));
    const auto ruined = cells_on(content, map, STAGE_FIRST_DAY.at(2));
    EXPECT_NE(standing, cracked) << static_cast<int>(map);
    EXPECT_NE(cracked, ruined) << static_cast<int>(map);
    EXPECT_NE(standing, ruined) << static_cast<int>(map);
    for (std::uint32_t day = 1; day < STAGE_FIRST_DAY.at(1); ++day) {
      EXPECT_EQ(cells_on(content, map, day), standing) << "day " << day;
    }
    for (auto day = STAGE_FIRST_DAY.at(1); day < STAGE_FIRST_DAY.at(2); ++day) {
      EXPECT_EQ(cells_on(content, map, day), cracked) << "day " << day;
    }
    for (auto day = STAGE_FIRST_DAY.at(2); day <= LAST_SCANNED_DAY; ++day) {
      EXPECT_EQ(cells_on(content, map, day), ruined) << "day " << day;
    }
  }
}

TEST(TemporaryContent, EveryMapReachesEverySourceExitAndInteriorAtEveryStage) {
  const auto content = temporary_content(1);
  World world(content.objects);
  for (std::uint32_t day = 1; day <= LAST_SCANNED_DAY; ++day) {
    ASSERT_EQ(world.update(day), WorldUpdateResult::Applied);
    EXPECT_EQ(path_problems(content, world.snapshot()),
              std::vector<std::string>{})
        << "day " << day;
  }
}

TEST(TemporaryContent, ThePathCheckNamesWhatACutPassageStrandsOnEachStage) {
  const auto content = temporary_content(1);
  struct Cut {
    std::uint32_t day;
    std::string_view cut;
    std::string_view stranded;
  };
  const std::array<Cut, 4> cuts{
      Cut{.day = 1,
          .cut = "Roof Ladder",
          .stranded = "Street: Roof Bag is out of reach"},
      Cut{.day = STAGE_FIRST_DAY.at(1),
          .cut = "Fire Escape",
          .stranded = "Street: Roof Bag is out of reach"},
      Cut{.day = STAGE_FIRST_DAY.at(2),
          .cut = "West Skywalk Ladder",
          .stranded = "Aquarium: Gift Shelf is out of reach"},
      Cut{.day = 1,
          .cut = "Dock Ladder",
          .stranded = "Street: the exit to Aquarium is out of reach"},
  };
  for (const auto &cut : cuts) {
    auto world = on_day(content, cut.day);
    ASSERT_EQ(path_problems(content, world), std::vector<std::string>{})
        << "day " << cut.day;
    std::erase_if(world.objects, [&](const ObjectSnapshot &object) {
      return object_name(object.id) == cut.cut;
    });
    const auto problems = path_problems(content, world);
    EXPECT_NE(std::ranges::find(problems, std::string(cut.stranded)),
              problems.end())
        << cut.cut << " on day " << cut.day << " found " << problems.size();
  }
}

} // namespace
} // namespace neocity::survival
