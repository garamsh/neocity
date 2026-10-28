#pragma once

#include "camera/camera.hpp"
#include "rendering/drawing.hpp"
#include "session/session.hpp"

#include <gameplay/engine.hpp>

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <optional>

namespace neocity {

/// Godot composition root: wires the main game, the menus, and the registered
/// experiments to input, camera, and canvas adapters.
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

  /// Exposes the main game's actor location for host integration assertions.
  [[nodiscard]] godot::Vector2 actor_position() const;
  /// Reports the open screen, or the id of the mounted experiment.
  [[nodiscard]] godot::String screen_name() const;

protected:
  static void _bind_methods();

private:
  void mount(const std::optional<godot::String> &scene);

  gameplay::AscentEngine engine;
  camera::AscentCamera camera;
  session::Session session;
  std::optional<rendering::AscentDrawing> drawing;
};

} // namespace neocity
