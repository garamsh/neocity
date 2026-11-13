#pragma once

#include <survival/world.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
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

/// Grid steps from a cell to the nearest cell of an area; zero inside it.
[[nodiscard]] std::int64_t distance(GridPosition position, WorldArea area);

/// What one cell means to movement. A floor is a one-way walkway: an actor
/// stands on it and jumps up through it. A ladder is climbed.
enum class Cell : std::uint8_t { Open, Floor, Ladder, LadderOnFloor, Blocked };

/// Every cell's meaning, read once from a world snapshot. Intact terrain is
/// floor and an intact ladder is climbed. An intact building blocks unless an
/// intact entrance or ladder opens the cell, and a broken entrance blocks
/// first, so the world's object order does not change the result. A broken
/// ladder only stops being climbed.
class CellMap {
public:
  /// Throws std::invalid_argument for an object reaching past `MAP_LIMIT`
  /// cells from the origin, beyond which sub-cell positions cannot follow.
  explicit CellMap(const WorldSnapshot &snapshot);
  /// Outside every object is open air.
  [[nodiscard]] Cell at(GridPosition cell) const;
  /// Whether `cell` lies below every object: a body there has fallen into
  /// the acid.
  [[nodiscard]] bool below(GridPosition cell) const;

private:
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
/// route planning.
[[nodiscard]] Body physics_tick(const CellMap &map, Body body,
                                ExplorationInput input);

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
/// one that falls into the acid is not.
[[nodiscard]] std::vector<Link> links(const CellMap &map, GridPosition from);

/// Every held cell reachable from `from` by links, `from` included.
[[nodiscard]] std::vector<GridPosition> reachable_cells(const CellMap &map,
                                                        GridPosition from);

/// The input that carries `body` along `move`: a jump waits until the
/// body is near enough the edge, and a body in the air keeps its heading.
[[nodiscard]] ExplorationInput steer(Move move, const Body &body);

/// Presentation categories share the world's MaterialSource identity.
enum class SourceKind : std::uint8_t { VendingMachine, Npc, Shop, Bag };

/// Caller-supplied finite stock, shared by both actors for the entire run.
struct SourceStock {
  ObjectId id;
  SourceKind kind = SourceKind::Bag;
  std::uint32_t remaining = 1;
  friend bool operator==(const SourceStock &, const SourceStock &) = default;
};

/// Temporary tuning. `collection_time` is how long one collection holds the
/// collecting actor.
struct ExplorationConfig {
  GridPosition base;
  GridPosition entry;
  std::uint32_t collection_distance = 1;
  std::vector<SourceStock> sources;
  std::chrono::microseconds collection_time{};
};

/// A collection under way: the actor already holds the item and takes no
/// command and no movement until `remaining` exploration time has passed.
struct Collecting {
  ObjectId source;
  std::chrono::microseconds remaining{};
  friend bool operator==(const Collecting &, const Collecting &) = default;
};

/// Detailed body and collection hold; base has no movement.
struct ExplorerSnapshot {
  Body body;
  std::optional<Collecting> collecting = std::nullopt;
  friend bool operator==(const ExplorerSnapshot &,
                         const ExplorerSnapshot &) = default;
};

/// Read-only values for a renderer or companion controller.
struct ExplorationSnapshot {
  ExplorerSnapshot chef;
  ExplorerSnapshot appraiser;
  std::vector<SourceStock> sources;
  friend bool operator==(const ExplorationSnapshot &,
                         const ExplorationSnapshot &) = default;
};

/// Collection refusals leave inventories, stock and progression unchanged.
enum class CollectionResult : std::uint8_t {
  Applied,
  NotExploring,
  UnknownSource,
  Unavailable,
  OutOfRange,
  Depleted,
  Busy
};

/// Coordinates existing progression/world commands; owns neither dependency.
/// Route actor entry, return and time through this instance during play.
class Exploration {
public:
  /// Throws std::invalid_argument for invalid stock, tuning or an unsafe entry.
  Exploration(Simulation &progression, World &city, ExplorationConfig settings);
  /// Starts at the configured entry with no partial movement.
  [[nodiscard]] Result enter(Actor actor);
  /// Synchronizes the detailed base position while preserving run state;
  /// refused with `Result::Busy` while the actor is collecting.
  [[nodiscard]] Result return_to_base(Actor actor);
  /// Consumes shared time once, immediately; movement retains sub-tick time.
  /// Each 100ms movement unit runs its physics ticks under one input.
  /// The world follows the run's clock here only, at the acid the caller
  /// passes.
  /// Answers `Result::RunOver` with no change once the run is over.
  [[nodiscard]] Result advance(std::chrono::microseconds elapsed,
                               const std::array<ExplorationInput, 2> &inputs,
                               AcidLevel acid);
  /// Checks identity, supply, distance and shared stock before recording the
  /// source's ingredient, then holds the actor for `collection_time` while the
  /// shared clock runs on. Expiry or a fall ends the hold, and the item is
  /// kept.
  [[nodiscard]] CollectionResult collect(Actor actor, ObjectId source);
  /// Returns independent values; requests are never retained in this snapshot.
  [[nodiscard]] ExplorationSnapshot snapshot() const;

private:
  struct Explorer {
    ExplorerSnapshot state;
    std::chrono::microseconds remainder{};
  };

  Explorer &explorer(Actor actor);
  [[nodiscard]] bool collecting(Actor actor);
  void synchronize();
  void tick(Actor actor, ExplorationInput input);
  void advance_world(AcidLevel acid);

  std::reference_wrapper<Simulation> simulation;
  std::reference_wrapper<World> world;
  ExplorationConfig config;
  Explorer chef;
  Explorer appraiser;
};

} // namespace neocity::survival
