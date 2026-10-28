#pragma once

#include "../session/session.hpp"

namespace neocity::input {

/// Samples menu navigation and main-game controls from the Godot input device.
session::Command sample_session_input();

} // namespace neocity::input
