#include "session.hpp"

#include "../hud/hud.hpp"
#include "../hud/menus.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <utility>

namespace neocity::session {
namespace {

constexpr double TICK = 1.0 / 120.0;

} // namespace

Session::Session(gameplay::AscentEngine &ascent_engine,
                 camera::AscentCamera &ascent_camera,
                 std::function<void()> reset_ascent,
                 std::span<const experiment::Entry> experiments,
                 Mount mount_scene)
    : ascent(ascent_engine), camera(ascent_camera),
      reset(std::move(reset_ascent)), catalog(experiments),
      mount(std::move(mount_scene)) {}

void Session::enter(Screen destination) {
  if (mounted) {
    mount(std::nullopt);
    mounted = false;
  }
  screen = destination;
  reset();
  accumulator = 0.0;
  pending = {};
  waiting_for_release = true;
}

void Session::choose(std::size_t option) {
  switch (screen) {
  case Screen::Main:
    if (option == 1) {
      enter(Screen::Ascent);
    } else if (option == 2) {
      enter(Screen::Developer);
    }
    return;
  case Screen::Developer:
    if (option >= 1 && option <= catalog.size()) {
      active =
          *std::next(catalog.begin(), static_cast<std::ptrdiff_t>(option - 1));
      enter(Screen::Experiment);
    }
    return;
  case Screen::Ascent:
  case Screen::Experiment:
    return;
  }
  std::abort();
}

void Session::advance(double delta, Command command) {
  if (command.menu) {
    switch (screen) {
    case Screen::Main:
      break;
    case Screen::Developer:
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
      mounted = true;
    }
    return;
  }
  if (command.option) {
    choose(*command.option);
    return;
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

} // namespace neocity::session
