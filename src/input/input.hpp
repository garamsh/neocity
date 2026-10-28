#pragma once

#include <gameplay/engine.hpp>

namespace neocity::input {

/// Translates the current Godot input actions into an engine command.
gameplay::PlayerCommand sample_input();

} // namespace neocity::input
