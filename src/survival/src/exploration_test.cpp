#include <survival/exploration.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <limits>
#include <stdexcept>

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

std::vector<ObjectDefinition> city() {
  return {{.id = FLOOR, .area = {.x = 0, .y = 0, .width = 20, .height = 3}},
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

TEST(Exploration, BothRolesMoveIdenticallyAndShareOneClock) {
  Simulation simulation({.exploration_budget = 5s, .seed = 9});
  World world(city());
  Exploration exploration(simulation, world, tuning());
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.enter(Actor::Appraiser), Result::Applied);
  ASSERT_EQ(exploration.advance(200ms, WALK, {}), Result::Applied);
  EXPECT_EQ(exploration.snapshot().chef.position, (GridPosition{.x = 2}));
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
  EXPECT_EQ(exploration.snapshot().chef.position, ENTRY);
  ASSERT_EQ(exploration.enter(Actor::Appraiser), Result::Applied);
  ASSERT_EQ(exploration.advance(40ms, WALK, {}), Result::Applied);
  EXPECT_EQ(exploration.snapshot().chef.position, (GridPosition{.x = 1}));
  EXPECT_EQ(exploration.snapshot().appraiser.position, ENTRY);
  ASSERT_EQ(exploration.advance(60ms, WALK, {}), Result::Applied);
  EXPECT_EQ(exploration.snapshot().appraiser.position, (GridPosition{.x = 1}));
}

TEST(Exploration, RunningExhaustsStaminaThenWalksAndRecoversForBothRoles) {
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(city());
  Exploration exploration(simulation, world, tuning());
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.enter(Actor::Appraiser), Result::Applied);
  ASSERT_EQ(exploration.advance(200ms, RUN, {}), Result::Applied);
  EXPECT_EQ(exploration.snapshot().chef.position, (GridPosition{.x = 4}));
  EXPECT_EQ(exploration.snapshot().chef.stamina, 0U);
  EXPECT_EQ(exploration.snapshot().chef, exploration.snapshot().appraiser);
  ASSERT_EQ(exploration.advance(100ms, RUN, {}), Result::Applied);
  EXPECT_EQ(exploration.snapshot().chef.position, (GridPosition{.x = 5}));
  EXPECT_EQ(exploration.snapshot().chef.stamina, 1U);
  ASSERT_EQ(exploration.advance(300ms, IDLE, {}), Result::Applied);
  EXPECT_EQ(exploration.snapshot().chef.stamina, 2U);
  EXPECT_EQ(exploration.snapshot().chef, exploration.snapshot().appraiser);
}

TEST(Exploration, RunningChecksThinBuildingsAndBrokenPassagesCellByCell) {
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
      EXPECT_EQ(exploration.snapshot().chef.position, ENTRY);
      ASSERT_EQ(exploration.advance(100ms, RUN, {1}), Result::Applied);
      EXPECT_EQ(exploration.snapshot().chef.position, (GridPosition{.x = 2}));
    } else {
      EXPECT_EQ(exploration.snapshot().chef.position, (GridPosition{.x = 2}));
      const std::array<ExplorationInput, 2> left{
          ExplorationInput{.direction = Direction::Left, .running = true}, {}};
      ASSERT_EQ(exploration.advance(100ms, left, {1}), Result::Applied);
      EXPECT_EQ(exploration.snapshot().chef.position, (GridPosition{.x = 2}));
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
      EXPECT_EQ(exploration.snapshot().chef.position, (GridPosition{.x = 2}));
      const std::array<ExplorationInput, 2> left{
          ExplorationInput{.direction = Direction::Left, .running = true}, {}};
      ASSERT_EQ(exploration.advance(100ms, left, {1}), Result::Applied);
      EXPECT_EQ(exploration.snapshot().chef.position, (GridPosition{.x = 2}));
      ASSERT_EQ(world.repair(ObjectId{21}, ToolId{1}), RepairResult::Applied);
      ASSERT_EQ(exploration.advance(100ms, left, {1}), Result::Applied);
      EXPECT_EQ(exploration.snapshot().chef.position, (GridPosition{.x = 1}));
    }
  }
}

TEST(Exploration, LiveLadderAndEntranceSupportVerticalMovementOverEmptySpace) {
  auto definitions = city();
  definitions.front().area.height = 1;
  definitions.push_back(
      {.id = OWNER, .kind = ObjectKind::Ladder, .area = {.x = 0, .y = -1}});
  definitions.push_back({.id = ObjectId{21},
                         .kind = ObjectKind::Entrance,
                         .area = {.x = 0, .y = -2}});
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(definitions);
  Exploration exploration(simulation, world, tuning());
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  const std::array<ExplorationInput, 2> up{
      ExplorationInput{.direction = Direction::Up}, {}};
  ASSERT_EQ(exploration.advance(200ms, up, {}), Result::Applied);
  EXPECT_EQ(exploration.snapshot().chef.position, (GridPosition{.y = -2}));
  EXPECT_EQ(simulation.snapshot().chef.location, Location::City);
  ASSERT_EQ(exploration.advance(100ms, up, {}), Result::Applied);
  EXPECT_EQ(exploration.snapshot().chef.position, BASE);
  EXPECT_EQ(simulation.snapshot().chef.health, 3U);
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
  ASSERT_EQ(exploration.advance(150ms, RUN, {}), Result::Applied);
  EXPECT_EQ(simulation.snapshot().chef.location, Location::Base);
  EXPECT_EQ(simulation.snapshot().chef.health, 3U);
  EXPECT_EQ(simulation.snapshot().chef.inventory,
            (std::vector{IngredientId{70}}));
  EXPECT_EQ(exploration.snapshot().chef.position, BASE);
  EXPECT_EQ(simulation.snapshot().remaining_exploration, 4900ms);
  EXPECT_EQ(simulation.snapshot().seed, 19U);
  EXPECT_EQ(simulation.snapshot().day, 1U);
  EXPECT_EQ(exploration.snapshot().sources.front().remaining, 0U);
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  EXPECT_EQ(exploration.snapshot().chef.position, ENTRY);
  ASSERT_EQ(exploration.advance(50ms, WALK, {}), Result::Applied);
  EXPECT_EQ(exploration.snapshot().chef.position, ENTRY);
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
    EXPECT_EQ(exploration.snapshot().chef.position, (GridPosition{.x = 1}));
    const auto persistent_world = world.snapshot();
    if (exhausted) {
      for (int hit = 0; hit < 3; ++hit) {
        ASSERT_EQ(exploration.damage(Actor::Chef), Result::Applied);
      }
    } else {
      ASSERT_EQ(exploration.return_to_base(Actor::Chef), Result::Applied);
    }
    EXPECT_EQ(exploration.snapshot().chef.position, BASE);
    EXPECT_EQ(simulation.snapshot().chef.health, 3U);
    EXPECT_EQ(simulation.snapshot().chef.inventory,
              (std::vector{IngredientId{70}}));
    EXPECT_EQ(simulation.snapshot().appraiser.location, Location::City);
    EXPECT_EQ(simulation.snapshot().remaining_exploration, 4850ms);
    EXPECT_EQ(world.snapshot(), persistent_world);
    ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
    ASSERT_EQ(exploration.advance(50ms, WALK, {}), Result::Applied);
    EXPECT_EQ(exploration.snapshot().chef.position, ENTRY);
    EXPECT_EQ(exploration.snapshot().appraiser.position,
              (GridPosition{.x = 2}));
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
  EXPECT_EQ(exploration.snapshot().chef.position, (GridPosition{.x = 1}));
  ASSERT_EQ(exploration.collect(Actor::Chef, SOURCE),
            CollectionResult::Applied);
  ASSERT_EQ(exploration.advance(10s, RUN, {}), Result::Applied);
  EXPECT_EQ(exploration.snapshot().chef.position, BASE);
  EXPECT_EQ(exploration.snapshot().appraiser.position, BASE);
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
  ASSERT_EQ(exploration.advance(300ms, WALK, {}), Result::Applied);
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

TEST(Exploration,
     WorldDeteriorationReturnsUnsupportedActorsWithoutResettingHistory) {
  auto definitions = city();
  definitions.front().destroyed_after = WorldThreshold{.elapsed = 150ms};
  Simulation simulation({.exploration_budget = 5s, .seed = 0});
  World world(definitions);
  Exploration exploration(simulation, world, tuning());
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.advance(100ms, IDLE, {}), Result::Applied);
  EXPECT_EQ(simulation.snapshot().chef.location, Location::City);
  ASSERT_EQ(exploration.advance(50ms, IDLE, {2}), Result::Applied);
  EXPECT_EQ(simulation.snapshot().chef.location, Location::Base);
  EXPECT_EQ(exploration.snapshot().chef.position, BASE);
  EXPECT_EQ(world.snapshot().objects.front().state, ObjectState::Destroyed);
  EXPECT_EQ(world.snapshot().peak_acid, (AcidLevel{2}));
  EXPECT_EQ(simulation.snapshot().acid, AcidLevel{});
  EXPECT_EQ(exploration.enter(Actor::Chef), Result::NotExploring);
  const auto before = simulation.snapshot();
  ASSERT_EQ(exploration.advance(1s, IDLE, {}), Result::Applied);
  EXPECT_EQ(simulation.snapshot(), before);
  EXPECT_EQ(world.snapshot().progression.elapsed, 150ms);
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
  EXPECT_EQ(exploration.snapshot().chef.position, (GridPosition{.x = 1}));
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
  const std::array<ExplorationInput, 2> up{
      ExplorationInput{.direction = Direction::Up}, {}};
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(exploration.collect(Actor::Chef, TOOL_SOURCE),
            CollectionResult::Applied);
  ASSERT_EQ(exploration.advance(100ms, up, {}), Result::Applied);
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
  ASSERT_EQ(exploration.snapshot().chef.position, (GridPosition{.x = 2}));
  EXPECT_EQ(exploration.repair(Actor::Chef, LADDER, WRENCH),
            RepairResult::Applied);
}

} // namespace
} // namespace neocity::survival
