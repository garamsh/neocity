#pragma once

#include <cstddef>
#include <cstdint>

#include "../camera/camera.hpp"
#include "../experiment/catalog.hpp"
#include "../rendering/drawing.hpp"
#include "survival_run.hpp"

#include <gameplay/engine.hpp>

#include <godot_cpp/variant/dictionary.hpp>

#include <array>
#include <functional>
#include <optional>
#include <span>

namespace neocity::session {

/// The shared host's screens: the main menu, the developer page, the survival
/// game and its choice of players, the ascent prototype, and one mounted
/// experiment scene.
enum class Screen : std::uint8_t {
  Main,
  Developer,
  SurvivalSetup,
  Survival,
  Ascent,
  Experiment
};

/// One host frame's sampled actions.
struct Command {
  /// Menu option pressed this frame, numbered from 1.
  std::optional<std::size_t> option;
  bool menu = false;
  bool controls_held = false;
  gameplay::PlayerCommand ascent;
  /// Each actor's sampled keys, chef first, while a survival run is open.
  std::array<survival::PlayerInput, 2> survival;
};

/// Mounts the given experiment scene, or removes the mounted one when empty.
using Mount = std::function<void(std::optional<godot::String> scene)>;

/// Builds a fresh run's rule objects and hands back references to them.
using StartSurvival = std::function<SurvivalFeatures()>;

/// Owns menu navigation, run lifetime, and fixed-step delivery to both games,
/// using implementations supplied by the scene composition root.
class Session {
public:
  /// Receives the ascent game, its camera and fresh-run reset, the survival
  /// run factory, the registered experiments, and the operation that mounts
  /// their scenes.
  Session(gameplay::AscentEngine &ascent_engine,
          camera::AscentCamera &ascent_camera,
          std::function<void()> reset_ascent, StartSurvival start_survival,
          std::span<const experiment::Entry> experiments, Mount mount_scene);
  /// Samples edges once and advances the open game: ascent on bounded frame
  /// time, survival on all of it.
  void advance(double delta, Command command);
  /// Draws the menus or the open game; a mounted experiment draws itself.
  void paint(godot::Node2D &canvas,
             const rendering::AscentDrawing &drawing) const;
  /// Names the current screen, using the experiment id while one is open.
  [[nodiscard]] godot::String screen_name() const;
  /// Exposes the ascent game's leader position for host integration checks.
  [[nodiscard]] godot::Vector2 actor_position() const;
  /// Who drives each survival actor, or nothing with no run open.
  [[nodiscard]] std::optional<survival::ControlMap> survival_control() const;
  /// Reports the open survival run; empty with no run open.
  [[nodiscard]] godot::Dictionary survival_status() const;

private:
  void enter(Screen destination);
  void choose(std::size_t option);
  void start_run(survival::ControlMap control);
  void tick_ascent(double delta);

  std::reference_wrapper<gameplay::AscentEngine> ascent;
  std::reference_wrapper<camera::AscentCamera> camera;
  std::function<void()> reset;
  StartSurvival start;
  std::optional<SurvivalRun> run;
  std::span<const experiment::Entry> catalog;
  Mount mount;
  Screen screen = Screen::Main;
  experiment::Entry active;
  bool waiting_for_release = true;
  double accumulator = 0.0;
  gameplay::PlayerCommand pending;
};

} // namespace neocity::session
