#pragma once

#include "../rendering/drawing.hpp"

#include <survival/companion.hpp>
#include <survival/content.hpp>

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/string.hpp>

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <variant>
#include <vector>

namespace neocity::hud {

/// A feature command an actor ran, with the feature's own answer.
struct ActionEvent {
  survival::Actor actor = survival::Actor::Chef;
  survival::Action action;
  survival::ActionResult result;
};

/// An actor stepped into an acid drip; `exhausted` means it ran out of health.
struct HazardEvent {
  survival::Actor actor = survival::Actor::Chef;
  bool exhausted = false;
};

/// An actor walked off the map and was carried back to base.
struct FallEvent {
  survival::Actor actor = survival::Actor::Chef;
};

/// The day's exploration time ran out.
struct TimeUpEvent {};

/// A city object changed state.
struct WorldEvent {
  survival::ObjectId object;
  survival::ObjectState state = survival::ObjectState::Intact;
};

/// What the screen reports to the player.
using Event =
    std::variant<ActionEvent, HazardEvent, FallEvent, TimeUpEvent, WorldEvent>;

/// What one human's interact and feed keys would do right now, and the errand
/// AI would run in that human's place. The NEXT line names that errand, so the
/// screen tells a player where to walk.
struct Offers {
  std::optional<survival::Action> interact;
  std::optional<survival::Action> feed;
  std::optional<survival::Errand> errand;
};

/// Everything one survival frame draws, read from snapshots.
struct SurvivalFrame {
  std::reference_wrapper<const survival::Content> content;
  survival::GameView view;
  survival::ControlMap control;
  /// Chef first; an AI actor's entry stays empty.
  std::array<Offers, 2> offers;
  /// Oldest first.
  std::vector<Event> events;
};

/// The authored moments of a run: its start and each way it can end.
enum class Cinematic : std::uint8_t { Intro, Ending, Loss };

/// Draws the choice of who plays which actor.
void paint_survival_setup(godot::Node2D &canvas,
                          const rendering::AscentDrawing &drawing);
/// Draws the city or the workshop, whichever the phase opens, with the HUD.
void paint_survival(godot::Node2D &canvas,
                    const rendering::AscentDrawing &drawing,
                    const SurvivalFrame &frame);
/// Draws the dark stage under a cinematic scene or, with `fallback`, the
/// placeholder card that stands in when no scene can play.
void paint_cinematic(godot::Node2D &canvas,
                     const rendering::AscentDrawing &drawing,
                     Cinematic cinematic, bool fallback,
                     const SurvivalFrame &frame);
/// Whether the chef can cook `recipe` from what the two actors carry and have
/// identified.
[[nodiscard]] bool recipe_ready(const survival::GameView &view,
                                const survival::Recipe &recipe);
/// The one line telling the player what to do next.
[[nodiscard]] godot::String next_step(const SurvivalFrame &frame);
/// The sentence the event log shows for `event`.
[[nodiscard]] godot::String describe(const Event &event);

} // namespace neocity::hud
