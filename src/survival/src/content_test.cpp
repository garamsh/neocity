#include <survival/content.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <optional>
#include <set>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace neocity::survival {
namespace {

using namespace std::chrono_literals;

constexpr ControlMap ALL_AI{.chef = Controller::Ai,
                            .appraiser = Controller::Ai};
constexpr std::array ACTORS{Actor::Chef, Actor::Appraiser};
constexpr auto STEP = 100ms;
constexpr std::int64_t STEP_LIMIT = 400'000;

/// The temporary game's features, advanced with a fresh view per actor.
class Game {
public:
  explicit Game(Content settings)
      : data(std::move(settings)), shared(data.run), map(data.objects),
        explorers(shared, map, data.companion.exploration),
        kitchen_work(shared, data.companion.workshop),
        controllers(data.companion) {}

  [[nodiscard]] const Content &content() const { return data; }
  [[nodiscard]] Simulation &simulation() { return shared; }
  [[nodiscard]] World &world() { return map; }
  [[nodiscard]] Exploration &exploration() { return explorers; }
  [[nodiscard]] Workshop &workshop() { return kitchen_work; }

  [[nodiscard]] GameView view() const {
    return {.progression = shared.snapshot(),
            .world = map.snapshot(),
            .exploration = explorers.snapshot(),
            .workshop = kitchen_work.snapshot()};
  }

  void step(ControlMap control) {
    std::array<ExplorationInput, 2> movement;
    for (std::size_t index = 0; index < ACTORS.size(); ++index) {
      const auto command =
          controllers.command(ACTORS.at(index), control, {}, view());
      movement.at(index) = command.movement;
      if (command.action) {
        (void)apply(explorers, kitchen_work, ACTORS.at(index), *command.action);
      }
    }
    ASSERT_EQ(explorers.advance(STEP, movement, shared.snapshot().acid),
              Result::Applied);
  }

private:
  Content data;
  Simulation shared;
  World map;
  Exploration explorers;
  Workshop kitchen_work;
  Companion controllers;
};

ObjectSnapshot object_named(const WorldSnapshot &world, std::string_view name) {
  const auto found =
      std::ranges::find_if(world.objects, [name](const ObjectSnapshot &object) {
        return object_name(object.id) == name;
      });
  if (found == world.objects.end()) {
    throw std::out_of_range("no such object");
  }
  return *found;
}

/// Runs AI-only days until `last_day` begins, or gives up at the step limit.
std::uint32_t play_days(Game &game, std::uint32_t last_day) {
  for (std::int64_t step = 0;
       step < STEP_LIMIT && game.simulation().snapshot().day < last_day;
       ++step) {
    game.step(ALL_AI);
  }
  return game.simulation().snapshot().day;
}

bool reachable(const Content &content, const WorldSnapshot &world,
               const WorldArea &area) {
  const auto entry = content.companion.exploration.entry;
  const auto range = content.companion.exploration.collection_distance;
  std::set<std::pair<std::int32_t, std::int32_t>> seen{{entry.x, entry.y}};
  std::deque<GridPosition> frontier{entry};
  while (!frontier.empty()) {
    const auto cell = frontier.front();
    frontier.pop_front();
    if (distance(cell, area) <= range) {
      return true;
    }
    for (const auto next : {GridPosition{.x = cell.x - 1, .y = cell.y},
                            GridPosition{.x = cell.x + 1, .y = cell.y},
                            GridPosition{.x = cell.x, .y = cell.y - 1},
                            GridPosition{.x = cell.x, .y = cell.y + 1}}) {
      if (cell_at(world, next) == Cell::Supported &&
          seen.insert({next.x, next.y}).second) {
        frontier.push_back(next);
      }
    }
  }
  return false;
}

TEST(TemporaryContent, ConstructsEveryFeatureWithinTheMap) {
  Game game(temporary_content(7));
  EXPECT_EQ(game.simulation().snapshot().seed, 7U);
  for (const auto &object : game.content().objects) {
    EXPECT_GE(object.area.x, 0);
    EXPECT_GE(object.area.y, 0);
    EXPECT_LE(object.area.x + object.area.width, game.content().map.width);
    EXPECT_LE(object.area.y + object.area.height, game.content().map.height);
    EXPECT_FALSE(object_name(object.id).empty());
  }
}

TEST(TemporaryContent, EveryNameLookupRefusesAnIdentityItNeverIssued) {
  EXPECT_EQ(ingredient_name(IngredientId{1}), "Tomato");
  EXPECT_THROW((void)ingredient_name(IngredientId{999}), std::out_of_range);
  EXPECT_THROW((void)food_name(FoodId{999}), std::out_of_range);
  EXPECT_THROW((void)tool_name(ToolId{999}), std::out_of_range);
  EXPECT_THROW((void)object_name(ObjectId{999}), std::out_of_range);
}

TEST(TemporaryContent, EveryRecipeIngredientAndBaseToolHasASource) {
  const auto content = temporary_content(1);
  const auto supplies = [&](auto member, auto id) {
    return std::ranges::any_of(content.objects, [&](const auto &object) {
      return std::invoke(member, object) == std::optional{id};
    });
  };
  for (const auto &recipe : content.companion.workshop.recipes) {
    EXPECT_FALSE(food_name(recipe.food).empty());
    for (const auto ingredient : recipe.ingredients) {
      EXPECT_TRUE(supplies(&ObjectDefinition::ingredient, ingredient));
    }
  }
  EXPECT_TRUE(
      supplies(&ObjectDefinition::tool, content.companion.workshop.magnifier));
  EXPECT_TRUE(
      supplies(&ObjectDefinition::tool, content.companion.workshop.spatula));
}

TEST(TemporaryContent, TheFirstDayReachesASourceOfEveryKind) {
  Game game(temporary_content(1));
  const auto world = game.world().snapshot();
  std::set<SourceKind> kinds;
  for (const auto &stock : game.exploration().snapshot().sources) {
    const auto object =
        std::ranges::find(world.objects, stock.id, &ObjectSnapshot::id);
    ASSERT_NE(object, world.objects.end());
    if (object->material_available &&
        reachable(game.content(), world, object->area)) {
      kinds.insert(stock.kind);
    }
  }
  EXPECT_EQ(kinds.size(), 4U);
}

TEST(TemporaryContent, AnAiPartyKeepsTheDaysMovingPastDayThirty) {
  for (const std::uint32_t seed : {1U, 2U, 3U, 4U, 5U}) {
    Game game(temporary_content(seed));
    EXPECT_EQ(play_days(game, 35), 35U) << "seed " << seed;
    EXPECT_GT(game.simulation().snapshot().acid.value, 0U) << "seed " << seed;
  }
}

TEST(TemporaryContent, AnAiPartyFeedsTheMonsterAndSatisfiesIt) {
  Game game(temporary_content(1));
  bool satisfied = false;
  for (std::int64_t step = 0;
       step < STEP_LIMIT && game.simulation().snapshot().day < 6; ++step) {
    game.step(ALL_AI);
    const auto last = game.workshop().snapshot().last_day;
    satisfied =
        satisfied || (last && last->reaction == MonsterReaction::Satisfied);
  }
  EXPECT_TRUE(satisfied);
  EXPECT_GE(game.workshop().snapshot().eaten_foods.size(), 2U);
}

TEST(TemporaryContent, TimeDamagesTheCityAndBringsTheFerryAcrossDays) {
  Game game(temporary_content(1));
  const auto start = game.world().snapshot();
  ASSERT_EQ(object_named(start, "Noodle Vendor").state, ObjectState::Intact);
  ASSERT_EQ(object_named(start, "Roof Ladder").state, ObjectState::Intact);
  ASSERT_EQ(object_named(start, "Drifted Ferry").state, ObjectState::Dormant);
  ASSERT_TRUE(object_named(start, "Vendor Cart").material_available);

  const std::array<ExplorationInput, 2> idle{};
  ASSERT_EQ(game.exploration().enter(Actor::Chef), Result::Applied);
  while (game.world().snapshot().progression.elapsed < 280s) {
    ASSERT_EQ(game.exploration().advance(STEP, idle, AcidLevel{}),
              Result::Applied);
    if (game.simulation().snapshot().phase == Phase::Workshop) {
      ASSERT_EQ(game.simulation().finish_day(AcidLevel{}), Result::Applied);
      ASSERT_EQ(game.exploration().enter(Actor::Chef), Result::Applied);
    }
  }
  const auto later = game.world().snapshot();
  EXPECT_EQ(object_named(later, "Noodle Vendor").state, ObjectState::Destroyed);
  EXPECT_FALSE(object_named(later, "Vendor Cart").material_available);
  EXPECT_EQ(object_named(later, "Roof Ladder").state, ObjectState::Destroyed);
  EXPECT_EQ(object_named(later, "Rotten Bridge").state, ObjectState::Destroyed);
  EXPECT_EQ(object_named(later, "Drifted Ferry").state, ObjectState::Intact);
  EXPECT_TRUE(object_named(later, "Ferry Bag").material_available);
}

TEST(TemporaryContent, TheFerryReopensTheEastAfterTheBridgeFalls) {
  Game game(temporary_content(1));
  const auto plaza = object_named(game.world().snapshot(), "Supply Crate").area;
  ASSERT_TRUE(reachable(game.content(), game.world().snapshot(), plaza));

  ASSERT_EQ(game.world().update({.elapsed = 0s, .acid = AcidLevel{40}}),
            WorldUpdateResult::Applied);
  const auto after = game.world().snapshot();
  ASSERT_EQ(object_named(after, "Rotten Bridge").state, ObjectState::Destroyed);
  EXPECT_TRUE(reachable(game.content(), after, plaza));

  auto without_ferry = after;
  std::erase_if(without_ferry.objects, [](const ObjectSnapshot &object) {
    return object_name(object.id) == "Drifted Ferry";
  });
  EXPECT_FALSE(reachable(game.content(), without_ferry, plaza));
}

TEST(TemporaryContent, TheBrokenLadderBlocksTheRooftopUntilRepaired) {
  Game game(temporary_content(1));
  const auto roof = object_named(game.world().snapshot(), "Roof Bag").area;
  ASSERT_TRUE(reachable(game.content(), game.world().snapshot(), roof));

  ASSERT_EQ(game.world().update({.elapsed = 0s, .acid = AcidLevel{10}}),
            WorldUpdateResult::Applied);
  const auto ladder = object_named(game.world().snapshot(), "Roof Ladder");
  ASSERT_EQ(ladder.state, ObjectState::Destroyed);
  EXPECT_FALSE(reachable(game.content(), game.world().snapshot(), roof));

  ASSERT_TRUE(ladder.repair_with.has_value());
  ASSERT_EQ(
      game.world().repair(ladder.id, ladder.repair_with.value_or(ToolId{})),
      RepairResult::Applied);
  EXPECT_TRUE(reachable(game.content(), game.world().snapshot(), roof));
}

} // namespace
} // namespace neocity::survival
