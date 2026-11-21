#include "tips.hpp"

#include <survival/companion.hpp>
#include <survival/workshop.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace neocity::session {
namespace {

/// How long a tip shows when nothing closes it first; a temporary value.
constexpr double TIP_SECONDS = 8.0;

/// Whether either actor carries an ingredient nobody has identified, which
/// the bag marks with a question mark.
bool carries_unidentified(const survival::GameView &view) {
  const auto &identified = view.workshop.identified;
  const auto unknown = [&identified](survival::IngredientId ingredient) {
    return ingredient != survival::ESCAPE_PART &&
           std::ranges::find(identified, ingredient) == identified.end();
  };
  return std::ranges::any_of(view.progression.chef.inventory, unknown) ||
         std::ranges::any_of(view.progression.appraiser.inventory, unknown);
}

/// Whether `frame` shows the concept `tip` explains.
bool due(hud::Tip tip, const hud::SurvivalFrame &frame) {
  const auto &view = frame.view;
  const auto &progression = view.progression;
  const bool at_base = progression.phase == survival::Phase::Workshop;
  switch (tip) {
  case hud::Tip::SetOut:
    return at_base && progression.remaining_exploration.count() > 0;
  case hud::Tip::Move:
  case hud::Tip::Collect:
    // The city shows no tip; the controls page lists these two.
    return false;
  case hud::Tip::Appraise:
    return carries_unidentified(view);
  case hud::Tip::Cook:
    return at_base &&
           std::ranges::any_of(frame.content.get().companion.workshop.recipes,
                               [&view](const survival::Recipe &recipe) {
                                 return hud::recipe_ready(view, recipe);
                               });
  case hud::Tip::Feed:
    return at_base && !view.workshop.foods.empty();
  case hud::Tip::Rejected:
    return !view.workshop.rejected_foods.empty();
  case hud::Tip::Acid:
    return progression.acid.value > 0;
  case hud::Tip::Ending:
    return view.workshop.last_day.has_value();
  }
  std::abort();
}

} // namespace

void Tips::advance(const hud::SurvivalFrame &frame, double delta, bool acted) {
  if (current) {
    if (std::isfinite(delta) && delta > 0.0) {
      shown_for += delta;
    }
    if (acted || shown_for >= TIP_SECONDS ||
        frame.view.progression.phase != survival::Phase::Workshop) {
      current.reset();
    }
  }
  if (current || frame.view.progression.phase != survival::Phase::Workshop) {
    return;
  }
  for (const auto tip : hud::TIPS) {
    if (std::ranges::find(seen, tip) == seen.end() && due(tip, frame)) {
      current = tip;
      shown_for = 0.0;
      seen.push_back(tip);
      return;
    }
  }
}

std::optional<hud::Tip> Tips::open() const { return current; }

const std::vector<hud::Tip> &Tips::opened() const { return seen; }

} // namespace neocity::session
