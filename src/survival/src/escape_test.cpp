#include <survival/escape.hpp>
#include <survival/exploration.hpp>
#include <survival/workshop.hpp>

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <random>

namespace neocity::survival {
namespace {

using namespace std::chrono_literals;
constexpr EscapeConfig CERTAIN{.first_day = 30, .chance_percent = 100};
constexpr EscapeConfig NEVER{.first_day = 30, .chance_percent = 0};
constexpr EscapeConfig EVEN{.first_day = 1, .chance_percent = 50};
constexpr IngredientId RICE{1};
constexpr FoodId PORRIDGE{10};
constexpr ObjectId FLOOR{1};
constexpr ObjectId SOURCE{2};
constexpr ObjectId WEAK{3};
constexpr AcidLevel ACID{10};
constexpr std::array<ExplorationInput, 2> IDLE{};

void finish_days(Simulation &simulation, std::uint32_t days) {
  for (std::uint32_t day = 0; day < days; ++day) {
    ASSERT_EQ(simulation.finish_day(AcidLevel{}), Result::Applied);
  }
}

WorkshopConfig balance() {
  return {.recipes = {{.food = PORRIDGE, .ingredients = {RICE}}},
          .new_food_score = 10,
          .new_ingredient_score = 3,
          .satisfying_score = 12,
          .disliked_acid_rise = AcidLevel{7},
          .unsatisfied_acid_rise = AcidLevel{2},
          .flavors = {}};
}

/// A run at base with one ingredient carried by the appraiser.
Simulation carrying_rice(std::uint32_t seed) {
  Simulation simulation({.exploration_budget = 10s, .seed = seed});
  EXPECT_EQ(simulation.enter(Actor::Appraiser), Result::Applied);
  EXPECT_EQ(simulation.collect(Actor::Appraiser, RICE), Result::Applied);
  EXPECT_EQ(simulation.return_to_base(Actor::Appraiser), Result::Applied);
  return simulation;
}

/// The finished day of the first hit at even odds, checking days from one.
std::uint32_t first_hit(Simulation &simulation, Escape &escape) {
  for (std::uint32_t day = 1; day < 200; ++day) {
    EXPECT_EQ(simulation.finish_day(AcidLevel{}), Result::Applied);
    if (escape.check() == EscapeResult::Escaped) {
      return day;
    }
  }
  ADD_FAILURE() << "no escape in 200 days";
  return 0;
}

/// The same question put to the engine the workshop builds from the run seed.
std::uint32_t first_hit_on_the_seed_engine(std::uint32_t seed) {
  std::mt19937 engine(seed);
  std::uint32_t day = 1;
  while (engine() % 100U >= EVEN.chance_percent) {
    ++day;
  }
  return day;
}

TEST(Escape, TemporaryValuesCheckFromDayThirtyAtOneInFive) {
  EXPECT_EQ(TEMPORARY_ESCAPE.first_day, 30U);
  EXPECT_EQ(TEMPORARY_ESCAPE.chance_percent, 20U);
}

TEST(Escape, TheFirstCheckFollowsTheFirstDayFinishing) {
  Simulation simulation({.exploration_budget = 10s, .seed = 1});
  Escape escape(simulation, CERTAIN);
  EXPECT_EQ(escape.check(), EscapeResult::NotDue);
  finish_days(simulation, 29);
  ASSERT_EQ(simulation.snapshot().day, 30U);
  EXPECT_EQ(escape.check(), EscapeResult::NotDue);
  EXPECT_EQ(escape.snapshot().rolls, 0U);
  EXPECT_EQ(simulation.snapshot().phase, Phase::Workshop);
  finish_days(simulation, 1);
  EXPECT_EQ(escape.check(), EscapeResult::Escaped);
  EXPECT_EQ(escape.snapshot(), (EscapeSnapshot{.checked_day = 30, .rolls = 1}));
  EXPECT_EQ(simulation.snapshot().phase, Phase::Escaped);
}

TEST(Escape, AMissStaysInTheWorkshopAndEachDayRollsOnce) {
  Simulation simulation({.exploration_budget = 10s, .seed = 1});
  Escape escape(simulation, NEVER);
  finish_days(simulation, 30);
  EXPECT_EQ(escape.check(), EscapeResult::Stayed);
  EXPECT_EQ(escape.check(), EscapeResult::NotDue);
  EXPECT_EQ(escape.snapshot().rolls, 1U);
  EXPECT_EQ(simulation.snapshot().phase, Phase::Workshop);
  finish_days(simulation, 1);
  EXPECT_EQ(escape.check(), EscapeResult::Stayed);
  EXPECT_EQ(escape.snapshot().rolls, 2U);
}

TEST(Escape, ARollWaitsWhileThePartyIsInTheCity) {
  Simulation simulation({.exploration_budget = 10s, .seed = 1});
  Escape escape(simulation, CERTAIN);
  finish_days(simulation, 30);
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  EXPECT_EQ(escape.check(), EscapeResult::NotDue);
  EXPECT_EQ(escape.snapshot().rolls, 0U);
  ASSERT_EQ(simulation.return_to_base(Actor::Chef), Result::Applied);
  EXPECT_EQ(escape.check(), EscapeResult::Escaped);
}

TEST(Escape, NothingIsRolledAfterEscaping) {
  Simulation simulation({.exploration_budget = 10s, .seed = 1});
  Escape escape(simulation, CERTAIN);
  finish_days(simulation, 30);
  ASSERT_EQ(escape.check(), EscapeResult::Escaped);
  const auto escaped = escape.snapshot();
  EXPECT_EQ(escape.check(), EscapeResult::NotDue);
  EXPECT_EQ(escape.snapshot(), escaped);
}

TEST(Escape, TheSameSeedEscapesOnTheSameDay) {
  Simulation first({.exploration_budget = 10s, .seed = 7});
  Escape first_escape(first, EVEN);
  Simulation second({.exploration_budget = 10s, .seed = 7});
  Escape second_escape(second, EVEN);
  EXPECT_EQ(first_hit(first, first_escape), first_hit(second, second_escape));
}

TEST(Escape, TheStreamIsNotTheEngineTheWorkshopBuildsFromTheSeed) {
  std::uint32_t different = 0;
  for (std::uint32_t seed = 0; seed < 64; ++seed) {
    Simulation simulation({.exploration_budget = 10s, .seed = seed});
    Escape escape(simulation, EVEN);
    if (first_hit(simulation, escape) != first_hit_on_the_seed_engine(seed)) {
      ++different;
    }
  }
  // Two independent streams at even odds agree on about a third of the seeds.
  EXPECT_GT(different, 24U);
}

TEST(Escape, EscapeChecksDoNotMoveWorkshopRolls) {
  for (std::uint32_t seed = 0; seed < 32; ++seed) {
    auto alone = carrying_rice(seed);
    Workshop alone_workshop(alone, balance());
    auto checked = carrying_rice(seed);
    Workshop checked_workshop(checked, balance());
    Escape escape(checked, {.first_day = 1, .chance_percent = 0});
    finish_days(checked, 3);
    ASSERT_EQ(escape.check(), EscapeResult::Stayed);
    ASSERT_EQ(escape.snapshot().rolls, 1U);
    EXPECT_EQ(checked_workshop.appraise(Actor::Appraiser, RICE),
              alone_workshop.appraise(Actor::Appraiser, RICE));
    EXPECT_EQ(checked_workshop.snapshot().rolls, 1U);
  }
}

TEST(Escape, WorkshopRollsDoNotMoveTheEscapeDay) {
  for (std::uint32_t seed = 0; seed < 32; ++seed) {
    Simulation quiet({.exploration_budget = 10s, .seed = seed});
    Escape quiet_escape(quiet, EVEN);
    auto rolled = carrying_rice(seed);
    Workshop workshop(rolled, balance());
    Escape rolled_escape(rolled, EVEN);
    const auto appraised = workshop.appraise(Actor::Appraiser, RICE);
    ASSERT_TRUE(appraised == WorkshopResult::Applied ||
                appraised == WorkshopResult::Failed);
    EXPECT_EQ(first_hit(quiet, quiet_escape), first_hit(rolled, rolled_escape));
  }
}

TEST(Escape, AStoredDishCannotBeFedAfterEscape) {
  for (std::uint32_t seed = 0; seed < 64; ++seed) {
    auto simulation = carrying_rice(seed);
    Workshop workshop(simulation, balance());
    if (workshop.appraise(Actor::Appraiser, RICE) != WorkshopResult::Applied ||
        workshop.cook(Actor::Chef, PORRIDGE) != WorkshopResult::Applied) {
      continue;
    }
    ASSERT_EQ(simulation.escape(), Result::Applied);
    const auto progression = simulation.snapshot();
    const auto base = workshop.snapshot();
    ASSERT_EQ(base.foods.size(), 1U);
    EXPECT_EQ(workshop.feed(PORRIDGE), WorkshopResult::NotInWorkshop);
    EXPECT_EQ(simulation.snapshot(), progression);
    EXPECT_EQ(workshop.snapshot(), base);
    return;
  }
  ADD_FAILURE() << "no seed in range cooks a dish";
}

// World has no phase; its update is reached only through these.
TEST(Escape, NoFeatureCommandOutsideWorldChangesStateAfterEscape) {
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
                           .sources = {{.id = SOURCE}},
                           .obstacles = {},
                           .patrols = {}});
  // The control: entering and collecting apply before the escape.
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Applied);
  ASSERT_EQ(exploration.return_to_base(Actor::Chef), Result::Applied);
  Workshop workshop(simulation, balance());
  Escape escape(simulation, {.first_day = 1, .chance_percent = 100});
  ASSERT_EQ(simulation.finish_day(ACID), Result::Applied);
  ASSERT_EQ(escape.check(), EscapeResult::Escaped);

  const auto progression = simulation.snapshot();
  const auto city = world.snapshot();
  const auto explored = exploration.snapshot();
  const auto base = workshop.snapshot();
  const auto rolls = escape.snapshot();
  ASSERT_EQ(progression.acid, ACID);
  EXPECT_EQ(exploration.enter(Actor::Chef), Result::RunOver);
  EXPECT_EQ(exploration.enter(Actor::Appraiser), Result::RunOver);
  EXPECT_EQ(exploration.return_to_base(Actor::Chef), Result::NotExploring);
  EXPECT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::NotExploring);
  EXPECT_EQ(exploration.advance(1s, IDLE), Result::RunOver);
  EXPECT_EQ(workshop.appraise(Actor::Appraiser, RICE),
            WorkshopResult::NotInWorkshop);
  EXPECT_EQ(workshop.cook(Actor::Chef, PORRIDGE),
            WorkshopResult::NotInWorkshop);
  EXPECT_EQ(workshop.feed(PORRIDGE), WorkshopResult::NoFood);
  EXPECT_EQ(workshop.end_day_unfed(AcidLevel{9}),
            WorkshopResult::NotInWorkshop);
  EXPECT_EQ(escape.check(), EscapeResult::NotDue);
  EXPECT_EQ(simulation.snapshot(), progression);
  EXPECT_EQ(world.snapshot(), city);
  EXPECT_EQ(exploration.snapshot(), explored);
  EXPECT_EQ(workshop.snapshot(), base);
  EXPECT_EQ(escape.snapshot(), rolls);
}

TEST(Escape, ALossOnTheDayOfACheckComesFirstAndDrawsNothing) {
  Simulation lost(
      {.exploration_budget = 10s, .seed = 1, .full_acid = AcidLevel{30}});
  Escape closed(lost, CERTAIN);
  for (std::uint32_t day = 0; day < 30; ++day) {
    ASSERT_EQ(lost.finish_day(AcidLevel{1}), Result::Applied);
  }
  ASSERT_EQ(lost.snapshot().phase, Phase::Lost);
  EXPECT_EQ(closed.check(), EscapeResult::NotDue);
  EXPECT_EQ(closed.snapshot().rolls, 0U);
  EXPECT_EQ(lost.snapshot().phase, Phase::Lost);

  // The control: one more unit of room and the same day's check escapes.
  Simulation spared(
      {.exploration_budget = 10s, .seed = 1, .full_acid = AcidLevel{31}});
  Escape open(spared, CERTAIN);
  for (std::uint32_t day = 0; day < 30; ++day) {
    ASSERT_EQ(spared.finish_day(AcidLevel{1}), Result::Applied);
  }
  EXPECT_EQ(open.check(), EscapeResult::Escaped);
  EXPECT_EQ(spared.snapshot().phase, Phase::Escaped);
}

TEST(Escape, NoFeatureCommandOutsideWorldChangesStateAfterALoss) {
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
                           .sources = {{.id = SOURCE}},
                           .obstacles = {},
                           .patrols = {}});
  // The control: entering and collecting apply before the loss.
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Applied);
  ASSERT_EQ(exploration.return_to_base(Actor::Chef), Result::Applied);
  Workshop workshop(simulation, balance());
  Escape escape(simulation, {.first_day = 1, .chance_percent = 100});
  ASSERT_EQ(workshop.end_day_unfed(ACID), WorkshopResult::Applied);
  ASSERT_EQ(simulation.snapshot().phase, Phase::Lost);

  const auto progression = simulation.snapshot();
  const auto city = world.snapshot();
  const auto explored = exploration.snapshot();
  const auto base = workshop.snapshot();
  const auto rolls = escape.snapshot();
  EXPECT_EQ(exploration.enter(Actor::Chef), Result::RunOver);
  EXPECT_EQ(exploration.enter(Actor::Appraiser), Result::RunOver);
  EXPECT_EQ(exploration.return_to_base(Actor::Chef), Result::NotExploring);
  EXPECT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::NotExploring);
  EXPECT_EQ(exploration.advance(1s, IDLE), Result::RunOver);
  EXPECT_EQ(workshop.appraise(Actor::Appraiser, RICE),
            WorkshopResult::NotInWorkshop);
  EXPECT_EQ(workshop.cook(Actor::Chef, PORRIDGE),
            WorkshopResult::NotInWorkshop);
  EXPECT_EQ(workshop.end_day_unfed(AcidLevel{9}),
            WorkshopResult::NotInWorkshop);
  EXPECT_EQ(escape.check(), EscapeResult::NotDue);
  EXPECT_EQ(simulation.snapshot(), progression);
  EXPECT_EQ(world.snapshot(), city);
  EXPECT_EQ(exploration.snapshot(), explored);
  EXPECT_EQ(workshop.snapshot(), base);
  EXPECT_EQ(escape.snapshot(), rolls);
}

} // namespace
} // namespace neocity::survival
