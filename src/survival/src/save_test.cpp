#include <survival/ending.hpp>
#include <survival/save.hpp>

#include "../../session/save.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

namespace neocity::survival {
namespace {

using namespace std::chrono_literals;

struct SavedRun {
  explicit SavedRun(Content settings = temporary_content(321, {.cook = 0s}))
      : content(std::move(settings)) {}

  Content content;
  Simulation simulation{content.run};
  World world{content.objects};
  Exploration exploration{simulation, world, content.companion.exploration};
  Workshop workshop{simulation, content.companion.workshop};
  Endings endings{simulation, workshop, TEMPORARY_ENDING};

  void carry(Actor actor, IngredientId ingredient) {
    ASSERT_EQ(simulation.enter(actor), Result::Applied);
    ASSERT_EQ(simulation.collect(actor, ingredient), Result::Applied);
    ASSERT_EQ(simulation.return_to_base(actor), Result::Applied);
  }

  std::optional<DaySave> checkpoint(ControlMap players = two_players()) {
    return DaySave::capture(simulation, workshop, exploration, players);
  }

  void prepare_day() {
    const auto &recipe = content.companion.workshop.recipes.front();
    for (int batch = 0; batch < 4; ++batch) {
      for (const auto ingredient : recipe.ingredients) {
        carry(Actor::Appraiser, ingredient);
        while (workshop.appraise(Actor::Appraiser, ingredient) ==
               WorkshopResult::Failed) {
        }
        ASSERT_EQ(workshop.store(Actor::Appraiser, ingredient),
                  WorkshopResult::Applied);
      }
      const auto cooked = workshop.cook(Actor::Chef, recipe.food);
      ASSERT_TRUE(cooked == WorkshopResult::Applied ||
                  cooked == WorkshopResult::Failed);
    }
    ASSERT_FALSE(workshop.snapshot().foods.empty());
    ASSERT_EQ(workshop.feed(recipe.food), WorkshopResult::Applied);
    ASSERT_EQ(workshop.end_day_unfed(AcidLevel{9}), WorkshopResult::Applied);
    ASSERT_EQ(exploration.advance(0us, {}), Result::Applied);
    carry(Actor::Appraiser, recipe.ingredients.front());
    ASSERT_EQ(workshop.store(Actor::Appraiser, recipe.ingredients.front()),
              WorkshopResult::Applied);
    ASSERT_GT(simulation.snapshot().acid.value, 0U);
    ASSERT_FALSE(simulation.snapshot().fridge.empty());
    ASSERT_FALSE(workshop.snapshot().foods.empty());
    ASSERT_FALSE(workshop.snapshot().identified.empty());
    ASSERT_FALSE(workshop.snapshot().eaten_foods.empty());
    ASSERT_GT(workshop.snapshot().rolls, 0U);
    carry(Actor::Chef, recipe.ingredients.front());
  }
};

TEST(DaySave, RoundTripPreservesTheWholeDayAndFutureRandomResults) {
  SavedRun original;
  original.prepare_day();
  const auto saved = original.checkpoint(single_player(Actor::Appraiser));
  ASSERT_TRUE(saved);
  const auto parsed = DaySave::parse(saved->encode(), original.content);
  ASSERT_TRUE(parsed);
  EXPECT_EQ(parsed->control(), single_player(Actor::Appraiser));
  EXPECT_EQ(parsed->seed(), 321U);
  SavedRun restored;
  parsed->restore(restored.simulation, restored.workshop, restored.exploration);
  EXPECT_EQ(restored.simulation.snapshot(), original.simulation.snapshot());
  EXPECT_EQ(restored.workshop.snapshot(), original.workshop.snapshot());
  EXPECT_EQ(restored.world.snapshot(), original.world.snapshot());
  EXPECT_EQ(restored.exploration.snapshot(), original.exploration.snapshot());
  for (int attempt = 0; attempt < 12; ++attempt) {
    const auto &recipe = original.content.companion.workshop.recipes.front();
    for (const auto ingredient : recipe.ingredients) {
      original.carry(Actor::Chef, ingredient);
      restored.carry(Actor::Chef, ingredient);
    }
    EXPECT_EQ(original.workshop.cook(Actor::Chef, recipe.food),
              restored.workshop.cook(Actor::Chef, recipe.food));
    EXPECT_EQ(original.workshop.snapshot(), restored.workshop.snapshot());
  }
}

TEST(DaySave, PreservesFedDishScoresAndRejectedFoodHistory) {
  SavedRun run;
  const auto recipe =
      *std::ranges::find_if(run.content.companion.workshop.recipes,
                            [](const Recipe &value) { return value.disliked; });
  for (const auto ingredient : recipe.ingredients) {
    run.carry(Actor::Appraiser, ingredient);
    while (run.workshop.appraise(Actor::Appraiser, ingredient) ==
           WorkshopResult::Failed) {
    }
  }
  for (int attempt = 0; attempt < 8 && run.workshop.snapshot().foods.empty();
       ++attempt) {
    for (const auto ingredient : recipe.ingredients) {
      run.carry(Actor::Chef, ingredient);
    }
    const auto result = run.workshop.cook(Actor::Chef, recipe.food);
    ASSERT_TRUE(result == WorkshopResult::Applied ||
                result == WorkshopResult::Failed);
  }
  ASSERT_FALSE(run.workshop.snapshot().foods.empty());
  ASSERT_EQ(run.workshop.feed(recipe.food), WorkshopResult::Applied);
  ASSERT_TRUE(run.workshop.snapshot().last_day->fed);
  ASSERT_EQ(run.workshop.snapshot().last_day->reaction,
            MonsterReaction::Disliked);
  ASSERT_FALSE(run.workshop.snapshot().rejected_foods.empty());
  const auto save = run.checkpoint();
  ASSERT_TRUE(save);
  const auto parsed = DaySave::parse(save->encode(), run.content);
  ASSERT_TRUE(parsed);
  SavedRun restored;
  parsed->restore(restored.simulation, restored.workshop, restored.exploration);
  EXPECT_EQ(restored.simulation.snapshot(), run.simulation.snapshot());
  EXPECT_EQ(restored.workshop.snapshot(), run.workshop.snapshot());
}

TEST(DaySave, PreservesPartlyEmptiedContainerStockAcrossADay) {
  auto content = temporary_content(321, {.cook = 0s});
  const auto ingredient =
      content.companion.workshop.recipes.front().ingredients.front();
  constexpr ObjectId source{9000};
  content.objects.push_back({.id = source,
                             .kind = ObjectKind::MaterialSource,
                             .area = {.x = 5, .y = 8},
                             .ingredient = ingredient});
  content.companion.exploration.sources.push_back(
      {.id = source,
       .capacity = 3,
       .items = {ingredient, ingredient, ingredient}});
  SavedRun run{content};
  ASSERT_EQ(run.exploration.enter(Actor::Chef), Result::Applied);
  ASSERT_EQ(run.exploration.search(Actor::Chef, source),
            CollectionResult::Applied);
  ASSERT_EQ(run.exploration.take(Actor::Chef, source, ingredient),
            CollectionResult::Applied);
  ASSERT_EQ(run.exploration.close_search(Actor::Chef),
            CollectionResult::Applied);
  ASSERT_EQ(run.exploration.return_to_base(Actor::Chef), Result::Applied);
  ASSERT_EQ(run.workshop.end_day_unfed(AcidLevel{1}), WorkshopResult::Applied);
  ASSERT_EQ(run.exploration.advance(0us, {}), Result::Applied);
  ASSERT_EQ(run.exploration.snapshot().sources.back().items.size(), 2U);
  const auto save = run.checkpoint();
  ASSERT_TRUE(save);
  const auto parsed = DaySave::parse(save->encode(), content);
  ASSERT_TRUE(parsed);
  SavedRun restored{content};
  parsed->restore(restored.simulation, restored.workshop, restored.exploration);
  EXPECT_EQ(restored.exploration.snapshot(), run.exploration.snapshot());
  EXPECT_EQ(restored.simulation.snapshot(), run.simulation.snapshot());
}

TEST(DaySave, CapturesTheFirstDayForEveryPlayerChoice) {
  SavedRun run;
  for (const auto players : {single_player(Actor::Chef),
                             single_player(Actor::Appraiser), two_players()}) {
    const auto save = run.checkpoint(players);
    ASSERT_TRUE(save);
    const auto parsed = DaySave::parse(save->encode(), run.content);
    ASSERT_TRUE(parsed);
    EXPECT_EQ(parsed->control(), players);
    SavedRun restored;
    parsed->restore(restored.simulation, restored.workshop,
                    restored.exploration);
    EXPECT_EQ(restored.simulation.snapshot(), run.simulation.snapshot());
    EXPECT_EQ(restored.workshop.snapshot(), run.workshop.snapshot());
  }
}

TEST(DaySave, RejectsBrokenOrDifferentVersionFilesWithValidControl) {
  SavedRun run;
  const auto save = run.checkpoint();
  ASSERT_TRUE(save);
  const auto bytes = save->encode();
  ASSERT_TRUE(DaySave::parse(bytes, run.content));
  for (const auto &bad : {std::string{}, std::string{"garbage"},
                          bytes.substr(0, bytes.size() / 2), bytes + "extra",
                          std::string(131073, 'x')}) {
    EXPECT_FALSE(DaySave::parse(bad, run.content));
  }
  auto version = bytes;
  version.replace(8, 1, "2");
  EXPECT_FALSE(DaySave::parse(version, run.content));
  auto incompatible = run.content;
  incompatible.run.exploration_budget += 1s;
  EXPECT_FALSE(DaySave::parse(bytes, incompatible));
}

TEST(DaySave, RejectsUnknownIdsInvalidRolesAndOutOfRangeNumbers) {
  SavedRun run;
  run.prepare_day();
  const auto save = run.checkpoint();
  ASSERT_TRUE(save);
  const auto bytes = save->encode();
  ASSERT_TRUE(DaySave::parse(bytes, run.content));
  auto unknown = run.content;
  unknown.objects.clear();
  EXPECT_FALSE(DaySave::parse(bytes, unknown));
  // The scalar prefix follows the magic, version and three time settings.
  const auto prefix = bytes.find(" 3 9 321 ");
  ASSERT_NE(prefix, std::string::npos);
  for (const auto &bad : {" 0 9 321 ", " 31 9 321 ", " 3 100 321 ",
                          " 3 -1 321 ", " 4294967296 9 321 "}) {
    auto corrupt = bytes;
    corrupt.replace(prefix, std::string{" 3 9 321 "}.size(), bad);
    EXPECT_FALSE(DaySave::parse(corrupt, run.content));
  }
  auto invalid_players = run.checkpoint({Controller::Ai, Controller::Ai});
  ASSERT_TRUE(invalid_players);
  EXPECT_FALSE(DaySave::parse(invalid_players->encode(), run.content));
}

TEST(DaySave, RefusesMiddayAndEndedCheckpoints) {
  SavedRun run;
  ASSERT_TRUE(run.checkpoint());
  run.workshop.advance(1s);
  EXPECT_FALSE(run.checkpoint());
  ASSERT_EQ(run.workshop.end_day_unfed(AcidLevel{1}), WorkshopResult::Applied);
  ASSERT_TRUE(run.checkpoint());
  ASSERT_EQ(run.simulation.enter(Actor::Chef), Result::Applied);
  EXPECT_FALSE(run.checkpoint());
  ASSERT_EQ(run.simulation.return_to_base(Actor::Chef), Result::Applied);
  ASSERT_EQ(run.simulation.end(Ending::MonsterSatisfied), Result::Applied);
  EXPECT_FALSE(run.checkpoint());
}

class SaveSlotTest : public testing::Test {
protected:
  void SetUp() override {
    directory =
        std::filesystem::temp_directory_path() /
        ("w278-save-" +
         std::to_string(
             std::chrono::steady_clock::now().time_since_epoch().count()));
    ASSERT_TRUE(std::filesystem::create_directory(directory));
  }
  void TearDown() override {
    std::error_code error;
    std::filesystem::remove_all(directory, error);
    EXPECT_FALSE(error);
  }
  std::filesystem::path directory;
};

TEST_F(SaveSlotTest, OverwritesOneSlotAndLeavesItUntilANewDayStarts) {
  SavedRun run;
  const session::SaveSlot slot{directory / "day.save"};
  EXPECT_FALSE(slot.load(run.content));
  ASSERT_TRUE(slot.update(run.simulation, run.workshop, run.exploration,
                          two_players()));
  ASSERT_TRUE(slot.load(run.content));
  const auto first = slot.load(run.content)->encode();
  // A new-game setup has no file operation.
  SavedRun replacement;
  EXPECT_EQ(slot.load(run.content)->encode(), first);
  replacement.prepare_day();
  ASSERT_TRUE(slot.update(replacement.simulation, replacement.workshop,
                          replacement.exploration, single_player(Actor::Chef)));
  const auto loaded = slot.load(run.content);
  ASSERT_TRUE(loaded);
  EXPECT_EQ(loaded->control(), single_player(Actor::Chef));
  EXPECT_NE(loaded->encode(), first);
  EXPECT_EQ(std::distance(std::filesystem::directory_iterator(directory),
                          std::filesystem::directory_iterator{}),
            1);
}

TEST_F(SaveSlotTest, HidesUnreadableAndCorruptFiles) {
  SavedRun run;
  const auto path = directory / "day.save";
  const session::SaveSlot slot{path};
  ASSERT_TRUE(slot.update(run.simulation, run.workshop, run.exploration,
                          two_players()));
  ASSERT_TRUE(slot.load(run.content));
  {
    std::ofstream output(path);
    output << "MME-DAY 2 broken";
    output.close();
    ASSERT_FALSE(output.fail());
  }
  EXPECT_FALSE(slot.load(run.content));
  const session::SaveSlot unreadable{directory / "missing" / "day.save"};
  EXPECT_FALSE(unreadable.load(run.content));
  EXPECT_FALSE(unreadable.update(run.simulation, run.workshop, run.exploration,
                                 two_players()));
}

TEST_F(SaveSlotTest, DeletesAfterEachEndingAndAfterAcidLoss) {
  for (const auto ending : {Ending::MonsterSatisfied, Ending::VomitedOut}) {
    SavedRun run;
    const auto path = directory / "day.save";
    const session::SaveSlot slot{path};
    ASSERT_TRUE(slot.update(run.simulation, run.workshop, run.exploration,
                            two_players()));
    ASSERT_TRUE(slot.load(run.content));
    ASSERT_EQ(run.simulation.end(ending), Result::Applied);
    ASSERT_EQ(run.simulation.snapshot().ending, ending);
    ASSERT_TRUE(slot.update(run.simulation, run.workshop, run.exploration,
                            two_players()));
    EXPECT_FALSE(std::filesystem::exists(path));
    EXPECT_FALSE(slot.load(run.content));
    EXPECT_TRUE(slot.update(run.simulation, run.workshop, run.exploration,
                            two_players()));
  }
  SavedRun lost;
  const auto path = directory / "day.save";
  const session::SaveSlot slot{path};
  ASSERT_TRUE(slot.update(lost.simulation, lost.workshop, lost.exploration,
                          two_players()));
  ASSERT_EQ(lost.workshop.end_day_unfed(lost.content.run.full_acid),
            WorkshopResult::Applied);
  ASSERT_EQ(lost.simulation.snapshot().phase, Phase::Lost);
  ASSERT_TRUE(slot.update(lost.simulation, lost.workshop, lost.exploration,
                          two_players()));
  EXPECT_FALSE(std::filesystem::exists(path));
}

} // namespace
} // namespace neocity::survival
