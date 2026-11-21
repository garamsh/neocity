#include <survival/ending.hpp>
#include <survival/save.hpp>

#include <algorithm>
#include <charconv>
#include <limits>
#include <memory>
#include <sstream>
#include <type_traits>

namespace neocity::survival {
namespace {

constexpr std::size_t MAX_ITEMS = 4096;
constexpr std::size_t MAX_BYTES = 131072;

struct Writer {
  std::ostringstream stream;

  template <typename T> void value(const T &number) { stream << number << ' '; }
  template <typename T> void list(const std::vector<T> &items) {
    value(items.size());
    for (const auto &item : items) {
      value(item.value);
    }
  }
};

struct Reader {
  std::istringstream stream;
  bool good = true;

  template <typename T> void value(T &number) {
    std::string token;
    if (!(stream >> token)) {
      good = false;
      return;
    }
    const char *const first = token.data();
    const char *const last = std::to_address(token.end());
    const auto parsed = std::from_chars(first, last, number);
    good = good && parsed.ec == std::errc{} && parsed.ptr == last;
  }
  template <typename T> void list(std::vector<T> &items) {
    std::size_t count = 0;
    value(count);
    if (!good || count > MAX_ITEMS) {
      good = false;
      return;
    }
    items.resize(count);
    for (auto &item : items) {
      value(item.value);
    }
  }
};

template <typename Archive, typename T> void field(Archive &archive, T &item) {
  archive.value(item);
}

template <typename Archive, typename T>
void enumeration(Archive &archive, T &value, unsigned last) {
  auto number = static_cast<unsigned>(value);
  field(archive, number);
  if constexpr (std::is_same_v<Archive, Reader>) {
    if (number > last) {
      archive.good = false;
      return;
    }
    value = static_cast<T>(number);
  }
}

template <typename Archive> void stock(Archive &archive, SourceStock &source) {
  archive.value(source.id.value);
  archive.list(source.items);
}

template <typename Archive> void dish(Archive &archive, Dish &value) {
  archive.value(value.food.value);
  enumeration(archive, value.quality, 1);
}

template <typename Archive> void outcome(Archive &archive, DayOutcome &value) {
  archive.value(value.completed_day);
  enumeration(archive, value.reaction, 3);
  archive.value(value.score);
  archive.value(value.acid_rise.value);
  archive.value(value.acid_drop.value);
  unsigned fed = value.fed ? 1U : 0U;
  field(archive, fed);
  if constexpr (std::is_same_v<Archive, Reader>) {
    archive.good = archive.good && fed <= 1;
    if (fed == 1) {
      value.fed.emplace();
    }
  }
  if (value.fed) {
    dish(archive, value.fed->dish);
    archive.value(value.fed->score.new_food);
    archive.value(value.fed->score.new_ingredients);
    archive.value(value.fed->score.new_ingredient_points);
    archive.value(value.fed->score.taste);
    archive.value(value.fed->score.fine);
    archive.value(value.fed->score.total);
  }
}

template <typename Archive>
void state(Archive &archive, Snapshot &progression, WorkshopSnapshot &kitchen,
           std::vector<SourceStock> &sources,
           std::array<std::uint32_t, 2> &touches, ControlMap &players) {
  archive.value(progression.day);
  archive.value(progression.acid.value);
  archive.value(progression.seed);
  archive.list(progression.chef.inventory);
  archive.list(progression.appraiser.inventory);
  archive.list(progression.fridge);
  archive.list(kitchen.identified);
  archive.list(kitchen.eaten_foods);
  archive.list(kitchen.eaten_ingredients);
  archive.list(kitchen.rejected_foods);
  archive.value(kitchen.satisfied_days);
  archive.value(kitchen.rolls);
  enumeration(archive, players.chef, 1);
  enumeration(archive, players.appraiser, 1);
  for (auto &touch : touches) {
    field(archive, touch);
  }
  std::size_t count = kitchen.foods.size();
  archive.value(count);
  if constexpr (std::is_same_v<Archive, Reader>) {
    if (!archive.good || count > MAX_ITEMS) {
      archive.good = false;
      return;
    }
    kitchen.foods.resize(count);
  }
  for (auto &food : kitchen.foods) {
    dish(archive, food);
  }
  unsigned last = kitchen.last_day ? 1U : 0U;
  field(archive, last);
  if constexpr (std::is_same_v<Archive, Reader>) {
    archive.good = archive.good && last <= 1;
    if (last == 1) {
      kitchen.last_day.emplace();
    }
  }
  if (kitchen.last_day) {
    outcome(archive, *kitchen.last_day);
  }
  count = sources.size();
  archive.value(count);
  if constexpr (std::is_same_v<Archive, Reader>) {
    if (!archive.good || count > MAX_ITEMS) {
      archive.good = false;
      return;
    }
    sources.resize(count);
  }
  for (auto &source : sources) {
    stock(archive, source);
  }
}

} // namespace

std::optional<DaySave> DaySave::capture(const Simulation &simulation,
                                        const Workshop &workshop,
                                        const Exploration &exploration,
                                        ControlMap control) {
  if (simulation.world.phase != Phase::Workshop ||
      simulation.world.remaining_exploration != simulation.daily_budget ||
      workshop.state.cooking ||
      workshop.state.remaining_base != workshop.config.base_time) {
    return std::nullopt;
  }
  DaySave save{workshop.generator};
  save.progression = simulation.snapshot();
  save.kitchen = workshop.snapshot();
  save.sources = exploration.config.sources;
  save.touches = exploration.touch_counts;
  save.random = workshop.generator;
  save.players = control;
  save.exploration_budget = simulation.daily_budget;
  save.cook_time = workshop.config.cook_time;
  return save;
}

std::string DaySave::encode() const {
  Writer archive;
  archive.stream << "MME-DAY 1 ";
  archive.value(exploration_budget.count());
  archive.value(cook_time.count());
  archive.value(kitchen.remaining_base ? kitchen.remaining_base->count() : 0);
  // The same field order serves both directions of the file boundary.
  auto copy = *this;
  state(archive, copy.progression, copy.kitchen, copy.sources, copy.touches,
        copy.players);
  archive.stream << random << '\n';
  return archive.stream.str();
}

std::optional<DaySave> DaySave::parse(std::string_view bytes,
                                      const Content &content) {
  if (bytes.size() > MAX_BYTES) {
    return std::nullopt;
  }
  Reader archive{.stream = std::istringstream{std::string{bytes}}};
  std::string magic;
  unsigned version = 0;
  archive.stream >> magic;
  archive.value(version);
  if (magic != "MME-DAY" || version != 1) {
    return std::nullopt;
  }
  DaySave save{std::mt19937{content.run.seed}};
  std::int64_t budget = 0;
  std::int64_t cook = 0;
  std::int64_t base = 0;
  archive.value(budget);
  archive.value(cook);
  archive.value(base);
  save.exploration_budget = std::chrono::microseconds{budget};
  save.cook_time = std::chrono::microseconds{cook};
  if (base != 0) {
    save.kitchen.remaining_base = std::chrono::microseconds{base};
  }
  state(archive, save.progression, save.kitchen, save.sources, save.touches,
        save.players);
  if (!archive.good || !(archive.stream >> save.random)) {
    return std::nullopt;
  }
  archive.stream >> std::ws;
  if (!archive.stream.eof() || !save.valid(content)) {
    return std::nullopt;
  }
  for (const auto &recipe : content.companion.workshop.recipes) {
    save.kitchen.ingredient_slots =
        std::max(save.kitchen.ingredient_slots,
                 static_cast<std::uint32_t>(recipe.ingredients.size()));
  }
  save.progression.inventory_slots = content.run.inventory_slots;
  save.progression.fridge_slots = content.run.fridge_slots;
  save.progression.remaining_exploration = content.run.exploration_budget;
  save.progression.world_time =
      content.run.exploration_budget * (save.progression.day - 1);
  for (std::size_t index = 0; index < save.sources.size(); ++index) {
    auto items = std::move(save.sources.at(index).items);
    save.sources.at(index) = content.companion.exploration.sources.at(index);
    save.sources.at(index).items = std::move(items);
  }
  return save;
}

bool DaySave::valid(const Content &content) const {
  const auto &config = content.companion.workshop;
  const auto &stock = content.companion.exploration.sources;
  const auto known_ingredient = [&content](IngredientId id) {
    return std::ranges::any_of(content.objects, [id](const auto &object) {
      return object.ingredient == id;
    });
  };
  const auto known_food = [&config](FoodId id) {
    return std::ranges::any_of(config.recipes, [id](const Recipe &recipe) {
      return recipe.food == id;
    });
  };
  const auto ingredients = [&known_ingredient](const auto &items) {
    return std::ranges::all_of(items, known_ingredient);
  };
  const auto foods = [&known_food](const auto &items) {
    return std::ranges::all_of(items, known_food);
  };
  if (exploration_budget != content.run.exploration_budget ||
      cook_time != config.cook_time ||
      kitchen.remaining_base != config.base_time || progression.day == 0 ||
      progression.day > TEMPORARY_ENDING.vomited_out_day ||
      progression.acid.value >= content.run.full_acid.value ||
      (players.chef == Controller::Ai && players.appraiser == Controller::Ai) ||
      kitchen.satisfied_days >= TEMPORARY_ENDING.satisfied_days ||
      kitchen.satisfied_days >= progression.day ||
      !ingredients(progression.chef.inventory) ||
      !ingredients(progression.appraiser.inventory) ||
      !ingredients(progression.fridge) || !ingredients(kitchen.identified) ||
      !ingredients(kitchen.eaten_ingredients) || !foods(kitchen.eaten_foods) ||
      !foods(kitchen.rejected_foods) ||
      progression.chef.inventory.size() > content.run.inventory_slots ||
      progression.appraiser.inventory.size() > content.run.inventory_slots ||
      progression.fridge.size() > content.run.fridge_slots ||
      sources.size() != stock.size()) {
    return false;
  }
  if (!std::ranges::all_of(kitchen.foods, [&known_food](const Dish &food) {
        return known_food(food.food);
      })) {
    return false;
  }
  if (kitchen.last_day.has_value() != (progression.day > 1)) {
    return false;
  }
  if (kitchen.last_day) {
    const auto &last = *kitchen.last_day;
    if (last.completed_day != progression.day - 1 ||
        last.fed.has_value() != (last.reaction != MonsterReaction::Unfed) ||
        (last.fed && (!known_food(last.fed->dish.food) ||
                      last.score != last.fed->score.total))) {
      return false;
    }
  }
  for (std::size_t index = 0; index < sources.size(); ++index) {
    const auto &source = sources.at(index);
    const auto &expected = stock.at(index);
    if (source.id != expected.id || source.items.size() > expected.capacity ||
        !ingredients(source.items)) {
      return false;
    }
  }
  return true;
}

void DaySave::restore(Simulation &simulation, Workshop &workshop,
                      Exploration &exploration) const {
  simulation.world = progression;
  workshop.state = kitchen;
  workshop.generator = random;
  exploration.config.sources = sources;
  exploration.touch_counts = touches;
  exploration.synchronize();
  exploration.advance_world();
}

ControlMap DaySave::control() const { return players; }
std::uint32_t DaySave::seed() const { return progression.seed; }

} // namespace neocity::survival
