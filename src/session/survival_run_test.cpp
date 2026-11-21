#include "../hud/title.hpp"
#include "survival_run.hpp"

#include <survival/save.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <vector>

namespace neocity::session {
namespace {

using namespace std::chrono_literals;

class RunRules {
public:
  explicit RunRules(survival::DayLengths lengths = {})
      : settings(survival::temporary_content(42, lengths)) {}

  [[nodiscard]] survival::Simulation &simulation() { return shared; }
  [[nodiscard]] survival::Workshop &workshop() { return kitchen; }

  SurvivalFeatures features() {
    return {.content = settings,
            .simulation = shared,
            .world = map,
            .exploration = explorers,
            .workshop = kitchen,
            .endings = finale,
            .companion = controllers};
  }

private:
  survival::Content settings;
  survival::Simulation shared{settings.run};
  survival::World map{settings.objects};
  survival::Exploration explorers{shared, map, settings.companion.exploration};
  survival::Workshop kitchen{shared, settings.companion.workshop};
  survival::Endings finale{shared, kitchen, survival::TEMPORARY_ENDING};
  survival::Companion controllers{settings.companion};
};

TEST(SurvivalAutosave, OffersContinueFirstOnlyWhenASlotWasParsed) {
  const auto &fresh = hud::title_entries(false);
  const auto &saved = hud::title_entries(true);
  EXPECT_EQ(fresh.front(), hud::TitleEntry::NewGame);
  EXPECT_EQ(saved.front(), hud::TitleEntry::Continue);
  EXPECT_EQ(saved.size(), fresh.size() + 1);
  for (std::size_t index = 0; index < fresh.size(); ++index) {
    EXPECT_EQ(saved.at(index + 1), fresh.at(index));
  }
}

TEST(SurvivalAutosave, StoresTheFirstDayButDoesNotOverwriteOnContinue) {
  RunRules rules;
  std::vector<survival::ControlMap> saved;
  const auto remember = [&saved](const SurvivalFeatures &features,
                                 survival::ControlMap players) {
    EXPECT_EQ(features.simulation.get().snapshot().day, 1U);
    EXPECT_TRUE(survival::DaySave::capture(
        features.simulation.get(), features.workshop.get(),
        features.exploration.get(), players));
    saved.push_back(players);
  };
  const auto players = survival::single_player(survival::Actor::Appraiser);
  const SurvivalRun fresh{rules.features(), players, 0, remember};
  ASSERT_EQ(saved.size(), 1U);
  EXPECT_EQ(saved.front(), players);
  const SurvivalRun continued{rules.features(), players, 0, remember, false};
  EXPECT_EQ(saved.size(), 1U);
}

TEST(SurvivalAutosave, StoresEveryNewDayInsideALongFrameWithFreshClocks) {
  RunRules rules{{.base = 1s}};
  std::vector<std::uint32_t> days;
  const auto remember = [&days](const SurvivalFeatures &features,
                                survival::ControlMap players) {
    const auto &simulation = features.simulation.get();
    const auto &workshop = features.workshop.get();
    EXPECT_EQ(workshop.snapshot().remaining_base, 1s);
    EXPECT_EQ(simulation.snapshot().remaining_exploration, 5min);
    EXPECT_TRUE(survival::DaySave::capture(
        simulation, workshop, features.exploration.get(), players));
    days.push_back(simulation.snapshot().day);
  };
  SurvivalRun run{rules.features(), survival::two_players(), 0, remember};
  (void)run.advance(3.2, {}, survival::Direction::Idle);
  EXPECT_EQ(days, (std::vector<std::uint32_t>{1, 2, 3, 4}));
  const auto remaining = rules.workshop().snapshot().remaining_base;
  EXPECT_LT(remaining.value_or(1s), 1s);
}

TEST(SurvivalAutosave, StoresBeforeThePartnerCanActOnTheNewDay) {
  RunRules rules;
  std::vector<survival::Snapshot> saved;
  const auto remember = [&saved](const SurvivalFeatures &features,
                                 survival::ControlMap players) {
    EXPECT_TRUE(survival::DaySave::capture(
        features.simulation.get(), features.workshop.get(),
        features.exploration.get(), players));
    saved.push_back(features.simulation.get().snapshot());
  };
  SurvivalRun run{rules.features(), survival::two_players(), 0, remember};
  std::array<survival::PlayerInput, 2> input{};
  input.at(0).end_day = true;
  input.at(1).travel = true;
  (void)run.advance(0.1, input, survival::Direction::Idle);
  ASSERT_EQ(saved.size(), 2U);
  EXPECT_EQ(saved.back().day, 2U);
  EXPECT_EQ(saved.back().phase, survival::Phase::Workshop);
  EXPECT_EQ(saved.back().appraiser.location, survival::Location::Base);
  EXPECT_EQ(rules.simulation().snapshot().appraiser.location,
            survival::Location::City);
}

TEST(SurvivalAutosave, ReportsTerminalRunsBeforeAnotherDayCanBeSaved) {
  RunRules rules{{.base = 1s}};
  std::vector<survival::Phase> states;
  const auto remember = [&states](const SurvivalFeatures &features,
                                  survival::ControlMap) {
    states.push_back(features.simulation.get().snapshot().phase);
  };
  SurvivalRun run{rules.features(), survival::two_players(), 0, remember};
  (void)run.advance(20.0, {}, survival::Direction::Idle);
  ASSERT_FALSE(states.empty());
  EXPECT_EQ(states.back(), survival::Phase::Lost);
  const auto count = states.size();
  (void)run.advance(20.0, {}, survival::Direction::Idle);
  EXPECT_EQ(states.size(), count);
}

} // namespace
} // namespace neocity::session
