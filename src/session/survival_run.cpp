#include "survival_run.hpp"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <optional>
#include <ranges>
#include <string_view>
#include <variant>
#include <vector>

namespace neocity::session {
namespace {

using namespace std::chrono_literals;
using survival::Actor;
using survival::GameView;
using survival::GridPosition;
using survival::PlayerInput;

constexpr auto STEP = 100ms;
constexpr double STEP_SECONDS = 0.1;
constexpr std::size_t EVENT_LIMIT = 4;
constexpr std::array ACTORS{Actor::Chef, Actor::Appraiser};
constexpr survival::ControlMap AI_ONLY{.chef = survival::Controller::Ai,
                                       .appraiser = survival::Controller::Ai};

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

std::uint32_t stamina_of(const survival::ExplorationSnapshot &snapshot,
                         Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return snapshot.chef.stamina;
  case Actor::Appraiser:
    return snapshot.appraiser.stamina;
  }
  std::abort();
}

bool in_hazard(const survival::Content &content, GridPosition position) {
  return std::ranges::any_of(content.hazards,
                             [position](const survival::WorldArea &area) {
                               return survival::distance(position, area) == 0;
                             });
}

/// Feeding and ending the day unfed each finish the day.
bool finishes_day(const survival::Action &action) {
  return std::holds_alternative<survival::Feed>(action) ||
         std::holds_alternative<survival::EndDayUnfed>(action);
}

/// The dishes that can be cooked now, in the cook panel's order.
std::vector<survival::FoodId> ready_dishes(const survival::Content &content,
                                           const GameView &view) {
  std::vector<survival::FoodId> ready;
  for (const auto &recipe : content.companion.workshop.recipes) {
    if (hud::recipe_ready(view, recipe)) {
      ready.push_back(recipe.food);
    }
  }
  return ready;
}

/// Keeps every edge until a step consumes it.
void retain_edges(PlayerInput &pending, const PlayerInput &input) {
  pending.travel = pending.travel || input.travel;
  pending.interact = pending.interact || input.interact;
  pending.replace_tool = pending.replace_tool || input.replace_tool;
  pending.feed = pending.feed || input.feed;
  pending.end_day = pending.end_day || input.end_day;
  pending.switch_actor = pending.switch_actor || input.switch_actor;
  pending.leave = pending.leave || input.leave;
}

godot::String from(std::string_view text) {
  return godot::String::utf8(text.data(),
                             static_cast<std::int64_t>(text.size()));
}

const char *phase_name(survival::Phase phase) {
  switch (phase) {
  case survival::Phase::Workshop:
    return "workshop";
  case survival::Phase::Exploration:
    return "exploration";
  case survival::Phase::Escaped:
    return "escaped";
  }
  std::abort();
}

const char *state_name(survival::ObjectState state) {
  switch (state) {
  case survival::ObjectState::Dormant:
    return "dormant";
  case survival::ObjectState::Intact:
    return "intact";
  case survival::ObjectState::Destroyed:
    return "destroyed";
  }
  std::abort();
}

const char *tool_state_name(survival::ToolState state) {
  switch (state) {
  case survival::ToolState::Intact:
    return "intact";
  case survival::ToolState::Damaged:
    return "damaged";
  }
  std::abort();
}

const char *reaction_name(survival::MonsterReaction reaction) {
  switch (reaction) {
  case survival::MonsterReaction::Unfed:
    return "unfed";
  case survival::MonsterReaction::Disliked:
    return "disliked";
  case survival::MonsterReaction::Unsatisfied:
    return "unsatisfied";
  case survival::MonsterReaction::Satisfied:
    return "satisfied";
  }
  std::abort();
}

const char *location_name(survival::Location location) {
  switch (location) {
  case survival::Location::Base:
    return "base";
  case survival::Location::City:
    return "city";
  }
  std::abort();
}

godot::Dictionary actor_status(const GameView &view,
                               survival::ControlMap control, Actor actor) {
  const auto &state = state_of(view.progression, actor);
  const auto position = position_of(view.exploration, actor);
  const auto keys = survival::key_set(control, actor);
  godot::Dictionary status;
  status.set("location", location_name(state.location));
  status.set("x", position.x);
  status.set("y", position.y);
  status.set("health", state.health);
  status.set("stamina", stamina_of(view.exploration, actor));
  status.set("ingredients", static_cast<std::int64_t>(state.inventory.size()));
  status.set("tools", static_cast<std::int64_t>(state.tools.size()));
  status.set("exhaustions", state.exhaustions);
  status.set("key_set", keys ? static_cast<std::int64_t>(*keys) : -1);
  return status;
}

} // namespace

SurvivalRun::SurvivalRun(SurvivalFeatures rules, survival::ControlMap players)
    : features(rules), controllers(players) {}

void SurvivalRun::advance(double delta, const std::array<PlayerInput, 2> &input,
                          survival::Direction dish_press) {
  if (std::isfinite(delta) && delta > 0.0) {
    accumulated += delta;
  }
  for (std::size_t index = 0; index < input.size(); ++index) {
    const auto &movement = input.at(index).movement;
    retain_edges(pending.at(index), input.at(index));
    // A press shorter than a step still walks one cell; a key held across a
    // step stops walking when released.
    if (movement.direction != survival::Direction::Idle &&
        movement.direction != held.at(index).direction) {
      pending.at(index).movement = movement;
    }
    held.at(index) = movement;
  }
  if (dish_press != survival::Direction::Idle) {
    dish_step = dish_press;
  }
  while (accumulated >= STEP_SECONDS && !escaped()) {
    accumulated -= STEP_SECONDS;
    for (std::size_t index = 0; index < held.size(); ++index) {
      if (held.at(index).direction != survival::Direction::Idle) {
        pending.at(index).movement = held.at(index);
      }
    }
    step();
    pending = {};
    dish_step = survival::Direction::Idle;
  }
}

bool SurvivalRun::escaped() const {
  return features.simulation.get().snapshot().phase == survival::Phase::Escaped;
}

survival::ControlMap SurvivalRun::control() const { return controllers; }

GameView SurvivalRun::view() const {
  return {.progression = features.simulation.get().snapshot(),
          .world = features.world.get().snapshot(),
          .exploration = features.exploration.get().snapshot(),
          .workshop = features.workshop.get().snapshot()};
}

survival::ActorCommand SurvivalRun::command_for(Actor actor,
                                                const PlayerInput &input,
                                                const GameView &current) const {
  auto command =
      features.companion.get().command(actor, controllers, input, current);
  if (chosen_dish && survival::key_set(controllers, actor) && command.action &&
      std::holds_alternative<survival::Cook>(*command.action)) {
    const auto ready = ready_dishes(features.content.get(), current);
    if (std::ranges::find(ready, *chosen_dish) != ready.end()) {
      command.action = survival::Cook{*chosen_dish};
    }
  }
  return command;
}

void SurvivalRun::pick_dish(const GameView &current) {
  const auto offer = command_for(Actor::Chef, {.interact = true}, current);
  const auto *cook =
      offer.action ? std::get_if<survival::Cook>(&*offer.action) : nullptr;
  const auto ready = ready_dishes(features.content.get(), current);
  if (cook == nullptr || ready.empty()) {
    return;
  }
  const auto from = static_cast<std::size_t>(
      std::ranges::find(ready, cook->food) - ready.begin());
  const std::size_t forward =
      dish_step == survival::Direction::Down ? 1 : ready.size() - 1;
  chosen_dish = ready.at((from + forward) % ready.size());
}

void SurvivalRun::step() {
  const auto &simulation = features.simulation.get();
  const auto progression = simulation.snapshot();
  const auto city = features.world.get().snapshot();
  const auto positions = features.exploration.get().snapshot();
  std::array<survival::ExplorationInput, 2> movement;
  std::array<bool, 2> walked_home{};
  if (dish_step != survival::Direction::Idle &&
      progression.phase == survival::Phase::Workshop &&
      survival::key_set(controllers, Actor::Chef)) {
    pick_dish(view());
  }
  for (std::size_t index = 0; index < ACTORS.size(); ++index) {
    const auto actor = ACTORS.at(index);
    const auto command = command_for(actor, pending.at(index), view());
    movement.at(index) = command.movement;
    if (command.action) {
      walked_home.at(index) =
          std::holds_alternative<survival::ReturnToBase>(*command.action);
      run_action(actor, *command.action, progression.day);
      // The roll follows the finishing command at once, before the partner's
      // command can set out and take the party out of the workshop.
      if (features.escape.get().check() == survival::EscapeResult::Escaped) {
        return;
      }
    }
  }
  if (features.exploration.get().advance(STEP, movement,
                                         simulation.snapshot().acid) !=
      survival::Result::Applied) {
    // unreachable: no step runs after escape, and 100ms cannot overflow time
    std::abort();
  }
  hurt_in_hazards(positions);
  report_returns(progression, walked_home);
  report_world_changes(city);
  for (std::size_t index = 0; index < ACTORS.size(); ++index) {
    if (pending.at(index).switch_actor) {
      controllers = survival::switch_control(controllers);
    }
    // A party of AI alone has no key left to take an actor back.
    const auto after_leaving = survival::leave(controllers, ACTORS.at(index));
    if (pending.at(index).leave && after_leaving != AI_ONLY) {
      controllers = after_leaving;
    }
  }
}

void SurvivalRun::run_action(Actor actor, const survival::Action &action,
                             std::uint32_t day) {
  // Two humans pressing in one step finish one day, not two.
  if (finishes_day(action) && features.simulation.get().snapshot().day != day) {
    return;
  }
  const auto result = survival::apply(features.exploration.get(),
                                      features.workshop.get(), actor, action);
  if (features.simulation.get().snapshot().day != day) {
    chosen_dish.reset();
  }
  // AI's collecting would crowd damage and map changes out of the log.
  if (survival::key_set(controllers, actor) ||
      !std::holds_alternative<survival::Collect>(action)) {
    record(
        hud::ActionEvent{.actor = actor, .action = action, .result = result});
  }
}

void SurvivalRun::hurt_in_hazards(const survival::ExplorationSnapshot &before) {
  const auto &content = features.content.get();
  const auto snapshot = features.exploration.get().snapshot();
  for (const auto actor : ACTORS) {
    const auto state = state_of(features.simulation.get().snapshot(), actor);
    if (state.location == survival::Location::City &&
        in_hazard(content, position_of(snapshot, actor)) &&
        !in_hazard(content, position_of(before, actor))) {
      if (features.exploration.get().damage(actor) !=
          survival::Result::Applied) {
        std::abort(); // unreachable: the actor is in the city
      }
      record(hud::HazardEvent{
          .actor = actor,
          .exhausted =
              state_of(features.simulation.get().snapshot(), actor).location ==
              survival::Location::Base});
    }
  }
}

void SurvivalRun::report_returns(const survival::Snapshot &before,
                                 const std::array<bool, 2> &walked_home) {
  const auto now = features.simulation.get().snapshot();
  const bool time_up = before.remaining_exploration.count() > 0 &&
                       now.remaining_exploration.count() == 0;
  if (time_up) {
    record(hud::TimeUpEvent{});
    return;
  }
  for (std::size_t index = 0; index < ACTORS.size(); ++index) {
    const auto actor = ACTORS.at(index);
    const auto &then = state_of(before, actor);
    const auto &current = state_of(now, actor);
    if (then.location == survival::Location::City &&
        current.location == survival::Location::Base &&
        current.exhaustions == then.exhaustions && !walked_home.at(index)) {
      record(hud::FallEvent{actor});
    }
  }
}

void SurvivalRun::report_world_changes(const survival::WorldSnapshot &before) {
  const auto now = features.world.get().snapshot();
  for (std::size_t index = 0; index < now.objects.size(); ++index) {
    const auto &object = now.objects.at(index);
    const auto then = before.objects.at(index).state;
    // A repair reports itself as an action.
    if (object.state != then && then != survival::ObjectState::Destroyed) {
      record(hud::WorldEvent{.object = object.id, .state = object.state});
    }
  }
}

void SurvivalRun::record(const hud::Event &event) {
  events.push_back(event);
  while (events.size() > EVENT_LIMIT) {
    events.pop_front();
  }
}

hud::SurvivalFrame SurvivalRun::frame() const {
  hud::SurvivalFrame result{.content = features.content,
                            .view = view(),
                            .control = controllers,
                            .offers = {},
                            .events = {events.begin(), events.end()}};
  for (std::size_t index = 0; index < ACTORS.size(); ++index) {
    const auto actor = ACTORS.at(index);
    if (survival::key_set(controllers, actor) && !escaped()) {
      result.offers.at(index) = {
          .interact =
              command_for(actor, {.interact = true}, result.view).action,
          .feed = command_for(actor, {.feed = true}, result.view).action};
    }
  }
  return result;
}

godot::Dictionary SurvivalRun::status() const {
  const auto current = frame();
  const auto &view = current.view;
  godot::Dictionary status;
  status.set("day", view.progression.day);
  status.set("phase", phase_name(view.progression.phase));
  status.set("remaining_ms",
             std::chrono::duration_cast<std::chrono::milliseconds>(
                 view.progression.remaining_exploration)
                 .count());
  status.set("elapsed_ms",
             std::chrono::duration_cast<std::chrono::milliseconds>(
                 view.world.progression.elapsed)
                 .count());
  status.set("acid", view.progression.acid.value);
  status.set("chef", actor_status(view, controllers, Actor::Chef));
  status.set("appraiser", actor_status(view, controllers, Actor::Appraiser));
  status.set("magnifier", tool_state_name(view.workshop.magnifier));
  status.set("spatula", tool_state_name(view.workshop.spatula));
  status.set("identified",
             static_cast<std::int64_t>(view.workshop.identified.size()));
  status.set("foods", static_cast<std::int64_t>(view.workshop.foods.size()));
  godot::Array stored;
  for (const auto food : view.workshop.foods) {
    stored.push_back(from(survival::food_name(food)));
  }
  status.set("stored_foods", stored);
  const auto &chef_offer = current.offers.at(0).interact;
  const auto *cook =
      chef_offer ? std::get_if<survival::Cook>(&*chef_offer) : nullptr;
  status.set("dish_offered", cook != nullptr
                                 ? from(survival::food_name(cook->food))
                                 : godot::String());
  status.set("eaten",
             static_cast<std::int64_t>(view.workshop.eaten_foods.size()));
  status.set("rolls", view.workshop.rolls);
  status.set("escape_rolls", features.escape.get().snapshot().rolls);
  if (view.workshop.last_day) {
    status.set("reaction", reaction_name(view.workshop.last_day->reaction));
    status.set("score", view.workshop.last_day->score);
  }
  godot::Dictionary objects;
  for (const auto &object : view.world.objects) {
    objects.set(from(survival::object_name(object.id)),
                state_name(object.state));
  }
  status.set("objects", objects);
  godot::Dictionary stock;
  for (const auto &source : view.exploration.sources) {
    stock.set(from(survival::object_name(source.id)), source.remaining);
  }
  status.set("stock", stock);
  status.set("next", hud::next_step(current));
  godot::Array log;
  for (const auto &event : current.events) {
    log.push_back(hud::describe(event));
  }
  status.set("events", log);
  return status;
}

} // namespace neocity::session
