#include <ascent/engine.hpp>

#include "course.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>

namespace neocity::ascent {

/// Reads and arranges the engine's run state for the tests below.
struct EngineProbe {
  static Point &feet(AscentEngine &game) { return game.feet; }
  static bool &checkpoint(AscentEngine &game) { return game.checkpoint; }
  static float &highest(AscentEngine &game) { return game.highest; }
  static float &invulnerability(AscentEngine &game) {
    return game.invulnerability;
  }
  static std::optional<std::size_t> &grounded(AscentEngine &game) {
    return game.grounded;
  }
  static float time(const AscentEngine &game) { return game.time; }
  static RunPhase phase(const AscentEngine &game) { return game.phase; }
};

namespace {

PlayerCommand hold(float x) {
  return PlayerCommand{.horizontal = x, .jump_held = true};
}

PlayerCommand pressing_jump(float x) {
  PlayerCommand command = hold(x);
  command.jump_pressed = true;
  return command;
}

PlayerCommand pressing_pause(float x) {
  PlayerCommand command = hold(x);
  command.pause_pressed = true;
  return command;
}

TEST(AscentEngine, JumpRisesAndLandsOnTheActualStartSurface) {
  AscentEngine game;
  game.step(pressing_jump(0.0F));
  for (int i = 0; i < 25; ++i) {
    game.step(hold(0.0F));
  }
  EXPECT_LT(game.snapshot().leader.feet.y, START.y - 100.0F);
  for (int i = 0; i < 110; ++i) {
    game.step(hold(0.0F));
  }
  EXPECT_EQ(game.snapshot().leader.feet.y, START.y);
  EXPECT_FALSE(game.snapshot().leader.airborne);
}

TEST(AscentEngine, PauseFreezesRulesButCanBeResumed) {
  AscentEngine game;
  game.step(pressing_pause(1.0F));
  const WorldSnapshot before = game.snapshot();
  for (int i = 0; i < 100; ++i) {
    game.step(hold(1.0F));
  }
  EXPECT_EQ(game.snapshot().leader.feet, before.leader.feet);
  EXPECT_EQ(game.snapshot().elapsed, before.elapsed);
  game.step(pressing_pause(1.0F));
  EXPECT_GT(game.snapshot().elapsed, before.elapsed);
}

TEST(AscentEngine, HoldingJumpDoesNotAutomaticallyJumpAgainAfterLanding) {
  AscentEngine game;
  game.step(pressing_jump(0.0F));
  for (int i = 0; i < 200; ++i) {
    game.step(hold(0.0F));
  }
  EXPECT_FALSE(game.snapshot().leader.airborne);
}

TEST(AscentEngine, SwitchingIdentityPreservesTheSharedCollisionPosition) {
  AscentEngine game;
  const Point before = game.snapshot().leader.feet;
  game.step(PlayerCommand{.switch_character = true});
  EXPECT_EQ(game.snapshot().leader.character, Character::Detective);
  EXPECT_EQ(game.snapshot().leader.feet, before);
}

TEST(AscentEngine, RetryRetainsTheReachedCheckpoint) {
  AscentEngine game;
  EngineProbe::checkpoint(game) = true;
  EngineProbe::feet(game) = Point{.x = 900.0F, .y = 300.0F};
  game.step(PlayerCommand{.retry_pressed = true});
  EXPECT_EQ(game.snapshot().leader.feet, game.snapshot().checkpoint);
  EXPECT_EQ(game.snapshot().retries, 1U);
}

TEST(AscentEngine, NonFiniteInputCannotPoisonTheWorld) {
  AscentEngine game;
  game.step(hold(std::numeric_limits<float>::quiet_NaN()));
  EXPECT_TRUE(std::isfinite(game.snapshot().leader.feet.x));
}

TEST(AscentEngine, ReachingTheIngredientCompletesAndRestartResetsProgress) {
  AscentEngine game;
  EngineProbe::feet(game) = GOAL;
  game.step(PlayerCommand{});
  EXPECT_EQ(game.snapshot().phase, RunPhase::Complete);
  game.step(PlayerCommand{.retry_pressed = true});
  EXPECT_EQ(game.snapshot().phase, RunPhase::Playing);
  EXPECT_EQ(game.snapshot().progress, 0.0F);
}

TEST(AscentEngine, WalkingOffALedgeEventuallyRecoversInsteadOfSoftlocking) {
  AscentEngine game;
  for (int i = 0; i < 500; ++i) {
    game.step(hold(1.0F));
  }
  EXPECT_GT(game.snapshot().retries, 0U);
}

TEST(AscentEngine, EveryAdjacentCourseSurfaceIsReachableWithTheShippedJump) {
  for (std::size_t index = 0; index < 17; ++index) {
    AscentEngine game;
    const std::vector<Surface> platforms = surfaces(0.0F);
    const Surface source = platforms.at(index);
    const Surface target = platforms.at(index + 1);
    const float direction = target.x > source.x ? 1.0F : -1.0F;
    EngineProbe::feet(game) =
        Point{.x = direction > 0.0F ? source.x + source.width - 18.0F
                                    : source.x + 18.0F,
              .y = source.y};
    EngineProbe::grounded(game) = index;
    EngineProbe::invulnerability(game) = 999.0F;
    EngineProbe::highest(game) = source.y;
    game.step(pressing_jump(direction));
    bool reached = false;
    for (int i = 0; i < 115; ++i) {
      const Surface target_now =
          surfaces(EngineProbe::time(game)).at(index + 1);
      const float desired = target_now.x + target_now.width * 0.5F;
      const float movement = std::clamp(
          (desired - EngineProbe::feet(game).x) * 0.06F, -1.0F, 1.0F);
      game.step(hold(movement));
      if (EngineProbe::grounded(game) == index + 1 ||
          EngineProbe::phase(game) == RunPhase::Complete) {
        reached = true;
        break;
      }
    }
    ASSERT_TRUE(reached) << "unreachable course step " << index << " -> "
                         << index + 1;
  }
}

TEST(AscentEngine, RetryAfterAHighFallDoesNotLoopAtTheCheckpoint) {
  AscentEngine game;
  EngineProbe::checkpoint(game) = true;
  EngineProbe::highest(game) = GOAL.y;
  game.step(PlayerCommand{.retry_pressed = true});
  for (int i = 0; i < 20; ++i) {
    game.step(PlayerCommand{});
  }
  EXPECT_EQ(game.snapshot().retries, 1U);
  EXPECT_EQ(game.snapshot().leader.feet, game.snapshot().checkpoint);
}

TEST(AscentEngine, MonsterContactRecoversOnceAndProtectsTheRetry) {
  AscentEngine game;
  const std::vector<Surface> platforms = surfaces(0.0F);
  const Hazard pest = hazards(0.0F, platforms).at(0);
  EngineProbe::feet(game) = Point{.x = pest.center.x, .y = platforms.at(3).y};
  EngineProbe::grounded(game) = 3;
  EngineProbe::invulnerability(game) = 0.0F;
  game.step(PlayerCommand{});
  EXPECT_EQ(game.snapshot().retries, 1U);
  EXPECT_EQ(game.snapshot().leader.feet, START);
  EngineProbe::feet(game) = Point{.x = pest.center.x, .y = platforms.at(3).y};
  EngineProbe::grounded(game) = 3;
  game.step(PlayerCommand{});
  EXPECT_EQ(game.snapshot().retries, 1U);
}

} // namespace
} // namespace neocity::ascent
