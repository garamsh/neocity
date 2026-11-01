#pragma once

#include "../session/session.hpp"

#include <optional>

namespace neocity::input {

/// Samples menu navigation and both games' controls from the Godot input
/// device; a survival run's keys need its `control`.
session::Command
sample_session_input(std::optional<survival::ControlMap> control);

} // namespace neocity::input
