#include "session.hpp"

#include "../hud/menus.hpp"
#include "../hud/survival.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <type_traits>
#include <utility>
#include <variant>

namespace neocity::session {
namespace {

/// How long each fallback card stands without a skip; temporary values.
constexpr double INTRO_CARD_SECONDS = 4.0;
constexpr double ENDING_CARD_SECONDS = 6.0;
constexpr double LOSS_CARD_SECONDS = 6.0;
constexpr const char *INTRO_SCENE =
    "res://levels/monster_mouth_escape/cinematics/intro/intro.tscn";
constexpr const char *ENDING_SCENE =
    "res://levels/monster_mouth_escape/cinematics/ending/ending.tscn";
constexpr const char *LOSS_SCENE =
    "res://levels/monster_mouth_escape/cinematics/loss/loss.tscn";

const char *cinematic_scene(hud::Cinematic cinematic) {
  switch (cinematic) {
  case hud::Cinematic::Intro:
    return INTRO_SCENE;
  case hud::Cinematic::Ending:
    return ENDING_SCENE;
  case hud::Cinematic::Loss:
    return LOSS_SCENE;
  }
  std::abort();
}

double card_seconds(hud::Cinematic cinematic) {
  switch (cinematic) {
  case hud::Cinematic::Intro:
    return INTRO_CARD_SECONDS;
  case hud::Cinematic::Ending:
    return ENDING_CARD_SECONDS;
  case hud::Cinematic::Loss:
    return LOSS_CARD_SECONDS;
  }
  std::abort();
}

const char *cinematic_name(hud::Cinematic cinematic) {
  switch (cinematic) {
  case hud::Cinematic::Intro:
    return "intro";
  case hud::Cinematic::Ending:
    return "ending";
  case hud::Cinematic::Loss:
    return "loss";
  }
  std::abort();
}

const char *interruption_name(hud::Interruption interruption) {
  switch (interruption) {
  case hud::Interruption::Paused:
    return "paused";
  case hud::Interruption::ConfirmLeave:
  case hud::Interruption::ConfirmLeaveFromPause:
    return "confirm_leave";
  case hud::Interruption::Resuming:
    return "resuming";
  }
  std::abort();
}

/// Moves `cursor` through `count` entries, wrapping at both ends.
void move_cursor(std::size_t &cursor, std::size_t count,
                 std::optional<MenuStep> step) {
  if (step == MenuStep::Up) {
    cursor = (cursor + count - 1) % count;
  } else if (step == MenuStep::Down) {
    cursor = (cursor + 1) % count;
  }
}

/// Marks the volume of `bus` and sets it to `level`.
void set_volume(Volume &volume, audio::Bus bus, std::uint8_t level) {
  volume.marked = bus;
  (bus == audio::Bus::Effects ? volume.effects : volume.music) =
      std::min(level, VOLUME_STEPS);
}

void press_menu_key(Command &command, hud::MenuKey key) {
  switch (key) {
  case hud::MenuKey::Confirm:
    command.confirm = true;
    return;
  case hud::MenuKey::Pause:
    command.pause = true;
    return;
  case hud::MenuKey::Menu:
    command.menu = true;
    return;
  }
  std::abort();
}

bool &key_of(survival::PlayerInput &input, hud::PlayerKey key) {
  switch (key) {
  case hud::PlayerKey::Interact:
    return input.interact;
  case hud::PlayerKey::Feed:
    return input.feed;
  case hud::PlayerKey::EndDay:
    return input.end_day;
  case hud::PlayerKey::ReplaceTool:
    return input.replace_tool;
  case hud::PlayerKey::Travel:
    return input.travel;
  }
  std::abort();
}

/// Presses the key of the human `press` names, or of every human.
void press_player_key(Command &command, survival::ControlMap control,
                      const hud::PlayerPress &press) {
  constexpr std::array actors{survival::Actor::Chef,
                              survival::Actor::Appraiser};
  for (std::size_t index = 0; index < actors.size(); ++index) {
    const auto actor = actors.at(index);
    if (survival::key_set(control, actor) &&
        (!press.actor || *press.actor == actor)) {
      key_of(command.survival.at(index), press.key) = true;
    }
  }
}

/// Ends the application; the running host's main loop is always its SceneTree.
void quit_game() {
  auto *tree = godot::Object::cast_to<godot::SceneTree>(
      godot::Engine::get_singleton()->get_main_loop());
  if (tree == nullptr) {
    std::abort();
  }
  tree->quit();
}

} // namespace

Session::Session(StartSurvival start_survival, CinematicStage cinematic_stage,
                 SoundStage sound_stage)
    : start(std::move(start_survival)), stage(std::move(cinematic_stage)),
      sound(std::move(sound_stage)) {}

void Session::enter(Screen destination) {
  if (playback) {
    stage.stop();
    playback.reset();
  }
  screen = destination;
  if (run) {
    next_event = run->next_event();
  }
  run.reset();
  interruption.reset();
  bursts.clear();
  strides = {};
  waiting_for_release = true;
}

void Session::choose(std::size_t option) {
  cue(audio::Cue::MenuSelect);
  switch (screen) {
  case Screen::Main:
    if (option >= 1 && option <= hud::TITLE_ENTRIES.size()) {
      select(hud::TITLE_ENTRIES.at(option - 1));
    }
    return;
  case Screen::SurvivalSetup:
    if (option >= 1 && option <= hud::SETUP_ENTRIES.size()) {
      select(hud::SETUP_ENTRIES.at(option - 1));
    }
    return;
  case Screen::Controls:
  case Screen::Survival:
    return;
  }
  std::abort();
}

void Session::select(hud::TitleEntry entry) {
  switch (entry) {
  case hud::TitleEntry::NewGame:
    enter(Screen::SurvivalSetup);
    return;
  case hud::TitleEntry::Controls:
    enter(Screen::Controls);
    return;
  case hud::TitleEntry::Quit:
    quit_game();
    return;
  }
  std::abort();
}

void Session::select(hud::SetupEntry entry) {
  switch (entry) {
  case hud::SetupEntry::Chef:
    start_run(survival::single_player(survival::Actor::Chef));
    return;
  case hud::SetupEntry::Appraiser:
    start_run(survival::single_player(survival::Actor::Appraiser));
    return;
  case hud::SetupEntry::TwoPlayers:
    start_run(survival::two_players());
    return;
  }
  std::abort();
}

// A step and Enter in one frame select the entry stepped to.
void Session::advance_title(const Command &command) {
  if (command.step == MenuStep::Up || command.step == MenuStep::Down) {
    cue(audio::Cue::MenuMove);
  }
  move_cursor(cursor, hud::TITLE_ENTRIES.size(), command.step);
  if (command.confirm) {
    cue(audio::Cue::MenuSelect);
    select(hud::TITLE_ENTRIES.at(cursor));
  }
}

void Session::advance_setup(const Command &command) {
  if (command.step == MenuStep::Up || command.step == MenuStep::Down) {
    cue(audio::Cue::MenuMove);
  }
  move_cursor(setup_cursor, hud::SETUP_ENTRIES.size(), command.step);
  if (command.confirm) {
    cue(audio::Cue::MenuSelect);
    select(hud::SETUP_ENTRIES.at(setup_cursor));
  }
}

void Session::start_run(survival::ControlMap control) {
  enter(Screen::Survival);
  run.emplace(start(), control, next_event);
  play(hud::Cinematic::Intro);
}

void Session::play(hud::Cinematic cinematic) {
  playback = Playback{.cinematic = cinematic, .card_left = std::nullopt};
  if (!stage.play(cinematic_scene(cinematic))) {
    playback->card_left = card_seconds(cinematic);
  }
}

// Only the skip reaches a cinematic; the run takes no key or time meanwhile.
void Session::advance_cinematic(Playback &open, double delta,
                                const Command &command) {
  const bool skipped = std::ranges::any_of(
      command.survival,
      [](const survival::PlayerInput &keys) { return keys.interact; });
  if (open.card_left) {
    if (std::isfinite(delta) && delta > 0.0) {
      *open.card_left -= delta;
    }
    if (skipped || *open.card_left <= 0.0) {
      finish_cinematic(open.cinematic);
    }
    return;
  }
  if (skipped) {
    stage.stop();
  }
  if (!stage.playing()) {
    finish_cinematic(open.cinematic);
  }
}

// The intro hands the run its first frame; the ending and the loss close it.
void Session::finish_cinematic(hud::Cinematic cinematic) {
  playback.reset();
  if (cinematic != hud::Cinematic::Intro) {
    enter(Screen::Main);
  }
}

void Session::advance(double delta, Command command) {
  pointer = command.pointer;
  if (command.click && command.pointer) {
    click_at(command, *command.pointer);
  }
  advance_screen(delta, command);
  framed.reset();
  shot.reset();
  if (run) {
    framed = run->frame();
  }
}

void Session::aim(const rendering::Art &drawing) {
  if (framed) {
    shot = hud::shot_of(drawing, *framed);
  }
  targets = shown_targets(drawing);
}

std::vector<hud::Target>
Session::shown_targets(const rendering::Art &drawing) const {
  switch (screen) {
  case Screen::Main:
    return hud::title_targets();
  case Screen::Controls:
    return hud::controls_targets(drawing);
  case Screen::SurvivalSetup:
    return hud::setup_targets();
  case Screen::Survival:
    if (playback || !framed) {
      return {};
    }
    if (interruption) {
      return hud::interruption_targets(drawing, *interruption,
                                       volume_rows(volume));
    }
    return hud::survival_targets(drawing, *framed);
  }
  std::abort();
}

// A click presses what a key would, for the target the player saw under it;
// a refused target presses nothing.
void Session::click_at(Command &command, godot::Vector2 point) {
  const auto *target = hud::target_at(targets, point);
  if (target == nullptr || !target->refusal.is_empty()) {
    return;
  }
  std::visit(
      [this, &command](const auto &press) {
        using Pressed = std::decay_t<decltype(press)>;
        if constexpr (std::is_same_v<Pressed, hud::OptionPress>) {
          command.option = press.number;
        } else if constexpr (std::is_same_v<Pressed, hud::MenuKey>) {
          press_menu_key(command, press);
        } else if constexpr (std::is_same_v<Pressed, hud::PlayerPress>) {
          if (run) {
            press_player_key(command, run->control(), press);
          }
        } else if constexpr (std::is_same_v<Pressed, hud::DishPress>) {
          command.dish_step = press.direction;
        } else {
          set_volume(volume, press.bus, press.level);
          send_volume();
          cue(audio::Cue::MenuMove);
        }
      },
      target->press);
}

void Session::advance_screen(double delta, const Command &command) {
  if (!volume_sent) {
    send_volume();
    volume_sent = true;
  }
  keep_music();
  if (interruption) {
    advance_interruption(*interruption, command);
    return;
  }
  // Esc goes back from a menu; in a run it pauses, in advance_run.
  if (command.menu || (command.pause && screen != Screen::Survival)) {
    switch (screen) {
    case Screen::Main:
      break;
    case Screen::Survival:
      // Mid-run the key asks first, in advance_run.
      if (!playback) {
        break;
      }
      enter(Screen::Main);
      return;
    case Screen::Controls:
    case Screen::SurvivalSetup:
      cue(audio::Cue::MenuBack);
      enter(Screen::Main);
      return;
    }
  }
  // A key held across a transition cannot become the next screen's first
  // action.
  if (waiting_for_release) {
    waiting_for_release = command.controls_held;
    return;
  }
  if (command.option &&
      (screen == Screen::Main || screen == Screen::SurvivalSetup)) {
    choose(*command.option);
    return;
  }
  if (screen == Screen::Main) {
    advance_title(command);
    return;
  }
  if (screen == Screen::SurvivalSetup) {
    advance_setup(command);
    return;
  }
  if (playback) {
    advance_cinematic(*playback, delta, command);
    return;
  }
  if (run) {
    advance_run(*run, delta, command);
  }
}

// The frame of the press was played up to it, so it spends its time, but no key
// pressed before the run stopped may act once it continues.
void Session::advance_run(SurvivalRun &open, double delta,
                          const Command &command) {
  std::optional<hud::Interruption> opening;
  if (command.menu) {
    opening = hud::Interruption::ConfirmLeave;
  } else if (command.pause) {
    opening = hud::Interruption::Paused;
  }
  age_bursts(delta);
  const auto before = open.view();
  std::vector<hud::Event> events;
  if (opening) {
    open.drop_input();
    events = open.advance(delta, {}, survival::Direction::Idle);
  } else {
    events = open.advance(delta, command.survival, command.dish_step);
  }
  react(moments(before, open.view(), events, strides));
  if (opening) {
    cue(audio::Cue::MenuSelect);
  }
  if (open.escaped()) {
    play(hud::Cinematic::Ending);
  } else if (open.lost()) {
    play(hud::Cinematic::Loss);
  } else {
    interruption = opening;
  }
}

// The run takes no key and no time here; a held key is waited out before it
// continues, under the same rule as a transition.
void Session::advance_interruption(hud::Interruption open,
                                   const Command &command) {
  switch (open) {
  case hud::Interruption::Paused:
    if (command.pause) {
      cue(audio::Cue::MenuBack);
      interruption = hud::Interruption::Resuming;
    } else if (command.step) {
      adjust_volume(*command.step);
    } else if (command.menu) {
      interruption = hud::Interruption::ConfirmLeaveFromPause;
    }
    return;
  case hud::Interruption::ConfirmLeave:
    if (command.confirm) {
      enter(Screen::Main);
    } else if (command.pause) {
      interruption = hud::Interruption::Resuming;
    }
    return;
  case hud::Interruption::ConfirmLeaveFromPause:
    if (command.confirm) {
      enter(Screen::Main);
    } else if (command.pause) {
      interruption = hud::Interruption::Paused;
    }
    return;
  case hud::Interruption::Resuming:
    if (!command.controls_held) {
      interruption.reset();
    }
    return;
  }
  std::abort();
}

void Session::cue(audio::Cue cue) const {
  if (volume.effects > 0) {
    sound.play(cue);
  }
}

void Session::react(const std::vector<Moment> &found) {
  for (const auto &moment : found) {
    cue(moment.cue);
    if (const auto flourish = flourish_of(moment.cue)) {
      bursts.push_back({.kind = *flourish, .at = moment.at, .age = 0.0F});
    }
  }
}

void Session::age_bursts(double delta) {
  if (!std::isfinite(delta) || delta <= 0.0) {
    return;
  }
  for (auto &burst : bursts) {
    burst.age += static_cast<float>(delta);
  }
  std::erase_if(bursts, [](const rendering::Burst &burst) {
    return burst.age >= rendering::lifetime(burst.kind);
  });
}

// Up and down choose the bus; left and right change its level by one step.
void Session::adjust_volume(MenuStep step) {
  auto &level =
      volume.marked == audio::Bus::Effects ? volume.effects : volume.music;
  switch (step) {
  case MenuStep::Up:
  case MenuStep::Down:
    volume.marked = volume.marked == audio::Bus::Effects ? audio::Bus::Music
                                                         : audio::Bus::Effects;
    break;
  case MenuStep::Left:
    level = level > 0 ? static_cast<std::uint8_t>(level - 1) : level;
    break;
  case MenuStep::Right:
    level = level < VOLUME_STEPS ? static_cast<std::uint8_t>(level + 1) : level;
    break;
  }
  send_volume();
  cue(audio::Cue::MenuMove);
}

void Session::send_volume() const {
  sound.volume(audio::Bus::Effects, loudness(volume.effects));
  sound.volume(audio::Bus::Music, loudness(volume.music));
}

// Off music plays nothing; a cinematic and a finished run bring their own.
void Session::keep_music() const {
  auto track = audio::Track::Title;
  float tension = 0.0F;
  if (screen == Screen::Survival) {
    track = audio::Track::Silence;
    if (run && !playback) {
      const auto progression = run->view().progression;
      tension = run->acid_fill();
      if (progression.phase == survival::Phase::Exploration) {
        track = audio::Track::City;
      } else if (progression.phase == survival::Phase::Workshop) {
        track = audio::Track::Workshop;
      }
    }
  }
  sound.music(volume.music > 0 ? track : audio::Track::Silence, tension);
}

void Session::paint(godot::Node2D &canvas,
                    const rendering::Art &drawing) const {
  // Lines another canvas wrote since are not this one's.
  static_cast<void>(drawing.take_written());
  static_cast<void>(drawing.take_sizes());
  paint_screen(canvas, drawing);
  hud::paint_pointer(canvas, drawing, targets, pointer);
  godot::PackedStringArray lines;
  for (const auto &line : drawing.take_written()) {
    lines.push_back(line);
  }
  canvas.set_meta("screen_text", lines);
  godot::PackedInt32Array sizes;
  for (const int size : drawing.take_sizes()) {
    sizes.push_back(size);
  }
  canvas.set_meta("screen_text_sizes", sizes);
}

void Session::paint_screen(godot::Node2D &canvas,
                           const rendering::Art &drawing) const {
  switch (screen) {
  case Screen::Main:
    hud::paint_title(canvas, drawing, hud::TITLE_ENTRIES.at(cursor));
    return;
  case Screen::Controls:
    hud::paint_controls(canvas, drawing);
    return;
  case Screen::SurvivalSetup:
    hud::paint_survival_setup(canvas, drawing,
                              hud::SETUP_ENTRIES.at(setup_cursor));
    return;
  case Screen::Survival:
    if (playback && framed) {
      hud::paint_cinematic(canvas, drawing, playback->cinematic,
                           playback->card_left.has_value(), *framed);
    } else if (framed && shot) {
      hud::paint_survival(canvas, drawing, *framed, *shot, bursts);
      if (interruption) {
        hud::paint_interruption(canvas, drawing, *interruption, *framed,
                                volume_rows(volume));
      }
    }
    return;
  }
  std::abort();
}

bool Session::playing() const {
  return screen == Screen::Survival && !playback;
}

void Session::paint_world(godot::Node2D &canvas,
                          const rendering::Art &drawing) const {
  if (playing() && framed && shot) {
    hud::paint_survival_world(canvas, drawing, *framed, *shot, bursts);
  }
}

void Session::paint_glow(godot::Node2D &canvas,
                         const rendering::Art &drawing) const {
  if (playing() && framed && shot) {
    hud::paint_survival_glow(canvas, drawing, *framed, *shot, bursts);
  }
}

godot::String Session::screen_name() const {
  switch (screen) {
  case Screen::Main:
    return "main";
  case Screen::Controls:
    return "controls";
  case Screen::SurvivalSetup:
    return "survival_setup";
  case Screen::Survival:
    return "survival";
  }
  std::abort();
}

std::optional<survival::ControlMap> Session::survival_control() const {
  return run ? std::optional{run->control()} : std::nullopt;
}

godot::Dictionary Session::survival_status() const {
  if (!run || !framed) {
    return {};
  }
  auto status = run->status(*framed);
  if (playback) {
    status.set("cinematic", cinematic_name(playback->cinematic));
    status.set("cinematic_source", playback->card_left ? "fallback" : "scene");
  }
  godot::Array shown;
  for (const auto &burst : bursts) {
    shown.push_back(rendering::flourish_name(burst.kind));
  }
  status.set("flourishes", shown);
  if (interruption) {
    status.set("interruption", interruption_name(*interruption));
    godot::Array prompt;
    for (const auto &line : hud::interruption_text(*interruption)) {
      prompt.push_back(line);
    }
    status.set("prompt", prompt);
  }
  return status;
}

} // namespace neocity::session
