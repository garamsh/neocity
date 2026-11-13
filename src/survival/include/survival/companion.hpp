#pragma once

#include <survival/exploration.hpp>
#include <survival/workshop.hpp>

#include <cstddef>
#include <optional>
#include <variant>

namespace neocity::survival {

/// Who drives one actor. The rules treat both the same.
enum class Controller : std::uint8_t { Human, Ai };

/// One controller per actor; every actor stays in play under either.
struct ControlMap {
  Controller chef = Controller::Human;
  Controller appraiser = Controller::Human;
  friend bool operator==(const ControlMap &, const ControlMap &) = default;
};

/// One-player play: the chosen actor is human and the other is AI.
[[nodiscard]] ControlMap single_player(Actor chosen);
/// Two-player play: both actors are human.
[[nodiscard]] ControlMap two_players();
/// Takes over the other actor: controllers trade places, so two humans keep
/// both.
[[nodiscard]] ControlMap switch_control(ControlMap control);
/// A human leaves locally: AI continues that actor from its current state.
[[nodiscard]] ControlMap leave(ControlMap control, Actor actor);
/// The local key set driving an actor, or nothing under AI. A lone human holds
/// the first set on either actor; two humans hold one set per actor.
[[nodiscard]] std::optional<std::size_t> key_set(ControlMap control,
                                                 Actor actor);

/// Everything a controller may read; holds no game object. `world` holds the
/// objects of the map the party is on (`exploration.map`), so a `CellMap` built
/// from it never mixes the cells of two maps.
struct GameView {
  Snapshot progression;
  /// The objects on the current map.
  WorldSnapshot world;
  /// The objects of every map, which a place's name reads.
  WorldSnapshot city;
  ExplorationSnapshot exploration;
  WorkshopSnapshot workshop;
};

struct Enter {
  friend bool operator==(const Enter &, const Enter &) = default;
};
struct ReturnToBase {
  friend bool operator==(const ReturnToBase &, const ReturnToBase &) = default;
};
struct Collect {
  ObjectId source;
  friend bool operator==(const Collect &, const Collect &) = default;
};
struct Appraise {
  IngredientId ingredient;
  friend bool operator==(const Appraise &, const Appraise &) = default;
};
struct Cook {
  FoodId food;
  friend bool operator==(const Cook &, const Cook &) = default;
};
struct Feed {
  FoodId food;
  friend bool operator==(const Feed &, const Feed &) = default;
};
struct EndDayUnfed {
  AcidLevel penalty;
  friend bool operator==(const EndDayUnfed &, const EndDayUnfed &) = default;
};

/// One discrete feature command, the same for a human and for AI.
using Action = std::variant<Enter, ReturnToBase, Collect, Appraise, Cook, Feed,
                            EndDayUnfed>;

/// The outcome the feature returned; a refusal changed nothing.
using ActionResult = std::variant<Result, CollectionResult, WorkshopResult>;

/// One actor's command for one step: `movement` goes to `Exploration::advance`
/// and `action` to `apply`. An empty command waits.
struct ActorCommand {
  ExplorationInput movement{};
  std::optional<Action> action = std::nullopt;
};

/// Runs one action through the feature that owns its rules.
[[nodiscard]] ActionResult apply(Exploration &exploration, Workshop &workshop,
                                 Actor actor, const Action &action);

/// What one human pressed this step, free of device detail. `switch_actor` and
/// `leave` are for the caller's ControlMap, not for `Companion`. `tap` is for
/// the caller too, which turns it into `movement` for one step.
struct PlayerInput {
  ExplorationInput movement{};
  /// A direction pressed and released between two samples, so never held.
  Direction tap = Direction::Idle;
  bool travel = false;
  bool interact = false;
  bool feed = false;
  bool end_day = false;
  bool switch_actor = false;
  bool leave = false;
};

/// The same values the features are built with, plus the temporary penalty for
/// a day ended unfed.
struct CompanionConfig {
  ExplorationConfig exploration;
  WorkshopConfig workshop;
  AcidLevel unfed_penalty;
};

/// What AI would go to next, and the first move on the way there: none once
/// the target works from where the actor stands.
struct Errand {
  ObjectId target;
  std::optional<Move> step = std::nullopt;
};

/// Whether setting out now gives an errand, by the search `errand` runs, from
/// the entrance.
[[nodiscard]] bool worth_entering(const CompanionConfig &config,
                                  const GameView &view);

/// Turns a view into the next command for either kind of controller. Holds no
/// game state and no random state, so one input gives one command.
class Companion {
public:
  explicit Companion(CompanionConfig settings);
  /// Translates `input` for a human; AI ignores it. A collecting actor's
  /// command is empty under either. Read a fresh view for each actor after
  /// applying the other's action, and advance at most one 100ms movement unit
  /// between commands, since AI steers from where it stands.
  [[nodiscard]] ActorCommand command(Actor actor, ControlMap control,
                                     const PlayerInput &input,
                                     const GameView &view) const;
  /// The errand AI would run in this actor's place, for telling a human the
  /// same way; in the air, from where a straight drop lands. Empty when
  /// nothing is in reach.
  [[nodiscard]] std::optional<Errand> errand(Actor actor,
                                             const GameView &view) const;

private:
  CompanionConfig config;
};

} // namespace neocity::survival
