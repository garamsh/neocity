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

const survival::ActorSnapshot &state_of(const survival::Snapshot &snapshot,
                                        Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return snapshot.chef;
  case Actor::Appraiser:
    return snapshot.appraiser;
  }
  std::abort();
}

} // namespace

Room::Room(std::uint32_t seed, std::uint32_t start_day)
    : data(survival::temporary_content(seed)), shared(data.run),
      map(data.objects), explorers(shared, map, data.companion.exploration),
      kitchen(shared, data.companion.workshop),
      escape_check(shared, survival::TEMPORARY_ESCAPE),
      controllers(data.companion) {
  while (shared.snapshot().day < start_day &&
         shared.snapshot().phase != survival::Phase::Escaped) {
    step({.inputs = {}, .control = AI_ONLY});
  }
}

void Room::step(const StepInput &input) {
  if (shared.snapshot().phase == survival::Phase::Escaped) {
    return;
  }
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
    // The roll follows the finishing command at once, as in `SurvivalRun`.
    if (command.action &&
        escape_check.check() == survival::EscapeResult::Escaped) {
      return;
    }
  }
  if (explorers.advance(STEP, movement, shared.snapshot().acid) !=
      survival::Result::Applied) {
    std::abort(); // unreachable: a 100ms step cannot overflow world time
  }
  const auto after = explorers.snapshot();
  for (const auto actor : ACTORS) {
    if (state_of(shared.snapshot(), actor).location ==
            survival::Location::City &&
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

survival::EscapeSnapshot Room::escape() const {
  return escape_check.snapshot();
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

std::size_t carried(const survival::GameView &view, Actor actor) {
  const auto &state = state_of(view.progression, actor);
  return state.inventory.size() + state.tools.size();
}

bool actor_progressed(const survival::GameView &before,
                      const survival::GameView &after, Actor actor) {
  const auto &then = state_of(before.progression, actor);
  const auto &now = state_of(after.progression, actor);
  return position_of(before.exploration, actor) !=
             position_of(after.exploration, actor) ||
         then.inventory != now.inventory || then.tools != now.tools;
}

void StepLog::record(std::uint64_t tick, const StepInput &input) {
  unbroken = unbroken && tick == entries.size() + 1;
  entries.push_back(input);
}

std::unique_ptr<Room> replay(const StepLog &log, std::uint32_t seed) {
  if (!log.complete()) {
    return nullptr;
  }
  auto room = std::make_unique<Room>(seed, 1);
  for (const auto &step : log.steps()) {
    room->step(step);
  }
  return room;
}

} // namespace neocity::lab
