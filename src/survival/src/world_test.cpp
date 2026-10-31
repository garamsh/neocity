#include <survival/world.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace neocity::survival {
namespace {

using namespace std::chrono_literals;

constexpr ObjectId NPC{11};
constexpr ObjectId NPC_SOURCE{12};
constexpr ObjectId TERRAIN{21};
constexpr ObjectId LADDER{31};
constexpr ObjectId ENTRANCE{32};
constexpr ObjectId BUILDING{41};
constexpr ObjectId ARRIVING_BUILDING{42};
constexpr ObjectId NEW_PATH{43};
constexpr ObjectId NEW_SOURCE{44};
constexpr ToolId WRENCH{1};
constexpr ToolId SPATULA{2};

std::vector<ObjectDefinition> city() {
  return {
      {.id = NPC,
       .kind = ObjectKind::Npc,
       .area = {.x = 5, .y = 0, .width = 1, .height = 2},
       .destroyed_after = WorldThreshold{.acid = AcidLevel{2}}},
      {.id = NPC_SOURCE,
       .kind = ObjectKind::MaterialSource,
       .ingredient = IngredientId{70},
       .supported_by = NPC},
      {.id = TERRAIN,
       .kind = ObjectKind::Terrain,
       .area = {.x = 0, .y = 0, .width = 10, .height = 1},
       .destroyed_after = WorldThreshold{.elapsed = 4s}},
      {.id = LADDER,
       .kind = ObjectKind::Ladder,
       .area = {.x = 10, .y = 0, .width = 1, .height = 8},
       .destroyed_after = WorldThreshold{.elapsed = 4s, .acid = AcidLevel{2}},
       .repair_with = WRENCH},
      {.id = ENTRANCE,
       .kind = ObjectKind::Entrance,
       .destroyed_after = WorldThreshold{.elapsed = 8s},
       .repair_with = WRENCH},
      {.id = BUILDING,
       .kind = ObjectKind::Building,
       .destroyed_after = WorldThreshold{.elapsed = 5s}},
      {.id = ARRIVING_BUILDING,
       .kind = ObjectKind::Building,
       .exposed_at = WorldThreshold{.elapsed = 5s}},
      {.id = NEW_PATH,
       .kind = ObjectKind::Entrance,
       .area = {.x = 20, .y = 0, .width = 2, .height = 3},
       .exposed_at = WorldThreshold{.elapsed = 5s}},
      {.id = NEW_SOURCE,
       .kind = ObjectKind::MaterialSource,
       .exposed_at = WorldThreshold{.elapsed = 5s},
       .destroyed_after = WorldThreshold{.elapsed = 8s},
       .ingredient = IngredientId{80},
       .supported_by = ARRIVING_BUILDING}};
}

ObjectSnapshot object(const WorldSnapshot &snapshot, ObjectId id) {
  const auto found =
      std::ranges::find_if(snapshot.objects, [id](const ObjectSnapshot &entry) {
        return entry.id == id;
      });
  if (found == snapshot.objects.end()) {
    throw std::runtime_error("missing fixture object");
  }
  return *found;
}

TEST(World, SnapshotsRetainStableIdentitiesAndAreasWithoutSharingMutableState) {
  const World world(city());
  auto snapshot = world.snapshot();
  EXPECT_EQ(snapshot.objects.size(), 9U);
  EXPECT_EQ(object(snapshot, NPC).kind, ObjectKind::Npc);
  EXPECT_EQ(object(snapshot, NPC).area,
            (WorldArea{.x = 5, .y = 0, .width = 1, .height = 2}));
  EXPECT_TRUE(object(snapshot, TERRAIN).traversable);
  EXPECT_TRUE(object(snapshot, LADDER).traversable);
  EXPECT_TRUE(object(snapshot, ENTRANCE).traversable);
  EXPECT_TRUE(object(snapshot, NPC_SOURCE).material_available);
  EXPECT_EQ(object(snapshot, NPC_SOURCE).ingredient, IngredientId{70});
  EXPECT_EQ(object(snapshot, LADDER).repair_with, WRENCH);
  EXPECT_EQ(object(snapshot, NEW_PATH).state, ObjectState::Dormant);
  EXPECT_FALSE(object(snapshot, NEW_PATH).traversable);
  EXPECT_FALSE(object(snapshot, NEW_SOURCE).material_available);
  snapshot.objects.clear();
  EXPECT_EQ(world.snapshot().objects.size(), 9U);
}

TEST(World, AcidDestroysNpcSupplyWithoutRemovingItsStableSourceIdentity) {
  World world(city());
  ASSERT_TRUE(object(world.snapshot(), NPC_SOURCE).material_available);
  ASSERT_EQ(world.update({.elapsed = 1s, .acid = {1}}),
            WorldUpdateResult::Applied);
  EXPECT_TRUE(object(world.snapshot(), NPC_SOURCE).material_available);
  ASSERT_EQ(world.update({.elapsed = 1s, .acid = {2}}),
            WorldUpdateResult::Applied);
  const auto snapshot = world.snapshot();
  EXPECT_EQ(object(snapshot, NPC).state, ObjectState::Destroyed);
  EXPECT_EQ(object(snapshot, NPC_SOURCE).id, NPC_SOURCE);
  EXPECT_FALSE(object(snapshot, NPC_SOURCE).material_available);
  EXPECT_TRUE(object(snapshot, TERRAIN).traversable);
}

TEST(World, TimeDamageRemovesTerrainAndPassageAccessAtTheirOwnThresholds) {
  World world(city());
  ASSERT_EQ(world.update({.elapsed = 4s - 1us}), WorldUpdateResult::Applied);
  EXPECT_TRUE(object(world.snapshot(), TERRAIN).traversable);
  EXPECT_TRUE(object(world.snapshot(), LADDER).traversable);
  ASSERT_EQ(world.update({.elapsed = 4s}), WorldUpdateResult::Applied);
  EXPECT_EQ(object(world.snapshot(), TERRAIN).state, ObjectState::Destroyed);
  EXPECT_FALSE(object(world.snapshot(), TERRAIN).traversable);
  EXPECT_FALSE(object(world.snapshot(), LADDER).traversable);
  EXPECT_TRUE(object(world.snapshot(), ENTRANCE).traversable);
  ASSERT_EQ(world.update({.elapsed = 8s}), WorldUpdateResult::Applied);
  EXPECT_FALSE(object(world.snapshot(), ENTRANCE).traversable);
}

TEST(World, DeteriorationAndArrivalExposeANewPathAndMaterialSource) {
  World world(city());
  ASSERT_EQ(object(world.snapshot(), BUILDING).state, ObjectState::Intact);
  ASSERT_EQ(object(world.snapshot(), ARRIVING_BUILDING).state,
            ObjectState::Dormant);
  ASSERT_EQ(world.update({.elapsed = 5s}), WorldUpdateResult::Applied);
  const auto snapshot = world.snapshot();
  EXPECT_EQ(object(snapshot, BUILDING).state, ObjectState::Destroyed);
  EXPECT_EQ(object(snapshot, ARRIVING_BUILDING).state, ObjectState::Intact);
  EXPECT_EQ(object(snapshot, NEW_PATH).id, NEW_PATH);
  EXPECT_TRUE(object(snapshot, NEW_PATH).traversable);
  EXPECT_EQ(object(snapshot, NEW_SOURCE).ingredient, IngredientId{80});
  EXPECT_TRUE(object(snapshot, NEW_SOURCE).material_available);
  ASSERT_EQ(world.update({.elapsed = 8s}), WorldUpdateResult::Applied);
  EXPECT_EQ(object(world.snapshot(), NEW_SOURCE).state, ObjectState::Destroyed);
  EXPECT_FALSE(object(world.snapshot(), NEW_SOURCE).material_available);
  EXPECT_EQ(object(world.snapshot(), ARRIVING_BUILDING).state,
            ObjectState::Intact);
}

TEST(World, RepairRequiresADamagedRepairablePassageAndTheMatchingTool) {
  World world(city());
  ASSERT_EQ(world.update({.elapsed = 4s}), WorldUpdateResult::Applied);
  const auto damaged = world.snapshot();
  ASSERT_EQ(object(damaged, LADDER).state, ObjectState::Destroyed);
  ASSERT_EQ(object(damaged, TERRAIN).state, ObjectState::Destroyed);
  ASSERT_EQ(object(damaged, ENTRANCE).state, ObjectState::Intact);
  EXPECT_EQ(world.repair(ObjectId{999}, WRENCH), RepairResult::UnknownObject);
  EXPECT_EQ(world.repair(ENTRANCE, WRENCH), RepairResult::NotDamaged);
  EXPECT_EQ(world.repair(TERRAIN, WRENCH), RepairResult::NotRepairable);
  EXPECT_EQ(world.repair(LADDER, SPATULA), RepairResult::WrongTool);
  EXPECT_EQ(world.snapshot(), damaged);
  ASSERT_EQ(world.repair(LADDER, WRENCH), RepairResult::Applied);
  const auto repaired = world.snapshot();
  EXPECT_TRUE(object(repaired, LADDER).traversable);
  EXPECT_FALSE(object(repaired, TERRAIN).traversable);
  EXPECT_EQ(world.repair(LADDER, WRENCH), RepairResult::NotDamaged);
  EXPECT_EQ(world.snapshot(), repaired);
  ASSERT_EQ(world.update({.elapsed = 4s}), WorldUpdateResult::Applied);
  EXPECT_EQ(world.snapshot(), repaired);
  ASSERT_EQ(world.update({.elapsed = 8s}), WorldUpdateResult::Applied);
  EXPECT_FALSE(object(world.snapshot(), LADDER).traversable);
}

TEST(World,
     DuplicateProgressAndAcidDecreaseDoNotUndoDamageOrRepeatRepairDamage) {
  World world(city());
  ASSERT_EQ(world.update({.elapsed = 5s, .acid = {3}}),
            WorldUpdateResult::Applied);
  ASSERT_EQ(world.repair(LADDER, WRENCH), RepairResult::Applied);
  const auto repaired = world.snapshot();
  ASSERT_TRUE(object(repaired, LADDER).traversable);
  ASSERT_EQ(world.update(repaired.progression), WorldUpdateResult::Applied);
  EXPECT_EQ(world.snapshot(), repaired);
  ASSERT_EQ(world.update({.elapsed = 5s, .acid = {0}}),
            WorldUpdateResult::Applied);
  auto lowered = repaired;
  lowered.progression.acid = {0};
  EXPECT_EQ(world.snapshot(), lowered);
  ASSERT_EQ(world.update({.elapsed = 5s, .acid = {4}}),
            WorldUpdateResult::Applied);
  EXPECT_TRUE(object(world.snapshot(), LADDER).traversable);
  EXPECT_EQ(object(world.snapshot(), NPC).state, ObjectState::Destroyed);
  EXPECT_TRUE(object(world.snapshot(), NEW_PATH).traversable);
  ASSERT_EQ(world.update({.elapsed = 5s, .acid = {5}}),
            WorldUpdateResult::Applied);
  EXPECT_FALSE(object(world.snapshot(), LADDER).traversable);
}

TEST(World, DecreasingCumulativeTimeIsRefusedWithoutApplyingItsAcidInput) {
  World world(city());
  ASSERT_EQ(world.update({.elapsed = 3s, .acid = {1}}),
            WorldUpdateResult::Applied);
  const auto before = world.snapshot();
  EXPECT_EQ(world.update({.elapsed = 2s, .acid = {9}}),
            WorldUpdateResult::DecreasingTime);
  EXPECT_EQ(world.snapshot(), before);
  ASSERT_EQ(world.update({.elapsed = 4s, .acid = {1}}),
            WorldUpdateResult::Applied);
  EXPECT_FALSE(object(world.snapshot(), LADDER).traversable);
  EXPECT_TRUE(object(world.snapshot(), NPC_SOURCE).material_available);
}

TEST(World, FramePartitionsProduceTheSameDamageExposureAndSupplySnapshot) {
  World whole(city());
  World partitioned(city());
  ASSERT_EQ(whole.update({.elapsed = 8s, .acid = {3}}),
            WorldUpdateResult::Applied);
  auto elapsed = 0us;
  for (const auto delta : {1us, 8332us, 16667us, 75000us, 7900000us}) {
    elapsed += delta;
    ASSERT_EQ(partitioned.update({.elapsed = elapsed, .acid = {3}}),
              WorldUpdateResult::Applied);
  }
  ASSERT_EQ(elapsed, 8s);
  EXPECT_EQ(whole.snapshot(), partitioned.snapshot());
}

TEST(World, InvalidDefinitionsAreRejectedBesideAValidControl) {
  const World valid(city());
  ASSERT_EQ(valid.snapshot().objects.size(), 9U);
  auto invalid = city();
  invalid.at(1).id = invalid.at(0).id;
  EXPECT_THROW((World(invalid)), std::invalid_argument);
  invalid = city();
  invalid.at(0).area.width = 0;
  EXPECT_THROW((World(invalid)), std::invalid_argument);
  invalid = city();
  invalid.at(0).destroyed_after = WorldThreshold{};
  EXPECT_THROW((World(invalid)), std::invalid_argument);
  invalid.at(0).destroyed_after = WorldThreshold{.elapsed = -1us};
  EXPECT_THROW((World(invalid)), std::invalid_argument);
  invalid.at(0).destroyed_after = WorldThreshold{.acid = AcidLevel{0}};
  EXPECT_THROW((World(invalid)), std::invalid_argument);
  invalid = city();
  invalid.at(1).ingredient.reset();
  EXPECT_THROW((World(invalid)), std::invalid_argument);
  invalid = city();
  invalid.at(1).supported_by = NEW_SOURCE;
  EXPECT_THROW((World(invalid)), std::invalid_argument);
  invalid = city();
  invalid.at(0).repair_with = WRENCH;
  EXPECT_THROW((World(invalid)), std::invalid_argument);
}

TEST(WorldIntegration,
     ActorReturnAndHealthRecoveryPreserveDeteriorationAndRepair) {
  Simulation simulation({.exploration_budget = 10s, .seed = 123});
  World world(city());
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.advance(5s), Result::Applied);
  const auto elapsed = 10s - simulation.snapshot().remaining_exploration;
  ASSERT_EQ(elapsed, 5s);
  ASSERT_EQ(world.update({.elapsed = elapsed, .acid = {3}}),
            WorldUpdateResult::Applied);
  ASSERT_EQ(world.repair(LADDER, WRENCH), RepairResult::Applied);
  const auto before_return = world.snapshot();
  ASSERT_EQ(object(before_return, NPC).state, ObjectState::Destroyed);
  ASSERT_TRUE(object(before_return, LADDER).traversable);
  for (int hit = 0; hit < 3; ++hit) {
    ASSERT_EQ(simulation.damage(Actor::Chef), Result::Applied);
  }
  ASSERT_EQ(simulation.snapshot().chef.location, Location::Base);
  ASSERT_EQ(simulation.snapshot().chef.health, 3);
  ASSERT_EQ(simulation.snapshot().remaining_exploration, 5s);
  ASSERT_EQ(world.update(
                {.elapsed = 10s - simulation.snapshot().remaining_exploration,
                 .acid = {3}}),
            WorldUpdateResult::Applied);
  EXPECT_EQ(world.snapshot(), before_return);
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.advance(1s), Result::Applied);
  ASSERT_EQ(world.update(
                {.elapsed = 10s - simulation.snapshot().remaining_exploration,
                 .acid = {3}}),
            WorldUpdateResult::Applied);
  EXPECT_TRUE(object(world.snapshot(), LADDER).traversable);
  EXPECT_FALSE(object(world.snapshot(), NPC_SOURCE).material_available);
}

} // namespace
} // namespace neocity::survival
