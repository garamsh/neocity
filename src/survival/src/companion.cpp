#include <survival/companion.hpp>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <deque>
#include <iterator>
#include <set>
#include <utility>
#include <vector>

namespace neocity::survival {
namespace {

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

const Body &body_of(const GameView &view, Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return view.exploration.chef.body;
  case Actor::Appraiser:
    return view.exploration.appraiser.body;
  }
  std::abort();
}

GridPosition cell_of_actor(const GameView &view, Actor actor) {
  return cell_of(body_of(view, actor).position);
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

/// An object to stand near and the action that works there. `lacking` marks a
/// source of what the party carries none of.
struct Goal {
  ObjectId target;
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

/// Stocked sources that are supplying, what the party lacks first.
std::vector<Goal> collection_goals(const GameView &view) {
  std::vector<Goal> goals;
  for (const auto &stock : view.exploration.sources) {
    const auto object =
        std::ranges::find(view.world.objects, stock.id, &ObjectSnapshot::id);
    if (stock.remaining > 0 && object != view.world.objects.end() &&
        object->material_available) {
      goals.push_back({.target = stock.id,
                       .area = object->area,
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
      goals.push_back({.target = object.id,
                       .area = object.area,
                       .action = Repair{.target = object.id, .tool = tool}});
    }
  }
  return goals;
}

const Goal *goal_in_range(const std::vector<Goal> &goals, GridPosition position,
                          std::uint32_t range) {
  const auto found =
      std::ranges::find_if(goals, [position, range](const Goal &goal) {
        return distance(position, goal.area) <= range;
      });
  return found != goals.end() ? &*found : nullptr;
}

/// A cell to search, with the first move of the route that reaches it.
struct Frontier {
  GridPosition position;
  std::optional<Move> first_move = std::nullopt;
};

/// A goal and the first move towards it: none once it works from where the
/// actor stands.
struct Route {
  Goal goal;
  std::optional<Move> step = std::nullopt;
};

/// Breadth-first over the links between held cells, so a route never falls.
std::optional<Route> route_to(const CellMap &map, GridPosition start,
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
    if (const auto *goal = goal_in_range(goals, cell.position, range)) {
      return Route{.goal = *goal, .step = cell.first_move};
    }
    for (const auto &link : links(map, cell.position)) {
      if (seen.insert({link.to.x, link.to.y}).second) {
        frontier.push_back(
            {.position = link.to,
             .first_move = cell.first_move ? cell.first_move : link.move});
      }
    }
  }
  return std::nullopt;
}

/// Whether repairing `passage` alone brings a goal into reach from `from`.
bool opens_a_way(const WorldSnapshot &world, const Goal &passage,
                 const std::vector<Goal> &goals, GridPosition from,
                 std::uint32_t range) {
  auto repaired = world;
  for (auto &object : repaired.objects) {
    if (object.id == passage.target) {
      object.traversable = true;
    }
  }
  return route_to(CellMap(repaired), from, goals, range).has_value();
}

/// Collection comes first, what the party lacks before anything else; repair is
/// the fallback when nothing is collectable, and only of a passage that opens
/// the way to stock.
std::optional<Route> next_errand(const CompanionConfig &config,
                                 const GameView &view, Actor actor,
                                 GridPosition from) {
  const auto range = config.exploration.collection_distance;
  const CellMap map(view.world);
  const auto goals = collection_goals(view);
  std::vector<Goal> lacking;
  std::ranges::copy_if(goals, std::back_inserter(lacking), &Goal::lacking);
  if (auto errand = route_to(map, from, lacking, range)) {
    return errand;
  }
  if (auto errand = route_to(map, from, goals, range)) {
    return errand;
  }
  std::vector<Goal> repairs;
  std::ranges::copy_if(repair_goals(view, actor), std::back_inserter(repairs),
                       [&](const Goal &passage) {
                         return opens_a_way(view.world, passage, goals, from,
                                            range);
                       });
  return route_to(map, from, repairs, range);
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

/// New before repeated, the tastiest first within each; a food the monster
/// rejected is never offered again. Reads no `Recipe::disliked`: a dislike is
/// learned by feeding.
std::optional<FoodId> food_to_cook(const WorkshopConfig &workshop,
                                   const GameView &view) {
  const auto rank = [&view, &workshop](const Recipe &recipe) {
    return std::pair{!eaten(view, recipe.food), taste_of(recipe, workshop)};
  };
  const Recipe *best = nullptr;
  for (const auto &recipe : workshop.recipes) {
    if (cookable(recipe, view) && !rejected(view, recipe.food) &&
        (best == nullptr || rank(recipe) > rank(*best))) {
      best = &recipe;
    }
  }
  return best != nullptr ? std::optional{best->food} : std::nullopt;
}

/// An uneaten dish first, then any other; a food the monster rejected is never
/// fed again, so a stored one of it counts as no dish.
std::optional<FoodId> food_to_feed(const GameView &view) {
  std::optional<FoodId> repeat;
  for (const auto &dish : view.workshop.foods) {
    if (rejected(view, dish.food)) {
      continue;
    }
    if (!eaten(view, dish.food)) {
      return dish.food;
    }
    if (!repeat) {
      repeat = dish.food;
    }
  }
  return repeat;
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

/// What AI does at base in this actor's role. One stored dish it would feed is
/// enough, so a chef spends no ingredients on a second.
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
  if (actor == Actor::Chef && food_to_feed(view)) {
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
      const auto position = cell_of_actor(view, actor);
      const auto range = config.exploration.collection_distance;
      const auto goals = collection_goals(view);
      const auto repairs = repair_goals(view, actor);
      const auto *goal = goal_in_range(goals, position, range);
      goal = goal != nullptr ? goal : goal_in_range(repairs, position, range);
      if (goal != nullptr) {
        command.action = goal->action;
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
  const auto &body = body_of(view, actor);
  // In the air there is nothing to choose: keep the way the jump or drop went.
  if (body.stance == Stance::Airborne) {
    return {.movement = {.direction = body.heading}};
  }
  const auto route = next_errand(config, view, actor, cell_of(body.position));
  if (!route) {
    return {.action = ReturnToBase{}};
  }
  if (!route->step) {
    return {.action = route->goal.action};
  }
  return {.movement = steer(*route->step, body)};
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

bool worth_entering(const CompanionConfig &config, Actor actor,
                    const GameView &view) {
  const auto entry = config.exploration.entry;
  return view.progression.remaining_exploration.count() > 0 &&
         stands_on(CellMap(view.world), entry) &&
         next_errand(config, view, actor, entry).has_value();
}

Companion::Companion(CompanionConfig settings) : config(std::move(settings)) {}

std::optional<Errand> Companion::errand(Actor actor,
                                        const GameView &view) const {
  const auto route =
      next_errand(config, view, actor, cell_of_actor(view, actor));
  if (!route) {
    return std::nullopt;
  }
  return Errand{.target = route->goal.target, .step = route->step};
}

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
