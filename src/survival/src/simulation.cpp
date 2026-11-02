#include <survival/simulation.hpp>

#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <utility>

namespace neocity::survival {
namespace {

template <typename Item> bool take(std::vector<Item> &inventory, Item item) {
  const auto found = std::ranges::find(inventory, item);
  if (found == inventory.end()) {
    return false;
  }
  inventory.erase(found);
  return true;
}

} // namespace

Simulation::Simulation(Config config)
    : daily_budget(config.exploration_budget), full_acid(config.full_acid) {
  if (config.exploration_budget <= std::chrono::microseconds::zero()) {
    throw std::invalid_argument("exploration budget must be positive");
  }
  if (config.full_acid.value == 0) {
    throw std::invalid_argument("full acid must be positive");
  }
  world.remaining_exploration = config.exploration_budget;
  world.seed = config.seed;
}

Result Simulation::enter(Actor actor) {
  if (run_over(world.phase)) {
    return Result::RunOver;
  }
  if (world.remaining_exploration == std::chrono::microseconds::zero()) {
    return Result::NoExplorationTime;
  }
  actor_state(actor).location = Location::City;
  world.phase = Phase::Exploration;
  return Result::Applied;
}

Result Simulation::return_to_base(Actor actor) {
  auto &state = actor_state(actor);
  if (state.location != Location::City) {
    return Result::NotExploring;
  }
  relocate(state);
  return Result::Applied;
}

Result Simulation::advance(std::chrono::microseconds elapsed) {
  if (elapsed < std::chrono::microseconds::zero()) {
    return Result::InvalidElapsed;
  }
  if (world.phase == Phase::Exploration) {
    world.remaining_exploration -=
        std::min(elapsed, world.remaining_exploration);
    if (world.remaining_exploration == std::chrono::microseconds::zero()) {
      relocate(world.chef);
      relocate(world.appraiser);
    }
  }
  return Result::Applied;
}

Result Simulation::damage(Actor actor) {
  auto &state = actor_state(actor);
  if (state.location != Location::City) {
    return Result::NotExploring;
  }
  if (state.health > 0) {
    --state.health;
  }
  if (state.health == 0) {
    ++state.exhaustions;
    relocate(state);
  }
  return Result::Applied;
}

Result Simulation::collect(Actor actor, IngredientId ingredient) {
  auto &state = actor_state(actor);
  if (state.location != Location::City) {
    return Result::NotExploring;
  }
  state.inventory.push_back(ingredient);
  ++state.load;
  return Result::Applied;
}

Result Simulation::collect_tool(Actor actor, ToolId tool) {
  auto &state = actor_state(actor);
  if (state.location != Location::City) {
    return Result::NotExploring;
  }
  state.tools.push_back(tool);
  return Result::Applied;
}

Result Simulation::consume(std::span<const IngredientId> ingredients) {
  if (world.phase != Phase::Workshop) {
    return Result::NotInWorkshop;
  }
  auto chef = world.chef.inventory;
  auto appraiser = world.appraiser.inventory;
  for (const auto ingredient : ingredients) {
    if (!take(chef, ingredient) && !take(appraiser, ingredient)) {
      return Result::MissingIngredients;
    }
  }
  world.chef.inventory = std::move(chef);
  world.appraiser.inventory = std::move(appraiser);
  return Result::Applied;
}

Result Simulation::consume_tool(ToolId tool) {
  if (world.phase != Phase::Workshop) {
    return Result::NotInWorkshop;
  }
  if (!take(world.chef.tools, tool) && !take(world.appraiser.tools, tool)) {
    return Result::MissingTool;
  }
  return Result::Applied;
}

Result Simulation::finish_day(AcidLevel acid_rise) {
  if (world.phase != Phase::Workshop) {
    return Result::NotInWorkshop;
  }
  world.acid.value +=
      std::min(acid_rise.value, full_acid.value - world.acid.value);
  ++world.day;
  world.remaining_exploration = daily_budget;
  world.chef.load = 0;
  world.appraiser.load = 0;
  if (world.acid == full_acid) {
    world.phase = Phase::Lost;
  }
  return Result::Applied;
}

Result Simulation::escape() {
  if (world.phase != Phase::Workshop) {
    return Result::NotInWorkshop;
  }
  world.phase = Phase::Escaped;
  return Result::Applied;
}

Snapshot Simulation::snapshot() const { return world; }

ActorSnapshot &Simulation::actor_state(Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return world.chef;
  case Actor::Appraiser:
    return world.appraiser;
  }
  std::abort();
}

void Simulation::relocate(ActorSnapshot &actor) {
  actor.location = Location::Base;
  actor.health = 3;
  if (world.chef.location == Location::Base &&
      world.appraiser.location == Location::Base) {
    world.phase = Phase::Workshop;
  }
}

} // namespace neocity::survival
