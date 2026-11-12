#pragma once

#include <cstddef>
#include <cstdint>

#include "../hud/menus.hpp"
#include "../rendering/display.hpp"
#include "../rendering/drawing.hpp"
#include "sound.hpp"
#include "survival_run.hpp"
#include "tips.hpp"

#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <array>
#include <functional>
#include <optional>
#include <vector>

namespace neocity::session {

/// The host's screens: the title screen, its controls, How to play and
/// settings pages, the choice of players, and the survival game.
enum class Screen : std::uint8_t {
  Main,
  Controls,
  HowToPlay,
  Settings,
  SurvivalSetup,
  Survival
};

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
  /// Where the mouse is on the screen.
  std::optional<godot::Vector2> pointer;
  /// The mouse button went down this frame, at `pointer`.
  bool click = false;
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

/// Keeps the player's display choice for the session: the host applies and
/// stores a choice the session reports.
struct DisplayStage {
  /// The choice in force when the session opens.
  rendering::Display shown;
  /// Applies and stores a new choice.
  std::function<void(const rendering::Display &display)> change;
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
  /// Receives the survival run factory, the stage that plays cinematics, the
  /// one that plays sound, and the one that keeps the display choice.
  Session(StartSurvival start_survival, CinematicStage cinematic_stage,
          SoundStage sound_stage, DisplayStage display_stage);
  /// Samples edges once and advances the open screen; a survival run takes all
  /// of the frame time.
  void advance(double delta, Command command);
  /// Works out what the frame the last advance left shows, once, for every
  /// layer that draws it, and what a click on it acts on; nothing paints the
  /// open run until it is called.
  void aim(const rendering::Art &drawing);
  /// Draws the menus or the open game, over what `paint_world` and
  /// `paint_glow` draw, and leaves on `canvas` the lines it wrote as the
  /// `screen_text` meta, an array of strings, and the sizes it drew them at as
  /// the `screen_text_sizes` meta, an array of integers. The `hud_tip` meta
  /// is emptied first, so it holds only a tip this frame drew.
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
  /// Reports the open survival run, any cinematic over it, the tip on the
  /// screen and the tips the run has opened; empty with no run open.
  [[nodiscard]] godot::Dictionary survival_status() const;
  /// Reports the display choice: its `width`, `height`, `resolution` label and
  /// `mode`.
  [[nodiscard]] godot::Dictionary display_status() const;
  /// The display choice in force.
  [[nodiscard]] const rendering::Display &display_choice() const;

private:
  void advance_screen(double delta, const Command &command);
  /// Sets the presses a click at `point` stands for in `command`.
  void click_at(Command &command, godot::Vector2 point);
  /// What a click acts on in what the open screen shows.
  [[nodiscard]] std::vector<hud::Target>
  shown_targets(const rendering::Art &drawing) const;
  /// Draws the current screen for `paint`.
  void paint_screen(godot::Node2D &canvas, const rendering::Art &drawing) const;
  /// Whether the survival screen is open under no cinematic.
  [[nodiscard]] bool playing() const;
  void enter(Screen destination);
  void choose(std::size_t option);
  void select(hud::TitleEntry entry);
  void select(hud::SetupEntry entry);
  void advance_title(const Command &command);
  void advance_setup(const Command &command);
  void advance_settings(const Command &command);
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
  /// Up and down choose a row of the pause screen's sound and display rows,
  /// left and right change it.
  void adjust_pause(MenuStep step);
  /// Moves `setting` of the display choice `step` places and hands the new
  /// choice to the host when it differs.
  void change_display(hud::DisplaySetting setting, int step);
  /// The display rows of the pause screen or the settings page, with the row
  /// at index `marked` marked, or none.
  [[nodiscard]] std::vector<hud::DisplayRow>
  display_rows(std::optional<std::size_t> marked) const;
  /// The marked display row of the pause screen; none while a volume is
  /// marked.
  [[nodiscard]] std::optional<std::size_t> pause_display_row() const;
  /// The volume rows of the pause screen, marked only while a volume is.
  [[nodiscard]] std::array<hud::VolumeRow, 2> shown_volumes() const;
  void send_volume() const;
  /// Keeps the open screen's music playing.
  void keep_music() const;

  /// The number the next run's first logged event takes.
  std::uint64_t next_event = 0;
  /// The marked row of the pause screen: effects, music, resolution, window.
  std::size_t pause_row = 0;
  /// The marked row of the settings page: resolution, window.
  std::size_t settings_row = 0;
  /// The marked title entry, an index into `hud::TITLE_ENTRIES`.
  std::size_t cursor = 0;
  /// The marked choice of players, an index into `hud::SETUP_ENTRIES`.
  std::size_t setup_cursor = 0;
  /// What a click acts on in what the screen shows, as the last aim worked
  /// it out; a click acts on what the player saw.
  std::vector<hud::Target> targets;
  /// The flourishes under way in the open run.
  std::vector<rendering::Burst> bursts;
  StartSurvival start;
  std::optional<Playback> playback;
  /// The open run's first-play tips; a new run starts them afresh.
  Tips tips;
  DisplayStage displaying;
  /// What `framed` shows, as the last aim worked it out.
  std::optional<rendering::Shot> shot;
  CinematicStage stage;
  SoundStage sound;
  std::optional<SurvivalRun> run;
  /// The open run's frame as the last advance left it, which every layer
  /// draws and the status reports, so it is read once a frame.
  std::optional<hud::SurvivalFrame> framed;
  Strides strides{};
  std::optional<godot::Vector2> pointer;
  bool volume_sent = false;
  Screen screen = Screen::Main;
  bool waiting_for_release = true;
  std::optional<hud::Interruption> interruption;
  Volume volume;
};

} // namespace neocity::session
