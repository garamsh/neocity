#include <survival/exploration.hpp>

#include "search_support.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <stdexcept>

namespace neocity::survival {
namespace {

using namespace std::chrono_literals;
constexpr ObjectId FLOOR{1};
constexpr ObjectId SOURCE{10};
constexpr std::uint32_t FIRST_DAY = 3;
constexpr std::uint32_t PARTS = 3;

/// A one-row walkway with a source of parts at x = 1 and the device at x = 4.
/// The chef enters at x = 0, out of reach of the device.
class Device {
public:
  explicit Device(std::uint32_t held = PARTS, AcidLevel full = AcidLevel{100})
      : simulation({.exploration_budget = 30s, .seed = 0, .full_acid = full}),
        world({{.id = FLOOR, .area = {.x = 0, .y = 0, .width = 20}},
               {.id = SOURCE,
                .kind = ObjectKind::MaterialSource,
                .area = {.x = 1},
                .ingredient = ESCAPE_PART}}),
        exploration(simulation, world,
                    {.base = {.x = -10, .y = -10},
                     .entry = {.x = 0, .y = 0},
                     .exits = {},
                     .interiors = {},
                     .sources = {stock(SOURCE, held, ESCAPE_PART)},
                     .obstacles = {},
                     .patrols = {},
                     .escape = EscapeDevice{.map = MapId::Street,
                                            .area = {.x = 4, .y = 0},
                                            .parts_needed = PARTS,
                                            .from_day = FIRST_DAY}}) {
    skip_to(1);
  }

  /// Ends days at base until `day` opens.
  void skip_to(std::uint32_t day) {
    while (simulation.snapshot().day < day) {
      if (simulation.finish_day({}) != Result::Applied) {
        throw std::logic_error("day refused");
      }
    }
    if (exploration.advance({}, {}) != Result::Applied) {
      throw std::logic_error("world refused");
    }
  }

  /// The chef enters and takes every part the source holds.
  void take_parts() {
    if (exploration.enter(Actor::Chef) != Result::Applied) {
      throw std::logic_error("entry refused");
    }
    while (collect(exploration, Actor::Chef, SOURCE) ==
           CollectionResult::Applied) {
    }
  }

  /// Walks the chef right until its feet are in `column`.
  void walk_to(std::int32_t column) {
    while (cell_of(exploration.snapshot().chef.body.position).x < column) {
      if (exploration.advance(
              100ms, {ExplorationInput{.direction = Direction::Right}, {}}) !=
          Result::Applied) {
        throw std::logic_error("advance refused");
      }
    }
  }

  Simulation simulation;
  World world;
  Exploration exploration;
};

TEST(Escape, ACarriedPartSetUsedAtTheDeviceOnItsDayEndsTheRun) {
  Device device;
  device.skip_to(FIRST_DAY);
  device.take_parts();
  device.walk_to(3);
  EXPECT_EQ(device.exploration.escape(Actor::Chef), Result::Applied);
  const auto snapshot = device.simulation.snapshot();
  EXPECT_EQ(snapshot.phase, Phase::Ended);
  EXPECT_EQ(snapshot.ending, Ending::Escaped);
}

TEST(Escape, TheDeviceRefusesBeforeItsFirstDay) {
  Device device;
  device.skip_to(FIRST_DAY - 1);
  device.take_parts();
  device.walk_to(3);
  const auto before = device.simulation.snapshot();
  EXPECT_EQ(device.exploration.escape(Actor::Chef), Result::TooEarly);
  EXPECT_EQ(device.simulation.snapshot(), before);
}

TEST(Escape, TheDeviceRefusesWithTooFewParts) {
  Device device(PARTS - 1);
  device.skip_to(FIRST_DAY);
  device.take_parts();
  device.walk_to(3);
  const auto before = device.simulation.snapshot();
  EXPECT_EQ(device.exploration.escape(Actor::Chef), Result::MissingIngredients);
  EXPECT_EQ(device.simulation.snapshot(), before);
}

TEST(Escape, TheDeviceRefusesAwayFromIt) {
  Device device;
  device.skip_to(FIRST_DAY);
  device.take_parts();
  EXPECT_EQ(device.exploration.escape(Actor::Chef), Result::NotAtDevice);
  EXPECT_EQ(device.simulation.snapshot().phase, Phase::Exploration);
}

TEST(Escape, AnActorAtBaseCannotUseTheDevice) {
  Device device;
  device.skip_to(FIRST_DAY);
  EXPECT_EQ(device.exploration.escape(Actor::Chef), Result::NotExploring);
}

TEST(Escape, ASearchingActorCannotUseTheDevice) {
  Device device;
  device.skip_to(FIRST_DAY);
  device.take_parts();
  EXPECT_EQ(device.exploration.search(Actor::Chef, SOURCE),
            CollectionResult::Applied);
  EXPECT_EQ(device.exploration.escape(Actor::Chef), Result::Busy);
}

TEST(Escape, ALostRunStaysLostWhateverThePartsCarried) {
  Device device(PARTS, AcidLevel{10});
  device.skip_to(FIRST_DAY);
  device.take_parts();
  device.walk_to(3);
  ASSERT_EQ(device.simulation.return_to_base(Actor::Chef), Result::Applied);
  ASSERT_EQ(device.simulation.finish_day(AcidLevel{10}), Result::Applied);
  ASSERT_EQ(device.simulation.snapshot().phase, Phase::Lost);
  const auto before = device.simulation.snapshot();
  EXPECT_EQ(device.exploration.escape(Actor::Chef), Result::RunOver);
  EXPECT_EQ(device.simulation.snapshot(), before);
}

TEST(Escape, ARunWithNoDeviceCannotEscape) {
  Simulation simulation({.exploration_budget = 30s, .seed = 0});
  World world({{.id = FLOOR, .area = {.x = 0, .y = 0, .width = 20}}});
  Exploration exploration(simulation, world,
                          {.base = {.x = -10, .y = -10},
                           .entry = {.x = 0, .y = 0},
                           .exits = {},
                           .interiors = {},
                           .sources = {},
                           .obstacles = {},
                           .patrols = {}});
  ASSERT_EQ(exploration.enter(Actor::Chef), Result::Applied);
  EXPECT_EQ(exploration.escape(Actor::Chef), Result::NotAtDevice);
}

TEST(Escape, TheDeviceNeedsPartsAndAFirstDay) {
  Simulation simulation({.exploration_budget = 30s, .seed = 0});
  World world({{.id = FLOOR, .area = {.x = 0, .y = 0, .width = 20}}});
  EXPECT_THROW(Exploration(simulation, world,
                           {.base = {.x = -10, .y = -10},
                            .entry = {.x = 0, .y = 0},
                            .exits = {},
                            .interiors = {},
                            .sources = {},
                            .obstacles = {},
                            .patrols = {},
                            .escape = EscapeDevice{.map = MapId::Street,
                                                   .area = {.x = 4, .y = 0},
                                                   .parts_needed = 0,
                                                   .from_day = 1}}),
               std::invalid_argument);
}

} // namespace
} // namespace neocity::survival
