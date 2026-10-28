#pragma once

#include <cstdint>

#include "../camera/camera.hpp"
#include "../rendering/drawing.hpp"

#include <gameplay/engine.hpp>
#include <pan_dodge/engine.hpp>
#include <pan_tilt/engine.hpp>

#include <functional>
#include <optional>

namespace neocity::session {

/// The shared host's selectable screens.
enum class Screen : std::uint8_t { Menu, Dodge, Tilt, Ascent };

/// One host frame's sampled actions; each rules library keeps its own command.
struct Command {
  std::optional<Screen> selection;
  bool menu = false;
  bool retry = false;
  bool controls_held = false;
  pan_dodge::Command dodge;
  pan_tilt::Command tilt;
  gameplay::PlayerCommand ascent;
};

/// Owns selection and fixed-step delivery, using implementations supplied by
/// the scene composition root.
class Session {
public:
  /// Receives independent engines, the legacy camera, and its fresh-run reset.
  Session(pan_dodge::Engine &dodge_engine, pan_tilt::Engine &tilt_engine,
          gameplay::AscentEngine &ascent_engine,
          camera::AscentCamera &ascent_camera,
          std::function<void()> reset_legacy);
  /// Samples edges once and accumulates bounded frame time for the active run.
  void advance(double delta, Command command);
  /// Delegates the selected screen to its world painter and HUD.
  void paint(godot::Node2D &canvas,
             const rendering::AscentDrawing &drawing) const;
  /// Exposes visible host state for the shipped-scene verification harness.
  [[nodiscard]] godot::String screen_name() const;
  /// Exposes the active scenario's displayed outcome.
  [[nodiscard]] godot::String outcome() const;
  /// Exposes active player position in its rules library's coordinates.
  [[nodiscard]] godot::Vector2 actor_position() const;
  /// Exposes tilt player inertia for repeatable input-based route checks.
  [[nodiscard]] godot::Vector2 actor_velocity() const;
  /// Exposes the first sliding hazard for live shared-tilt checks.
  [[nodiscard]] godot::Vector2 hazard_position() const;
  /// Exposes the normalized pan tilt shown on screen.
  [[nodiscard]] godot::Vector2 pan_tilt() const;

private:
  void enter(Screen destination);
  void tick();
  std::reference_wrapper<pan_dodge::Engine> dodge;
  std::reference_wrapper<pan_tilt::Engine> tilt;
  std::reference_wrapper<gameplay::AscentEngine> ascent;
  std::reference_wrapper<camera::AscentCamera> camera;
  std::function<void()> reset_ascent;
  Screen screen = Screen::Menu;
  bool waiting_for_release = true;
  double accumulator = 0.0;
  Command pending;
};

} // namespace neocity::session
