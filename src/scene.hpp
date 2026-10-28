#pragma once

#include "camera/camera.hpp"
#include "rendering/drawing.hpp"

#include <gameplay/engine.hpp>

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <optional>

namespace neocity {

/// Godot composition root: wires the pure engine to input, camera, and canvas
/// adapters.
// The suppressed diagnostics fire inside godot-cpp's GDCLASS expansion.
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class AscentScene : public godot::Node2D {
  // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
  GDCLASS(AscentScene, godot::Node2D)

public:
  AscentScene();

  void _ready() override;
  void _process(double delta) override;
  void _draw() override;

  /// Exposes the actual actor location for host integration assertions.
  [[nodiscard]] godot::Vector2 actor_position() const;

protected:
  static void _bind_methods();

private:
  gameplay::AscentEngine engine;
  camera::AscentCamera camera;
  std::optional<rendering::AscentDrawing> drawing;
  double accumulator = 0.0;
  gameplay::PlayerCommand pending;
};

} // namespace neocity
