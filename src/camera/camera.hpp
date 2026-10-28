#pragma once

#include <gameplay/world_snapshot.hpp>

namespace neocity::camera {

/// Vertical camera that follows the leading actor through the course.
class AscentCamera {
public:
  explicit AscentCamera(const gameplay::WorldSnapshot &snapshot);

  void follow(const gameplay::WorldSnapshot &snapshot, float delta);

  /// The world Y coordinate at the top of the view.
  [[nodiscard]] float y() const { return top; }

private:
  float top;
};

} // namespace neocity::camera
