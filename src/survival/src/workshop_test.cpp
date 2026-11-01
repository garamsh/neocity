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
constexpr ToolId NEW_MAGNIFIER{20};
constexpr ToolId NEW_SPATULA{21};

WorkshopConfig balance() {
  return {
      .recipes = {{.food = PORRIDGE, .ingredients = {RICE}},
                  {.food = OMELET, .ingredients = {RICE, EGG}},
                  {.food = CURRY, .ingredients = {CHILI}, .disliked = true}},
      .magnifier = NEW_MAGNIFIER,
      .spatula = NEW_SPATULA,
      .new_food_score = 10,
      .new_ingredient_score = 3,
      .satisfying_score = 12,
      .disliked_acid_rise = AcidLevel{7},
      .unsatisfied_acid_rise = AcidLevel{2}};
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

/// A run at base carrying the listed ingredients and tools on the chef.
Simulation carrying(std::uint32_t seed,
                    const std::vector<IngredientId> &ingredients,
                    const std::vector<ToolId> &tools = {}) {
  Simulation simulation({.exploration_budget = 10s, .seed = seed});
  EXPECT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  for (const auto ingredient : ingredients) {
    EXPECT_EQ(simulation.collect(Actor::Chef, ingredient), Result::Applied);
  }
  for (const auto tool : tools) {
    EXPECT_EQ(simulation.collect_tool(Actor::Chef, tool), Result::Applied);
  }
  EXPECT_EQ(simulation.return_to_base(Actor::Chef), Result::Applied);
  EXPECT_EQ(simulation.snapshot().chef.inventory, ingredients);
  return simulation;
}

/// Finds a seed whose first eight draws succeed even with damaged tools.
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

TEST(Workshop, IntactMagnifierSucceedsThroughRoll89AndFailsAtRoll90) {
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

TEST(Workshop, DamagedMagnifierSucceedsThroughRoll39AndFailsAtRoll40) {
  auto below = carrying(seed_for_roll(39), {RICE});
  Workshop succeeding(below, balance());
  succeeding.damage_tool(WorkshopTool::Magnifier);
  EXPECT_EQ(succeeding.appraise(Actor::Appraiser, RICE),
            WorkshopResult::Applied);

  auto at = carrying(seed_for_roll(40), {RICE});
  Workshop failing(at, balance());
  failing.damage_tool(WorkshopTool::Magnifier);
  EXPECT_EQ(failing.appraise(Actor::Appraiser, RICE), WorkshopResult::Failed);
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

TEST(Workshop, IntactSpatulaSucceedsThroughRoll89AndFailsAtRoll90) {
  auto below = carrying(seed_for_cook_roll(89), {RICE});
  Workshop succeeding(below, balance());
  ASSERT_EQ(succeeding.appraise(Actor::Appraiser, RICE),
            WorkshopResult::Applied);
  EXPECT_EQ(succeeding.cook(Actor::Chef, PORRIDGE), WorkshopResult::Applied);
  EXPECT_EQ(succeeding.snapshot().foods, (std::vector<FoodId>{PORRIDGE}));

  auto at = carrying(seed_for_cook_roll(90), {RICE});
  Workshop failing(at, balance());
  ASSERT_EQ(failing.appraise(Actor::Appraiser, RICE), WorkshopResult::Applied);
  EXPECT_EQ(failing.cook(Actor::Chef, PORRIDGE), WorkshopResult::Failed);
  EXPECT_TRUE(failing.snapshot().foods.empty());
}

TEST(Workshop, DamagedSpatulaSucceedsThroughRoll39AndFailsAtRoll40) {
  auto below = carrying(seed_for_cook_roll(39), {RICE});
  Workshop succeeding(below, balance());
  ASSERT_EQ(succeeding.appraise(Actor::Appraiser, RICE),
            WorkshopResult::Applied);
  succeeding.damage_tool(WorkshopTool::Spatula);
  EXPECT_EQ(succeeding.cook(Actor::Chef, PORRIDGE), WorkshopResult::Applied);

  auto at = carrying(seed_for_cook_roll(40), {RICE});
  Workshop failing(at, balance());
  ASSERT_EQ(failing.appraise(Actor::Appraiser, RICE), WorkshopResult::Applied);
  failing.damage_tool(WorkshopTool::Spatula);
  EXPECT_EQ(failing.cook(Actor::Chef, PORRIDGE), WorkshopResult::Failed);
}

TEST(Workshop, EachToolDamagesAndReplacesIndependently) {
  auto simulation =
      carrying(seed_for_roll(60), {RICE}, {NEW_MAGNIFIER, NEW_SPATULA});
  Workshop workshop(simulation, balance());
  workshop.damage_tool(WorkshopTool::Magnifier);
  EXPECT_EQ(workshop.snapshot().magnifier, ToolState::Damaged);
  EXPECT_EQ(workshop.snapshot().spatula, ToolState::Intact);
  ASSERT_EQ(workshop.replace_tool(WorkshopTool::Magnifier),
            WorkshopResult::Applied);
  EXPECT_EQ(workshop.snapshot().magnifier, ToolState::Intact);
  EXPECT_EQ(simulation.snapshot().chef.tools,
            (std::vector<ToolId>{NEW_SPATULA}));
  workshop.damage_tool(WorkshopTool::Spatula);
  EXPECT_EQ(workshop.snapshot().magnifier, ToolState::Intact);
  EXPECT_EQ(workshop.snapshot().spatula, ToolState::Damaged);
  EXPECT_EQ(workshop.appraise(Actor::Appraiser, RICE), WorkshopResult::Applied);
}

TEST(Workshop, ToolsDamageDuringExplorationButAreReplacedOnlyAtBase) {
  Simulation simulation({.exploration_budget = 10s, .seed = 1});
  Workshop workshop(simulation, balance());
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.collect_tool(Actor::Chef, NEW_SPATULA), Result::Applied);
  workshop.damage_tool(WorkshopTool::Spatula);
  EXPECT_EQ(workshop.snapshot().spatula, ToolState::Damaged);
  EXPECT_EQ(workshop.replace_tool(WorkshopTool::Spatula),
            WorkshopResult::NotInWorkshop);
  EXPECT_EQ(workshop.snapshot().spatula, ToolState::Damaged);
  ASSERT_EQ(simulation.return_to_base(Actor::Chef), Result::Applied);
  EXPECT_EQ(workshop.replace_tool(WorkshopTool::Spatula),
            WorkshopResult::Applied);
  EXPECT_EQ(workshop.snapshot().spatula, ToolState::Intact);
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

TEST(Workshop, BaseWorkUsesNoHealthStaminaOrTime) {
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
  EXPECT_EQ(simulation.snapshot().chef.health, 3);
  EXPECT_EQ(simulation.snapshot().appraiser.health, 3);
}

TEST(Workshop, NewFoodsScoreAboveRepeatsAndRepeatsMissTheThreshold) {
  auto simulation = carrying(lucky_seed(), {RICE, RICE});
  Workshop workshop(simulation, balance());
  ASSERT_EQ(workshop.appraise(Actor::Appraiser, RICE), WorkshopResult::Applied);
  ASSERT_EQ(workshop.cook(Actor::Chef, PORRIDGE), WorkshopResult::Applied);
  ASSERT_EQ(workshop.cook(Actor::Chef, PORRIDGE), WorkshopResult::Applied);

  ASSERT_EQ(workshop.feed(PORRIDGE), WorkshopResult::Applied);
  EXPECT_EQ(workshop.snapshot().last_day,
            (DayOutcome{.completed_day = 1,
                        .reaction = MonsterReaction::Satisfied,
                        .score = 13,
                        .acid_rise = AcidLevel{0}}));
  EXPECT_EQ(simulation.snapshot().acid, AcidLevel{0});

  ASSERT_EQ(workshop.feed(PORRIDGE), WorkshopResult::Applied);
  EXPECT_EQ(workshop.snapshot().last_day,
            (DayOutcome{.completed_day = 2,
                        .reaction = MonsterReaction::Unsatisfied,
                        .score = 0,
                        .acid_rise = AcidLevel{2}}));
  EXPECT_EQ(simulation.snapshot().acid, AcidLevel{2});
  EXPECT_EQ(simulation.snapshot().day, 3U);
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
  EXPECT_EQ(workshop.snapshot().last_day,
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
  ASSERT_EQ(workshop.snapshot().foods,
            (std::vector<FoodId>{PORRIDGE, PORRIDGE}));
  ASSERT_EQ(workshop.feed(PORRIDGE), WorkshopResult::Applied);
  EXPECT_EQ(workshop.snapshot().foods, (std::vector<FoodId>{PORRIDGE}));
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
  auto simulation = carrying(42, {RICE, RICE, EGG}, {NEW_MAGNIFIER});
  Workshop workshop(simulation, balance());
  workshop.damage_tool(WorkshopTool::Magnifier);
  EXPECT_EQ(workshop.appraise(Actor::Appraiser, RICE), WorkshopResult::Failed);
  ASSERT_EQ(workshop.replace_tool(WorkshopTool::Magnifier),
            WorkshopResult::Applied);
  EXPECT_EQ(workshop.appraise(Actor::Appraiser, RICE), WorkshopResult::Applied);
  workshop.damage_tool(WorkshopTool::Spatula);
  EXPECT_EQ(workshop.cook(Actor::Chef, PORRIDGE), WorkshopResult::Failed);
  EXPECT_EQ(workshop.snapshot().rolls, 3U);
}

TEST(Workshop, SameSeedAndCommandsReplayTheSameRun) {
  const auto play = [](std::uint32_t seed) {
    auto simulation = carrying(seed, {RICE, RICE, EGG, EGG});
    Workshop workshop(simulation, balance());
    workshop.damage_tool(WorkshopTool::Spatula);
    workshop.damage_tool(WorkshopTool::Magnifier);
    std::vector<WorkshopResult> results;
    for (int attempt = 0; attempt < 6; ++attempt) {
      results.push_back(workshop.appraise(Actor::Appraiser, RICE));
      results.push_back(workshop.appraise(Actor::Appraiser, EGG));
      results.push_back(workshop.cook(Actor::Chef, OMELET));
    }
    return std::make_tuple(results, workshop.snapshot(), simulation.snapshot());
  };
  EXPECT_EQ(play(1234), play(1234));
  EXPECT_NE(std::get<0>(play(1234)), std::get<0>(play(4321)));
}

TEST(Workshop, ReplacementSpendsOneCarriedToolAndRestoresTheIntactRate) {
  auto simulation =
      carrying(seed_for_roll(60), {RICE}, {NEW_MAGNIFIER, NEW_MAGNIFIER});
  Workshop workshop(simulation, balance());
  workshop.damage_tool(WorkshopTool::Magnifier);
  ASSERT_EQ(workshop.snapshot().magnifier, ToolState::Damaged);
  EXPECT_EQ(workshop.replace_tool(WorkshopTool::Magnifier),
            WorkshopResult::Applied);
  EXPECT_EQ(simulation.snapshot().chef.tools,
            (std::vector<ToolId>{NEW_MAGNIFIER}));
  // Roll 60 succeeds only at the intact rate.
  EXPECT_EQ(workshop.appraise(Actor::Appraiser, RICE), WorkshopResult::Applied);
}

TEST(Workshop, ReplacementNeedsTheConfiguredToolForThatBaseTool) {
  auto simulation = carrying(1, {}, {NEW_SPATULA});
  Workshop workshop(simulation, balance());
  workshop.damage_tool(WorkshopTool::Magnifier);
  EXPECT_EQ(workshop.replace_tool(WorkshopTool::Magnifier),
            WorkshopResult::NotCarried);
  EXPECT_EQ(workshop.snapshot().magnifier, ToolState::Damaged);
  EXPECT_EQ(simulation.snapshot().chef.tools,
            (std::vector<ToolId>{NEW_SPATULA}));
  workshop.damage_tool(WorkshopTool::Spatula);
  EXPECT_EQ(workshop.replace_tool(WorkshopTool::Spatula),
            WorkshopResult::Applied);
  EXPECT_EQ(workshop.snapshot().spatula, ToolState::Intact);
}

TEST(Workshop, ReplacingAnIntactToolIsRefusedAndKeepsTheCarriedTool) {
  auto simulation = carrying(1, {}, {NEW_MAGNIFIER});
  Workshop workshop(simulation, balance());
  EXPECT_EQ(workshop.replace_tool(WorkshopTool::Magnifier),
            WorkshopResult::ToolIntact);
  EXPECT_EQ(simulation.snapshot().chef.tools,
            (std::vector<ToolId>{NEW_MAGNIFIER}));
  workshop.damage_tool(WorkshopTool::Magnifier);
  EXPECT_EQ(workshop.replace_tool(WorkshopTool::Magnifier),
            WorkshopResult::Applied);
}

TEST(Workshop, ExhaustionDamagesOnlyThatActorsBaseTool) {
  Simulation simulation({.exploration_budget = 10s, .seed = 1});
  Workshop workshop(simulation, balance());
  const auto exhaust = [&](Actor actor) {
    ASSERT_EQ(simulation.enter(actor), Result::Applied);
    for (int hit = 0; hit < 3; ++hit) {
      ASSERT_EQ(simulation.damage(actor), Result::Applied);
    }
  };
  exhaust(Actor::Appraiser);
  EXPECT_EQ(workshop.snapshot().magnifier, ToolState::Damaged);
  EXPECT_EQ(workshop.snapshot().spatula, ToolState::Intact);
  exhaust(Actor::Chef);
  EXPECT_EQ(workshop.snapshot().spatula, ToolState::Damaged);
}

TEST(Workshop, ExhaustionLowersTheNextAttemptToTheDamagedRate) {
  auto simulation = carrying(seed_for_roll(60), {RICE});
  Workshop workshop(simulation, balance());
  ASSERT_EQ(simulation.enter(Actor::Appraiser), Result::Applied);
  for (int hit = 0; hit < 3; ++hit) {
    ASSERT_EQ(simulation.damage(Actor::Appraiser), Result::Applied);
  }
  // Roll 60 succeeds at the intact rate and fails at the damaged rate.
  EXPECT_EQ(workshop.appraise(Actor::Appraiser, RICE), WorkshopResult::Failed);
}

TEST(Workshop, HarmlessReturnsLeaveToolsIntact) {
  Simulation simulation({.exploration_budget = 10s, .seed = 1});
  Workshop workshop(simulation, balance());
  ASSERT_EQ(simulation.enter(Actor::Appraiser), Result::Applied);
  ASSERT_EQ(simulation.damage(Actor::Appraiser), Result::Applied);
  ASSERT_EQ(simulation.return_to_base(Actor::Appraiser), Result::Applied);
  EXPECT_EQ(workshop.snapshot().magnifier, ToolState::Intact);
}

TEST(Workshop, ReplacementSettlesExhaustionUntilTheNextOne) {
  auto simulation = carrying(1, {}, {NEW_MAGNIFIER, NEW_MAGNIFIER});
  Workshop workshop(simulation, balance());
  const auto exhaust = [&] {
    ASSERT_EQ(simulation.enter(Actor::Appraiser), Result::Applied);
    for (int hit = 0; hit < 3; ++hit) {
      ASSERT_EQ(simulation.damage(Actor::Appraiser), Result::Applied);
    }
  };
  exhaust();
  ASSERT_EQ(workshop.snapshot().magnifier, ToolState::Damaged);
  ASSERT_EQ(workshop.replace_tool(WorkshopTool::Magnifier),
            WorkshopResult::Applied);
  EXPECT_EQ(workshop.snapshot().magnifier, ToolState::Intact);
  exhaust();
  EXPECT_EQ(workshop.snapshot().magnifier, ToolState::Damaged);
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

TEST(Workshop, RejectsOneToolIdentityForBothBaseTools) {
  Simulation simulation({.exploration_budget = 10s, .seed = 1});
  auto shared = balance();
  shared.spatula = shared.magnifier;
  EXPECT_THROW(Workshop(simulation, shared), std::invalid_argument);
  EXPECT_NO_THROW(Workshop(simulation, balance()));
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
               {.id = ACID_FLOOR,
                .area = {.x = 9},
                .destroyed_after = WorldThreshold{.acid = AcidLevel{7}}}});
  Exploration exploration(
      simulation, world,
      {.base = {.x = -5}, .entry = {}, .sources = {{.id = SOURCE}}});
  Workshop workshop(simulation, balance());
  const std::array<ExplorationInput, 2> idle{};

  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Applied);
  ASSERT_EQ(exploration.advance(1s, idle, {}), Result::Applied);
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
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.advance(100ms, idle, after.acid), Result::Applied);
  EXPECT_EQ(simulation.snapshot().remaining_exploration, 900ms);
  EXPECT_EQ(world.snapshot().progression.acid, AcidLevel{7});
  const auto city = world.snapshot();
  const auto floor =
      std::ranges::find(city.objects, ACID_FLOOR, &ObjectSnapshot::id);
  ASSERT_NE(floor, city.objects.end());
  EXPECT_EQ(floor->state, ObjectState::Destroyed);
}

TEST(WorkshopIntegration, FoundToolRestoresTheBaseToolAfterAnExhaustion) {
  constexpr ObjectId FLOOR{1};
  constexpr ObjectId CRATE{2};
  Simulation simulation({.exploration_budget = 1s, .seed = seed_for_roll(60)});
  World world({{.id = FLOOR, .area = {.x = 0, .y = 0, .width = 5, .height = 1}},
               {.id = CRATE,
                .kind = ObjectKind::MaterialSource,
                .area = {.x = 1},
                .tool = NEW_MAGNIFIER}});
  Exploration exploration(
      simulation, world,
      {.base = {.x = -5}, .entry = {}, .sources = {{.id = CRATE}}});
  Workshop workshop(simulation, balance());

  ASSERT_EQ(exploration.enter(Actor::Appraiser), Result::Applied);
  ASSERT_EQ(exploration.collect(Actor::Appraiser, CRATE),
            CollectionResult::Applied);
  for (int hit = 0; hit < 3; ++hit) {
    ASSERT_EQ(exploration.damage(Actor::Appraiser), Result::Applied);
  }
  ASSERT_EQ(simulation.snapshot().appraiser.location, Location::Base);
  ASSERT_EQ(simulation.snapshot().appraiser.tools,
            (std::vector<ToolId>{NEW_MAGNIFIER}));
  EXPECT_EQ(workshop.snapshot().magnifier, ToolState::Damaged);

  EXPECT_EQ(workshop.replace_tool(WorkshopTool::Magnifier),
            WorkshopResult::Applied);
  EXPECT_EQ(workshop.snapshot().magnifier, ToolState::Intact);
  EXPECT_TRUE(simulation.snapshot().appraiser.tools.empty());
  EXPECT_EQ(workshop.replace_tool(WorkshopTool::Magnifier),
            WorkshopResult::ToolIntact);
}

} // namespace
} // namespace neocity::survival
