#pragma once

#include <gameplay/world_snapshot.hpp>

#include <cstddef>
#include <vector>

namespace neocity::gameplay {

inline constexpr float STEP = 1.0F / 120.0F;
inline constexpr Point START{.x = 160.0F, .y = 2280.0F};
inline constexpr float WIDTH = 1280.0F;
inline constexpr std::size_t CHECKPOINT_INDEX = 8;
inline constexpr Point GOAL{.x = 448.0F, .y = 566.0F};

/// The course's landing surfaces at simulation time `time`.
std::vector<Surface> surfaces(float time);

/// The course's obstacles at simulation time `time`, placed on `platforms`.
std::vector<Hazard> hazards(float time, const std::vector<Surface> &platforms);

} // namespace neocity::gameplay
