#include <survival/exploration.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
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
          .exits = {},
          .interiors = {},
          .collection_distance = 1,
          .sources = {{.id = SOURCE}},
          .obstacles = {},
          .patrols = {}};
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

/// Finishes days, with the acid unchanged, until `day`, then lets the world
/// follow. Everyone is at base, as a day only ends there.
void skip_to_day(Simulation &simulation, Exploration &exploration,
                 std::uint32_t day) {
  while (simulation.snapshot().day < day) {
    if (simulation.finish_day({}) != Result::Applied) {
      throw std::logic_error("course day refused");
    }
  }
  if (exploration.advance({}, {}) != Result::Applied) {
    throw std::logic_error("course day not applied");
  }
}

/// Exploration over `objects` entered by the chef at `entry` on `day`.
class Course {
public:
  Course(const std::vector<ObjectDefinition> &objects, GridPosition entry,
         std::uint32_t day = 1)
      : simulation({.exploration_budget = 30s, .seed = 0}), world(objects),
        exploration(simulation, world,
                    {.base = BASE,
                     .entry = entry,
                     .exits = {},
                     .interiors = {},
                     .sources = {},
                     .obstacles = {},
                     .patrols = {}}) {
    skip_to_day(simulation, exploration, day);
    if (exploration.enter(Actor::Chef) != Result::Applied) {
      throw std::logic_error("course entry refused");
    }
  }

  /// Advances with the chef pressing `input` and the appraiser at base.
  void press(std::chrono::milliseconds elapsed, ExplorationInput input) {
    if (exploration.advance(elapsed, {input, {}}) != Result::Applied) {
      throw std::logic_error("course advance refused");
    }
  }

  [[nodiscard]] ExplorerSnapshot chef() const {
    return exploration.snapshot().chef;
  }
  [[nodiscard]] Location location() const {
    return simulation.snapshot().chef.location;
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

constexpr ExplorationInput RUN_RIGHT{.direction = Direction::Right,
                                     .running = true};

TEST(Exploration, RunningNeverTires) {
  Course floor({floor_at(FLOOR, 0, 0, 600)}, ENTRY);
  const auto run_ten_units = [&floor] {
    const auto before = floor.chef().body.position.x;
    for (int unit = 0; unit < 10; ++unit) {
      floor.press(100ms, RUN_RIGHT);
    }
    return floor.chef().body.position.x - before;
  };
  const auto first = run_ten_units();
  ASSERT_GT(first, 10);
  for (int unit = 0; unit < 180; ++unit) {
    floor.press(100ms, RUN_RIGHT);
  }
  EXPECT_EQ(run_ten_units(), first);
}

TEST(Exploration, BothRolesMoveIdenticallyAndShareOneClock) {
  Simulation simulation({.exploration_budget = 5s, .seed = 9});
  World world(city());
  Exploration exploration(simulation, world, tuning());
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.enter(Actor::Appraiser), Result::Applied);
  ASSERT_EQ(exploration.advance(200ms, WALK), Result::Applied);
  EXPECT_EQ(at(exploration.snapshot().chef), (GridPosition{.x = 2}));
  EXPECT_EQ(exploration.snapshot().chef, exploration.snapshot().appraiser);
  EXPECT_EQ(simulation.snapshot().remaining_exploration, 4800ms);
  EXPECT_EQ(simulation.snapshot().world_time, 200ms);
  EXPECT_EQ(simulation.snapshot().acid, AcidLevel{});
}

TEST(Exploration,
     ShortFramesConsumeTimeImmediatelyAndRetainMovementForEachActor) {
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(city());
  Exploration exploration(simulation, world, tuning());
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.advance(60ms, WALK), Result::Applied);
  EXPECT_EQ(simulation.snapshot().remaining_exploration, 4940ms);
  EXPECT_EQ(simulation.snapshot().world_time, 60ms);
  EXPECT_EQ(at(exploration.snapshot().chef), ENTRY);
  ASSERT_EQ(exploration.enter(Actor::Appraiser), Result::Applied);
  ASSERT_EQ(exploration.advance(40ms, WALK), Result::Applied);
  EXPECT_EQ(at(exploration.snapshot().chef), (GridPosition{.x = 1}));
  EXPECT_EQ(at(exploration.snapshot().appraiser), ENTRY);
  ASSERT_EQ(exploration.advance(60ms, WALK), Result::Applied);
  EXPECT_EQ(at(exploration.snapshot().appraiser), (GridPosition{.x = 1}));
}

TEST(Exploration, BuildingsAndBrokenEntrancesBlockButABrokenLadderDoesNot) {
  for (const auto kind :
       {ObjectKind::Building, ObjectKind::Ladder, ObjectKind::Entrance}) {
    for (const bool fallen : {false, true}) {
      auto definitions = city();
      definitions.push_back({.id = OWNER,
                             .kind = kind,
                             .area = {.x = 1},
                             .destroyed_from_day = 2});
      Simulation simulation({.exploration_budget = 5s, .seed = 0});
      World world(definitions);
      Exploration exploration(simulation, world, tuning());
      skip_to_day(simulation, exploration, fallen ? 2 : 1);
      ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
      ASSERT_EQ(exploration.advance(100ms, RUN), Result::Applied);
      const auto blocks = (kind == ObjectKind::Building && !fallen) ||
                          (kind == ObjectKind::Entrance && fallen);
      EXPECT_EQ(at(exploration.snapshot().chef),
                blocks ? ENTRY : GridPosition{.x = 2});
    }
  }
}

TEST(Exploration,
     AnIntactPassageOverridesABuildingAndABrokenOneDoesNotInEitherOrder) {
  for (const auto kind : {ObjectKind::Ladder, ObjectKind::Entrance}) {
    for (const bool reversed : {false, true}) {
      for (const bool fallen : {false, true}) {
        auto definitions = city();
        definitions.push_back(
            {.id = OWNER, .kind = ObjectKind::Building, .area = {.x = 1}});
        definitions.push_back({.id = ObjectId{21},
                               .kind = kind,
                               .area = {.x = 1},
                               .destroyed_from_day = 2});
        if (reversed) {
          std::ranges::reverse(definitions);
        }
        Simulation simulation({.exploration_budget = 5s, .seed = 0});
        World world(definitions);
        Exploration exploration(simulation, world, tuning());
        skip_to_day(simulation, exploration, fallen ? 2 : 1);
        ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
        ASSERT_EQ(exploration.advance(100ms, RUN), Result::Applied);
        EXPECT_EQ(at(exploration.snapshot().chef),
                  fallen ? ENTRY : GridPosition{.x = 2});
      }
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
                    .destroyed_from_day = 2}},
                  ENTRY, fallen ? 2 : 1);
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
                    .destroyed_from_day = 2}},
                  {.x = 2, .y = 4}, broken ? 2 : 1);
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
  ASSERT_EQ(exploration.advance(1s, RUN), Result::Applied);
  EXPECT_EQ(simulation.snapshot().chef.location, Location::Base);
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
  ASSERT_EQ(exploration.advance(50ms, WALK), Result::Applied);
  EXPECT_EQ(at(exploration.snapshot().chef), ENTRY);
}

TEST(Exploration, ManualReturnClearsPositionAndPartialMovementOnly) {
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(city());
  Exploration exploration(simulation, world, tuning());
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.enter(Actor::Appraiser), Result::Applied);
  ASSERT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Applied);
  ASSERT_EQ(exploration.advance(150ms, WALK), Result::Applied);
  EXPECT_EQ(at(exploration.snapshot().chef), (GridPosition{.x = 1}));
  const auto persistent_world = world.snapshot();
  ASSERT_EQ(exploration.return_to_base(Actor::Chef), Result::Applied);
  EXPECT_EQ(at(exploration.snapshot().chef), BASE);
  EXPECT_EQ(simulation.snapshot().chef.inventory,
            (std::vector{IngredientId{70}}));
  EXPECT_EQ(simulation.snapshot().appraiser.location, Location::City);
  EXPECT_EQ(simulation.snapshot().remaining_exploration, 4850ms);
  EXPECT_EQ(world.snapshot(), persistent_world);
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.advance(50ms, WALK), Result::Applied);
  EXPECT_EQ(at(exploration.snapshot().chef), ENTRY);
  EXPECT_EQ(at(exploration.snapshot().appraiser), (GridPosition{.x = 2}));
  EXPECT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Depleted);
}

TEST(Exploration, ExpiryStopsBothActorsBeforeMovementOrCollectionBeyondBudget) {
  Simulation simulation({.exploration_budget = 150ms, .seed = 0});
  World world(city());
  Exploration exploration(simulation, world, tuning());
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.enter(Actor::Appraiser), Result::Applied);
  ASSERT_EQ(exploration.advance(100ms, WALK), Result::Applied);
  EXPECT_EQ(at(exploration.snapshot().chef), (GridPosition{.x = 1}));
  ASSERT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Applied);
  ASSERT_EQ(exploration.advance(10s, RUN), Result::Applied);
  EXPECT_EQ(at(exploration.snapshot().chef), BASE);
  EXPECT_EQ(at(exploration.snapshot().appraiser), BASE);
  EXPECT_EQ(simulation.snapshot().remaining_exploration, 0us);
  EXPECT_EQ(simulation.snapshot().phase, Phase::Workshop);
  EXPECT_EQ(simulation.snapshot().world_time, 150ms);
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
  ASSERT_EQ(exploration.advance(400ms, WALK), Result::Applied);
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
  ASSERT_EQ(exploration.advance(100ms, left), Result::Applied);
  EXPECT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Applied);
}

TEST(Exploration, SupplyTracksDormantDestroyedAndOwnerDestroyedWorldObjects) {
  for (const bool destroy_owner : {false, true}) {
    auto definitions = city();
    definitions.back().exposed_from_day = 2;
    if (destroy_owner) {
      definitions.push_back(
          {.id = OWNER, .kind = ObjectKind::Npc, .destroyed_from_day = 3});
      definitions.at(1).supported_by = OWNER;
    } else {
      definitions.back().destroyed_from_day = 3;
    }
    auto config = tuning();
    config.sources.front().remaining = 2;
    Simulation simulation({.exploration_budget = 5s, .seed = 0});
    World world(definitions);
    Exploration exploration(simulation, world, config);
    ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
    EXPECT_EQ(exploration.collect(Actor::Chef, SOURCE),
              CollectionResult::Unavailable);
    ASSERT_EQ(exploration.return_to_base(Actor::Chef), Result::Applied);
    skip_to_day(simulation, exploration, 2);
    ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
    ASSERT_EQ(exploration.collect(Actor::Chef, SOURCE),
              CollectionResult::Applied);
    ASSERT_EQ(exploration.return_to_base(Actor::Chef), Result::Applied);
    skip_to_day(simulation, exploration, 3);
    ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
    const auto before = simulation.snapshot();
    EXPECT_EQ(exploration.collect(Actor::Chef, SOURCE),
              CollectionResult::Unavailable);
    EXPECT_EQ(simulation.snapshot(), before);
    EXPECT_EQ(exploration.snapshot().sources.front().remaining, 1U);
  }
}

TEST(Exploration, AFloorGoneOnItsDayRefusesEntryAndAcidNeverMovesTheMap) {
  auto definitions = city();
  definitions.front().destroyed_from_day = 2;
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(definitions);
  Exploration exploration(simulation, world, tuning());
  ASSERT_EQ(simulation.finish_day(AcidLevel{3}), Result::Applied);
  ASSERT_EQ(exploration.advance(0us, IDLE), Result::Applied);
  EXPECT_EQ(world.snapshot().objects.front().state, ObjectState::Destroyed);
  EXPECT_EQ(exploration.enter(Actor::Chef), Result::NotExploring);

  const auto acid_only = city();
  Simulation acid_simulation({.exploration_budget = 5s, .seed = 0});
  World acid_world(acid_only);
  Exploration acid_exploration(acid_simulation, acid_world, tuning());
  const auto before = acid_world.snapshot().objects;
  ASSERT_EQ(acid_simulation.finish_day(AcidLevel{3}), Result::Applied);
  ASSERT_EQ(acid_exploration.advance(0us, IDLE), Result::Applied);
  EXPECT_EQ(acid_simulation.snapshot().acid, AcidLevel{3});
  EXPECT_EQ(acid_world.snapshot().objects, before);
  EXPECT_EQ(acid_exploration.enter(Actor::Chef), Result::Applied);
}

TEST(Exploration, NegativeElapsedAndRefusedBaseCommandsChangeNothing) {
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(city());
  Exploration exploration(simulation, world, tuning());
  EXPECT_EQ(exploration.return_to_base(Actor::Chef), Result::NotExploring);
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.advance(50ms, WALK), Result::Applied);
  const auto before = simulation.snapshot();
  const auto before_world = world.snapshot();
  const auto before_exploration = exploration.snapshot();
  EXPECT_EQ(exploration.advance(-1us, RUN), Result::InvalidElapsed);
  EXPECT_EQ(simulation.snapshot(), before);
  EXPECT_EQ(world.snapshot(), before_world);
  EXPECT_EQ(exploration.snapshot(), before_exploration);
  ASSERT_EQ(exploration.advance(50ms, WALK), Result::Applied);
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
  config.entry = BASE;
  EXPECT_THROW(Exploration(simulation, world, config), std::invalid_argument);
  EXPECT_EQ(simulation.snapshot().phase, Phase::Workshop);
}

constexpr ObjectId SECOND_SOURCE{30};

/// The city with a second ingredient source beside the first.
std::vector<ObjectDefinition> two_source_city() {
  auto objects = city();
  objects.push_back({.id = SECOND_SOURCE,
                     .kind = ObjectKind::MaterialSource,
                     .area = {.x = 1},
                     .ingredient = IngredientId{71}});
  return objects;
}

ExplorationConfig two_source_tuning() {
  auto config = tuning();
  config.sources.push_back({.id = SECOND_SOURCE, .remaining = 2});
  return config;
}

TEST(Exploration, AdvanceIsRefusedAfterEscapeAndLeavesTheWorld) {
  constexpr ObjectId WEAK{30};
  auto objects = city();
  objects.push_back({.id = WEAK,
                     .kind = ObjectKind::Npc,
                     .area = {.x = 6},
                     .destroyed_from_day = 2});
  const auto state_of_weak = [](const World &world) {
    return world.snapshot().objects.back().state;
  };

  // The control: the same call in a run that stayed destroys the object.
  Simulation stayed({.exploration_budget = 5s, .seed = 1});
  World stayed_world(objects);
  Exploration stayed_exploration(stayed, stayed_world, tuning());
  ASSERT_EQ(stayed.finish_day({}), Result::Applied);
  ASSERT_EQ(state_of_weak(stayed_world), ObjectState::Intact);
  EXPECT_EQ(stayed_exploration.advance(1s, IDLE), Result::Applied);
  EXPECT_EQ(state_of_weak(stayed_world), ObjectState::Destroyed);

  Simulation escaped({.exploration_budget = 5s, .seed = 1});
  World escaped_world(objects);
  Exploration escaped_exploration(escaped, escaped_world, tuning());
  ASSERT_EQ(escaped.finish_day({}), Result::Applied);
  ASSERT_EQ(escaped.escape(), Result::Applied);
  const auto before = escaped_world.snapshot();
  EXPECT_EQ(escaped_exploration.advance(1s, IDLE), Result::RunOver);
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
  EXPECT_EQ(simulation.snapshot().world_time, 0us);
  EXPECT_EQ(exploration.snapshot().chef.collecting,
            (Collecting{.source = SOURCE, .remaining = 2s}));

  const auto held_at = exploration.snapshot().chef.body.position;
  ASSERT_EQ(exploration.advance(1s, WALK), Result::Applied);
  EXPECT_EQ(exploration.snapshot().chef.body.position, held_at);
  EXPECT_EQ(simulation.snapshot().remaining_exploration, 4s);
  EXPECT_EQ(simulation.snapshot().world_time, 1s);
  EXPECT_EQ(exploration.snapshot().chef.collecting,
            (Collecting{.source = SOURCE, .remaining = 1s}));

  ASSERT_EQ(exploration.advance(1s, WALK), Result::Applied);
  EXPECT_EQ(exploration.snapshot().chef.collecting, std::nullopt);
  ASSERT_EQ(exploration.advance(100ms, WALK), Result::Applied);
  EXPECT_GT(exploration.snapshot().chef.body.position.x, held_at.x);
}

TEST(Exploration, ACollectingActorTakesNoOtherCommandUntilItIsDone) {
  auto settings = two_source_tuning();
  settings.collection_time = 1s;
  settings.sources.at(0).remaining = 3;
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(two_source_city());
  Exploration exploration(simulation, world, settings);
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.collect(Actor::Chef, SECOND_SOURCE),
            CollectionResult::Applied);
  ASSERT_EQ(exploration.advance(1s, IDLE), Result::Applied);
  ASSERT_EQ(exploration.snapshot().chef.collecting, std::nullopt);
  ASSERT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Applied);

  EXPECT_EQ(exploration.collect(Actor::Chef, SOURCE), CollectionResult::Busy);
  EXPECT_EQ(exploration.return_to_base(Actor::Chef), Result::Busy);
  EXPECT_EQ(simulation.snapshot().chef.inventory.size(), 2U);
  EXPECT_EQ(simulation.snapshot().chef.location, Location::City);

  ASSERT_EQ(exploration.advance(1s, IDLE), Result::Applied);
  EXPECT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Applied);
  EXPECT_EQ(simulation.snapshot().chef.inventory.size(), 3U);
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
  ASSERT_EQ(exploration.advance(2s, IDLE), Result::Applied);
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
  ASSERT_EQ(exploration.advance(5s, IDLE), Result::Applied);
  const auto snapshot = simulation.snapshot();
  EXPECT_EQ(snapshot.phase, Phase::Workshop);
  EXPECT_EQ(snapshot.chef.location, Location::Base);
  EXPECT_EQ(snapshot.chef.inventory.size(), 1U);
  EXPECT_EQ(exploration.snapshot().chef.collecting, std::nullopt);
  EXPECT_EQ(simulation.snapshot().world_time, 5s);
}

TEST(Exploration, TheWorldFollowsAFinishedDayOnceFromBase) {
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(city());
  Exploration exploration(simulation, world, tuning());
  ASSERT_EQ(exploration.advance(0us, IDLE), Result::Applied);
  ASSERT_EQ(world.snapshot().day, 1U);
  ASSERT_EQ(simulation.finish_day({}), Result::Applied);
  ASSERT_EQ(exploration.advance(1s, IDLE), Result::Applied);
  EXPECT_EQ(world.snapshot().day, 2U);
  ASSERT_EQ(exploration.advance(1s, IDLE), Result::Applied);
  EXPECT_EQ(world.snapshot().day, 2U);
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
  objects.at(1).destroyed_from_day = 2;
  auto settings = tuning();
  settings.sources = {{.id = SOURCE, .remaining = 3}};
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(objects);
  Exploration exploration(simulation, world, settings);
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Applied);
  ASSERT_EQ(exploration.return_to_base(Actor::Chef), Result::Applied);
  skip_to_day(simulation, exploration, 2);
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  EXPECT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Unavailable);
  EXPECT_EQ(simulation.snapshot().chef.inventory.size(), 1U);
  EXPECT_EQ(exploration.snapshot().sources.at(0).remaining, 2U);
}

constexpr ObjectId HALL_FLOOR{2};
constexpr ObjectId SHOP{3};
constexpr ObjectId HALL_SOURCE{11};
constexpr GridPosition HALL_ARRIVAL{.x = 2, .y = 0};
constexpr WorldArea EXIT_ZONE{.x = 15, .y = 0, .width = 3, .height = 1};

/// A street floor with an exit zone at its east end to an aquarium floor, and a
/// shop with an inside; both actors may be sent out together.
class Travel {
public:
  Travel()
      : simulation({.exploration_budget = 600s, .seed = 0}), world(objects()),
        exploration(simulation, world, config()) {}

  static std::vector<ObjectDefinition> objects() {
    return {floor_at(FLOOR, 0, 0, 20),
            {.id = SHOP,
             .kind = ObjectKind::Building,
             .area = {.x = 4, .y = -2, .width = 5, .height = 2}},
            {.id = HALL_FLOOR,
             .area = {.x = 0, .y = 0, .width = 10},
             .map = MapId::Aquarium},
            {.id = HALL_SOURCE,
             .kind = ObjectKind::MaterialSource,
             .area = {.x = 3},
             .map = MapId::Aquarium,
             .ingredient = IngredientId{70}}};
  }

  static ExplorationConfig config() {
    return {.base = BASE,
            .entry = ENTRY,
            .exits = {{.from = MapId::Street,
                       .zone = EXIT_ZONE,
                       .to = MapId::Aquarium,
                       .arrival = HALL_ARRIVAL}},
            .interiors = {{.building = SHOP,
                           .area = {.x = 5, .y = 0, .width = 2, .height = 1}}},
            .sources = {{.id = HALL_SOURCE}},
            .obstacles = {},
            .patrols = {}};
  }

  void both_enter() {
    ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
    ASSERT_EQ(exploration.enter(Actor::Appraiser), Result::Applied);
  }

  /// Walks `actor` east, the other idle, until it stands at `x` or past or
  /// the party changes map.
  void walk_to(Actor actor, std::int32_t x) {
    std::array<ExplorationInput, 2> inputs{};
    inputs.at(actor == Actor::Chef ? 0 : 1) = RIGHT;
    const auto map = exploration.snapshot().map;
    for (int tick = 0; tick < 400 && at(explorer(actor)).x < x &&
                       exploration.snapshot().map == map;
         ++tick) {
      ASSERT_EQ(exploration.advance(50ms, inputs), Result::Applied);
    }
    ASSERT_TRUE(at(explorer(actor)).x >= x ||
                exploration.snapshot().map != map);
  }

  /// Walks both actors east side by side until the party changes map or the
  /// chef passes `x`.
  void walk_together_to(std::int32_t x) {
    const std::array<ExplorationInput, 2> inputs{RIGHT, RIGHT};
    const auto map = exploration.snapshot().map;
    for (int tick = 0; tick < 400 && at(explorer(Actor::Chef)).x < x &&
                       exploration.snapshot().map == map;
         ++tick) {
      ASSERT_EQ(exploration.advance(50ms, inputs), Result::Applied);
    }
  }

  /// Presses east for `actor` alone for `ticks` steps of 50ms, the other idle.
  void push_east(Actor actor, int ticks) {
    std::array<ExplorationInput, 2> inputs{};
    inputs.at(actor == Actor::Chef ? 0 : 1) = RIGHT;
    for (int tick = 0; tick < ticks; ++tick) {
      ASSERT_EQ(exploration.advance(50ms, inputs), Result::Applied);
    }
  }

  [[nodiscard]] ExplorerSnapshot explorer(Actor actor) const {
    const auto snapshot = exploration.snapshot();
    return actor == Actor::Chef ? snapshot.chef : snapshot.appraiser;
  }

  [[nodiscard]] Simulation &sim() { return simulation; }
  [[nodiscard]] Exploration &exploring() { return exploration; }

private:
  Simulation simulation;
  World world;
  Exploration exploration;
};

TEST(Exploration, OneActorWalkingToAnExitStopsAtTheScreenLock) {
  Travel travel;
  travel.both_enter();
  ASSERT_EQ(travel.exploring().snapshot().map, MapId::Street);
  ASSERT_GT((EXIT_ZONE.x - ENTRY.x) * SUBCELLS, SCREEN_LOCK);
  travel.push_east(Actor::Chef, 400);
  const auto chef = travel.explorer(Actor::Chef).body.position.x;
  const auto appraiser = travel.explorer(Actor::Appraiser).body.position.x;
  EXPECT_LE(chef - appraiser, SCREEN_LOCK);
  EXPECT_GT(chef - appraiser, SCREEN_LOCK - PHYSICS.walk);
  EXPECT_LT(at(travel.explorer(Actor::Chef)).x, EXIT_ZONE.x);
  EXPECT_EQ(travel.exploring().snapshot().map, MapId::Street);
  EXPECT_EQ(at(travel.explorer(Actor::Appraiser)), ENTRY);
}

TEST(Exploration, AnActorStoppedByTheScreenLockMovesOnOnceTheOtherCatchesUp) {
  Travel travel;
  travel.both_enter();
  travel.push_east(Actor::Chef, 400);
  const auto stopped = travel.explorer(Actor::Chef).body.position.x;
  travel.push_east(Actor::Appraiser, 10);
  travel.push_east(Actor::Chef, 100);
  EXPECT_GT(travel.explorer(Actor::Chef).body.position.x, stopped);
  EXPECT_LE(travel.explorer(Actor::Chef).body.position.x -
                travel.explorer(Actor::Appraiser).body.position.x,
            SCREEN_LOCK);
}

TEST(Exploration, TheScreenLockStopsOnlyTheSidewaysMotion) {
  Travel travel;
  travel.both_enter();
  travel.push_east(Actor::Chef, 400);
  const std::array<ExplorationInput, 2> jump{JUMP_RIGHT, ExplorationInput{}};
  ASSERT_EQ(travel.exploring().advance(100ms, jump), Result::Applied);
  EXPECT_EQ(travel.sim().snapshot().chef.location, Location::City);
  EXPECT_LT(travel.explorer(Actor::Chef).body.position.y, 0);
  EXPECT_LE(travel.explorer(Actor::Chef).body.position.x -
                travel.explorer(Actor::Appraiser).body.position.x,
            SCREEN_LOCK);
}

TEST(Exploration, BothActorsInAnExitZoneChangeTheMapAndArriveTogether) {
  Travel travel;
  travel.both_enter();
  travel.walk_together_to(16);
  const auto snapshot = travel.exploring().snapshot();
  EXPECT_EQ(snapshot.map, MapId::Aquarium);
  EXPECT_EQ(at(snapshot.chef), HALL_ARRIVAL);
  EXPECT_EQ(at(snapshot.appraiser), HALL_ARRIVAL);
  EXPECT_EQ(travel.sim().snapshot().chef.location, Location::City);
  EXPECT_EQ(travel.sim().snapshot().appraiser.location, Location::City);
}

TEST(Exploration, ASourceOnAnotherMapIsOutOfRange) {
  Travel travel;
  travel.both_enter();
  EXPECT_EQ(travel.exploring().collect(Actor::Chef, HALL_SOURCE),
            CollectionResult::OutOfRange);
  travel.walk_together_to(16);
  ASSERT_EQ(travel.exploring().snapshot().map, MapId::Aquarium);
  EXPECT_EQ(travel.exploring().collect(Actor::Chef, HALL_SOURCE),
            CollectionResult::Applied);
}

TEST(Exploration, AnActorCannotEnterTheStreetWhileTheOtherIsInTheAquarium) {
  Travel travel;
  travel.both_enter();
  travel.walk_together_to(16);
  ASSERT_EQ(travel.exploring().return_to_base(Actor::Chef), Result::Applied);
  EXPECT_EQ(travel.exploring().enter(Actor::Chef), Result::NotExploring);
  ASSERT_EQ(travel.exploring().return_to_base(Actor::Appraiser),
            Result::Applied);
  EXPECT_EQ(travel.exploring().snapshot().map, MapId::Street);
  EXPECT_EQ(travel.exploring().enter(Actor::Chef), Result::Applied);
}

TEST(Exploration, TheSnapshotShowsWhichBuildingAnActorIsInside) {
  Travel travel;
  travel.both_enter();
  EXPECT_FALSE(travel.explorer(Actor::Chef).interior.has_value());
  travel.walk_to(Actor::Chef, 5);
  EXPECT_EQ(travel.explorer(Actor::Chef).interior, SHOP);
  EXPECT_FALSE(travel.explorer(Actor::Appraiser).interior.has_value());
  travel.walk_to(Actor::Chef, 8);
  EXPECT_FALSE(travel.explorer(Actor::Chef).interior.has_value());
}

TEST(Exploration, AnExitNeedsASafeArrivalOnADifferentMap) {
  auto bad_arrival = Travel::config();
  bad_arrival.exits.front().arrival = {.x = 40, .y = 0};
  Simulation first({.exploration_budget = 60s, .seed = 0});
  World first_world(Travel::objects());
  EXPECT_THROW(Exploration(first, first_world, bad_arrival),
               std::invalid_argument);
  auto same_map = Travel::config();
  same_map.exits.front().to = MapId::Street;
  Simulation second({.exploration_budget = 60s, .seed = 0});
  World second_world(Travel::objects());
  EXPECT_THROW(Exploration(second, second_world, same_map),
               std::invalid_argument);
}

/// A walkway from x = 0 to 59 with the party entering at x = 30, an obstacle
/// west of the entry, a source and an exit zone to an aquarium floor beside
/// that obstacle, and whatever mutant patrols the test adds. Time spent away
/// from the map is ten seconds.
class Danger {
public:
  static constexpr GridPosition WEST_ENTRY{.x = 30, .y = 0};
  static constexpr GridPosition OBSTACLE{.x = 20, .y = 0};
  static constexpr WorldArea WEST_EXIT{
      .x = 18, .y = 0, .width = 5, .height = 1};
  static constexpr ObjectId NEAR_SOURCE{12};
  static constexpr ObjectId AQUARIUM_FLOOR{13};
  static constexpr auto ABSENCE = 10s;

  explicit Danger(std::vector<Obstacle> obstacles = {{.cell = OBSTACLE}},
                  std::vector<Patrol> patrols = {})
      : simulation({.exploration_budget = 600s, .seed = 0}),
        world({floor_at(FLOOR, 0, 0, 60),
               {.id = NEAR_SOURCE,
                .kind = ObjectKind::MaterialSource,
                .area = {.x = 21},
                .ingredient = IngredientId{70}},
               {.id = AQUARIUM_FLOOR,
                .area = {.x = 0, .y = 0, .width = 10},
                .map = MapId::Aquarium}}),
        exploration(simulation, world,
                    {.base = BASE,
                     .entry = WEST_ENTRY,
                     .exits = {{.from = MapId::Street,
                                .zone = WEST_EXIT,
                                .to = MapId::Aquarium,
                                .arrival = {.x = 2, .y = 0}}},
                     .interiors = {},
                     .sources = {{.id = NEAR_SOURCE}},
                     .obstacles = std::move(obstacles),
                     .patrols = std::move(patrols),
                     .absence = ABSENCE}) {}

  void enter(Actor actor) {
    ASSERT_EQ(exploration.enter(actor), Result::Applied);
  }
  void both_enter() {
    enter(Actor::Chef);
    enter(Actor::Appraiser);
  }

  /// Presses `direction` for `actor` alone, the other idle, for one tick.
  void press(Actor actor, Direction direction) {
    std::array<ExplorationInput, 2> inputs{};
    inputs.at(actor == Actor::Chef ? 0 : 1) = {.direction = direction};
    ASSERT_EQ(exploration.advance(20ms, inputs), Result::Applied);
  }

  /// Idles `ticks` physics ticks.
  void idle(int ticks) {
    for (int tick = 0; tick < ticks; ++tick) {
      ASSERT_EQ(exploration.advance(20ms, IDLE), Result::Applied);
    }
  }

  /// Walks `actor` west until it is absent or the walkway has been crossed.
  void walk_west_until_absent(Actor actor) {
    for (int tick = 0; tick < 800 && !explorer(actor).absent; ++tick) {
      press(actor, Direction::Left);
    }
  }

  [[nodiscard]] ExplorerSnapshot explorer(Actor actor) const {
    const auto snapshot = exploration.snapshot();
    return actor == Actor::Chef ? snapshot.chef : snapshot.appraiser;
  }
  [[nodiscard]] Simulation &sim() { return simulation; }
  [[nodiscard]] Exploration &exploring() { return exploration; }

private:
  Simulation simulation;
  World world;
  Exploration exploration;
};

TEST(Exploration, AnActorThatTouchesAnObstacleIsAbsentAndReturnsAtItsPartner) {
  Danger danger;
  danger.both_enter();
  ASSERT_FALSE(danger.explorer(Actor::Chef).absent);
  danger.walk_west_until_absent(Actor::Chef);
  ASSERT_TRUE(danger.explorer(Actor::Chef).absent);
  EXPECT_EQ(danger.sim().snapshot().chef.location, Location::City);
  const auto away = danger.explorer(Actor::Chef).body.position;
  ASSERT_LT(away.x, danger.explorer(Actor::Appraiser).body.position.x);
  // 9.9 s later the actor is still away.
  danger.idle(495);
  EXPECT_TRUE(danger.explorer(Actor::Chef).absent);
  EXPECT_EQ(danger.explorer(Actor::Chef).body.position, away);
  // At 10 s it is beside the partner.
  danger.idle(5);
  EXPECT_FALSE(danger.explorer(Actor::Chef).absent);
  EXPECT_EQ(danger.explorer(Actor::Chef).body.position,
            danger.explorer(Actor::Appraiser).body.position);
  EXPECT_EQ(danger.sim().snapshot().chef.location, Location::City);
}

TEST(Exploration, AnActorAloneInTheCityThatTouchesAnObstacleGoesHome) {
  Danger danger;
  danger.enter(Actor::Chef);
  danger.walk_west_until_absent(Actor::Chef);
  EXPECT_FALSE(danger.explorer(Actor::Chef).absent);
  EXPECT_EQ(danger.sim().snapshot().chef.location, Location::Base);
}

TEST(Exploration, BothActorsTouchedGoHomeKeepingInventoryAndDay) {
  Danger danger;
  danger.both_enter();
  ASSERT_EQ(danger.sim().collect(Actor::Chef, IngredientId{70}),
            Result::Applied);
  ASSERT_EQ(danger.sim().collect(Actor::Appraiser, IngredientId{71}),
            Result::Applied);
  const auto before = danger.sim().snapshot();
  danger.walk_west_until_absent(Actor::Chef);
  ASSERT_TRUE(danger.explorer(Actor::Chef).absent);
  // The partner touches the same obstacle while the chef is away.
  danger.walk_west_until_absent(Actor::Appraiser);
  const auto after = danger.sim().snapshot();
  EXPECT_EQ(after.chef.location, Location::Base);
  EXPECT_EQ(after.appraiser.location, Location::Base);
  EXPECT_EQ(after.chef.inventory, before.chef.inventory);
  EXPECT_EQ(after.appraiser.inventory, before.appraiser.inventory);
  EXPECT_EQ(after.day, before.day);
  EXPECT_FALSE(danger.explorer(Actor::Chef).absent);
  EXPECT_FALSE(danger.explorer(Actor::Appraiser).absent);
}

TEST(Exploration, AnObstacleAloneSendsAnActorAwayWithNoMutantOnTheMap) {
  Danger danger({{.cell = Danger::OBSTACLE}}, {});
  danger.both_enter();
  ASSERT_TRUE(danger.exploring().snapshot().mutants.empty());
  danger.walk_west_until_absent(Actor::Chef);
  EXPECT_TRUE(danger.explorer(Actor::Chef).absent);
}

TEST(Exploration, AMutantAloneSendsAnActorAwayWithNoObstacleOnTheMap) {
  Danger danger({}, {{.from = {.x = 18, .y = 0}, .to = {.x = 22, .y = 0}}});
  danger.both_enter();
  ASSERT_TRUE(danger.exploring().snapshot().obstacles.empty());
  ASSERT_EQ(danger.exploring().snapshot().mutants.size(), 1U);
  danger.walk_west_until_absent(Actor::Chef);
  EXPECT_TRUE(danger.explorer(Actor::Chef).absent);
}

TEST(Exploration, WalkingTheSameWayPastNothingTouchesNoOne) {
  Danger danger({}, {});
  danger.both_enter();
  danger.walk_west_until_absent(Actor::Chef);
  EXPECT_FALSE(danger.explorer(Actor::Chef).absent);
  EXPECT_LT(at(danger.explorer(Actor::Chef)).x, Danger::OBSTACLE.x);
}

TEST(Exploration, AMutantWalksItsPatrolBackAndForthOverTime) {
  Danger danger({}, {{.from = {.x = 40, .y = 0}, .to = {.x = 44, .y = 0}}});
  danger.both_enter();
  std::vector<std::int32_t> seen;
  for (int second = 0; second < 12; ++second) {
    danger.idle(50);
    const auto mutants = danger.exploring().snapshot().mutants;
    ASSERT_EQ(mutants.size(), 1U);
    const auto x = mutants.front().position.x;
    EXPECT_GE(x, 40 * SUBCELLS);
    EXPECT_LE(x, 45 * SUBCELLS);
    seen.push_back(x);
  }
  const auto swing = std::ranges::minmax(seen);
  EXPECT_GE(swing.max - swing.min, 2 * SUBCELLS);
}

TEST(Exploration, TheScreenLockIgnoresAnActorThatIsAbsent) {
  Danger danger;
  danger.both_enter();
  danger.walk_west_until_absent(Actor::Chef);
  const auto chef = danger.explorer(Actor::Chef);
  ASSERT_TRUE(chef.absent);
  // The partner walks east, beyond the lock's reach of the chef's body.
  for (int tick = 0; tick < 120; ++tick) {
    danger.press(Actor::Appraiser, Direction::Right);
  }
  ASSERT_TRUE(danger.explorer(Actor::Chef).absent);
  EXPECT_GT(danger.explorer(Actor::Appraiser).body.position.x -
                chef.body.position.x,
            SCREEN_LOCK);
}

TEST(Exploration, AnAbsentActorTakesNoMovementCommand) {
  Danger danger;
  danger.both_enter();
  danger.walk_west_until_absent(Actor::Chef);
  ASSERT_TRUE(danger.explorer(Actor::Chef).absent);
  const auto away = danger.explorer(Actor::Chef).body.position;
  danger.press(Actor::Chef, Direction::Left);
  EXPECT_EQ(danger.explorer(Actor::Chef).body.position, away);
}

TEST(Exploration, TheTimeUpWhileAbsentSendsEveryoneHome) {
  Simulation simulation({.exploration_budget = 20s, .seed = 0});
  World world({floor_at(FLOOR, 0, 0, 60)});
  Exploration exploration(simulation, world,
                          {.base = BASE,
                           .entry = Danger::WEST_ENTRY,
                           .exits = {},
                           .interiors = {},
                           .sources = {},
                           .obstacles = {{.cell = Danger::OBSTACLE}},
                           .patrols = {},
                           .absence = 10s});
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.enter(Actor::Appraiser), Result::Applied);
  for (int tick = 0; tick < 800 && !exploration.snapshot().chef.absent;
       ++tick) {
    ASSERT_EQ(exploration.advance(20ms, {LEFT, {}}), Result::Applied);
  }
  ASSERT_TRUE(exploration.snapshot().chef.absent);
  ASSERT_EQ(exploration.advance(20s, IDLE), Result::Applied);
  EXPECT_EQ(simulation.snapshot().chef.location, Location::Base);
  EXPECT_EQ(simulation.snapshot().appraiser.location, Location::Base);
  EXPECT_FALSE(exploration.snapshot().chef.absent);
}

TEST(Exploration, AnObstacleOrPatrolNeedsAPositiveAbsence) {
  Simulation simulation({.exploration_budget = 20s, .seed = 0});
  World world({floor_at(FLOOR, 0, 0, 60)});
  EXPECT_THROW(Exploration(simulation, world,
                           {.base = BASE,
                            .entry = Danger::WEST_ENTRY,
                            .exits = {},
                            .interiors = {},
                            .sources = {},
                            .obstacles = {{.cell = Danger::OBSTACLE}},
                            .patrols = {}}),
               std::invalid_argument);
}

TEST(Exploration, ARouteNeverPassesThroughAnObstacleCell) {
  const World world({floor_at(FLOOR, 0, 0, 60)});
  const GridPosition obstacle = Danger::OBSTACLE;
  const CellMap open(world.snapshot());
  const CellMap blocked(world.snapshot(), {obstacle});
  const auto walks_into = [&](const CellMap &cells) {
    const GridPosition beside{.x = obstacle.x + 1, .y = obstacle.y};
    return std::ranges::any_of(links(cells, beside), [&](const Link &link) {
      return link.to == obstacle;
    });
  };
  ASSERT_TRUE(walks_into(open));
  EXPECT_FALSE(walks_into(blocked));
  EXPECT_TRUE(blocked.obstructed(obstacle));
}

TEST(Exploration, AnAbsentActorCannotCollectFromASourceInRange) {
  Danger danger;
  danger.both_enter();
  danger.walk_west_until_absent(Actor::Chef);
  ASSERT_TRUE(danger.explorer(Actor::Chef).absent);
  ASSERT_LE(distance(at(danger.explorer(Actor::Chef)),
                     {.x = 21, .y = 0, .width = 1, .height = 1}),
            1);
  EXPECT_EQ(danger.exploring().collect(Actor::Chef, Danger::NEAR_SOURCE),
            CollectionResult::NotExploring);
  // The control: the partner, on the map and in range, collects.
  danger.press(Actor::Appraiser, Direction::Left);
  for (int tick = 0; tick < 100 && at(danger.explorer(Actor::Appraiser)).x > 22;
       ++tick) {
    danger.press(Actor::Appraiser, Direction::Left);
  }
  EXPECT_EQ(danger.exploring().collect(Actor::Appraiser, Danger::NEAR_SOURCE),
            CollectionResult::Applied);
}

TEST(Exploration, TheMapDoesNotChangeWhileAnActorOfThePairIsAbsent) {
  Danger danger;
  danger.both_enter();
  danger.walk_west_until_absent(Actor::Chef);
  ASSERT_TRUE(danger.explorer(Actor::Chef).absent);
  ASSERT_GE(at(danger.explorer(Actor::Chef)).x, Danger::WEST_EXIT.x);
  ASSERT_LT(at(danger.explorer(Actor::Chef)).x,
            Danger::WEST_EXIT.x + Danger::WEST_EXIT.width);
  for (int tick = 0;
       tick < 400 && at(danger.explorer(Actor::Appraiser)).x >
                         Danger::WEST_EXIT.x + Danger::WEST_EXIT.width - 1;
       ++tick) {
    danger.press(Actor::Appraiser, Direction::Left);
  }
  ASSERT_LT(at(danger.explorer(Actor::Appraiser)).x,
            Danger::WEST_EXIT.x + Danger::WEST_EXIT.width);
  EXPECT_EQ(danger.exploring().snapshot().map, MapId::Street);
  // The control: once the chef is back both stand in the zone and the party
  // crosses at the next movement unit.
  danger.idle(500);
  ASSERT_FALSE(danger.explorer(Actor::Chef).absent);
  danger.idle(5);
  EXPECT_EQ(danger.exploring().snapshot().map, MapId::Aquarium);
}

TEST(Exploration, AnAbsentActorGoesHomeWhenItsPartnerReturnsToBase) {
  Danger danger;
  danger.both_enter();
  danger.walk_west_until_absent(Actor::Chef);
  ASSERT_TRUE(danger.explorer(Actor::Chef).absent);
  ASSERT_EQ(danger.exploring().return_to_base(Actor::Appraiser),
            Result::Applied);
  EXPECT_EQ(danger.sim().snapshot().chef.location, Location::Base);
  EXPECT_FALSE(danger.explorer(Actor::Chef).absent);
}

TEST(Exploration, AMutantThatWalksOntoAnActorWhoStandsStillSendsItAwayAtOnce) {
  const Patrol patrol{.from = {.x = 26, .y = 0}, .to = {.x = 34, .y = 0}};
  Danger danger({}, {patrol});
  danger.enter(Actor::Chef);
  const auto here = at(danger.explorer(Actor::Chef));
  for (int unit = 0; unit < 40; ++unit) {
    // The unit that puts the mutant on the actor is the one that touches.
    const auto next =
        patrol_step(patrol, danger.exploring().snapshot().mutants.front());
    const bool lands = cell_of(next.position) == here;
    ASSERT_EQ(danger.exploring().advance(100ms, IDLE), Result::Applied);
    if (lands) {
      EXPECT_EQ(danger.sim().snapshot().chef.location, Location::Base);
      return;
    }
    ASSERT_EQ(danger.sim().snapshot().chef.location, Location::City);
  }
  FAIL() << "the mutant never reached the actor";
}

} // namespace
} // namespace neocity::survival
