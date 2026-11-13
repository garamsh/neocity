#include <survival/exploration.hpp>
#include <survival/workshop.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace neocity::survival {
namespace {

using namespace std::chrono_literals;
constexpr IngredientId RICE{1};
constexpr IngredientId EGG{2};
constexpr IngredientId CHILI{3};
constexpr FoodId PORRIDGE{10};
constexpr FoodId OMELET{11};
constexpr FoodId CURRY{12};

WorkshopConfig balance() {
  return {
      .recipes = {{.food = PORRIDGE, .ingredients = {RICE}},
                  {.food = OMELET, .ingredients = {RICE, EGG}},
                  {.food = CURRY, .ingredients = {CHILI}, .disliked = true}},
      .new_food_score = 10,
      .new_ingredient_score = 3,
      .satisfying_score = 12,
      .disliked_acid_rise = AcidLevel{7},
      .unsatisfied_acid_rise = AcidLevel{2},
      .flavors = {}};
}

/// The stored dishes' foods, in storage order.
std::vector<FoodId> stored(const Workshop &workshop) {
  std::vector<FoodId> foods;
  for (const auto &dish : workshop.snapshot().foods) {
    foods.push_back(dish.food);
  }
  return foods;
}

/// Finds a seed whose first draw lands on the requested percentile.
std::uint32_t seed_for_roll(std::uint32_t roll) {
  for (std::uint32_t seed = 0;; ++seed) {
    std::mt19937 engine(seed);
    if (engine() % 100U == roll) {
      return seed;
    }
  }
}

/// A run at base carrying the listed ingredients on the chef.
Simulation carrying(std::uint32_t seed,
                    const std::vector<IngredientId> &ingredients) {
  Simulation simulation({.exploration_budget = 10s, .seed = seed});
  EXPECT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  for (const auto ingredient : ingredients) {
    EXPECT_EQ(simulation.collect(Actor::Chef, ingredient), Result::Applied);
  }
  EXPECT_EQ(simulation.return_to_base(Actor::Chef), Result::Applied);
  EXPECT_EQ(simulation.snapshot().chef.inventory, ingredients);
  return simulation;
}

/// Finds a seed whose first eight draws all land in the fine half of the
/// success range.
std::uint32_t lucky_seed() {
  for (std::uint32_t seed = 0;; ++seed) {
    std::mt19937 engine(seed);
    bool lucky = true;
    for (int draw = 0; draw < 8; ++draw) {
      lucky = engine() % 100U < 40U && lucky;
    }
    if (lucky) {
      return seed;
    }
  }
}

/// The default's day 0 and Unfed reaction match no outcome a test expects.
DayOutcome last_day(const Workshop &workshop) {
  return workshop.snapshot().last_day.value_or(DayOutcome{});
}

/// The outcome with its dish left out, for tests about the reaction and acid.
DayOutcome reaction_of(const Workshop &workshop) {
  auto outcome = last_day(workshop);
  outcome.fed.reset();
  return outcome;
}

TEST(Workshop, AppraisalSucceedsThroughRoll89AndFailsAtRoll90) {
  auto below = carrying(seed_for_roll(89), {RICE});
  Workshop succeeding(below, balance());
  EXPECT_EQ(succeeding.appraise(Actor::Appraiser, RICE),
            WorkshopResult::Applied);
  EXPECT_EQ(succeeding.snapshot().identified,
            (std::vector<IngredientId>{RICE}));

  auto at = carrying(seed_for_roll(90), {RICE});
  Workshop failing(at, balance());
  EXPECT_EQ(failing.appraise(Actor::Appraiser, RICE), WorkshopResult::Failed);
  EXPECT_TRUE(failing.snapshot().identified.empty());
}

/// Second draw of the seed decides cooking after one appraisal draw; the seed
/// search keeps the first draw a success below 40.
std::uint32_t seed_for_cook_roll(std::uint32_t roll) {
  for (std::uint32_t seed = 0;; ++seed) {
    std::mt19937 engine(seed);
    const auto appraisal = engine() % 100U;
    if (appraisal < 40 && engine() % 100U == roll) {
      return seed;
    }
  }
}

TEST(Workshop, CookingSucceedsThroughRoll89AndFailsAtRoll90) {
  auto below = carrying(seed_for_cook_roll(89), {RICE});
  Workshop succeeding(below, balance());
  ASSERT_EQ(succeeding.appraise(Actor::Appraiser, RICE),
            WorkshopResult::Applied);
  EXPECT_EQ(succeeding.cook(Actor::Chef, PORRIDGE), WorkshopResult::Applied);
  EXPECT_EQ(stored(succeeding), (std::vector<FoodId>{PORRIDGE}));

  auto at = carrying(seed_for_cook_roll(90), {RICE});
  Workshop failing(at, balance());
  ASSERT_EQ(failing.appraise(Actor::Appraiser, RICE), WorkshopResult::Applied);
  EXPECT_EQ(failing.cook(Actor::Chef, PORRIDGE), WorkshopResult::Failed);
  EXPECT_TRUE(failing.snapshot().foods.empty());
}

TEST(Workshop, AppraisalRefusalsKeepStateAndTheRandomSequence) {
  auto simulation = carrying(seed_for_roll(0), {RICE});
  Workshop workshop(simulation, balance());
  const auto before = workshop.snapshot();
  const auto progress = simulation.snapshot();
  EXPECT_EQ(workshop.appraise(Actor::Chef, RICE), WorkshopResult::WrongRole);
  EXPECT_EQ(workshop.appraise(Actor::Appraiser, EGG),
            WorkshopResult::NotCarried);
  EXPECT_EQ(workshop.snapshot(), before);
  EXPECT_EQ(simulation.snapshot(), progress);
  // A consumed draw would move past roll 0, the seed's only known success.
  EXPECT_EQ(workshop.appraise(Actor::Appraiser, RICE), WorkshopResult::Applied);
  EXPECT_EQ(workshop.snapshot().rolls, 1U);
  EXPECT_EQ(workshop.appraise(Actor::Appraiser, RICE),
            WorkshopResult::AlreadyIdentified);
  EXPECT_EQ(workshop.snapshot().rolls, 1U);
}

TEST(Workshop, AppraisalAcceptsAnIngredientCarriedByEitherActor) {
  Simulation simulation({.exploration_budget = 10s, .seed = seed_for_roll(0)});
  ASSERT_EQ(simulation.enter(Actor::Appraiser), Result::Applied);
  ASSERT_EQ(simulation.collect(Actor::Appraiser, EGG), Result::Applied);
  ASSERT_EQ(simulation.return_to_base(Actor::Appraiser), Result::Applied);
  Workshop workshop(simulation, balance());
  EXPECT_EQ(workshop.appraise(Actor::Appraiser, EGG), WorkshopResult::Applied);
}

TEST(Workshop, CookingRefusalsKeepIngredientsAndTheRandomSequence) {
  auto simulation = carrying(lucky_seed(), {RICE});
  Workshop workshop(simulation, balance());
  EXPECT_EQ(workshop.cook(Actor::Chef, PORRIDGE),
            WorkshopResult::NotIdentified);
  ASSERT_EQ(workshop.appraise(Actor::Appraiser, RICE), WorkshopResult::Applied);
  const auto before = workshop.snapshot();
  const auto progress = simulation.snapshot();
  ASSERT_EQ(before.rolls, 1U);
  EXPECT_EQ(workshop.cook(Actor::Appraiser, PORRIDGE),
            WorkshopResult::WrongRole);
  EXPECT_EQ(workshop.cook(Actor::Chef, FoodId{99}),
            WorkshopResult::UnknownRecipe);
  EXPECT_EQ(workshop.cook(Actor::Chef, OMELET), WorkshopResult::NotIdentified);
  EXPECT_EQ(workshop.snapshot(), before);
  EXPECT_EQ(simulation.snapshot(), progress);
  EXPECT_EQ(workshop.cook(Actor::Chef, PORRIDGE), WorkshopResult::Applied);
  EXPECT_EQ(workshop.snapshot().rolls, 2U);
}

TEST(Workshop, CookingAMissingIngredientRefusesWithoutSpendingTheRest) {
  auto simulation = carrying(lucky_seed(), {RICE, EGG});
  Workshop workshop(simulation, balance());
  ASSERT_EQ(workshop.appraise(Actor::Appraiser, RICE), WorkshopResult::Applied);
  ASSERT_EQ(workshop.appraise(Actor::Appraiser, EGG), WorkshopResult::Applied);
  const std::vector<IngredientId> spent{RICE};
  ASSERT_EQ(simulation.consume(spent), Result::Applied);
  const auto rolls = workshop.snapshot().rolls;
  EXPECT_EQ(workshop.cook(Actor::Chef, OMELET), WorkshopResult::NotCarried);
  EXPECT_EQ(simulation.snapshot().chef.inventory,
            (std::vector<IngredientId>{EGG}));
  EXPECT_EQ(workshop.snapshot().rolls, rolls);

  auto stocked = carrying(lucky_seed(), {RICE, EGG});
  Workshop control(stocked, balance());
  ASSERT_EQ(control.appraise(Actor::Appraiser, RICE), WorkshopResult::Applied);
  ASSERT_EQ(control.appraise(Actor::Appraiser, EGG), WorkshopResult::Applied);
  EXPECT_EQ(control.cook(Actor::Chef, OMELET), WorkshopResult::Applied);
  EXPECT_TRUE(stocked.snapshot().chef.inventory.empty());
}

TEST(Workshop, FailedCookingSpendsIngredientsAndStoresNoDish) {
  auto simulation = carrying(seed_for_cook_roll(90), {RICE, EGG});
  Workshop workshop(simulation, balance());
  ASSERT_EQ(workshop.appraise(Actor::Appraiser, RICE), WorkshopResult::Applied);
  ASSERT_EQ(workshop.cook(Actor::Chef, PORRIDGE), WorkshopResult::Failed);
  EXPECT_EQ(simulation.snapshot().chef.inventory,
            (std::vector<IngredientId>{EGG}));
  EXPECT_TRUE(workshop.snapshot().foods.empty());
  EXPECT_EQ(workshop.feed(PORRIDGE), WorkshopResult::NoFood);
  EXPECT_EQ(simulation.snapshot().day, 1U);
}

TEST(Workshop, CommandsAreRefusedWhileAnActorIsExploring) {
  auto simulation = carrying(lucky_seed(), {RICE});
  Workshop workshop(simulation, balance());
  ASSERT_EQ(workshop.appraise(Actor::Appraiser, RICE), WorkshopResult::Applied);
  ASSERT_EQ(workshop.cook(Actor::Chef, PORRIDGE), WorkshopResult::Applied);
  ASSERT_EQ(simulation.enter(Actor::Appraiser), Result::Applied);
  const auto before = workshop.snapshot();
  const auto progress = simulation.snapshot();
  EXPECT_EQ(workshop.appraise(Actor::Appraiser, RICE),
            WorkshopResult::NotInWorkshop);
  EXPECT_EQ(workshop.cook(Actor::Chef, PORRIDGE),
            WorkshopResult::NotInWorkshop);
  EXPECT_EQ(workshop.feed(PORRIDGE), WorkshopResult::NotInWorkshop);
  EXPECT_EQ(workshop.end_day_unfed(AcidLevel{1}),
            WorkshopResult::NotInWorkshop);
  EXPECT_EQ(workshop.snapshot(), before);
  EXPECT_EQ(simulation.snapshot(), progress);
  ASSERT_EQ(simulation.return_to_base(Actor::Appraiser), Result::Applied);
  EXPECT_EQ(workshop.feed(PORRIDGE), WorkshopResult::Applied);
}

TEST(Workshop, BaseWorkUsesNoTime) {
  auto simulation = carrying(lucky_seed(), {RICE});
  Workshop workshop(simulation, balance());
  const auto before = simulation.snapshot();
  ASSERT_EQ(before.remaining_exploration, 10s);
  ASSERT_EQ(simulation.advance(24h), Result::Applied);
  ASSERT_EQ(workshop.appraise(Actor::Appraiser, RICE), WorkshopResult::Applied);
  ASSERT_EQ(simulation.advance(24h), Result::Applied);
  ASSERT_EQ(workshop.cook(Actor::Chef, PORRIDGE), WorkshopResult::Applied);
  auto expected = before;
  expected.chef.inventory.clear();
  EXPECT_EQ(simulation.snapshot(), expected);
}

TEST(Workshop, NewFoodsScoreAboveRepeatsAndRepeatsMissTheThreshold) {
  auto simulation = carrying(lucky_seed(), {RICE, RICE});
  Workshop workshop(simulation, balance());
  ASSERT_EQ(workshop.appraise(Actor::Appraiser, RICE), WorkshopResult::Applied);
  ASSERT_EQ(workshop.cook(Actor::Chef, PORRIDGE), WorkshopResult::Applied);
  ASSERT_EQ(workshop.cook(Actor::Chef, PORRIDGE), WorkshopResult::Applied);

  ASSERT_EQ(workshop.feed(PORRIDGE), WorkshopResult::Applied);
  EXPECT_EQ(reaction_of(workshop),
            (DayOutcome{.completed_day = 1,
                        .reaction = MonsterReaction::Satisfied,
                        .score = 13,
                        .acid_rise = AcidLevel{0}}));
  EXPECT_EQ(simulation.snapshot().acid, AcidLevel{0});

  ASSERT_EQ(workshop.feed(PORRIDGE), WorkshopResult::Applied);
  EXPECT_EQ(reaction_of(workshop),
            (DayOutcome{.completed_day = 2,
                        .reaction = MonsterReaction::Unsatisfied,
                        .score = 0,
                        .acid_rise = AcidLevel{2}}));
  EXPECT_EQ(simulation.snapshot().acid, AcidLevel{2});
  EXPECT_EQ(simulation.snapshot().day, 3U);
}

TEST(DishScore, AddsEachPartForAPlainAndAFineDish) {
  auto config = balance();
  config.flavors = {{.ingredient = RICE, .taste = 4},
                    {.ingredient = EGG, .taste = 6}};
  config.fine_dish_score = 8;
  WorkshopSnapshot history;
  history.eaten_ingredients = {RICE};
  EXPECT_EQ(dish_score({.food = OMELET}, history, config),
            (DishScore{.new_food = 10,
                       .new_ingredients = 1,
                       .new_ingredient_points = 3,
                       .taste = 10,
                       .fine = 0,
                       .total = 23}));
  const auto fine = dish_score({.food = OMELET, .quality = DishQuality::Fine},
                               history, config);
  EXPECT_EQ(fine.value_or(DishScore{}).fine, 8U);
  EXPECT_EQ(fine.value_or(DishScore{}).total, 31U);
}

TEST(DishScore, AnEatenFoodAddsNoNoveltyAndADislikeChangesNothing) {
  const auto config = balance();
  WorkshopSnapshot history;
  history.eaten_foods = {PORRIDGE};
  history.eaten_ingredients = {RICE};
  EXPECT_EQ(dish_score({.food = PORRIDGE}, history, config), DishScore{});
  auto disliked = balance();
  disliked.recipes.at(1).disliked = true;
  EXPECT_EQ(dish_score({.food = OMELET}, {}, config),
            dish_score({.food = OMELET}, {}, disliked));
}

TEST(DishScore, ADishOfNoRecipeHasNoScore) {
  EXPECT_FALSE(dish_score({.food = FoodId{99}}, {}, balance()).has_value());
}

TEST(Workshop, FeedingScoresWhatThePreviewSaidAndReportsTheDish) {
  auto simulation = carrying(seed_for_roll(0), {RICE});
  Workshop workshop(simulation, balance());
  ASSERT_EQ(workshop.appraise(Actor::Appraiser, RICE), WorkshopResult::Applied);
  ASSERT_EQ(workshop.cook(Actor::Chef, PORRIDGE), WorkshopResult::Applied);
  ASSERT_EQ(workshop.snapshot().foods.size(), 1U);
  const auto dish = workshop.snapshot().foods.front();
  const auto preview = dish_score(dish, workshop.snapshot(), balance());
  ASSERT_TRUE(preview.has_value());
  ASSERT_EQ(workshop.feed(PORRIDGE), WorkshopResult::Applied);
  const auto outcome = last_day(workshop);
  EXPECT_EQ(outcome.fed,
            (FedDish{.dish = dish, .score = preview.value_or(DishScore{})}));
  EXPECT_EQ(outcome.score, preview.value_or(DishScore{}).total);
}

TEST(Workshop, ADayEndedUnfedReportsNoDish) {
  Simulation simulation({.exploration_budget = 10s, .seed = 1});
  Workshop workshop(simulation, balance());
  ASSERT_EQ(workshop.end_day_unfed(AcidLevel{1}), WorkshopResult::Applied);
  ASSERT_TRUE(workshop.snapshot().last_day.has_value());
  EXPECT_FALSE(last_day(workshop).fed.has_value());
}

TEST(Workshop, ScoreEqualToTheThresholdSatisfiesAndOneBelowDoesNot) {
  // PORRIDGE scores 13 as a new food with one new ingredient.
  for (const auto &[threshold, reaction] :
       {std::pair{13U, MonsterReaction::Satisfied},
        std::pair{14U, MonsterReaction::Unsatisfied}}) {
    auto config = balance();
    config.satisfying_score = threshold;
    auto simulation = carrying(lucky_seed(), {RICE});
    Workshop workshop(simulation, config);
    ASSERT_EQ(workshop.appraise(Actor::Appraiser, RICE),
              WorkshopResult::Applied);
    ASSERT_EQ(workshop.cook(Actor::Chef, PORRIDGE), WorkshopResult::Applied);
    ASSERT_EQ(workshop.feed(PORRIDGE), WorkshopResult::Applied);
    EXPECT_EQ(last_day(workshop).score, 13U);
    EXPECT_EQ(last_day(workshop).reaction, reaction);
  }
}

TEST(Workshop, ANewFoodOfEatenIngredientsScoresOnlyItsUnseenParts) {
  auto simulation = carrying(lucky_seed(), {RICE, RICE, EGG});
  Workshop workshop(simulation, balance());
  ASSERT_EQ(workshop.appraise(Actor::Appraiser, RICE), WorkshopResult::Applied);
  ASSERT_EQ(workshop.appraise(Actor::Appraiser, EGG), WorkshopResult::Applied);
  ASSERT_EQ(workshop.cook(Actor::Chef, PORRIDGE), WorkshopResult::Applied);
  ASSERT_EQ(workshop.feed(PORRIDGE), WorkshopResult::Applied);
  ASSERT_EQ(workshop.cook(Actor::Chef, OMELET), WorkshopResult::Applied);
  ASSERT_EQ(workshop.feed(OMELET), WorkshopResult::Applied);
  // New food 10, EGG 3; RICE was already eaten.
  EXPECT_EQ(last_day(workshop).score, 13U);
  EXPECT_EQ(last_day(workshop).reaction, MonsterReaction::Satisfied);
}

TEST(Workshop, RepeatedIngredientInOneRecipeCountsAsOneNewIngredient) {
  auto config = balance();
  config.recipes.push_back(
      {.food = FoodId{13}, .ingredients = {RICE, RICE}, .disliked = false});
  auto simulation = carrying(lucky_seed(), {RICE, RICE});
  Workshop workshop(simulation, config);
  ASSERT_EQ(workshop.appraise(Actor::Appraiser, RICE), WorkshopResult::Applied);
  ASSERT_EQ(workshop.cook(Actor::Chef, FoodId{13}), WorkshopResult::Applied);
  ASSERT_EQ(workshop.feed(FoodId{13}), WorkshopResult::Applied);
  EXPECT_EQ(last_day(workshop).score, 13U);
}

TEST(Workshop, DislikedFoodRaisesAcidEvenWhenItsScorePasses) {
  auto simulation = carrying(lucky_seed(), {CHILI});
  Workshop workshop(simulation, balance());
  ASSERT_EQ(workshop.appraise(Actor::Appraiser, CHILI),
            WorkshopResult::Applied);
  ASSERT_EQ(workshop.cook(Actor::Chef, CURRY), WorkshopResult::Applied);
  ASSERT_EQ(workshop.feed(CURRY), WorkshopResult::Applied);
  EXPECT_EQ(reaction_of(workshop),
            (DayOutcome{.completed_day = 1,
                        .reaction = MonsterReaction::Disliked,
                        .score = 13,
                        .acid_rise = AcidLevel{7}}));
  EXPECT_EQ(simulation.snapshot().acid, AcidLevel{7});
  EXPECT_EQ(simulation.snapshot().day, 2U);
}

TEST(Workshop, FeedingSpendsOneDishAndKeepsTheOthersForLaterDays) {
  auto simulation = carrying(lucky_seed(), {RICE, RICE});
  Workshop workshop(simulation, balance());
  ASSERT_EQ(workshop.appraise(Actor::Appraiser, RICE), WorkshopResult::Applied);
  ASSERT_EQ(workshop.cook(Actor::Chef, PORRIDGE), WorkshopResult::Applied);
  ASSERT_EQ(workshop.cook(Actor::Chef, PORRIDGE), WorkshopResult::Applied);
  ASSERT_EQ(stored(workshop), (std::vector<FoodId>{PORRIDGE, PORRIDGE}));
  ASSERT_EQ(workshop.feed(PORRIDGE), WorkshopResult::Applied);
  EXPECT_EQ(stored(workshop), (std::vector<FoodId>{PORRIDGE}));
  EXPECT_EQ(workshop.snapshot().eaten_foods, (std::vector<FoodId>{PORRIDGE}));
  EXPECT_EQ(workshop.snapshot().eaten_ingredients,
            (std::vector<IngredientId>{RICE}));
}

TEST(Workshop, DayEndsUnfedWithTheCallerPenaltyAndCreatesNothing) {
  Simulation simulation({.exploration_budget = 10s, .seed = 3});
  Workshop workshop(simulation, balance());
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.advance(10s), Result::Applied);
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::NoExplorationTime);
  ASSERT_TRUE(simulation.snapshot().chef.inventory.empty());
  EXPECT_EQ(workshop.end_day_unfed(AcidLevel{5}), WorkshopResult::Applied);
  EXPECT_EQ(simulation.snapshot().day, 2U);
  EXPECT_EQ(simulation.snapshot().acid, AcidLevel{5});
  EXPECT_EQ(simulation.snapshot().remaining_exploration, 10s);
  EXPECT_TRUE(simulation.snapshot().chef.inventory.empty());
  EXPECT_TRUE(workshop.snapshot().foods.empty());
  EXPECT_EQ(workshop.snapshot().rolls, 0U);
  EXPECT_EQ(workshop.snapshot().last_day,
            (DayOutcome{.completed_day = 1,
                        .reaction = MonsterReaction::Unfed,
                        .score = 0,
                        .acid_rise = AcidLevel{5}}));
  EXPECT_EQ(simulation.enter(Actor::Chef), Result::Applied);
}

TEST(Workshop, DayOutcomeReportsDayThirtyForTheEscapeRules) {
  Simulation simulation({.exploration_budget = 10s, .seed = 3});
  Workshop workshop(simulation, balance());
  for (std::uint32_t day = 1; day <= 30; ++day) {
    ASSERT_EQ(workshop.end_day_unfed(AcidLevel{1}), WorkshopResult::Applied);
    EXPECT_EQ(last_day(workshop).completed_day, day);
  }
  EXPECT_EQ(simulation.snapshot().day, 31U);
  EXPECT_EQ(simulation.snapshot().acid, AcidLevel{30});
  EXPECT_EQ(simulation.snapshot().phase, Phase::Workshop);
}

TEST(Workshop, SharesOneRandomSequenceAcrossAppraisalAndCooking) {
  // First draws of mt19937 seeded with 42: 1608637542, 3421126067, 4083286876.
  // Their percentiles are 42, 67 and 76, all below 90.
  auto simulation = carrying(42, {RICE, RICE, EGG});
  Workshop workshop(simulation, balance());
  EXPECT_EQ(workshop.appraise(Actor::Appraiser, RICE), WorkshopResult::Applied);
  EXPECT_EQ(workshop.appraise(Actor::Appraiser, EGG), WorkshopResult::Applied);
  EXPECT_EQ(workshop.cook(Actor::Chef, OMELET), WorkshopResult::Applied);
  EXPECT_EQ(workshop.snapshot().rolls, 3U);
}

TEST(Workshop, SameSeedAndCommandsReplayTheSameRun) {
  const auto play = [](std::uint32_t seed) {
    auto simulation = carrying(seed, {RICE, RICE, EGG, EGG});
    Workshop workshop(simulation, balance());
    std::vector<WorkshopResult> results;
    for (int attempt = 0; attempt < 6; ++attempt) {
      results.push_back(workshop.appraise(Actor::Appraiser, RICE));
      results.push_back(workshop.appraise(Actor::Appraiser, EGG));
      results.push_back(workshop.cook(Actor::Chef, OMELET));
    }
    return std::make_tuple(results, workshop.snapshot(), simulation.snapshot());
  };
  EXPECT_EQ(play(1234), play(1234));
  bool differs = false;
  for (std::uint32_t seed = 0; seed < 16; ++seed) {
    differs = differs || play(seed) != play(1234);
  }
  EXPECT_TRUE(differs);
}

TEST(Workshop, EatenIngredientsHoldEachKindOnce) {
  auto config = balance();
  config.recipes.push_back({.food = FoodId{13}, .ingredients = {RICE, RICE}});
  auto simulation = carrying(lucky_seed(), {RICE, RICE, RICE, RICE});
  Workshop workshop(simulation, config);
  ASSERT_EQ(workshop.appraise(Actor::Appraiser, RICE), WorkshopResult::Applied);
  ASSERT_EQ(workshop.cook(Actor::Chef, FoodId{13}), WorkshopResult::Applied);
  ASSERT_EQ(workshop.cook(Actor::Chef, PORRIDGE), WorkshopResult::Applied);
  ASSERT_EQ(workshop.feed(FoodId{13}), WorkshopResult::Applied);
  ASSERT_EQ(workshop.feed(PORRIDGE), WorkshopResult::Applied);
  EXPECT_EQ(workshop.snapshot().eaten_ingredients,
            (std::vector<IngredientId>{RICE}));
  EXPECT_EQ(workshop.snapshot().eaten_foods,
            (std::vector<FoodId>{FoodId{13}, PORRIDGE}));
}

TEST(Workshop, RejectsEmptyRecipesAndRepeatedFoods) {
  Simulation simulation({.exploration_budget = 10s, .seed = 1});
  auto empty = balance();
  empty.recipes.push_back({.food = FoodId{20}, .ingredients = {}});
  EXPECT_THROW(Workshop(simulation, empty), std::invalid_argument);
  auto repeated = balance();
  repeated.recipes.push_back({.food = OMELET, .ingredients = {EGG}});
  EXPECT_THROW(Workshop(simulation, repeated), std::invalid_argument);
  EXPECT_NO_THROW(Workshop(simulation, balance()));
}

TEST(WorkshopIntegration, FeedingResultReachesTheNextExplorationDay) {
  constexpr ObjectId FLOOR{1};
  constexpr ObjectId SOURCE{2};
  constexpr ObjectId ACID_FLOOR{3};
  Simulation simulation({.exploration_budget = 1s, .seed = lucky_seed()});
  World world({{.id = FLOOR, .area = {.x = 0, .y = 0, .width = 5, .height = 1}},
               {.id = SOURCE,
                .kind = ObjectKind::MaterialSource,
                .area = {.x = 1},
                .ingredient = CHILI},
               {.id = ACID_FLOOR, .area = {.x = 9}, .destroyed_from_day = 2}});
  Exploration exploration(simulation, world,
                          {.base = {.x = -5},
                           .entry = {},
                           .exits = {},
                           .interiors = {},
                           .sources = {{.id = SOURCE}},
                           .obstacles = {},
                           .patrols = {}});
  Workshop workshop(simulation, balance());
  const std::array<ExplorationInput, 2> idle{};

  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Applied);
  ASSERT_EQ(exploration.advance(1s, idle), Result::Applied);
  ASSERT_EQ(simulation.snapshot().phase, Phase::Workshop);
  ASSERT_EQ(simulation.snapshot().remaining_exploration, 0us);
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::NoExplorationTime);

  ASSERT_EQ(workshop.appraise(Actor::Appraiser, CHILI),
            WorkshopResult::Applied);
  ASSERT_EQ(workshop.cook(Actor::Chef, CURRY), WorkshopResult::Applied);
  ASSERT_EQ(workshop.feed(CURRY), WorkshopResult::Applied);

  const auto after = simulation.snapshot();
  EXPECT_EQ(after.day, 2U);
  EXPECT_EQ(after.acid, AcidLevel{7});
  ASSERT_EQ(world.update(after.day), WorldUpdateResult::Applied);
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.advance(100ms, idle), Result::Applied);
  EXPECT_EQ(simulation.snapshot().remaining_exploration, 900ms);
  const auto city = world.snapshot();
  const auto floor =
      std::ranges::find(city.objects, ACID_FLOOR, &ObjectSnapshot::id);
  ASSERT_NE(floor, city.objects.end());
  EXPECT_EQ(floor->state, ObjectState::Destroyed);
}

WorkshopConfig flavored() {
  auto config = balance();
  config.new_food_score = 10;
  config.new_ingredient_score = 3;
  config.satisfying_score = 20;
  config.unsatisfied_acid_rise = AcidLevel{8};
  config.flavors = {{.ingredient = RICE, .taste = 2},
                    {.ingredient = EGG, .taste = 4}};
  config.fine_dish_score = 5;
  return config;
}

/// A seed whose appraisal draw succeeds and whose cook draw is `roll`.
std::uint32_t seed_for_appraised_cook(std::uint32_t roll) {
  for (std::uint32_t seed = 0;; ++seed) {
    std::mt19937 engine(seed);
    const auto first = engine() % 100U;
    if (first < 90U && engine() % 100U == roll) {
      return seed;
    }
  }
}

TEST(Workshop, TheLowerHalfOfTheSuccessRangeCooksAFineDish) {
  for (const auto &[roll, quality] :
       {std::pair{44U, DishQuality::Fine}, std::pair{45U, DishQuality::Plain},
        std::pair{89U, DishQuality::Plain}}) {
    auto simulation = carrying(seed_for_appraised_cook(roll), {RICE});
    Workshop workshop(simulation, balance());
    ASSERT_EQ(workshop.appraise(Actor::Appraiser, RICE),
              WorkshopResult::Applied);
    ASSERT_EQ(workshop.cook(Actor::Chef, PORRIDGE), WorkshopResult::Applied);
    ASSERT_EQ(workshop.snapshot().foods.size(), 1U);
    EXPECT_EQ(workshop.snapshot().foods.front().quality, quality);
    EXPECT_EQ(workshop.snapshot().rolls, 2U);
  }
}

TEST(Workshop, TheScoreAddsNoveltyTasteAndTheCookingResult) {
  auto plain = carrying(seed_for_appraised_cook(50), {RICE});
  Workshop plain_kitchen(plain, flavored());
  ASSERT_EQ(plain_kitchen.appraise(Actor::Appraiser, RICE),
            WorkshopResult::Applied);
  ASSERT_EQ(plain_kitchen.cook(Actor::Chef, PORRIDGE), WorkshopResult::Applied);
  ASSERT_EQ(plain_kitchen.feed(PORRIDGE), WorkshopResult::Applied);
  EXPECT_EQ(last_day(plain_kitchen).score, 10U + 3U + 2U);

  auto fine = carrying(seed_for_appraised_cook(10), {RICE});
  Workshop fine_kitchen(fine, flavored());
  ASSERT_EQ(fine_kitchen.appraise(Actor::Appraiser, RICE),
            WorkshopResult::Applied);
  ASSERT_EQ(fine_kitchen.cook(Actor::Chef, PORRIDGE), WorkshopResult::Applied);
  ASSERT_EQ(fine_kitchen.feed(PORRIDGE), WorkshopResult::Applied);
  EXPECT_EQ(last_day(fine_kitchen).score, 10U + 3U + 2U + 5U);
}

TEST(Workshop, TasteCountsEveryIngredientOfARecipe) {
  const auto config = flavored();
  EXPECT_EQ(taste_of(config.recipes.at(1), config), 2U + 4U);
  EXPECT_EQ(taste_of(config.recipes.at(2), config), 0U);
}

TEST(Workshop, ASatisfyingMealHoldsTheAcidWithoutLoweringIt) {
  auto simulation = carrying(seed_for_appraised_cook(10), {RICE});
  ASSERT_EQ(simulation.finish_day(AcidLevel{6}), Result::Applied);
  Workshop workshop(simulation, flavored());
  ASSERT_EQ(workshop.appraise(Actor::Appraiser, RICE), WorkshopResult::Applied);
  ASSERT_EQ(workshop.cook(Actor::Chef, PORRIDGE), WorkshopResult::Applied);
  ASSERT_EQ(workshop.feed(PORRIDGE), WorkshopResult::Applied);
  EXPECT_EQ(last_day(workshop).reaction, MonsterReaction::Satisfied);
  EXPECT_EQ(last_day(workshop).acid_rise, AcidLevel{});
  EXPECT_EQ(simulation.snapshot().acid, AcidLevel{6});
}

TEST(Workshop, BelowTheThresholdAcidRisesWithTheShortfallRoundedUp) {
  auto simulation = carrying(seed_for_appraised_cook(50), {RICE});
  Workshop workshop(simulation, flavored());
  ASSERT_EQ(workshop.appraise(Actor::Appraiser, RICE), WorkshopResult::Applied);
  ASSERT_EQ(workshop.cook(Actor::Chef, PORRIDGE), WorkshopResult::Applied);
  ASSERT_EQ(workshop.feed(PORRIDGE), WorkshopResult::Applied);
  EXPECT_EQ(last_day(workshop).score, 15U);
  EXPECT_EQ(last_day(workshop).reaction, MonsterReaction::Unsatisfied);
  EXPECT_EQ(last_day(workshop).acid_rise, AcidLevel{2});
  EXPECT_EQ(simulation.snapshot().acid, AcidLevel{2});
}

TEST(Workshop, ARepeatOfTheSameDishRaisesMoreThanItsFirstServing) {
  auto simulation = carrying(lucky_seed(), {RICE, RICE});
  auto config = flavored();
  config.satisfying_score = 100;
  Workshop workshop(simulation, config);
  ASSERT_EQ(workshop.appraise(Actor::Appraiser, RICE), WorkshopResult::Applied);
  ASSERT_EQ(workshop.cook(Actor::Chef, PORRIDGE), WorkshopResult::Applied);
  ASSERT_EQ(workshop.cook(Actor::Chef, PORRIDGE), WorkshopResult::Applied);
  ASSERT_EQ(workshop.feed(PORRIDGE), WorkshopResult::Applied);
  const auto first = last_day(workshop);
  ASSERT_EQ(workshop.feed(PORRIDGE), WorkshopResult::Applied);
  const auto repeat = last_day(workshop);
  EXPECT_LT(repeat.score, first.score);
  EXPECT_GT(repeat.acid_rise.value, first.acid_rise.value);
}

TEST(Workshop, ADislikedFoodIsRememberedAndAWantedOneIsNot) {
  auto simulation = carrying(lucky_seed(), {CHILI, RICE});
  Workshop workshop(simulation, balance());
  ASSERT_EQ(workshop.appraise(Actor::Appraiser, CHILI),
            WorkshopResult::Applied);
  ASSERT_EQ(workshop.appraise(Actor::Appraiser, RICE), WorkshopResult::Applied);
  ASSERT_EQ(workshop.cook(Actor::Chef, CURRY), WorkshopResult::Applied);
  ASSERT_EQ(workshop.cook(Actor::Chef, PORRIDGE), WorkshopResult::Applied);
  ASSERT_EQ(workshop.feed(PORRIDGE), WorkshopResult::Applied);
  EXPECT_TRUE(workshop.snapshot().rejected_foods.empty());
  ASSERT_EQ(workshop.feed(CURRY), WorkshopResult::Applied);
  EXPECT_EQ(workshop.snapshot().rejected_foods, (std::vector<FoodId>{CURRY}));
}

TEST(Workshop, TheMealThatFillsTheAcidIsRecordedAndNothingFollows) {
  Simulation simulation({.exploration_budget = 10s,
                         .seed = lucky_seed(),
                         .full_acid = AcidLevel{7}});
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.collect(Actor::Chef, CHILI), Result::Applied);
  ASSERT_EQ(simulation.collect(Actor::Chef, RICE), Result::Applied);
  ASSERT_EQ(simulation.return_to_base(Actor::Chef), Result::Applied);
  Workshop workshop(simulation, balance());
  ASSERT_EQ(workshop.appraise(Actor::Appraiser, CHILI),
            WorkshopResult::Applied);
  ASSERT_EQ(workshop.cook(Actor::Chef, CURRY), WorkshopResult::Applied);
  // The control: the role and the dish are accepted before the loss.
  ASSERT_EQ(workshop.feed(CURRY), WorkshopResult::Applied);
  ASSERT_EQ(simulation.snapshot().phase, Phase::Lost);
  EXPECT_EQ(last_day(workshop).reaction, MonsterReaction::Disliked);

  const auto progression = simulation.snapshot();
  const auto kitchen = workshop.snapshot();
  EXPECT_EQ(workshop.appraise(Actor::Appraiser, RICE),
            WorkshopResult::NotInWorkshop);
  EXPECT_EQ(workshop.cook(Actor::Chef, PORRIDGE),
            WorkshopResult::NotInWorkshop);
  EXPECT_EQ(workshop.feed(CURRY), WorkshopResult::NoFood);
  EXPECT_EQ(workshop.end_day_unfed(AcidLevel{1}),
            WorkshopResult::NotInWorkshop);
  EXPECT_EQ(simulation.snapshot(), progression);
  EXPECT_EQ(workshop.snapshot(), kitchen);
}

} // namespace
} // namespace neocity::survival
