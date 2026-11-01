#include "survival.hpp"

#include "../rendering/survival.hpp"

#include <godot_cpp/variant/vector2.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace neocity::hud {
namespace {

using godot::String;
using godot::Vector2;
using rendering::color;
using rendering::rect;
using survival::Actor;
using survival::GameView;
using survival::GridPosition;
using survival::IngredientId;
using survival::Location;
using survival::Phase;

constexpr std::array ACTORS{Actor::Chef, Actor::Appraiser};
/// The acid level drawn as a full gauge; the rules have no such cap.
constexpr std::uint32_t FULL_ACID = 100;
constexpr float CARD_Y = 588.0F;
constexpr float CARD_WIDTH = 276.0F;
constexpr float CARD_HEIGHT = 84.0F;
constexpr float NEXT_Y = 674.0F;
constexpr float PANEL_Y = 318.0F;
constexpr float PANEL_HEIGHT = 348.0F;
constexpr float PANEL_WIDTH = 414.0F;
/// The workshop's event log, kept above the monster's bowl at y 157.
constexpr float EVENT_Y = 64.0F;
constexpr float EVENT_HEIGHT = 90.0F;

/// The key each action is bound to in project.godot, one entry per key set.
struct KeyLabels {
  const char *move;
  const char *run;
  const char *travel;
  const char *interact;
  const char *replace_tool;
  const char *feed;
  const char *end_day;
  const char *switch_actor;
  const char *leave;
};

constexpr std::array KEY_LABELS{KeyLabels{.move = "WASD",
                                          .run = "Shift",
                                          .travel = "Q",
                                          .interact = "E",
                                          .replace_tool = "R",
                                          .feed = "F",
                                          .end_day = "G",
                                          .switch_actor = "Tab",
                                          .leave = "X"},
                                KeyLabels{.move = "IJKL",
                                          .run = "H",
                                          .travel = "U",
                                          .interact = "O",
                                          .replace_tool = "P",
                                          .feed = "[",
                                          .end_day = "]",
                                          .switch_actor = "B",
                                          .leave = "N"}};

/// A human chef always holds the first key set's up and down.
constexpr const char *CHOOSE_DISH_KEYS = "W/S";

String from(std::string_view text) {
  return String::utf8(text.data(), static_cast<std::int64_t>(text.size()));
}

String number(std::uint64_t value) {
  return String::num_int64(static_cast<std::int64_t>(value));
}

String key(std::string_view label) { return "[" + from(label) + "] "; }

const char *actor_name(Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return "Chef";
  case Actor::Appraiser:
    return "Appraiser";
  }
  std::abort();
}

std::size_t index_of(Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return 0;
  case Actor::Appraiser:
    return 1;
  }
  std::abort();
}

const survival::ActorSnapshot &state_of(const GameView &view, Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return view.progression.chef;
  case Actor::Appraiser:
    return view.progression.appraiser;
  }
  std::abort();
}

std::uint32_t stamina_of(const GameView &view, Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return view.exploration.chef.stamina;
  case Actor::Appraiser:
    return view.exploration.appraiser.stamina;
  }
  std::abort();
}

survival::ToolState role_tool_state(const GameView &view, Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return view.workshop.spatula;
  case Actor::Appraiser:
    return view.workshop.magnifier;
  }
  std::abort();
}

const char *role_tool_name(Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return "Spatula";
  case Actor::Appraiser:
    return "Magnifier";
  }
  std::abort();
}

survival::ToolId role_replacement(const survival::WorkshopConfig &workshop,
                                  Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return workshop.spatula;
  case Actor::Appraiser:
    return workshop.magnifier;
  }
  std::abort();
}

const char *tool_condition(survival::ToolState state) {
  switch (state) {
  case survival::ToolState::Intact:
    return "intact, 90%";
  case survival::ToolState::Damaged:
    return "DAMAGED, 40%";
  }
  std::abort();
}

String clock(std::chrono::microseconds remaining) {
  const auto seconds = std::chrono::ceil<std::chrono::seconds>(remaining);
  const auto whole = static_cast<std::uint64_t>(seconds.count());
  return number(whole / 60) + (whole % 60 < 10 ? ":0" : ":") +
         number(whole % 60);
}

bool carried(const GameView &view, survival::ToolId tool) {
  return std::ranges::any_of(ACTORS, [&](Actor actor) {
    const auto &tools = state_of(view, actor).tools;
    return std::ranges::find(tools, tool) != tools.end();
  });
}

bool identified(const GameView &view, IngredientId ingredient) {
  const auto &known = view.workshop.identified;
  return std::ranges::find(known, ingredient) != known.end();
}

/// Ingredients the two actors carry together, by identity value.
std::map<std::uint32_t, std::uint32_t> pack_of(const GameView &view) {
  std::map<std::uint32_t, std::uint32_t> pack;
  for (const auto actor : ACTORS) {
    for (const auto ingredient : state_of(view, actor).inventory) {
      ++pack[ingredient.value];
    }
  }
  return pack;
}

String offered_step(const survival::Action &action, const KeyLabels &keys) {
  return std::visit(
      [&keys](const auto &offer) -> String {
        using Offer = std::decay_t<decltype(offer)>;
        if constexpr (std::is_same_v<Offer, survival::Collect>) {
          return key(keys.interact) + "Collect from " +
                 from(survival::object_name(offer.source));
        } else if constexpr (std::is_same_v<Offer, survival::Repair>) {
          return key(keys.interact) + "Repair " +
                 from(survival::object_name(offer.target));
        } else if constexpr (std::is_same_v<Offer, survival::Appraise>) {
          return key(keys.interact) + "Appraise " +
                 from(survival::ingredient_name(offer.ingredient));
        } else if constexpr (std::is_same_v<Offer, survival::Cook>) {
          return key(keys.interact) + "Cook " +
                 from(survival::food_name(offer.food));
        } else if constexpr (std::is_same_v<Offer, survival::Feed>) {
          return key(keys.feed) + "Feed " +
                 from(survival::food_name(offer.food)) +
                 " to the monster: this ends the day";
        } else {
          return "";
        }
      },
      action);
}

/// Where `actor` has something to do: a stocked source that is supplying, or a
/// broken passage it carries the tool for.
std::vector<survival::WorldArea> errand_areas(const GameView &view,
                                              Actor actor) {
  std::vector<survival::WorldArea> areas;
  const auto &objects = view.world.objects;
  for (const auto &stock : view.exploration.sources) {
    const auto object =
        std::ranges::find(objects, stock.id, &survival::ObjectSnapshot::id);
    if (stock.remaining > 0 && object != objects.end() &&
        object->material_available) {
      areas.push_back(object->area);
    }
  }
  const auto &tools = state_of(view, actor).tools;
  for (const auto &object : objects) {
    if (object.state == survival::ObjectState::Destroyed &&
        object.repair_with &&
        std::ranges::find(tools, *object.repair_with) != tools.end()) {
      areas.push_back(object.area);
    }
  }
  return areas;
}

/// Searches outward from `from` over the cells movement allows.
bool errand_in_reach(const SurvivalFrame &frame, Actor actor,
                     GridPosition from) {
  const auto areas = errand_areas(frame.view, actor);
  const auto range =
      frame.content.get().companion.exploration.collection_distance;
  std::set<std::pair<std::int32_t, std::int32_t>> seen{{from.x, from.y}};
  std::deque<GridPosition> frontier{from};
  while (!frontier.empty()) {
    const auto cell = frontier.front();
    frontier.pop_front();
    if (std::ranges::any_of(areas, [cell, range](const auto &area) {
          return survival::distance(cell, area) <= range;
        })) {
      return true;
    }
    for (const auto next : {GridPosition{.x = cell.x - 1, .y = cell.y},
                            GridPosition{.x = cell.x + 1, .y = cell.y},
                            GridPosition{.x = cell.x, .y = cell.y - 1},
                            GridPosition{.x = cell.x, .y = cell.y + 1}}) {
      if (survival::cell_at(frame.view.world, next) ==
              survival::Cell::Supported &&
          seen.insert({next.x, next.y}).second) {
        frontier.push_back(next);
      }
    }
  }
  return false;
}

GridPosition position_of(const GameView &view, Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return view.exploration.chef.position;
  case Actor::Appraiser:
    return view.exploration.appraiser.position;
  }
  std::abort();
}

/// Whether setting out today would find anything to do.
bool worth_setting_out(const SurvivalFrame &frame) {
  const auto entry = frame.content.get().companion.exploration.entry;
  return frame.view.progression.remaining_exploration.count() > 0 &&
         std::ranges::any_of(ACTORS, [&frame, entry](Actor actor) {
           return errand_in_reach(frame, actor, entry);
         });
}

String city_step(const SurvivalFrame &frame, Actor actor,
                 const KeyLabels &keys) {
  const auto &offer = frame.offers.at(index_of(actor)).interact;
  if (offer) {
    return offered_step(*offer, keys);
  }
  if (!errand_in_reach(frame, actor, position_of(frame.view, actor))) {
    return "Nothing is within reach for now. " + from(keys.travel) +
           " returns to base.";
  }
  return "Walk to a gold-framed marker and press " + from(keys.interact) +
         ". Stay on the paved ground: a fall sends you to base. " +
         from(keys.travel) + " returns.";
}

std::optional<String> role_step(const SurvivalFrame &frame, Actor actor,
                                const char *busy) {
  const auto keys = survival::key_set(frame.control, actor);
  if (!keys) {
    return busy;
  }
  const auto &offer = frame.offers.at(index_of(actor)).interact;
  if (offer) {
    return offered_step(*offer, KEY_LABELS.at(*keys));
  }
  return std::nullopt;
}

std::optional<String> replacement_step(const SurvivalFrame &frame) {
  const auto &workshop = frame.content.get().companion.workshop;
  for (const auto actor : ACTORS) {
    const auto keys = survival::key_set(frame.control, actor);
    if (keys &&
        role_tool_state(frame.view, actor) == survival::ToolState::Damaged &&
        carried(frame.view, role_replacement(workshop, actor))) {
      return key(KEY_LABELS.at(*keys).replace_tool) + "Replace the damaged " +
             role_tool_name(actor);
    }
  }
  return std::nullopt;
}

bool has_unidentified(const GameView &view) {
  const auto pack = pack_of(view);
  return std::ranges::any_of(pack, [&view](const auto &entry) {
    return !identified(view, IngredientId{entry.first});
  });
}

std::optional<String> workshop_craft_step(const SurvivalFrame &frame) {
  if (has_unidentified(frame.view)) {
    if (auto step = role_step(frame, Actor::Appraiser,
                              "The appraiser is identifying your finds.")) {
      return step;
    }
  }
  if (const auto step = replacement_step(frame)) {
    return step;
  }
  const auto chef_keys = survival::key_set(frame.control, Actor::Chef);
  if (chef_keys && frame.view.workshop.foods.empty()) {
    const auto &offer = frame.offers.at(index_of(Actor::Chef)).interact;
    if (offer) {
      return offered_step(*offer, KEY_LABELS.at(*chef_keys));
    }
  }
  // Feeding ends the day, and a stored dish keeps while the city still gives.
  if (worth_setting_out(frame)) {
    return std::nullopt;
  }
  for (const auto actor : ACTORS) {
    const auto keys = survival::key_set(frame.control, actor);
    const auto &offer = frame.offers.at(index_of(actor)).feed;
    if (keys && offer) {
      return offered_step(*offer, KEY_LABELS.at(*keys));
    }
  }
  return std::nullopt;
}

String workshop_step(const SurvivalFrame &frame, const KeyLabels &keys) {
  if (auto step = workshop_craft_step(frame)) {
    return *step;
  }
  const auto &progression = frame.view.progression;
  if (progression.remaining_exploration.count() > 0) {
    return key(keys.travel) + "Set out into the city: " +
           clock(progression.remaining_exploration) + " left today.";
  }
  return key(keys.end_day) + "End the day unfed: acid rises by " +
         number(frame.content.get().companion.unfed_penalty.value) + ".";
}

void bar(godot::Node2D &canvas, float x, float y, float width, float height,
         float fill, godot::Color tint) {
  canvas.draw_rect(rect(x, y, width, height), color(44, 40, 66, 1.0F));
  canvas.draw_rect(rect(x, y, width * std::clamp(fill, 0.0F, 1.0F), height),
                   tint);
}

void frame_box(godot::Node2D &canvas, float x, float y, float width,
               float height) {
  canvas.draw_rect(rect(x, y, width, height), color(14, 12, 26, 0.94F));
  canvas.draw_rect(rect(x, y, width, height), color(120, 104, 150, 1.0F), false,
                   2.0F);
}

void paint_top_bar(godot::Node2D &canvas,
                   const rendering::AscentDrawing &drawing,
                   const SurvivalFrame &frame) {
  const auto tones = rendering::palette();
  const auto &progression = frame.view.progression;
  canvas.draw_rect(rect(0.0F, 0.0F, 1280.0F, 62.0F), color(14, 12, 26, 1.0F));
  drawing.text(canvas, 24.0F, 42.0F, "DAY " + number(progression.day), 30,
               tones.gold);
  drawing.text(canvas, 190.0F, 40.0F, "ACID", 24, color(150, 255, 120, 1.0F));
  bar(canvas, 262.0F, 20.0F, 440.0F, 24.0F,
      static_cast<float>(progression.acid.value) /
          static_cast<float>(FULL_ACID),
      color(90, 226, 80, 1.0F));
  drawing.text(canvas, 716.0F, 40.0F,
               number(progression.acid.value) + " / " + number(FULL_ACID), 22,
               tones.white);
  drawing.text(canvas, 880.0F, 40.0F, "TIME LEFT", 20, tones.muted);
  drawing.text(canvas, 990.0F, 44.0F, clock(progression.remaining_exploration),
               32, tones.white);
  drawing.text(canvas, 1140.0F, 40.0F, "M: main menu", 16, tones.muted);
}

void paint_hearts(godot::Node2D &canvas, float x, float y,
                  std::uint8_t health) {
  for (std::uint8_t slot = 0; slot < 3; ++slot) {
    const float left = x + static_cast<float>(slot) * 22.0F;
    const auto tint =
        slot < health ? color(235, 64, 84, 1.0F) : color(60, 50, 70, 1.0F);
    canvas.draw_circle(Vector2(left + 5.0F, y + 5.0F), 5.0F, tint);
    canvas.draw_circle(Vector2(left + 13.0F, y + 5.0F), 5.0F, tint);
    canvas.draw_rect(rect(left + 2.0F, y + 6.0F, 14.0F, 7.0F), tint);
  }
}

void paint_actor_card(godot::Node2D &canvas,
                      const rendering::AscentDrawing &drawing,
                      const SurvivalFrame &frame, Actor actor, float x) {
  const auto tones = rendering::palette();
  const auto &view = frame.view;
  const auto &state = state_of(view, actor);
  const auto keys = survival::key_set(frame.control, actor);
  const auto stamina =
      static_cast<float>(stamina_of(view, actor)) /
      static_cast<float>(
          frame.content.get().companion.exploration.maximum_stamina);
  frame_box(canvas, x, CARD_Y, CARD_WIDTH, CARD_HEIGHT);
  drawing.sprite(canvas, actor == Actor::Chef ? 0 : 2,
                 rect(x + 4.0F, CARD_Y + 6.0F, 62.0F, 72.0F), false);
  const String who = keys ? "P" + number(*keys + 1) : String("AI");
  drawing.text(canvas, x + 72.0F, CARD_Y + 22.0F,
               String(actor_name(actor)) + "  " + who, 18, tones.white);
  drawing.text(canvas, x + 196.0F, CARD_Y + 22.0F,
               state.location == Location::City ? "in city" : "at base", 15,
               state.location == Location::City ? tones.cyan : tones.gold);
  paint_hearts(canvas, x + 72.0F, CARD_Y + 29.0F, state.health);
  bar(canvas, x + 146.0F, CARD_Y + 31.0F, 120.0F, 10.0F, stamina,
      color(96, 200, 250, 1.0F));
  const auto tool = role_tool_state(view, actor);
  drawing.text(canvas, x + 72.0F, CARD_Y + 60.0F,
               String(role_tool_name(actor)) + " " + tool_condition(tool), 15,
               tool == survival::ToolState::Intact
                   ? tones.muted
                   : color(255, 120, 110, 1.0F));
  std::map<std::uint32_t, std::uint32_t> spares;
  for (const auto spare : state.tools) {
    ++spares[spare.value];
  }
  String tools;
  for (const auto &[value, count] : spares) {
    tools += from(survival::tool_name(survival::ToolId{value})) +
             (count > 1 ? " x" + number(count) + "  " : String("  "));
  }
  drawing.text(canvas, x + 72.0F, CARD_Y + 77.0F,
               spares.empty() ? String("No spare tools") : tools, 12,
               tones.muted);
}

void paint_pack(godot::Node2D &canvas, const rendering::AscentDrawing &drawing,
                const GameView &view, float x) {
  const auto tones = rendering::palette();
  frame_box(canvas, x, CARD_Y, 276.0F, CARD_HEIGHT);
  drawing.text(canvas, x + 10.0F, CARD_Y + 18.0F, "PACK", 15, tones.gold);
  drawing.text(canvas, x + 62.0F, CARD_Y + 18.0F, "? = not yet identified", 13,
               tones.muted);
  const auto pack = pack_of(view);
  if (pack.empty()) {
    drawing.text(canvas, x + 10.0F, CARD_Y + 44.0F, "Empty", 15, tones.muted);
  }
  std::size_t index = 0;
  for (const auto &[value, count] : pack) {
    const auto ingredient = IngredientId{value};
    const float column = static_cast<float>(index % 3) * 90.0F;
    const std::size_t rows_above = index / 3;
    const float row = static_cast<float>(rows_above) * 19.0F;
    const bool known = identified(view, ingredient);
    drawing.text(canvas, x + 10.0F + column, CARD_Y + 38.0F + row,
                 from(survival::ingredient_name(ingredient)) + " " +
                     number(count) + (known ? "" : "?"),
                 13, known ? tones.white : tones.gold);
    ++index;
  }
}

void paint_events(godot::Node2D &canvas,
                  const rendering::AscentDrawing &drawing,
                  const SurvivalFrame &frame, float x, float y, float height) {
  const auto tones = rendering::palette();
  if (frame.events.empty()) {
    return;
  }
  frame_box(canvas, x, y, 1280.0F - x - 8.0F, height);
  float line = y + 19.0F;
  for (const auto &event : frame.events) {
    drawing.text(canvas, x + 10.0F, line, describe(event), 15, tones.white);
    line += 20.0F;
  }
}

String key_legend(const SurvivalFrame &frame) {
  const bool city = frame.view.progression.phase == Phase::Exploration;
  const auto legend = [city](const KeyLabels &keys, bool chef) {
    if (city) {
      return from(keys.move) + " move  " + from(keys.run) + " run  " +
             from(keys.interact) + " use  " + from(keys.travel) +
             " return to base";
    }
    return (chef ? from(CHOOSE_DISH_KEYS) + " choose dish  " : String()) +
           from(keys.interact) + " appraise/cook  " + from(keys.feed) +
           " feed  " + from(keys.replace_tool) + " replace tool  " +
           from(keys.travel) + " set out  " + from(keys.end_day) +
           " end day unfed";
  };
  const auto chef = survival::key_set(frame.control, Actor::Chef);
  const auto appraiser = survival::key_set(frame.control, Actor::Appraiser);
  if (chef && appraiser) {
    return "P1  " + legend(KEY_LABELS.at(0), true) + "    P2  " +
           legend(KEY_LABELS.at(1), false);
  }
  const auto &keys = KEY_LABELS.at(0);
  return legend(keys, chef.has_value()) + "   " + from(keys.switch_actor) +
         " swap actor";
}

void paint_next_step(godot::Node2D &canvas,
                     const rendering::AscentDrawing &drawing,
                     const SurvivalFrame &frame) {
  const auto tones = rendering::palette();
  canvas.draw_rect(rect(0.0F, NEXT_Y, 1280.0F, 720.0F - NEXT_Y),
                   color(14, 12, 26, 1.0F));
  drawing.text(canvas, 14.0F, NEXT_Y + 22.0F, "NEXT", 18, tones.cyan);
  drawing.text(canvas, 70.0F, NEXT_Y + 22.0F, next_step(frame), 18,
               tones.white);
  drawing.text(canvas, 14.0F, NEXT_Y + 40.0F, key_legend(frame), 14,
               tones.muted);
}

/// The panel's last line: whose job it is, in the keys of the player doing it.
void paint_panel_job(godot::Node2D &canvas,
                     const rendering::AscentDrawing &drawing, float x,
                     const String &job) {
  drawing.text(canvas, x + 14.0F, PANEL_Y + PANEL_HEIGHT - 16.0F, job, 17,
               rendering::palette().cyan);
}

void paint_appraise_panel(godot::Node2D &canvas,
                          const rendering::AscentDrawing &drawing,
                          const SurvivalFrame &frame, float x) {
  const auto tones = rendering::palette();
  const auto &view = frame.view;
  const auto keys = survival::key_set(frame.control, Actor::Appraiser);
  frame_box(canvas, x, PANEL_Y, PANEL_WIDTH, PANEL_HEIGHT);
  paint_panel_job(canvas, drawing, x,
                  keys ? key(KEY_LABELS.at(*keys).interact) +
                             "Appraise the next unknown ingredient"
                       : String("The AI appraiser works here."));
  drawing.text(canvas, x + 14.0F, PANEL_Y + 30.0F, "APPRAISE", 24, tones.gold);
  drawing.text(canvas, x + 150.0F, PANEL_Y + 30.0F,
               String("Magnifier ") +
                   tool_condition(role_tool_state(view, Actor::Appraiser)),
               16, tones.muted);
  float y = PANEL_Y + 60.0F;
  const auto pack = pack_of(view);
  if (pack.empty()) {
    drawing.text(canvas, x + 14.0F, y, "Nothing carried. Find ingredients.", 18,
                 tones.muted);
  }
  for (const auto &[value, count] : pack) {
    const auto ingredient = IngredientId{value};
    const bool known = identified(view, ingredient);
    drawing.text(canvas, x + 14.0F, y,
                 from(survival::ingredient_name(ingredient)) + " x" +
                     number(count),
                 18, tones.white);
    drawing.text(canvas, x + 200.0F, y, known ? "identified" : "unknown", 18,
                 known ? tones.cyan : tones.gold);
    y += 25.0F;
  }
}

/// The cook panel's last line: what the chef's key would really cook.
String cook_job(const SurvivalFrame &frame) {
  const auto keys = survival::key_set(frame.control, Actor::Chef);
  if (!keys) {
    return "The AI chef works here.";
  }
  const auto &offer = frame.offers.at(index_of(Actor::Chef)).interact;
  return offer ? offered_step(*offer, KEY_LABELS.at(*keys)) + "   " +
                     key(CHOOSE_DISH_KEYS) + "Choose"
               : String("No dish can be cooked yet.");
}

/// The dish the chef's key would cook, if any.
std::optional<survival::FoodId> offered_dish(const SurvivalFrame &frame) {
  const auto &offer = frame.offers.at(index_of(Actor::Chef)).interact;
  const auto *cook = offer ? std::get_if<survival::Cook>(&*offer) : nullptr;
  return cook != nullptr ? std::optional{cook->food} : std::nullopt;
}

void paint_cook_panel(godot::Node2D &canvas,
                      const rendering::AscentDrawing &drawing,
                      const SurvivalFrame &frame, float x) {
  const auto tones = rendering::palette();
  const auto &view = frame.view;
  frame_box(canvas, x, PANEL_Y, PANEL_WIDTH, PANEL_HEIGHT);
  paint_panel_job(canvas, drawing, x, cook_job(frame));
  drawing.text(canvas, x + 14.0F, PANEL_Y + 30.0F, "COOK", 24, tones.gold);
  drawing.text(canvas, x + 150.0F, PANEL_Y + 30.0F,
               String("Spatula ") +
                   tool_condition(role_tool_state(view, Actor::Chef)),
               16, tones.muted);
  float y = PANEL_Y + 60.0F;
  const auto offered = offered_dish(frame);
  for (const auto &recipe : frame.content.get().companion.workshop.recipes) {
    const auto missing = recipe_status(view, recipe);
    if (offered == recipe.food) {
      drawing.text(canvas, x + 3.0F, y, ">", 18, tones.gold);
    }
    const auto &eaten = view.workshop.eaten_foods;
    const bool repeat = std::ranges::find(eaten, recipe.food) != eaten.end();
    drawing.text(canvas, x + 14.0F, y, from(survival::food_name(recipe.food)),
                 18, tones.white);
    drawing.text(canvas, x + 170.0F, y,
                 missing.is_empty() ? String("ready") : missing, 16,
                 missing.is_empty() ? tones.cyan : tones.muted);
    if (repeat) {
      drawing.text(canvas, x + 340.0F, y, "eaten", 16, tones.gold);
    }
    y += 25.0F;
  }
}

const char *reaction_text(survival::MonsterReaction reaction) {
  switch (reaction) {
  case survival::MonsterReaction::Unfed:
    return "Unfed";
  case survival::MonsterReaction::Disliked:
    return "It HATED that";
  case survival::MonsterReaction::Unsatisfied:
    return "Unsatisfied";
  case survival::MonsterReaction::Satisfied:
    return "Satisfied";
  }
  std::abort();
}

void paint_feed_panel(godot::Node2D &canvas,
                      const rendering::AscentDrawing &drawing,
                      const SurvivalFrame &frame, float x) {
  const auto tones = rendering::palette();
  const auto &workshop = frame.view.workshop;
  const auto &tuning = frame.content.get().companion.workshop;
  const auto &keys = KEY_LABELS.at(0);
  frame_box(canvas, x, PANEL_Y, PANEL_WIDTH, PANEL_HEIGHT);
  paint_panel_job(
      canvas, drawing, x,
      key(keys.feed) + "Feed   " + key(keys.end_day) +
          "End the day unfed: acid +" +
          number(frame.content.get().companion.unfed_penalty.value));
  drawing.text(canvas, x + 14.0F, PANEL_Y + 30.0F, "FEED", 24, tones.gold);
  drawing.text(canvas, x + 150.0F, PANEL_Y + 30.0F,
               "Needs a score of " + number(tuning.satisfying_score), 16,
               tones.muted);
  float y = PANEL_Y + 60.0F;
  if (workshop.foods.empty()) {
    drawing.text(canvas, x + 14.0F, y, "No dish stored. The chef must cook.",
                 18, tones.muted);
    y += 25.0F;
  }
  for (const auto food : workshop.foods) {
    drawing.text(canvas, x + 14.0F, y, from(survival::food_name(food)), 18,
                 tones.cyan);
    y += 25.0F;
  }
  drawing.text(canvas, x + 14.0F, y + 12.0F,
               "It craves new dishes and ingredients.", 16, tones.muted);
  drawing.text(canvas, x + 14.0F, y + 34.0F,
               "A rejected meal raises acid and wrecks the city.", 16,
               tones.muted);
  if (workshop.last_day) {
    const auto &last = *workshop.last_day;
    drawing.text(canvas, x + 14.0F, y + 70.0F,
                 "Day " + number(last.completed_day) + ": " +
                     reaction_text(last.reaction),
                 20, tones.white);
    drawing.text(canvas, x + 14.0F, y + 94.0F,
                 "Score " + number(last.score) + ", acid +" +
                     number(last.acid_rise.value),
                 18, tones.muted);
  }
}

void paint_workshop_screen(godot::Node2D &canvas,
                           const rendering::AscentDrawing &drawing,
                           const SurvivalFrame &frame) {
  rendering::paint_workshop(canvas, drawing,
                            !frame.view.workshop.foods.empty());
  paint_appraise_panel(canvas, drawing, frame, 10.0F);
  paint_cook_panel(canvas, drawing, frame, 433.0F);
  paint_feed_panel(canvas, drawing, frame, 856.0F);
  paint_events(canvas, drawing, frame, 840.0F, EVENT_Y, EVENT_HEIGHT);
}

void paint_city_screen(godot::Node2D &canvas,
                       const rendering::AscentDrawing &drawing,
                       const SurvivalFrame &frame) {
  rendering::paint_city(canvas, drawing, frame.content.get(), frame.view,
                        frame.control);
  paint_actor_card(canvas, drawing, frame, Actor::Chef, 8.0F);
  paint_actor_card(canvas, drawing, frame, Actor::Appraiser, 290.0F);
  paint_pack(canvas, drawing, frame.view, 572.0F);
  paint_events(canvas, drawing, frame, 854.0F, CARD_Y, CARD_HEIGHT);
}

String refusal_text(survival::Result result) {
  switch (result) {
  case survival::Result::Applied:
    return "";
  case survival::Result::NoExplorationTime:
    return "No exploration time left today.";
  case survival::Result::NotExploring:
    return "The way out is blocked.";
  case survival::Result::InvalidElapsed:
  case survival::Result::NotInWorkshop:
  case survival::Result::MissingIngredients:
  case survival::Result::MissingTool:
  case survival::Result::Escaped:
    return "That cannot be done now.";
  }
  std::abort();
}

String refusal_text(survival::CollectionResult result) {
  switch (result) {
  case survival::CollectionResult::Applied:
    return "";
  case survival::CollectionResult::Depleted:
    return "That source is empty.";
  case survival::CollectionResult::Unavailable:
    return "That source is gone.";
  case survival::CollectionResult::OutOfRange:
    return "Move closer.";
  case survival::CollectionResult::NotExploring:
  case survival::CollectionResult::UnknownSource:
    return "That cannot be done now.";
  }
  std::abort();
}

String refusal_text(survival::RepairResult result) {
  switch (result) {
  case survival::RepairResult::Applied:
    return "";
  case survival::RepairResult::OutOfRange:
    return "Move closer.";
  case survival::RepairResult::ToolNotCarried:
  case survival::RepairResult::WrongTool:
    return "That repair needs the right tool.";
  case survival::RepairResult::UnknownObject:
  case survival::RepairResult::NotDamaged:
  case survival::RepairResult::NotRepairable:
  case survival::RepairResult::NotExploring:
    return "That cannot be done now.";
  }
  std::abort();
}

String refusal_text(survival::WorkshopResult result) {
  switch (result) {
  case survival::WorkshopResult::Applied:
  case survival::WorkshopResult::Failed:
    return "";
  case survival::WorkshopResult::NotInWorkshop:
    return "Both actors must be at base.";
  case survival::WorkshopResult::ToolIntact:
    return "That tool is not damaged.";
  case survival::WorkshopResult::NotCarried:
    return "No replacement is carried.";
  case survival::WorkshopResult::NoFood:
    return "No dish is stored.";
  case survival::WorkshopResult::WrongRole:
  case survival::WorkshopResult::AlreadyIdentified:
  case survival::WorkshopResult::NotIdentified:
  case survival::WorkshopResult::UnknownRecipe:
    return "That cannot be done now.";
  }
  std::abort();
}

bool workshop_failed(const survival::ActionResult &result) {
  const auto *workshop = std::get_if<survival::WorkshopResult>(&result);
  return workshop != nullptr && *workshop == survival::WorkshopResult::Failed;
}

String done_text(const ActionEvent &event) {
  const String who = actor_name(event.actor);
  const bool failed = workshop_failed(event.result);
  return std::visit(
      [&](const auto &action) -> String {
        using Done = std::decay_t<decltype(action)>;
        if constexpr (std::is_same_v<Done, survival::Enter>) {
          return who + String(" set out.");
        } else if constexpr (std::is_same_v<Done, survival::ReturnToBase>) {
          return who + String(" returned to base.");
        } else if constexpr (std::is_same_v<Done, survival::Collect>) {
          return who + String(" took from ") +
                 from(survival::object_name(action.source)) + ".";
        } else if constexpr (std::is_same_v<Done, survival::Repair>) {
          return who + String(" repaired ") +
                 from(survival::object_name(action.target)) + ".";
        } else if constexpr (std::is_same_v<Done, survival::Appraise>) {
          return from(survival::ingredient_name(action.ingredient)) +
                 (failed ? String(" appraisal failed: try again.")
                         : String(" identified."));
        } else if constexpr (std::is_same_v<Done, survival::Cook>) {
          return from(survival::food_name(action.food)) +
                 (failed ? String(" burned: ingredients lost.")
                         : String(" cooked."));
        } else if constexpr (std::is_same_v<Done, survival::ReplaceTool>) {
          return who + String(" replaced a tool.");
        } else if constexpr (std::is_same_v<Done, survival::Feed>) {
          return "Fed " + from(survival::food_name(action.food)) +
                 " to the monster.";
        } else {
          return "The day ended unfed: acid +" + number(action.penalty.value) +
                 ".";
        }
      },
      event.action);
}

} // namespace

void paint_survival_setup(godot::Node2D &canvas,
                          const rendering::AscentDrawing &drawing) {
  const auto tones = rendering::palette();
  rendering::paint_mouth(canvas, 0.1F);
  canvas.draw_rect(rect(0.0F, 0.0F, 1280.0F, 720.0F), color(14, 12, 26, 0.6F));
  drawing.text(canvas, 130.0F, 170.0F, "WHO IS PLAYING?", 46, tones.white);
  drawing.sprite(canvas, 0, rect(130.0F, 220.0F, 130.0F, 150.0F), false);
  drawing.text(canvas, 130.0F, 410.0F, "1   CHEF", 32, tones.gold);
  drawing.text(canvas, 130.0F, 444.0F, "You cook. AI appraises.", 20,
               tones.white);
  drawing.sprite(canvas, 2, rect(530.0F, 220.0F, 130.0F, 150.0F), false);
  drawing.text(canvas, 530.0F, 410.0F, "2   APPRAISER", 32,
               color(150, 255, 120, 1.0F));
  drawing.text(canvas, 530.0F, 444.0F, "You appraise. AI cooks.", 20,
               tones.white);
  drawing.sprite(canvas, 0, rect(890.0F, 220.0F, 104.0F, 120.0F), false);
  drawing.sprite(canvas, 2, rect(1000.0F, 220.0F, 104.0F, 120.0F), false);
  drawing.text(canvas, 890.0F, 410.0F, "3   TWO PLAYERS", 32, tones.cyan);
  drawing.text(canvas, 890.0F, 444.0F, "P1 chef (WASD), P2 appraiser (IJKL).",
               20, tones.white);
  drawing.text(canvas, 130.0F, 560.0F,
               "Gather by day, appraise and cook at base, and feed the monster "
               "before the acid wrecks the city.",
               22, tones.white);
  drawing.text(canvas, 130.0F, 640.0F, "Press 1, 2 or 3.   M: main menu", 22,
               tones.muted);
}

void paint_survival(godot::Node2D &canvas,
                    const rendering::AscentDrawing &drawing,
                    const SurvivalFrame &frame) {
  const auto &progression = frame.view.progression;
  rendering::paint_mouth(canvas, static_cast<float>(progression.acid.value) /
                                     static_cast<float>(FULL_ACID));
  if (progression.phase == Phase::Exploration) {
    paint_city_screen(canvas, drawing, frame);
  } else {
    paint_workshop_screen(canvas, drawing, frame);
  }
  paint_top_bar(canvas, drawing, frame);
  paint_next_step(canvas, drawing, frame);
}

void paint_cinematic(godot::Node2D &canvas,
                     const rendering::AscentDrawing &drawing,
                     Cinematic cinematic, bool fallback,
                     const SurvivalFrame &frame) {
  canvas.draw_rect(rect(0.0F, 0.0F, 1280.0F, 720.0F), color(8, 6, 16, 1.0F));
  if (!fallback) {
    return;
  }
  const auto tones = rendering::palette();
  const auto &progression = frame.view.progression;
  rendering::paint_mouth(canvas, static_cast<float>(progression.acid.value) /
                                     static_cast<float>(FULL_ACID));
  canvas.draw_rect(rect(0.0F, 0.0F, 1280.0F, 720.0F), color(14, 12, 26, 0.6F));
  drawing.sprite(canvas, 0, rect(130.0F, 300.0F, 130.0F, 150.0F), false);
  drawing.sprite(canvas, 2, rect(280.0F, 300.0F, 130.0F, 150.0F), false);
  switch (cinematic) {
  case Cinematic::Intro:
    drawing.text(canvas, 130.0F, 170.0F, "SWALLOWED", 46, tones.white);
    drawing.text(canvas, 130.0F, 230.0F,
                 "A monster ate the city. Keep it fed, and look for a way out.",
                 24, tones.white);
    break;
  case Cinematic::Ending:
    drawing.text(canvas, 130.0F, 170.0F, "ESCAPED", 46, tones.gold);
    drawing.text(canvas, 130.0F, 230.0F,
                 "The party got out of the monster after " +
                     number(progression.day - 1) + String(" days."),
                 24, tones.white);
    break;
  }
  drawing.text(canvas, 130.0F, 560.0F, "Placeholder card: no cinematic yet.",
               20, tones.muted);
  drawing.text(canvas, 130.0F, 640.0F, "E: skip   M: main menu", 22,
               tones.muted);
}

String recipe_status(const GameView &view, const survival::Recipe &recipe) {
  auto pack = pack_of(view);
  String missing;
  for (const auto ingredient : recipe.ingredients) {
    auto &count = pack[ingredient.value];
    if (count == 0) {
      missing += (missing.is_empty() ? "need " : ", ") +
                 from(survival::ingredient_name(ingredient));
    } else {
      --count;
      if (!identified(view, ingredient)) {
        missing += (missing.is_empty() ? "identify " : ", ") +
                   from(survival::ingredient_name(ingredient));
      }
    }
  }
  return missing;
}

String next_step(const SurvivalFrame &frame) {
  const auto &view = frame.view;
  if (view.progression.phase == Phase::Escaped) {
    return "The party has escaped.";
  }
  if (view.progression.phase == Phase::Workshop) {
    return workshop_step(frame, KEY_LABELS.at(0));
  }
  for (const auto actor : ACTORS) {
    const auto keys = survival::key_set(frame.control, actor);
    if (keys && state_of(view, actor).location == Location::City) {
      return city_step(frame, actor, KEY_LABELS.at(*keys));
    }
  }
  return "Waiting for the party to reach base.";
}

String describe(const Event &event) {
  return std::visit(
      [](const auto &happened) -> String {
        using Happened = std::decay_t<decltype(happened)>;
        if constexpr (std::is_same_v<Happened, ActionEvent>) {
          const String refusal =
              std::visit([](auto result) { return refusal_text(result); },
                         happened.result);
          return refusal.is_empty() ? done_text(happened) : refusal;
        } else if constexpr (std::is_same_v<Happened, HazardEvent>) {
          return String(actor_name(happened.actor)) +
                 (happened.exhausted
                      ? String(" collapsed and was carried to base.")
                      : String(" was burned by an acid drip."));
        } else if constexpr (std::is_same_v<Happened, FallEvent>) {
          return String(actor_name(happened.actor)) +
                 String(" fell and washed back to base.");
        } else if constexpr (std::is_same_v<Happened, TimeUpEvent>) {
          return "Time is up: everyone is back at base.";
        } else {
          const String name = from(survival::object_name(happened.object));
          return happened.state == survival::ObjectState::Destroyed
                     ? name + String(" was destroyed.")
                     : name + String(" appeared.");
        }
      },
      event);
}

} // namespace neocity::hud
