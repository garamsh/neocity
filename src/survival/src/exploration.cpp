#include <survival/exploration.hpp>

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <utility>

namespace neocity::survival {
namespace {

using namespace std::chrono_literals;
constexpr auto MOVEMENT_TICK = 100ms;
constexpr std::array ACTORS{Actor::Chef, Actor::Appraiser};

ActorSnapshot actor_state(const Snapshot &snapshot, Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return snapshot.chef;
  case Actor::Appraiser:
    return snapshot.appraiser;
  }
  std::abort();
}

GridPosition direction_offset(Direction direction) {
  switch (direction) {
  case Direction::Idle:
    return {};
  case Direction::Left:
    return {.x = -1};
  case Direction::Right:
    return {.x = 1};
  case Direction::Up:
    return {.y = -1};
  case Direction::Down:
    return {.y = 1};
  }
  std::abort();
}

void require_applied(Result result) {
  if (result != Result::Applied) {
    throw std::logic_error("exploration command lost its active actor");
  }
}

} // namespace

std::int64_t distance(GridPosition position, WorldArea area) {
  const auto right = std::int64_t{area.x} + area.width - 1;
  const auto bottom = std::int64_t{area.y} + area.height - 1;
  return std::max({std::int64_t{area.x} - position.x,
                   std::int64_t{position.x} - right, std::int64_t{0}}) +
         std::max({std::int64_t{area.y} - position.y,
                   std::int64_t{position.y} - bottom, std::int64_t{0}});
}

Cell cell_at(const WorldSnapshot &snapshot, GridPosition position) {
  bool supported = false;
  bool building = false;
  bool open_passage = false;
  bool broken_passage = false;
  for (const auto &object : snapshot.objects) {
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
  if (broken_passage || (building && !open_passage)) {
    return Cell::Blocked;
  }
  return supported ? Cell::Supported : Cell::Falling;
}

Exploration::Exploration(Simulation &progression, World &city,
                         ExplorationConfig settings)
    : simulation(progression), world(city), config(std::move(settings)) {
  const auto snapshot = world.get().snapshot();
  if (config.maximum_stamina == 0 || config.collection_time < 0us ||
      cell_at(snapshot, config.entry) != Cell::Supported) {
    throw std::invalid_argument("exploration requires stamina and safe entry");
  }
  std::vector<ObjectId> ids;
  for (const auto &source : config.sources) {
    const auto object = std::ranges::find_if(
        snapshot.objects,
        [&](const ObjectSnapshot &entry) { return entry.id == source.id; });
    if (source.remaining == 0 ||
        std::ranges::find(ids, source.id) != ids.end() ||
        object == snapshot.objects.end() ||
        object->kind != ObjectKind::MaterialSource) {
      throw std::invalid_argument(
          "stock requires unique existing material sources");
    }
    ids.push_back(source.id);
  }
  synchronize();
}

Result Exploration::enter(Actor actor) {
  if (actor_state(simulation.get().snapshot(), actor).location ==
      Location::City) {
    return Result::Applied;
  }
  if (cell_at(world.get().snapshot(), config.entry) != Cell::Supported) {
    return Result::NotExploring;
  }
  const auto result = simulation.get().enter(actor);
  if (result == Result::Applied) {
    explorer(actor) = {
        .state = {.position = config.entry, .stamina = config.maximum_stamina}};
  }
  return result;
}

Result Exploration::return_to_base(Actor actor) {
  const auto result = simulation.get().return_to_base(actor);
  if (result == Result::Applied) {
    synchronize();
  }
  return result;
}

Result Exploration::damage(Actor actor) {
  const auto result = simulation.get().damage(actor);
  if (result == Result::Applied) {
    synchronize();
  }
  return result;
}

Result Exploration::advance(std::chrono::microseconds elapsed,
                            const std::array<ExplorationInput, 2> &inputs,
                            AcidLevel acid) {
  if (run_over(simulation.get().snapshot().phase)) {
    return Result::RunOver;
  }
  const auto progression = world.get().snapshot().progression;
  const auto available = simulation.get().snapshot().remaining_exploration;
  if (elapsed < 0us ||
      std::min(elapsed, available) >
          std::chrono::microseconds::max() - progression.elapsed) {
    return Result::InvalidElapsed;
  }
  synchronize();
  auto remaining = elapsed;
  // Apply acid and a finished day's time even with no elapsed time, including
  // while at base.
  advance_world(acid);
  check_support();
  while (remaining > 0us &&
         simulation.get().snapshot().phase == Phase::Exploration) {
    const auto before = simulation.get().snapshot();
    auto step = std::min(remaining, before.remaining_exploration);
    for (const auto actor : ACTORS) {
      if (actor_state(before, actor).location == Location::City) {
        step = std::min(step, MOVEMENT_TICK - explorer(actor).remainder);
      }
    }
    require_applied(simulation.get().advance(step));
    advance_world(acid);
    remaining -= step;
    for (const auto actor : ACTORS) {
      if (actor_state(before, actor).location == Location::City) {
        explorer(actor).remainder += step;
      }
    }
    synchronize();
    check_support();
    for (std::size_t index = 0; index < ACTORS.size(); ++index) {
      const auto actor = ACTORS.at(index);
      auto &state = explorer(actor);
      if (actor_state(simulation.get().snapshot(), actor).location ==
              Location::City &&
          state.remainder == MOVEMENT_TICK) {
        state.remainder = 0us;
        tick(actor, inputs.at(index));
      }
    }
  }
  return Result::Applied;
}

CollectionResult Exploration::collect(Actor actor, ObjectId source) {
  if (actor_state(simulation.get().snapshot(), actor).location !=
      Location::City) {
    return CollectionResult::NotExploring;
  }
  const auto stock =
      std::ranges::find_if(config.sources, [source](const SourceStock &entry) {
        return entry.id == source;
      });
  const auto snapshot = world.get().snapshot();
  const auto object = std::ranges::find_if(
      snapshot.objects,
      [source](const ObjectSnapshot &entry) { return entry.id == source; });
  if (stock == config.sources.end() || object == snapshot.objects.end() ||
      object->kind != ObjectKind::MaterialSource) {
    return CollectionResult::UnknownSource;
  }
  if (!object->material_available) {
    return CollectionResult::Unavailable;
  }
  if (distance(explorer(actor).state.position, object->area) >
      config.collection_distance) {
    return CollectionResult::OutOfRange;
  }
  if (stock->remaining == 0) {
    return CollectionResult::Depleted;
  }
  if (object->ingredient) {
    require_applied(simulation.get().collect(actor, *object->ingredient));
  } else if (object->tool) {
    require_applied(simulation.get().collect_tool(actor, *object->tool));
  }
  --stock->remaining;
  const auto spent =
      std::min(config.collection_time,
               simulation.get().snapshot().remaining_exploration);
  require_applied(simulation.get().advance(spent));
  advance_world(snapshot.progression.acid);
  synchronize();
  return CollectionResult::Applied;
}

RepairResult Exploration::repair(Actor actor, ObjectId target, ToolId tool) {
  const auto state = actor_state(simulation.get().snapshot(), actor);
  if (state.location != Location::City) {
    return RepairResult::NotExploring;
  }
  const auto snapshot = world.get().snapshot();
  const auto object = std::ranges::find_if(
      snapshot.objects,
      [target](const ObjectSnapshot &entry) { return entry.id == target; });
  if (object == snapshot.objects.end()) {
    return RepairResult::UnknownObject;
  }
  if (distance(explorer(actor).state.position, object->area) >
      config.collection_distance) {
    return RepairResult::OutOfRange;
  }
  if (std::ranges::find(state.tools, tool) == state.tools.end()) {
    return RepairResult::ToolNotCarried;
  }
  return world.get().repair(target, tool);
}

ExplorationSnapshot Exploration::snapshot() const {
  return {.chef = chef.state,
          .appraiser = appraiser.state,
          .sources = config.sources};
}

Exploration::Explorer &Exploration::explorer(Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return chef;
  case Actor::Appraiser:
    return appraiser;
  }
  std::abort();
}

void Exploration::synchronize() {
  const auto snapshot = simulation.get().snapshot();
  for (const auto actor : ACTORS) {
    if (actor_state(snapshot, actor).location == Location::Base) {
      explorer(actor) = {.state = {.position = config.base,
                                   .stamina = config.maximum_stamina}};
    }
  }
}

void Exploration::advance_world(AcidLevel acid) {
  if (world.get().update({.elapsed = simulation.get().snapshot().world_time,
                          .acid = acid}) != WorldUpdateResult::Applied) {
    throw std::logic_error("exploration world time regressed");
  }
}

void Exploration::check_support() {
  const auto snapshot = world.get().snapshot();
  for (const auto actor : ACTORS) {
    if (actor_state(simulation.get().snapshot(), actor).location ==
            Location::City &&
        cell_at(snapshot, explorer(actor).state.position) != Cell::Supported) {
      require_applied(return_to_base(actor));
    }
  }
}

void Exploration::tick(Actor actor, ExplorationInput input) {
  auto &state = explorer(actor).state;
  const bool running =
      input.running && input.direction != Direction::Idle && state.stamina > 0;
  if (running) {
    --state.stamina;
  } else if (state.stamina < config.maximum_stamina) {
    ++state.stamina;
  }
  const auto offset = direction_offset(input.direction);
  const auto snapshot = world.get().snapshot();
  const int cells = running ? 2 : 1;
  for (int index = 0; index < cells; ++index) {
    const auto x = std::int64_t{state.position.x} + offset.x;
    const auto y = std::int64_t{state.position.y} + offset.y;
    if (x < std::numeric_limits<std::int32_t>::min() ||
        x > std::numeric_limits<std::int32_t>::max() ||
        y < std::numeric_limits<std::int32_t>::min() ||
        y > std::numeric_limits<std::int32_t>::max()) {
      return;
    }
    const GridPosition next{.x = static_cast<std::int32_t>(x),
                            .y = static_cast<std::int32_t>(y)};
    switch (cell_at(snapshot, next)) {
    case Cell::Supported:
      state.position = next;
      break;
    case Cell::Blocked:
      return;
    case Cell::Falling:
      require_applied(return_to_base(actor));
      return;
    }
  }
}

} // namespace neocity::survival
