#pragma once

#include <cstddef>
#include <cstdint>

#include "../hud/menus.hpp"
#include "../rendering/drawing.hpp"
#include "sound.hpp"
#include "survival_run.hpp"

#include <godot_cpp/variant/dictionary.hpp>

#include <array>
#include <functional>
#include <optional>
#include <vector>

namespace neocity::session {

/// The host's screens: the title screen, its controls page, the choice of
/// players, and the survival game.
enum class Screen : std::uint8_t { Main, Controls, SurvivalSetup, Survival };

/// A press that moves the cursor of the title screen or the choice of players,
/// or, left and right, changes a volume on the pause screen.
enum class MenuStep : std::uint8_t { Up, Down, Left, Right };

/// One host frame's sampled actions.
struct Command {
  /// Menu option pressed this frame, numbered from 1.
  std::optional<std::size_t> option;
  /// A menu cursor's up or down press this frame.
  std::optional<MenuStep> step;
  bool menu = false;
  /// The pause key, which also goes back from a menu.
  bool pause = false;
  /// The key that accepts a question on screen or a menu's marked entry.
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
  /// Receives the survival run factory, the stage that plays cinematics, and
  /// the one that plays sound.
  Session(StartSurvival start_survival, CinematicStage cinematic_stage,
          SoundStage sound_stage);
  /// Samples edges once and advances the open screen; a survival run takes all
  /// of the frame time.
  void advance(double delta, Command command);
  /// Works out what the frame the last advance left shows, once, for every
  /// layer that draws it; nothing paints the open run until it is called.
  void aim(const rendering::Art &drawing);
  /// Draws the menus or the open game, over what `paint_world` and
  /// `paint_glow` draw.
  void paint(godot::Node2D &canvas, const rendering::Art &drawing) const;
  /// Draws the open run's scene, behind everything `paint` draws.
  void paint_world(godot::Node2D &canvas, const rendering::Art &drawing) const;
  /// Draws the open run's light over its scene, for a canvas that blends by
  /// adding.
  void paint_glow(godot::Node2D &canvas, const rendering::Art &drawing) const;
  /// Names the current screen.
  [[nodiscard]] godot::String screen_name() const;
  /// Who drives each survival actor, or nothing with no run open.
  [[nodiscard]] std::optional<survival::ControlMap> survival_control() const;
  /// Reports the open survival run and any cinematic over it; empty with no
  /// run open.
  [[nodiscard]] godot::Dictionary survival_status() const;

private:
  void advance_screen(double delta, const Command &command);
  /// Whether a run plays under no cinematic, with its frame and shot read.
  [[nodiscard]] bool playing() const;
  void enter(Screen destination);
  void choose(std::size_t option);
  void select(hud::TitleEntry entry);
  void select(hud::SetupEntry entry);
  void advance_title(const Command &command);
  void advance_setup(const Command &command);
  void start_run(survival::ControlMap control);
  void advance_run(SurvivalRun &open, double delta, const Command &command);
  void advance_interruption(hud::Interruption open, const Command &command);
  void play(hud::Cinematic cinematic);
  void advance_cinematic(Playback &open, double delta, const Command &command);
  void finish_cinematic(hud::Cinematic cinematic);
  /// Plays `cue` unless the effects are off.
  void cue(audio::Cue cue) const;
  /// Plays the moments of a run's frame and starts their flourishes.
  void react(const std::vector<Moment> &found);
  /// Moves the flourishes on by `delta` seconds and drops the finished ones.
  void age_bursts(double delta);
  void adjust_volume(MenuStep step);
  void send_volume() const;
  /// Keeps the open screen's music playing.
  void keep_music() const;

  StartSurvival start;
  std::optional<SurvivalRun> run;
  /// The open run's frame as the last advance left it, which every layer
  /// draws, so it is read once a frame.
  std::optional<hud::SurvivalFrame> framed;
  /// What `framed` shows, as the last aim worked it out.
  std::optional<rendering::Shot> shot;
  CinematicStage stage;
  SoundStage sound;
  Volume volume;
  bool volume_sent = false;
  Strides strides{};
  /// The flourishes under way in the open run.
  std::vector<rendering::Burst> bursts;
  std::optional<Playback> playback;
  std::optional<hud::Interruption> interruption;
  Screen screen = Screen::Main;
  /// The marked title entry, an index into `hud::TITLE_ENTRIES`.
  std::size_t cursor = 0;
  /// The marked choice of players, an index into `hud::SETUP_ENTRIES`.
  std::size_t setup_cursor = 0;
  bool waiting_for_release = true;
};

} // namespace neocity::session
