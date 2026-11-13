#include <survival/exploration.hpp>

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>

namespace neocity::survival {
namespace {

using namespace std::chrono_literals;
constexpr auto MOVEMENT_TICK = 100ms;
constexpr auto TICKS_PER_UNIT = MOVEMENT_TICK / PHYSICS.tick;
constexpr std::array ACTORS{Actor::Chef, Actor::Appraiser};

/// A steered jump waits while one more walking unit would stay in the cell,
/// so it leaves with at most this much room before the edge.
constexpr std::int32_t JUMP_ROOM =
    static_cast<std::int32_t>(TICKS_PER_UNIT) * PHYSICS.walk - 1;
/// Ticks a link's simulated body may take before it counts as lost.
constexpr int LINK_TICKS = 200;
/// Temporary: sub-cells a mutant walks in one movement unit, a third of a
/// walking actor's.
constexpr std::int32_t MUTANT_STEP = 8;

ActorSnapshot actor_state(const Snapshot &snapshot, Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return snapshot.chef;
  case Actor::Appraiser:
    return snapshot.appraiser;
  }
  std::abort();
}

void require_applied(Result result) {
  if (result != Result::Applied) {
    throw std::logic_error("exploration command lost its active actor");
  }
}

/// Rounds towards negative infinity, so cells left of zero stay whole.
std::int32_t floor_div(std::int32_t value, std::int32_t divisor) {
  const auto quotient = value / divisor;
  return quotient * divisor > value ? quotient - 1 : quotient;
}

bool floor_at(Cell cell) {
  return cell == Cell::Floor || cell == Cell::LadderOnFloor;
}

bool ladder_at(Cell cell) {
  return cell == Cell::Ladder || cell == Cell::LadderOnFloor;
}

bool sideways(Direction direction) {
  return direction == Direction::Left || direction == Direction::Right;
}

/// The line feet rest on in `row`.
std::int32_t floor_line(std::int32_t row) { return (row + 1) * SUBCELLS; }

/// The row a walking body ends in on entering `column` from `row`: the same
/// row, a step up, or a step down; nothing when it walks off into the air.
std::optional<std::int32_t> settle(const CellMap &map, std::int32_t column,
                                   std::int32_t row) {
  for (const auto candidate : {row, row - 1, row + 1}) {
    if (stands_on(map, {.x = column, .y = candidate})) {
      return candidate;
    }
  }
  return std::nullopt;
}

/// The sideways part of a tick: stops at a blocked cell's edge, and a standing
/// body steps up, steps down, or walks off into the air.
void move_sideways(const CellMap &map, Body &body, Direction direction,
                   std::int32_t speed) {
  const auto step = direction == Direction::Left ? -speed : speed;
  body.heading = direction;
  const auto from = cell_of(body.position);
  const auto to_x = body.position.x + step;
  const auto column = floor_div(to_x, SUBCELLS);
  const auto stop = [&] {
    body.position.x = step < 0 ? from.x * SUBCELLS : floor_line(from.x) - 1;
  };
  if (column == from.x) {
    body.position.x = to_x;
    return;
  }
  if (map.at({.x = column, .y = from.y}) == Cell::Blocked) {
    stop();
    return;
  }
  if (body.stance != Stance::Standing) {
    body.position.x = to_x;
    return;
  }
  const auto row = settle(map, column, from.y);
  body.position.x = to_x;
  if (row) {
    body.position.y = floor_line(*row);
  } else {
    body.stance = Stance::Airborne;
    body.fall_speed = 0;
  }
}

/// The vertical part of a tick in the air: rising stops under a blocked cell,
/// and falling lands on the first floor line it reaches.
void move_in_air(const CellMap &map, Body &body) {
  const auto column = cell_of(body.position).x;
  const auto from = body.position.y;
  const auto to = from + body.fall_speed;
  const bool rising = body.fall_speed < 0;
  body.fall_speed = std::min(body.fall_speed + PHYSICS.gravity, PHYSICS.fall);
  if (rising) {
    const auto row = floor_div(to - 1, SUBCELLS);
    if (map.at({.x = column, .y = row}) == Cell::Blocked) {
      body.position.y = floor_line(row + 1);
      body.fall_speed = 0;
      return;
    }
    body.position.y = to;
    return;
  }
  for (auto row = floor_div(from - 1, SUBCELLS); floor_line(row) <= to; ++row) {
    if (floor_line(row) >= from && stands_on(map, {.x = column, .y = row})) {
      body.position.y = floor_line(row);
      body.stance = Stance::Standing;
      body.fall_speed = 0;
      return;
    }
  }
  body.position.y = to;
}

/// The highest and lowest feet line of the run of ladder through `cell`.
std::pair<std::int32_t, std::int32_t> ladder_span(const CellMap &map,
                                                  GridPosition cell) {
  auto top = cell.y;
  while (ladder_at(map.at({.x = cell.x, .y = top - 1}))) {
    --top;
  }
  auto bottom = cell.y;
  while (ladder_at(map.at({.x = cell.x, .y = bottom + 1}))) {
    ++bottom;
  }
  return {floor_line(top), floor_line(bottom)};
}

/// Whether `direction` takes hold of a ladder from where `body` is: there is
/// ladder that way to climb.
bool grabs(const CellMap &map, const Body &body, Direction direction) {
  const auto cell = cell_of(body.position);
  if (!ladder_at(map.at(cell))) {
    return false;
  }
  const auto [top, bottom] = ladder_span(map, cell);
  return (direction == Direction::Up && body.position.y > top) ||
         (direction == Direction::Down && body.position.y < bottom);
}

/// Up or down a ladder by one tick, inside the run of ladder it is on.
void climb(const CellMap &map, Body &body, Direction direction) {
  const auto cell = cell_of(body.position);
  const auto [top, bottom] = ladder_span(map, cell);
  body.position.x = cell.x * SUBCELLS + SUBCELLS / 2;
  body.stance = Stance::Climbing;
  body.fall_speed = 0;
  if (direction == Direction::Up) {
    body.position.y = std::max(top, body.position.y - PHYSICS.climb);
  } else {
    body.position.y = std::min(bottom, body.position.y + PHYSICS.climb);
  }
}

/// Sub-cell units between a body and the edge of its cell towards
/// `direction`.
std::int32_t edge_room(const Body &body, Direction direction) {
  const auto offset = body.position.x - cell_of(body.position).x * SUBCELLS;
  return direction == Direction::Left ? offset : SUBCELLS - 1 - offset;
}

Direction direction_of(Move move) {
  switch (move) {
  case Move::Left:
  case Move::JumpLeft:
    return Direction::Left;
  case Move::Right:
  case Move::JumpRight:
    return Direction::Right;
  case Move::ClimbUp:
    return Direction::Up;
  case Move::ClimbDown:
    return Direction::Down;
  }
  std::abort();
}

/// Where a body steered by `move` comes to stand in another cell, or nothing
/// when a wall stops it, it falls into the acid, or it never settles.
std::optional<GridPosition> follow(const CellMap &map, Body body, Move move) {
  const auto start = cell_of(body.position);
  for (int tick = 0; tick < LINK_TICKS; ++tick) {
    const auto next = physics_tick(map, body, steer(move, body));
    if (next == body) {
      return std::nullopt;
    }
    body = next;
    const auto cell = cell_of(body.position);
    if (map.below(cell) || map.obstructed(cell)) {
      return std::nullopt;
    }
    if (body.stance == Stance::Standing && cell != start) {
      return cell;
    }
  }
  return std::nullopt;
}

/// Where a steered jump towards `direction` from `from` lands: it leaves from
/// anywhere between the room it waits for and the edge, so both ends must
/// land on the same row.
std::optional<GridPosition> jump_landing(const CellMap &map, GridPosition from,
                                         Move jump) {
  const auto direction = direction_of(jump);
  const auto edge =
      direction == Direction::Left ? from.x * SUBCELLS : floor_line(from.x) - 1;
  const auto inward = direction == Direction::Left ? 1 : -1;
  std::optional<GridPosition> landing;
  for (const auto room : {JUMP_ROOM, 0}) {
    const Body takeoff{
        .position = {.x = edge + inward * room, .y = floor_line(from.y)},
        .heading = direction};
    const auto landed = follow(map, takeoff, jump);
    if (!landed || (landing && landing->y != landed->y)) {
      return std::nullopt;
    }
    landing = landing ? landing : landed;
  }
  return landing;
}

} // namespace

GridPosition cell_of(Position position) {
  return {.x = floor_div(position.x, SUBCELLS),
          .y = floor_div(position.y - 1, SUBCELLS)};
}

Position standing_in(GridPosition cell) {
  return {.x = cell.x * SUBCELLS + SUBCELLS / 2, .y = floor_line(cell.y)};
}

std::int64_t distance(GridPosition position, WorldArea area) {
  const auto right = std::int64_t{area.x} + area.width - 1;
  const auto bottom = std::int64_t{area.y} + area.height - 1;
  return std::max({std::int64_t{area.x} - position.x,
                   std::int64_t{position.x} - right, std::int64_t{0}}) +
         std::max({std::int64_t{area.y} - position.y,
                   std::int64_t{position.y} - bottom, std::int64_t{0}});
}

bool stands_on(const CellMap &map, GridPosition cell) {
  const auto here = map.at(cell);
  return floor_at(here) ||
         (here != Cell::Blocked &&
          map.at({.x = cell.x, .y = cell.y + 1}) == Cell::Blocked);
}

bool holds(const CellMap &map, GridPosition cell) {
  return stands_on(map, cell) || ladder_at(map.at(cell));
}

CellMap::CellMap(const WorldSnapshot &snapshot, std::vector<GridPosition> sites)
    : obstacles(std::move(sites)) {
  if (snapshot.objects.empty()) {
    return;
  }
  auto left = MAP_LIMIT;
  auto top = MAP_LIMIT;
  auto right = -MAP_LIMIT;
  auto bottom = -MAP_LIMIT;
  for (const auto &object : snapshot.objects) {
    const auto &area = object.area;
    const auto far_x = std::int64_t{area.x} + area.width;
    const auto far_y = std::int64_t{area.y} + area.height;
    if (area.x < -MAP_LIMIT || area.y < -MAP_LIMIT || far_x > MAP_LIMIT ||
        far_y > MAP_LIMIT || area.width <= 0 || area.height <= 0) {
      throw std::invalid_argument("a city reaches past the movement limit");
    }
    left = std::min(left, area.x);
    top = std::min(top, area.y);
    right = std::max(right, static_cast<std::int32_t>(far_x));
    bottom = std::max(bottom, static_cast<std::int32_t>(far_y));
  }
  origin = {.x = left, .y = top};
  width = right - left;
  height = bottom - top;
  struct Marks {
    bool floor = false;
    bool ladder = false;
    bool building = false;
    bool open_passage = false;
    bool broken_passage = false;
  };
  std::vector<Marks> marks(static_cast<std::size_t>(width) *
                           static_cast<std::size_t>(height));
  for (const auto &object : snapshot.objects) {
    if (object.state == ObjectState::Dormant) {
      continue;
    }
    for (auto y = object.area.y; y < object.area.y + object.area.height; ++y) {
      for (auto x = object.area.x; x < object.area.x + object.area.width; ++x) {
        auto &mark = marks.at(static_cast<std::size_t>(y - top) *
                                  static_cast<std::size_t>(width) +
                              static_cast<std::size_t>(x - left));
        switch (object.kind) {
        case ObjectKind::Building:
          mark.building = mark.building || object.state == ObjectState::Intact;
          break;
        case ObjectKind::Ladder:
          mark.ladder = mark.ladder || object.traversable;
          mark.open_passage = mark.open_passage || object.traversable;
          break;
        case ObjectKind::Entrance:
          mark.broken_passage = mark.broken_passage || !object.traversable;
          mark.open_passage = mark.open_passage || object.traversable;
          break;
        case ObjectKind::Terrain:
          mark.floor = mark.floor || object.traversable;
          break;
        case ObjectKind::Npc:
        case ObjectKind::MaterialSource:
          break;
        }
      }
    }
  }
  cells.reserve(marks.size());
  for (const auto &mark : marks) {
    if (mark.broken_passage || (mark.building && !mark.open_passage)) {
      cells.push_back(Cell::Blocked);
    } else if (mark.ladder) {
      cells.push_back(mark.floor ? Cell::LadderOnFloor : Cell::Ladder);
    } else {
      cells.push_back(mark.floor ? Cell::Floor : Cell::Open);
    }
  }
}

bool CellMap::below(GridPosition cell) const {
  return std::int64_t{cell.y} >= std::int64_t{origin.y} + height;
}

bool CellMap::obstructed(GridPosition cell) const {
  return std::ranges::find(obstacles, cell) != obstacles.end();
}

Cell CellMap::at(GridPosition cell) const {
  const auto x = std::int64_t{cell.x} - origin.x;
  const auto y = std::int64_t{cell.y} - origin.y;
  if (x < 0 || y < 0 || x >= width || y >= height) {
    return Cell::Open;
  }
  return cells.at(static_cast<std::size_t>(y * width + x));
}

Body physics_tick(const CellMap &map, Body body, ExplorationInput input) {
  const auto direction = input.direction;
  if (body.stance == Stance::Climbing &&
      !ladder_at(map.at(cell_of(body.position)))) {
    body.stance = Stance::Airborne;
    body.fall_speed = 0;
  }
  if (input.jump && body.stance != Stance::Airborne) {
    body.stance = Stance::Airborne;
    body.fall_speed = -PHYSICS.jump;
  } else if ((direction == Direction::Up || direction == Direction::Down) &&
             (body.stance == Stance::Climbing || grabs(map, body, direction))) {
    climb(map, body, direction);
    return body;
  }
  if (body.stance == Stance::Climbing) {
    if (!sideways(direction)) {
      return body;
    }
    // Stepping off a ladder puts the feet on the floor of its cell, if any.
    const auto cell = cell_of(body.position);
    if (stands_on(map, cell)) {
      body.stance = Stance::Standing;
      body.position.y = floor_line(cell.y);
    } else {
      body.stance = Stance::Airborne;
    }
  }
  const bool ran =
      sideways(direction) && input.running && body.stance == Stance::Standing;
  if (sideways(direction)) {
    move_sideways(map, body, direction, ran ? PHYSICS.run : PHYSICS.walk);
  }
  if (body.stance == Stance::Standing &&
      !stands_on(map, cell_of(body.position))) {
    body.stance = Stance::Airborne;
    body.fall_speed = 0;
  }
  if (body.stance == Stance::Airborne) {
    move_in_air(map, body);
  }
  return body;
}

ExplorationInput steer(Move move, const Body &body) {
  if (body.stance == Stance::Airborne) {
    return {.direction = body.heading};
  }
  const auto direction = direction_of(move);
  switch (move) {
  case Move::Left:
  case Move::Right:
  case Move::ClimbUp:
  case Move::ClimbDown:
    return {.direction = direction};
  case Move::JumpLeft:
  case Move::JumpRight:
    return {.direction = direction,
            .jump = body.stance == Stance::Standing &&
                    edge_room(body, direction) <= JUMP_ROOM};
  }
  std::abort();
}

std::vector<Link> links(const CellMap &map, GridPosition from) {
  std::vector<Link> found;
  if (!holds(map, from)) {
    return found;
  }
  const bool on_floor = stands_on(map, from);
  const Body standing{.position = standing_in(from),
                      .stance = on_floor ? Stance::Standing : Stance::Climbing};
  for (const auto &[walk, jump] : {std::pair{Move::Left, Move::JumpLeft},
                                   std::pair{Move::Right, Move::JumpRight}}) {
    const auto walked = follow(map, standing, walk);
    if (walked) {
      found.push_back({.to = *walked, .move = walk});
    }
    // A jump is only a way where walking does not reach the next column.
    const bool next_door = walked && std::abs(walked->x - from.x) == 1 &&
                           std::abs(walked->y - from.y) <= 1;
    if (next_door || !on_floor) {
      continue;
    }
    if (const auto landed = jump_landing(map, from, jump)) {
      found.push_back({.to = *landed, .move = jump});
    }
  }
  if (ladder_at(map.at(from))) {
    for (const auto &[move, row] : {std::pair{Move::ClimbUp, from.y - 1},
                                    std::pair{Move::ClimbDown, from.y + 1}}) {
      const GridPosition next{.x = from.x, .y = row};
      if (ladder_at(map.at(next))) {
        found.push_back({.to = next, .move = move});
      }
    }
  }
  return found;
}

std::vector<GridPosition> reachable_cells(const CellMap &map,
                                          GridPosition from) {
  std::vector<GridPosition> seen{from};
  std::deque<GridPosition> frontier{from};
  while (!frontier.empty()) {
    const auto cell = frontier.front();
    frontier.pop_front();
    for (const auto &link : links(map, cell)) {
      if (std::ranges::find(seen, link.to) == seen.end()) {
        seen.push_back(link.to);
        frontier.push_back(link.to);
      }
    }
  }
  return seen;
}

Exploration::Exploration(Simulation &progression, World &city,
                         ExplorationConfig settings)
    : simulation(progression), world(city), config(std::move(settings)) {
  const auto snapshot = world.get().snapshot();
  if (config.collection_time < 0us ||
      !stands_on(CellMap(on_map(snapshot, MapId::Street)), config.entry)) {
    throw std::invalid_argument("exploration requires a safe entry");
  }
  for (const auto &exit : config.exits) {
    const auto arrival = CellMap(on_map(snapshot, exit.to));
    if (exit.from == exit.to || !stands_on(arrival, exit.arrival) ||
        std::ranges::any_of(config.exits, [&](const MapExit &other) {
          return other.from == exit.to &&
                 distance(exit.arrival, other.zone) == 0;
        })) {
      throw std::invalid_argument("a map exit requires a safe arrival");
    }
  }
  if ((!config.obstacles.empty() || !config.patrols.empty()) &&
      config.absence <= 0us) {
    throw std::invalid_argument(
        "obstacles and patrols need a positive absence");
  }
  for (const auto &patrol : config.patrols) {
    if (patrol.from.y != patrol.to.y || patrol.from.x >= patrol.to.x) {
      throw std::invalid_argument("a patrol runs left to right along one row");
    }
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
  const auto other = actor == Actor::Chef ? Actor::Appraiser : Actor::Chef;
  if (current != MapId::Street &&
      actor_state(simulation.get().snapshot(), other).location ==
          Location::City) {
    return Result::NotExploring;
  }
  current = MapId::Street;
  if (!stands_on(cells(), config.entry)) {
    return Result::NotExploring;
  }
  const auto result = simulation.get().enter(actor);
  if (result == Result::Applied) {
    explorer(actor) = {
        .state = {.body = {.position = standing_in(config.entry)}}};
  }
  return result;
}

Result Exploration::return_to_base(Actor actor) {
  if (collecting(actor)) {
    return Result::Busy;
  }
  const auto result = simulation.get().return_to_base(actor);
  if (result == Result::Applied) {
    synchronize();
  }
  return result;
}

Result Exploration::advance(std::chrono::microseconds elapsed,
                            const std::array<ExplorationInput, 2> &inputs) {
  if (run_over(simulation.get().snapshot().phase)) {
    return Result::RunOver;
  }
  if (elapsed < 0us) {
    return Result::InvalidElapsed;
  }
  synchronize();
  auto remaining = elapsed;
  // Apply a finished day even with no elapsed time, including while at base.
  advance_world();
  while (remaining > 0us &&
         simulation.get().snapshot().phase == Phase::Exploration) {
    const auto before = simulation.get().snapshot();
    auto step = std::min(remaining, before.remaining_exploration);
    for (const auto actor : ACTORS) {
      if (actor_state(before, actor).location == Location::City) {
        step = std::min(step, MOVEMENT_TICK - explorer(actor).remainder);
      }
    }
    step = std::min(step, MOVEMENT_TICK - mutant_clock);
    require_applied(simulation.get().advance(step));
    advance_world();
    remaining -= step;
    mutant_clock += step;
    for (const auto actor : ACTORS) {
      if (actor_state(before, actor).location == Location::City) {
        auto &state = explorer(actor);
        state.remainder += step;
        if (auto &held = state.state.collecting) {
          held->remaining -= std::min(step, held->remaining);
          if (held->remaining == 0us) {
            held.reset();
          }
        }
        if (auto &gone = state.state.absent) {
          *gone -= std::min(step, *gone);
        }
      }
    }
    synchronize();
    release_absent();
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
    if (mutant_clock == MOVEMENT_TICK) {
      mutant_clock = 0us;
      walk_mutants();
    }
    travel();
  }
  return Result::Applied;
}

CollectionResult Exploration::collect(Actor actor, ObjectId source) {
  if (actor_state(simulation.get().snapshot(), actor).location !=
          Location::City ||
      explorer(actor).state.absent) {
    return CollectionResult::NotExploring;
  }
  if (collecting(actor)) {
    return CollectionResult::Busy;
  }
  const auto stock =
      std::ranges::find_if(config.sources, [source](const SourceStock &entry) {
        return entry.id == source;
      });
  const auto snapshot = world.get().snapshot();
  const auto object = std::ranges::find_if(
      snapshot.objects,
      [source](const ObjectSnapshot &entry) { return entry.id == source; });
  if (object != snapshot.objects.end() && object->map != current) {
    return CollectionResult::OutOfRange;
  }
  if (stock == config.sources.end() || object == snapshot.objects.end() ||
      object->kind != ObjectKind::MaterialSource) {
    return CollectionResult::UnknownSource;
  }
  if (!object->material_available) {
    return CollectionResult::Unavailable;
  }
  if (distance(cell_of(explorer(actor).state.body.position), object->area) >
      config.collection_distance) {
    return CollectionResult::OutOfRange;
  }
  if (stock->remaining == 0) {
    return CollectionResult::Depleted;
  }
  if (!object->ingredient) {
    return CollectionResult::UnknownSource;
  }
  require_applied(simulation.get().collect(actor, *object->ingredient));
  --stock->remaining;
  if (config.collection_time > 0us) {
    explorer(actor).state.collecting =
        Collecting{.source = source, .remaining = config.collection_time};
  }
  return CollectionResult::Applied;
}

ExplorationSnapshot Exploration::snapshot() const {
  ExplorationSnapshot result{.map = current,
                             .chef = chef.state,
                             .appraiser = appraiser.state,
                             .sources = config.sources,
                             .obstacles = obstacles_on(current),
                             .mutants = {}};
  for (const auto &mutant : mutants) {
    if (mutant.patrol.map == current) {
      result.mutants.push_back(mutant.state);
    }
  }
  if (current == MapId::Street) {
    for (auto *state : {&result.chef, &result.appraiser}) {
      const auto cell = cell_of(state->body.position);
      const auto inside = std::ranges::find_if(
          config.interiors, [cell](const Interior &interior) {
            return distance(cell, interior.area) == 0;
          });
      if (inside != config.interiors.end()) {
        state->interior = inside->building;
      }
    }
  }
  return result;
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

bool Exploration::collecting(Actor actor) {
  return explorer(actor).state.collecting.has_value();
}

void Exploration::synchronize() {
  // An actor left absent with its partner at base has no one to return to.
  for (const auto actor : ACTORS) {
    const auto partner = actor == Actor::Chef ? Actor::Appraiser : Actor::Chef;
    const auto snapshot = simulation.get().snapshot();
    if (explorer(actor).state.absent &&
        actor_state(snapshot, actor).location == Location::City &&
        actor_state(snapshot, partner).location == Location::Base) {
      require_applied(simulation.get().return_to_base(actor));
    }
  }
  const auto snapshot = simulation.get().snapshot();
  if (actor_state(snapshot, Actor::Chef).location == Location::Base &&
      actor_state(snapshot, Actor::Appraiser).location == Location::Base) {
    current = MapId::Street;
    mutant_clock = 0us;
    mutants.clear();
    for (const auto &patrol : config.patrols) {
      mutants.push_back(
          {.patrol = patrol, .state = {.position = standing_in(patrol.from)}});
    }
  }
  for (const auto actor : ACTORS) {
    if (actor_state(snapshot, actor).location == Location::Base) {
      explorer(actor) = {
          .state = {.body = {.position = standing_in(config.base)}}};
    }
  }
}

void Exploration::advance_world() {
  if (world.get().update(simulation.get().snapshot().day) !=
      WorldUpdateResult::Applied) {
    throw std::logic_error("exploration day regressed");
  }
}

std::vector<GridPosition> Exploration::obstacles_on(MapId map) const {
  std::vector<GridPosition> cells;
  for (const auto &obstacle : config.obstacles) {
    if (obstacle.map == map) {
      cells.push_back(obstacle.cell);
    }
  }
  return cells;
}

CellMap Exploration::cells() const {
  return CellMap(on_map(world.get().snapshot(), current),
                 obstacles_on(current));
}

void Exploration::travel() {
  const auto snapshot = simulation.get().snapshot();
  if (actor_state(snapshot, Actor::Chef).location != Location::City ||
      actor_state(snapshot, Actor::Appraiser).location != Location::City ||
      chef.state.absent || appraiser.state.absent) {
    return;
  }
  const auto inside = [this](const MapExit &exit) {
    return exit.from == current &&
           distance(cell_of(chef.state.body.position), exit.zone) == 0 &&
           distance(cell_of(appraiser.state.body.position), exit.zone) == 0;
  };
  const auto found = std::ranges::find_if(config.exits, inside);
  if (found == config.exits.end()) {
    return;
  }
  current = found->to;
  for (auto *state : {&chef.state, &appraiser.state}) {
    state->body = {.position = standing_in(found->arrival)};
  }
}

bool Exploration::touched(const CellMap &map, const Body &body) const {
  const auto cell = cell_of(body.position);
  return map.obstructed(cell) ||
         std::ranges::any_of(mutants, [&](const Mutant &mutant) {
           return mutant.patrol.map == current &&
                  cell_of(mutant.state.position) == cell;
         });
}

void Exploration::send_away(Actor actor) {
  const auto partner = actor == Actor::Chef ? Actor::Appraiser : Actor::Chef;
  const auto elsewhere =
      actor_state(simulation.get().snapshot(), partner).location !=
          Location::City ||
      explorer(partner).state.absent;
  if (elsewhere) {
    for (const auto leaving : {actor, partner}) {
      if (actor_state(simulation.get().snapshot(), leaving).location ==
          Location::City) {
        require_applied(simulation.get().return_to_base(leaving));
      }
    }
    synchronize();
    return;
  }
  auto &state = explorer(actor).state;
  state.collecting.reset();
  state.absent = config.absence;
}

void Exploration::release_absent() {
  for (const auto actor : ACTORS) {
    auto &state = explorer(actor).state;
    if (state.absent == 0us) {
      const auto partner =
          actor == Actor::Chef ? Actor::Appraiser : Actor::Chef;
      state.body = explorer(partner).state.body;
      state.absent.reset();
    }
  }
}

void Exploration::walk_mutants() {
  for (auto &mutant : mutants) {
    const auto left = standing_in(mutant.patrol.from).x;
    const auto right = standing_in(mutant.patrol.to).x;
    auto &state = mutant.state;
    state.position.x +=
        state.heading == Direction::Right ? MUTANT_STEP : -MUTANT_STEP;
    if (state.position.x >= right) {
      state.position.x = right;
      state.heading = Direction::Left;
    } else if (state.position.x <= left) {
      state.position.x = left;
      state.heading = Direction::Right;
    }
  }
  const auto map = cells();
  for (const auto actor : ACTORS) {
    const auto &state = explorer(actor).state;
    if (actor_state(simulation.get().snapshot(), actor).location ==
            Location::City &&
        !state.absent && touched(map, state.body)) {
      send_away(actor);
    }
  }
}

void Exploration::tick(Actor actor, ExplorationInput input) {
  auto &state = explorer(actor).state;
  if (state.absent) {
    return;
  }
  if (state.collecting) {
    input = {};
  }
  const auto map = cells();
  const auto partner = actor == Actor::Chef ? Actor::Appraiser : Actor::Chef;
  const bool together =
      actor_state(simulation.get().snapshot(), partner).location ==
          Location::City &&
      !explorer(partner).state.absent;
  const auto partner_x = explorer(partner).state.body.position.x;
  for (auto count = 0; count < TICKS_PER_UNIT; ++count) {
    auto next = physics_tick(map, state.body, input);
    const auto gap = std::abs(next.position.x - partner_x);
    if (together && gap > SCREEN_LOCK &&
        gap > std::abs(state.body.position.x - partner_x)) {
      next = physics_tick(map, state.body, {.jump = input.jump});
    }
    state.body = next;
    if (map.below(cell_of(state.body.position))) {
      require_applied(simulation.get().return_to_base(actor));
      synchronize();
      return;
    }
    if (touched(map, state.body)) {
      send_away(actor);
      return;
    }
  }
}

} // namespace neocity::survival
