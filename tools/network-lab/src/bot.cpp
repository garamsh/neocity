#include "bot.hpp"

#include <type_traits>
#include <variant>

namespace neocity::lab {
namespace {

using namespace neocity::survival;

constexpr ControlMap AI_ONLY{.chef = Controller::Ai,
                             .appraiser = Controller::Ai};

/// One key per action kind; `Companion` picks the target for that key.
void press(PlayerInput &input, const Action &action) {
  std::visit(
      [&input](const auto &chosen) {
        using Chosen = std::decay_t<decltype(chosen)>;
        if constexpr (std::is_same_v<Chosen, Enter> ||
                      std::is_same_v<Chosen, ReturnToBase>) {
          input.travel = true;
        } else if constexpr (std::is_same_v<Chosen, ReplaceTool>) {
          input.replace_tool = true;
        } else if constexpr (std::is_same_v<Chosen, Feed>) {
          input.feed = true;
        } else if constexpr (std::is_same_v<Chosen, EndDayUnfed>) {
          input.end_day = true;
        } else {
          input.interact = true;
        }
      },
      action);
}

} // namespace

PlayerInput bot_input(const Companion &brain, Actor actor,
                      const GameView &view) {
  const auto command = brain.command(actor, AI_ONLY, {}, view);
  PlayerInput input{.movement = command.movement};
  if (command.action) {
    press(input, *command.action);
  }
  return input;
}

} // namespace neocity::lab
