#include "search_support.hpp"

#include <survival/ending.hpp>
#include <survival/exploration.hpp>
#include <survival/workshop.hpp>

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdint>

namespace neocity::survival {
namespace {

using namespace std::chrono_literals;
constexpr EndingConfig SEVEN{.satisfied_days = 7};
constexpr IngredientId RICE{1};
constexpr FoodId PORRIDGE{10};
constexpr ObjectId FLOOR{1};
constexpr ObjectId SOURCE{2};
constexpr ObjectId WEAK{3};
constexpr AcidLevel ACID{10};
constexpr std::array<ExplorationInput, 2> IDLE{};

/// Rice alone scores 5, the satisfying score, on every feeding.
WorkshopConfig balance() {
  return {.recipes = {{.food = PORRIDGE, .ingredients = {RICE}}},
          .new_food_score = 10,
          .new_ingredient_score = 3,
          .satisfying_score = 5,
          .disliked_acid_rise = AcidLevel{7},
          .unsatisfied_acid_rise = AcidLevel{2},
          .flavors = {{.ingredient = RICE, .taste = 5}}};
}

/// Gathers rice, appraises and cooks until a dish is stored, which the roll
/// failing 10% of the time may take several tries.
void store_a_dish(Simulation &simulation, Workshop &workshop) {
  for (int attempt = 0; attempt < 50 && workshop.snapshot().foods.empty();
       ++attempt) {
    ASSERT_EQ(simulation.enter(Actor::Appraiser), Result::Applied);
    ASSERT_EQ(simulation.collect(Actor::Appraiser, RICE), Result::Applied);
    ASSERT_EQ(simulation.return_to_base(Actor::Appraiser), Result::Applied);
    if (workshop.snapshot().identified.empty()) {
      (void)workshop.appraise(Actor::Appraiser, RICE);
    } else {
      (void)workshop.cook(Actor::Chef, PORRIDGE);
    }
  }
  ASSERT_FALSE(workshop.snapshot().foods.empty());
}

/// Feeds a dish the monster is satisfied by, finishing the day.
void feed_to_satisfaction(Simulation &simulation, Workshop &workshop) {
  store_a_dish(simulation, workshop);
  ASSERT_EQ(workshop.feed(PORRIDGE), WorkshopResult::Applied);
  const auto last = workshop.snapshot().last_day;
  if (!last) {
    FAIL() << "feeding left no day outcome";
  }
  ASSERT_EQ(last->reaction, MonsterReaction::Satisfied);
}

TEST(Ending, TemporaryValueIsSevenSatisfiedDays) {
  EXPECT_EQ(TEMPORARY_ENDING.satisfied_days, 7U);
}

TEST(Ending, SixSatisfiedDaysAreNoEndingAndTheSeventhIsMonsterSatisfied) {
  Simulation simulation({.exploration_budget = 10s, .seed = 1});
  Workshop workshop(simulation, balance());
  Endings endings(simulation, workshop, SEVEN);
  EXPECT_EQ(endings.check(), std::nullopt);
  for (std::uint32_t day = 1; day <= 6; ++day) {
    feed_to_satisfaction(simulation, workshop);
    ASSERT_EQ(workshop.snapshot().satisfied_days, day);
    EXPECT_EQ(endings.check(), std::nullopt) << "day " << day;
    EXPECT_EQ(simulation.snapshot().phase, Phase::Workshop);
  }
  feed_to_satisfaction(simulation, workshop);
  EXPECT_EQ(endings.check(), Ending::MonsterSatisfied);
  EXPECT_EQ(simulation.snapshot().phase, Phase::Ended);
  EXPECT_EQ(simulation.snapshot().ending, Ending::MonsterSatisfied);
}

TEST(Ending, TheSatisfiedDaysNeedNotFollowEachOther) {
  Simulation simulation({.exploration_budget = 10s, .seed = 1});
  Workshop workshop(simulation, balance());
  Endings endings(simulation, workshop, SEVEN);
  for (std::uint32_t day = 0; day < 3; ++day) {
    feed_to_satisfaction(simulation, workshop);
  }
  ASSERT_EQ(workshop.end_day_unfed(AcidLevel{1}), WorkshopResult::Applied);
  for (std::uint32_t day = 0; day < 3; ++day) {
    feed_to_satisfaction(simulation, workshop);
  }
  ASSERT_EQ(workshop.end_day_unfed(AcidLevel{1}), WorkshopResult::Applied);
  // The control: six satisfied days with unfed days between them end nothing.
  EXPECT_EQ(endings.check(), std::nullopt);
  feed_to_satisfaction(simulation, workshop);
  EXPECT_EQ(endings.check(), Ending::MonsterSatisfied);
}

TEST(Ending, TheEndingWaitsWhileThePartyIsInTheCity) {
  Simulation simulation({.exploration_budget = 10s, .seed = 1});
  Workshop workshop(simulation, balance());
  Endings endings(simulation, workshop, SEVEN);
  for (std::uint32_t day = 0; day < 7; ++day) {
    feed_to_satisfaction(simulation, workshop);
  }
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  EXPECT_EQ(endings.check(), std::nullopt);
  EXPECT_EQ(simulation.snapshot().phase, Phase::Exploration);
  ASSERT_EQ(simulation.return_to_base(Actor::Chef), Result::Applied);
  EXPECT_EQ(endings.check(), Ending::MonsterSatisfied);
}

TEST(Ending, ARepeatedCheckAfterTheEndingChangesNothing) {
  Simulation simulation({.exploration_budget = 10s, .seed = 1});
  Workshop workshop(simulation, balance());
  Endings endings(simulation, workshop, SEVEN);
  for (std::uint32_t day = 0; day < 7; ++day) {
    feed_to_satisfaction(simulation, workshop);
  }
  ASSERT_EQ(endings.check(), Ending::MonsterSatisfied);
  const auto ended = simulation.snapshot();
  EXPECT_EQ(endings.check(), std::nullopt);
  EXPECT_EQ(simulation.snapshot(), ended);
}

TEST(Ending, AFullAcidLossIsNotTurnedIntoAnEnding) {
  const auto seven_days_then = [](Simulation &simulation, Workshop &workshop) {
    for (std::uint32_t day = 0; day < 7; ++day) {
      feed_to_satisfaction(simulation, workshop);
    }
  };
  // The control: the seventh satisfied day, checked, ends the run.
  Simulation spared({.exploration_budget = 10s, .seed = 1, .full_acid = ACID});
  Workshop spared_workshop(spared, balance());
  Endings open(spared, spared_workshop, SEVEN);
  seven_days_then(spared, spared_workshop);
  EXPECT_EQ(open.check(), Ending::MonsterSatisfied);

  // The same seven days, with the acid filling before the check: lost first.
  Simulation lost({.exploration_budget = 10s, .seed = 1, .full_acid = ACID});
  Workshop lost_workshop(lost, balance());
  Endings closed(lost, lost_workshop, SEVEN);
  seven_days_then(lost, lost_workshop);
  ASSERT_EQ(lost_workshop.end_day_unfed(ACID), WorkshopResult::Applied);
  ASSERT_EQ(lost.snapshot().phase, Phase::Lost);
  ASSERT_GE(lost_workshop.snapshot().satisfied_days, SEVEN.satisfied_days);
  EXPECT_EQ(closed.check(), std::nullopt);
  EXPECT_EQ(lost.snapshot().phase, Phase::Lost);
  EXPECT_EQ(lost.snapshot().ending, std::nullopt);
}

TEST(Ending, AStoredDishCannotBeFedAfterTheEnding) {
  Simulation simulation({.exploration_budget = 10s, .seed = 1});
  Workshop workshop(simulation, balance());
  store_a_dish(simulation, workshop);
  ASSERT_EQ(simulation.end(Ending::MonsterSatisfied), Result::Applied);
  const auto progression = simulation.snapshot();
  const auto base = workshop.snapshot();
  ASSERT_EQ(base.foods.size(), 1U);
  EXPECT_EQ(workshop.feed(PORRIDGE), WorkshopResult::NotInWorkshop);
  EXPECT_EQ(simulation.snapshot(), progression);
  EXPECT_EQ(workshop.snapshot(), base);
}

// World has no phase; its update is reached only through these.
TEST(Ending, NoFeatureCommandOutsideWorldChangesStateAfterTheEnding) {
  Simulation simulation({.exploration_budget = 10s, .seed = 2});
  World world({{.id = FLOOR, .area = {.x = 0, .y = 0, .width = 8, .height = 1}},
               {.id = SOURCE,
                .kind = ObjectKind::MaterialSource,
                .area = {.x = 1},
                .ingredient = RICE},
               {.id = WEAK,
                .kind = ObjectKind::Npc,
                .area = {.x = 6},
                .destroyed_from_day = 2}});
  Exploration exploration(simulation, world,
                          {.base = {.x = -5, .y = -5},
                           .entry = {},
                           .exits = {},
                           .interiors = {},
                           .collection_distance = 2,
                           .sources = {stock(SOURCE, 1)},
                           .obstacles = {},
                           .patrols = {}});
  // The control: entering and collecting apply before the ending.
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(collect(exploration, Actor::Chef, SOURCE),
            CollectionResult::Applied);
  ASSERT_EQ(exploration.return_to_base(Actor::Chef), Result::Applied);
  Workshop workshop(simulation, balance());
  Endings endings(simulation, workshop, {.satisfied_days = 1});
  feed_to_satisfaction(simulation, workshop);
  ASSERT_EQ(endings.check(), Ending::MonsterSatisfied);

  const auto progression = simulation.snapshot();
  const auto city = world.snapshot();
  const auto explored = exploration.snapshot();
  const auto base = workshop.snapshot();
  EXPECT_EQ(exploration.enter(Actor::Chef), Result::RunOver);
  EXPECT_EQ(exploration.enter(Actor::Appraiser), Result::RunOver);
  EXPECT_EQ(exploration.return_to_base(Actor::Chef), Result::NotExploring);
  EXPECT_EQ(collect(exploration, Actor::Chef, SOURCE),
            CollectionResult::NotExploring);
  EXPECT_EQ(exploration.advance(1s, IDLE), Result::RunOver);
  EXPECT_EQ(workshop.appraise(Actor::Appraiser, RICE),
            WorkshopResult::NotInWorkshop);
  EXPECT_EQ(workshop.cook(Actor::Chef, PORRIDGE),
            WorkshopResult::NotInWorkshop);
  EXPECT_EQ(workshop.feed(PORRIDGE), WorkshopResult::NoFood);
  EXPECT_EQ(workshop.end_day_unfed(AcidLevel{9}),
            WorkshopResult::NotInWorkshop);
  EXPECT_EQ(endings.check(), std::nullopt);
  EXPECT_EQ(simulation.snapshot(), progression);
  EXPECT_EQ(world.snapshot(), city);
  EXPECT_EQ(exploration.snapshot(), explored);
  EXPECT_EQ(workshop.snapshot(), base);
}

TEST(Ending, NoFeatureCommandOutsideWorldChangesStateAfterALoss) {
  Simulation simulation(
      {.exploration_budget = 10s, .seed = 2, .full_acid = ACID});
  World world({{.id = FLOOR, .area = {.x = 0, .y = 0, .width = 8, .height = 1}},
               {.id = SOURCE,
                .kind = ObjectKind::MaterialSource,
                .area = {.x = 1},
                .ingredient = RICE}});
  Exploration exploration(simulation, world,
                          {.base = {.x = -5, .y = -5},
                           .entry = {},
                           .exits = {},
                           .interiors = {},
                           .collection_distance = 2,
                           .sources = {stock(SOURCE, 1)},
                           .obstacles = {},
                           .patrols = {}});
  // The control: entering and collecting apply before the loss.
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(collect(exploration, Actor::Chef, SOURCE),
            CollectionResult::Applied);
  ASSERT_EQ(exploration.return_to_base(Actor::Chef), Result::Applied);
  Workshop workshop(simulation, balance());
  Endings endings(simulation, workshop, {.satisfied_days = 0});
  ASSERT_EQ(workshop.end_day_unfed(ACID), WorkshopResult::Applied);
  ASSERT_EQ(simulation.snapshot().phase, Phase::Lost);

  const auto progression = simulation.snapshot();
  const auto city = world.snapshot();
  const auto explored = exploration.snapshot();
  const auto base = workshop.snapshot();
  EXPECT_EQ(exploration.enter(Actor::Chef), Result::RunOver);
  EXPECT_EQ(exploration.enter(Actor::Appraiser), Result::RunOver);
  EXPECT_EQ(exploration.return_to_base(Actor::Chef), Result::NotExploring);
  EXPECT_EQ(collect(exploration, Actor::Chef, SOURCE),
            CollectionResult::NotExploring);
  EXPECT_EQ(exploration.advance(1s, IDLE), Result::RunOver);
  EXPECT_EQ(workshop.appraise(Actor::Appraiser, RICE),
            WorkshopResult::NotInWorkshop);
  EXPECT_EQ(workshop.cook(Actor::Chef, PORRIDGE),
            WorkshopResult::NotInWorkshop);
  EXPECT_EQ(workshop.end_day_unfed(AcidLevel{9}),
            WorkshopResult::NotInWorkshop);
  EXPECT_EQ(endings.check(), std::nullopt);
  EXPECT_EQ(simulation.snapshot(), progression);
  EXPECT_EQ(world.snapshot(), city);
  EXPECT_EQ(exploration.snapshot(), explored);
  EXPECT_EQ(workshop.snapshot(), base);
}

} // namespace
} // namespace neocity::survival
