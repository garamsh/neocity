#include "menu.hpp"

#include "survival.hpp"

#include <godot_cpp/classes/canvas_item.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/scene_tree.hpp>

#include <array>
#include <cstddef>

namespace neocity::input {
namespace {

/// One action per menu number, bound to the digit keys in project.godot.
constexpr std::array OPTIONS{"option_1", "option_2", "option_3", "option_4"};

/// Where the mouse is on the current scene's canvas, which the screen draws
/// on; none with no scene that draws.
std::optional<godot::Vector2> pointer() {
  const auto *tree = godot::Object::cast_to<godot::SceneTree>(
      godot::Engine::get_singleton()->get_main_loop());
  const auto *scene =
      tree != nullptr
          ? godot::Object::cast_to<godot::CanvasItem>(tree->get_current_scene())
          : nullptr;
  if (scene == nullptr) {
    return std::nullopt;
  }
  return scene->get_global_mouse_position();
}

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
  } else if (input->is_action_just_pressed("menu_left")) {
    command.step = session::MenuStep::Left;
  } else if (input->is_action_just_pressed("menu_right")) {
    command.step = session::MenuStep::Right;
  }
  command.menu = input->is_action_just_pressed("menu");
  command.pause = input->is_action_just_pressed("pause");
  command.confirm = input->is_action_just_pressed("confirm");
  command.pointer = pointer();
  command.click = input->is_action_just_pressed("click");
  for (const auto *action : {"menu_up", "menu_down", "menu_left", "menu_right",
                             "pause", "menu", "confirm", "click"}) {
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
