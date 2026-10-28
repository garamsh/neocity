#pragma once

#include <cstddef>
#include <cstdint>

#include "../camera/camera.hpp"
#include "../experiment/catalog.hpp"
#include "../rendering/drawing.hpp"

#include <gameplay/engine.hpp>

#include <functional>
#include <optional>
#include <span>

namespace neocity::session {

/// The shared host's screens: the main menu, the developer page, the main
/// game, and one mounted experiment scene.
enum class Screen : std::uint8_t { Main, Developer, Ascent, Experiment };

/// One host frame's sampled actions.
struct Command {
  /// Menu option pressed this frame, numbered from 1.
  std::optional<std::size_t> option;
  bool menu = false;
  bool controls_held = false;
  gameplay::PlayerCommand ascent;
};

/// Mounts the given experiment scene, or removes the mounted one when empty.
using Mount = std::function<void(std::optional<godot::String> scene)>;

/// Owns menu navigation, run lifetime, and fixed-step delivery to the main
/// game, using implementations supplied by the scene composition root.
class Session {
public:
  /// Receives the main game, its camera and fresh-run reset, the registered
  /// experiments, and the operation that mounts their scenes.
  Session(gameplay::AscentEngine &ascent_engine,
          camera::AscentCamera &ascent_camera,
          std::function<void()> reset_ascent,
          std::span<const experiment::Entry> experiments, Mount mount_scene);
  /// Samples edges once and accumulates bounded frame time for the main game.
  void advance(double delta, Command command);
  /// Draws the menus or the main game; a mounted experiment draws itself.
  void paint(godot::Node2D &canvas,
             const rendering::AscentDrawing &drawing) const;
  /// Names the current screen, using the experiment id while one is open.
  [[nodiscard]] godot::String screen_name() const;
  /// Exposes the main game's leader position for host integration checks.
  [[nodiscard]] godot::Vector2 actor_position() const;

private:
  void enter(Screen destination);
  void choose(std::size_t option);
  void tick_ascent(double delta);

  std::reference_wrapper<gameplay::AscentEngine> ascent;
  std::reference_wrapper<camera::AscentCamera> camera;
  std::function<void()> reset;
  std::span<const experiment::Entry> catalog;
  Mount mount;
  Screen screen = Screen::Main;
  experiment::Entry active;
  bool mounted = false;
  bool waiting_for_release = true;
  double accumulator = 0.0;
  gameplay::PlayerCommand pending;
};

} // namespace neocity::session
