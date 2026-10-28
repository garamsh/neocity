#pragma once

#include "../session/session.hpp"

namespace neocity::input {

/// Samples menu and scenario controls from the same Godot input device.
session::Command sample_session_input();

} // namespace neocity::input
