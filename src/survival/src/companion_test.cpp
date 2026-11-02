#include <survival/companion.hpp>

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace neocity::survival {
namespace {

using namespace std::chrono_literals;

constexpr ObjectId FLOOR{1};
constexpr ObjectId SOURCE{10};
constexpr ObjectId FAR_SOURCE{11};
constexpr ObjectId TOOL_SOURCE{12};
constexpr ObjectId WALL{20};
constexpr ObjectId DOOR{21};
constexpr IngredientId CARROT{70};
constexpr IngredientId ONION{71};
constexpr ToolId MAGNIFIER{80};
constexpr ToolId SPATULA{81};
constexpr ToolId WRENCH{82};
constexpr FoodId SOUP{90};
constexpr FoodId STEW{91};
constexpr GridPosition BASE{.x = -10, .y = -10};
constexpr GridPosition ENTRY{.x = 0, .y = 0};
constexpr AcidLevel UNFED_PENALTY{3};
constexpr ControlMap ALL_AI{.chef = Controller::Ai,
                            .appraiser = Controller::Ai};
constexpr std::array ACTORS{Actor::Chef, Actor::Appraiser};
constexpr PlayerInput NO_INPUT{};

ObjectDefinition floor_of(std::int32_t width, std::int32_t height = 1) {
  return {.id = FLOOR, .area = {.width = width, .height = height}};
}

ObjectDefinition ingredient_source(ObjectId id, std::int32_t x,
                                   std::int32_t y = 0) {
  return {.id = id,
          .kind = ObjectKind::MaterialSource,
          .area = {.x = x, .y = y},
          .ingredient = CARROT};
}

ExplorationConfig tuning(std::vector<SourceStock> sources) {
  return {.base = BASE,
          .entry = ENTRY,
          .maximum_stamina = 2,
          .collection_distance = 1,
          .sources = std::move(sources)};
}

WorkshopConfig kitchen(std::vector<Recipe> recipes = {
                           {.food = SOUP, .ingredients = {CARROT}}}) {
  return {.recipes = std::move(recipes),
          .magnifier = MAGNIFIER,
          .spatula = SPATULA,
          .new_food_score = 10,
          .new_ingredient_score = 5,
          .satisfying_score = 12,
          .disliked_acid_rise = AcidLevel{7},
          .unsatisfied_acid_rise = AcidLevel{2},
          .flavors = {}};
}

bool refused(const ActionResult &result) {
  return std::visit(
      [](auto value) {
        using Value = decltype(value);
        if constexpr (std::is_same_v<Value, WorkshopResult>) {
          return value != WorkshopResult::Applied &&
                 value != WorkshopResult::Failed;
        } else {
          return value != Value::Applied;
        }
      },
      result);
}

bool same_command(const ActorCommand &left, const ActorCommand &right) {
  return left.movement.direction == right.movement.direction &&
         left.movement.running == right.movement.running &&
         left.movement.jump == right.movement.jump &&
         left.action == right.action;
}

/// The real features under one controller loop, as a library integration.
class Game {
public:
  Game(const std::vector<ObjectDefinition> &city,
       const ExplorationConfig &places, const WorkshopConfig &work,
       std::chrono::microseconds budget = 3s, std::uint32_t seed = 11)
      : shared({.exploration_budget = budget, .seed = seed}), map(city),
        explorers(shared, map, places), kitchen_work(shared, work),
        controllers({.exploration = places,
                     .workshop = work,
                     .unfed_penalty = UNFED_PENALTY}) {}

  [[nodiscard]] Simulation &simulation() { return shared; }
  [[nodiscard]] World &world() { return map; }
  [[nodiscard]] Exploration &exploration() { return explorers; }
  [[nodiscard]] Workshop &workshop() { return kitchen_work; }
  [[nodiscard]] std::uint32_t refusals() const { return refused_actions; }

  [[nodiscard]] GameView view() const {
    return {.progression = shared.snapshot(),
            .world = map.snapshot(),
            .exploration = explorers.snapshot(),
            .workshop = kitchen_work.snapshot()};
  }

  void step(ControlMap control,
            const std::array<PlayerInput, 2> &inputs = {NO_INPUT, NO_INPUT},
            std::chrono::microseconds elapsed = 100ms) {
    std::array<ExplorationInput, 2> movement{};
    for (std::size_t index = 0; index < ACTORS.size(); ++index) {
      const auto actor = ACTORS.at(index);
      const auto command =
          controllers.command(actor, control, inputs.at(index), view());
      movement.at(index) = command.movement;
      if (command.action) {
        refused_actions += static_cast<std::uint32_t>(
            refused(apply(explorers, kitchen_work, actor, *command.action)));
      }
    }
    ASSERT_EQ(explorers.advance(elapsed, movement, shared.snapshot().acid),
              Result::Applied);
  }

private:
  Simulation shared;
  World map;
  Exploration explorers;
  Workshop kitchen_work;
  Companion controllers;
  std::uint32_t refused_actions = 0;
};

ObjectSnapshot object_of(ObjectId id, ObjectKind kind, std::int32_t x) {
  ObjectSnapshot object;
  object.id = id;
  object.kind = kind;
  object.area.x = x;
  return object;
}

/// Values for one decision: a floor at base, day one, with time remaining.
GameView base_view() {
  GameView view;
  view.progression.remaining_exploration = 5s;
  auto floor = object_of(FLOOR, ObjectKind::Terrain, 0);
  floor.area.width = 10;
  floor.traversable = true;
  view.world.objects.push_back(floor);
  view.exploration.chef.body.position = standing_in(BASE);
  view.exploration.appraiser.body.position = standing_in(BASE);
  return view;
}

GameView city_view(GridPosition position) {
  auto view = base_view();
  view.progression.phase = Phase::Exploration;
  view.progression.chef.location = Location::City;
  view.progression.appraiser.location = Location::City;
  view.exploration.chef.body.position = standing_in(position);
  view.exploration.appraiser.body.position = standing_in(position);
  return view;
}

void add_source(GameView &view, ObjectId id, std::int32_t x) {
  auto source = object_of(id, ObjectKind::MaterialSource, x);
  source.material_available = true;
  source.ingredient = CARROT;
  view.world.objects.push_back(source);
  view.exploration.sources.push_back({.id = id});
}

/// The chef at `position`, carrying the wrench for a broken door at x 1 in a
/// wall too tall to jump.
GameView view_with_broken_door(GridPosition position) {
  auto view = city_view(position);
  auto door = object_of(DOOR, ObjectKind::Entrance, 1);
  door.state = ObjectState::Destroyed;
  door.repair_with = WRENCH;
  view.world.objects.push_back(door);
  auto wall = object_of(WALL, ObjectKind::Building, 1);
  wall.area.y = -1;
  view.world.objects.push_back(wall);
  view.progression.chef.tools = {WRENCH};
  return view;
}

Companion companion_with(const WorkshopConfig &work = kitchen()) {
  return Companion({.exploration = tuning({}),
                    .workshop = work,
                    .unfed_penalty = UNFED_PENALTY});
}

std::optional<Action> ai_action(const GameView &view, Actor actor,
                                ControlMap control = ALL_AI) {
  return companion_with().command(actor, control, NO_INPUT, view).action;
}

/// The cell an explorer's feet stand in.
GridPosition at(const ExplorerSnapshot &explorer) {
  return cell_of(explorer.body.position);
}

TEST(Grid, DistanceCountsFromTheNearestCellOfAnAreaOnEverySide) {
  constexpr WorldArea AREA{.x = 2, .y = 2, .width = 3, .height = 3};
  EXPECT_EQ(distance({.x = 3, .y = 3}, AREA), 0);
  EXPECT_EQ(distance({.x = 4, .y = 4}, AREA), 0);
  EXPECT_EQ(distance({.x = 0, .y = 3}, AREA), 2);
  EXPECT_EQ(distance({.x = 6, .y = 3}, AREA), 2);
  EXPECT_EQ(distance({.x = 3, .y = 0}, AREA), 2);
  EXPECT_EQ(distance({.x = 3, .y = 6}, AREA), 2);
  EXPECT_EQ(distance({.x = 6, .y = 6}, AREA), 4);
}

TEST(Control, SinglePlayerMakesTheChosenActorHumanAndTheOtherAi) {
  EXPECT_EQ(
      single_player(Actor::Chef),
      (ControlMap{.chef = Controller::Human, .appraiser = Controller::Ai}));
  EXPECT_EQ(
      single_player(Actor::Appraiser),
      (ControlMap{.chef = Controller::Ai, .appraiser = Controller::Human}));
}

TEST(Control, SwitchingMovesTheLoneHumanButLeavesTwoHumansInPlace) {
  EXPECT_EQ(switch_control(single_player(Actor::Chef)),
            single_player(Actor::Appraiser));
  EXPECT_EQ(switch_control(two_players()), two_players());
}

TEST(Control, LeavingGivesOnlyThatActorToAiWithoutMovingAHuman) {
  EXPECT_EQ(leave(two_players(), Actor::Chef), single_player(Actor::Appraiser));
  EXPECT_EQ(leave(two_players(), Actor::Appraiser), single_player(Actor::Chef));
  EXPECT_EQ(leave(single_player(Actor::Chef), Actor::Chef), ALL_AI);
  EXPECT_EQ(leave(single_player(Actor::Chef), Actor::Appraiser),
            single_player(Actor::Chef));
}

TEST(Control, TheLoneHumanHoldsTheFirstKeySetOnEitherActor) {
  EXPECT_EQ(key_set(single_player(Actor::Chef), Actor::Chef), 0U);
  EXPECT_EQ(key_set(single_player(Actor::Chef), Actor::Appraiser),
            std::nullopt);
  EXPECT_EQ(key_set(single_player(Actor::Appraiser), Actor::Appraiser), 0U);
  EXPECT_EQ(key_set(single_player(Actor::Appraiser), Actor::Chef),
            std::nullopt);
}

TEST(Control, TwoHumansHoldOneKeySetEachAndAiHoldsNone) {
  EXPECT_EQ(key_set(two_players(), Actor::Chef), 0U);
  EXPECT_EQ(key_set(two_players(), Actor::Appraiser), 1U);
  EXPECT_EQ(key_set(ALL_AI, Actor::Chef), std::nullopt);
  EXPECT_EQ(key_set(ALL_AI, Actor::Appraiser), std::nullopt);
}

TEST(Companion, WalksToASourceAndCollectsWithinRange) {
  Game game({floor_of(10), ingredient_source(SOURCE, 3)},
            tuning({{.id = SOURCE}}), kitchen());
  game.step(ALL_AI);
  EXPECT_EQ(at(game.exploration().snapshot().chef), ENTRY);
  game.step(ALL_AI);
  EXPECT_EQ(at(game.exploration().snapshot().chef), (GridPosition{.x = 1}));
  EXPECT_EQ(at(game.exploration().snapshot().appraiser),
            (GridPosition{.x = 1}));
  game.step(ALL_AI);
  EXPECT_EQ(at(game.exploration().snapshot().chef), (GridPosition{.x = 2}));
  ASSERT_TRUE(game.simulation().snapshot().chef.inventory.empty());
  game.step(ALL_AI);
  EXPECT_EQ(game.simulation().snapshot().chef.inventory,
            std::vector<IngredientId>{CARROT});
  EXPECT_EQ(game.exploration().snapshot().sources.front().remaining, 0U);
  EXPECT_EQ(game.refusals(), 0U);
}

TEST(Companion, JumpsAGapAndClimbsALadderToAHighSourceWithoutFalling) {
  const std::vector<ObjectDefinition> city{
      {.id = FLOOR, .area = {.y = 3, .width = 4}},
      {.id = ObjectId{2}, .area = {.x = 6, .y = 3, .width = 3}},
      {.id = ObjectId{3},
       .kind = ObjectKind::Ladder,
       .area = {.x = 8, .height = 4}},
      {.id = ObjectId{4}, .area = {.x = 7, .width = 4}},
      ingredient_source(SOURCE, 10)};
  auto places = tuning({{.id = SOURCE}});
  places.entry = {.y = 3};
  Game game(city, places, kitchen(), 30s);
  game.step(ALL_AI);
  bool jumped = false;
  bool climbed = false;
  while (game.simulation().snapshot().chef.inventory.empty()) {
    ASSERT_EQ(game.simulation().snapshot().chef.location, Location::City);
    ASSERT_GT(game.simulation().snapshot().remaining_exploration, 0us);
    game.step(ALL_AI);
    const auto stance = game.exploration().snapshot().chef.body.stance;
    jumped = jumped || stance == Stance::Airborne;
    climbed = climbed || stance == Stance::Climbing;
  }
  EXPECT_TRUE(jumped);
  EXPECT_TRUE(climbed);
  EXPECT_EQ(game.exploration().snapshot().sources.front().remaining, 0U);
}
TEST(Companion, JumpsALowBuildingAndWalksThroughATallOnesOpenEntrance) {
  const std::vector<ObjectDefinition> over{
      {.id = FLOOR, .area = {.y = 1, .width = 6}},
      {.id = WALL, .kind = ObjectKind::Building, .area = {.x = 2, .y = 1}},
      ingredient_source(SOURCE, 5, 1)};
  auto places = tuning({{.id = SOURCE}});
  places.entry = {.y = 1};
  Game jumped(over, places, kitchen());
  const std::vector<ObjectDefinition> through{
      {.id = FLOOR, .area = {.y = 1, .width = 6}},
      {.id = WALL,
       .kind = ObjectKind::Building,
       .area = {.x = 2, .y = -1, .height = 3}},
      {.id = DOOR, .kind = ObjectKind::Entrance, .area = {.x = 2, .y = 1}},
      ingredient_source(SOURCE, 5, 1)};
  Game entered(through, places, kitchen());
  for (int tick = 0; tick < 12; ++tick) {
    jumped.step(ALL_AI);
    entered.step(ALL_AI);
  }
  EXPECT_EQ(jumped.exploration().snapshot().sources.front().remaining, 0U);
  EXPECT_EQ(entered.exploration().snapshot().sources.front().remaining, 0U);
}
TEST(Companion, ABrokenEntranceBlocksTheRouteUnlessItCanBeJumped) {
  const auto broken = [](std::int32_t height) {
    return std::vector<ObjectDefinition>{
        {.id = FLOOR, .area = {.y = 1, .width = 6}},
        {.id = WALL,
         .kind = ObjectKind::Building,
         .area = {.x = 2, .y = 2 - height, .height = height}},
        {.id = DOOR,
         .kind = ObjectKind::Entrance,
         .area = {.x = 2, .y = 1},
         .destroyed_after = WorldThreshold{.acid = AcidLevel{1}},
         .repair_with = WRENCH},
        ingredient_source(SOURCE, 5, 1)};
  };
  auto places = tuning({{.id = SOURCE}});
  places.entry = {.y = 1};
  Game sealed(broken(3), places, kitchen());
  ASSERT_EQ(sealed.world().update({.acid = AcidLevel{1}}),
            WorldUpdateResult::Applied);
  Game hurdle(broken(1), places, kitchen());
  ASSERT_EQ(hurdle.world().update({.acid = AcidLevel{1}}),
            WorldUpdateResult::Applied);
  sealed.step(ALL_AI);
  hurdle.step(ALL_AI);
  EXPECT_EQ(sealed.simulation().snapshot().chef.location, Location::Base);
  EXPECT_EQ(sealed.simulation().snapshot().day, 2U);
  EXPECT_EQ(hurdle.simulation().snapshot().chef.location, Location::City);
}
TEST(Companion, ADestroyedBuildingNoLongerBlocksItsCell) {
  auto view = city_view({.x = 2});
  add_source(view, SOURCE, 9);
  auto building = object_of(WALL, ObjectKind::Building, 3);
  building.area.y = -1;
  building.area.height = 2;
  view.world.objects.push_back(building);
  ASSERT_EQ(ai_action(view, Actor::Chef), Action{ReturnToBase{}});
  view.world.objects.back().state = ObjectState::Destroyed;
  EXPECT_EQ(companion_with()
                .command(Actor::Chef, ALL_AI, NO_INPUT, view)
                .movement.direction,
            Direction::Right);
}

TEST(Companion, ACollectableSourceComesBeforeANearerRepair) {
  auto view = view_with_broken_door({.x = 3});
  add_source(view, SOURCE, 9);
  add_source(view, FAR_SOURCE, 0);
  const auto heading = [&] {
    return companion_with()
        .command(Actor::Chef, ALL_AI, NO_INPUT, view)
        .movement.direction;
  };
  EXPECT_EQ(heading(), Direction::Right);
  view.exploration.sources.front().remaining = 0;
  EXPECT_EQ(heading(), Direction::Left);
}

TEST(Companion, ARepairIsAnErrandOnlyWhileItOpensTheWayToStock) {
  auto view = view_with_broken_door({.x = 3});
  add_source(view, FAR_SOURCE, 0);
  ASSERT_EQ(CellMap(view.world).at({.x = 1}), Cell::Blocked);
  const auto errand = companion_with().errand(Actor::Chef, view);
  ASSERT_TRUE(errand.has_value());
  EXPECT_EQ(errand.value_or(Errand{}).target, DOOR);
  view.exploration.sources.front().remaining = 0;
  EXPECT_FALSE(companion_with().errand(Actor::Chef, view).has_value());
  EXPECT_EQ(ai_action(view, Actor::Chef), Action{ReturnToBase{}});
}

TEST(Companion, AHumanStillRepairsAPassageThatOpensNothing) {
  const auto view = view_with_broken_door({.x = 2});
  EXPECT_EQ(companion_with()
                .command(Actor::Chef, single_player(Actor::Chef),
                         {.interact = true}, view)
                .action,
            (Action{Repair{.target = DOOR, .tool = WRENCH}}));
}

TEST(Companion, SettingOutIsWorthItOnlyForStockARepairCanReach) {
  auto view = view_with_broken_door({.x = 3});
  view.progression.phase = Phase::Workshop;
  view.progression.chef.location = Location::Base;
  view.progression.appraiser.location = Location::Base;
  view.world.objects.at(1).area.x = 2;
  view.world.objects.at(2).area.x = 2;
  add_source(view, FAR_SOURCE, 5);
  const CompanionConfig config{.exploration = tuning({}),
                               .workshop = kitchen(),
                               .unfed_penalty = UNFED_PENALTY};
  ASSERT_EQ(CellMap(view.world).at({.x = 2}), Cell::Blocked);
  EXPECT_TRUE(worth_entering(config, Actor::Chef, view));
  EXPECT_FALSE(worth_entering(config, Actor::Appraiser, view));
  view.exploration.sources.front().remaining = 0;
  EXPECT_FALSE(worth_entering(config, Actor::Chef, view));
}

TEST(Companion, ADormantPassageDoesNotBlockItsCell) {
  Game game({floor_of(5),
             {.id = DOOR,
              .kind = ObjectKind::Entrance,
              .area = {.x = 2},
              .exposed_at = WorldThreshold{.elapsed = 1h}},
             ingredient_source(SOURCE, 4)},
            tuning({{.id = SOURCE}}), kitchen());
  ASSERT_EQ(game.view().world.objects.at(1).state, ObjectState::Dormant);
  for (int tick = 0; tick < 8; ++tick) {
    game.step(ALL_AI);
  }
  EXPECT_EQ(game.exploration().snapshot().sources.front().remaining, 0U);
}

TEST(Companion, SkipsDepletedAndUnavailableSourcesForOneItCanUse) {
  auto view = city_view({.x = 5});
  add_source(view, SOURCE, 3);
  add_source(view, FAR_SOURCE, 9);
  const auto companion = companion_with();
  const auto heading = [&](const GameView &candidate) {
    return companion.command(Actor::Chef, ALL_AI, NO_INPUT, candidate)
        .movement.direction;
  };
  ASSERT_EQ(heading(view), Direction::Left);
  auto depleted = view;
  depleted.exploration.sources.front().remaining = 0;
  EXPECT_EQ(heading(depleted), Direction::Right);
  auto unavailable = view;
  unavailable.world.objects.at(1).material_available = false;
  EXPECT_EQ(heading(unavailable), Direction::Right);
}

TEST(Companion, ReturnsToBaseWhenNothingInTheCityIsReachable) {
  auto view = city_view({.x = 5});
  add_source(view, SOURCE, 7);
  ASSERT_EQ(ai_action(view, Actor::Chef), std::nullopt);
  view.world.objects.at(1).material_available = false;
  EXPECT_EQ(ai_action(view, Actor::Chef), Action{ReturnToBase{}});
}

TEST(Companion, EntersOnlyWithTimeASafeEntryAndSomethingToDo) {
  auto view = base_view();
  add_source(view, SOURCE, 1);
  ASSERT_EQ(ai_action(view, Actor::Chef), Action{Enter{}});
  auto no_time = view;
  no_time.progression.remaining_exploration = 0us;
  EXPECT_EQ(ai_action(no_time, Actor::Chef),
            Action{EndDayUnfed{UNFED_PENALTY}});
  auto no_entry = view;
  no_entry.world.objects.front().traversable = false;
  EXPECT_EQ(ai_action(no_entry, Actor::Chef),
            Action{EndDayUnfed{UNFED_PENALTY}});
  auto nothing = view;
  nothing.exploration.sources.clear();
  EXPECT_EQ(ai_action(nothing, Actor::Chef),
            Action{EndDayUnfed{UNFED_PENALTY}});
}

TEST(Companion, UsesACarriedToolOnABrokenPassageOnlyWhenNothingIsCollectable) {
  const ObjectDefinition door{.id = DOOR,
                              .kind = ObjectKind::Entrance,
                              .area = {.x = 3},
                              .destroyed_after =
                                  WorldThreshold{.acid = AcidLevel{1}},
                              .repair_with = WRENCH};
  const ObjectDefinition wall{
      .id = WALL, .kind = ObjectKind::Building, .area = {.x = 3, .y = -1}};
  const std::vector<ObjectDefinition> city{floor_of(6),
                                           door,
                                           wall,
                                           ingredient_source(SOURCE, 5),
                                           {.id = TOOL_SOURCE,
                                            .kind = ObjectKind::MaterialSource,
                                            .area = {.x = 1},
                                            .tool = WRENCH}};
  Game equipped(city, tuning({{.id = SOURCE}, {.id = TOOL_SOURCE}}), kitchen());
  ASSERT_EQ(equipped.world().update({.acid = AcidLevel{1}}),
            WorldUpdateResult::Applied);
  for (int tick = 0; tick < 12; ++tick) {
    equipped.step(ALL_AI);
  }
  EXPECT_EQ(equipped.simulation().snapshot().chef.tools,
            std::vector<ToolId>{WRENCH});
  EXPECT_EQ(equipped.exploration().snapshot().sources.front().remaining, 0U);
  EXPECT_EQ(equipped.refusals(), 0U);

  Game unequipped(city, tuning({{.id = SOURCE}}), kitchen());
  ASSERT_EQ(unequipped.world().update({.acid = AcidLevel{1}}),
            WorldUpdateResult::Applied);
  unequipped.step(ALL_AI);
  EXPECT_EQ(unequipped.simulation().snapshot().chef.location, Location::Base);
  EXPECT_EQ(unequipped.exploration().snapshot().sources.front().remaining, 1U);
}

TEST(Companion, WaitsAtTheWorkshopForAHumanToSetOutOrEndTheDay) {
  auto view = base_view();
  add_source(view, SOURCE, 3);
  ASSERT_EQ(ai_action(view, Actor::Appraiser), Action{Enter{}});
  EXPECT_EQ(ai_action(view, Actor::Appraiser, single_player(Actor::Chef)),
            std::nullopt);
  view.exploration.sources.clear();
  ASSERT_EQ(ai_action(view, Actor::Chef), Action{EndDayUnfed{UNFED_PENALTY}});
  EXPECT_EQ(ai_action(view, Actor::Chef, single_player(Actor::Appraiser)),
            std::nullopt);
}

TEST(Companion, FollowsAHumanOutAndBackSoTheWorkshopIsNeverHeldShut) {
  Game game({floor_of(10), ingredient_source(SOURCE, 8)},
            tuning({{.id = SOURCE}}), kitchen());
  const auto control = single_player(Actor::Chef);
  game.step(control);
  ASSERT_EQ(game.simulation().snapshot().phase, Phase::Workshop);
  game.step(control, {PlayerInput{.travel = true}, NO_INPUT});
  ASSERT_EQ(game.simulation().snapshot().chef.location, Location::City);
  EXPECT_EQ(game.simulation().snapshot().appraiser.location, Location::City);
  game.step(control, {PlayerInput{.travel = true}, NO_INPUT});
  ASSERT_EQ(game.simulation().snapshot().chef.location, Location::Base);
  EXPECT_EQ(game.simulation().snapshot().appraiser.location, Location::Base);
  EXPECT_EQ(game.simulation().snapshot().phase, Phase::Workshop);
}

TEST(Companion, KeepsCollectingWhileItsHumanPartnerStaysOut) {
  Game game({floor_of(10), ingredient_source(SOURCE, 2)},
            tuning({{.id = SOURCE}}), kitchen());
  const auto control = single_player(Actor::Chef);
  game.step(control, {PlayerInput{.travel = true}, NO_INPUT});
  for (int tick = 0; tick < 3; ++tick) {
    game.step(control);
  }
  EXPECT_EQ(game.simulation().snapshot().appraiser.inventory,
            std::vector<IngredientId>{CARROT});
  game.step(control);
  EXPECT_EQ(game.simulation().snapshot().chef.location, Location::City);
  EXPECT_EQ(game.simulation().snapshot().appraiser.location, Location::Base);
}

TEST(Companion, TheAppraiserAppraisesEachCarriedIngredientKindOnce) {
  auto view = base_view();
  view.progression.chef.inventory = {CARROT};
  view.progression.appraiser.inventory = {ONION};
  EXPECT_EQ(ai_action(view, Actor::Appraiser), Action{Appraise{CARROT}});
  view.workshop.identified = {CARROT};
  EXPECT_EQ(ai_action(view, Actor::Appraiser), Action{Appraise{ONION}});
  view.workshop.identified = {CARROT, ONION};
  EXPECT_EQ(ai_action(view, Actor::Appraiser), std::nullopt);
}

TEST(Companion, TheChefCooksOnlyWhatIsIdentifiedAndCarried) {
  auto view = base_view();
  view.progression.appraiser.inventory = {CARROT};
  view.workshop.identified = {CARROT};
  ASSERT_EQ(ai_action(view, Actor::Chef), Action{Cook{SOUP}});
  auto unidentified = view;
  unidentified.workshop.identified.clear();
  EXPECT_FALSE(std::holds_alternative<Cook>(
      ai_action(unidentified, Actor::Chef).value_or(Enter{})));
  auto missing = view;
  missing.progression.appraiser.inventory.clear();
  EXPECT_NE(ai_action(missing, Actor::Chef), Action{Cook{SOUP}});
}

TEST(Companion, ARecipeNeedsEveryRepeatedIngredientCarried) {
  auto view = base_view();
  view.progression.chef.inventory = {CARROT};
  view.workshop.identified = {CARROT};
  const auto companion = companion_with(
      kitchen({{.food = STEW, .ingredients = {CARROT, CARROT}}}));
  const auto action = [&] {
    return companion.command(Actor::Chef, ALL_AI, NO_INPUT, view).action;
  };
  EXPECT_NE(action(), Action{Cook{STEW}});
  view.progression.appraiser.inventory = {CARROT};
  EXPECT_EQ(action(), Action{Cook{STEW}});
}

TEST(Companion, ARoleActsOnlyInItsOwnRole) {
  auto view = base_view();
  view.progression.chef.inventory = {CARROT};
  ASSERT_EQ(ai_action(view, Actor::Appraiser), Action{Appraise{CARROT}});
  EXPECT_EQ(ai_action(view, Actor::Chef), std::nullopt);
  view.workshop.identified = {CARROT};
  ASSERT_EQ(ai_action(view, Actor::Chef), Action{Cook{SOUP}});
  EXPECT_EQ(ai_action(view, Actor::Appraiser), std::nullopt);
}

TEST(Companion, PrefersAnUneatenDishAndStillCooksARepeatRatherThanNothing) {
  auto view = base_view();
  view.progression.chef.inventory = {CARROT};
  view.workshop.identified = {CARROT};
  const auto companion =
      companion_with(kitchen({{.food = SOUP, .ingredients = {CARROT}},
                              {.food = STEW, .ingredients = {CARROT}}}));
  const auto action = [&] {
    return companion.command(Actor::Chef, ALL_AI, NO_INPUT, view).action;
  };
  EXPECT_EQ(action(), Action{Cook{SOUP}});
  view.workshop.eaten_foods = {SOUP};
  EXPECT_EQ(action(), Action{Cook{STEW}});
  view.workshop.eaten_foods = {SOUP, STEW};
  EXPECT_EQ(action(), Action{Cook{SOUP}});
}

TEST(Companion, AHiddenDislikeDoesNotChangeWhatItCooks) {
  auto view = base_view();
  view.progression.chef.inventory = {CARROT};
  view.workshop.identified = {CARROT};
  const auto disliked = companion_with(
      kitchen({{.food = SOUP, .ingredients = {CARROT}, .disliked = true},
               {.food = STEW, .ingredients = {CARROT}}}));
  EXPECT_EQ(disliked.command(Actor::Chef, ALL_AI, NO_INPUT, view).action,
            Action{Cook{SOUP}});
}

TEST(Companion, OneStoredDishStopsTheChefCookingAnother) {
  auto view = base_view();
  view.progression.chef.inventory = {CARROT};
  view.workshop.identified = {CARROT};
  ASSERT_EQ(ai_action(view, Actor::Chef), Action{Cook{SOUP}});
  view.workshop.foods = {{.food = SOUP}};
  EXPECT_EQ(ai_action(view, Actor::Chef), Action{Feed{SOUP}});
}

TEST(Companion, ReplacesItsOwnDamagedToolWithOneEitherActorCarries) {
  auto view = base_view();
  view.workshop.magnifier = ToolState::Damaged;
  view.progression.chef.tools = {MAGNIFIER};
  EXPECT_EQ(ai_action(view, Actor::Appraiser),
            Action{ReplaceTool{WorkshopTool::Magnifier}});
  EXPECT_NE(ai_action(view, Actor::Chef),
            Action{ReplaceTool{WorkshopTool::Magnifier}});
  view.workshop.magnifier = ToolState::Intact;
  view.workshop.spatula = ToolState::Damaged;
  view.progression.chef.tools = {};
  view.progression.appraiser.tools = {SPATULA};
  EXPECT_EQ(ai_action(view, Actor::Chef),
            Action{ReplaceTool{WorkshopTool::Spatula}});
}

TEST(Companion, ReplacesNothingIntactOrWithoutTheMatchingTool) {
  auto view = base_view();
  view.progression.chef.tools = {MAGNIFIER};
  EXPECT_EQ(ai_action(view, Actor::Appraiser), std::nullopt);
  view.workshop.magnifier = ToolState::Damaged;
  view.progression.chef.tools = {SPATULA};
  EXPECT_EQ(ai_action(view, Actor::Appraiser), std::nullopt);
  view.progression.chef.tools = {MAGNIFIER};
  EXPECT_EQ(ai_action(view, Actor::Appraiser),
            Action{ReplaceTool{WorkshopTool::Magnifier}});
}

TEST(Companion, OnlyTheChefEndsAnAiDayAndFeedsAnUneatenDishFirst) {
  auto view = base_view();
  view.workshop.foods = {{.food = SOUP}, {.food = STEW}};
  view.workshop.eaten_foods = {SOUP};
  EXPECT_EQ(ai_action(view, Actor::Chef), Action{Feed{STEW}});
  EXPECT_EQ(ai_action(view, Actor::Appraiser), std::nullopt);
  view.workshop.foods = {{.food = SOUP}};
  EXPECT_EQ(ai_action(view, Actor::Chef), Action{Feed{SOUP}});
  view.workshop.foods = {};
  EXPECT_EQ(ai_action(view, Actor::Chef), Action{EndDayUnfed{UNFED_PENALTY}});
}

TEST(Companion, NeitherActorLeavesOrEndsTheDayWhileThePartnerHasBaseWork) {
  auto view = base_view();
  add_source(view, SOURCE, 3);
  ASSERT_EQ(ai_action(view, Actor::Chef), Action{Enter{}});
  view.progression.appraiser.inventory = {CARROT};
  EXPECT_EQ(ai_action(view, Actor::Chef), std::nullopt);
  EXPECT_EQ(ai_action(view, Actor::Appraiser), Action{Appraise{CARROT}});
}

TEST(Companion, DoesNothingOnceEscaped) {
  auto view = base_view();
  add_source(view, SOURCE, 3);
  view.progression.phase = Phase::Escaped;
  EXPECT_EQ(ai_action(view, Actor::Chef), std::nullopt);
  EXPECT_EQ(ai_action(view, Actor::Appraiser), std::nullopt);
}

TEST(Companion, AiIgnoresPressesAndAHumanDoesOnlyWhatWasPressed) {
  auto view = base_view();
  add_source(view, SOURCE, 3);
  const auto companion = companion_with();
  const PlayerInput pressed{.travel = true};
  EXPECT_EQ(
      companion
          .command(Actor::Chef, single_player(Actor::Appraiser), pressed, view)
          .action,
      std::nullopt);
  EXPECT_EQ(
      companion.command(Actor::Chef, single_player(Actor::Chef), pressed, view)
          .action,
      Action{Enter{}});
  EXPECT_EQ(
      companion.command(Actor::Chef, single_player(Actor::Chef), NO_INPUT, view)
          .action,
      std::nullopt);
}

TEST(Companion, AHumanCollectsOnlyWithinRangeAndMovesAsPressed) {
  auto view = city_view({.x = 2});
  add_source(view, SOURCE, 3);
  const auto companion = companion_with();
  const PlayerInput pressed{
      .movement = {.direction = Direction::Right, .running = true},
      .interact = true};
  const auto near =
      companion.command(Actor::Chef, two_players(), pressed, view);
  EXPECT_EQ(near.action, Action{Collect{SOURCE}});
  EXPECT_EQ(near.movement.direction, Direction::Right);
  EXPECT_TRUE(near.movement.running);
  view.exploration.chef.body.position = standing_in({.x = 0});
  EXPECT_EQ(companion.command(Actor::Chef, two_players(), pressed, view).action,
            std::nullopt);
}

TEST(Companion, AHumanRepairsWithTheirOwnToolWhenNothingIsCollectableInRange) {
  auto view = city_view({.x = 2});
  auto door = object_of(DOOR, ObjectKind::Entrance, 3);
  door.state = ObjectState::Destroyed;
  door.repair_with = WRENCH;
  view.world.objects.push_back(door);
  const auto companion = companion_with();
  const PlayerInput pressed{.interact = true};
  EXPECT_EQ(companion.command(Actor::Chef, two_players(), pressed, view).action,
            std::nullopt);
  view.progression.chef.tools = {WRENCH};
  EXPECT_EQ(companion.command(Actor::Chef, two_players(), pressed, view).action,
            (Action{Repair{.target = DOOR, .tool = WRENCH}}));
  EXPECT_EQ(
      companion.command(Actor::Appraiser, two_players(), pressed, view).action,
      std::nullopt);
}

TEST(Companion, TravelLeavesFromBaseAndReturnsFromTheCity) {
  const auto companion = companion_with();
  const PlayerInput pressed{.travel = true};
  EXPECT_EQ(
      companion.command(Actor::Appraiser, two_players(), pressed, base_view())
          .action,
      Action{Enter{}});
  EXPECT_EQ(companion
                .command(Actor::Appraiser, two_players(), pressed,
                         city_view({.x = 1}))
                .action,
            Action{ReturnToBase{}});
}

TEST(Companion, AHumanInteractsAtBaseThroughTheirRole) {
  auto view = base_view();
  view.progression.chef.inventory = {CARROT};
  const auto companion = companion_with();
  const PlayerInput pressed{.interact = true};
  EXPECT_EQ(
      companion.command(Actor::Appraiser, two_players(), pressed, view).action,
      Action{Appraise{CARROT}});
  EXPECT_EQ(companion.command(Actor::Chef, two_players(), pressed, view).action,
            std::nullopt);
  view.workshop.identified = {CARROT};
  view.workshop.foods = {{.food = SOUP}};
  EXPECT_EQ(companion.command(Actor::Chef, two_players(), pressed, view).action,
            Action{Cook{SOUP}});
}

TEST(Companion, AHumanReplacesTheirRoleToolFeedsAndEndsTheDayOnRequest) {
  auto view = base_view();
  view.workshop.foods = {{.food = SOUP}};
  const auto companion = companion_with();
  const auto press = [&](Actor actor, const PlayerInput &input) {
    return companion.command(actor, two_players(), input, view).action;
  };
  EXPECT_EQ(press(Actor::Appraiser, {.replace_tool = true}),
            Action{ReplaceTool{WorkshopTool::Magnifier}});
  EXPECT_EQ(press(Actor::Chef, {.replace_tool = true}),
            Action{ReplaceTool{WorkshopTool::Spatula}});
  EXPECT_EQ(press(Actor::Appraiser, {.feed = true}), Action{Feed{SOUP}});
  EXPECT_EQ(press(Actor::Appraiser, {.end_day = true}),
            Action{EndDayUnfed{UNFED_PENALTY}});
  view.workshop.foods = {};
  EXPECT_EQ(press(Actor::Chef, {.feed = true}), std::nullopt);
}

TEST(Companion, AnActionStillFacesTheRulesOfItsFeature) {
  Game game({floor_of(4), ingredient_source(SOURCE, 3)},
            tuning({{.id = SOURCE}}), kitchen());
  EXPECT_EQ(
      apply(game.exploration(), game.workshop(), Actor::Chef, Appraise{CARROT}),
      ActionResult{WorkshopResult::WrongRole});
  EXPECT_EQ(
      apply(game.exploration(), game.workshop(), Actor::Appraiser, Cook{SOUP}),
      ActionResult{WorkshopResult::WrongRole});
  EXPECT_EQ(apply(game.exploration(), game.workshop(), Actor::Chef, Feed{SOUP}),
            ActionResult{WorkshopResult::NoFood});
  EXPECT_EQ(apply(game.exploration(), game.workshop(), Actor::Chef,
                  ReplaceTool{WorkshopTool::Spatula}),
            ActionResult{WorkshopResult::ToolIntact});
  EXPECT_EQ(
      apply(game.exploration(), game.workshop(), Actor::Chef, Collect{SOURCE}),
      ActionResult{CollectionResult::NotExploring});
  EXPECT_EQ(apply(game.exploration(), game.workshop(), Actor::Chef,
                  Repair{.target = DOOR, .tool = WRENCH}),
            ActionResult{RepairResult::NotExploring});
  EXPECT_EQ(
      apply(game.exploration(), game.workshop(), Actor::Chef, ReturnToBase{}),
      ActionResult{Result::NotExploring});
  ASSERT_EQ(apply(game.exploration(), game.workshop(), Actor::Chef, Enter{}),
            ActionResult{Result::Applied});
  EXPECT_EQ(
      apply(game.exploration(), game.workshop(), Actor::Chef, Collect{SOURCE}),
      ActionResult{CollectionResult::OutOfRange});
  EXPECT_EQ(apply(game.exploration(), game.workshop(), Actor::Chef,
                  EndDayUnfed{UNFED_PENALTY}),
            ActionResult{WorkshopResult::NotInWorkshop});
}

TEST(Companion, ASecondCommandOnTheSameViewFindsTheSameDecision) {
  auto view = city_view({.x = 2});
  add_source(view, SOURCE, 8);
  const auto companion = companion_with();
  const auto first = companion.command(Actor::Chef, ALL_AI, NO_INPUT, view);
  EXPECT_EQ(first.movement.direction, Direction::Right);
  EXPECT_TRUE(same_command(
      first, companion.command(Actor::Chef, ALL_AI, NO_INPUT, view)));
}

TEST(Companion, ALeftActorKeepsItsStateAndAiCarriesOnFromIt) {
  Game game({floor_of(10), ingredient_source(SOURCE, 3)},
            tuning({{.id = SOURCE}}), kitchen());
  const std::array<PlayerInput, 2> out{PlayerInput{.travel = true},
                                       PlayerInput{.travel = true}};
  game.step(two_players(), out);
  const std::array<PlayerInput, 2> right{
      PlayerInput{.movement = {.direction = Direction::Right}},
      PlayerInput{.movement = {.direction = Direction::Right}}};
  game.step(two_players(), right);
  const auto before = game.view();
  ASSERT_EQ(at(before.exploration.chef), (GridPosition{.x = 1}));
  const auto control = leave(two_players(), Actor::Chef);
  ASSERT_EQ(game.view().progression, before.progression);
  game.step(control, {NO_INPUT, NO_INPUT});
  EXPECT_EQ(at(game.exploration().snapshot().chef), (GridPosition{.x = 2}));
  EXPECT_EQ(at(game.exploration().snapshot().appraiser),
            (GridPosition{.x = 1}));
  game.step(control, {NO_INPUT, NO_INPUT});
  EXPECT_EQ(game.simulation().snapshot().chef.inventory,
            std::vector<IngredientId>{CARROT});
}

TEST(Companion, SwitchingHandsTheFormerHumanActorToAiMidRun) {
  Game game({floor_of(10), ingredient_source(SOURCE, 2)},
            tuning({{.id = SOURCE}}), kitchen());
  const auto control = single_player(Actor::Chef);
  game.step(control, {PlayerInput{.travel = true}, NO_INPUT});
  game.step(control);
  ASSERT_EQ(at(game.exploration().snapshot().chef), ENTRY);
  ASSERT_NE(at(game.exploration().snapshot().appraiser), ENTRY);
  const auto switched = switch_control(control);
  game.step(switched);
  EXPECT_NE(at(game.exploration().snapshot().chef), ENTRY);
}

/// Steps until `day` arrives, asserting a bound so a stuck AI fails the test.
void play_until_day(Game &game, std::uint32_t day, ControlMap control) {
  constexpr int MAXIMUM_STEPS = 20000;
  for (int step = 0; step < MAXIMUM_STEPS; ++step) {
    if (game.simulation().snapshot().day >= day) {
      return;
    }
    game.step(control);
  }
  FAIL() << "the party never reached day " << day;
}

TEST(CompanionRun, AnAiPartyFeedsTheMonsterAcrossDaysWithoutCreatingAnything) {
  const auto recipes = kitchen({{.food = SOUP, .ingredients = {CARROT}}});
  Game game({floor_of(10),
             ingredient_source(SOURCE, 3),
             {.id = FAR_SOURCE,
              .kind = ObjectKind::MaterialSource,
              .area = {.x = 6},
              .tool = SPATULA}},
            tuning({{.id = SOURCE, .remaining = 20},
                    {.id = FAR_SOURCE, .remaining = 2}}),
            recipes);
  game.workshop().damage_tool(WorkshopTool::Spatula);
  constexpr std::uint32_t LAST_DAY = 8;
  std::uint32_t day = 1;
  std::uint64_t expected_acid = 0;
  std::uint64_t day_time = 0;
  for (int step = 0; step < 20000 && day < LAST_DAY; ++step) {
    const auto before = game.view();
    game.step(ALL_AI);
    const auto after = game.view();
    day_time +=
        static_cast<std::uint64_t>((before.progression.remaining_exploration -
                                    after.progression.remaining_exploration)
                                       .count());
    if (after.progression.day != day) {
      ASSERT_EQ(after.progression.day, day + 1);
      ASSERT_TRUE(after.workshop.last_day.has_value());
      expected_acid +=
          after.workshop.last_day.value_or(DayOutcome{}).acid_rise.value;
      day_time = 0;
      day = after.progression.day;
    }
    ASSERT_EQ(after.progression.acid.value, expected_acid);
    ASSERT_LE(day_time, static_cast<std::uint64_t>(
                            std::chrono::microseconds(3s).count()));
  }
  ASSERT_EQ(day, LAST_DAY);
  const auto view = game.view();
  const auto taken = 20U - view.exploration.sources.front().remaining;
  EXPECT_GT(taken, 0U);
  EXPECT_LE(view.progression.chef.inventory.size() +
                view.progression.appraiser.inventory.size() +
                view.workshop.foods.size() + view.workshop.eaten_foods.size(),
            taken);
  EXPECT_FALSE(view.workshop.eaten_foods.empty());
  EXPECT_EQ(view.workshop.spatula, ToolState::Intact);
  EXPECT_EQ(view.exploration.sources.back().remaining, 0U);
  EXPECT_EQ(game.refusals(), 0U);
}

TEST(CompanionRun, ReplayingTheSameRunGivesTheSameState) {
  const std::vector<ObjectDefinition> city{floor_of(10),
                                           ingredient_source(SOURCE, 3)};
  Game first(city, tuning({{.id = SOURCE, .remaining = 9}}), kitchen());
  Game second(city, tuning({{.id = SOURCE, .remaining = 9}}), kitchen());
  play_until_day(first, 6, ALL_AI);
  play_until_day(second, 6, ALL_AI);
  EXPECT_EQ(first.simulation().snapshot(), second.simulation().snapshot());
  EXPECT_EQ(first.workshop().snapshot(), second.workshop().snapshot());
  EXPECT_EQ(first.exploration().snapshot(), second.exploration().snapshot());
}

TEST(CompanionRun, DaysPassWhenTheCityHoldsNoIngredients) {
  Game game({floor_of(10)}, tuning({}), kitchen());
  play_until_day(game, 6, ALL_AI);
  const auto snapshot = game.simulation().snapshot();
  EXPECT_EQ(snapshot.acid.value, 5 * UNFED_PENALTY.value);
  EXPECT_EQ(snapshot.chef.location, Location::Base);
  EXPECT_EQ(game.workshop().snapshot().last_day,
            (DayOutcome{.completed_day = 5,
                        .reaction = MonsterReaction::Unfed,
                        .score = 0,
                        .acid_rise = UNFED_PENALTY}));
  EXPECT_EQ(game.refusals(), 0U);
}

TEST(CompanionRun, DaysPassWhenEverySourceIsUnreachable) {
  Game game({floor_of(4), ingredient_source(SOURCE, 8)},
            tuning({{.id = SOURCE}}), kitchen());
  play_until_day(game, 6, ALL_AI);
  EXPECT_EQ(game.exploration().snapshot().sources.front().remaining, 1U);
  EXPECT_EQ(game.simulation().snapshot().remaining_exploration, 3s);
  EXPECT_EQ(game.refusals(), 0U);
}

TEST(CompanionRun, DaysPassWhenASourceIsFartherThanTheDayAllows) {
  Game game({floor_of(10), ingredient_source(SOURCE, 9)},
            tuning({{.id = SOURCE}}), kitchen(), 300ms);
  play_until_day(game, 4, ALL_AI);
  EXPECT_EQ(game.exploration().snapshot().sources.front().remaining, 1U);
  EXPECT_GT(game.world().snapshot().progression.elapsed, 0us);
  EXPECT_EQ(game.refusals(), 0U);
}

TEST(CompanionRun, AnAiPartnerStaysAtBaseWhenTheCityHoldsNothingForIt) {
  Game game({floor_of(4), ingredient_source(SOURCE, 8)},
            tuning({{.id = SOURCE}}), kitchen());
  const auto control = single_player(Actor::Chef);
  game.step(control, {PlayerInput{.travel = true}, NO_INPUT});
  ASSERT_EQ(game.simulation().snapshot().chef.location, Location::City);
  ASSERT_EQ(game.simulation().snapshot().appraiser.location, Location::Base);
  game.step(control);
  EXPECT_EQ(game.simulation().snapshot().appraiser.location, Location::Base);
  EXPECT_EQ(game.simulation().snapshot().phase, Phase::Exploration);
  EXPECT_EQ(game.refusals(), 0U);
}

TEST(CompanionRun, AnAiPartyAppraisesAgainAfterAFailedRoll) {
  Game game({floor_of(10), ingredient_source(SOURCE, 1)},
            tuning({{.id = SOURCE}}), kitchen());
  game.workshop().damage_tool(WorkshopTool::Magnifier);
  play_until_day(game, 2, ALL_AI);
  const auto workshop = game.workshop().snapshot();
  EXPECT_EQ(workshop.identified, std::vector<IngredientId>{CARROT});
  EXPECT_GE(workshop.rolls, 1U);
  EXPECT_EQ(game.refusals(), 0U);
}

TEST(Companion, TheChefCooksTheTastiestNewDish) {
  auto work = kitchen({{.food = SOUP, .ingredients = {CARROT}},
                       {.food = STEW, .ingredients = {ONION}}});
  work.flavors = {{.ingredient = CARROT, .taste = 1},
                  {.ingredient = ONION, .taste = 9}};
  auto view = base_view();
  view.workshop.identified = {CARROT, ONION};
  view.progression.chef.inventory = {CARROT, ONION};
  EXPECT_EQ(
      companion_with(work).command(Actor::Chef, ALL_AI, NO_INPUT, view).action,
      (std::optional<Action>{Cook{STEW}}));
  view.workshop.eaten_foods = {STEW};
  EXPECT_EQ(
      companion_with(work).command(Actor::Chef, ALL_AI, NO_INPUT, view).action,
      (std::optional<Action>{Cook{SOUP}}));
}

TEST(Companion, TheChefNeverCooksARejectedDishAgain) {
  auto work = kitchen({{.food = SOUP, .ingredients = {CARROT}},
                       {.food = STEW, .ingredients = {ONION}}});
  work.flavors = {{.ingredient = CARROT, .taste = 1},
                  {.ingredient = ONION, .taste = 9}};
  auto view = base_view();
  view.workshop.identified = {CARROT, ONION};
  view.progression.chef.inventory = {CARROT, ONION};
  view.workshop.eaten_foods = {SOUP, STEW};
  view.workshop.rejected_foods = {STEW};
  EXPECT_EQ(
      companion_with(work).command(Actor::Chef, ALL_AI, NO_INPUT, view).action,
      (std::optional<Action>{Cook{SOUP}}));
  view.progression.chef.inventory = {ONION};
  EXPECT_NE(
      companion_with(work).command(Actor::Chef, ALL_AI, NO_INPUT, view).action,
      (std::optional<Action>{Cook{STEW}}));
  view.workshop.rejected_foods.clear();
  EXPECT_EQ(
      companion_with(work).command(Actor::Chef, ALL_AI, NO_INPUT, view).action,
      (std::optional<Action>{Cook{STEW}}));
}

TEST(Companion, NoOneFeedsARejectedDishAgain) {
  auto view = base_view();
  view.workshop.foods = {{.food = SOUP}};
  view.workshop.eaten_foods = {SOUP};
  view.workshop.rejected_foods = {SOUP};
  const auto companion = companion_with();
  const auto press = [&] {
    return companion.command(Actor::Chef, two_players(), {.feed = true}, view)
        .action;
  };
  EXPECT_EQ(press(), std::nullopt);
  EXPECT_EQ(ai_action(view, Actor::Chef), Action{EndDayUnfed{UNFED_PENALTY}});
  view.workshop.foods = {{.food = SOUP}, {.food = STEW}};
  view.workshop.eaten_foods = {SOUP, STEW};
  EXPECT_EQ(press(), Action{Feed{STEW}});
  EXPECT_EQ(ai_action(view, Actor::Chef), Action{Feed{STEW}});
  view.workshop.foods = {{.food = SOUP}};
  view.workshop.rejected_foods.clear();
  EXPECT_EQ(press(), Action{Feed{SOUP}});
  EXPECT_EQ(ai_action(view, Actor::Chef), Action{Feed{SOUP}});
}

TEST(Companion, AStoredRejectedDishDoesNotStopTheChefCooking) {
  const auto companion =
      companion_with(kitchen({{.food = SOUP, .ingredients = {CARROT}},
                              {.food = STEW, .ingredients = {ONION}}}));
  auto view = base_view();
  view.progression.chef.inventory = {ONION};
  view.workshop.identified = {ONION};
  view.workshop.foods = {{.food = SOUP}};
  view.workshop.eaten_foods = {SOUP};
  view.workshop.rejected_foods = {SOUP};
  const auto action = [&] {
    return companion.command(Actor::Chef, ALL_AI, NO_INPUT, view).action;
  };
  EXPECT_EQ(action(), Action{Cook{STEW}});
  view.workshop.rejected_foods.clear();
  EXPECT_EQ(action(), Action{Feed{SOUP}});
}

TEST(Companion, TheErrandNamesWhereAiWouldWalkAndStopsStepping) {
  constexpr ObjectId NEAR{70};
  auto view = city_view({.x = 2});
  add_source(view, NEAR, 6);
  const auto companion = companion_with();
  const auto walking = companion.errand(Actor::Chef, view).value_or(Errand{});
  EXPECT_EQ(walking.target, NEAR);
  EXPECT_EQ(walking.step, Move::Right);
  view.exploration.chef.body.position = standing_in({.x = 5});
  const auto there = companion.errand(Actor::Chef, view).value_or(Errand{});
  EXPECT_EQ(there.target, NEAR);
  EXPECT_EQ(there.step, std::nullopt);
  view.exploration.sources.back().remaining = 0;
  EXPECT_FALSE(companion.errand(Actor::Chef, view).has_value());
}

TEST(Companion, AiWalksToWhatThePartyLacksBeforeANearerSource) {
  constexpr ObjectId NEAR{70};
  constexpr ObjectId FAR{72};
  auto view = city_view({.x = 2});
  add_source(view, NEAR, 2);
  add_source(view, FAR, 6);
  view.world.objects.back().ingredient = ONION;
  EXPECT_EQ(ai_action(view, Actor::Chef),
            (std::optional<Action>{Collect{NEAR}}));
  view.progression.appraiser.inventory = {CARROT};
  const auto lacking =
      companion_with().command(Actor::Chef, ALL_AI, NO_INPUT, view);
  EXPECT_EQ(lacking.movement.direction, Direction::Right);
  EXPECT_FALSE(lacking.action.has_value());
}

TEST(Companion, ALostPartyIsGivenNoCommand) {
  auto view = base_view();
  view.workshop.foods = {{.food = SOUP}};
  ASSERT_TRUE(ai_action(view, Actor::Chef).has_value());
  view.progression.phase = Phase::Lost;
  EXPECT_FALSE(ai_action(view, Actor::Chef).has_value());
  EXPECT_FALSE(ai_action(view, Actor::Appraiser).has_value());
}

} // namespace
} // namespace neocity::survival
