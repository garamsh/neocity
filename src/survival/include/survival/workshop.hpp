#pragma once

#include <survival/simulation.hpp>

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <random>
#include <vector>

namespace neocity::survival {

/// Caller-assigned identity of a cooked dish.
struct FoodId {
  std::uint32_t value = 0;
  friend bool operator==(const FoodId &, const FoodId &) = default;
};

/// How well a dish turned out; a failed cook makes no dish at all.
enum class DishQuality : std::uint8_t { Plain, Fine };

/// A cooked dish waiting to be fed.
struct Dish {
  FoodId food;
  DishQuality quality = DishQuality::Plain;
  friend bool operator==(const Dish &, const Dish &) = default;
};

/// How much one ingredient adds to a dish's taste.
struct Flavor {
  IngredientId ingredient;
  std::uint32_t taste = 0;
};

/// Temporary recipe data, supplied by the caller and not a final recipe list.
struct Recipe {
  FoodId food;
  std::vector<IngredientId> ingredients;
  bool disliked = false;
};

/// Temporary caller-supplied balance values, with no production default.
/// An ingredient with no `flavors` entry adds no taste, and `fine_dish_score`
/// is what a fine dish adds over a plain one. A day fed to satisfaction lowers
/// acid by `satisfied_acid_drop`. `cook_time` is how long a cook takes, with
/// none the dish is done at once. `base_time` is the base's time each day, and
/// with none the base has no clock; when it runs out the day ends unfed at
/// `timeout_acid_rise`.
struct WorkshopConfig {
  std::vector<Recipe> recipes;
  std::uint32_t new_food_score;
  std::uint32_t new_ingredient_score;
  std::uint32_t satisfying_score;
  AcidLevel disliked_acid_rise;
  AcidLevel unsatisfied_acid_rise;
  std::vector<Flavor> flavors;
  std::uint32_t fine_dish_score = 0;
  AcidLevel satisfied_acid_drop{};
  std::chrono::microseconds cook_time{};
  std::optional<std::chrono::microseconds> base_time = std::nullopt;
  AcidLevel timeout_acid_rise{};
};

/// What a recipe's ingredients add to a dish's score, whoever cooks it.
[[nodiscard]] std::uint64_t taste_of(const Recipe &recipe,
                                     const WorkshopConfig &config);

/// What feeding a dish would score, part by part. Says nothing of a dislike,
/// which is learned by feeding.
struct DishScore {
  std::uint64_t new_food = 0;
  /// How many ingredient kinds would be eaten for the first time.
  std::uint32_t new_ingredients = 0;
  std::uint64_t new_ingredient_points = 0;
  std::uint64_t taste = 0;
  std::uint64_t fine = 0;
  std::uint64_t total = 0;
  friend bool operator==(const DishScore &, const DishScore &) = default;
};

/// The dish a day ended on and how it scored.
struct FedDish {
  Dish dish;
  DishScore score;
  friend bool operator==(const FedDish &, const FedDish &) = default;
};

/// Unfed marks a day ended without feeding.
enum class MonsterReaction : std::uint8_t {
  Unfed,
  Disliked,
  Unsatisfied,
  Satisfied
};

/// Result of a finished day.
struct DayOutcome {
  std::uint32_t completed_day = 0;
  MonsterReaction reaction = MonsterReaction::Unfed;
  std::uint64_t score = 0;
  AcidLevel acid_rise;
  AcidLevel acid_drop{};
  /// Empty on a day ended unfed.
  std::optional<FedDish> fed = std::nullopt;
  friend bool operator==(const DayOutcome &, const DayOutcome &) = default;
};

/// A cook in progress. `slots` are the ingredients in the cooking slots, in
/// recipe order; the dish is done when `elapsed` reaches `duration`.
struct Cooking {
  FoodId food;
  std::vector<IngredientId> slots;
  std::chrono::microseconds elapsed{};
  std::chrono::microseconds duration{};
  friend bool operator==(const Cooking &, const Cooking &) = default;
};

/// How far `cooking` has come, from 0 to 1.
[[nodiscard]] float progress_of(const Cooking &cooking);

/// Read-only values for a renderer or companion controller. `rejected_foods`
/// are the fed foods the monster disliked, `satisfied_days` counts the days it
/// was fed to satisfaction, and `rolls` counts the random draws consumed so
/// far. `cooking` is set while a cook is in progress, and `ingredient_slots`
/// is how many ingredients its slots hold at most. `remaining_base` is the
/// base's time left today, and is empty when the base has no clock.
struct WorkshopSnapshot {
  std::vector<IngredientId> identified;
  std::vector<Dish> foods;
  std::vector<FoodId> eaten_foods;
  std::vector<IngredientId> eaten_ingredients;
  std::vector<FoodId> rejected_foods;
  std::optional<DayOutcome> last_day;
  std::uint32_t satisfied_days = 0;
  std::uint64_t rolls = 0;
  std::optional<Cooking> cooking;
  std::uint32_t ingredient_slots = 0;
  std::optional<std::chrono::microseconds> remaining_base;
  friend bool operator==(const WorkshopSnapshot &,
                         const WorkshopSnapshot &) = default;
};

/// What feeding `dish` would score after the history in `workshop`, or nothing
/// for a dish of no configured recipe.
[[nodiscard]] std::optional<DishScore>
dish_score(const Dish &dish, const WorkshopSnapshot &workshop,
           const WorkshopConfig &config);

/// Failed is a spent attempt; every other non-Applied value is a refusal that
/// leaves all state and the random sequence unchanged. Full also refuses a cook
/// while the cooking slots are in use.
enum class WorkshopResult : std::uint8_t {
  Applied,
  Failed,
  WrongRole,
  NotInWorkshop,
  AlreadyIdentified,
  NotCarried,
  NotIdentified,
  UnknownRecipe,
  NoFood,
  Full
};

/// Untimed base roles; owns appraisal facts, dishes, feeding history and its
/// random state, seeded once from the run seed. Appraisal and cooking succeed
/// 90% of the time. Progression stays in the supplied Simulation, which must
/// outlive this instance.
class Workshop {
public:
  /// Throws std::invalid_argument for an empty recipe, a repeated food, a
  /// negative cook time or a nonpositive base time.
  Workshop(Simulation &progression, WorkshopConfig settings);
  /// The appraiser records a carried ingredient as identified on success.
  [[nodiscard]] WorkshopResult appraise(Actor actor, IngredientId ingredient);
  /// Moves one carried `ingredient` into the fridge; `Full` with no free fridge
  /// slot. Appraisal and cooking read the fridge with both inventories.
  [[nodiscard]] WorkshopResult store(Actor actor, IngredientId ingredient);
  /// Moves one `ingredient` out of the fridge into the actor's inventory;
  /// `Full` with no free inventory slot, `NotCarried` if the fridge lacks it.
  [[nodiscard]] WorkshopResult withdraw(Actor actor, IngredientId ingredient);
  /// The chef puts the identified ingredients into the cooking slots and the
  /// dish is done after `cook_time` of base time; `Full` while a cook is in
  /// progress. The ingredients are spent at once. Failure produces no dish, and
  /// the roll made when the cook ends decides whether a dish is fine.
  [[nodiscard]] WorkshopResult cook(Actor actor, FoodId food);
  /// Passes `elapsed` of base time, which only runs in the workshop phase: it
  /// moves a cook on and, when the day's base time is spent, ends the day
  /// unfed at `timeout_acid_rise`. A negative time passes nothing.
  void advance(std::chrono::microseconds elapsed);
  /// Feeds one stored dish, then starts the next day. The score adds novelty,
  /// the ingredients' taste and the dish's quality: at the satisfying score
  /// acid falls by `satisfied_acid_drop`, and below it acid rises with the
  /// shortfall. A cook in progress is lost with the day.
  [[nodiscard]] WorkshopResult feed(FoodId food);
  /// Starts the next day with no dish at the caller's acid penalty.
  [[nodiscard]] WorkshopResult end_day_unfed(AcidLevel acid_penalty);
  /// Returns independent values.
  [[nodiscard]] WorkshopSnapshot snapshot() const;

private:
  [[nodiscard]] bool in_workshop() const;
  std::uint32_t roll();
  /// Ends the cook in progress with its roll: `Failed` or a stored dish.
  WorkshopResult finish_cooking(FoodId food);
  /// Starts the next day's base time and drops a cook left unfinished.
  void start_day();

  std::reference_wrapper<Simulation> simulation;
  WorkshopConfig config;
  WorkshopSnapshot state;
  std::mt19937 generator;
};

} // namespace neocity::survival
