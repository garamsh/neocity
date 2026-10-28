#include <pan_dodge/engine.hpp>

#include <gtest/gtest.h>

#include <cmath>

namespace neocity::pan_dodge {
namespace {

void move(Engine &engine, Command command, int ticks) {
  for (int i = 0; i < ticks; ++i) {
    engine.step(command);
  }
}

void escape(Engine &engine) {
  move(engine, {.vertical = Direction::Negative}, 174);
  move(engine, {.horizontal = Direction::Positive}, 102);
  move(engine, {.vertical = Direction::Negative}, 60);
}

TEST(PanDodge, DirectInputMovesAndReleaseStopsWithoutMovingHazards) {
  Engine engine;
  const auto initial = engine.snapshot();
  move(engine, {.vertical = Direction::Negative}, 30);
  const auto moved = engine.snapshot();
  EXPECT_FLOAT_EQ(moved.player.center.x, initial.player.center.x);
  EXPECT_NEAR(moved.player.center.y, initial.player.center.y - 50.0F, 0.001F);
  move(engine, {}, 30);
  EXPECT_FLOAT_EQ(engine.snapshot().player.center.y, moved.player.center.y);
  for (std::size_t i = 0; i < initial.hazards.size(); ++i) {
    EXPECT_FLOAT_EQ(engine.snapshot().hazards.at(i).center.x,
                    initial.hazards.at(i).center.x);
    EXPECT_FLOAT_EQ(engine.snapshot().hazards.at(i).center.y,
                    initial.hazards.at(i).center.y);
  }
}

TEST(PanDodge, DiagonalInputHasTheSameSpeedAsAnAxis) {
  Engine engine;
  const auto initial = engine.snapshot().player.center;
  move(engine,
       {.horizontal = Direction::Positive, .vertical = Direction::Negative},
       10);
  const auto moved = engine.snapshot().player.center;
  EXPECT_NEAR(std::hypot(moved.x - initial.x, moved.y - initial.y),
              200.0F / 12.0F, 0.001F);
}

TEST(PanDodge, TheRimBlocksTravelExceptAtTheMarkedExit) {
  Engine blocked;
  move(blocked, {.horizontal = Direction::Negative}, 200);
  const auto world = blocked.snapshot();
  ASSERT_EQ(world.outcome, Outcome::Playing);
  EXPECT_LE(std::hypot(world.player.center.x, world.player.center.y),
            world.pan_radius - world.player.radius + 0.001F);
  Engine open;
  escape(open);
  EXPECT_EQ(open.snapshot().outcome, Outcome::Escaped);
  EXPECT_LT(open.snapshot().player.center.y, -open.snapshot().pan_radius);
}

TEST(PanDodge, ContactLosesWhileTheSafeRouteEscapesAndBothFreeze) {
  Engine hit;
  move(hit, {.horizontal = Direction::Positive}, 100);
  ASSERT_EQ(hit.snapshot().outcome, Outcome::Hit);
  const auto stopped = hit.snapshot().player.center;
  move(hit, {.vertical = Direction::Negative}, 60);
  EXPECT_FLOAT_EQ(hit.snapshot().player.center.y, stopped.y);
  Engine escaped;
  escape(escaped);
  ASSERT_EQ(escaped.snapshot().outcome, Outcome::Escaped);
  const auto exit = escaped.snapshot().player.center;
  move(escaped, {.vertical = Direction::Positive}, 100);
  EXPECT_FLOAT_EQ(escaped.snapshot().player.center.y, exit.y);
}

TEST(PanDodge, ResetRestoresBothTerminalOutcomesToAFreshPlayableRun) {
  Engine engine;
  const auto initial = engine.snapshot();
  move(engine, {.horizontal = Direction::Positive}, 100);
  ASSERT_EQ(engine.snapshot().outcome, Outcome::Hit);
  engine.reset();
  EXPECT_EQ(engine.snapshot().outcome, Outcome::Playing);
  EXPECT_FLOAT_EQ(engine.snapshot().player.center.x, initial.player.center.x);
  escape(engine);
  ASSERT_EQ(engine.snapshot().outcome, Outcome::Escaped);
  engine.reset();
  EXPECT_EQ(engine.snapshot().outcome, Outcome::Playing);
  EXPECT_FLOAT_EQ(engine.snapshot().player.center.y, initial.player.center.y);
  engine.step({.vertical = Direction::Negative});
  EXPECT_LT(engine.snapshot().player.center.y, initial.player.center.y);
}

} // namespace
} // namespace neocity::pan_dodge
