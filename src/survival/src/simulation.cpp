#include <survival/simulation.hpp>

#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <utility>

namespace neocity::survival {
namespace {

bool take(std::vector<IngredientId> &inventory, IngredientId item) {
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
    const auto spent = std::min(elapsed, world.remaining_exploration);
    world.remaining_exploration -= spent;
    world.world_time += spent;
    if (world.remaining_exploration == std::chrono::microseconds::zero()) {
      relocate(world.chef);
      relocate(world.appraiser);
    }
  }
  return Result::Applied;
}

Result Simulation::collect(Actor actor, IngredientId ingredient) {
  auto &state = actor_state(actor);
  if (state.location != Location::City) {
    return Result::NotExploring;
  }
  state.inventory.push_back(ingredient);
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

Result Simulation::finish_day(AcidLevel acid_rise) {
  if (world.phase != Phase::Workshop) {
    return Result::NotInWorkshop;
  }
  world.acid.value +=
      std::min(acid_rise.value, full_acid.value - world.acid.value);
  ++world.day;
  world.world_time += world.remaining_exploration;
  world.remaining_exploration = daily_budget;
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
  if (world.chef.location == Location::Base &&
      world.appraiser.location == Location::Base) {
    world.phase = Phase::Workshop;
  }
}

} // namespace neocity::survival
