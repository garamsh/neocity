#include <survival/world.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
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
constexpr ObjectId AQUARIUM_FLOOR{45};

std::vector<ObjectDefinition> city() {
  return {
      {.id = NPC,
       .kind = ObjectKind::Npc,
       .area = {.x = 5, .y = 0, .width = 1, .height = 2},
       .destroyed_from_day = 6},
      {.id = NPC_SOURCE,
       .kind = ObjectKind::MaterialSource,
       .ingredient = IngredientId{70},
       .supported_by = NPC},
      {.id = TERRAIN,
       .kind = ObjectKind::Terrain,
       .area = {.x = 0, .y = 0, .width = 10, .height = 1},
       .destroyed_from_day = 5},
      {.id = LADDER,
       .kind = ObjectKind::Ladder,
       .area = {.x = 10, .y = 0, .width = 1, .height = 8},
       .destroyed_from_day = 5},
      {.id = ENTRANCE, .kind = ObjectKind::Entrance, .destroyed_from_day = 10},
      {.id = BUILDING, .kind = ObjectKind::Building, .destroyed_from_day = 7},
      {.id = ARRIVING_BUILDING,
       .kind = ObjectKind::Building,
       .exposed_from_day = 7},
      {.id = NEW_PATH,
       .kind = ObjectKind::Entrance,
       .area = {.x = 20, .y = 0, .width = 2, .height = 3},
       .exposed_from_day = 7},
      {.id = NEW_SOURCE,
       .kind = ObjectKind::MaterialSource,
       .exposed_from_day = 7,
       .destroyed_from_day = 10,
       .ingredient = IngredientId{80},
       .supported_by = ARRIVING_BUILDING},
      {.id = AQUARIUM_FLOOR,
       .kind = ObjectKind::Terrain,
       .area = {.x = 0, .y = 0, .width = 6, .height = 1},
       .map = MapId::Aquarium}};
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
  EXPECT_EQ(snapshot.objects.size(), 10U);
  EXPECT_EQ(object(snapshot, NPC).kind, ObjectKind::Npc);
  EXPECT_EQ(object(snapshot, NPC).area,
            (WorldArea{.x = 5, .y = 0, .width = 1, .height = 2}));
  EXPECT_TRUE(object(snapshot, TERRAIN).traversable);
  EXPECT_TRUE(object(snapshot, LADDER).traversable);
  EXPECT_TRUE(object(snapshot, ENTRANCE).traversable);
  EXPECT_TRUE(object(snapshot, NPC_SOURCE).material_available);
  EXPECT_EQ(object(snapshot, NPC_SOURCE).ingredient, IngredientId{70});
  EXPECT_EQ(object(snapshot, NEW_PATH).state, ObjectState::Dormant);
  EXPECT_FALSE(object(snapshot, NEW_PATH).traversable);
  EXPECT_FALSE(object(snapshot, NEW_SOURCE).material_available);
  EXPECT_EQ(object(snapshot, AQUARIUM_FLOOR).map, MapId::Aquarium);
  EXPECT_EQ(object(snapshot, TERRAIN).map, MapId::Street);
  snapshot.objects.clear();
  EXPECT_EQ(world.snapshot().objects.size(), 10U);
}

TEST(World, DamageStagesBeginOnTheirDaysAndDependOnTheDayAlone) {
  EXPECT_EQ(damage_stage(0), DamageStage::Standing);
  EXPECT_EQ(damage_stage(1), DamageStage::Standing);
  EXPECT_EQ(damage_stage(4), DamageStage::Standing);
  EXPECT_EQ(damage_stage(5), DamageStage::Cracked);
  EXPECT_EQ(damage_stage(9), DamageStage::Cracked);
  EXPECT_EQ(damage_stage(10), DamageStage::Ruined);
  EXPECT_EQ(damage_stage(500), DamageStage::Ruined);
  World world(city());
  EXPECT_EQ(world.snapshot().stage, DamageStage::Standing);
  ASSERT_EQ(world.update(5), WorldUpdateResult::Applied);
  EXPECT_EQ(world.snapshot().day, 5U);
  EXPECT_EQ(world.snapshot().stage, DamageStage::Cracked);
}

TEST(World, DayDamageRemovesTerrainAndPassageAccessOnItsOwnDay) {
  World world(city());
  ASSERT_EQ(world.update(4), WorldUpdateResult::Applied);
  EXPECT_TRUE(object(world.snapshot(), TERRAIN).traversable);
  EXPECT_TRUE(object(world.snapshot(), LADDER).traversable);
  ASSERT_EQ(world.update(5), WorldUpdateResult::Applied);
  EXPECT_EQ(object(world.snapshot(), TERRAIN).state, ObjectState::Destroyed);
  EXPECT_FALSE(object(world.snapshot(), TERRAIN).traversable);
  EXPECT_FALSE(object(world.snapshot(), LADDER).traversable);
  EXPECT_TRUE(object(world.snapshot(), ENTRANCE).traversable);
  ASSERT_EQ(world.update(10), WorldUpdateResult::Applied);
  EXPECT_FALSE(object(world.snapshot(), ENTRANCE).traversable);
}

TEST(World, DeteriorationAndArrivalExposeANewPathAndMaterialSource) {
  World world(city());
  ASSERT_EQ(object(world.snapshot(), BUILDING).state, ObjectState::Intact);
  ASSERT_EQ(object(world.snapshot(), ARRIVING_BUILDING).state,
            ObjectState::Dormant);
  ASSERT_EQ(world.update(7), WorldUpdateResult::Applied);
  const auto snapshot = world.snapshot();
  EXPECT_EQ(object(snapshot, BUILDING).state, ObjectState::Destroyed);
  EXPECT_EQ(object(snapshot, ARRIVING_BUILDING).state, ObjectState::Intact);
  EXPECT_TRUE(object(snapshot, NEW_PATH).traversable);
  EXPECT_EQ(object(snapshot, NEW_SOURCE).ingredient, IngredientId{80});
  EXPECT_TRUE(object(snapshot, NEW_SOURCE).material_available);
  ASSERT_EQ(world.update(10), WorldUpdateResult::Applied);
  EXPECT_EQ(object(world.snapshot(), NEW_SOURCE).state, ObjectState::Destroyed);
  EXPECT_FALSE(object(world.snapshot(), NEW_SOURCE).material_available);
  EXPECT_EQ(object(world.snapshot(), ARRIVING_BUILDING).state,
            ObjectState::Intact);
}

TEST(World, AnOwnerDestroyedLeavesItsSourceUnavailableButItsIdentityStays) {
  World world(city());
  ASSERT_TRUE(object(world.snapshot(), NPC_SOURCE).material_available);
  ASSERT_EQ(world.update(5), WorldUpdateResult::Applied);
  EXPECT_TRUE(object(world.snapshot(), NPC_SOURCE).material_available);
  ASSERT_EQ(world.update(6), WorldUpdateResult::Applied);
  const auto snapshot = world.snapshot();
  EXPECT_EQ(object(snapshot, NPC).state, ObjectState::Destroyed);
  EXPECT_EQ(object(snapshot, NPC_SOURCE).id, NPC_SOURCE);
  EXPECT_FALSE(object(snapshot, NPC_SOURCE).material_available);
}

TEST(World, ADestroyedPassageStaysDestroyedForTheRestOfTheRun) {
  World world(city());
  ASSERT_EQ(world.update(5), WorldUpdateResult::Applied);
  const auto damaged = world.snapshot();
  ASSERT_EQ(object(damaged, LADDER).state, ObjectState::Destroyed);
  ASSERT_EQ(object(damaged, TERRAIN).state, ObjectState::Destroyed);
  ASSERT_EQ(object(damaged, ENTRANCE).state, ObjectState::Intact);
  EXPECT_FALSE(object(damaged, LADDER).traversable);
  ASSERT_EQ(world.update(5), WorldUpdateResult::Applied);
  EXPECT_EQ(world.snapshot(), damaged);
  ASSERT_EQ(world.update(10), WorldUpdateResult::Applied);
  EXPECT_FALSE(object(world.snapshot(), LADDER).traversable);
  EXPECT_FALSE(object(world.snapshot(), ENTRANCE).traversable);
}

TEST(World, EarlierDayIsRefusedWithoutChangingTheWorld) {
  World world(city());
  ASSERT_EQ(world.update(6), WorldUpdateResult::Applied);
  const auto before = world.snapshot();
  EXPECT_EQ(world.update(5), WorldUpdateResult::DecreasingDay);
  EXPECT_EQ(world.snapshot(), before);
  ASSERT_EQ(world.update(7), WorldUpdateResult::Applied);
  EXPECT_FALSE(object(world.snapshot(), LADDER).traversable);
}

TEST(World, SteppingDayByDayAndJumpingStraightThereGiveTheSameWorld) {
  World whole(city());
  World stepped(city());
  ASSERT_EQ(whole.update(10), WorldUpdateResult::Applied);
  for (std::uint32_t day = 1; day <= 10; ++day) {
    ASSERT_EQ(stepped.update(day), WorldUpdateResult::Applied);
  }
  EXPECT_EQ(whole.snapshot(), stepped.snapshot());
}

TEST(World, OnMapKeepsOnlyThatMapsObjectsAndTheDay) {
  World world(city());
  ASSERT_EQ(world.update(5), WorldUpdateResult::Applied);
  const auto aquarium = on_map(world.snapshot(), MapId::Aquarium);
  ASSERT_EQ(aquarium.objects.size(), 1U);
  EXPECT_EQ(aquarium.objects.at(0).id, AQUARIUM_FLOOR);
  EXPECT_EQ(aquarium.day, 5U);
  EXPECT_EQ(aquarium.stage, DamageStage::Cracked);
  const auto street = on_map(world.snapshot(), MapId::Street);
  EXPECT_EQ(street.objects.size(), 9U);
}

TEST(World, InvalidDefinitionsAreRejectedBesideAValidControl) {
  const World valid(city());
  ASSERT_EQ(valid.snapshot().objects.size(), 10U);
  auto invalid = city();
  invalid.at(1).id = invalid.at(0).id;
  EXPECT_THROW((World(invalid)), std::invalid_argument);
  invalid = city();
  invalid.at(0).area.width = 0;
  EXPECT_THROW((World(invalid)), std::invalid_argument);
  invalid = city();
  invalid.at(0).destroyed_from_day = 0;
  EXPECT_THROW((World(invalid)), std::invalid_argument);
  invalid = city();
  invalid.at(0).exposed_from_day = 0;
  EXPECT_THROW((World(invalid)), std::invalid_argument);
  invalid = city();
  invalid.at(1).ingredient.reset();
  EXPECT_THROW((World(invalid)), std::invalid_argument);
  invalid = city();
  invalid.at(1).supported_by = NEW_SOURCE;
  EXPECT_THROW((World(invalid)), std::invalid_argument);
  invalid = city();
  invalid.at(0).ingredient = IngredientId{1};
  EXPECT_THROW((World(invalid)), std::invalid_argument);
}

TEST(WorldIntegration, ActorReturnPreservesDeterioration) {
  Simulation simulation({.exploration_budget = 10s, .seed = 123});
  World world(city());
  ASSERT_EQ(world.update(6), WorldUpdateResult::Applied);
  const auto damaged = world.snapshot();
  ASSERT_EQ(object(damaged, NPC).state, ObjectState::Destroyed);
  ASSERT_FALSE(object(damaged, LADDER).traversable);
  ASSERT_EQ(simulation.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(simulation.return_to_base(Actor::Chef), Result::Applied);
  ASSERT_EQ(world.update(simulation.snapshot().day + 5),
            WorldUpdateResult::Applied);
  EXPECT_EQ(world.snapshot().objects, damaged.objects);
  EXPECT_FALSE(object(world.snapshot(), LADDER).traversable);
  EXPECT_FALSE(object(world.snapshot(), NPC_SOURCE).material_available);
}

TEST(World, RejectsASourceWithNoIngredientAndAnIngredientOutsideASource) {
  const ObjectDefinition valid{.id = ObjectId{1},
                               .kind = ObjectKind::MaterialSource,
                               .ingredient = IngredientId{1}};
  EXPECT_NO_THROW(World({valid}));
  auto neither = valid;
  neither.ingredient = std::nullopt;
  EXPECT_THROW(World({neither}), std::invalid_argument);
  auto terrain = valid;
  terrain.kind = ObjectKind::Terrain;
  EXPECT_THROW(World({terrain}), std::invalid_argument);
}

} // namespace
} // namespace neocity::survival
