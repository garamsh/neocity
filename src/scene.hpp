#pragma once

#include "camera/camera.hpp"
#include "rendering/drawing.hpp"
#include "session/session.hpp"

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

  /// Reports the selected experiment or menu.
  [[nodiscard]] godot::String screen_name() const;
  /// Reports the displayed outcome of the active experiment.
  [[nodiscard]] godot::String outcome() const;
  /// Reports the tilt player's current velocity for input route checks.
  [[nodiscard]] godot::Vector2 actor_velocity() const;
  /// Reports a sliding hazard's location for live tilt checks.
  [[nodiscard]] godot::Vector2 hazard_position() const;
  /// Reports the displayed tilt direction.
  [[nodiscard]] godot::Vector2 pan_tilt() const;

protected:
  static void _bind_methods();

private:
  gameplay::AscentEngine engine;
  pan_dodge::Engine dodge;
  pan_tilt::Engine tilt;
  camera::AscentCamera camera;
  session::Session session;
  std::optional<rendering::AscentDrawing> drawing;
};

} // namespace neocity
