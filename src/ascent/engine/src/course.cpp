#include "course.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace neocity::ascent {

namespace {

/// Euclidean remainder, non-negative for a positive divisor.
float rem_euclid(float value, float divisor) {
  const float r = std::fmod(value, divisor);
  return r < 0.0F ? r + std::abs(divisor) : r;
}

} // namespace

std::vector<Surface> surfaces(float time) {
  constexpr std::array xs{60.0F,   310.0F, 560.0F, 810.0F, 1020.0F, 800.0F,
                          550.0F,  300.0F, 80.0F,  320.0F, 560.0F,  800.0F,
                          1030.0F, 800.0F, 550.0F, 300.0F, 80.0F,   330.0F};
  std::vector<Surface> result;
  result.reserve(xs.size());
  for (std::size_t i = 0; i < xs.size(); ++i) {
    const bool moving = i == 6 || i == 13;
    result.push_back(Surface{
        .x = xs.at(i) + (moving ? std::sin(time * 0.7F) * 32.0F : 0.0F),
        .y = START.y - static_cast<float>(i) * 100.0F,
        .width = i == 0 ? 300.0F : 190.0F,
        .moving = moving,
    });
  }
  return result;
}

std::vector<Hazard> hazards(float time, const std::vector<Surface> &platforms) {
  struct Patrol {
    std::size_t index;
    float phase;
  };
  std::vector<Hazard> result;
  for (const Patrol patrol : {Patrol{.index = 3, .phase = 0.0F},
                              Patrol{.index = 11, .phase = 2.1F}}) {
    const Surface &p = platforms.at(patrol.index);
    result.push_back(Hazard{
        .center = {.x = p.x + p.width * 0.5F +
                        std::sin(time * 1.6F + patrol.phase) * 52.0F,
                   .y = p.y - 17.0F},
        .radius = 23.0F,
        .kind = HazardKind::Patrol,
        .warning = false,
        .active = true,
    });
  }
  result.push_back(Hazard{
      .center = {.x = 600.0F + std::sin(time * 1.1F) * 85.0F,
                 .y = platforms.at(7).y - 65.0F},
      .radius = 21.0F,
      .kind = HazardKind::Flyer,
      .warning = false,
      .active = true,
  });
  const float cycle = rem_euclid(time, 4.0F);
  result.push_back(Hazard{
      .center = {.x = 1130.0F - std::fmax(cycle - 1.0F, 0.0F) * 175.0F,
                 .y = platforms.at(14).y - 26.0F},
      .radius = 10.0F,
      .kind = HazardKind::Projectile,
      .warning = cycle < 1.0F,
      .active = 1.0F <= cycle && cycle < 3.4F,
  });
  return result;
}

} // namespace neocity::ascent
