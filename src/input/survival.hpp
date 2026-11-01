#pragma once

#include <survival/companion.hpp>

#include <array>

namespace neocity::input {

/// Samples the Godot input actions of each human actor, chef first; an AI
/// actor's entry stays empty.
std::array<survival::PlayerInput, 2>
sample_survival_input(survival::ControlMap control);

} // namespace neocity::input
