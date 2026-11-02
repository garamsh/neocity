#include <survival/workshop.hpp>

#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <utility>

namespace neocity::survival {
namespace {

constexpr std::uint32_t INTACT_SUCCESS_PERCENT = 90;
constexpr std::uint32_t DAMAGED_SUCCESS_PERCENT = 40;

std::uint32_t success_percent(ToolState tool) {
  switch (tool) {
  case ToolState::Intact:
    return INTACT_SUCCESS_PERCENT;
  case ToolState::Damaged:
    return DAMAGED_SUCCESS_PERCENT;
  }
  std::abort();
}

bool carried(const Snapshot &snapshot, IngredientId ingredient) {
  return std::ranges::find(snapshot.chef.inventory, ingredient) !=
             snapshot.chef.inventory.end() ||
         std::ranges::find(snapshot.appraiser.inventory, ingredient) !=
             snapshot.appraiser.inventory.end();
}

std::uint32_t owner_exhaustions(const Snapshot &snapshot, WorkshopTool tool) {
  switch (tool) {
  case WorkshopTool::Magnifier:
    return snapshot.appraiser.exhaustions;
  case WorkshopTool::Spatula:
    return snapshot.chef.exhaustions;
  }
  std::abort();
}

MonsterReaction react(const Recipe &recipe, std::uint64_t score,
                      std::uint32_t satisfying_score) {
  if (recipe.disliked) {
    return MonsterReaction::Disliked;
  }
  return score >= satisfying_score ? MonsterReaction::Satisfied
                                   : MonsterReaction::Unsatisfied;
}

/// Rounds up, so any meal below the satisfying score raises acid. A score of
/// zero rises by the whole configured amount.
AcidLevel shortfall_rise(std::uint64_t score, const WorkshopConfig &config) {
  const std::uint64_t satisfying = config.satisfying_score;
  const auto rise = (std::uint64_t{config.unsatisfied_acid_rise.value} *
                         (satisfying - score) +
                     satisfying - 1) /
                    satisfying;
  return AcidLevel{static_cast<std::uint32_t>(rise)};
}

AcidLevel acid_rise(MonsterReaction reaction, std::uint64_t score,
                    const WorkshopConfig &config) {
  switch (reaction) {
  case MonsterReaction::Disliked:
    return config.disliked_acid_rise;
  case MonsterReaction::Unsatisfied:
    return shortfall_rise(score, config);
  case MonsterReaction::Satisfied:
  case MonsterReaction::Unfed:
    return {};
  }
  std::abort();
}

} // namespace

std::uint64_t taste_of(const Recipe &recipe, const WorkshopConfig &config) {
  std::uint64_t taste = 0;
  for (const auto ingredient : recipe.ingredients) {
    const auto flavor =
        std::ranges::find(config.flavors, ingredient, &Flavor::ingredient);
    taste += flavor != config.flavors.end() ? flavor->taste : 0U;
  }
  return taste;
}

Workshop::Workshop(Simulation &progression, WorkshopConfig settings)
    : simulation(progression), config(std::move(settings)),
      generator(progression.snapshot().seed) {
  if (config.magnifier == config.spatula) {
    throw std::invalid_argument("base tools require distinct identities");
  }
  std::vector<FoodId> foods;
  for (const auto &recipe : config.recipes) {
    if (recipe.ingredients.empty() ||
        std::ranges::find(foods, recipe.food) != foods.end()) {
      throw std::invalid_argument(
          "recipes require ingredients and unique foods");
    }
    foods.push_back(recipe.food);
  }
}

WorkshopResult Workshop::appraise(Actor actor, IngredientId ingredient) {
  if (!in_workshop()) {
    return WorkshopResult::NotInWorkshop;
  }
  if (actor != Actor::Appraiser) {
    return WorkshopResult::WrongRole;
  }
  if (std::ranges::find(state.identified, ingredient) !=
      state.identified.end()) {
    return WorkshopResult::AlreadyIdentified;
  }
  if (!carried(simulation.get().snapshot(), ingredient)) {
    return WorkshopResult::NotCarried;
  }
  if (roll() >= success_percent(condition(WorkshopTool::Magnifier))) {
    return WorkshopResult::Failed;
  }
  state.identified.push_back(ingredient);
  return WorkshopResult::Applied;
}

WorkshopResult Workshop::cook(Actor actor, FoodId food) {
  if (!in_workshop()) {
    return WorkshopResult::NotInWorkshop;
  }
  if (actor != Actor::Chef) {
    return WorkshopResult::WrongRole;
  }
  const auto recipe = std::ranges::find(config.recipes, food, &Recipe::food);
  if (recipe == config.recipes.end()) {
    return WorkshopResult::UnknownRecipe;
  }
  const auto is_identified = [this](IngredientId ingredient) {
    return std::ranges::find(state.identified, ingredient) !=
           state.identified.end();
  };
  if (!std::ranges::all_of(recipe->ingredients, is_identified)) {
    return WorkshopResult::NotIdentified;
  }
  // Consume before rolling: a refusal must leave the random sequence untouched.
  if (simulation.get().consume(recipe->ingredients) != Result::Applied) {
    return WorkshopResult::NotCarried;
  }
  const auto result = roll();
  const auto success = success_percent(condition(WorkshopTool::Spatula));
  if (result >= success) {
    return WorkshopResult::Failed;
  }
  state.foods.push_back({.food = food,
                         .quality = result * 2U < success
                                        ? DishQuality::Fine
                                        : DishQuality::Plain});
  return WorkshopResult::Applied;
}

WorkshopResult Workshop::feed(FoodId food) {
  const auto stored = std::ranges::find(state.foods, food, &Dish::food);
  if (stored == state.foods.end()) {
    return WorkshopResult::NoFood;
  }
  // Stored dishes always come from a configured recipe.
  const auto &recipe = *std::ranges::find(config.recipes, food, &Recipe::food);
  std::vector<IngredientId> new_ingredients;
  for (const auto ingredient : recipe.ingredients) {
    if (std::ranges::find(state.eaten_ingredients, ingredient) ==
            state.eaten_ingredients.end() &&
        std::ranges::find(new_ingredients, ingredient) ==
            new_ingredients.end()) {
      new_ingredients.push_back(ingredient);
    }
  }
  const bool new_food =
      std::ranges::find(state.eaten_foods, food) == state.eaten_foods.end();
  const std::uint64_t score =
      (new_food ? config.new_food_score : 0U) +
      std::uint64_t{config.new_ingredient_score} * new_ingredients.size() +
      taste_of(recipe, config) +
      (stored->quality == DishQuality::Fine ? config.fine_dish_score : 0U);
  const auto reaction = react(recipe, score, config.satisfying_score);
  const auto rise = acid_rise(reaction, score, config);
  const auto day = simulation.get().snapshot().day;
  if (simulation.get().finish_day(rise) != Result::Applied) {
    return WorkshopResult::NotInWorkshop;
  }
  state.foods.erase(stored);
  state.eaten_foods.push_back(food);
  if (reaction == MonsterReaction::Disliked &&
      std::ranges::find(state.rejected_foods, food) ==
          state.rejected_foods.end()) {
    state.rejected_foods.push_back(food);
  }
  state.eaten_ingredients.insert(state.eaten_ingredients.end(),
                                 new_ingredients.begin(),
                                 new_ingredients.end());
  state.last_day = {.completed_day = day,
                    .reaction = reaction,
                    .score = score,
                    .acid_rise = rise};
  return WorkshopResult::Applied;
}

WorkshopResult Workshop::end_day_unfed(AcidLevel acid_penalty) {
  const auto day = simulation.get().snapshot().day;
  if (simulation.get().finish_day(acid_penalty) != Result::Applied) {
    return WorkshopResult::NotInWorkshop;
  }
  state.last_day = {.completed_day = day,
                    .reaction = MonsterReaction::Unfed,
                    .score = 0,
                    .acid_rise = acid_penalty};
  return WorkshopResult::Applied;
}

void Workshop::damage_tool(WorkshopTool tool) {
  if (run_over(simulation.get().snapshot().phase)) {
    return;
  }
  tool_state(tool) = ToolState::Damaged;
}

WorkshopResult Workshop::replace_tool(WorkshopTool tool) {
  if (!in_workshop()) {
    return WorkshopResult::NotInWorkshop;
  }
  if (condition(tool) == ToolState::Intact) {
    return WorkshopResult::ToolIntact;
  }
  const auto replacement =
      tool == WorkshopTool::Magnifier ? config.magnifier : config.spatula;
  if (simulation.get().consume_tool(replacement) != Result::Applied) {
    return WorkshopResult::NotCarried;
  }
  tool_state(tool) = ToolState::Intact;
  settled_exhaustions(tool) =
      owner_exhaustions(simulation.get().snapshot(), tool);
  return WorkshopResult::Applied;
}

WorkshopSnapshot Workshop::snapshot() const {
  auto result = state;
  result.magnifier = condition(WorkshopTool::Magnifier);
  result.spatula = condition(WorkshopTool::Spatula);
  return result;
}

ToolState &Workshop::tool_state(WorkshopTool tool) {
  switch (tool) {
  case WorkshopTool::Magnifier:
    return state.magnifier;
  case WorkshopTool::Spatula:
    return state.spatula;
  }
  std::abort();
}

std::uint32_t &Workshop::settled_exhaustions(WorkshopTool tool) {
  switch (tool) {
  case WorkshopTool::Magnifier:
    return magnifier_exhaustions;
  case WorkshopTool::Spatula:
    return spatula_exhaustions;
  }
  std::abort();
}

// Derived from the count so no exhaustion path can miss a tool.
ToolState Workshop::condition(WorkshopTool tool) const {
  const auto explicit_state =
      tool == WorkshopTool::Magnifier ? state.magnifier : state.spatula;
  const auto settled = tool == WorkshopTool::Magnifier ? magnifier_exhaustions
                                                       : spatula_exhaustions;
  return owner_exhaustions(simulation.get().snapshot(), tool) > settled
             ? ToolState::Damaged
             : explicit_state;
}

bool Workshop::in_workshop() const {
  return simulation.get().snapshot().phase == Phase::Workshop;
}

// The standard fixes mt19937's output but not uniform_int_distribution's, so
// this transform replays a seed identically on every implementation.
std::uint32_t Workshop::roll() {
  ++state.rolls;
  return static_cast<std::uint32_t>(generator() % 100U);
}

} // namespace neocity::survival
