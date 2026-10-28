#include "collision.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <optional>

namespace neocity::gameplay {
namespace {

constexpr std::array LEDGE{
    Surface{.x = 100.0F, .y = 200.0F, .width = 160.0F, .moving = false}};

TEST(Collision, DescendingCrossingLandsEvenWhenOneTickPassesTheSurface) {
  EXPECT_EQ(landing(Point{.x = 150.0F, .y = 180.0F},
                    Point{.x = 150.0F, .y = 215.0F}, LEDGE),
            std::optional<std::size_t>{0});
}

TEST(Collision, RisingThroughAOneWayLedgeDoesNotLand) {
  EXPECT_EQ(landing(Point{.x = 150.0F, .y = 215.0F},
                    Point{.x = 150.0F, .y = 180.0F}, LEDGE),
            std::nullopt);
}

TEST(Collision, FallingOutsideThePlatformDoesNotSnapSideways) {
  EXPECT_EQ(landing(Point{.x = 300.0F, .y = 180.0F},
                    Point{.x = 300.0F, .y = 215.0F}, LEDGE),
            std::nullopt);
}

} // namespace
} // namespace neocity::gameplay
