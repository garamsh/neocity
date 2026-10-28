#include "experiments.hpp"

#include "input.hpp"

#include <godot_cpp/classes/input.hpp>

namespace neocity::input {
namespace {

pan_dodge::Direction dodge_axis(float value) {
  if (value < 0.0F) {
    return pan_dodge::Direction::Negative;
  }
  return value > 0.0F ? pan_dodge::Direction::Positive
                      : pan_dodge::Direction::Neutral;
}

pan_tilt::Direction tilt_axis(float value) {
  if (value < 0.0F) {
    return pan_tilt::Direction::Negative;
  }
  return value > 0.0F ? pan_tilt::Direction::Positive
                      : pan_tilt::Direction::Neutral;
}

} // namespace

session::Command sample_session_input() {
  const godot::Input *input = godot::Input::get_singleton();
  session::Command command;
  if (input->is_action_just_pressed("select_dodge")) {
    command.selection = session::Screen::Dodge;
  } else if (input->is_action_just_pressed("select_tilt")) {
    command.selection = session::Screen::Tilt;
  } else if (input->is_action_just_pressed("select_ascent")) {
    command.selection = session::Screen::Ascent;
  }
  command.menu = input->is_action_just_pressed("menu");
  command.retry = input->is_action_just_pressed("retry");
  const float x = input->get_axis("move_left", "move_right");
  const float y = input->get_axis("pan_up", "pan_down");
  command.dodge = {.horizontal = dodge_axis(x), .vertical = dodge_axis(y)};
  command.tilt = {.horizontal = tilt_axis(x), .vertical = tilt_axis(y)};
  command.ascent = sample_input();
  for (const auto *action :
       {"move_left", "move_right", "pan_up", "pan_down", "jump",
        "switch_character", "pause", "retry", "menu", "select_dodge",
        "select_tilt", "select_ascent"}) {
    command.controls_held =
        command.controls_held || input->is_action_pressed(action);
  }
  return command;
}

} // namespace neocity::input
