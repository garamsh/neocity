#include <survival/workshop.hpp>

#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <utility>

namespace neocity::survival {
namespace {

constexpr std::uint32_t SUCCESS_PERCENT = 90;

bool carried(const Snapshot &snapshot, IngredientId ingredient) {
  return std::ranges::find(snapshot.chef.inventory, ingredient) !=
             snapshot.chef.inventory.end() ||
         std::ranges::find(snapshot.appraiser.inventory, ingredient) !=
             snapshot.appraiser.inventory.end();
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

std::vector<IngredientId> first_eaten(const Recipe &recipe,
                                      const WorkshopSnapshot &workshop) {
  std::vector<IngredientId> fresh;
  for (const auto ingredient : recipe.ingredients) {
    if (std::ranges::find(workshop.eaten_ingredients, ingredient) ==
            workshop.eaten_ingredients.end() &&
        std::ranges::find(fresh, ingredient) == fresh.end()) {
      fresh.push_back(ingredient);
    }
  }
  return fresh;
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
  if (roll() >= SUCCESS_PERCENT) {
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
  if (result >= SUCCESS_PERCENT) {
    return WorkshopResult::Failed;
  }
  state.foods.push_back({.food = food,
                         .quality = result * 2U < SUCCESS_PERCENT
                                        ? DishQuality::Fine
                                        : DishQuality::Plain});
  return WorkshopResult::Applied;
}

std::optional<DishScore> dish_score(const Dish &dish,
                                    const WorkshopSnapshot &workshop,
                                    const WorkshopConfig &config) {
  const auto recipe =
      std::ranges::find(config.recipes, dish.food, &Recipe::food);
  if (recipe == config.recipes.end()) {
    return std::nullopt;
  }
  const bool new_food = std::ranges::find(workshop.eaten_foods, dish.food) ==
                        workshop.eaten_foods.end();
  const auto new_ingredients =
      static_cast<std::uint32_t>(first_eaten(*recipe, workshop).size());
  DishScore score{
      .new_food = new_food ? config.new_food_score : 0U,
      .new_ingredients = new_ingredients,
      .new_ingredient_points =
          std::uint64_t{config.new_ingredient_score} * new_ingredients,
      .taste = taste_of(*recipe, config),
      .fine = dish.quality == DishQuality::Fine ? config.fine_dish_score : 0U};
  score.total =
      score.new_food + score.new_ingredient_points + score.taste + score.fine;
  return score;
}

WorkshopResult Workshop::feed(FoodId food) {
  const auto stored = std::ranges::find(state.foods, food, &Dish::food);
  if (stored == state.foods.end()) {
    return WorkshopResult::NoFood;
  }
  // Stored dishes always come from a configured recipe.
  const auto &recipe = *std::ranges::find(config.recipes, food, &Recipe::food);
  const auto new_ingredients = first_eaten(recipe, state);
  const auto fed = FedDish{
      .dish = *stored,
      .score = dish_score(*stored, state, config).value_or(DishScore{})};
  const auto score = fed.score.total;
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
                    .acid_rise = rise,
                    .fed = fed};
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

WorkshopSnapshot Workshop::snapshot() const { return state; }

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
