#include "menu.hpp"

#include "survival.hpp"

#include <godot_cpp/classes/input.hpp>

#include <array>
#include <cstddef>

namespace neocity::input {
namespace {

/// One action per menu number, bound to the digit keys in project.godot.
constexpr std::array OPTIONS{"option_1", "option_2", "option_3"};

} // namespace

session::Command
sample_session_input(std::optional<survival::ControlMap> control) {
  const godot::Input *input = godot::Input::get_singleton();
  session::Command command;
  for (std::size_t i = 0; i < OPTIONS.size(); ++i) {
    command.controls_held =
        command.controls_held || input->is_action_pressed(OPTIONS.at(i));
    if (!command.option && input->is_action_just_pressed(OPTIONS.at(i))) {
      command.option = i + 1;
    }
  }
  if (input->is_action_just_pressed("menu_up")) {
    command.step = session::MenuStep::Up;
  } else if (input->is_action_just_pressed("menu_down")) {
    command.step = session::MenuStep::Down;
  }
  command.menu = input->is_action_just_pressed("menu");
  command.pause = input->is_action_just_pressed("pause");
  command.confirm = input->is_action_just_pressed("confirm");
  for (const auto *action :
       {"menu_up", "menu_down", "pause", "menu", "confirm"}) {
    command.controls_held =
        command.controls_held || input->is_action_pressed(action);
  }
  command.controls_held = command.controls_held || survival_controls_held();
  if (control) {
    command.survival = sample_survival_input(*control);
    command.dish_step = sample_dish_step(*control);
  }
  return command;
}

} // namespace neocity::input
