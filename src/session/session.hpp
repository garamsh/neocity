#pragma once

#include <cstddef>
#include <cstdint>

#include "../hud/menus.hpp"
#include "../rendering/drawing.hpp"
#include "survival_run.hpp"

#include <godot_cpp/variant/dictionary.hpp>

#include <array>
#include <functional>
#include <optional>

namespace neocity::session {

/// The host's screens: the title screen, its controls page, the choice of
/// players, and the survival game.
enum class Screen : std::uint8_t { Main, Controls, SurvivalSetup, Survival };

/// A press that moves the title screen's cursor.
enum class MenuStep : std::uint8_t { Up, Down };

/// One host frame's sampled actions.
struct Command {
  /// Menu option pressed this frame, numbered from 1.
  std::optional<std::size_t> option;
  /// The title cursor's up or down press this frame.
  std::optional<MenuStep> step;
  bool menu = false;
  /// The pause key, which also goes back from a menu.
  bool pause = false;
  /// The key that accepts a question on screen or the title's marked entry.
  bool confirm = false;
  bool controls_held = false;
  /// Each actor's sampled keys, chef first, while a survival run is open.
  std::array<survival::PlayerInput, 2> survival;
  /// A human chef's up or down press this frame.
  survival::Direction dish_step = survival::Direction::Idle;
};

/// Plays one cinematic scene at a time for the session, without ever failing
/// the host.
struct CinematicStage {
  /// Loads and starts a scene; false when it cannot play, leaving nothing
  /// open.
  std::function<bool(const godot::String &scene)> play;
  /// Whether the started scene is still running.
  std::function<bool()> playing;
  /// Closes the started scene, if any.
  std::function<void()> stop;
};

/// Builds a fresh run's rule objects and hands back references to them.
using StartSurvival = std::function<SurvivalFeatures()>;

/// The open cinematic. `card_left` is the fallback card's remaining seconds,
/// and empty while a scene plays on the stage.
struct Playback {
  hud::Cinematic cinematic = hud::Cinematic::Intro;
  std::optional<double> card_left;
};

/// Owns menu navigation, run lifetime, and fixed-step delivery to the
/// survival game, using implementations supplied by the scene composition
/// root.
class Session {
public:
  /// Receives the survival run factory and the stage that plays cinematics.
  Session(StartSurvival start_survival, CinematicStage cinematic_stage);
  /// Samples edges once and advances the open screen; a survival run takes all
  /// of the frame time.
  void advance(double delta, Command command);
  /// Draws the menus or the open game.
  void paint(godot::Node2D &canvas,
             const rendering::AscentDrawing &drawing) const;
  /// Names the current screen.
  [[nodiscard]] godot::String screen_name() const;
  /// Who drives each survival actor, or nothing with no run open.
  [[nodiscard]] std::optional<survival::ControlMap> survival_control() const;
  /// Reports the open survival run and any cinematic over it; empty with no
  /// run open.
  [[nodiscard]] godot::Dictionary survival_status() const;

private:
  void enter(Screen destination);
  void choose(std::size_t option);
  void select(hud::TitleEntry entry);
  void advance_title(const Command &command);
  void start_run(survival::ControlMap control);
  void advance_run(SurvivalRun &open, double delta, const Command &command);
  void advance_interruption(hud::Interruption open, const Command &command);
  void play(hud::Cinematic cinematic);
  void advance_cinematic(Playback &open, double delta, const Command &command);
  void finish_cinematic(hud::Cinematic cinematic);

  StartSurvival start;
  std::optional<SurvivalRun> run;
  CinematicStage stage;
  std::optional<Playback> playback;
  std::optional<hud::Interruption> interruption;
  Screen screen = Screen::Main;
  /// The marked title entry, an index into `hud::TITLE_ENTRIES`.
  std::size_t cursor = 0;
  bool waiting_for_release = true;
};

} // namespace neocity::session
