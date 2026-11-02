#include <survival/companion.hpp>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <deque>
#include <iterator>
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

/// An object to stand near and the action that works there. `lacking` marks a
/// source of what the party carries none of.
struct Goal {
  WorldArea area;
  Action action;
  bool lacking = false;
};

bool party_carries(const GameView &view, const ObjectSnapshot &source) {
  const auto holds = [&source](const ActorSnapshot &state) {
    return std::ranges::find(state.inventory, source.ingredient,
                             [](IngredientId held) {
                               return std::optional{held};
                             }) != state.inventory.end() ||
           std::ranges::find(state.tools, source.tool, [](ToolId held) {
             return std::optional{held};
           }) != state.tools.end();
  };
  return holds(view.progression.chef) || holds(view.progression.appraiser);
}

/// Stocked sources this actor can still take from today, what the party lacks
/// first.
std::vector<Goal> collection_goals(const CompanionConfig &config,
                                   const GameView &view, Actor actor) {
  const bool load_left =
      state_of(view, actor).load < config.exploration.daily_load;
  std::vector<Goal> goals;
  for (const auto &stock : view.exploration.sources) {
    const auto object =
        std::ranges::find(view.world.objects, stock.id, &ObjectSnapshot::id);
    if (stock.remaining > 0 && object != view.world.objects.end() &&
        object->material_available && (object->tool || load_left)) {
      goals.push_back({.area = object->area,
                       .action = Collect{stock.id},
                       .lacking = !party_carries(view, *object)});
    }
  }
  std::ranges::stable_partition(goals, &Goal::lacking);
  return goals;
}

std::vector<Goal> repair_goals(const GameView &view, Actor actor) {
  std::vector<Goal> goals;
  const auto &tools = state_of(view, actor).tools;
  for (const auto &object : view.world.objects) {
    if (object.state != ObjectState::Destroyed || !object.repair_with) {
      continue;
    }
    const auto tool = *object.repair_with;
    if (std::ranges::find(tools, tool) != tools.end()) {
      goals.push_back({.area = object.area,
                       .action = Repair{.target = object.id, .tool = tool}});
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

/// Breadth-first over supported cells, so a route never leaves support. Gives
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
      if (next && cell_at(world, *next) == Cell::Supported &&
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

/// Collection comes first, what the party lacks before anything else; repair is
/// the fallback when nothing is collectable.
std::optional<ActorCommand> next_errand(const CompanionConfig &config,
                                        const GameView &view, Actor actor,
                                        GridPosition from) {
  const auto range = config.exploration.collection_distance;
  const auto goals = collection_goals(config, view, actor);
  std::vector<Goal> lacking;
  std::ranges::copy_if(goals, std::back_inserter(lacking), &Goal::lacking);
  if (auto errand = route_to(view.world, from, lacking, range)) {
    return errand;
  }
  if (auto errand = route_to(view.world, from, goals, range)) {
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

bool rejected(const GameView &view, FoodId food) {
  return std::ranges::find(view.workshop.rejected_foods, food) !=
         view.workshop.rejected_foods.end();
}

/// New before repeated and rejected last, the tastiest first within each.
/// Reads no `Recipe::disliked`: a dislike is learned by feeding.
std::optional<FoodId> food_to_cook(const WorkshopConfig &workshop,
                                   const GameView &view) {
  const auto rank = [&view, &workshop](const Recipe &recipe) {
    return std::pair{
        !eaten(view, recipe.food) ? 2 : int{!rejected(view, recipe.food)},
        taste_of(recipe, workshop)};
  };
  const Recipe *best = nullptr;
  for (const auto &recipe : workshop.recipes) {
    if (cookable(recipe, view) &&
        (best == nullptr || rank(recipe) > rank(*best))) {
      best = &recipe;
    }
  }
  return best != nullptr ? std::optional{best->food} : std::nullopt;
}

std::optional<FoodId> food_to_feed(const GameView &view) {
  const auto &foods = view.workshop.foods;
  const auto fresh = std::ranges::find_if(
      foods, [&view](const Dish &dish) { return !eaten(view, dish.food); });
  if (fresh != foods.end()) {
    return fresh->food;
  }
  return foods.empty() ? std::nullopt : std::optional{foods.front().food};
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
      command.action =
          goal_in_range(collection_goals(config, view, actor), position, range);
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
         cell_at(view.world, entry) == Cell::Supported &&
         next_errand(config, view, actor, entry).has_value();
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
  auto errand = next_errand(config, view, actor, position_of(view, actor));
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
  case Phase::Lost:
    return {};
  }
  std::abort();
}

ActionResult run(Exploration &exploration, Workshop & /*workshop*/, Actor actor,
                 Enter /*command*/) {
  return exploration.enter(actor);
}

ActionResult run(Exploration &exploration, Workshop & /*workshop*/, Actor actor,
                 ReturnToBase /*command*/) {
  return exploration.return_to_base(actor);
}

ActionResult run(Exploration &exploration, Workshop & /*workshop*/, Actor actor,
                 Collect command) {
  return exploration.collect(actor, command.source);
}

ActionResult run(Exploration &exploration, Workshop & /*workshop*/, Actor actor,
                 Repair command) {
  return exploration.repair(actor, command.target, command.tool);
}

ActionResult run(Exploration & /*exploration*/, Workshop &workshop, Actor actor,
                 Appraise command) {
  return workshop.appraise(actor, command.ingredient);
}

ActionResult run(Exploration & /*exploration*/, Workshop &workshop, Actor actor,
                 Cook command) {
  return workshop.cook(actor, command.food);
}

ActionResult run(Exploration & /*exploration*/, Workshop &workshop,
                 Actor /*actor*/, ReplaceTool command) {
  return workshop.replace_tool(command.tool);
}

ActionResult run(Exploration & /*exploration*/, Workshop &workshop,
                 Actor /*actor*/, Feed command) {
  return workshop.feed(command.food);
}

ActionResult run(Exploration & /*exploration*/, Workshop &workshop,
                 Actor /*actor*/, EndDayUnfed command) {
  return workshop.end_day_unfed(command.penalty);
}

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
      [&](const auto &command) {
        return run(exploration, workshop, actor, command);
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
