#include <survival/companion.hpp>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <deque>
#include <limits>
#include <set>
#include <utility>
#include <vector>

namespace neocity::survival {
namespace {

constexpr std::array STEPS{Direction::Left, Direction::Right, Direction::Up,
                           Direction::Down};

Actor partner_of(Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return Actor::Appraiser;
  case Actor::Appraiser:
    return Actor::Chef;
  }
  std::abort();
}

Controller controller_of(ControlMap control, Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return control.chef;
  case Actor::Appraiser:
    return control.appraiser;
  }
  std::abort();
}

const ActorSnapshot &state_of(const GameView &view, Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return view.progression.chef;
  case Actor::Appraiser:
    return view.progression.appraiser;
  }
  std::abort();
}

GridPosition position_of(const GameView &view, Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return view.exploration.chef.position;
  case Actor::Appraiser:
    return view.exploration.appraiser.position;
  }
  std::abort();
}

WorkshopTool role_tool(Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return WorkshopTool::Spatula;
  case Actor::Appraiser:
    return WorkshopTool::Magnifier;
  }
  std::abort();
}

ToolState tool_state(const WorkshopSnapshot &workshop, WorkshopTool tool) {
  switch (tool) {
  case WorkshopTool::Magnifier:
    return workshop.magnifier;
  case WorkshopTool::Spatula:
    return workshop.spatula;
  }
  std::abort();
}

ToolId replacement_for(const WorkshopConfig &workshop, WorkshopTool tool) {
  switch (tool) {
  case WorkshopTool::Magnifier:
    return workshop.magnifier;
  case WorkshopTool::Spatula:
    return workshop.spatula;
  }
  std::abort();
}

std::optional<GridPosition> neighbour(GridPosition position,
                                      Direction direction) {
  auto x = std::int64_t{position.x};
  auto y = std::int64_t{position.y};
  switch (direction) {
  case Direction::Idle:
    break;
  case Direction::Left:
    --x;
    break;
  case Direction::Right:
    ++x;
    break;
  case Direction::Up:
    --y;
    break;
  case Direction::Down:
    ++y;
    break;
  }
  if (x < std::numeric_limits<std::int32_t>::min() ||
      x > std::numeric_limits<std::int32_t>::max() ||
      y < std::numeric_limits<std::int32_t>::min() ||
      y > std::numeric_limits<std::int32_t>::max()) {
    return std::nullopt;
  }
  return GridPosition{.x = static_cast<std::int32_t>(x),
                      .y = static_cast<std::int32_t>(y)};
}

std::int64_t distance(GridPosition position, WorldArea area) {
  const auto right = std::int64_t{area.x} + area.width - 1;
  const auto bottom = std::int64_t{area.y} + area.height - 1;
  return std::max({std::int64_t{area.x} - position.x,
                   std::int64_t{position.x} - right, std::int64_t{0}}) +
         std::max({std::int64_t{area.y} - position.y,
                   std::int64_t{position.y} - bottom, std::int64_t{0}});
}

// Exploration keeps its cell rule private, so planning repeats it here.
bool walkable(const WorldSnapshot &world, GridPosition position) {
  bool supported = false;
  bool building = false;
  bool open_passage = false;
  bool broken_passage = false;
  for (const auto &object : world.objects) {
    if (object.state == ObjectState::Dormant ||
        distance(position, object.area) != 0) {
      continue;
    }
    switch (object.kind) {
    case ObjectKind::Building:
      building = building || object.state == ObjectState::Intact;
      break;
    case ObjectKind::Ladder:
    case ObjectKind::Entrance:
      broken_passage = broken_passage || !object.traversable;
      open_passage = open_passage || object.traversable;
      supported = supported || object.traversable;
      break;
    case ObjectKind::Terrain:
      supported = supported || object.traversable;
      break;
    case ObjectKind::Npc:
    case ObjectKind::MaterialSource:
      break;
    }
  }
  return supported && !broken_passage && (!building || open_passage);
}

/// An object to stand near and the action that works there.
struct Goal {
  WorldArea area;
  Action action;
};

std::vector<Goal> collection_goals(const GameView &view) {
  std::vector<Goal> goals;
  for (const auto &stock : view.exploration.sources) {
    const auto object =
        std::ranges::find(view.world.objects, stock.id, &ObjectSnapshot::id);
    if (stock.remaining > 0 && object != view.world.objects.end() &&
        object->material_available) {
      goals.push_back({.area = object->area, .action = Collect{stock.id}});
    }
  }
  return goals;
}

std::vector<Goal> repair_goals(const GameView &view, Actor actor) {
  std::vector<Goal> goals;
  const auto &tools = state_of(view, actor).tools;
  for (const auto &object : view.world.objects) {
    if (object.state == ObjectState::Destroyed && object.repair_with &&
        std::ranges::find(tools, *object.repair_with) != tools.end()) {
      goals.push_back(
          {.area = object.area,
           .action = Repair{.target = object.id, .tool = *object.repair_with}});
    }
  }
  return goals;
}

std::optional<Action> goal_in_range(const std::vector<Goal> &goals,
                                    GridPosition position,
                                    std::uint32_t range) {
  for (const auto &goal : goals) {
    if (distance(position, goal.area) <= range) {
      return goal.action;
    }
  }
  return std::nullopt;
}

/// A cell to search, with the first step of the route that reaches it.
struct Frontier {
  GridPosition position;
  Direction first_step = Direction::Idle;
};

/// Breadth-first over walkable cells, so a route never leaves support. Gives
/// the goal's action once it works from `start`, otherwise the next step.
std::optional<ActorCommand> route_to(const WorldSnapshot &world,
                                     GridPosition start,
                                     const std::vector<Goal> &goals,
                                     std::uint32_t range) {
  if (goals.empty()) {
    return std::nullopt;
  }
  std::set<std::pair<std::int32_t, std::int32_t>> seen{{start.x, start.y}};
  std::deque<Frontier> frontier{{.position = start}};
  while (!frontier.empty()) {
    const auto cell = frontier.front();
    frontier.pop_front();
    if (const auto action = goal_in_range(goals, cell.position, range)) {
      if (cell.first_step == Direction::Idle) {
        return ActorCommand{.action = action};
      }
      return ActorCommand{.movement = {.direction = cell.first_step}};
    }
    for (const auto step : STEPS) {
      const auto next = neighbour(cell.position, step);
      if (next && walkable(world, *next) &&
          seen.insert({next->x, next->y}).second) {
        frontier.push_back({.position = *next,
                            .first_step = cell.first_step == Direction::Idle
                                              ? step
                                              : cell.first_step});
      }
    }
  }
  return std::nullopt;
}

/// Collection comes first; repair is the fallback when nothing is collectable.
std::optional<ActorCommand> next_errand(const GameView &view, Actor actor,
                                        GridPosition from,
                                        std::uint32_t range) {
  if (auto errand = route_to(view.world, from, collection_goals(view), range)) {
    return errand;
  }
  return route_to(view.world, from, repair_goals(view, actor), range);
}

std::optional<IngredientId> first_unidentified(const GameView &view) {
  for (const auto *inventory : {&view.progression.chef.inventory,
                                &view.progression.appraiser.inventory}) {
    for (const auto ingredient : *inventory) {
      if (std::ranges::find(view.workshop.identified, ingredient) ==
          view.workshop.identified.end()) {
        return ingredient;
      }
    }
  }
  return std::nullopt;
}

bool cookable(const Recipe &recipe, const GameView &view) {
  auto carried = view.progression.chef.inventory;
  carried.insert(carried.end(), view.progression.appraiser.inventory.begin(),
                 view.progression.appraiser.inventory.end());
  for (const auto ingredient : recipe.ingredients) {
    const auto found = std::ranges::find(carried, ingredient);
    if (found == carried.end() ||
        std::ranges::find(view.workshop.identified, ingredient) ==
            view.workshop.identified.end()) {
      return false;
    }
    carried.erase(found);
  }
  return true;
}

bool eaten(const GameView &view, FoodId food) {
  return std::ranges::find(view.workshop.eaten_foods, food) !=
         view.workshop.eaten_foods.end();
}

// Reads no `Recipe::disliked`: a dislike is learned by feeding.
std::optional<FoodId> food_to_cook(const WorkshopConfig &workshop,
                                   const GameView &view) {
  std::optional<FoodId> repeat;
  for (const auto &recipe : workshop.recipes) {
    if (!cookable(recipe, view)) {
      continue;
    }
    if (!eaten(view, recipe.food)) {
      return recipe.food;
    }
    repeat = repeat.value_or(recipe.food);
  }
  return repeat;
}

std::optional<FoodId> food_to_feed(const GameView &view) {
  const auto &foods = view.workshop.foods;
  const auto fresh = std::ranges::find_if(
      foods, [&view](FoodId food) { return !eaten(view, food); });
  if (fresh != foods.end()) {
    return *fresh;
  }
  return foods.empty() ? std::nullopt : std::optional{foods.front()};
}

std::optional<Action> role_craft(const WorkshopConfig &workshop, Actor actor,
                                 const GameView &view) {
  switch (actor) {
  case Actor::Appraiser:
    if (const auto ingredient = first_unidentified(view)) {
      return Appraise{*ingredient};
    }
    return std::nullopt;
  case Actor::Chef:
    if (const auto food = food_to_cook(workshop, view)) {
      return Cook{*food};
    }
    return std::nullopt;
  }
  std::abort();
}

/// What AI does at base in this actor's role. One stored dish is enough, so a
/// chef spends no ingredients on a second.
std::optional<Action> ai_role_work(const WorkshopConfig &workshop, Actor actor,
                                   const GameView &view) {
  const auto tool = role_tool(actor);
  const auto replacement = replacement_for(workshop, tool);
  const auto carries = [replacement](const ActorSnapshot &state) {
    return std::ranges::find(state.tools, replacement) != state.tools.end();
  };
  if (tool_state(view.workshop, tool) == ToolState::Damaged &&
      (carries(view.progression.chef) || carries(view.progression.appraiser))) {
    return ReplaceTool{tool};
  }
  if (actor == Actor::Chef && !view.workshop.foods.empty()) {
    return std::nullopt;
  }
  return role_craft(workshop, actor, view);
}

ActorCommand translate(const CompanionConfig &config, Actor actor,
                       const PlayerInput &input, const GameView &view) {
  ActorCommand command;
  if (state_of(view, actor).location == Location::City) {
    command.movement = input.movement;
    if (input.travel) {
      command.action = ReturnToBase{};
    } else if (input.interact) {
      const auto position = position_of(view, actor);
      const auto range = config.exploration.collection_distance;
      command.action = goal_in_range(collection_goals(view), position, range);
      if (!command.action) {
        command.action =
            goal_in_range(repair_goals(view, actor), position, range);
      }
    }
    return command;
  }
  if (input.travel) {
    command.action = Enter{};
  } else if (input.interact) {
    command.action = role_craft(config.workshop, actor, view);
  } else if (input.replace_tool) {
    command.action = ReplaceTool{role_tool(actor)};
  } else if (input.feed) {
    if (const auto food = food_to_feed(view)) {
      command.action = Feed{*food};
    }
  } else if (input.end_day) {
    command.action = EndDayUnfed{config.unfed_penalty};
  }
  return command;
}

bool worth_entering(const CompanionConfig &config, Actor actor,
                    const GameView &view) {
  const auto entry = config.exploration.entry;
  return view.progression.remaining_exploration.count() > 0 &&
         walkable(view.world, entry) &&
         next_errand(view, actor, entry, config.exploration.collection_distance)
             .has_value();
}

ActorCommand ai_at_workshop(const CompanionConfig &config, Actor actor,
                            ControlMap control, const GameView &view) {
  if (const auto work = ai_role_work(config.workshop, actor, view)) {
    return {.action = work};
  }
  const auto partner = partner_of(actor);
  // A human sets out and ends the day: the workshop has no clock to rush them.
  if (controller_of(control, partner) == Controller::Human ||
      ai_role_work(config.workshop, partner, view)) {
    return {};
  }
  if (worth_entering(config, actor, view)) {
    return {.action = Enter{}};
  }
  // One actor ends the day, so a shared step cannot end two.
  if (actor != Actor::Chef) {
    return {};
  }
  if (const auto food = food_to_feed(view)) {
    return {.action = Feed{*food}};
  }
  return {.action = EndDayUnfed{config.unfed_penalty}};
}

ActorCommand ai_in_exploration(const CompanionConfig &config, Actor actor,
                               ControlMap control, const GameView &view) {
  const auto in_city = state_of(view, actor).location == Location::City;
  const auto partner = partner_of(actor);
  // Staying out with a human at base would hold the workshop shut.
  if (controller_of(control, partner) == Controller::Human &&
      state_of(view, partner).location == Location::Base) {
    return in_city ? ActorCommand{.action = ReturnToBase{}} : ActorCommand{};
  }
  if (!in_city) {
    return worth_entering(config, actor, view) ? ActorCommand{.action = Enter{}}
                                               : ActorCommand{};
  }
  auto errand = next_errand(view, actor, position_of(view, actor),
                            config.exploration.collection_distance);
  return errand ? *errand : ActorCommand{.action = ReturnToBase{}};
}

ActorCommand decide(const CompanionConfig &config, Actor actor,
                    ControlMap control, const GameView &view) {
  switch (view.progression.phase) {
  case Phase::Workshop:
    return ai_at_workshop(config, actor, control, view);
  case Phase::Exploration:
    return ai_in_exploration(config, actor, control, view);
  case Phase::Escaped:
    return {};
  }
  std::abort();
}

template <typename... Handlers> struct Overloaded : Handlers... {
  using Handlers::operator()...;
};

} // namespace

ControlMap single_player(Actor chosen) {
  switch (chosen) {
  case Actor::Chef:
    return {.chef = Controller::Human, .appraiser = Controller::Ai};
  case Actor::Appraiser:
    return {.chef = Controller::Ai, .appraiser = Controller::Human};
  }
  std::abort();
}

ControlMap two_players() { return {}; }

ControlMap switch_control(ControlMap control) {
  return {.chef = control.appraiser, .appraiser = control.chef};
}

ControlMap leave(ControlMap control, Actor actor) {
  switch (actor) {
  case Actor::Chef:
    control.chef = Controller::Ai;
    return control;
  case Actor::Appraiser:
    control.appraiser = Controller::Ai;
    return control;
  }
  std::abort();
}

std::optional<std::size_t> key_set(ControlMap control, Actor actor) {
  if (controller_of(control, actor) == Controller::Ai) {
    return std::nullopt;
  }
  const bool second =
      actor == Actor::Appraiser && control.chef == Controller::Human;
  return second ? std::size_t{1} : std::size_t{0};
}

ActionResult apply(Exploration &exploration, Workshop &workshop, Actor actor,
                   const Action &action) {
  return std::visit(
      Overloaded{
          [&](Enter) -> ActionResult { return exploration.enter(actor); },
          [&](ReturnToBase) -> ActionResult {
            return exploration.return_to_base(actor);
          },
          [&](Collect command) -> ActionResult {
            return exploration.collect(actor, command.source);
          },
          [&](Repair command) -> ActionResult {
            return exploration.repair(actor, command.target, command.tool);
          },
          [&](Appraise command) -> ActionResult {
            return workshop.appraise(actor, command.ingredient);
          },
          [&](Cook command) -> ActionResult {
            return workshop.cook(actor, command.food);
          },
          [&](ReplaceTool command) -> ActionResult {
            return workshop.replace_tool(command.tool);
          },
          [&](Feed command) -> ActionResult {
            return workshop.feed(command.food);
          },
          [&](EndDayUnfed command) -> ActionResult {
            return workshop.end_day_unfed(command.penalty);
          },
      },
      action);
}

Companion::Companion(CompanionConfig settings) : config(std::move(settings)) {}

ActorCommand Companion::command(Actor actor, ControlMap control,
                                const PlayerInput &input,
                                const GameView &view) const {
  switch (controller_of(control, actor)) {
  case Controller::Human:
    return translate(config, actor, input, view);
  case Controller::Ai:
    return decide(config, actor, control, view);
  }
  std::abort();
}

} // namespace neocity::survival
