#include "room.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <variant>

namespace neocity::lab {
namespace {

using namespace std::chrono_literals;
using survival::Actor;
using survival::GridPosition;

constexpr auto STEP = 100ms;
constexpr std::array ACTORS{Actor::Chef, Actor::Appraiser};
constexpr survival::ControlMap AI_ONLY{.chef = survival::Controller::Ai,
                                       .appraiser = survival::Controller::Ai};

bool finishes_day(const survival::Action &action) {
  return std::holds_alternative<survival::Feed>(action) ||
         std::holds_alternative<survival::EndDayUnfed>(action);
}

bool in_hazard(const survival::Content &content, GridPosition position) {
  return std::ranges::any_of(content.hazards,
                             [position](const survival::WorldArea &area) {
                               return survival::distance(position, area) == 0;
                             });
}

GridPosition position_of(const survival::ExplorationSnapshot &snapshot,
                         Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return snapshot.chef.position;
  case Actor::Appraiser:
    return snapshot.appraiser.position;
  }
  std::abort();
}

survival::Location location_of(const survival::Snapshot &snapshot,
                               Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return snapshot.chef.location;
  case Actor::Appraiser:
    return snapshot.appraiser.location;
  }
  std::abort();
}

} // namespace

Room::Room(std::uint32_t seed, std::uint32_t start_day)
    : data(survival::temporary_content(seed)), shared(data.run),
      map(data.objects), explorers(shared, map, data.companion.exploration),
      kitchen(shared, data.companion.workshop), controllers(data.companion) {
  while (shared.snapshot().day < start_day) {
    step({.inputs = {}, .control = AI_ONLY});
  }
}

void Room::step(const StepInput &input) {
  const auto before = explorers.snapshot();
  const auto day = shared.snapshot().day;
  std::array<survival::ExplorationInput, 2> movement;
  for (std::size_t index = 0; index < ACTORS.size(); ++index) {
    const auto actor = ACTORS.at(index);
    const auto command = controllers.command(actor, input.control,
                                             input.inputs.at(index), view());
    movement.at(index) = command.movement;
    if (command.action &&
        !(finishes_day(*command.action) && shared.snapshot().day != day)) {
      (void)survival::apply(explorers, kitchen, actor, *command.action);
    }
  }
  if (explorers.advance(STEP, movement, shared.snapshot().acid) !=
      survival::Result::Applied) {
    std::abort(); // unreachable: a 100ms step cannot overflow world time
  }
  const auto after = explorers.snapshot();
  for (const auto actor : ACTORS) {
    if (location_of(shared.snapshot(), actor) == survival::Location::City &&
        in_hazard(data, position_of(after, actor)) &&
        !in_hazard(data, position_of(before, actor))) {
      (void)explorers.damage(actor);
    }
  }
}

survival::GameView Room::view() const {
  return {.progression = shared.snapshot(),
          .world = map.snapshot(),
          .exploration = explorers.snapshot(),
          .workshop = kitchen.snapshot()};
}

const survival::CompanionConfig &Room::companion_config() const {
  return data.companion;
}

bool same_view(const survival::GameView &left,
               const survival::GameView &right) {
  return left.progression == right.progression && left.world == right.world &&
         left.exploration == right.exploration &&
         left.workshop == right.workshop;
}

} // namespace neocity::lab
