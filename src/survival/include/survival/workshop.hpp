#pragma once

#include <survival/simulation.hpp>

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

/// Base tools; exploration carries them as decoration only.
enum class WorkshopTool : std::uint8_t { Magnifier, Spatula };

/// A damaged tool succeeds 40% of the time instead of 90%.
enum class ToolState : std::uint8_t { Intact, Damaged };

/// Temporary recipe data, supplied by the caller and not a final recipe list.
struct Recipe {
  FoodId food;
  std::vector<IngredientId> ingredients;
  bool disliked = false;
};

/// Temporary caller-supplied balance values, with no production default.
struct WorkshopConfig {
  std::vector<Recipe> recipes;
  std::uint32_t new_food_score;
  std::uint32_t new_ingredient_score;
  std::uint32_t satisfying_score;
  std::uint32_t disliked_acid_rise;
  std::uint32_t unsatisfied_acid_rise;
};

/// Unfed marks a day ended without feeding.
enum class MonsterReaction : std::uint8_t {
  Unfed,
  Disliked,
  Unsatisfied,
  Satisfied
};

/// Result of a finished day; escape rules read the day number from here.
struct DayOutcome {
  std::uint32_t completed_day = 0;
  MonsterReaction reaction = MonsterReaction::Unfed;
  std::uint64_t score = 0;
  std::uint32_t acid_rise = 0;
  friend bool operator==(const DayOutcome &, const DayOutcome &) = default;
};

/// Read-only values for a renderer or companion controller. `rolls` counts the
/// random draws consumed so far.
struct WorkshopSnapshot {
  ToolState magnifier = ToolState::Intact;
  ToolState spatula = ToolState::Intact;
  std::vector<IngredientId> identified;
  std::vector<FoodId> foods;
  std::vector<FoodId> eaten_foods;
  std::vector<IngredientId> eaten_ingredients;
  std::optional<DayOutcome> last_day;
  std::uint64_t rolls = 0;
  friend bool operator==(const WorkshopSnapshot &,
                         const WorkshopSnapshot &) = default;
};

/// Failed is a spent attempt; every other non-Applied value is a refusal that
/// leaves all state and the random sequence unchanged.
enum class WorkshopResult : std::uint8_t {
  Applied,
  Failed,
  WrongRole,
  NotInWorkshop,
  AlreadyIdentified,
  NotCarried,
  NotIdentified,
  UnknownRecipe,
  NoFood
};

/// Untimed base roles; owns tools, appraisal facts, dishes, feeding history and
/// its random state, seeded once from the run seed. Progression stays in the
/// supplied Simulation, which must outlive this instance.
class Workshop {
public:
  /// Throws std::invalid_argument for an empty recipe or a repeated food.
  Workshop(Simulation &progression, WorkshopConfig settings);
  /// The appraiser records a carried ingredient as identified on success.
  [[nodiscard]] WorkshopResult appraise(Actor actor, IngredientId ingredient);
  /// The chef spends the identified ingredients; failure produces no dish.
  [[nodiscard]] WorkshopResult cook(Actor actor, FoodId food);
  /// Feeds one stored dish, then starts the next day.
  [[nodiscard]] WorkshopResult feed(FoodId food);
  /// Starts the next day with no dish at the caller's acid penalty.
  [[nodiscard]] WorkshopResult end_day_unfed(std::uint32_t acid_penalty);
  /// Applies in any phase: damage events can arrive during exploration.
  void damage_tool(WorkshopTool tool);
  /// Restores the tool; possession and cost are checked by the caller.
  [[nodiscard]] WorkshopResult replace_tool(WorkshopTool tool);
  /// Returns independent values.
  [[nodiscard]] WorkshopSnapshot snapshot() const;

private:
  ToolState &tool_state(WorkshopTool tool);
  [[nodiscard]] bool in_workshop() const;
  bool attempt(ToolState tool);

  std::reference_wrapper<Simulation> simulation;
  WorkshopConfig config;
  WorkshopSnapshot state;
  std::mt19937 generator;
};

} // namespace neocity::survival
