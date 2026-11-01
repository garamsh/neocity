#include "bot.hpp"
#include "room.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace neocity::lab {
namespace {

using survival::Actor;

constexpr std::uint32_t SEED = 7;
constexpr int STEPS = 150;
constexpr int RUN_LIMIT = 100000;

/// Two bots playing keys from each step's view, as remote seats do.
std::vector<StepInput> play(Room &room, int steps) {
  const survival::Companion brain(room.companion_config());
  std::vector<StepInput> log;
  for (int step = 0; step < steps; ++step) {
    const auto view = room.view();
    const StepInput next{.inputs = {bot_input(brain, Actor::Chef, view),
                                    bot_input(brain, Actor::Appraiser, view)},
                         .control = survival::two_players()};
    room.step(next);
    log.push_back(next);
  }
  return log;
}

TEST(Room, KeyDrivenBotsCollectAndFinishDays) {
  Room room(SEED, 1);
  (void)play(room, STEPS);
  const auto view = room.view();
  EXPECT_GT(view.progression.day, 1U);
  EXPECT_FALSE(view.workshop.eaten_foods.empty());
}

TEST(Room, ReplayingTheLogReachesTheSameState) {
  Room played(SEED, 1);
  const auto log = play(played, STEPS);
  ASSERT_GT(played.view().workshop.rolls, 0U);

  Room replayed(SEED, 1);
  for (const auto &step : log) {
    replayed.step(step);
  }

  EXPECT_TRUE(same_view(played.view(), replayed.view()));
}

TEST(Room, ReplayMissingOneStepDivergesWhileTheWholeLogMatches) {
  Room played(SEED, 1);
  const auto log = play(played, STEPS);

  Room whole(SEED, 1);
  Room shortened(SEED, 1);
  for (std::size_t step = 0; step < log.size(); ++step) {
    whole.step(log.at(step));
    if (step > 0) {
      shortened.step(log.at(step));
    }
  }

  EXPECT_TRUE(same_view(played.view(), whole.view()));
  EXPECT_FALSE(same_view(played.view(), shortened.view()));
}

TEST(StepLog, ReplayRefusesAGapAndAcceptsTheSameStepsInOrder) {
  Room played(SEED, 1);
  const auto steps = play(played, STEPS);
  StepLog whole;
  StepLog gapped;
  for (std::size_t index = 0; index < steps.size(); ++index) {
    whole.record(index + 1, steps.at(index));
    // Tick 2 never arrives.
    if (index != 1) {
      gapped.record(index + 1, steps.at(index));
    }
  }

  const auto refused = replay(gapped, SEED);
  const auto accepted = replay(whole, SEED);

  EXPECT_EQ(refused, nullptr);
  ASSERT_NE(accepted, nullptr);
  EXPECT_TRUE(same_view(accepted->view(), played.view()));
}

TEST(Room, EscapeEndsTheRunAndLaterStepsChangeNothing) {
  // Day one's room cannot have escaped, so the phase below is the rule's.
  Room room(SEED, 1);
  ASSERT_NE(room.view().progression.phase, survival::Phase::Escaped);
  const StepInput ai_only{.inputs = {},
                          .control = {.chef = survival::Controller::Ai,
                                      .appraiser = survival::Controller::Ai}};
  for (int step = 0; step < RUN_LIMIT &&
                     room.view().progression.phase != survival::Phase::Escaped;
       ++step) {
    room.step(ai_only);
  }
  ASSERT_EQ(room.view().progression.phase, survival::Phase::Escaped);
  const auto ended = room.view();

  room.step(ai_only);

  EXPECT_TRUE(same_view(room.view(), ended));
  EXPECT_GT(room.escape().rolls, 0U);
}

TEST(Room, ActorProgressCountsMovementAndItemsAndNotADayAlone) {
  const auto before = Room(SEED, 1).view();
  auto rollover = before;
  ++rollover.progression.day;
  auto moved = before;
  ++moved.exploration.appraiser.position.x;
  auto collected = before;
  collected.progression.appraiser.inventory.push_back({});

  EXPECT_FALSE(actor_progressed(before, rollover, Actor::Appraiser));
  EXPECT_TRUE(actor_progressed(before, moved, Actor::Appraiser));
  EXPECT_FALSE(actor_progressed(before, moved, Actor::Chef));
  EXPECT_TRUE(actor_progressed(before, collected, Actor::Appraiser));
}

TEST(Room, StartDayPlaysTheSameAiRunOnEveryProcess) {
  const Room first(SEED, 3);
  const Room second(SEED, 3);
  ASSERT_EQ(first.view().progression.day, 3U);
  EXPECT_TRUE(same_view(first.view(), second.view()));
}

} // namespace
} // namespace neocity::lab
