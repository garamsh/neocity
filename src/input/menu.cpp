#include "menu.hpp"

#include "input.hpp"

#include <godot_cpp/classes/input.hpp>

#include "../experiment/catalog.hpp"

#include <array>
#include <cstddef>

namespace neocity::input {
namespace {

/// One action per menu number, bound to the digit keys in project.godot.
constexpr std::array OPTIONS{"option_1", "option_2", "option_3",
                             "option_4", "option_5", "option_6",
                             "option_7", "option_8", "option_9"};
static_assert(experiment::CATALOG.size() <= OPTIONS.size(),
              "the developer page selects at most nine experiments");

} // namespace

session::Command sample_session_input() {
  const godot::Input *input = godot::Input::get_singleton();
  session::Command command;
  for (std::size_t i = 0; i < OPTIONS.size(); ++i) {
    command.controls_held =
        command.controls_held || input->is_action_pressed(OPTIONS.at(i));
    if (!command.option && input->is_action_just_pressed(OPTIONS.at(i))) {
      command.option = i + 1;
    }
  }
  command.menu = input->is_action_just_pressed("menu");
  command.ascent = sample_input();
  for (const auto *action :
       {"move_left", "move_right", "move_up", "move_down", "jump",
        "switch_character", "pause", "retry", "menu"}) {
    command.controls_held =
        command.controls_held || input->is_action_pressed(action);
  }
  return command;
}

} // namespace neocity::input
