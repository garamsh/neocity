#include <survival/simulation.hpp>

#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace neocity::survival {

Simulation::Simulation(Config config) {
  if (config.exploration_budget <= std::chrono::microseconds::zero()) {
    throw std::invalid_argument("exploration budget must be positive");
  }
  world.remaining_exploration = config.exploration_budget;
  world.seed = config.seed;
}

Result Simulation::enter(Actor actor) {
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
