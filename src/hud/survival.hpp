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

/// The escape check of finished day `day` did not hit.
struct EscapeMissEvent {
  std::uint32_t day = 0;
};

/// What the screen reports to the player.
using Event = std::variant<ActionEvent, HazardEvent, FallEvent, TimeUpEvent,
                           WorldEvent, EscapeMissEvent>;

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
  /// An AI chef has base work to do on its next step.
  bool ai_chef_working = false;
  /// How many city objects changed during the last finished day, the acid it
  /// raised included.
  std::uint32_t last_day_map_changes = 0;
  /// Oldest first.
  std::vector<Event> events;
};

/// A line whose words change with play, and the width its place gives it.
struct FittedText {
  godot::String text;
  int size = 0;
  float width = 0.0F;
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
/// When the escape checks begin, on every survival screen.
[[nodiscard]] godot::String escape_goal();
/// The last finished day's result, line by line; empty before the first.
[[nodiscard]] std::vector<godot::String> day_result(const SurvivalFrame &frame);
/// The offered dish's score, plain and fine, against the threshold; empty when
/// the chef's key offers no dish.
[[nodiscard]] godot::String offered_forecast(const SurvivalFrame &frame);
/// The ingredients eaten so far, wrapped for the appraise panel.
[[nodiscard]] std::vector<godot::String>
eaten_lines(const std::vector<survival::IngredientId> &eaten);
/// The appraise panel's last line.
[[nodiscard]] godot::String appraise_job(const SurvivalFrame &frame);
/// Stocked, supplying sources no human can walk to: a human in the city from
/// where they stand, one at base from the entrance.
[[nodiscard]] std::vector<survival::ObjectId>
out_of_reach(const SurvivalFrame &frame);
/// Every changing line of this frame, with the cook panel's status in each form
/// any pack could give it.
[[nodiscard]] std::vector<FittedText> changing_text(const SurvivalFrame &frame);
/// The sentence the event log shows for `event`.
[[nodiscard]] godot::String describe(const Event &event);

} // namespace neocity::hud
