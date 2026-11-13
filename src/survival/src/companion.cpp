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

const ExplorerSnapshot &explorer_of(const GameView &view, Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return view.exploration.chef;
  case Actor::Appraiser:
    return view.exploration.appraiser;
  }
  std::abort();
}

const Body &body_of(const GameView &view, Actor actor) {
  return explorer_of(view, actor).body;
}

GridPosition cell_of_actor(const GameView &view, Actor actor) {
  return cell_of(body_of(view, actor).position);
}

/// Where an errand is planned from: the actor's cell, or in the air the cell a
/// straight drop lands in, so the way holds through a jump or a fall.
GridPosition landing_cell(const GameView &view, Actor actor) {
  const CellMap map(view.world, view.exploration.obstacles);
  auto cell = cell_of_actor(view, actor);
  while (!holds(map, cell) && !map.below(cell)) {
    ++cell.y;
  }
  return cell;
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
                             }) != state.inventory.end();
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

/// The cells in x, either side of its partner, an actor may be searched
/// through: one short of the screen lock, so every step of a route keeps the
/// lock's room.
constexpr std::int32_t LEASH = SCREEN_LOCK / SUBCELLS - 1;

/// Where an actor's partner stands while both are in the city, which a search
/// keeps within the leash of; none for an actor free to go anywhere.
using Partner = std::optional<GridPosition>;

/// Whether `cell` lies within `reach` cells in x of the partner, if any.
bool leashed(const Partner &partner, GridPosition cell,
             std::int32_t reach = LEASH) {
  return !partner || std::abs(cell.x - partner->x) <= reach;
}

/// Breadth-first over the links between held cells, so a route never falls.
/// Finds the nearest cell `reached` accepts and the first move towards it,
/// through no cell outside the leash, which reaches as far as the start lies
/// from the partner, so an actor out of the leash can close the gap.
template <typename Reached>
std::optional<Frontier> search(const CellMap &map, GridPosition start,
                               const Partner &partner, Reached reached) {
  const auto reach =
      partner ? std::max(LEASH, std::abs(start.x - partner->x)) : LEASH;
  std::set<std::pair<std::int32_t, std::int32_t>> seen{{start.x, start.y}};
  std::deque<Frontier> frontier{{.position = start}};
  while (!frontier.empty()) {
    const auto cell = frontier.front();
    frontier.pop_front();
    if (reached(cell.position)) {
      return cell;
    }
    for (const auto &link : links(map, cell.position)) {
      if (leashed(partner, link.to, reach) &&
          seen.insert({link.to.x, link.to.y}).second) {
        frontier.push_back(
            {.position = link.to,
             .first_move = cell.first_move ? cell.first_move : link.move});
      }
    }
  }
  return std::nullopt;
}

std::optional<Route> route_to(const CellMap &map, GridPosition start,
                              const Partner &partner,
                              const std::vector<Goal> &goals,
                              std::uint32_t range) {
  if (goals.empty()) {
    return std::nullopt;
  }
  const auto found =
      search(map, start, partner, [&goals, range](GridPosition cell) {
        return goal_in_range(goals, cell, range) != nullptr;
      });
  if (!found) {
    return std::nullopt;
  }
  return Route{.goal = *goal_in_range(goals, found->position, range),
               .step = found->first_move};
}

/// What the party lacks first, then any other stocked source, among `goals`.
std::optional<Route> next_errand(const CompanionConfig &config,
                                 const GameView &view, GridPosition from,
                                 const Partner &partner,
                                 const std::vector<Goal> &goals) {
  const auto range = config.exploration.collection_distance;
  const CellMap map(view.world, view.exploration.obstacles);
  std::vector<Goal> lacking;
  std::ranges::copy_if(goals, std::back_inserter(lacking), &Goal::lacking);
  if (auto errand = route_to(map, from, partner, lacking, range)) {
    return errand;
  }
  return route_to(map, from, partner, goals, range);
}

std::optional<Route> next_errand(const CompanionConfig &config,
                                 const GameView &view, GridPosition from) {
  return next_errand(config, view, from, std::nullopt, collection_goals(view));
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
  if (actor == Actor::Chef && food_to_feed(view)) {
    return std::nullopt;
  }
  return role_craft(workshop, actor, view);
}

ActorCommand translate(const CompanionConfig &config, Actor actor,
                       const PlayerInput &input, const GameView &view) {
  ActorCommand command;
  if (state_of(view, actor).location == Location::City) {
    if (explorer_of(view, actor).collecting) {
      return command;
    }
    command.movement = input.movement;
    if (input.travel) {
      command.action = ReturnToBase{};
    } else if (input.interact) {
      const auto position = cell_of_actor(view, actor);
      const auto range = config.exploration.collection_distance;
      const auto goals = collection_goals(view);
      if (const auto *goal = goal_in_range(goals, position, range)) {
        command.action = goal->action;
      }
    }
    return command;
  }
  if (input.travel) {
    command.action = Enter{};
  } else if (input.interact) {
    command.action = role_craft(config.workshop, actor, view);
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
  if (worth_entering(config, view)) {
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

/// The goals an actor can work from within the lock of its partner, which
/// stands at `partner`.
std::vector<Goal> goals_near(const GameView &view, GridPosition partner) {
  auto goals = collection_goals(view);
  std::erase_if(goals, [partner](const Goal &goal) {
    return !leashed(partner, {.x = goal.area.x, .y = goal.area.y}) &&
           !leashed(partner,
                    {.x = goal.area.x + goal.area.width - 1, .y = goal.area.y});
  });
  return goals;
}

/// The exit zone of this map that holds `cell`, if any.
const WorldArea *exit_zone_at(const CompanionConfig &config,
                              const GameView &view, GridPosition cell) {
  const auto found =
      std::ranges::find_if(config.exploration.exits, [&](const MapExit &exit) {
        return exit.from == view.exploration.map &&
               distance(cell, exit.zone) == 0;
      });
  return found != config.exploration.exits.end() ? &found->zone : nullptr;
}

/// The route to where `partner` is kept company, within the leash of
/// `leash`: inside the exit zone it stands in, else within two cells across
/// and one up or down of it.
std::optional<Frontier> company_route(const CompanionConfig &config,
                                      const GameView &view, GridPosition from,
                                      GridPosition partner,
                                      const Partner &leash) {
  const CellMap map(view.world, view.exploration.obstacles);
  const auto *zone = exit_zone_at(config, view, partner);
  return search(map, from, leash, [&](GridPosition cell) {
    if (zone != nullptr) {
      return distance(cell, *zone) == 0;
    }
    return std::abs(cell.x - partner.x) <= 2 &&
           std::abs(cell.y - partner.y) <= 1;
  });
}

/// Where an actor waits for its partner: it walks to the company the leash
/// allows, and when the leash holds no way, to the nearest way there is.
ActorCommand keep_company(const CompanionConfig &config, const GameView &view,
                          const Body &body, GridPosition partner) {
  const auto from = cell_of(body.position);
  auto found = company_route(config, view, from, partner, partner);
  if (!found) {
    found = company_route(config, view, from, partner, std::nullopt);
  }
  if (!found || !found->first_move) {
    return {};
  }
  return {.movement = steer(*found->first_move, body)};
}

/// Whether `actor` goes where its partner goes: always after a human, and the
/// appraiser after the chef when both are AI.
bool follows(ControlMap control, Actor actor) {
  return controller_of(control, partner_of(actor)) == Controller::Human ||
         actor == Actor::Appraiser;
}

/// How far across, in cells, the one that leads waits for the one that follows
/// before it goes on, so a follower's detour never meets the screen lock.
constexpr std::int32_t TETHER = 3;

ActorCommand plan_alone(const CompanionConfig &config, const GameView &view,
                        const Body &body, GridPosition from) {
  const auto route = next_errand(config, view, from);
  if (!route) {
    return {.action = ReturnToBase{}};
  }
  if (!route->step) {
    return {.action = route->goal.action};
  }
  return {.movement = steer(*route->step, body)};
}

ActorCommand ai_in_exploration(const CompanionConfig &config, Actor actor,
                               ControlMap control, const GameView &view) {
  const auto in_city = state_of(view, actor).location == Location::City;
  // A collecting actor takes no command until the collection is done.
  if (in_city && explorer_of(view, actor).collecting) {
    return {};
  }
  const auto partner = partner_of(actor);
  // Staying out with a human at base would hold the workshop shut.
  if (controller_of(control, partner) == Controller::Human &&
      state_of(view, partner).location == Location::Base) {
    return in_city ? ActorCommand{.action = ReturnToBase{}} : ActorCommand{};
  }
  if (!in_city) {
    // Entering is refused while the partner is out on another map.
    if (state_of(view, partner).location == Location::City &&
        view.exploration.map != MapId::Street) {
      return {};
    }
    return worth_entering(config, view) ? ActorCommand{.action = Enter{}}
                                        : ActorCommand{};
  }
  const auto &body = body_of(view, actor);
  // In the air there is nothing to choose: keep the way the jump or drop went.
  if (body.stance == Stance::Airborne) {
    return {.movement = {.direction = body.heading}};
  }
  const auto from = cell_of(body.position);
  const auto together = state_of(view, partner).location == Location::City &&
                        !explorer_of(view, partner).absent;
  if (!together) {
    return plan_alone(config, view, body, from);
  }
  const auto beside = landing_cell(view, partner);
  if (follows(control, actor)) {
    // A partner in an exit zone is going through it: no errand holds the party.
    if (exit_zone_at(config, view, beside) != nullptr) {
      return keep_company(config, view, body, beside);
    }
    const auto route =
        next_errand(config, view, from, beside, goals_near(view, beside));
    if (route) {
      return route->step ? ActorCommand{.movement = steer(*route->step, body)}
                         : ActorCommand{.action = route->goal.action};
    }
    // Errands out of the lock's reach keep it with its partner; with none left
    // anywhere it goes home.
    if (next_errand(config, view, from)) {
      return keep_company(config, view, body, beside);
    }
    return {.action = ReturnToBase{}};
  }
  const auto command = plan_alone(config, view, body, from);
  if (!command.action && std::abs(beside.x - from.x) > TETHER) {
    // The leader waits for a follower that can come back within the lock, and
    // meets one whose only way lies beyond it.
    const GridPosition follower = beside;
    const GridPosition leader = from;
    return company_route(config, view, follower, leader, leader)
               ? ActorCommand{}
               : keep_company(config, view, body, beside);
  }
  return command;
}

/// Temporary policy: movement units ahead an AI actor looks for a mutant, and
/// sub-cells it walks in that time at most. A walking actor covers 25
/// sub-cells in a unit; the reach is rounded up to 30 for margin.
constexpr int PATROL_LOOKAHEAD = 2;
constexpr std::int32_t ACTOR_UNIT_REACH = 30;

/// Whether walking the way `command` moves would put the actor in a cell a
/// mutant stands in or reaches within the next movement units. Only a walk
/// along the ground is held: a body in the air keeps its way, because `decide`
/// would drop its sideways move, and a climb moves no sideways.
bool mutant_ahead(const CompanionConfig &config, Actor actor,
                  const ActorCommand &command, const GameView &view) {
  const auto direction = command.movement.direction;
  const auto &body = body_of(view, actor);
  if ((direction != Direction::Left && direction != Direction::Right) ||
      command.action || body.stance == Stance::Airborne ||
      state_of(view, actor).location != Location::City) {
    return false;
  }
  const auto sign = direction == Direction::Right ? 1 : -1;
  const auto row = cell_of(body.position).y;
  const auto reach = Position{.x = body.position.x +
                                   sign * PATROL_LOOKAHEAD * ACTOR_UNIT_REACH,
                              .y = body.position.y};
  const auto near = std::min(cell_of(body.position).x, cell_of(reach).x);
  const auto far = std::max(cell_of(body.position).x, cell_of(reach).x);
  for (const auto &mutant : view.exploration.mutants) {
    const auto cell = cell_of(mutant.position);
    const auto patrol = std::ranges::find_if(
        config.exploration.patrols, [&](const Patrol &entry) {
          return entry.map == view.exploration.map && entry.from.y == cell.y &&
                 entry.from.x <= cell.x && cell.x <= entry.to.x;
        });
    if (cell.y != row || patrol == config.exploration.patrols.end()) {
      continue;
    }
    auto ahead = mutant;
    for (int unit = 0; unit <= PATROL_LOOKAHEAD; ++unit) {
      const auto at = cell_of(ahead.position).x;
      if (near <= at && at <= far) {
        return true;
      }
      ahead = patrol_step(*patrol, ahead);
    }
  }
  return false;
}

ActorCommand decide(const CompanionConfig &config, Actor actor,
                    ControlMap control, const GameView &view) {
  switch (view.progression.phase) {
  case Phase::Workshop:
    return ai_at_workshop(config, actor, control, view);
  case Phase::Exploration: {
    const auto command = ai_in_exploration(config, actor, control, view);
    return mutant_ahead(config, actor, command, view) ? ActorCommand{}
                                                      : command;
  }
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

ActionResult run(Exploration & /*exploration*/, Workshop &workshop, Actor actor,
                 Appraise command) {
  return workshop.appraise(actor, command.ingredient);
}

ActionResult run(Exploration & /*exploration*/, Workshop &workshop, Actor actor,
                 Cook command) {
  return workshop.cook(actor, command.food);
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

bool worth_entering(const CompanionConfig &config, const GameView &view) {
  const auto entry = config.exploration.entry;
  return view.progression.remaining_exploration.count() > 0 &&
         stands_on(CellMap(view.world, view.exploration.obstacles), entry) &&
         next_errand(config, view, entry).has_value();
}

Companion::Companion(CompanionConfig settings) : config(std::move(settings)) {}

std::optional<Errand> Companion::errand(Actor actor,
                                        const GameView &view) const {
  const auto route = next_errand(config, view, landing_cell(view, actor));
  if (!route) {
    return std::nullopt;
  }
  return Errand{.target = route->goal.target, .step = route->step};
}

ActorCommand Companion::command(Actor actor, ControlMap control,
                                const PlayerInput &input,
                                const GameView &view) const {
  // An absent actor is off the map and takes no command.
  if (explorer_of(view, actor).absent) {
    return {};
  }
  switch (controller_of(control, actor)) {
  case Controller::Human:
    return translate(config, actor, input, view);
  case Controller::Ai:
    return decide(config, actor, control, view);
  }
  std::abort();
}

} // namespace neocity::survival
