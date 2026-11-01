#include "bot.hpp"
#include "room.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace neocity::lab {
namespace {

using survival::Actor;

constexpr std::uint32_t SEED = 7;
constexpr int STEPS = 1500;

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

TEST(Room, ReplayMissingOneStepDiverges) {
  Room played(SEED, 1);
  const auto log = play(played, STEPS);

  Room replayed(SEED, 1);
  for (std::size_t step = 1; step < log.size(); ++step) {
    replayed.step(log.at(step));
  }

  EXPECT_FALSE(same_view(played.view(), replayed.view()));
}

TEST(Room, StartDayPlaysTheSameAiRunOnEveryProcess) {
  const Room first(SEED, 3);
  const Room second(SEED, 3);
  ASSERT_EQ(first.view().progression.day, 3U);
  EXPECT_TRUE(same_view(first.view(), second.view()));
}

} // namespace
} // namespace neocity::lab
