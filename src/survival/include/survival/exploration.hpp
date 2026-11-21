#pragma once

#include <survival/world.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace neocity::survival {

/// Map cell coordinates; x grows to the right and y downwards.
struct GridPosition {
  std::int32_t x = 0;
  std::int32_t y = 0;
  friend bool operator==(const GridPosition &, const GridPosition &) = default;
};

/// Sub-cell units across one cell, in either axis.
inline constexpr std::int32_t SUBCELLS = 32;

/// A point in sub-cell units: an actor's x is its centre line and its y the
/// line its feet are on.
struct Position {
  std::int32_t x = 0;
  std::int32_t y = 0;
  friend bool operator==(const Position &, const Position &) = default;
};

/// The cell an actor at `position` is in: the one its feet stand in.
[[nodiscard]] GridPosition cell_of(Position position);
/// Feet on the floor of `cell`, at its centre.
[[nodiscard]] Position standing_in(GridPosition cell);

/// How far from the origin, in cells, a city that actors move in may reach.
inline constexpr std::int32_t MAP_LIMIT = 1024;

/// The widest gap in x, in sub-cell units, between two actors that are both in
/// the city. A movement that would widen the gap past it is stopped, so one
/// screen at the close camera always holds both. A temporary value: twelve
/// cells of the twenty the 1920x1080 canvas shows at that camera, leaving four
/// cells at each side.
inline constexpr std::int32_t SCREEN_LOCK = 12 * SUBCELLS;

/// Grid steps from a cell to the nearest cell of an area; zero inside it.
[[nodiscard]] std::int64_t distance(GridPosition position, WorldArea area);

/// What one cell means to movement. A floor is a one-way walkway: an actor
/// stands on it and jumps up through it. A ladder is climbed.
enum class Cell : std::uint8_t { Open, Floor, Ladder, LadderOnFloor, Blocked };

/// Every cell's meaning, read once from a world snapshot. Intact terrain is
/// floor and an intact ladder is climbed. An intact building blocks unless an
/// intact entrance or ladder opens the cell, and a broken entrance blocks
/// first, so the world's object order does not change the result. A broken
/// ladder only stops being climbed. Obstacles are cells a body must not enter:
/// a route never passes through one.
class CellMap {
public:
  /// Throws std::invalid_argument for an object reaching past `MAP_LIMIT`
  /// cells from the origin, beyond which sub-cell positions cannot follow.
  explicit CellMap(const WorldSnapshot &snapshot,
                   std::vector<GridPosition> sites = {});
  /// Outside every object is open air.
  [[nodiscard]] Cell at(GridPosition cell) const;
  /// Whether `cell` lies below every object: a body there has fallen into
  /// the acid.
  [[nodiscard]] bool below(GridPosition cell) const;
  /// Whether an obstacle stands in `cell`.
  [[nodiscard]] bool obstructed(GridPosition cell) const;

private:
  std::vector<GridPosition> obstacles;
  GridPosition origin;
  std::int32_t width = 0;
  std::int32_t height = 0;
  std::vector<Cell> cells;
};

/// Whether feet can rest in `cell`: on its floor, or on top of a blocked cell
/// below it, so a low heap of rubble is a hurdle to jump onto.
[[nodiscard]] bool stands_on(const CellMap &map, GridPosition cell);
/// Whether an actor can be in `cell` without falling: standing or on a ladder.
[[nodiscard]] bool holds(const CellMap &map, GridPosition cell);

/// The same movement choices apply to both actors. Up and down climb.
enum class Direction : std::uint8_t { Idle, Left, Right, Up, Down };

/// How an actor is held up.
enum class Stance : std::uint8_t { Standing, Airborne, Climbing };

/// Temporary tuning of the side view, in sub-cell units per physics tick.
/// Running is faster on the ground only, so a jump's reach does not grow with
/// it: a jump clears a two-cell gap at its own height, or a one-cell gap one
/// row up, and rises less than two rows.
struct Physics {
  std::chrono::microseconds tick{};
  std::int32_t walk = 0;
  std::int32_t run = 0;
  std::int32_t climb = 0;
  std::int32_t jump = 0;
  std::int32_t gravity = 0;
  std::int32_t fall = 0;
};

inline constexpr Physics PHYSICS{.tick = std::chrono::milliseconds{20},
                                 .walk = 5,
                                 .run = 10,
                                 .climb = 4,
                                 .jump = 9,
                                 .gravity = 1,
                                 .fall = 12};

/// One actor's place and motion between physics ticks.
struct Body {
  Position position;
  Stance stance = Stance::Standing;
  /// Downward speed; negative while rising.
  std::int32_t fall_speed = 0;
  /// The way it last moved sideways: left or right.
  Direction heading = Direction::Right;
  friend bool operator==(const Body &, const Body &) = default;
};

/// Current input only; no movement or collection requests are queued.
/// `jump` is a press, tried on every physics tick of one movement unit until
/// it takes. Running has no limit.
struct ExplorationInput {
  Direction direction = Direction::Idle;
  bool running = false;
  bool jump = false;
};

/// One physics tick of `body` under `input`, the rule behind both movement and
/// route planning. A walk or run covers `speed_percent` of its usual
/// sub-cells, rounded down.
[[nodiscard]] Body physics_tick(const CellMap &map, Body body,
                                ExplorationInput input,
                                std::int32_t speed_percent = 100);

/// One way out of a cell an actor holds, and the move that takes it.
enum class Move : std::uint8_t {
  Left,
  Right,
  ClimbUp,
  ClimbDown,
  JumpLeft,
  JumpRight
};

/// A move and the cell it ends in.
struct Link {
  GridPosition to;
  Move move = Move::Left;
  friend bool operator==(const Link &, const Link &) = default;
};

/// Every way out of `from` that ends held, worked out by `physics_tick` for a
/// body that follows `steer`. Walking that drops to a lower floor is a link;
/// one that falls into the acid or enters an obstacle's cell is not.
[[nodiscard]] std::vector<Link> links(const CellMap &map, GridPosition from);

/// Every held cell reachable from `from` by links, `from` included.
[[nodiscard]] std::vector<GridPosition> reachable_cells(const CellMap &map,
                                                        GridPosition from);

/// The input that carries `body` along `move`: a jump waits until the
/// body is near enough the edge, and a body in the air keeps its heading.
[[nodiscard]] ExplorationInput steer(Move move, const Body &body);

/// Presentation categories share the world's MaterialSource identity. A bag is
/// a container that the tide brings and takes.
enum class SourceKind : std::uint8_t { Crates, Locker, Shelf, Bag };

/// A search container, shared by both actors for the entire run: `capacity`
/// slots, of which `items` fill one each. What is left in it stays in it.
struct SourceStock {
  ObjectId id;
  SourceKind kind = SourceKind::Crates;
  std::uint32_t capacity = 1;
  std::vector<IngredientId> items;
  friend bool operator==(const SourceStock &, const SourceStock &) = default;
};

/// What one ingredient weighs; an ingredient with no entry weighs nothing.
struct IngredientWeight {
  IngredientId ingredient;
  std::uint32_t weight = 0;
};

/// What an actor can carry before it slows. Left out, nothing slows an actor.
struct CarryRules {
  std::vector<IngredientWeight> weights;
  std::uint32_t max_weight = std::numeric_limits<std::uint32_t>::max();
  /// The share of its walking and running speed an actor over `max_weight`
  /// keeps, in percent.
  std::int32_t overweight_percent = 100;
};

/// The weight of one `ingredient`.
[[nodiscard]] std::uint32_t weight_of(const CarryRules &rules,
                                      IngredientId ingredient);
/// The weight of every ingredient in `items`.
[[nodiscard]] std::uint32_t carried_weight(const CarryRules &rules,
                                           std::span<const IngredientId> items);
/// Whether `items` weigh more than `rules.max_weight`.
[[nodiscard]] bool overweight(const CarryRules &rules,
                              std::span<const IngredientId> items);

/// A zone on one map where both actors together move to another map. The two
/// stand at `arrival` of the other map, which lies outside every zone there.
struct MapExit {
  MapId from = MapId::Street;
  WorldArea zone;
  MapId to = MapId::Aquarium;
  GridPosition arrival;
};

/// A named floor or roof that must remain reachable on every damage stage.
struct InteriorFloor {
  std::string_view name;
  WorldArea area;
};

/// A building's occupied zone and the floors and roof its routes must reach.
struct Interior {
  ObjectId building;
  WorldArea area;
  MapId map = MapId::Street;
  std::vector<InteriorFloor> floors;
};

/// A fixed obstacle: the cell of a map whose touch sends an actor away.
struct Obstacle {
  MapId map = MapId::Street;
  GridPosition cell;
};

/// A mutant's patrol: it walks one row of a map back and forth between the
/// cells `from` and `to`, which share a row, `from` on the left.
struct Patrol {
  MapId map = MapId::Street;
  GridPosition from;
  GridPosition to;
};

/// Temporary tuning. `carry` slows an actor that carries too much. `entry` is
/// the base entrance, a cell of the street. `absence` is how long an actor that
/// touched a mutant or an obstacle is gone from the map before it returns
/// beside its partner; it is positive whenever the config has patrols or
/// obstacles.
struct ExplorationConfig {
  GridPosition base;
  GridPosition entry;
  std::vector<MapExit> exits;
  std::vector<Interior> interiors;
  std::uint32_t collection_distance = 1;
  std::vector<SourceStock> sources;
  CarryRules carry = {};
  std::vector<Obstacle> obstacles;
  std::vector<Patrol> patrols;
  std::chrono::microseconds absence{};
};

/// A search under way: the actor has `source` open and takes no movement until
/// it closes the search.
struct Searching {
  ObjectId source;
  friend bool operator==(const Searching &, const Searching &) = default;
};

/// Detailed body and search; base has no movement. `interior` is the
/// building whose zone the body stands in, if any. `absent` is the exploration
/// time left before an actor that touched a mutant or an obstacle returns: it
/// is in the city but not on the map, and its body is where it was touched.
struct ExplorerSnapshot {
  Body body;
  std::optional<Searching> searching = std::nullopt;
  std::optional<ObjectId> interior = std::nullopt;
  std::optional<std::chrono::microseconds> absent = std::nullopt;
  friend bool operator==(const ExplorerSnapshot &,
                         const ExplorerSnapshot &) = default;
};

/// Where one mutant stands on its patrol and the way it walks.
struct MutantSnapshot {
  Position position;
  Direction heading = Direction::Right;
  friend bool operator==(const MutantSnapshot &,
                         const MutantSnapshot &) = default;
};

/// Temporary: sub-cells a mutant walks in one movement unit, a third of a
/// walking actor's.
inline constexpr std::int32_t MUTANT_STEP = 8;

/// The mutant one movement unit on: it walks `MUTANT_STEP` sub-cells along
/// `patrol` and turns round on the centre of an end cell.
[[nodiscard]] MutantSnapshot patrol_step(const Patrol &patrol,
                                         MutantSnapshot mutant);

/// Read-only values for a renderer or companion controller. `map` is the map
/// both actors share; the street whenever nobody is out. `obstacles` and
/// `mutants` are those of that map. `touches` counts, from the start of the
/// run, each actor's touches of a mutant or an obstacle, chef first.
struct ExplorationSnapshot {
  MapId map = MapId::Street;
  ExplorerSnapshot chef;
  ExplorerSnapshot appraiser;
  std::vector<SourceStock> sources;
  std::vector<GridPosition> obstacles;
  std::vector<MutantSnapshot> mutants;
  std::array<std::uint32_t, 2> touches{};
  friend bool operator==(const ExplorationSnapshot &,
                         const ExplorationSnapshot &) = default;
};

/// Search refusals leave inventories, containers and progression unchanged.
/// `Occupied`: the other actor has the container open. `Busy`: this actor
/// already searches. `NotSearching`: the container is not open for this actor.
enum class CollectionResult : std::uint8_t {
  Applied,
  NotExploring,
  UnknownSource,
  Unavailable,
  OutOfRange,
  Occupied,
  Busy,
  NotSearching,
  MissingItem,
  InventoryFull,
  ContainerFull
};

/// Coordinates existing progression/world commands; owns neither dependency.
/// Route actor entry, return and time through this instance during play.
class Exploration {
public:
  /// Throws std::invalid_argument for invalid stock, tuning or an unsafe entry.
  Exploration(Simulation &progression, World &city, ExplorationConfig settings);
  /// Starts at the configured entry with no partial movement. Refused with
  /// `Result::NotExploring` while the other actor is out on another map, so
  /// the two are never on different maps.
  [[nodiscard]] Result enter(Actor actor);
  /// Synchronizes the detailed base position while preserving run state;
  /// refused with `Result::Busy` while the actor is searching.
  [[nodiscard]] Result return_to_base(Actor actor);
  /// Consumes shared time once, immediately; movement retains sub-tick time.
  /// Each 100ms movement unit runs its physics ticks under one input. While
  /// both actors are in the city, a tick that would widen the gap in x between
  /// them past `SCREEN_LOCK` leaves its actor's sideways position as it was;
  /// an absent actor holds nobody to the lock. Mutants walk their patrols one
  /// step of each movement unit on the shared clock. An actor that then stands
  /// in the cell of a mutant or an obstacle is absent for `absence`, and
  /// returns to its partner's body, or to base if the partner is not in the
  /// city. A touch while the partner is absent, or with the partner not in the
  /// city, sends every actor in the city to base.
  /// The world follows the run's day here only. When both actors stand in an
  /// exit zone of their map after a movement unit, both move to the other map.
  /// Answers `Result::RunOver` with no change once the run is over.
  [[nodiscard]] Result advance(std::chrono::microseconds elapsed,
                               const std::array<ExplorationInput, 2> &inputs);
  /// Checks identity, map, supply, distance and that the other actor does not
  /// have it open, then opens the container for `actor`, who stays in place
  /// while the shared clock runs on until `close_search`. Returning to base
  /// is refused while searching; a touch or the end of the day ends it.
  [[nodiscard]] CollectionResult search(Actor actor, ObjectId source);
  /// Closes the container `actor` has open.
  [[nodiscard]] CollectionResult close_search(Actor actor);
  /// Moves one `ingredient` from the open container to the actor's inventory:
  /// `InventoryFull` with no free slot. The container keeps what is left.
  [[nodiscard]] CollectionResult take(Actor actor, ObjectId source,
                                      IngredientId ingredient);
  /// Moves one `ingredient` from the actor's inventory back into the open
  /// container: `ContainerFull` with no free slot.
  [[nodiscard]] CollectionResult put(Actor actor, ObjectId source,
                                     IngredientId ingredient);
  /// Returns independent values; requests are never retained in this snapshot.
  [[nodiscard]] ExplorationSnapshot snapshot() const;

private:
  friend class DaySave;
  struct Explorer {
    ExplorerSnapshot state;
    std::chrono::microseconds remainder{};
  };

  Explorer &explorer(Actor actor);
  [[nodiscard]] bool searching(Actor actor);
  [[nodiscard]] SourceStock *open_container(Actor actor, ObjectId source);
  void synchronize();
  void tick(Actor actor, ExplorationInput input);
  [[nodiscard]] bool touched(const CellMap &map, const Body &body) const;
  void send_away(Actor actor);
  void release_absent();
  void walk_mutants();
  [[nodiscard]] std::vector<GridPosition> obstacles_on(MapId map) const;
  void advance_world();
  [[nodiscard]] CellMap cells() const;
  void travel();

  std::reference_wrapper<Simulation> simulation;
  std::reference_wrapper<World> world;
  ExplorationConfig config;
  std::array<std::uint32_t, 2> touch_counts{};
  struct Mutant {
    Patrol patrol;
    MutantSnapshot state;
  };

  Explorer chef;
  Explorer appraiser;
  std::vector<Mutant> mutants;
  std::chrono::microseconds mutant_clock{};
  MapId current = MapId::Street;
};

} // namespace neocity::survival
