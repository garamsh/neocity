#include "session.hpp"

#include "../hud/hud.hpp"
#include "../hud/menus.hpp"
#include "../hud/survival.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <utility>

namespace neocity::session {
namespace {

constexpr double TICK = 1.0 / 120.0;
/// How long each fallback card stands without a skip; temporary values.
constexpr double INTRO_CARD_SECONDS = 4.0;
constexpr double ENDING_CARD_SECONDS = 6.0;
constexpr const char *INTRO_SCENE =
    "res://levels/monster_mouth_escape/cinematics/intro/intro.tscn";
constexpr const char *ENDING_SCENE =
    "res://levels/monster_mouth_escape/cinematics/ending/ending.tscn";

const char *cinematic_scene(hud::Cinematic cinematic) {
  switch (cinematic) {
  case hud::Cinematic::Intro:
    return INTRO_SCENE;
  case hud::Cinematic::Ending:
    return ENDING_SCENE;
  }
  std::abort();
}

double card_seconds(hud::Cinematic cinematic) {
  switch (cinematic) {
  case hud::Cinematic::Intro:
    return INTRO_CARD_SECONDS;
  case hud::Cinematic::Ending:
    return ENDING_CARD_SECONDS;
  }
  std::abort();
}

const char *cinematic_name(hud::Cinematic cinematic) {
  switch (cinematic) {
  case hud::Cinematic::Intro:
    return "intro";
  case hud::Cinematic::Ending:
    return "ending";
  }
  std::abort();
}

} // namespace

Session::Session(gameplay::AscentEngine &ascent_engine,
                 camera::AscentCamera &ascent_camera,
                 std::function<void()> reset_ascent,
                 StartSurvival start_survival,
                 std::span<const experiment::Entry> experiments,
                 Mount mount_scene, CinematicStage cinematic_stage)
    : ascent(ascent_engine), camera(ascent_camera),
      reset(std::move(reset_ascent)), start(std::move(start_survival)),
      catalog(experiments), mount(std::move(mount_scene)),
      stage(std::move(cinematic_stage)) {}

void Session::enter(Screen destination) {
  if (screen == Screen::Experiment && !waiting_for_release) {
    mount(std::nullopt);
  }
  if (playback) {
    stage.stop();
    playback.reset();
  }
  screen = destination;
  reset();
  run.reset();
  accumulator = 0.0;
  pending = {};
  waiting_for_release = true;
}

void Session::choose(std::size_t option) {
  switch (screen) {
  case Screen::Main:
    if (option == 1) {
      enter(Screen::SurvivalSetup);
    } else if (option == 2) {
      enter(Screen::Developer);
    } else if (option == 3) {
      enter(Screen::Ascent);
    }
    return;
  case Screen::SurvivalSetup:
    if (option == 1) {
      start_run(survival::single_player(survival::Actor::Chef));
    } else if (option == 2) {
      start_run(survival::single_player(survival::Actor::Appraiser));
    } else if (option == 3) {
      start_run(survival::two_players());
    }
    return;
  case Screen::Developer:
    if (option >= 1 && option <= catalog.size()) {
      active =
          *std::next(catalog.begin(), static_cast<std::ptrdiff_t>(option - 1));
      enter(Screen::Experiment);
    }
    return;
  case Screen::Survival:
  case Screen::Ascent:
  case Screen::Experiment:
    return;
  }
  std::abort();
}

void Session::start_run(survival::ControlMap control) {
  enter(Screen::Survival);
  run.emplace(start(), control);
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

// The intro hands the run its first frame; the ending closes the run.
void Session::finish_cinematic(hud::Cinematic cinematic) {
  playback.reset();
  if (cinematic == hud::Cinematic::Ending) {
    enter(Screen::Main);
  }
}

void Session::advance(double delta, Command command) {
  if (command.menu) {
    switch (screen) {
    case Screen::Main:
      break;
    case Screen::Developer:
    case Screen::SurvivalSetup:
    case Screen::Survival:
    case Screen::Ascent:
      enter(Screen::Main);
      return;
    case Screen::Experiment:
      enter(Screen::Developer);
      return;
    }
  }
  // A key held across a transition cannot become the next run's first action,
  // so an experiment is mounted only once every control is released.
  if (waiting_for_release) {
    waiting_for_release = command.controls_held;
    if (!waiting_for_release && screen == Screen::Experiment) {
      mount(godot::String::utf8(active.scene));
    }
    return;
  }
  if (command.option &&
      (screen == Screen::Main || screen == Screen::Developer ||
       screen == Screen::SurvivalSetup)) {
    choose(*command.option);
    return;
  }
  if (playback) {
    advance_cinematic(*playback, delta, command);
    return;
  }
  if (run) {
    run->advance(delta, command.survival);
    if (run->escaped()) {
      play(hud::Cinematic::Ending);
    }
  }
  if (screen == Screen::Ascent) {
    pending.horizontal = command.ascent.horizontal;
    pending.jump_held = command.ascent.jump_held;
    pending.jump_pressed = pending.jump_pressed || command.ascent.jump_pressed;
    pending.switch_character =
        pending.switch_character || command.ascent.switch_character;
    pending.pause_pressed =
        pending.pause_pressed || command.ascent.pause_pressed;
    pending.retry_pressed =
        pending.retry_pressed || command.ascent.retry_pressed;
    tick_ascent(delta);
  }
}

void Session::tick_ascent(double delta) {
  if (std::isfinite(delta) && delta > 0.0) {
    accumulator = std::min(accumulator + delta, 0.1);
  }
  while (accumulator >= TICK) {
    ascent.get().step(pending);
    pending.jump_pressed = false;
    pending.switch_character = false;
    pending.pause_pressed = false;
    pending.retry_pressed = false;
    accumulator -= TICK;
  }
  camera.get().follow(ascent.get().snapshot(),
                      static_cast<float>(std::clamp(delta, 0.0, 0.1)));
}

void Session::paint(godot::Node2D &canvas,
                    const rendering::AscentDrawing &drawing) const {
  switch (screen) {
  case Screen::Main:
    hud::paint_main_menu(canvas, drawing);
    return;
  case Screen::Developer:
    hud::paint_developer_menu(canvas, drawing, catalog);
    return;
  case Screen::SurvivalSetup:
    hud::paint_survival_setup(canvas, drawing);
    return;
  case Screen::Survival:
    if (playback && run) {
      hud::paint_cinematic(canvas, drawing, playback->cinematic,
                           playback->card_left.has_value(), run->frame());
    } else if (run) {
      hud::paint_survival(canvas, drawing, run->frame());
    }
    return;
  case Screen::Ascent:
    drawing.paint(canvas, ascent.get().snapshot(), camera.get().y());
    hud::paint(canvas, drawing, ascent.get().snapshot());
    drawing.text(canvas, 1050.0F, 695.0F, "M: main menu", 18,
                 rendering::palette().white);
    return;
  case Screen::Experiment:
    return;
  }
  std::abort();
}

godot::String Session::screen_name() const {
  switch (screen) {
  case Screen::Main:
    return "main";
  case Screen::Developer:
    return "developer";
  case Screen::SurvivalSetup:
    return "survival_setup";
  case Screen::Survival:
    return "survival";
  case Screen::Ascent:
    return "ascent";
  case Screen::Experiment:
    return godot::String::utf8(active.id);
  }
  std::abort();
}

godot::Vector2 Session::actor_position() const {
  if (screen != Screen::Ascent) {
    return {};
  }
  const auto p = ascent.get().snapshot().leader.feet;
  return {p.x, p.y};
}

std::optional<survival::ControlMap> Session::survival_control() const {
  return run ? std::optional{run->control()} : std::nullopt;
}

godot::Dictionary Session::survival_status() const {
  if (!run) {
    return {};
  }
  auto status = run->status();
  if (playback) {
    status.set("cinematic", cinematic_name(playback->cinematic));
    status.set("cinematic_source", playback->card_left ? "fallback" : "scene");
  }
  return status;
}

} // namespace neocity::session
