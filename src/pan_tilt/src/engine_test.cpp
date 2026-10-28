#include <pan_tilt/engine.hpp>

#include <gtest/gtest.h>

#include <cmath>

namespace neocity::pan_tilt {
namespace {

void tilt(Engine &engine, Command command, int ticks) {
  for (int i = 0; i < ticks; ++i) {
    engine.step(command);
  }
}

Direction steer(float force) {
  if (force < -0.1F) {
    return Direction::Negative;
  }
  return force > 0.1F ? Direction::Positive : Direction::Neutral;
}

void escape(Engine &engine) {
  const std::array<Point, 3> route{{{.x = -170.0F, .y = -135.0F},
                                    {.x = 0.0F, .y = -170.0F},
                                    {.x = 0.0F, .y = -280.0F}}};
  std::size_t waypoint = 0;
  for (int tick = 0;
       tick < 1200 && engine.snapshot().outcome == Outcome::Playing; ++tick) {
    const auto player = engine.snapshot().player;
    auto target = route.at(waypoint);
    if (waypoint < route.size() - 1 &&
        std::hypot(target.x - player.center.x, target.y - player.center.y) <
            25.0F) {
      target = route.at(++waypoint);
    }
    engine.step({.horizontal = steer(2.0F * (target.x - player.center.x) -
                                     player.velocity.x),
                 .vertical = steer(2.0F * (target.y - player.center.y) -
                                   player.velocity.y)});
  }
}

TEST(PanTilt, OneTiltAcceleratesPlayerAndEveryHazardByTheSameAmount) {
  Engine engine;
  const auto initial = engine.snapshot();
  tilt(engine, {.horizontal = Direction::Positive}, 30);
  const auto moved = engine.snapshot();
  ASSERT_EQ(moved.outcome, Outcome::Playing);
  EXPECT_GT(moved.tilt.x, 0.5F);
  EXPECT_GT(moved.player.velocity.x, 10.0F);
  EXPECT_GT(moved.player.center.x, initial.player.center.x + 1.0F);
  EXPECT_FLOAT_EQ(moved.player.center.y, initial.player.center.y);
  for (std::size_t i = 0; i < moved.hazards.size(); ++i) {
    EXPECT_FLOAT_EQ(moved.hazards.at(i).velocity.x, moved.player.velocity.x);
    EXPECT_NEAR(moved.hazards.at(i).center.x - initial.hazards.at(i).center.x,
                moved.player.center.x - initial.player.center.x, 0.001F);
  }
}

TEST(PanTilt, ReleaseLevelsThePanButReverseTiltBrakesTheRemainingInertia) {
  Engine engine;
  tilt(engine, {.vertical = Direction::Negative}, 35);
  const auto moving = engine.snapshot();
  ASSERT_LT(moving.player.velocity.y, -10.0F);
  tilt(engine, {}, 35);
  const auto coasting = engine.snapshot();
  EXPECT_FLOAT_EQ(coasting.tilt.y, 0.0F);
  EXPECT_LT(coasting.player.velocity.y, -10.0F);
  EXPECT_LT(coasting.player.center.y, moving.player.center.y);
  tilt(engine, {.vertical = Direction::Positive}, 40);
  EXPECT_GT(engine.snapshot().player.velocity.y, coasting.player.velocity.y);
}

TEST(PanTilt, SlidingHazardsBounceAtTheRimAndContactLoses) {
  Engine engine;
  bool bounced = false;
  for (int tick = 0;
       tick < 400 && engine.snapshot().outcome == Outcome::Playing; ++tick) {
    engine.step({.vertical = Direction::Negative});
    const auto world = engine.snapshot();
    for (const auto &hazard : world.hazards) {
      EXPECT_LE(std::hypot(hazard.center.x, hazard.center.y),
                world.pan_radius - hazard.radius + 0.001F);
      bounced = bounced || hazard.velocity.y > 0.0F;
    }
  }
  EXPECT_TRUE(bounced);
  EXPECT_EQ(engine.snapshot().outcome, Outcome::Hit);
}

TEST(PanTilt, SteeringAroundHazardsEscapesAndFreezesEveryBody) {
  Engine engine;
  escape(engine);
  ASSERT_EQ(engine.snapshot().outcome, Outcome::Escaped);
  const auto won = engine.snapshot();
  EXPECT_LT(won.player.center.y, -won.pan_radius);
  tilt(engine, {.vertical = Direction::Positive}, 120);
  EXPECT_FLOAT_EQ(engine.snapshot().player.center.y, won.player.center.y);
  EXPECT_FLOAT_EQ(engine.snapshot().hazards.front().center.y,
                  won.hazards.front().center.y);
}

TEST(PanTilt, ResetClearsLossAndWinIncludingTiltAndEveryVelocity) {
  Engine engine;
  const auto initial = engine.snapshot();
  tilt(engine, {.horizontal = Direction::Negative}, 400);
  ASSERT_EQ(engine.snapshot().outcome, Outcome::Hit);
  const auto lost = engine.snapshot();
  tilt(engine, {.vertical = Direction::Negative}, 20);
  EXPECT_FLOAT_EQ(engine.snapshot().player.center.x, lost.player.center.x);
  engine.reset();
  EXPECT_EQ(engine.snapshot().outcome, Outcome::Playing);
  EXPECT_FLOAT_EQ(engine.snapshot().player.center.x, initial.player.center.x);
  EXPECT_FLOAT_EQ(engine.snapshot().tilt.x, 0.0F);
  EXPECT_FLOAT_EQ(engine.snapshot().player.velocity.x, 0.0F);
  for (const auto &hazard : engine.snapshot().hazards) {
    EXPECT_FLOAT_EQ(hazard.velocity.x, 0.0F);
    EXPECT_FLOAT_EQ(hazard.velocity.y, 0.0F);
  }
  escape(engine);
  ASSERT_EQ(engine.snapshot().outcome, Outcome::Escaped);
  engine.reset();
  EXPECT_EQ(engine.snapshot().outcome, Outcome::Playing);
  EXPECT_FLOAT_EQ(engine.snapshot().player.center.y, initial.player.center.y);
  EXPECT_FLOAT_EQ(engine.snapshot().tilt.y, 0.0F);
  EXPECT_FLOAT_EQ(engine.snapshot().player.velocity.y, 0.0F);
  EXPECT_FLOAT_EQ(engine.snapshot().hazards.front().center.y,
                  initial.hazards.front().center.y);
}

} // namespace
} // namespace neocity::pan_tilt
