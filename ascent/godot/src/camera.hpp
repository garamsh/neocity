#pragma once

#include <ascent/world_snapshot.hpp>

namespace neocity::ascent {

/// Vertical camera that follows the leading actor through the course.
class AscentCamera {
public:
  explicit AscentCamera(const WorldSnapshot &snapshot);

  void follow(const WorldSnapshot &snapshot, float delta);

  /// The world Y coordinate at the top of the view.
  [[nodiscard]] float y() const { return top; }

private:
  float top;
};

} // namespace neocity::ascent
