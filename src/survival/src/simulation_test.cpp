#include <survival/simulation.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>

namespace neocity::survival {
namespace {

using namespace std::chrono_literals;

TEST(Survival, StartsWithTwoHealthyActorsAtAnUntimedBase) {
  const Simulation simulation({.exploration_budget = 12s, .seed = 42});
  const auto state = simulation.snapshot();
  EXPECT_EQ(state.chef.location, Location::Base);
  EXPECT_EQ(state.appraiser.location, Location::Base);
  EXPECT_EQ(state.chef.health, 3);
  EXPECT_EQ(state.appraiser.health, 3);
  EXPECT_TRUE(state.chef.inventory.empty());
  EXPECT_TRUE(state.appraiser.inventory.empty());
  EXPECT_EQ(state.phase, Phase::Workshop);
  EXPECT_EQ(state.day, 1U);
  EXPECT_EQ(state.remaining_exploration, 12s);
  EXPECT_EQ(state.acid, 0U);
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
  ASSERT_EQ(simulation.damage(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.damage(Actor::Appraiser), Result::Applied);
  ASSERT_EQ(simulation.damage(Actor::Appraiser), Result::Applied);
  const auto exploring = simulation.snapshot();
  ASSERT_EQ(exploring.phase, Phase::Exploration);
  ASSERT_EQ(exploring.chef.location, Location::City);
  ASSERT_EQ(exploring.appraiser.location, Location::City);
  ASSERT_EQ(exploring.chef.health, 2);
  ASSERT_EQ(exploring.appraiser.health, 1);
  ASSERT_EQ(exploring.chef.inventory, (std::vector<IngredientId>{{17}}));
  ASSERT_EQ(exploring.appraiser.inventory, (std::vector<IngredientId>{{23}}));

  ASSERT_EQ(simulation.advance(9s), Result::Applied);
  EXPECT_EQ(simulation.snapshot().remaining_exploration, 1s);
  EXPECT_EQ(simulation.snapshot().phase, Phase::Exploration);
  ASSERT_EQ(simulation.advance(5s), Result::Applied);
  auto expected = exploring;
  expected.chef.location = Location::Base;
  expected.appraiser.location = Location::Base;
  expected.chef.health = 3;
  expected.appraiser.health = 3;
  expected.phase = Phase::Workshop;
  expected.remaining_exploration = 0us;
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

TEST(Survival, ExhaustionRelocatesOnlyThatActorWithoutResettingTheRun) {
  Simulation simulation({.exploration_budget = 10s, .seed = 4});
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.enter(Actor::Appraiser), Result::Applied);
  ASSERT_EQ(simulation.collect(Actor::Chef, IngredientId{12}), Result::Applied);
  ASSERT_EQ(simulation.collect(Actor::Appraiser, IngredientId{34}),
            Result::Applied);
  ASSERT_EQ(simulation.advance(3s), Result::Applied);
  ASSERT_EQ(simulation.damage(Actor::Chef), Result::Applied);
  EXPECT_EQ(simulation.snapshot().chef.health, 2);
  EXPECT_EQ(simulation.snapshot().chef.location, Location::City);
  ASSERT_EQ(simulation.damage(Actor::Chef), Result::Applied);
  const auto before = simulation.snapshot();
  ASSERT_EQ(before.chef.health, 1);
  ASSERT_EQ(before.remaining_exploration, 7s);
  ASSERT_EQ(before.chef.inventory, (std::vector<IngredientId>{{12}}));
  ASSERT_EQ(before.appraiser.inventory, (std::vector<IngredientId>{{34}}));
  ASSERT_EQ(simulation.damage(Actor::Chef), Result::Applied);
  auto expected = before;
  expected.chef.health = 3;
  expected.chef.location = Location::Base;
  EXPECT_EQ(simulation.snapshot(), expected);
  ASSERT_EQ(simulation.advance(1s), Result::Applied);
  EXPECT_EQ(simulation.snapshot().remaining_exploration, 6s);
  EXPECT_EQ(simulation.snapshot().appraiser.location, Location::City);
}

TEST(Survival, ExhaustedActorReentersWithRecoveredHealthAndPreservedProgress) {
  Simulation simulation({.exploration_budget = 10s, .seed = 5});
  ASSERT_EQ(simulation.enter(Actor::Appraiser), Result::Applied);
  ASSERT_EQ(simulation.collect(Actor::Appraiser, IngredientId{6}),
            Result::Applied);
  ASSERT_EQ(simulation.advance(2s), Result::Applied);
  for (int hit = 0; hit < 3; ++hit) {
    ASSERT_EQ(simulation.damage(Actor::Appraiser), Result::Applied);
  }
  const auto exhausted = simulation.snapshot();
  ASSERT_EQ(exhausted.appraiser.health, 3);
  ASSERT_EQ(exhausted.appraiser.location, Location::Base);
  ASSERT_EQ(exhausted.phase, Phase::Workshop);
  ASSERT_EQ(exhausted.remaining_exploration, 8s);
  ASSERT_EQ(exhausted.appraiser.inventory, (std::vector<IngredientId>{{6}}));
  ASSERT_EQ(simulation.advance(24h), Result::Applied);
  EXPECT_EQ(simulation.snapshot(), exhausted);
  ASSERT_EQ(simulation.enter(Actor::Appraiser), Result::Applied);
  auto reentered = exhausted;
  reentered.appraiser.location = Location::City;
  reentered.phase = Phase::Exploration;
  EXPECT_EQ(simulation.snapshot(), reentered);
  ASSERT_EQ(simulation.damage(Actor::Appraiser), Result::Applied);
  EXPECT_EQ(simulation.snapshot().appraiser.health, 2);
  EXPECT_EQ(simulation.snapshot().appraiser.location, Location::City);
  ASSERT_EQ(simulation.return_to_base(Actor::Appraiser), Result::Applied);
  EXPECT_EQ(simulation.snapshot(), exhausted);
}

TEST(Survival, BaseCommandsAreRefusedWithoutAffectingExploringCompanion) {
  Simulation simulation({.exploration_budget = 10s, .seed = 6});
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  const auto before = simulation.snapshot();
  ASSERT_EQ(before.appraiser.location, Location::Base);
  EXPECT_EQ(simulation.collect(Actor::Appraiser, IngredientId{9}),
            Result::NotExploring);
  EXPECT_EQ(simulation.damage(Actor::Appraiser), Result::NotExploring);
  EXPECT_EQ(simulation.return_to_base(Actor::Appraiser), Result::NotExploring);
  EXPECT_EQ(simulation.snapshot(), before);
  ASSERT_EQ(simulation.enter(Actor::Appraiser), Result::Applied);
  EXPECT_EQ(simulation.collect(Actor::Appraiser, IngredientId{9}),
            Result::Applied);
  EXPECT_EQ(simulation.damage(Actor::Appraiser), Result::Applied);
  EXPECT_EQ(simulation.return_to_base(Actor::Appraiser), Result::Applied);
  EXPECT_EQ(simulation.snapshot().appraiser.inventory,
            (std::vector<IngredientId>{{9}}));
  EXPECT_EQ(simulation.snapshot().appraiser.health, 3);
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
    ASSERT_EQ(simulation->damage(Actor::Chef), Result::Applied);
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

} // namespace
} // namespace neocity::survival
