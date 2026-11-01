#pragma once

#include <survival/companion.hpp>

#include <array>

namespace neocity::input {

/// Samples the Godot input actions of each human actor, chef first; an AI
/// actor's entry stays empty.
std::array<survival::PlayerInput, 2>
sample_survival_input(survival::ControlMap control);

/// The up or down press a human chef made this frame, whatever else is held;
/// `Idle` otherwise. Dish choice reads presses, which movement's held direction
/// cannot show.
survival::Direction sample_dish_step(survival::ControlMap control);

/// Reports whether any key of either survival key set is held.
bool survival_controls_held();

} // namespace neocity::input
