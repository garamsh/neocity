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
/// Temporary: how long a human's interact key waits after that actor arrives
/// in the city or at base, so a press aimed at the old place does not act here.
constexpr std::chrono::milliseconds INTERACT_HOLD = 300ms;
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

const survival::Body &body_of(const survival::ExplorationSnapshot &snapshot,
                              Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return snapshot.chef.body;
  case Actor::Appraiser:
    return snapshot.appraiser.body;
  }
  std::abort();
}

GridPosition position_of(const survival::ExplorationSnapshot &snapshot,
                         Actor actor) {
  return survival::cell_of(body_of(snapshot, actor).position);
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

/// How long the collection under way still holds `actor`; zero with none.
std::chrono::microseconds
collecting_of(const survival::ExplorationSnapshot &snapshot, Actor actor) {
  const auto held = [&] {
    switch (actor) {
    case Actor::Chef:
      return snapshot.chef.collecting;
    case Actor::Appraiser:
      return snapshot.appraiser.collecting;
    }
    std::abort();
  }();
  return held ? held->remaining : std::chrono::microseconds{};
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
  if (input.tap != survival::Direction::Idle) {
    pending.tap = input.tap;
  }
  pending.travel = pending.travel || input.travel;
  pending.interact = pending.interact || input.interact;
  pending.replace_tool = pending.replace_tool || input.replace_tool;
  pending.feed = pending.feed || input.feed;
  pending.end_day = pending.end_day || input.end_day;
  pending.switch_actor = pending.switch_actor || input.switch_actor;
  pending.leave = pending.leave || input.leave;
  pending.movement.jump = pending.movement.jump || input.movement.jump;
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
  case survival::Phase::Lost:
    return "lost";
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

const char *stance_name(survival::Stance stance) {
  switch (stance) {
  case survival::Stance::Standing:
    return "standing";
  case survival::Stance::Airborne:
    return "airborne";
  case survival::Stance::Climbing:
    return "climbing";
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
  const auto &body = body_of(view.exploration, actor);
  const auto keys = survival::key_set(control, actor);
  godot::Dictionary status;
  status.set("location", location_name(state.location));
  status.set("x", position.x);
  status.set("y", position.y);
  status.set("sub_x", body.position.x);
  status.set("sub_y", body.position.y);
  status.set("stance", stance_name(body.stance));
  status.set("health", state.health);
  status.set("stamina", stamina_of(view.exploration, actor));
  status.set("collecting_ms",
             std::chrono::duration_cast<std::chrono::milliseconds>(
                 collecting_of(view.exploration, actor))
                 .count());
  status.set("ingredients", static_cast<std::int64_t>(state.inventory.size()));
  status.set("tools", static_cast<std::int64_t>(state.tools.size()));
  status.set("exhaustions", state.exhaustions);
  status.set("key_set", keys ? static_cast<std::int64_t>(*keys) : -1);
  return status;
}

} // namespace

SurvivalRun::SurvivalRun(SurvivalFeatures rules, survival::ControlMap players,
                         std::uint64_t first_event)
    : features(rules), controllers(players),
      started_together(
          survival::key_set(players, Actor::Chef).has_value() &&
          survival::key_set(players, Actor::Appraiser).has_value()),
      first_logged(first_event) {}

std::vector<hud::Event>
SurvivalRun::advance(double delta, const std::array<PlayerInput, 2> &input,
                     survival::Direction dish_press) {
  fresh.clear();
  if (std::isfinite(delta) && delta > 0.0) {
    accumulated += delta;
  }
  for (std::size_t index = 0; index < input.size(); ++index) {
    const auto &movement = input.at(index).movement;
    // A press shorter than a step still walks one movement unit; a key held
    // across a step stops walking when released.
    if (movement.direction != survival::Direction::Idle &&
        movement.direction != held.at(index).direction) {
      pending.at(index).movement.direction = movement.direction;
      pending.at(index).movement.running = movement.running;
    }
    retain_edges(pending.at(index), input.at(index));
    held.at(index) = movement;
  }
  if (dish_press != survival::Direction::Idle) {
    dish_step = dish_press;
  }
  while (accumulated >= STEP_SECONDS && !over()) {
    accumulated -= STEP_SECONDS;
    for (std::size_t index = 0; index < held.size(); ++index) {
      auto &next = pending.at(index);
      const bool jump = next.movement.jump;
      if (next.tap != survival::Direction::Idle) {
        next.movement = {.direction = next.tap,
                         .running = held.at(index).running};
      } else if (held.at(index).direction != survival::Direction::Idle) {
        next.movement = held.at(index);
      }
      next.movement.jump = jump;
    }
    step();
    pending = {};
    dish_step = survival::Direction::Idle;
  }
  return fresh;
}

void SurvivalRun::drop_input() {
  pending = {};
  dish_step = survival::Direction::Idle;
}

bool SurvivalRun::escaped() const {
  return features.simulation.get().snapshot().phase == survival::Phase::Escaped;
}

bool SurvivalRun::lost() const {
  return features.simulation.get().snapshot().phase == survival::Phase::Lost;
}

bool SurvivalRun::over() const {
  return survival::run_over(features.simulation.get().snapshot().phase);
}

survival::ControlMap SurvivalRun::control() const { return controllers; }

float SurvivalRun::acid_fill() const {
  const auto full = features.content.get().run.full_acid.value;
  const auto acid = features.simulation.get().snapshot().acid.value;
  return full > 0
             ? std::clamp(static_cast<float>(acid) / static_cast<float>(full),
                          0.0F, 1.0F)
             : 0.0F;
}

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
  const auto moved = [&simulation, &progression](Actor actor) {
    return state_of(simulation.snapshot(), actor).location !=
           state_of(progression, actor).location;
  };
  for (std::size_t index = 0; index < ACTORS.size(); ++index) {
    const auto actor = ACTORS.at(index);
    if (moved(actor)) {
      interact_hold.at(index) = INTERACT_HOLD;
    }
    if (interact_hold.at(index) > 0ms) {
      pending.at(index).interact = false;
    }
    const auto command = command_for(actor, pending.at(index), view());
    movement.at(index) = command.movement;
    if (command.action) {
      walked_home.at(index) =
          std::holds_alternative<survival::ReturnToBase>(*command.action);
      run_action(actor, *command.action, progression.day);
      // The roll follows the finishing command at once, before the partner's
      // command can set out and take the party out of the workshop.
      if (features.escape.get().check() == survival::EscapeResult::Stayed) {
        record(hud::EscapeMissEvent{.day = progression.day});
      }
      if (over()) {
        return;
      }
    }
  }
  if (features.exploration.get().advance(STEP, movement,
                                         simulation.snapshot().acid) !=
      survival::Result::Applied) {
    // unreachable: no step runs once the run is over, and 100ms cannot overflow
    // time
    std::abort();
  }
  hurt_in_hazards(positions);
  report_returns(progression, walked_home);
  report_world_changes(city);
  if (simulation.snapshot().day != progression.day) {
    last_day_map_changes = map_changes;
    map_changes = 0;
  }
  for (std::size_t index = 0; index < ACTORS.size(); ++index) {
    interact_hold.at(index) = moved(ACTORS.at(index))
                                  ? INTERACT_HOLD
                                  : std::max(interact_hold.at(index) - STEP,
                                             std::chrono::milliseconds{0});
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
  const auto &sources = features.content.get().companion.exploration.sources;
  for (std::size_t index = 0; index < now.objects.size(); ++index) {
    const auto &object = now.objects.at(index);
    const auto then = before.objects.at(index).state;
    // A repair reports itself as an action, and a bag coming and going shows
    // on the map alone, so the tide's daily bags leave the log to the rest.
    const auto bag = std::ranges::any_of(
        sources, [&object](const survival::SourceStock &source) {
          return source.id == object.id &&
                 source.kind == survival::SourceKind::Bag;
        });
    if (object.state != then && then != survival::ObjectState::Destroyed &&
        !bag) {
      record(hud::WorldEvent{.object = object.id, .state = object.state});
      ++map_changes;
    }
  }
}

void SurvivalRun::record(const hud::Event &event) {
  fresh.push_back(event);
  events.push_back(event);
  while (events.size() > EVENT_LIMIT) {
    events.pop_front();
    ++first_logged;
  }
}

hud::SurvivalFrame SurvivalRun::frame() const {
  hud::SurvivalFrame result{.content = features.content,
                            .view = view(),
                            .control = controllers,
                            .offers = {},
                            .started_together = started_together,
                            .last_day_map_changes = last_day_map_changes,
                            .events = {events.begin(), events.end()},
                            .first_event = first_logged};
  const auto chef = features.companion.get().command(Actor::Chef, controllers,
                                                     {}, result.view);
  result.ai_chef_working =
      !survival::key_set(controllers, Actor::Chef) && chef.action &&
      (std::holds_alternative<survival::Cook>(*chef.action) ||
       std::holds_alternative<survival::ReplaceTool>(*chef.action));
  for (std::size_t index = 0; index < ACTORS.size(); ++index) {
    const auto actor = ACTORS.at(index);
    if (survival::key_set(controllers, actor) && !over()) {
      result.offers.at(index) = {
          .interact =
              command_for(actor, {.interact = true}, result.view).action,
          .feed = command_for(actor, {.feed = true}, result.view).action,
          .errand = state_of(result.view.progression, actor).location ==
                            survival::Location::City
                        ? features.companion.get().errand(actor, result.view)
                        : std::nullopt};
    }
  }
  return result;
}

std::uint64_t SurvivalRun::next_event() const {
  return first_logged + events.size();
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
  status.set("full_acid", features.content.get().run.full_acid.value);
  status.set("chef", actor_status(view, controllers, Actor::Chef));
  status.set("appraiser", actor_status(view, controllers, Actor::Appraiser));
  status.set("magnifier", tool_state_name(view.workshop.magnifier));
  status.set("spatula", tool_state_name(view.workshop.spatula));
  status.set("identified",
             static_cast<std::int64_t>(view.workshop.identified.size()));
  status.set("foods", static_cast<std::int64_t>(view.workshop.foods.size()));
  godot::Array stored;
  for (const auto &dish : view.workshop.foods) {
    stored.push_back(from(survival::food_name(dish.food)));
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
  godot::Dictionary places;
  for (const auto &object : view.world.objects) {
    places.set(from(survival::object_name(object.id)),
               rendering::place_name(view, object.id));
  }
  status.set("places", places);
  godot::Dictionary stock;
  for (const auto &source : view.exploration.sources) {
    stock.set(from(survival::object_name(source.id)), source.remaining);
  }
  status.set("stock", stock);
  status.set("next", hud::next_step(current));
  status.set("hand_off", hud::hand_off_notice(current));
  godot::Array keys;
  for (const auto &row : hud::key_rows(current)) {
    keys.push_back(row.action + godot::String(": ") + row.first +
                   godot::String(" / ") + row.second);
  }
  status.set("pause_keys", keys);
  status.set("goal", hud::escape_goal());
  godot::Array result;
  for (const auto &line : hud::day_result(current)) {
    result.push_back(line);
  }
  status.set("day_result", result);
  status.set("forecast", hud::offered_forecast(current));
  godot::Array eaten;
  for (const auto &line : hud::eaten_lines(view.workshop.eaten_ingredients)) {
    eaten.push_back(line);
  }
  status.set("eaten_lines", eaten);
  status.set("appraise_job", hud::appraise_job(current));
  godot::Array stranded;
  for (const auto source : hud::out_of_reach(current)) {
    stranded.push_back(from(survival::object_name(source)));
  }
  status.set("out_of_reach", stranded);
  godot::Array fitted;
  for (const auto &line : hud::changing_text(current)) {
    godot::Dictionary entry;
    entry.set("text", line.text);
    entry.set("size", line.size);
    entry.set("width", line.width);
    fitted.push_back(entry);
  }
  status.set("changing_text", fitted);
  godot::Array actions;
  for (const auto &label : hud::action_labels(current)) {
    actions.push_back(label);
  }
  status.set("actions", actions);
  godot::Array log;
  for (const auto &event : current.events) {
    log.push_back(hud::describe(view, event));
  }
  status.set("events", log);
  return status;
}

} // namespace neocity::session
