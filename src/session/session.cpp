#include "session.hpp"

#include "../hud/experiments.hpp"
#include "../hud/hud.hpp"
#include "../rendering/pan.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>

namespace neocity::session {
namespace {

constexpr double TICK = 1.0 / 120.0;

} // namespace

Session::Session(pan_dodge::Engine &dodge_engine, pan_tilt::Engine &tilt_engine,
                 gameplay::AscentEngine &ascent_engine,
                 camera::AscentCamera &ascent_camera,
                 std::function<void()> reset_legacy)
    : dodge(dodge_engine), tilt(tilt_engine), ascent(ascent_engine),
      camera(ascent_camera), reset_ascent(std::move(reset_legacy)) {}

void Session::enter(Screen destination) {
  screen = destination;
  dodge.get().reset();
  tilt.get().reset();
  reset_ascent();
  accumulator = 0.0;
  pending = {};
  waiting_for_release = true;
}

void Session::advance(double delta, Command command) {
  if (command.menu && screen != Screen::Menu) {
    enter(Screen::Menu);
    return;
  }
  // A key held across a transition cannot become the next run's first action.
  if (waiting_for_release) {
    waiting_for_release = command.controls_held;
    return;
  }
  if (screen == Screen::Menu) {
    if (command.selection) {
      enter(*command.selection);
    }
    return;
  }
  if (command.retry && screen != Screen::Ascent) {
    enter(screen);
    return;
  }
  pending.dodge = command.dodge;
  pending.tilt = command.tilt;
  pending.ascent.horizontal = command.ascent.horizontal;
  pending.ascent.jump_held = command.ascent.jump_held;
  pending.ascent.jump_pressed =
      pending.ascent.jump_pressed || command.ascent.jump_pressed;
  pending.ascent.switch_character =
      pending.ascent.switch_character || command.ascent.switch_character;
  pending.ascent.pause_pressed =
      pending.ascent.pause_pressed || command.ascent.pause_pressed;
  pending.ascent.retry_pressed =
      pending.ascent.retry_pressed || command.ascent.retry_pressed;
  if (std::isfinite(delta) && delta > 0.0) {
    accumulator = std::min(accumulator + delta, 0.1);
  }
  while (accumulator >= TICK) {
    tick();
    pending.ascent.jump_pressed = false;
    pending.ascent.switch_character = false;
    pending.ascent.pause_pressed = false;
    pending.ascent.retry_pressed = false;
    accumulator -= TICK;
  }
  if (screen == Screen::Ascent) {
    camera.get().follow(ascent.get().snapshot(),
                        static_cast<float>(std::clamp(delta, 0.0, 0.1)));
  }
}

void Session::tick() {
  switch (screen) {
  case Screen::Menu:
    return;
  case Screen::Dodge:
    dodge.get().step(pending.dodge);
    return;
  case Screen::Tilt:
    tilt.get().step(pending.tilt);
    return;
  case Screen::Ascent:
    ascent.get().step(pending.ascent);
    return;
  }
  std::abort();
}

void Session::paint(godot::Node2D &canvas,
                    const rendering::AscentDrawing &drawing) const {
  switch (screen) {
  case Screen::Menu:
    hud::paint_menu(canvas, drawing);
    return;
  case Screen::Dodge:
    rendering::paint_pan(canvas, dodge.get().snapshot());
    hud::paint_experiment(canvas, drawing, false, outcome());
    return;
  case Screen::Tilt:
    rendering::paint_pan(canvas, tilt.get().snapshot());
    hud::paint_experiment(canvas, drawing, true, outcome());
    return;
  case Screen::Ascent:
    drawing.paint(canvas, ascent.get().snapshot(), camera.get().y());
    hud::paint(canvas, drawing, ascent.get().snapshot());
    drawing.text(canvas, 1030.0F, 695.0F, "M: experiment menu", 18,
                 rendering::palette().white);
    return;
  }
  std::abort();
}

godot::String Session::screen_name() const {
  switch (screen) {
  case Screen::Menu:
    return "menu";
  case Screen::Dodge:
    return "dodge";
  case Screen::Tilt:
    return "tilt";
  case Screen::Ascent:
    return "ascent";
  }
  std::abort();
}

godot::String Session::outcome() const {
  switch (screen) {
  case Screen::Menu:
    return "menu";
  case Screen::Ascent:
    return "reference";
  case Screen::Dodge:
    switch (dodge.get().snapshot().outcome) {
    case pan_dodge::Outcome::Playing:
      return "playing";
    case pan_dodge::Outcome::Escaped:
      return "escaped";
    case pan_dodge::Outcome::Hit:
      return "hit";
    }
    std::abort();
  case Screen::Tilt:
    switch (tilt.get().snapshot().outcome) {
    case pan_tilt::Outcome::Playing:
      return "playing";
    case pan_tilt::Outcome::Escaped:
      return "escaped";
    case pan_tilt::Outcome::Hit:
      return "hit";
    }
    std::abort();
  }
  std::abort();
}

godot::Vector2 Session::actor_position() const {
  switch (screen) {
  case Screen::Menu:
    return {};
  case Screen::Dodge: {
    const auto p = dodge.get().snapshot().player.center;
    return {p.x, p.y};
  }
  case Screen::Tilt: {
    const auto p = tilt.get().snapshot().player.center;
    return {p.x, p.y};
  }
  case Screen::Ascent: {
    const auto p = ascent.get().snapshot().leader.feet;
    return {p.x, p.y};
  }
  }
  std::abort();
}

godot::Vector2 Session::actor_velocity() const {
  const auto v = tilt.get().snapshot().player.velocity;
  return screen == Screen::Tilt ? godot::Vector2(v.x, v.y) : godot::Vector2{};
}

godot::Vector2 Session::hazard_position() const {
  const auto p = tilt.get().snapshot().hazards.front().center;
  return screen == Screen::Tilt ? godot::Vector2(p.x, p.y) : godot::Vector2{};
}

godot::Vector2 Session::pan_tilt() const {
  const auto value = tilt.get().snapshot().tilt;
  return screen == Screen::Tilt ? godot::Vector2(value.x, value.y)
                                : godot::Vector2{};
}

} // namespace neocity::session
