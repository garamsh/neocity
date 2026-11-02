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
#include <string_view>
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
constexpr auto ARRIVAL_SCAN_STEP = 5s;
constexpr auto ARRIVAL_SCAN_END = 1600s;
constexpr std::uint32_t ACID_SCAN_STEP = 5;
constexpr std::uint32_t ACID_SCAN_END = 100;
/// The share of attempts an intact spatula turns into a dish.
constexpr std::uint32_t INTACT_SPATULA_PERCENT = 90;

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
            .world = map.snapshot(),
            .exploration = explorers.snapshot(),
            .workshop = kitchen_work.snapshot()};
  }

  void step(ControlMap control) {
    std::array<ExplorationInput, 2> movement;
    for (std::size_t index = 0; index < ACTORS.size(); ++index) {
      const auto command =
          controllers.command(ACTORS.at(index), control, {}, view());
      movement.at(index) = command.movement;
      if (command.action) {
        (void)apply(explorers, kitchen_work, ACTORS.at(index), *command.action);
      }
      if (run_over(shared.snapshot().phase)) {
        return;
      }
    }
    ASSERT_EQ(explorers.advance(STEP, movement, shared.snapshot().acid),
              Result::Applied);
  }

private:
  Content data;
  Simulation shared;
  World map;
  Exploration explorers;
  Workshop kitchen_work;
  Companion controllers;
};

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

bool reachable(const Content &content, const WorldSnapshot &world,
               const WorldArea &area) {
  const auto entry = content.companion.exploration.entry;
  const auto range = content.companion.exploration.collection_distance;
  std::set<std::pair<std::int32_t, std::int32_t>> seen{{entry.x, entry.y}};
  std::deque<GridPosition> frontier{entry};
  while (!frontier.empty()) {
    const auto cell = frontier.front();
    frontier.pop_front();
    if (distance(cell, area) <= range) {
      return true;
    }
    for (const auto next : {GridPosition{.x = cell.x - 1, .y = cell.y},
                            GridPosition{.x = cell.x + 1, .y = cell.y},
                            GridPosition{.x = cell.x, .y = cell.y - 1},
                            GridPosition{.x = cell.x, .y = cell.y + 1}}) {
      if (cell_at(world, next) == Cell::Supported &&
          seen.insert({next.x, next.y}).second) {
        frontier.push_back(next);
      }
    }
  }
  return false;
}

/// The source's world object, as a snapshot.
ObjectSnapshot object_of(const WorldSnapshot &world, ObjectId id) {
  const auto found = std::ranges::find(world.objects, id, &ObjectSnapshot::id);
  if (found == world.objects.end()) {
    throw std::out_of_range("no such object");
  }
  return *found;
}

/// A run in which only exploration time passes.
std::vector<WorldProgression> time_passing() {
  std::vector<WorldProgression> moments;
  for (auto elapsed = 0s; elapsed <= ARRIVAL_SCAN_END;
       elapsed += ARRIVAL_SCAN_STEP) {
    moments.push_back({.elapsed = elapsed});
  }
  return moments;
}

/// A run in which only acid rises, as when no one sets out.
std::vector<WorldProgression> acid_rising() {
  std::vector<WorldProgression> moments;
  for (std::uint32_t acid = 0; acid <= ACID_SCAN_END; acid += ACID_SCAN_STEP) {
    moments.push_back({.acid = AcidLevel{acid}});
  }
  return moments;
}

/// The sources a party can stand in reach of while they supply along `moments`,
/// with no repair.
std::set<std::uint32_t>
sources_reached_along(const Content &content,
                      const std::vector<WorldProgression> &moments) {
  std::set<std::uint32_t> found;
  World world(content.objects);
  for (const auto &moment : moments) {
    if (world.update(moment) != WorldUpdateResult::Applied) {
      throw std::logic_error("time went backwards");
    }
    const auto now = world.snapshot();
    for (const auto &stock : content.companion.exploration.sources) {
      const auto object = object_of(now, stock.id);
      if (object.material_available && reachable(content, now, object.area)) {
        found.insert(stock.id.value);
      }
    }
  }
  return found;
}

/// The sources reached by either way a run can go.
std::set<std::uint32_t> sources_ever_reached(const Content &content) {
  auto found = sources_reached_along(content, time_passing());
  found.merge(sources_reached_along(content, acid_rising()));
  return found;
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

/// The city as a run opens it.
WorldSnapshot temporary_world(const Content &content) {
  return World(content.objects).snapshot();
}

/// Ingredients still in sources that can hand them out.
std::uint32_t supplied_stock(const Content &content,
                             const WorldSnapshot &world) {
  std::uint32_t total = 0;
  for (const auto &stock : content.companion.exploration.sources) {
    const auto object = object_of(world, stock.id);
    total +=
        object.material_available && object.ingredient ? stock.remaining : 0U;
  }
  return total;
}

TEST(TemporaryContent, ConstructsEveryFeatureWithinTheMap) {
  Game game(temporary_content(7));
  EXPECT_EQ(game.simulation().snapshot().seed, 7U);
  for (const auto &object : game.content().objects) {
    EXPECT_GE(object.area.x, 0);
    EXPECT_GE(object.area.y, 0);
    EXPECT_LE(object.area.x + object.area.width, game.content().map.width);
    EXPECT_LE(object.area.y + object.area.height, game.content().map.height);
    EXPECT_FALSE(object_name(object.id).empty());
  }
}

TEST(TemporaryContent, EveryNameLookupRefusesAnIdentityItNeverIssued) {
  EXPECT_EQ(ingredient_name(IngredientId{1}), "Tomato");
  EXPECT_THROW((void)ingredient_name(IngredientId{999}), std::out_of_range);
  EXPECT_THROW((void)food_name(FoodId{999}), std::out_of_range);
  EXPECT_THROW((void)tool_name(ToolId{999}), std::out_of_range);
  EXPECT_THROW((void)object_name(ObjectId{999}), std::out_of_range);
}

TEST(TemporaryContent, EveryRecipeIngredientAndBaseToolHasASource) {
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
  EXPECT_TRUE(
      supplies(&ObjectDefinition::tool, content.companion.workshop.magnifier));
  EXPECT_TRUE(
      supplies(&ObjectDefinition::tool, content.companion.workshop.spatula));
}

TEST(TemporaryContent, EveryStockedSourceCanBeReachedWhileItSupplies) {
  const auto content = temporary_content(1);
  const auto reached = sources_ever_reached(content);
  for (const auto &stock : content.companion.exploration.sources) {
    EXPECT_TRUE(reached.contains(stock.id.value)) << object_name(stock.id);
  }
}

TEST(TemporaryContent, EverySourceAcidBringsCanBeReachedWithNoTimeSpent) {
  const auto content = temporary_content(1);
  const auto reached = sources_reached_along(content, acid_rising());
  std::size_t arrivals = 0;
  for (const auto &stock : content.companion.exploration.sources) {
    const auto object =
        std::ranges::find(content.objects, stock.id, &ObjectDefinition::id);
    ASSERT_NE(object, content.objects.end());
    if (object->exposed_at && object->exposed_at->acid) {
      ++arrivals;
      EXPECT_TRUE(reached.contains(stock.id.value)) << object_name(stock.id);
    }
  }
  EXPECT_EQ(arrivals, 9U);
}

TEST(TemporaryContent, EveryBreakablePassageCanBeReachedForRepair) {
  const auto content = temporary_content(1);
  World world(content.objects);
  ASSERT_EQ(world.update({.elapsed = ARRIVAL_SCAN_END}),
            WorldUpdateResult::Applied);
  const auto ruined = world.snapshot();
  std::size_t broken = 0;
  for (const auto &object : ruined.objects) {
    if (object.repair_with && object.state == ObjectState::Destroyed) {
      ++broken;
      EXPECT_TRUE(reachable(content, ruined, object.area))
          << object_name(object.id);
    }
  }
  EXPECT_EQ(broken, 2U);
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
  EXPECT_GE(dishes * INTACT_SPATULA_PERCENT, TEMPORARY_ESCAPE.first_day * 100U)
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
        reachable(game.content(), world, object->area)) {
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

TEST(TemporaryContent,
     TheDayEndsByTimeWhileStockRemainsBecauseCollectingTakesTime) {
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
  content.companion.exploration.collection_time = 0us;
  const auto [free_left, free_stock] = first_trip(content);
  EXPECT_GT(free_left, 0us);
  EXPECT_LT(free_stock, stock);
}

TEST(TemporaryContent, AnAiPartyFeedsTheDislikedDishOnce) {
  for (const std::uint32_t seed : {1U, 2U, 3U, 4U, 5U}) {
    Game game(temporary_content(seed));
    std::uint32_t day = 0;
    std::uint32_t disliked = 0;
    for (std::uint32_t step = 0;
         step < STEP_LIMIT && !run_over(game.simulation().snapshot().phase);
         ++step) {
      game.step(ALL_AI);
      const auto last = game.workshop().snapshot().last_day;
      if (last && last->completed_day != day) {
        day = last->completed_day;
        disliked += last->reaction == MonsterReaction::Disliked ? 1U : 0U;
      }
    }
    EXPECT_TRUE(run_over(game.simulation().snapshot().phase))
        << "seed " << seed;
    EXPECT_LE(disliked, 1U) << "seed " << seed;
  }
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

TEST(TemporaryContent, AcidCarriesAWashedUpBagAwayAfterBringingIt) {
  World world(temporary_content(1).objects);
  ASSERT_EQ(object_named(world.snapshot(), "Washed-up Bag 1").state,
            ObjectState::Dormant);
  ASSERT_EQ(world.update({.acid = AcidLevel{39}}), WorldUpdateResult::Applied);
  EXPECT_TRUE(
      object_named(world.snapshot(), "Washed-up Bag 1").material_available);
  ASSERT_EQ(world.update({.acid = AcidLevel{40}}), WorldUpdateResult::Applied);
  EXPECT_EQ(object_named(world.snapshot(), "Washed-up Bag 1").state,
            ObjectState::Destroyed);
}

TEST(TemporaryContent, RisingAcidStrandsStockThatNoOneCollected) {
  const auto content = temporary_content(1);
  World world(content.objects);
  const auto opening = supplied_stock(content, world.snapshot());
  ASSERT_GT(opening, 0U);
  ASSERT_EQ(world.update({.acid = AcidLevel{60}}), WorldUpdateResult::Applied);
  const auto later = world.snapshot();
  std::uint32_t stranded = 0;
  for (const auto &stock : content.companion.exploration.sources) {
    const auto start = object_of(temporary_world(content), stock.id);
    const auto now = object_of(later, stock.id);
    if (start.material_available && start.ingredient &&
        !now.material_available) {
      stranded += stock.remaining;
    }
  }
  EXPECT_GE(stranded, opening / 2) << stranded << " of " << opening;
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

TEST(TemporaryContent, TimeDamagesTheCityAndBringsTheFerryAcrossDays) {
  Game game(temporary_content(1));
  const auto start = game.world().snapshot();
  ASSERT_EQ(object_named(start, "Noodle Vendor").state, ObjectState::Intact);
  ASSERT_EQ(object_named(start, "Roof Ladder").state, ObjectState::Intact);
  ASSERT_EQ(object_named(start, "Drifted Ferry").state, ObjectState::Dormant);
  ASSERT_TRUE(object_named(start, "Vendor Cart").material_available);

  const std::array<ExplorationInput, 2> idle{};
  ASSERT_EQ(game.exploration().enter(Actor::Chef), Result::Applied);
  while (game.world().snapshot().progression.elapsed < 1120s) {
    ASSERT_EQ(game.exploration().advance(STEP, idle, AcidLevel{}),
              Result::Applied);
    if (game.simulation().snapshot().phase == Phase::Workshop) {
      ASSERT_EQ(game.simulation().finish_day(AcidLevel{}), Result::Applied);
      ASSERT_EQ(game.exploration().enter(Actor::Chef), Result::Applied);
    }
  }
  const auto later = game.world().snapshot();
  EXPECT_EQ(object_named(later, "Noodle Vendor").state, ObjectState::Destroyed);
  EXPECT_FALSE(object_named(later, "Vendor Cart").material_available);
  EXPECT_EQ(object_named(later, "Roof Ladder").state, ObjectState::Destroyed);
  EXPECT_EQ(object_named(later, "Rotten Bridge").state, ObjectState::Destroyed);
  EXPECT_EQ(object_named(later, "Drifted Ferry").state, ObjectState::Intact);
  EXPECT_TRUE(object_named(later, "Ferry Bag").material_available);
}

TEST(TemporaryContent, TheFerryReopensTheEastAfterTheBridgeFalls) {
  Game game(temporary_content(1));
  const auto plaza = object_named(game.world().snapshot(), "Supply Crate").area;
  ASSERT_TRUE(reachable(game.content(), game.world().snapshot(), plaza));

  ASSERT_EQ(game.world().update({.elapsed = 0s, .acid = AcidLevel{40}}),
            WorldUpdateResult::Applied);
  const auto after = game.world().snapshot();
  ASSERT_EQ(object_named(after, "Rotten Bridge").state, ObjectState::Destroyed);
  EXPECT_TRUE(reachable(game.content(), after, plaza));

  auto without_ferry = after;
  std::erase_if(without_ferry.objects, [](const ObjectSnapshot &object) {
    return object_name(object.id) == "Drifted Ferry";
  });
  EXPECT_FALSE(reachable(game.content(), without_ferry, plaza));
}

TEST(TemporaryContent, TheBrokenDoorBlocksTheColdLockerUntilRepaired) {
  Game game(temporary_content(1));
  const auto locker = object_named(game.world().snapshot(), "Cold Locker").area;
  ASSERT_TRUE(reachable(game.content(), game.world().snapshot(), locker));

  ASSERT_EQ(game.world().update({.elapsed = 0s, .acid = AcidLevel{20}}),
            WorldUpdateResult::Applied);
  const auto door = object_named(game.world().snapshot(), "Cold Store Door");
  ASSERT_EQ(door.state, ObjectState::Destroyed);
  EXPECT_FALSE(reachable(game.content(), game.world().snapshot(), locker));

  ASSERT_TRUE(door.repair_with.has_value());
  ASSERT_EQ(game.world().repair(door.id, door.repair_with.value_or(ToolId{})),
            RepairResult::Applied);
  EXPECT_TRUE(reachable(game.content(), game.world().snapshot(), locker));
}

TEST(TemporaryContent, TheBrokenLadderBlocksTheRooftopUntilRepaired) {
  Game game(temporary_content(1));
  const auto roof = object_named(game.world().snapshot(), "Roof Bag").area;
  ASSERT_TRUE(reachable(game.content(), game.world().snapshot(), roof));

  ASSERT_EQ(game.world().update({.elapsed = 0s, .acid = AcidLevel{10}}),
            WorldUpdateResult::Applied);
  const auto ladder = object_named(game.world().snapshot(), "Roof Ladder");
  ASSERT_EQ(ladder.state, ObjectState::Destroyed);
  EXPECT_FALSE(reachable(game.content(), game.world().snapshot(), roof));

  ASSERT_TRUE(ladder.repair_with.has_value());
  ASSERT_EQ(
      game.world().repair(ladder.id, ladder.repair_with.value_or(ToolId{})),
      RepairResult::Applied);
  EXPECT_TRUE(reachable(game.content(), game.world().snapshot(), roof));
}

} // namespace
} // namespace neocity::survival
