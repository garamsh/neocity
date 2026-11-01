#pragma once

#include <survival/companion.hpp>

namespace neocity::lab {

/// Plays one actor from the received view and presses what a person would: the
/// choice of an AI-only party, sent as keys.
[[nodiscard]] survival::PlayerInput bot_input(const survival::Companion &brain,
                                              survival::Actor actor,
                                              const survival::GameView &view);

} // namespace neocity::lab
