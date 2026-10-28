#include "camera.hpp"

#include <algorithm>
#include <cmath>

namespace neocity::ascent {

AscentCamera::AscentCamera(const WorldSnapshot &snapshot)
    : top(snapshot.leader.feet.y - 560.0F) {}

void AscentCamera::follow(const WorldSnapshot &snapshot, float delta) {
  const float target =
      std::clamp(snapshot.leader.feet.y - 540.0F, 0.0F, 1740.0F);
  if (std::abs(target - top) > 500.0F) {
    top = target;
  } else {
    top += (target - top) * (1.0F - std::exp(-5.0F * delta));
  }
}

} // namespace neocity::ascent
