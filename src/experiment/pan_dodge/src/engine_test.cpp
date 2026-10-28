#include <pan_dodge/engine.hpp>

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>

namespace neocity::pan_dodge {
namespace {

constexpr float TICK = 1.0F / 120.0F;

void move(Engine &engine, Command command, int ticks) {
  for (int i = 0; i < ticks; ++i) {
    engine.step(command);
  }
}

void idle_until(Engine &engine, std::uint32_t tick) {
  while (engine.snapshot().tick < tick &&
         engine.snapshot().outcome == Outcome::Playing) {
    engine.step({});
  }
}

Direction toward(float delta) {
  if (delta < -3.0F) {
    return Direction::Negative;
  }
  return delta > 3.0F ? Direction::Positive : Direction::Neutral;
}

/// Steers at a point just beyond the travelling exit until the run ends.
void chase_exit(Engine &engine) {
  for (int i = 0; i < 600 && engine.snapshot().outcome == Outcome::Playing;
       ++i) {
    const auto world = engine.snapshot();
    const float reach = world.pan_radius + 40.0F;
    engine.step({.horizontal = toward(reach * std::cos(world.exit_angle) -
                                      world.player.center.x),
                 .vertical = toward(reach * std::sin(world.exit_angle) -
                                    world.player.center.y)});
  }
}

/// Circles the pan centre so tracked drops land behind the player.
void orbit(Engine &engine, int ticks) {
  for (int i = 0; i < ticks && engine.snapshot().outcome == Outcome::Playing;
       ++i) {
    const auto p = engine.snapshot().player.center;
    const float distance = std::hypot(p.x, p.y);
    const float radial = distance < 1.0F ? 0.0F : 150.0F - distance;
    engine.step({.horizontal = toward(-p.y + radial * p.x / 150.0F),
                 .vertical = toward(p.x + radial * p.y / 150.0F)});
  }
}

TEST(PanDodge, DirectInputMovesThePlayerOnTopOfTheSharedSway) {
  Engine still;
  Engine moving;
  move(still, {}, 30);
  move(moving, {.vertical = Direction::Negative}, 30);
  const auto rest = still.snapshot().player.center;
  const auto moved = moving.snapshot().player.center;
  EXPECT_NEAR(moved.x, rest.x, 0.001F);
  EXPECT_NEAR(moved.y, rest.y - 170.0F * 30.0F * TICK, 0.01F);
}

TEST(PanDodge, DiagonalInputHasTheSameSpeedAsAnAxis) {
  Engine still;
  Engine moving;
  move(still, {}, 10);
  move(moving,
       {.horizontal = Direction::Positive, .vertical = Direction::Negative},
       10);
  const auto rest = still.snapshot().player.center;
  const auto moved = moving.snapshot().player.center;
  EXPECT_NEAR(std::hypot(moved.x - rest.x, moved.y - rest.y),
              170.0F * 10.0F * TICK, 0.01F);
}

TEST(PanDodge, ThePanSwaysByItselfAndItsExitTravelsAroundTheRim) {
  Engine engine;
  const auto initial = engine.snapshot();
  move(engine, {}, 120);
  const auto later = engine.snapshot();
  EXPECT_NEAR(later.exit_angle - initial.exit_angle, 0.35F, 0.001F);
  EXPECT_GT(std::abs(later.tilt.x - initial.tilt.x), 0.1F);
  EXPECT_GT(std::hypot(later.player.center.x, later.player.center.y), 10.0F);
}

TEST(PanDodge, TheRimBlocksTravelExceptThroughTheTravellingExit) {
  Engine blocked;
  move(blocked, {.vertical = Direction::Positive}, 250);
  const auto world = blocked.snapshot();
  ASSERT_EQ(world.outcome, Outcome::Playing);
  EXPECT_NEAR(std::hypot(world.player.center.x, world.player.center.y),
              world.pan_radius - world.player.radius, 0.01F);

  Engine open;
  chase_exit(open);
  ASSERT_EQ(open.snapshot().outcome, Outcome::Escaped);
  EXPECT_GT(std::hypot(open.snapshot().player.center.x,
                       open.snapshot().player.center.y),
            open.snapshot().pan_radius);
}

TEST(PanDodge, SteeringThroughTheTravellingExitEscapes) {
  Engine engine;
  chase_exit(engine);
  const auto world = engine.snapshot();
  ASSERT_EQ(world.outcome, Outcome::Escaped);
  EXPECT_GT(std::hypot(world.player.center.x, world.player.center.y),
            world.pan_radius);
  EXPECT_GT(std::abs(world.exit_angle + 1.5707964F), 0.1F);
}

TEST(PanDodge, ADropFollowsTheIdlePlayerButMissesOnceThePlayerLeavesItsMark) {
  Engine followed;
  idle_until(followed, 200);
  move(followed, {.horizontal = Direction::Positive}, 20);
  const auto tracking = followed.snapshot();
  ASSERT_EQ(tracking.hazards.size(), 1U);
  ASSERT_EQ(tracking.hazards.front().phase, Phase::Tracking);
  EXPECT_FLOAT_EQ(tracking.hazards.front().body.center.x,
                  tracking.player.center.x);
  EXPECT_FLOAT_EQ(tracking.hazards.front().body.center.y,
                  tracking.player.center.y);

  Engine idle;
  idle_until(idle, 299);
  ASSERT_EQ(idle.snapshot().outcome, Outcome::Playing);
  ASSERT_EQ(idle.snapshot().hazards.size(), 1U);
  EXPECT_EQ(idle.snapshot().hazards.front().phase, Phase::Falling);
  idle.step({});
  EXPECT_EQ(idle.snapshot().outcome, Outcome::Hit);

  Engine dodging;
  idle_until(dodging, 260);
  ASSERT_EQ(dodging.snapshot().hazards.front().phase, Phase::Falling);
  move(dodging, {.horizontal = Direction::Positive}, 60);
  const auto world = dodging.snapshot();
  EXPECT_EQ(world.outcome, Outcome::Playing);
  ASSERT_EQ(world.hazards.size(), 2U);
  EXPECT_EQ(world.hazards.front().phase, Phase::Landed);
}

TEST(PanDodge, HazardsKeepDroppingAndDespawnOffTheRimOrAfterTheirLife) {
  Engine engine;
  std::size_t most = 0;
  bool expiring = false;
  bool at_rim = false;
  while (engine.snapshot().tick < 1500 &&
         engine.snapshot().outcome == Outcome::Playing) {
    orbit(engine, 1);
    const auto world = engine.snapshot();
    most = std::max(most, world.hazards.size());
    for (const Hazard &hazard : world.hazards) {
      const float distance =
          std::hypot(hazard.body.center.x, hazard.body.center.y);
      ASSERT_GT(hazard.life, 0.0F);
      if (hazard.phase == Phase::Landed) {
        ASSERT_LE(distance, world.pan_radius);
        expiring = expiring || (hazard.life <= 1.5F * TICK &&
                                distance < world.pan_radius - 20.0F);
        at_rim = at_rim || distance > world.pan_radius - 2.0F;
      }
    }
  }
  ASSERT_EQ(engine.snapshot().tick, 1500U);
  EXPECT_TRUE(expiring);
  EXPECT_TRUE(at_rim);
  EXPECT_GE(most, 3U);
  EXPECT_LE(most, 6U);
}

TEST(PanDodge, TerminalOutcomesFreezeTheWholePan) {
  Engine hit;
  idle_until(hit, 300);
  ASSERT_EQ(hit.snapshot().outcome, Outcome::Hit);
  const auto stopped = hit.snapshot();
  move(hit, {.vertical = Direction::Negative}, 60);
  EXPECT_EQ(hit.snapshot().tick, stopped.tick);
  EXPECT_FLOAT_EQ(hit.snapshot().player.center.y, stopped.player.center.y);
  EXPECT_FLOAT_EQ(hit.snapshot().hazards.front().body.center.x,
                  stopped.hazards.front().body.center.x);

  Engine escaped;
  chase_exit(escaped);
  ASSERT_EQ(escaped.snapshot().outcome, Outcome::Escaped);
  const auto exit = escaped.snapshot();
  move(escaped, {.vertical = Direction::Positive}, 100);
  EXPECT_FLOAT_EQ(escaped.snapshot().player.center.y, exit.player.center.y);
  EXPECT_FLOAT_EQ(escaped.snapshot().exit_angle, exit.exit_angle);
}

TEST(PanDodge, ResetRestoresAFreshRunAndTheSameDropSequence) {
  Engine fresh;
  move(fresh, {.horizontal = Direction::Positive}, 30);
  idle_until(fresh, 300);
  const auto expected = fresh.snapshot().hazards;

  Engine engine;
  idle_until(engine, 300);
  ASSERT_EQ(engine.snapshot().outcome, Outcome::Hit);
  engine.reset();
  const auto restarted = engine.snapshot();
  EXPECT_EQ(restarted.outcome, Outcome::Playing);
  EXPECT_EQ(restarted.tick, 0U);
  EXPECT_TRUE(restarted.hazards.empty());
  EXPECT_FLOAT_EQ(restarted.player.center.x, 0.0F);
  EXPECT_FLOAT_EQ(restarted.player.center.y, 0.0F);

  move(engine, {.horizontal = Direction::Positive}, 30);
  idle_until(engine, 300);
  ASSERT_EQ(engine.snapshot().outcome, Outcome::Hit);
  ASSERT_EQ(expected.size(), 1U);
  ASSERT_EQ(engine.snapshot().hazards.size(), expected.size());
  EXPECT_FLOAT_EQ(engine.snapshot().hazards.front().body.center.x,
                  expected.front().body.center.x);
  EXPECT_FLOAT_EQ(engine.snapshot().hazards.front().body.radius,
                  expected.front().body.radius);
}

} // namespace
} // namespace neocity::pan_dodge
