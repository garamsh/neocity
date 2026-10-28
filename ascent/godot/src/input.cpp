#include "input.hpp"

#include <godot_cpp/classes/input.hpp>

namespace neocity::ascent {

PlayerCommand sample_input() {
  const godot::Input *input = godot::Input::get_singleton();
  return PlayerCommand{
      .horizontal = input->get_axis("move_left", "move_right"),
      .jump_pressed = input->is_action_just_pressed("jump"),
      .jump_held = input->is_action_pressed("jump"),
      .switch_character = input->is_action_just_pressed("switch_character"),
      .pause_pressed = input->is_action_just_pressed("pause"),
      .retry_pressed = input->is_action_just_pressed("retry"),
  };
}

} // namespace neocity::ascent
