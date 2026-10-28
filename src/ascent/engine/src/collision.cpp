#include "collision.hpp"

#include <algorithm>

namespace neocity::ascent {

std::optional<std::size_t> landing(Point previous, Point next,
                                   std::span<const Surface> surfaces) {
  if (next.y < previous.y) {
    return std::nullopt;
  }
  std::optional<std::size_t> found;
  float found_y = 0.0F;
  std::size_t index = 0;
  for (const Surface &p : surfaces) {
    const bool crossed = previous.y <= p.y + 0.1F && next.y >= p.y &&
                         next.x + HALF_WIDTH > p.x &&
                         next.x - HALF_WIDTH < p.x + p.width;
    // Strict comparison keeps the first of equally high surfaces.
    if (crossed && (!found || p.y < found_y)) {
      found = index;
      found_y = p.y;
    }
    ++index;
  }
  return found;
}

bool touches(Point feet, const Hazard &hazard) {
  if (!hazard.active) {
    return false;
  }
  const float nearest_x =
      std::clamp(hazard.center.x, feet.x - HALF_WIDTH, feet.x + HALF_WIDTH);
  const float nearest_y =
      std::clamp(hazard.center.y, feet.y - BODY_HEIGHT, feet.y);
  const float dx = nearest_x - hazard.center.x;
  const float dy = nearest_y - hazard.center.y;
  return dx * dx + dy * dy < hazard.radius * hazard.radius;
}

} // namespace neocity::ascent
