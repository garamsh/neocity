#include <survival/exploration.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <limits>
#include <stdexcept>
#include <vector>

namespace neocity::survival {
namespace {

using namespace std::chrono_literals;
constexpr ObjectId FLOOR{1};
constexpr ObjectId SOURCE{10};
constexpr ObjectId OWNER{20};
constexpr GridPosition BASE{.x = -10, .y = -10};
constexpr GridPosition ENTRY{.x = 0, .y = 0};
constexpr std::array<ExplorationInput, 2> IDLE{};
constexpr std::array<ExplorationInput, 2> WALK{
    ExplorationInput{.direction = Direction::Right},
    ExplorationInput{.direction = Direction::Right}};
constexpr std::array<ExplorationInput, 2> RUN{
    ExplorationInput{.direction = Direction::Right, .running = true},
    ExplorationInput{.direction = Direction::Right, .running = true}};

/// A one-row walkway from x = 0 to 19 with a source at x = 1.
std::vector<ObjectDefinition> city() {
  return {{.id = FLOOR, .area = {.x = 0, .y = 0, .width = 20}},
          {.id = SOURCE,
           .kind = ObjectKind::MaterialSource,
           .area = {.x = 1},
           .ingredient = IngredientId{70}}};
}

ExplorationConfig tuning() {
  return {.base = BASE,
          .entry = ENTRY,
          .maximum_stamina = 2,
          .collection_distance = 1,
          .sources = {{.id = SOURCE}}};
}

/// The cell an explorer's feet stand in.
GridPosition at(const ExplorerSnapshot &explorer) {
  return cell_of(explorer.body.position);
}

/// One-row floor `width` cells wide from `x` on `row`.
ObjectDefinition floor_at(ObjectId id, std::int32_t x, std::int32_t row,
                          std::int32_t width) {
  return {.id = id, .area = {.x = x, .y = row, .width = width}};
}

/// Exploration over `objects` entered by the chef at `entry`; the acid lies
/// below the lowest object.
class Course {
public:
  Course(const std::vector<ObjectDefinition> &objects, GridPosition entry)
      : simulation({.exploration_budget = 30s, .seed = 0}), world(objects),
        exploration(simulation, world,
                    {.base = BASE,
                     .entry = entry,
                     .maximum_stamina = 2,
                     .sources = {}}) {
    if (exploration.enter(Actor::Chef) != Result::Applied) {
      throw std::logic_error("course entry refused");
    }
  }

  /// Advances with the chef pressing `input` and the appraiser at base.
  void press(std::chrono::milliseconds elapsed, ExplorationInput input) {
    if (exploration.advance(elapsed, {input, {}}, {}) != Result::Applied) {
      throw std::logic_error("course advance refused");
    }
  }

  [[nodiscard]] ExplorerSnapshot chef() const {
    return exploration.snapshot().chef;
  }
  [[nodiscard]] Location location() const {
    return simulation.snapshot().chef.location;
  }

  /// Advances no time at `acid`, so the world changes it brings apply.
  [[nodiscard]] Result acid(AcidLevel acid) {
    return exploration.advance({}, {}, acid);
  }

private:
  Simulation simulation;
  World world;
  Exploration exploration;
};

constexpr ExplorationInput RIGHT{.direction = Direction::Right};
constexpr ExplorationInput LEFT{.direction = Direction::Left};
constexpr ExplorationInput UP{.direction = Direction::Up};
constexpr ExplorationInput DOWN{.direction = Direction::Down};
constexpr ExplorationInput JUMP_RIGHT{.direction = Direction::Right,
                                      .jump = true};

TEST(Exploration, BothRolesMoveIdenticallyAndShareOneClock) {
  Simulation simulation({.exploration_budget = 5s, .seed = 9});
  World world(city());
  Exploration exploration(simulation, world, tuning());
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.enter(Actor::Appraiser), Result::Applied);
  ASSERT_EQ(exploration.advance(200ms, WALK, {}), Result::Applied);
  EXPECT_EQ(at(exploration.snapshot().chef), (GridPosition{.x = 2}));
  EXPECT_EQ(exploration.snapshot().chef, exploration.snapshot().appraiser);
  EXPECT_EQ(simulation.snapshot().remaining_exploration, 4800ms);
  EXPECT_EQ(world.snapshot().progression.elapsed, 200ms);
  EXPECT_EQ(simulation.snapshot().acid, AcidLevel{});
}

TEST(Exploration,
     ShortFramesConsumeTimeImmediatelyAndRetainMovementForEachActor) {
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(city());
  Exploration exploration(simulation, world, tuning());
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.advance(60ms, WALK, {}), Result::Applied);
  EXPECT_EQ(simulation.snapshot().remaining_exploration, 4940ms);
  EXPECT_EQ(world.snapshot().progression.elapsed, 60ms);
  EXPECT_EQ(at(exploration.snapshot().chef), ENTRY);
  ASSERT_EQ(exploration.enter(Actor::Appraiser), Result::Applied);
  ASSERT_EQ(exploration.advance(40ms, WALK, {}), Result::Applied);
  EXPECT_EQ(at(exploration.snapshot().chef), (GridPosition{.x = 1}));
  EXPECT_EQ(at(exploration.snapshot().appraiser), ENTRY);
  ASSERT_EQ(exploration.advance(60ms, WALK, {}), Result::Applied);
  EXPECT_EQ(at(exploration.snapshot().appraiser), (GridPosition{.x = 1}));
}

TEST(Exploration, RunningExhaustsStaminaThenWalksAndRecoversForBothRoles) {
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(city());
  Exploration exploration(simulation, world, tuning());
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.enter(Actor::Appraiser), Result::Applied);
  ASSERT_EQ(exploration.advance(200ms, RUN, {}), Result::Applied);
  EXPECT_EQ(at(exploration.snapshot().chef), (GridPosition{.x = 3}));
  EXPECT_EQ(exploration.snapshot().chef.stamina, 0U);
  EXPECT_EQ(exploration.snapshot().chef, exploration.snapshot().appraiser);
  ASSERT_EQ(exploration.advance(100ms, RUN, {}), Result::Applied);
  EXPECT_EQ(at(exploration.snapshot().chef), (GridPosition{.x = 4}));
  EXPECT_EQ(exploration.snapshot().chef.stamina, 1U);
  ASSERT_EQ(exploration.advance(300ms, IDLE, {}), Result::Applied);
  EXPECT_EQ(exploration.snapshot().chef.stamina, 2U);
  EXPECT_EQ(exploration.snapshot().chef, exploration.snapshot().appraiser);
}

TEST(Exploration, BuildingsAndBrokenEntrancesBlockButABrokenLadderDoesNot) {
  for (const auto kind :
       {ObjectKind::Building, ObjectKind::Ladder, ObjectKind::Entrance}) {
    auto definitions = city();
    definitions.push_back(
        {.id = OWNER,
         .kind = kind,
         .area = {.x = 1},
         .destroyed_after = WorldThreshold{.acid = AcidLevel{1}}});
    Simulation simulation({.exploration_budget = 5s, .seed = 0});
    World world(definitions);
    Exploration exploration(simulation, world, tuning());
    ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
    ASSERT_EQ(exploration.advance(100ms, RUN, {}), Result::Applied);
    if (kind == ObjectKind::Building) {
      EXPECT_EQ(at(exploration.snapshot().chef), ENTRY);
      ASSERT_EQ(exploration.advance(100ms, RUN, {1}), Result::Applied);
      EXPECT_EQ(at(exploration.snapshot().chef), (GridPosition{.x = 2}));
    } else {
      EXPECT_EQ(at(exploration.snapshot().chef), (GridPosition{.x = 2}));
      const std::array<ExplorationInput, 2> left{LEFT, {}};
      ASSERT_EQ(exploration.advance(100ms, left, {1}), Result::Applied);
      const auto expected = kind == ObjectKind::Ladder ? GridPosition{.x = 1}
                                                       : GridPosition{.x = 2};
      EXPECT_EQ(at(exploration.snapshot().chef), expected);
    }
  }
}

TEST(Exploration,
     OpenPassageOverridesBuildingAndRepairRestoresAccessInEitherOrder) {
  for (const auto kind : {ObjectKind::Ladder, ObjectKind::Entrance}) {
    for (const bool reversed : {false, true}) {
      auto definitions = city();
      definitions.push_back(
          {.id = OWNER, .kind = ObjectKind::Building, .area = {.x = 1}});
      definitions.push_back(
          {.id = ObjectId{21},
           .kind = kind,
           .area = {.x = 1},
           .destroyed_after = WorldThreshold{.acid = AcidLevel{1}},
           .repair_with = ToolId{1}});
      if (reversed) {
        std::ranges::reverse(definitions);
      }
      Simulation simulation({.exploration_budget = 5s, .seed = 0});
      World world(definitions);
      Exploration exploration(simulation, world, tuning());
      ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
      ASSERT_EQ(exploration.advance(100ms, RUN, {}), Result::Applied);
      EXPECT_EQ(at(exploration.snapshot().chef), (GridPosition{.x = 2}));
      const std::array<ExplorationInput, 2> left{LEFT, {}};
      ASSERT_EQ(exploration.advance(100ms, left, {1}), Result::Applied);
      EXPECT_EQ(at(exploration.snapshot().chef), (GridPosition{.x = 2}));
      ASSERT_EQ(world.repair(ObjectId{21}, ToolId{1}), RepairResult::Applied);
      ASSERT_EQ(exploration.advance(100ms, left, {1}), Result::Applied);
      EXPECT_EQ(at(exploration.snapshot().chef), (GridPosition{.x = 1}));
    }
  }
}

TEST(Exploration, GravityDropsAWalkerOffAnEdgeOntoTheFloorBelow) {
  Course course({floor_at(FLOOR, 0, 0, 3), floor_at(OWNER, 0, 3, 10)}, ENTRY);
  course.press(1s, RIGHT);
  EXPECT_EQ(course.location(), Location::City);
  EXPECT_EQ(course.chef().body.stance, Stance::Standing);
  EXPECT_EQ(at(course.chef()).y, 3);
  EXPECT_GT(at(course.chef()).x, 2);
}

TEST(Exploration, AJumpClearsATwoCellGapButNotAThreeCellGap) {
  for (const std::int32_t gap : {2, 3}) {
    const std::int32_t far = 4 + gap;
    Course course({floor_at(FLOOR, 0, 0, 4), floor_at(OWNER, far, 0, 4)},
                  {.x = 3});
    course.press(100ms, JUMP_RIGHT);
    course.press(300ms, RIGHT);
    course.press(200ms, {});
    if (gap == 2) {
      EXPECT_EQ(course.location(), Location::City);
      EXPECT_EQ(at(course.chef()), (GridPosition{.x = far}));
    } else {
      EXPECT_EQ(course.location(), Location::Base);
    }
  }
}

TEST(Exploration, AJumpLandsOneRowUpButNeverTwo) {
  for (const std::int32_t rows : {1, 2}) {
    Course course({floor_at(FLOOR, 0, 2, 4), floor_at(OWNER, 5, 2 - rows, 4)},
                  {.x = 3, .y = 2});
    course.press(100ms, JUMP_RIGHT);
    course.press(200ms, RIGHT);
    course.press(200ms, {});
    if (rows == 1) {
      EXPECT_EQ(course.chef().body.stance, Stance::Standing);
      EXPECT_EQ(at(course.chef()), (GridPosition{.x = 5, .y = 1}));
    } else {
      EXPECT_EQ(course.location(), Location::Base);
    }
  }
}

TEST(Exploration, AJumpPassesUpThroughAFloorAndLandsOnIt) {
  Course course({floor_at(FLOOR, 0, 2, 6), floor_at(OWNER, 0, 1, 6)},
                {.x = 1, .y = 2});
  course.press(100ms, {.jump = true});
  course.press(400ms, {});
  EXPECT_EQ(course.chef().body.stance, Stance::Standing);
  EXPECT_EQ(at(course.chef()), (GridPosition{.x = 1, .y = 1}));
}

TEST(Exploration, AJumpPressedInTheAirTakesOnLandingWithinItsUnit) {
  for (const bool on_landing : {false, true}) {
    Course course({floor_at(FLOOR, 0, 2, 6)}, {.x = 1, .y = 2});
    course.press(100ms, {.jump = true});
    course.press(100ms, {});
    // Airborne for this whole unit, so its press is lost.
    course.press(100ms, {.jump = !on_landing});
    // Lands on this unit's fourth tick and jumps on its fifth.
    course.press(100ms, {.jump = on_landing});
    EXPECT_EQ(course.chef().body.stance,
              on_landing ? Stance::Airborne : Stance::Standing);
  }
}

TEST(Exploration, ABuildingStopsBothAWalkAndAJumpUntilItFalls) {
  for (const bool fallen : {false, true}) {
    Course course({floor_at(FLOOR, 0, 0, 8),
                   {.id = OWNER,
                    .kind = ObjectKind::Building,
                    .area = {.x = 2, .y = -1, .height = 2},
                    .destroyed_after = WorldThreshold{.acid = AcidLevel{1}}}},
                  ENTRY);
    if (fallen) {
      ASSERT_EQ(course.acid(AcidLevel{1}), Result::Applied);
    }
    course.press(300ms, RIGHT);
    course.press(100ms, JUMP_RIGHT);
    course.press(400ms, RIGHT);
    if (fallen) {
      EXPECT_GT(at(course.chef()).x, 2);
    } else {
      EXPECT_EQ(at(course.chef()), (GridPosition{.x = 1}));
    }
  }
}

TEST(Exploration, ALowBlockIsAHurdleToJumpOntoAndStepDownFrom) {
  Course course({floor_at(FLOOR, 0, 1, 8),
                 {.id = OWNER,
                  .kind = ObjectKind::Building,
                  .area = {.x = 2, .y = 1, .width = 2}}},
                {.x = 1, .y = 1});
  course.press(300ms, RIGHT);
  ASSERT_EQ(at(course.chef()), (GridPosition{.x = 1, .y = 1}));
  course.press(100ms, JUMP_RIGHT);
  course.press(200ms, RIGHT);
  EXPECT_EQ(course.chef().body.stance, Stance::Standing);
  EXPECT_EQ(at(course.chef()), (GridPosition{.x = 3, .y = 0}));
  course.press(100ms, RIGHT);
  EXPECT_EQ(at(course.chef()), (GridPosition{.x = 4, .y = 1}));
}

TEST(Exploration, StairsAreWalkedUpAndDownAStepAtATime) {
  Course course({floor_at(FLOOR, 0, 3, 3), floor_at(ObjectId{2}, 3, 2, 1),
                 floor_at(ObjectId{3}, 4, 1, 1), floor_at(OWNER, 5, 0, 4)},
                {.x = 0, .y = 3});
  course.press(600ms, RIGHT);
  EXPECT_EQ(course.chef().body.stance, Stance::Standing);
  EXPECT_EQ(at(course.chef()), (GridPosition{.x = 5, .y = 0}));
  course.press(600ms, LEFT);
  EXPECT_EQ(course.chef().body.stance, Stance::Standing);
  EXPECT_EQ(at(course.chef()), (GridPosition{.x = 0, .y = 3}));
}

TEST(Exploration, ALadderIsClimbedBetweenItsEndsAndLeftOntoAFloor) {
  for (const bool broken : {false, true}) {
    Course course({floor_at(FLOOR, 0, 4, 6),
                   floor_at(ObjectId{2}, 2, 1, 4),
                   {.id = OWNER,
                    .kind = ObjectKind::Ladder,
                    .area = {.x = 2, .y = 1, .height = 4},
                    .destroyed_after = WorldThreshold{.acid = AcidLevel{1}}}},
                  {.x = 2, .y = 4});
    if (broken) {
      ASSERT_EQ(course.acid(AcidLevel{1}), Result::Applied);
    }
    course.press(100ms, UP);
    if (broken) {
      EXPECT_EQ(course.chef().body.stance, Stance::Standing);
      EXPECT_EQ(at(course.chef()), (GridPosition{.x = 2, .y = 4}));
      continue;
    }
    const auto partway = course.chef().body;
    EXPECT_EQ(partway.stance, Stance::Climbing);
    course.press(300ms, {});
    EXPECT_EQ(course.chef().body, partway);
    course.press(1s, UP);
    EXPECT_EQ(course.chef().body.position, standing_in({.x = 2, .y = 1}));
    course.press(100ms, RIGHT);
    EXPECT_EQ(course.chef().body.stance, Stance::Standing);
    EXPECT_EQ(at(course.chef()), (GridPosition{.x = 3, .y = 1}));
    course.press(100ms, LEFT);
    course.press(1s, DOWN);
    EXPECT_EQ(course.chef().body.position, standing_in({.x = 2, .y = 4}));
  }
}

TEST(Exploration, ACityPastTheMovementLimitIsRefused) {
  auto definitions = city();
  ASSERT_NO_THROW(CellMap(World(definitions).snapshot()));
  definitions.push_back(
      {.id = OWNER, .area = {.x = MAP_LIMIT - 1, .width = 2}});
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(definitions);
  EXPECT_THROW(CellMap(world.snapshot()), std::invalid_argument);
  EXPECT_THROW(Exploration(simulation, world, tuning()), std::invalid_argument);
}

TEST(Exploration, FallPreservesCollectedItemsAndTimeAndAllowsFreshEntry) {
  auto definitions = city();
  definitions.front().area.width = 2;
  Simulation simulation({.exploration_budget = 5s, .seed = 19});
  World world(definitions);
  Exploration exploration(simulation, world, tuning());
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Applied);
  ASSERT_EQ(exploration.damage(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.advance(1s, RUN, {}), Result::Applied);
  EXPECT_EQ(simulation.snapshot().chef.location, Location::Base);
  EXPECT_EQ(simulation.snapshot().chef.health, 3U);
  EXPECT_EQ(simulation.snapshot().chef.inventory,
            (std::vector{IngredientId{70}}));
  EXPECT_EQ(at(exploration.snapshot().chef), BASE);
  // Off the edge in the first unit, below the floor on the second.
  EXPECT_EQ(simulation.snapshot().remaining_exploration, 4800ms);
  EXPECT_EQ(simulation.snapshot().seed, 19U);
  EXPECT_EQ(simulation.snapshot().day, 1U);
  EXPECT_EQ(exploration.snapshot().sources.front().remaining, 0U);
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  EXPECT_EQ(at(exploration.snapshot().chef), ENTRY);
  ASSERT_EQ(exploration.advance(50ms, WALK, {}), Result::Applied);
  EXPECT_EQ(at(exploration.snapshot().chef), ENTRY);
}

TEST(Exploration, ManualAndHealthReturnsClearPositionAndPartialMovementOnly) {
  for (const bool exhausted : {false, true}) {
    Simulation simulation({.exploration_budget = 5s, .seed = 0});
    World world(city());
    Exploration exploration(simulation, world, tuning());
    ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
    ASSERT_EQ(exploration.enter(Actor::Appraiser), Result::Applied);
    ASSERT_EQ(exploration.collect(Actor::Chef, SOURCE),
              CollectionResult::Applied);
    ASSERT_EQ(exploration.advance(150ms, WALK, {}), Result::Applied);
    EXPECT_EQ(at(exploration.snapshot().chef), (GridPosition{.x = 1}));
    const auto persistent_world = world.snapshot();
    if (exhausted) {
      for (int hit = 0; hit < 3; ++hit) {
        ASSERT_EQ(exploration.damage(Actor::Chef), Result::Applied);
      }
    } else {
      ASSERT_EQ(exploration.return_to_base(Actor::Chef), Result::Applied);
    }
    EXPECT_EQ(at(exploration.snapshot().chef), BASE);
    EXPECT_EQ(simulation.snapshot().chef.health, 3U);
    EXPECT_EQ(simulation.snapshot().chef.inventory,
              (std::vector{IngredientId{70}}));
    EXPECT_EQ(simulation.snapshot().appraiser.location, Location::City);
    EXPECT_EQ(simulation.snapshot().remaining_exploration, 4850ms);
    EXPECT_EQ(world.snapshot(), persistent_world);
    ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
    ASSERT_EQ(exploration.advance(50ms, WALK, {}), Result::Applied);
    EXPECT_EQ(at(exploration.snapshot().chef), ENTRY);
    EXPECT_EQ(at(exploration.snapshot().appraiser), (GridPosition{.x = 2}));
    EXPECT_EQ(exploration.collect(Actor::Chef, SOURCE),
              CollectionResult::Depleted);
  }
}

TEST(Exploration, ExpiryStopsBothActorsBeforeMovementOrCollectionBeyondBudget) {
  Simulation simulation({.exploration_budget = 150ms, .seed = 0});
  World world(city());
  Exploration exploration(simulation, world, tuning());
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.enter(Actor::Appraiser), Result::Applied);
  ASSERT_EQ(exploration.advance(100ms, WALK, {}), Result::Applied);
  EXPECT_EQ(at(exploration.snapshot().chef), (GridPosition{.x = 1}));
  ASSERT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Applied);
  ASSERT_EQ(exploration.advance(10s, RUN, {}), Result::Applied);
  EXPECT_EQ(at(exploration.snapshot().chef), BASE);
  EXPECT_EQ(at(exploration.snapshot().appraiser), BASE);
  EXPECT_EQ(simulation.snapshot().remaining_exploration, 0us);
  EXPECT_EQ(simulation.snapshot().phase, Phase::Workshop);
  EXPECT_EQ(world.snapshot().progression.elapsed, 150ms);
  EXPECT_EQ(exploration.collect(Actor::Appraiser, SOURCE),
            CollectionResult::NotExploring);
  EXPECT_EQ(exploration.enter(Actor::Chef), Result::NoExplorationTime);
  EXPECT_EQ(simulation.snapshot().chef.inventory,
            (std::vector{IngredientId{70}}));
}

TEST(Exploration, AllFourSourceKindsShareFiniteStockAndActorRules) {
  auto definitions = city();
  auto config = tuning();
  config.sources.clear();
  const std::array kinds{SourceKind::VendingMachine, SourceKind::Npc,
                         SourceKind::Shop, SourceKind::Bag};
  for (std::size_t index = 0; index < kinds.size(); ++index) {
    const ObjectId id{static_cast<std::uint32_t>(100 + index)};
    definitions.push_back(
        {.id = id,
         .kind = ObjectKind::MaterialSource,
         .ingredient = IngredientId{static_cast<std::uint32_t>(index)}});
    config.sources.push_back(
        {.id = id, .kind = kinds.at(index), .remaining = 2});
  }
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(definitions);
  Exploration exploration(simulation, world, config);
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.enter(Actor::Appraiser), Result::Applied);
  for (const auto &source : config.sources) {
    EXPECT_EQ(exploration.collect(Actor::Chef, source.id),
              CollectionResult::Applied);
    EXPECT_EQ(exploration.collect(Actor::Appraiser, source.id),
              CollectionResult::Applied);
    EXPECT_EQ(exploration.collect(Actor::Chef, source.id),
              CollectionResult::Depleted);
  }
  EXPECT_EQ(simulation.snapshot().chef.inventory,
            simulation.snapshot().appraiser.inventory);
  EXPECT_EQ(simulation.snapshot().chef.inventory.size(), 4U);
  auto copied = exploration.snapshot();
  EXPECT_EQ(copied.sources.front().kind, SourceKind::VendingMachine);
  copied.sources.front().remaining = 50;
  EXPECT_EQ(exploration.snapshot().sources.front().remaining, 0U);
}

TEST(Exploration,
     CollectionRejectsUnknownAndDistantSourcesWithoutChangingState) {
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(city());
  Exploration exploration(simulation, world, tuning());
  const auto at_base = exploration.snapshot();
  EXPECT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::NotExploring);
  EXPECT_EQ(exploration.snapshot(), at_base);
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.advance(400ms, WALK, {}), Result::Applied);
  const auto before = simulation.snapshot();
  const auto stock = exploration.snapshot();
  EXPECT_EQ(exploration.collect(Actor::Chef, ObjectId{999}),
            CollectionResult::UnknownSource);
  EXPECT_EQ(exploration.collect(Actor::Chef, FLOOR),
            CollectionResult::UnknownSource);
  EXPECT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::OutOfRange);
  EXPECT_EQ(simulation.snapshot(), before);
  EXPECT_EQ(exploration.snapshot(), stock);
  const std::array<ExplorationInput, 2> left{
      ExplorationInput{.direction = Direction::Left}, {}};
  ASSERT_EQ(exploration.advance(100ms, left, {}), Result::Applied);
  EXPECT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Applied);
}

TEST(Exploration, SupplyTracksDormantDestroyedAndOwnerDestroyedWorldObjects) {
  for (const bool destroy_owner : {false, true}) {
    auto definitions = city();
    definitions.back().exposed_at = WorldThreshold{.elapsed = 100ms};
    if (destroy_owner) {
      definitions.push_back(
          {.id = OWNER,
           .kind = ObjectKind::Npc,
           .destroyed_after = WorldThreshold{.elapsed = 200ms}});
      definitions.at(1).supported_by = OWNER;
    } else {
      definitions.back().destroyed_after = WorldThreshold{.elapsed = 200ms};
    }
    auto config = tuning();
    config.sources.front().remaining = 2;
    Simulation simulation({.exploration_budget = 5s, .seed = 0});
    World world(definitions);
    Exploration exploration(simulation, world, config);
    ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
    EXPECT_EQ(exploration.collect(Actor::Chef, SOURCE),
              CollectionResult::Unavailable);
    ASSERT_EQ(exploration.advance(100ms, IDLE, {}), Result::Applied);
    ASSERT_EQ(exploration.collect(Actor::Chef, SOURCE),
              CollectionResult::Applied);
    ASSERT_EQ(exploration.advance(100ms, IDLE, {}), Result::Applied);
    const auto before = simulation.snapshot();
    EXPECT_EQ(exploration.collect(Actor::Chef, SOURCE),
              CollectionResult::Unavailable);
    EXPECT_EQ(simulation.snapshot(), before);
    EXPECT_EQ(exploration.snapshot().sources.front().remaining, 1U);
  }
}

TEST(Exploration, AFloorThatBreaksDropsItsWalkerIntoTheAcidKeepingHistory) {
  auto definitions = city();
  definitions.front().destroyed_after = WorldThreshold{.elapsed = 150ms};
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(definitions);
  Exploration exploration(simulation, world, tuning());
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.advance(100ms, IDLE, {}), Result::Applied);
  EXPECT_EQ(simulation.snapshot().chef.location, Location::City);
  ASSERT_EQ(exploration.advance(50ms, IDLE, {2}), Result::Applied);
  EXPECT_EQ(world.snapshot().objects.front().state, ObjectState::Destroyed);
  EXPECT_EQ(simulation.snapshot().chef.location, Location::City);
  ASSERT_EQ(exploration.advance(1s, IDLE, {}), Result::Applied);
  EXPECT_EQ(simulation.snapshot().chef.location, Location::Base);
  EXPECT_EQ(simulation.snapshot().chef.health, 3U);
  EXPECT_EQ(at(exploration.snapshot().chef), BASE);
  EXPECT_EQ(world.snapshot().peak_acid, (AcidLevel{2}));
  EXPECT_EQ(simulation.snapshot().acid, AcidLevel{});
  EXPECT_EQ(exploration.enter(Actor::Chef), Result::NotExploring);
  const auto before = simulation.snapshot();
  const auto settled = world.snapshot().progression.elapsed;
  ASSERT_EQ(exploration.advance(1s, IDLE, {}), Result::Applied);
  EXPECT_EQ(simulation.snapshot(), before);
  EXPECT_EQ(world.snapshot().progression.elapsed, settled);
  EXPECT_EQ(world.snapshot().peak_acid, (AcidLevel{2}));
}

TEST(Exploration, NegativeElapsedAndRefusedBaseCommandsChangeNothing) {
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(city());
  Exploration exploration(simulation, world, tuning());
  EXPECT_EQ(exploration.damage(Actor::Chef), Result::NotExploring);
  EXPECT_EQ(exploration.return_to_base(Actor::Chef), Result::NotExploring);
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.advance(50ms, WALK, {}), Result::Applied);
  const auto before = simulation.snapshot();
  const auto before_world = world.snapshot();
  const auto before_exploration = exploration.snapshot();
  EXPECT_EQ(exploration.advance(-1us, RUN, {999}), Result::InvalidElapsed);
  EXPECT_EQ(simulation.snapshot(), before);
  EXPECT_EQ(world.snapshot(), before_world);
  EXPECT_EQ(exploration.snapshot(), before_exploration);
  ASSERT_EQ(exploration.advance(50ms, WALK, {}), Result::Applied);
  EXPECT_EQ(at(exploration.snapshot().chef), (GridPosition{.x = 1}));
}

TEST(Exploration,
     ConfigurationRequiresSafeEntryAndUniqueExistingMaterialStock) {
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(city());
  auto config = tuning();
  EXPECT_NO_THROW(Exploration(simulation, world, config));
  config.sources.push_back(config.sources.front());
  EXPECT_THROW(Exploration(simulation, world, config), std::invalid_argument);
  config = tuning();
  config.sources.front().id = FLOOR;
  EXPECT_THROW(Exploration(simulation, world, config), std::invalid_argument);
  config.sources.front().id = ObjectId{999};
  EXPECT_THROW(Exploration(simulation, world, config), std::invalid_argument);
  config = tuning();
  config.sources.front().remaining = 0;
  EXPECT_THROW(Exploration(simulation, world, config), std::invalid_argument);
  config = tuning();
  config.maximum_stamina = 0;
  EXPECT_THROW(Exploration(simulation, world, config), std::invalid_argument);
  config = tuning();
  config.entry = BASE;
  EXPECT_THROW(Exploration(simulation, world, config), std::invalid_argument);
  EXPECT_EQ(simulation.snapshot().phase, Phase::Workshop);
}

constexpr ObjectId TOOL_SOURCE{30};
constexpr ObjectId LADDER{31};
constexpr ToolId WRENCH{5};
constexpr ToolId HAMMER{6};

/// The ladder at x = 3 breaks at acid 1 and takes a wrench; the crate at x = 1
/// holds two wrenches.
std::vector<ObjectDefinition> toolbox_city() {
  auto objects = city();
  objects.push_back({.id = TOOL_SOURCE,
                     .kind = ObjectKind::MaterialSource,
                     .area = {.x = 1},
                     .tool = WRENCH});
  objects.push_back({.id = LADDER,
                     .kind = ObjectKind::Ladder,
                     .area = {.x = 3},
                     .destroyed_after = WorldThreshold{.acid = AcidLevel{1}},
                     .repair_with = WRENCH});
  return objects;
}

ExplorationConfig toolbox_tuning() {
  auto config = tuning();
  config.sources.push_back({.id = TOOL_SOURCE, .remaining = 2});
  return config;
}

TEST(Exploration, BothRolesCollectToolsWithoutTouchingIngredients) {
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(toolbox_city());
  Exploration exploration(simulation, world, toolbox_tuning());
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.enter(Actor::Appraiser), Result::Applied);
  EXPECT_EQ(exploration.collect(Actor::Chef, TOOL_SOURCE),
            CollectionResult::Applied);
  EXPECT_EQ(exploration.collect(Actor::Appraiser, TOOL_SOURCE),
            CollectionResult::Applied);
  EXPECT_EQ(exploration.collect(Actor::Appraiser, TOOL_SOURCE),
            CollectionResult::Depleted);
  EXPECT_EQ(simulation.snapshot().chef.tools, (std::vector<ToolId>{WRENCH}));
  EXPECT_EQ(simulation.snapshot().appraiser.tools,
            (std::vector<ToolId>{WRENCH}));
  EXPECT_TRUE(simulation.snapshot().chef.inventory.empty());
  EXPECT_TRUE(simulation.snapshot().appraiser.inventory.empty());
}

TEST(Exploration, ToolCollectionChecksDistanceAndSupply) {
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  auto objects = toolbox_city();
  objects.at(2).destroyed_after = WorldThreshold{.acid = AcidLevel{1}};
  objects.at(2).area = {.x = 2};
  World world(objects);
  Exploration exploration(simulation, world, toolbox_tuning());
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  EXPECT_EQ(exploration.collect(Actor::Chef, TOOL_SOURCE),
            CollectionResult::OutOfRange);
  ASSERT_EQ(exploration.advance(100ms, WALK, {}), Result::Applied);
  ASSERT_EQ(exploration.advance({}, IDLE, AcidLevel{1}), Result::Applied);
  EXPECT_EQ(exploration.collect(Actor::Chef, TOOL_SOURCE),
            CollectionResult::Unavailable);
  EXPECT_TRUE(simulation.snapshot().chef.tools.empty());
  EXPECT_EQ(exploration.snapshot().sources.at(1).remaining, 2U);
}

TEST(Exploration, ToolsSurviveAFallAndAnExhaustionAndStockStaysSpent) {
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(toolbox_city());
  Exploration exploration(simulation, world, toolbox_tuning());
  const std::array<ExplorationInput, 2> off_the_edge{LEFT, {}};
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.collect(Actor::Chef, TOOL_SOURCE),
            CollectionResult::Applied);
  ASSERT_EQ(exploration.advance(100ms, off_the_edge, {}), Result::Applied);
  ASSERT_EQ(simulation.snapshot().chef.location, Location::Base);
  EXPECT_EQ(simulation.snapshot().chef.tools, (std::vector<ToolId>{WRENCH}));
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  for (int hit = 0; hit < 3; ++hit) {
    ASSERT_EQ(exploration.damage(Actor::Chef), Result::Applied);
  }
  ASSERT_EQ(simulation.snapshot().chef.location, Location::Base);
  EXPECT_EQ(simulation.snapshot().chef.tools, (std::vector<ToolId>{WRENCH}));
  EXPECT_EQ(exploration.snapshot().sources.at(1).remaining, 1U);
}

TEST(Exploration, RepairNeedsTheCorrectToolCarriedByThatActorAndKeepsIt) {
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  auto objects = toolbox_city();
  objects.at(2).area = {.x = 0, .y = 1};
  objects.push_back({.id = ObjectId{32},
                     .kind = ObjectKind::MaterialSource,
                     .area = {.x = 0, .y = 1},
                     .tool = HAMMER});
  World world(objects);
  auto config = tuning();
  config.collection_distance = 3;
  config.sources.push_back({.id = TOOL_SOURCE});
  config.sources.push_back({.id = ObjectId{32}});
  Exploration exploration(simulation, world, config);
  const auto ladder_state = [&] {
    const auto snapshot = world.snapshot();
    return std::ranges::find(snapshot.objects, LADDER, &ObjectSnapshot::id)
        ->state;
  };
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.enter(Actor::Appraiser), Result::Applied);
  ASSERT_EQ(exploration.advance({}, IDLE, AcidLevel{1}), Result::Applied);
  ASSERT_EQ(ladder_state(), ObjectState::Destroyed);

  EXPECT_EQ(exploration.repair(Actor::Chef, LADDER, WRENCH),
            RepairResult::ToolNotCarried);
  ASSERT_EQ(exploration.collect(Actor::Appraiser, TOOL_SOURCE),
            CollectionResult::Applied);
  EXPECT_EQ(exploration.repair(Actor::Chef, LADDER, WRENCH),
            RepairResult::ToolNotCarried);
  ASSERT_EQ(exploration.collect(Actor::Chef, ObjectId{32}),
            CollectionResult::Applied);
  EXPECT_EQ(exploration.repair(Actor::Chef, LADDER, HAMMER),
            RepairResult::WrongTool);
  EXPECT_EQ(ladder_state(), ObjectState::Destroyed);

  EXPECT_EQ(exploration.repair(Actor::Appraiser, LADDER, WRENCH),
            RepairResult::Applied);
  EXPECT_EQ(ladder_state(), ObjectState::Intact);
  EXPECT_EQ(simulation.snapshot().appraiser.tools,
            (std::vector<ToolId>{WRENCH}));
}

TEST(Exploration, RepairIsRefusedFromBaseOutOfRangeAndForUnknownObjects) {
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(toolbox_city());
  Exploration exploration(simulation, world, toolbox_tuning());
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.collect(Actor::Chef, TOOL_SOURCE),
            CollectionResult::Applied);
  ASSERT_EQ(exploration.advance({}, IDLE, AcidLevel{1}), Result::Applied);
  EXPECT_EQ(exploration.repair(Actor::Chef, ObjectId{99}, WRENCH),
            RepairResult::UnknownObject);
  EXPECT_EQ(exploration.repair(Actor::Chef, LADDER, WRENCH),
            RepairResult::OutOfRange);
  EXPECT_EQ(exploration.repair(Actor::Appraiser, LADDER, WRENCH),
            RepairResult::NotExploring);
  ASSERT_EQ(exploration.advance(200ms, WALK, AcidLevel{1}), Result::Applied);
  ASSERT_EQ(at(exploration.snapshot().chef), (GridPosition{.x = 2}));
  EXPECT_EQ(exploration.repair(Actor::Chef, LADDER, WRENCH),
            RepairResult::Applied);
}

TEST(Exploration, AdvanceIsRefusedAfterEscapeAndLeavesTheWorld) {
  constexpr ObjectId WEAK{30};
  constexpr AcidLevel ACID{10};
  auto objects = city();
  objects.push_back({.id = WEAK,
                     .kind = ObjectKind::Npc,
                     .area = {.x = 6},
                     .destroyed_after = WorldThreshold{.acid = ACID}});
  const auto state_of_weak = [](const World &world) {
    return world.snapshot().objects.back().state;
  };

  // The control: the same call in a run that stayed destroys the object.
  Simulation stayed({.exploration_budget = 5s, .seed = 1});
  World stayed_world(objects);
  Exploration stayed_exploration(stayed, stayed_world, tuning());
  ASSERT_EQ(stayed.finish_day(ACID), Result::Applied);
  ASSERT_EQ(state_of_weak(stayed_world), ObjectState::Intact);
  EXPECT_EQ(stayed_exploration.advance(1s, IDLE, ACID), Result::Applied);
  EXPECT_EQ(state_of_weak(stayed_world), ObjectState::Destroyed);

  Simulation escaped({.exploration_budget = 5s, .seed = 1});
  World escaped_world(objects);
  Exploration escaped_exploration(escaped, escaped_world, tuning());
  ASSERT_EQ(escaped.finish_day(ACID), Result::Applied);
  ASSERT_EQ(escaped.escape(), Result::Applied);
  const auto before = escaped_world.snapshot();
  EXPECT_EQ(escaped_exploration.advance(1s, IDLE, ACID), Result::RunOver);
  EXPECT_EQ(escaped_world.snapshot(), before);
  EXPECT_EQ(state_of_weak(escaped_world), ObjectState::Intact);
}

TEST(Exploration, ACollectionHoldsTheActorWhileTheSharedClockRunsOn) {
  auto settings = tuning();
  settings.sources = {{.id = SOURCE, .remaining = 5}};
  settings.collection_time = 2s;
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(city());
  Exploration exploration(simulation, world, settings);
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Applied);
  EXPECT_EQ(simulation.snapshot().chef.inventory.size(), 1U);
  EXPECT_EQ(simulation.snapshot().remaining_exploration, 5s);
  EXPECT_EQ(world.snapshot().progression.elapsed, 0us);
  EXPECT_EQ(exploration.snapshot().chef.collecting,
            (Collecting{.source = SOURCE, .remaining = 2s}));

  const auto held_at = exploration.snapshot().chef.body.position;
  ASSERT_EQ(exploration.advance(1s, WALK, {}), Result::Applied);
  EXPECT_EQ(exploration.snapshot().chef.body.position, held_at);
  EXPECT_EQ(simulation.snapshot().remaining_exploration, 4s);
  EXPECT_EQ(world.snapshot().progression.elapsed, 1s);
  EXPECT_EQ(exploration.snapshot().chef.collecting,
            (Collecting{.source = SOURCE, .remaining = 1s}));

  ASSERT_EQ(exploration.advance(1s, WALK, {}), Result::Applied);
  EXPECT_EQ(exploration.snapshot().chef.collecting, std::nullopt);
  ASSERT_EQ(exploration.advance(100ms, WALK, {}), Result::Applied);
  EXPECT_GT(exploration.snapshot().chef.body.position.x, held_at.x);
}

TEST(Exploration, ACollectingActorTakesNoOtherCommandUntilItIsDone) {
  auto settings = toolbox_tuning();
  settings.collection_time = 1s;
  settings.sources.at(0).remaining = 3;
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(toolbox_city());
  Exploration exploration(simulation, world, settings);
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.collect(Actor::Chef, TOOL_SOURCE),
            CollectionResult::Applied);
  ASSERT_EQ(exploration.advance(1s, IDLE, AcidLevel{1}), Result::Applied);
  ASSERT_EQ(exploration.snapshot().chef.collecting, std::nullopt);
  ASSERT_EQ(exploration.advance(200ms, WALK, AcidLevel{1}), Result::Applied);
  ASSERT_EQ(at(exploration.snapshot().chef), (GridPosition{.x = 2}));
  ASSERT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Applied);

  EXPECT_EQ(exploration.collect(Actor::Chef, SOURCE), CollectionResult::Busy);
  EXPECT_EQ(exploration.repair(Actor::Chef, LADDER, WRENCH),
            RepairResult::Busy);
  EXPECT_EQ(exploration.return_to_base(Actor::Chef), Result::Busy);
  EXPECT_EQ(simulation.snapshot().chef.inventory.size(), 1U);
  EXPECT_EQ(simulation.snapshot().chef.location, Location::City);

  ASSERT_EQ(exploration.advance(1s, IDLE, AcidLevel{1}), Result::Applied);
  EXPECT_EQ(exploration.repair(Actor::Chef, LADDER, WRENCH),
            RepairResult::Applied);
  EXPECT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Applied);
  EXPECT_EQ(simulation.snapshot().chef.inventory.size(), 2U);
}

TEST(Exploration, BothActorsCollectAtOnceAndTheClockRunsOnce) {
  auto settings = tuning();
  settings.sources = {{.id = SOURCE, .remaining = 5}};
  settings.collection_time = 2s;
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(city());
  Exploration exploration(simulation, world, settings);
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.enter(Actor::Appraiser), Result::Applied);
  ASSERT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Applied);
  ASSERT_EQ(exploration.collect(Actor::Appraiser, SOURCE),
            CollectionResult::Applied);
  ASSERT_EQ(exploration.advance(2s, IDLE, {}), Result::Applied);
  EXPECT_EQ(simulation.snapshot().remaining_exploration, 3s);
  EXPECT_EQ(exploration.snapshot().chef.collecting, std::nullopt);
  EXPECT_EQ(exploration.snapshot().appraiser.collecting, std::nullopt);
  EXPECT_EQ(exploration.snapshot().sources.at(0).remaining, 3U);
}

TEST(Exploration, ARefusedCollectionHoldsNobody) {
  auto settings = tuning();
  settings.sources = {{.id = SOURCE, .remaining = 1}};
  settings.collection_time = 2s;
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(city());
  Exploration exploration(simulation, world, settings);
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.enter(Actor::Appraiser), Result::Applied);
  ASSERT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Applied);
  EXPECT_EQ(exploration.collect(Actor::Appraiser, SOURCE),
            CollectionResult::Depleted);
  EXPECT_EQ(exploration.snapshot().appraiser.collecting, std::nullopt);
  EXPECT_TRUE(exploration.snapshot().chef.collecting.has_value());
}

TEST(Exploration, ExpiryEndsACollectionAndKeepsItsItem) {
  auto settings = tuning();
  settings.sources = {{.id = SOURCE, .remaining = 5}};
  settings.collection_time = 8s;
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(city());
  Exploration exploration(simulation, world, settings);
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Applied);
  ASSERT_EQ(exploration.advance(5s, IDLE, {}), Result::Applied);
  const auto snapshot = simulation.snapshot();
  EXPECT_EQ(snapshot.phase, Phase::Workshop);
  EXPECT_EQ(snapshot.chef.location, Location::Base);
  EXPECT_EQ(snapshot.chef.inventory.size(), 1U);
  EXPECT_EQ(exploration.snapshot().chef.collecting, std::nullopt);
  EXPECT_EQ(world.snapshot().progression.elapsed, 5s);
}

TEST(Exploration, ExhaustionEndsACollectionAndKeepsItsItem) {
  auto settings = tuning();
  settings.sources = {{.id = SOURCE, .remaining = 5}};
  settings.collection_time = 2s;
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(city());
  Exploration exploration(simulation, world, settings);
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Applied);
  for (int hit = 0; hit < 3; ++hit) {
    ASSERT_EQ(exploration.damage(Actor::Chef), Result::Applied);
  }
  EXPECT_EQ(simulation.snapshot().chef.location, Location::Base);
  EXPECT_EQ(simulation.snapshot().chef.inventory.size(), 1U);
  EXPECT_EQ(exploration.snapshot().chef.collecting, std::nullopt);
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  EXPECT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Applied);
}

TEST(Exploration, AFallEndsACollectionAtBaseAndKeepsItsItem) {
  auto definitions = city();
  definitions.front().destroyed_after = WorldThreshold{.elapsed = 150ms};
  auto settings = tuning();
  settings.sources = {{.id = SOURCE, .remaining = 5}};
  settings.collection_time = 2s;
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(definitions);
  Exploration exploration(simulation, world, settings);
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Applied);
  ASSERT_EQ(exploration.advance(150ms, IDLE, {}), Result::Applied);
  ASSERT_EQ(world.snapshot().objects.front().state, ObjectState::Destroyed);
  ASSERT_TRUE(exploration.snapshot().chef.collecting.has_value());
  ASSERT_EQ(simulation.snapshot().chef.location, Location::City);

  ASSERT_EQ(exploration.advance(1s, IDLE, {}), Result::Applied);
  EXPECT_EQ(simulation.snapshot().chef.location, Location::Base);
  EXPECT_EQ(at(exploration.snapshot().chef), BASE);
  EXPECT_EQ(simulation.snapshot().chef.inventory,
            (std::vector{IngredientId{70}}));
  EXPECT_EQ(exploration.snapshot().chef.collecting, std::nullopt);
}

TEST(Exploration, AFinishedDaysUnspentTimeReachesTheWorldOnceFromBase) {
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(city());
  Exploration exploration(simulation, world, tuning());
  ASSERT_EQ(exploration.advance(0us, IDLE, AcidLevel{}), Result::Applied);
  ASSERT_EQ(world.snapshot().progression.elapsed, 0us);
  ASSERT_EQ(simulation.finish_day(AcidLevel{}), Result::Applied);
  ASSERT_EQ(exploration.advance(1s, IDLE, AcidLevel{}), Result::Applied);
  EXPECT_EQ(world.snapshot().progression.elapsed, 5s);
  ASSERT_EQ(exploration.advance(1s, IDLE, AcidLevel{}), Result::Applied);
  EXPECT_EQ(world.snapshot().progression.elapsed, 5s);
}

TEST(Exploration, ACollectionTimeBelowZeroIsRefused) {
  auto settings = tuning();
  settings.collection_time = -1us;
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(city());
  EXPECT_THROW(Exploration(simulation, world, settings), std::invalid_argument);
  settings.collection_time = 0us;
  EXPECT_NO_THROW(Exploration(simulation, world, settings));
}

TEST(Exploration, ADestroyedSourceKeepsItsStockOutOfReach) {
  auto objects = city();
  objects.at(1).destroyed_after = WorldThreshold{.acid = AcidLevel{5}};
  auto settings = tuning();
  settings.sources = {{.id = SOURCE, .remaining = 3}};
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(objects);
  Exploration exploration(simulation, world, settings);
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Applied);
  ASSERT_EQ(exploration.advance(100ms, IDLE, AcidLevel{5}), Result::Applied);
  EXPECT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Unavailable);
  EXPECT_EQ(simulation.snapshot().chef.inventory.size(), 1U);
  EXPECT_EQ(exploration.snapshot().sources.at(0).remaining, 2U);
}

} // namespace
} // namespace neocity::survival
