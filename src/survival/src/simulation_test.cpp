#include <survival/simulation.hpp>

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace neocity::survival {
namespace {

using namespace std::chrono_literals;

constexpr IngredientId RICE{5};

TEST(Survival, StartsWithTwoActorsAtAnUntimedBase) {
  const Simulation simulation({.exploration_budget = 12s, .seed = 42});
  const auto state = simulation.snapshot();
  EXPECT_EQ(state.chef.location, Location::Base);
  EXPECT_EQ(state.appraiser.location, Location::Base);
  EXPECT_TRUE(state.chef.inventory.empty());
  EXPECT_TRUE(state.appraiser.inventory.empty());
  EXPECT_EQ(state.phase, Phase::Workshop);
  EXPECT_EQ(state.day, 1U);
  EXPECT_EQ(state.remaining_exploration, 12s);
  EXPECT_EQ(state.acid, AcidLevel{});
  EXPECT_EQ(state.seed, 42U);
}

TEST(Survival, RequiresAPositiveCallerSuppliedExplorationBudget) {
  EXPECT_THROW((Simulation({.exploration_budget = 0us, .seed = 1})),
               std::invalid_argument);
  EXPECT_THROW((Simulation({.exploration_budget = -1us, .seed = 1})),
               std::invalid_argument);
  const Simulation accepted({.exploration_budget = 1us, .seed = 1});
  EXPECT_EQ(accepted.snapshot().remaining_exploration, 1us);
}

TEST(Survival, TimeExpiryReturnsBothActorsAndPreservesTheirInventory) {
  Simulation simulation({.exploration_budget = 10s, .seed = 2});
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.enter(Actor::Appraiser), Result::Applied);
  ASSERT_EQ(simulation.collect(Actor::Chef, IngredientId{17}), Result::Applied);
  ASSERT_EQ(simulation.collect(Actor::Appraiser, IngredientId{23}),
            Result::Applied);
  const auto exploring = simulation.snapshot();
  ASSERT_EQ(exploring.phase, Phase::Exploration);
  ASSERT_EQ(exploring.chef.location, Location::City);
  ASSERT_EQ(exploring.appraiser.location, Location::City);
  ASSERT_EQ(exploring.chef.inventory, (std::vector<IngredientId>{{17}}));
  ASSERT_EQ(exploring.appraiser.inventory, (std::vector<IngredientId>{{23}}));

  ASSERT_EQ(simulation.advance(9s), Result::Applied);
  EXPECT_EQ(simulation.snapshot().remaining_exploration, 1s);
  EXPECT_EQ(simulation.snapshot().phase, Phase::Exploration);
  ASSERT_EQ(simulation.advance(5s), Result::Applied);
  auto expected = exploring;
  expected.chef.location = Location::Base;
  expected.appraiser.location = Location::Base;
  expected.phase = Phase::Workshop;
  expected.remaining_exploration = 0us;
  expected.world_time = 10s;
  EXPECT_EQ(simulation.snapshot(), expected);
  EXPECT_EQ(simulation.enter(Actor::Chef), Result::NoExplorationTime);
  EXPECT_EQ(simulation.enter(Actor::Appraiser), Result::NoExplorationTime);
  EXPECT_EQ(simulation.snapshot(), expected);
}

TEST(Survival, WorkshopTimeDoesNotConsumeTheRemainingExplorationBudget) {
  Simulation simulation({.exploration_budget = 10s, .seed = 3});
  const auto initial = simulation.snapshot();
  ASSERT_EQ(simulation.advance(24h), Result::Applied);
  EXPECT_EQ(simulation.snapshot(), initial);
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.advance(2s), Result::Applied);
  ASSERT_EQ(simulation.return_to_base(Actor::Chef), Result::Applied);
  const auto returned = simulation.snapshot();
  ASSERT_EQ(returned.phase, Phase::Workshop);
  ASSERT_EQ(returned.remaining_exploration, 8s);
  ASSERT_EQ(simulation.advance(24h), Result::Applied);
  EXPECT_EQ(simulation.snapshot(), returned);
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.advance(1s), Result::Applied);
  EXPECT_EQ(simulation.snapshot().remaining_exploration, 7s);
}

TEST(Survival, BaseCommandsAreRefusedWithoutAffectingExploringCompanion) {
  Simulation simulation({.exploration_budget = 10s, .seed = 6});
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  const auto before = simulation.snapshot();
  ASSERT_EQ(before.appraiser.location, Location::Base);
  EXPECT_EQ(simulation.collect(Actor::Appraiser, IngredientId{9}),
            Result::NotExploring);
  EXPECT_EQ(simulation.return_to_base(Actor::Appraiser), Result::NotExploring);
  EXPECT_EQ(simulation.snapshot(), before);
  ASSERT_EQ(simulation.enter(Actor::Appraiser), Result::Applied);
  EXPECT_EQ(simulation.collect(Actor::Appraiser, IngredientId{9}),
            Result::Applied);
  EXPECT_EQ(simulation.return_to_base(Actor::Appraiser), Result::Applied);
  EXPECT_EQ(simulation.snapshot().appraiser.inventory,
            (std::vector<IngredientId>{{9}}));
  EXPECT_EQ(simulation.snapshot().phase, Phase::Exploration);
  EXPECT_EQ(simulation.snapshot().chef, before.chef);
}

TEST(Survival, RejectsNegativeElapsedTimeWithoutRewindingTheRun) {
  Simulation simulation({.exploration_budget = 10s, .seed = 7});
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  const auto before = simulation.snapshot();
  EXPECT_EQ(simulation.advance(-1us), Result::InvalidElapsed);
  EXPECT_EQ(simulation.snapshot(), before);
  ASSERT_EQ(simulation.advance(1us), Result::Applied);
  EXPECT_EQ(simulation.snapshot().remaining_exploration, 10s - 1us);
}

TEST(Survival, FramePartitionsAndSeededCommandReplayProduceTheSameSnapshot) {
  Simulation whole({.exploration_budget = 1s, .seed = 99});
  Simulation partitioned({.exploration_budget = 1s, .seed = 99});
  for (auto *simulation : {&whole, &partitioned}) {
    ASSERT_EQ(simulation->enter(Actor::Chef), Result::Applied);
    ASSERT_EQ(simulation->collect(Actor::Chef, IngredientId{18}),
              Result::Applied);
  }
  ASSERT_EQ(whole.advance(100ms), Result::Applied);
  for (const auto elapsed : {1us, 8332us, 16667us, 75000us}) {
    ASSERT_EQ(partitioned.advance(elapsed), Result::Applied);
  }
  EXPECT_EQ(whole.snapshot(), partitioned.snapshot());
  EXPECT_EQ(whole.snapshot().remaining_exploration, 900ms);
  auto detached = whole.snapshot();
  detached.chef.inventory.clear();
  EXPECT_EQ(whole.snapshot().chef.inventory, (std::vector<IngredientId>{{18}}));
  ASSERT_EQ(whole.advance(900ms), Result::Applied);
  ASSERT_EQ(partitioned.advance(2s), Result::Applied);
  EXPECT_EQ(whole.snapshot(), partitioned.snapshot());
  EXPECT_EQ(whole.snapshot().phase, Phase::Workshop);
}

TEST(Survival, ConsumeTakesIngredientsFromBothInventoriesChefFirst) {
  Simulation simulation({.exploration_budget = 10s, .seed = 8});
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.enter(Actor::Appraiser), Result::Applied);
  ASSERT_EQ(simulation.collect(Actor::Chef, IngredientId{1}), Result::Applied);
  ASSERT_EQ(simulation.collect(Actor::Chef, IngredientId{2}), Result::Applied);
  ASSERT_EQ(simulation.collect(Actor::Appraiser, IngredientId{1}),
            Result::Applied);
  ASSERT_EQ(simulation.collect(Actor::Appraiser, IngredientId{3}),
            Result::Applied);
  ASSERT_EQ(simulation.return_to_base(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.return_to_base(Actor::Appraiser), Result::Applied);
  const std::vector<IngredientId> request{{1}, {3}};
  ASSERT_EQ(simulation.consume(request), Result::Applied);
  EXPECT_EQ(simulation.snapshot().chef.inventory,
            (std::vector<IngredientId>{{2}}));
  EXPECT_EQ(simulation.snapshot().appraiser.inventory,
            (std::vector<IngredientId>{{1}}));
}

TEST(Survival, ConsumeRefusesAPartiallyCarriedRequestWithoutTakingAnything) {
  Simulation simulation({.exploration_budget = 10s, .seed = 8});
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.collect(Actor::Chef, IngredientId{1}), Result::Applied);
  ASSERT_EQ(simulation.collect(Actor::Chef, IngredientId{2}), Result::Applied);
  ASSERT_EQ(simulation.return_to_base(Actor::Chef), Result::Applied);
  const auto before = simulation.snapshot();
  const std::vector<IngredientId> repeated{{1}, {1}};
  const std::vector<IngredientId> absent{{2}, {3}};
  EXPECT_EQ(simulation.consume(repeated), Result::MissingIngredients);
  EXPECT_EQ(simulation.consume(absent), Result::MissingIngredients);
  EXPECT_EQ(simulation.snapshot(), before);
  const std::vector<IngredientId> available{{1}, {2}};
  EXPECT_EQ(simulation.consume(available), Result::Applied);
  EXPECT_TRUE(simulation.snapshot().chef.inventory.empty());
}

TEST(Survival, FinishDayRaisesAcidAndRestoresTheBudgetKeepingInventory) {
  Simulation simulation({.exploration_budget = 10s, .seed = 9});
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.collect(Actor::Chef, IngredientId{4}), Result::Applied);
  ASSERT_EQ(simulation.advance(10s), Result::Applied);
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::NoExplorationTime);
  auto expected = simulation.snapshot();
  ASSERT_EQ(expected.remaining_exploration, 0us);
  ASSERT_EQ(simulation.finish_day(AcidLevel{5}), Result::Applied);
  expected.day = 2;
  expected.acid = AcidLevel{5};
  expected.remaining_exploration = 10s;
  EXPECT_EQ(simulation.snapshot(), expected);
  ASSERT_EQ(simulation.finish_day({}), Result::Applied);
  EXPECT_EQ(simulation.snapshot().day, 3U);
  EXPECT_EQ(simulation.snapshot().acid, AcidLevel{5});
  EXPECT_EQ(simulation.enter(Actor::Chef), Result::Applied);
}

TEST(Survival, AcidSaturatesInsteadOfWrappingBackToSafety) {
  constexpr auto limit = std::numeric_limits<std::uint32_t>::max();
  Simulation simulation({.exploration_budget = 10s, .seed = 9});
  ASSERT_EQ(simulation.finish_day(AcidLevel{limit - 1}), Result::Applied);
  ASSERT_EQ(simulation.snapshot().acid, AcidLevel{limit - 1});
  ASSERT_EQ(simulation.finish_day(AcidLevel{limit}), Result::Applied);
  EXPECT_EQ(simulation.snapshot().acid, AcidLevel{limit});
}

TEST(Survival, WorkshopResultsAreRefusedWhileAnActorIsExploring) {
  Simulation simulation({.exploration_budget = 10s, .seed = 10});
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.collect(Actor::Chef, IngredientId{1}), Result::Applied);
  const auto before = simulation.snapshot();
  const std::vector<IngredientId> request{{1}};
  EXPECT_EQ(simulation.consume(request), Result::NotInWorkshop);
  EXPECT_EQ(simulation.finish_day(AcidLevel{1}), Result::NotInWorkshop);
  EXPECT_EQ(simulation.snapshot(), before);
  ASSERT_EQ(simulation.return_to_base(Actor::Chef), Result::Applied);
  EXPECT_EQ(simulation.consume(request), Result::Applied);
  EXPECT_EQ(simulation.finish_day(AcidLevel{1}), Result::Applied);
}

TEST(Survival, EndingIsRefusedUntilBothActorsAreBackInTheWorkshop) {
  Simulation simulation({.exploration_budget = 10s, .seed = 3});
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  const auto exploring = simulation.snapshot();
  EXPECT_EQ(simulation.end(Ending::MonsterSatisfied), Result::NotInWorkshop);
  EXPECT_EQ(simulation.snapshot(), exploring);
  ASSERT_EQ(simulation.return_to_base(Actor::Chef), Result::Applied);
  EXPECT_EQ(simulation.end(Ending::MonsterSatisfied), Result::Applied);
  EXPECT_EQ(simulation.snapshot().phase, Phase::Ended);
}

TEST(Survival, NoCommandChangesTheSnapshotAfterTheEnding) {
  const std::array ingredients{RICE};
  Simulation simulation({.exploration_budget = 10s, .seed = 4});
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.collect(Actor::Chef, RICE), Result::Applied);
  ASSERT_EQ(simulation.return_to_base(Actor::Chef), Result::Applied);
  // The control: each refused command applies here, one phase earlier.
  Simulation control = simulation;
  EXPECT_EQ(control.consume(ingredients), Result::Applied);
  EXPECT_EQ(control.finish_day(AcidLevel{1}), Result::Applied);
  EXPECT_EQ(control.enter(Actor::Chef), Result::Applied);

  ASSERT_EQ(simulation.end(Ending::MonsterSatisfied), Result::Applied);
  const auto ended = simulation.snapshot();
  ASSERT_EQ(ended.phase, Phase::Ended);
  ASSERT_EQ(ended.ending, Ending::MonsterSatisfied);
  EXPECT_EQ(simulation.enter(Actor::Chef), Result::RunOver);
  EXPECT_EQ(simulation.enter(Actor::Appraiser), Result::RunOver);
  EXPECT_EQ(simulation.return_to_base(Actor::Chef), Result::NotExploring);
  EXPECT_EQ(simulation.advance(1s), Result::Applied);
  EXPECT_EQ(simulation.collect(Actor::Chef, RICE), Result::NotExploring);
  EXPECT_EQ(simulation.consume(ingredients), Result::NotInWorkshop);
  EXPECT_EQ(simulation.finish_day(AcidLevel{1}), Result::NotInWorkshop);
  EXPECT_EQ(simulation.end(Ending::MonsterSatisfied), Result::NotInWorkshop);
  EXPECT_EQ(simulation.snapshot(), ended);
}

TEST(Simulation, FullAcidMustBePositive) {
  EXPECT_THROW(
      (Simulation(
          {.exploration_budget = 1s, .seed = 1, .full_acid = AcidLevel{0}})),
      std::invalid_argument);
  const Simulation accepted(
      {.exploration_budget = 1s, .seed = 1, .full_acid = AcidLevel{1}});
  EXPECT_EQ(accepted.snapshot().phase, Phase::Workshop);
}

TEST(Simulation, ReachingFullAcidLosesTheRunAndOneBelowDoesNot) {
  Simulation simulation(
      {.exploration_budget = 10s, .seed = 1, .full_acid = AcidLevel{10}});
  ASSERT_EQ(simulation.finish_day(AcidLevel{9}), Result::Applied);
  EXPECT_EQ(simulation.snapshot().phase, Phase::Workshop);
  ASSERT_EQ(simulation.finish_day(AcidLevel{1}), Result::Applied);
  EXPECT_EQ(simulation.snapshot().phase, Phase::Lost);
  EXPECT_EQ(simulation.snapshot().day, 3U);
}

TEST(Simulation, AFinishedDayAddsTheSameTimeWhateverWasSpent) {
  Simulation simulation({.exploration_budget = 10s, .seed = 1});
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.advance(3s), Result::Applied);
  EXPECT_EQ(simulation.snapshot().world_time, 3s);
  ASSERT_EQ(simulation.return_to_base(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.finish_day(AcidLevel{}), Result::Applied);
  EXPECT_EQ(simulation.snapshot().world_time, 10s);
  ASSERT_EQ(simulation.finish_day(AcidLevel{}), Result::Applied);
  EXPECT_EQ(simulation.snapshot().world_time, 20s);
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.advance(15s), Result::Applied);
  ASSERT_EQ(simulation.finish_day(AcidLevel{}), Result::Applied);
  EXPECT_EQ(simulation.snapshot().world_time, 30s);
}

TEST(Simulation, AcidStopsAtFull) {
  Simulation simulation(
      {.exploration_budget = 10s, .seed = 1, .full_acid = AcidLevel{10}});
  ASSERT_EQ(simulation.finish_day(AcidLevel{25}), Result::Applied);
  EXPECT_EQ(simulation.snapshot().acid, AcidLevel{10});
  EXPECT_EQ(simulation.snapshot().phase, Phase::Lost);
}

TEST(Simulation, AFinishedDayLowersAcidByTheDropAfterTheRise) {
  Simulation simulation({.exploration_budget = 10s, .seed = 1});
  ASSERT_EQ(simulation.finish_day(AcidLevel{10}), Result::Applied);
  ASSERT_EQ(simulation.finish_day(AcidLevel{3}, AcidLevel{5}), Result::Applied);
  EXPECT_EQ(simulation.snapshot().acid, AcidLevel{8});
  ASSERT_EQ(simulation.finish_day({}, AcidLevel{100}), Result::Applied);
  EXPECT_EQ(simulation.snapshot().acid, AcidLevel{});
}

TEST(Simulation, NoCommandChangesTheSnapshotAfterALoss) {
  const std::vector<IngredientId> ingredients{RICE};
  Simulation simulation(
      {.exploration_budget = 10s, .seed = 1, .full_acid = AcidLevel{5}});
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.collect(Actor::Chef, RICE), Result::Applied);
  ASSERT_EQ(simulation.return_to_base(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.finish_day(AcidLevel{5}), Result::Applied);
  const auto lost = simulation.snapshot();
  ASSERT_EQ(lost.phase, Phase::Lost);
  EXPECT_EQ(simulation.enter(Actor::Chef), Result::RunOver);
  EXPECT_EQ(simulation.enter(Actor::Appraiser), Result::RunOver);
  EXPECT_EQ(simulation.return_to_base(Actor::Chef), Result::NotExploring);
  EXPECT_EQ(simulation.advance(1s), Result::Applied);
  EXPECT_EQ(simulation.collect(Actor::Chef, RICE), Result::NotExploring);
  EXPECT_EQ(simulation.consume(ingredients), Result::NotInWorkshop);
  EXPECT_EQ(simulation.finish_day(AcidLevel{1}), Result::NotInWorkshop);
  EXPECT_EQ(simulation.end(Ending::MonsterSatisfied), Result::NotInWorkshop);
  EXPECT_EQ(simulation.snapshot(), lost);
}

TEST(Simulation, LosingKeepsWhatTheActorsCarry) {
  Simulation simulation(
      {.exploration_budget = 10s, .seed = 1, .full_acid = AcidLevel{5}});
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.collect(Actor::Chef, RICE), Result::Applied);
  ASSERT_EQ(simulation.return_to_base(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.finish_day(AcidLevel{5}), Result::Applied);
  EXPECT_EQ(simulation.snapshot().chef.inventory,
            (std::vector<IngredientId>{RICE}));
}

/// A run with the chef in the city carrying `items`, in a two-slot inventory.
Simulation chef_in_city(std::uint32_t inventory_slots,
                        std::uint32_t fridge_slots) {
  Simulation simulation({.exploration_budget = 10s,
                         .seed = 1,
                         .inventory_slots = inventory_slots,
                         .fridge_slots = fridge_slots});
  EXPECT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  return simulation;
}

TEST(Survival, AnInventoryHoldsOnlyItsConfiguredSlots) {
  auto simulation = chef_in_city(2, 5);
  EXPECT_EQ(simulation.collect(Actor::Chef, RICE), Result::Applied);
  EXPECT_EQ(simulation.collect(Actor::Chef, RICE), Result::Applied);
  EXPECT_EQ(simulation.collect(Actor::Chef, RICE), Result::Full);
  EXPECT_EQ(simulation.snapshot().chef.inventory.size(), 2U);
  EXPECT_EQ(simulation.snapshot().inventory_slots, 2U);
  EXPECT_EQ(simulation.snapshot().fridge_slots, 5U);
}

TEST(Survival, SlotCountsOfZeroAreRefused) {
  EXPECT_THROW(
      (Simulation({.exploration_budget = 1s, .seed = 1, .inventory_slots = 0})),
      std::invalid_argument);
  EXPECT_THROW(
      (Simulation({.exploration_budget = 1s, .seed = 1, .fridge_slots = 0})),
      std::invalid_argument);
}

TEST(Survival, TheFridgeStoresAtBaseOnlyAndWithdrawsWhatItHolds) {
  auto simulation = chef_in_city(4, 2);
  ASSERT_EQ(simulation.collect(Actor::Chef, RICE), Result::Applied);
  EXPECT_EQ(simulation.store(Actor::Chef, RICE), Result::NotInWorkshop);
  ASSERT_EQ(simulation.return_to_base(Actor::Chef), Result::Applied);
  EXPECT_EQ(simulation.store(Actor::Chef, IngredientId{9}),
            Result::MissingIngredients);
  ASSERT_EQ(simulation.store(Actor::Chef, RICE), Result::Applied);
  EXPECT_TRUE(simulation.snapshot().chef.inventory.empty());
  EXPECT_EQ(simulation.snapshot().fridge, (std::vector{RICE}));
  EXPECT_EQ(simulation.withdraw(Actor::Appraiser, IngredientId{9}),
            Result::MissingIngredients);
  ASSERT_EQ(simulation.withdraw(Actor::Appraiser, RICE), Result::Applied);
  EXPECT_TRUE(simulation.snapshot().fridge.empty());
  EXPECT_EQ(simulation.snapshot().appraiser.inventory, (std::vector{RICE}));
}

TEST(Survival, AFullFridgeRefusesAndAFullInventoryRefusesAWithdrawal) {
  auto simulation = chef_in_city(1, 1);
  ASSERT_EQ(simulation.collect(Actor::Chef, RICE), Result::Applied);
  ASSERT_EQ(simulation.enter(Actor::Appraiser), Result::Applied);
  ASSERT_EQ(simulation.collect(Actor::Appraiser, RICE), Result::Applied);
  ASSERT_EQ(simulation.return_to_base(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.return_to_base(Actor::Appraiser), Result::Applied);
  ASSERT_EQ(simulation.store(Actor::Chef, RICE), Result::Applied);
  EXPECT_EQ(simulation.store(Actor::Appraiser, RICE), Result::Full);
  EXPECT_EQ(simulation.snapshot().appraiser.inventory, (std::vector{RICE}));
  // The appraiser's single slot is taken, so it cannot withdraw.
  EXPECT_EQ(simulation.withdraw(Actor::Appraiser, RICE), Result::Full);
  EXPECT_EQ(simulation.snapshot().fridge, (std::vector{RICE}));
}

TEST(Survival, TheFridgeKeepsItsContentsToTheNextDayWithoutSpoiling) {
  auto simulation = chef_in_city(4, 4);
  ASSERT_EQ(simulation.collect(Actor::Chef, RICE), Result::Applied);
  ASSERT_EQ(simulation.return_to_base(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.store(Actor::Chef, RICE), Result::Applied);
  for (int day = 0; day < 3; ++day) {
    ASSERT_EQ(simulation.finish_day({}), Result::Applied);
  }
  EXPECT_EQ(simulation.snapshot().day, 4U);
  EXPECT_EQ(simulation.snapshot().fridge, (std::vector{RICE}));
}

TEST(Survival, CookingSpendsTheChefThenTheAppraiserThenTheFridge) {
  auto simulation = chef_in_city(4, 4);
  ASSERT_EQ(simulation.collect(Actor::Chef, RICE), Result::Applied);
  ASSERT_EQ(simulation.return_to_base(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.store(Actor::Chef, RICE), Result::Applied);
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.collect(Actor::Chef, RICE), Result::Applied);
  ASSERT_EQ(simulation.return_to_base(Actor::Chef), Result::Applied);
  const std::array both{RICE, RICE};
  ASSERT_EQ(simulation.consume(both), Result::Applied);
  EXPECT_TRUE(simulation.snapshot().chef.inventory.empty());
  EXPECT_TRUE(simulation.snapshot().fridge.empty());
  const std::array three{RICE, RICE, RICE};
  EXPECT_EQ(simulation.consume(three), Result::MissingIngredients);
}

} // namespace
} // namespace neocity::survival
