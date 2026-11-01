#include "survival.hpp"

#include <godot_cpp/classes/input.hpp>

#include <algorithm>
#include <cstddef>

namespace neocity::input {
namespace {

/// One local player's actions, bound in project.godot.
struct KeySet {
  const char *left;
  const char *right;
  const char *up;
  const char *down;
  const char *run;
  const char *travel;
  const char *interact;
  const char *replace_tool;
  const char *feed;
  const char *end_day;
  const char *switch_actor;
  const char *leave;
};

constexpr std::array KEY_SETS{KeySet{.left = "survival_1_left",
                                     .right = "survival_1_right",
                                     .up = "survival_1_up",
                                     .down = "survival_1_down",
                                     .run = "survival_1_run",
                                     .travel = "survival_1_travel",
                                     .interact = "survival_1_interact",
                                     .replace_tool = "survival_1_replace_tool",
                                     .feed = "survival_1_feed",
                                     .end_day = "survival_1_end_day",
                                     .switch_actor = "survival_1_switch",
                                     .leave = "survival_1_leave"},
                              KeySet{.left = "survival_2_left",
                                     .right = "survival_2_right",
                                     .up = "survival_2_up",
                                     .down = "survival_2_down",
                                     .run = "survival_2_run",
                                     .travel = "survival_2_travel",
                                     .interact = "survival_2_interact",
                                     .replace_tool = "survival_2_replace_tool",
                                     .feed = "survival_2_feed",
                                     .end_day = "survival_2_end_day",
                                     .switch_actor = "survival_2_switch",
                                     .leave = "survival_2_leave"}};

template <typename Test>
survival::Direction first_direction(const KeySet &keys, Test test) {
  if (test(keys.left)) {
    return survival::Direction::Left;
  }
  if (test(keys.right)) {
    return survival::Direction::Right;
  }
  if (test(keys.up)) {
    return survival::Direction::Up;
  }
  if (test(keys.down)) {
    return survival::Direction::Down;
  }
  return survival::Direction::Idle;
}

/// A key pressed and released inside one frame is never seen as held, so it
/// reads from the press edge and wins that frame over a key still held.
survival::Direction sampled_direction(const godot::Input &input,
                                      const KeySet &keys) {
  const auto tapped = first_direction(keys, [&input](const char *action) {
    return input.is_action_just_pressed(action) &&
           !input.is_action_pressed(action);
  });
  if (tapped != survival::Direction::Idle) {
    return tapped;
  }
  return first_direction(keys, [&input](const char *action) {
    return input.is_action_pressed(action);
  });
}

survival::PlayerInput sample_keys(const godot::Input &input,
                                  const KeySet &keys) {
  return {
      .movement = {.direction = sampled_direction(input, keys),
                   .running = input.is_action_pressed(keys.run)},
      .travel = input.is_action_just_pressed(keys.travel),
      .interact = input.is_action_just_pressed(keys.interact),
      .replace_tool = input.is_action_just_pressed(keys.replace_tool),
      .feed = input.is_action_just_pressed(keys.feed),
      .end_day = input.is_action_just_pressed(keys.end_day),
      .switch_actor = input.is_action_just_pressed(keys.switch_actor),
      .leave = input.is_action_just_pressed(keys.leave),
  };
}

} // namespace

std::array<survival::PlayerInput, 2>
sample_survival_input(survival::ControlMap control) {
  const godot::Input *input = godot::Input::get_singleton();
  std::array<survival::PlayerInput, 2> inputs;
  constexpr std::array actors{survival::Actor::Chef,
                              survival::Actor::Appraiser};
  for (std::size_t index = 0; index < actors.size(); ++index) {
    if (const auto keys = survival::key_set(control, actors.at(index))) {
      inputs.at(index) = sample_keys(*input, KEY_SETS.at(*keys));
    }
  }
  return inputs;
}

survival::Direction sample_dish_step(survival::ControlMap control) {
  const auto keys = survival::key_set(control, survival::Actor::Chef);
  if (!keys) {
    return survival::Direction::Idle;
  }
  const godot::Input *input = godot::Input::get_singleton();
  const auto &actions = KEY_SETS.at(*keys);
  if (input->is_action_just_pressed(actions.up)) {
    return survival::Direction::Up;
  }
  if (input->is_action_just_pressed(actions.down)) {
    return survival::Direction::Down;
  }
  return survival::Direction::Idle;
}

bool survival_controls_held() {
  const godot::Input *input = godot::Input::get_singleton();
  return std::ranges::any_of(KEY_SETS, [input](const KeySet &keys) {
    return std::ranges::any_of(
        std::array{keys.left, keys.right, keys.up, keys.down, keys.run,
                   keys.travel, keys.interact, keys.replace_tool, keys.feed,
                   keys.end_day, keys.switch_actor, keys.leave},
        [input](const char *action) {
          return input->is_action_pressed(action);
        });
  });
}

} // namespace neocity::input
