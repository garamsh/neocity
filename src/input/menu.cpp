#include "menu.hpp"

#include "input.hpp"

#include <godot_cpp/classes/input.hpp>

#include <array>
#include <cstddef>

namespace neocity::input {

session::Command sample_session_input() {
  const godot::Input *input = godot::Input::get_singleton();
  session::Command command;
  constexpr std::array options{"option_1", "option_2"};
  for (std::size_t i = 0; i < options.size(); ++i) {
    if (!command.option && input->is_action_just_pressed(options.at(i))) {
      command.option = i + 1;
    }
  }
  command.menu = input->is_action_just_pressed("menu");
  command.ascent = sample_input();
  for (const auto *action :
       {"move_left", "move_right", "move_up", "move_down", "jump",
        "switch_character", "pause", "retry", "menu", "option_1", "option_2"}) {
    command.controls_held =
        command.controls_held || input->is_action_pressed(action);
  }
  return command;
}

} // namespace neocity::input
