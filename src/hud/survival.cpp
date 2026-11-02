#include "survival.hpp"

#include "../rendering/survival.hpp"

#include <survival/escape.hpp>

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
constexpr float SCREEN_WIDTH = 1280.0F;
constexpr float SCREEN_HEIGHT = 720.0F;
constexpr float SCREEN_EDGE = 1272.0F;
constexpr float INSET = rendering::PANEL_INSET;
/// Plates along the screen's edges run this far past it, so only their inner
/// frame shows there.
constexpr float HANG = 14.0F;
/// The top band's plates hang from the top edge, clear of the map at y 70.
constexpr float BAND_HEIGHT = 80.0F;
constexpr float BAND_LINE = 30.0F;
constexpr float BAND_SECOND_LINE = 44.0F;
constexpr float DAY_RIGHT = 230.0F;
constexpr float DAY_TEXT_X = INSET - HANG;
constexpr int GOAL_SIZE = 13;
constexpr float GOAL_WIDTH = DAY_RIGHT - INSET - DAY_TEXT_X;
constexpr float ACID_X = 236.0F;
constexpr float ACID_Y = 3.0F;
constexpr float ACID_WIDTH = 330.0F;
/// The bar frame's corners, and its dark trough between y 18 and 40.
constexpr float BAR_BORDER = 20.0F;
constexpr float BAR_FRAME_HEIGHT = 56.0F;
constexpr float TIME_X = 572.0F;
/// The hand-off notice shares the time plate's lower line.
constexpr float HAND_OFF_X = TIME_X + INSET;
constexpr int HAND_OFF_SIZE = 13;
constexpr float HAND_OFF_WIDTH = SCREEN_WIDTH + HANG - INSET - HAND_OFF_X;
constexpr float NEXT_Y = 674.0F;
constexpr float NEXT_TEXT_X = 70.0F;
constexpr int NEXT_SIZE = 18;
constexpr float LEGEND_X = 14.0F;
constexpr int LEGEND_SIZE = 14;
/// The city's plates sit below the labels of the lowest map row.
constexpr float CARD_Y = 540.0F;
constexpr float CARD_HEIGHT = 140.0F;
constexpr float CARD_WIDTH = 330.0F;
/// Where a card's words start, right of the portrait.
constexpr float CARD_COLUMN = 68.0F;
constexpr float CARD_TOOL_TEXT = 30.0F;
constexpr float CARD_SPARES = 130.0F;
constexpr int CARD_TOOLS_SIZE = 13;
constexpr float HEART_PITCH = 32.0F;
/// Tool icons are narrow inside their 48 px, so they sit closer than that.
constexpr float TOOL_PITCH = 24.0F;
constexpr float PACK_X = 646.0F;
constexpr float PACK_WIDTH = 326.0F;
constexpr float INGREDIENT_PITCH = 40.0F;
constexpr float CITY_EVENT_X = 972.0F;
constexpr int EVENT_SIZE = 13;
constexpr float EVENT_LINE_HEIGHT = 18.0F;
/// The workshop's panels, under the kitchen floor at y 310.
constexpr float PANEL_Y = 312.0F;
constexpr float PANEL_HEIGHT = 368.0F;
constexpr float APPRAISE_X = -HANG;
constexpr float APPRAISE_WIDTH = 354.0F;
constexpr float COOK_X = 340.0F;
constexpr float COOK_WIDTH = 600.0F;
constexpr float FEED_X = 940.0F;
constexpr float FEED_WIDTH = 354.0F;
constexpr int PANEL_TITLE_SIZE = 22;
constexpr float SLOT = 64.0F;
constexpr float SLOT_PITCH = 72.0F;
constexpr std::size_t SLOTS_PER_ROW = 4;
/// An icon sits in the middle of a slot.
constexpr float SLOT_ICON = 8.0F;
constexpr float JOB_Y = 612.0F;
constexpr float BUTTON_HEIGHT = 44.0F;
constexpr float BUTTON_TEXT_X = 12.0F;
constexpr int JOB_SIZE = 15;
constexpr float COOK_FIRST_ROW = 60.0F;
constexpr float COOK_STATUS_X = 150.0F;
constexpr float COOK_EATEN_X = 436.0F;
constexpr float COOK_TASTE_X = 520.0F;
constexpr int COOK_STATUS_SIZE = 13;
constexpr int COOK_NAME_SIZE = 15;
constexpr float COOK_ROW_HEIGHT = 19.0F;
/// The offered dish's slot, forecast and key, under the recipes.
constexpr float COOK_OFFER_Y = 262.0F;
constexpr float COOK_OFFER_TEXT = 72.0F;
constexpr int FORECAST_SIZE = 13;
constexpr int EATEN_SIZE = 14;
constexpr std::size_t EATEN_PER_LINE = 4;
constexpr float FEED_BUTTON_Y = 562.0F;
constexpr int FEED_NAME_SIZE = 16;
/// The day's result, over the kitchen and clear of the two actors' heads.
constexpr float RESULT_X = 30.0F;
constexpr float RESULT_Y = 60.0F;
constexpr float RESULT_WIDTH = 760.0F;
constexpr float RESULT_LINE_HEIGHT = 21.0F;
constexpr int RESULT_SIZE = 16;
/// The workshop's event log, kept above the monster's bowl at y 157.
constexpr float EVENT_X = 840.0F;
constexpr float EVENT_Y = 46.0F;
constexpr float EVENT_HEIGHT = 114.0F;
constexpr float PAUSE_BOX_X = 150.0F;
constexpr float PAUSE_BOX_Y = 20.0F;
constexpr float PAUSE_BOX_WIDTH = 980.0F;
constexpr float PAUSE_BOX_HEIGHT = 680.0F;
constexpr float PAUSE_TEXT_X = 200.0F;
constexpr float PAUSE_FIRST_X = 680.0F;
constexpr float PAUSE_SECOND_X = 900.0F;
constexpr float PAUSE_ROWS_Y = 170.0F;
constexpr float PAUSE_ROW_HEIGHT = 40.0F;
constexpr int PAUSE_ROW_SIZE = 20;
constexpr float NOTICE_BOX_X = 280.0F;
constexpr float NOTICE_BOX_Y = 230.0F;
constexpr float NOTICE_BOX_WIDTH = 720.0F;
constexpr float NOTICE_BOX_HEIGHT = 250.0F;
constexpr float NOTICE_TEXT_X = 310.0F;
constexpr int INTERRUPTION_TITLE_SIZE = 36;
constexpr int INTERRUPTION_LINE_SIZE = 22;
constexpr const char *NO_SPARE_TOOLS = "No spare tools";
constexpr const char *UNIDENTIFIED_MARK = "? = not yet identified";
constexpr std::array FEED_RULES{
    "New and delicious food holds the acid back.",
    "Anything else raises it. Full acid ends the run."};
constexpr float SETUP_X = 200.0F;
constexpr float SETUP_Y = 170.0F;
constexpr float SETUP_WIDTH = 880.0F;
constexpr float SETUP_HEIGHT = 120.0F;
constexpr float SETUP_PITCH = 130.0F;

/// The key each action is bound to in project.godot, one entry per key set.
struct KeyLabels {
  const char *move;
  const char *every_move;
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
                                          .every_move = "WASD or arrows",
                                          .run = "Shift",
                                          .travel = "Q",
                                          .interact = "E",
                                          .replace_tool = "R",
                                          .feed = "F",
                                          .end_day = "G",
                                          .switch_actor = "Tab",
                                          .leave = "X"},
                                KeyLabels{.move = "IJKL",
                                          .every_move = "IJKL",
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
constexpr const char *EVERY_CHOOSE_DISH_KEY = "W/S or up/down";

/// The menu keys project.godot binds, the same for both players.
struct MenuKeys {
  const char *pause;
  const char *leave;
};

constexpr MenuKeys MENU_KEYS{.pause = "Esc", .leave = "M, then Enter"};

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

GridPosition position_of(const GameView &view, Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return view.exploration.chef.position;
  case Actor::Appraiser:
    return view.exploration.appraiser.position;
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

/// An ingredient, tool or dish name and its icon in the art set.
struct NamedIcon {
  std::string_view name;
  rendering::Image image;
};

/// Icons by the display names `survival::ingredient_name`, `tool_name` and
/// `food_name` give.
constexpr std::array ICONS{
    NamedIcon{.name = "Tomato", .image = rendering::Image::IngredientTomato},
    NamedIcon{.name = "Fish", .image = rendering::Image::IngredientFish},
    NamedIcon{.name = "Kelp", .image = rendering::Image::IngredientKelp},
    NamedIcon{.name = "Mushroom",
              .image = rendering::Image::IngredientMushroom},
    NamedIcon{.name = "Pickles", .image = rendering::Image::IngredientPickles},
    NamedIcon{.name = "Meat", .image = rendering::Image::IngredientMeat},
    NamedIcon{.name = "Truffle", .image = rendering::Image::IngredientTruffle},
    NamedIcon{.name = "Magnifier", .image = rendering::Image::ToolMagnifier},
    NamedIcon{.name = "Spatula", .image = rendering::Image::ToolSpatula},
    NamedIcon{.name = "Wrench", .image = rendering::Image::ToolWrench},
    NamedIcon{.name = "Mushroom Soup",
              .image = rendering::Image::DishMushroomSoup},
    NamedIcon{.name = "Fish Stew", .image = rendering::Image::DishFishStew},
    NamedIcon{.name = "Kelp Wrap", .image = rendering::Image::DishKelpWrap},
    NamedIcon{.name = "Meat Skewer", .image = rendering::Image::DishMeatSkewer},
    NamedIcon{.name = "Truffle Roast",
              .image = rendering::Image::DishTruffleRoast},
    NamedIcon{.name = "Pickle Surprise",
              .image = rendering::Image::DishPickleSurprise},
    NamedIcon{.name = "Sea Salad", .image = rendering::Image::DishSeaSalad},
    NamedIcon{.name = "Fish Pie", .image = rendering::Image::DishFishPie},
    NamedIcon{.name = "Meat Stew", .image = rendering::Image::DishMeatStew},
    NamedIcon{.name = "Truffle Soup",
              .image = rendering::Image::DishTruffleSoup},
    NamedIcon{.name = "Surf and Turf",
              .image = rendering::Image::DishSurfAndTurf}};

/// Draws the 48 px icon of `name` with its top-left corner at `at`; a name the
/// art set has no icon for is written there instead.
void paint_item(godot::Node2D &canvas, const rendering::Art &drawing,
                std::string_view name, Vector2 at) {
  const auto *icon = std::ranges::find(ICONS, name, &NamedIcon::name);
  if (icon != ICONS.end()) {
    drawing.image(canvas, icon->image, at);
    return;
  }
  drawing.text(canvas, at.x, at.y + 28.0F, from(name), CARD_TOOLS_SIZE,
               rendering::palette().white);
}

/// Words on a small opaque plate, for counts and marks drawn over an icon.
void tag(godot::Node2D &canvas, const rendering::Art &drawing, float x,
         float baseline, const String &text, int size, godot::Color tint) {
  const auto height = static_cast<float>(size);
  canvas.draw_rect(rect(x - 2.0F, baseline - height + 1.0F,
                        drawing.text_width(text, size) + 4.0F, height + 3.0F),
                   color(14, 12, 26, 1.0F));
  drawing.text(canvas, x, baseline, text, size, tint);
}

/// A kit button with words centred on its height, dark on the yellow one.
void paint_button(godot::Node2D &canvas, const rendering::Art &drawing,
                  godot::Rect2 area, const String &text, int size,
                  bool marked) {
  const auto tones = rendering::palette();
  drawing.button(canvas, area, marked);
  drawing.text(canvas, area.position.x + BUTTON_TEXT_X,
               area.position.y +
                   (area.size.y + static_cast<float>(size)) / 2.0F - 3.0F,
               text, size, marked ? tones.ink : tones.white);
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

using Cells = std::set<std::pair<std::int32_t, std::int32_t>>;

/// Adds to `seen` every cell movement allows from `from`.
void add_walkable(Cells &seen, const survival::WorldSnapshot &world,
                  GridPosition from) {
  seen.insert({from.x, from.y});
  std::deque<GridPosition> frontier{from};
  while (!frontier.empty()) {
    const auto cell = frontier.front();
    frontier.pop_front();
    for (const auto next : {GridPosition{.x = cell.x - 1, .y = cell.y},
                            GridPosition{.x = cell.x + 1, .y = cell.y},
                            GridPosition{.x = cell.x, .y = cell.y - 1},
                            GridPosition{.x = cell.x, .y = cell.y + 1}}) {
      if (survival::cell_at(world, next) == survival::Cell::Supported &&
          seen.insert({next.x, next.y}).second) {
        frontier.push_back(next);
      }
    }
  }
}

const char *direction_word(survival::Direction direction) {
  switch (direction) {
  case survival::Direction::Idle:
    return "over";
  case survival::Direction::Left:
    return "left";
  case survival::Direction::Right:
    return "right";
  case survival::Direction::Up:
    return "up";
  case survival::Direction::Down:
    return "down";
  }
  std::abort();
}

/// Whether setting out now gives this human something to do. An AI partner's
/// errand does not count: the city line would send the human straight home.
bool worth_setting_out(const SurvivalFrame &frame, Actor actor) {
  return survival::key_set(frame.control, actor).has_value() &&
         survival::worth_entering(frame.content.get().companion, actor,
                                  frame.view);
}

/// Whether setting out today gives any human something to do.
bool worth_setting_out(const SurvivalFrame &frame) {
  return std::ranges::any_of(ACTORS, [&frame](Actor actor) {
    return worth_setting_out(frame, actor);
  });
}

/// Every human's set-out key, so two players each read their own.
String set_out_keys(const SurvivalFrame &frame) {
  String keys;
  for (const auto actor : ACTORS) {
    if (const auto set = survival::key_set(frame.control, actor)) {
      keys += (keys.is_empty() ? String() : String("/")) + "[" +
              from(KEY_LABELS.at(*set).travel) + "]";
    }
  }
  return keys + " ";
}

String collection_clock(const SurvivalFrame &frame) {
  return clock(frame.content.get().companion.exploration.collection_time);
}

String city_step(const SurvivalFrame &frame, Actor actor,
                 const KeyLabels &keys) {
  const auto &offers = frame.offers.at(index_of(actor));
  const bool collecting =
      offers.interact &&
      std::holds_alternative<survival::Collect>(*offers.interact);
  const bool walking =
      offers.errand && offers.errand->step != survival::Direction::Idle;
  // A source passed on the way to what the party lacks would spend the clock,
  // and a repair with no errand left opens no way to stock.
  if (offers.interact && offers.errand && !(collecting && walking)) {
    return offered_step(*offers.interact, keys) +
           (collecting ? " (spends " + collection_clock(frame) + ")"
                       : String());
  }
  if (!offers.errand) {
    return "Nothing is within reach for now. " + from(keys.travel) +
           " returns to base.";
  }
  return "Walk " + String(direction_word(offers.errand->step)) + " to " +
         from(survival::object_name(offers.errand->target)) + ". " +
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

/// Whether a human's feed key would feed a stored dish. A rejected food is not
/// fed, so a store of only those counts as empty.
bool dish_to_feed(const SurvivalFrame &frame) {
  return std::ranges::any_of(
      frame.offers, [](const auto &offers) { return offers.feed.has_value(); });
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
  if (chef_keys && !dish_to_feed(frame)) {
    const auto &offer = frame.offers.at(index_of(Actor::Chef)).interact;
    if (offer) {
      return offered_step(*offer, KEY_LABELS.at(*chef_keys));
    }
  }
  // An AI chef cooks on its next step, so the day must not end unfed first.
  if (frame.ai_chef_working && !dish_to_feed(frame)) {
    return "The AI chef is cooking.";
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
  if (worth_setting_out(frame)) {
    return set_out_keys(frame) + "Set out into the city: " +
           clock(frame.view.progression.remaining_exploration) + " left today.";
  }
  return key(keys.end_day) + "End the day unfed: acid rises by " +
         number(frame.content.get().companion.unfed_penalty.value) + ".";
}

float acid_ratio(const SurvivalFrame &frame) {
  return static_cast<float>(frame.view.progression.acid.value) /
         static_cast<float>(frame.content.get().run.full_acid.value);
}

/// A gauge with a dark rim, for values too small for the kit's bar frame.
void bar(godot::Node2D &canvas, float x, float y, float width, float height,
         float fill, godot::Color tint) {
  canvas.draw_rect(rect(x - 2.0F, y - 2.0F, width + 4.0F, height + 4.0F),
                   color(0, 0, 0, 1.0F));
  canvas.draw_rect(rect(x, y, width, height), color(44, 40, 66, 1.0F));
  canvas.draw_rect(rect(x, y, width * std::clamp(fill, 0.0F, 1.0F), height),
                   tint);
}

void paint_top_bar(godot::Node2D &canvas, const rendering::Art &drawing,
                   const SurvivalFrame &frame) {
  const auto tones = rendering::palette();
  const auto &progression = frame.view.progression;
  drawing.panel(canvas, rect(-HANG, -HANG, DAY_RIGHT + HANG, BAND_HEIGHT));
  drawing.image(canvas, rendering::Image::IconDay, Vector2(DAY_TEXT_X, 2.0F));
  drawing.text(canvas, DAY_TEXT_X + 38.0F, BAND_LINE,
               "DAY " + number(progression.day), 24, tones.gold);
  drawing.text(canvas, DAY_TEXT_X, BAND_SECOND_LINE, escape_goal(), GOAL_SIZE,
               tones.muted);
  drawing.frame(canvas, rendering::Image::BarFrame,
                rect(ACID_X, ACID_Y, ACID_WIDTH, BAR_FRAME_HEIGHT), BAR_BORDER,
                color(25, 24, 38, 1.0F));
  drawing.image(canvas, rendering::Image::IconAcid,
                Vector2(ACID_X + 14.0F, ACID_Y + 12.0F));
  drawing.text(canvas, ACID_X + 48.0F, ACID_Y + 36.0F, "ACID", 16,
               color(150, 255, 120, 1.0F));
  bar(canvas, ACID_X + 92.0F, ACID_Y + 22.0F, ACID_WIDTH - 188.0F, 12.0F,
      acid_ratio(frame), color(90, 226, 80, 1.0F));
  drawing.text(canvas, ACID_X + ACID_WIDTH - 90.0F, ACID_Y + 36.0F,
               number(progression.acid.value) + " / " +
                   number(frame.content.get().run.full_acid.value),
               16, tones.white);
  drawing.panel(canvas,
                rect(TIME_X, -HANG, SCREEN_WIDTH + HANG - TIME_X, BAND_HEIGHT));
  drawing.image(canvas, rendering::Image::IconClock,
                Vector2(TIME_X + INSET - 2.0F, 2.0F));
  drawing.text(canvas, TIME_X + INSET + 36.0F, BAND_LINE - 4.0F, "TIME LEFT",
               14, tones.muted);
  drawing.text(canvas, TIME_X + INSET + 116.0F, BAND_LINE,
               clock(progression.remaining_exploration), 24, tones.white);
  const String menu_keys = "Esc: pause   M: main menu";
  drawing.text(canvas,
               SCREEN_WIDTH + HANG - INSET - drawing.text_width(menu_keys, 14),
               BAND_LINE - 4.0F, menu_keys, 14, tones.muted);
  drawing.text(canvas, HAND_OFF_X, BAND_SECOND_LINE, hand_off_notice(frame),
               HAND_OFF_SIZE, tones.gold);
}

void paint_hearts(godot::Node2D &canvas, const rendering::Art &drawing, float x,
                  float y, std::uint8_t health) {
  for (std::uint8_t slot = 0; slot < 3; ++slot) {
    drawing.image(canvas,
                  slot < health ? rendering::Image::HeartFull
                                : rendering::Image::HeartEmpty,
                  Vector2(x + static_cast<float>(slot) * HEART_PITCH, y));
  }
}

/// Each spare tool an actor carries, as its icon with a count past one.
void paint_spare_tools(godot::Node2D &canvas, const rendering::Art &drawing,
                       const survival::ActorSnapshot &state, float x, float y) {
  std::map<std::uint32_t, std::uint32_t> spares;
  for (const auto spare : state.tools) {
    ++spares[spare.value];
  }
  if (spares.empty()) {
    drawing.text(canvas, x, y + 30.0F, NO_SPARE_TOOLS, CARD_TOOLS_SIZE,
                 rendering::palette().muted);
    return;
  }
  float left = x;
  for (const auto &[value, count] : spares) {
    paint_item(canvas, drawing, survival::tool_name(survival::ToolId{value}),
               Vector2(left, y));
    if (count > 1) {
      tag(canvas, drawing, left + 26.0F, y + 46.0F, "x" + number(count),
          CARD_TOOLS_SIZE, rendering::palette().white);
    }
    left += TOOL_PITCH;
  }
}

/// A base tool's icon, name and condition, as a panel's header shows them.
void paint_role_tool(godot::Node2D &canvas, const rendering::Art &drawing,
                     const GameView &view, Actor actor, float x, float y) {
  const auto tones = rendering::palette();
  const auto tool = role_tool_state(view, actor);
  paint_item(canvas, drawing, role_tool_name(actor), Vector2(x, y));
  drawing.text(canvas, x + 46.0F, y + 22.0F, role_tool_name(actor), 14,
               tones.white);
  drawing.text(canvas, x + 46.0F, y + 40.0F, tool_condition(tool), 14,
               tool == survival::ToolState::Intact
                   ? tones.muted
                   : color(255, 120, 110, 1.0F));
}

void paint_actor_card(godot::Node2D &canvas, const rendering::Art &drawing,
                      const SurvivalFrame &frame, Actor actor, float x) {
  const auto tones = rendering::palette();
  const auto &view = frame.view;
  const auto &state = state_of(view, actor);
  const auto keys = survival::key_set(frame.control, actor);
  const auto stamina =
      static_cast<float>(stamina_of(view, actor)) /
      static_cast<float>(
          frame.content.get().companion.exploration.maximum_stamina);
  drawing.panel(canvas, rect(x, CARD_Y, CARD_WIDTH, CARD_HEIGHT));
  const float left = x + INSET;
  const float top = CARD_Y + INSET;
  drawing.image(canvas, rendering::Image::ItemSlot, Vector2(left, top - 4.0F));
  drawing.image(canvas,
                actor == Actor::Chef ? rendering::Image::ChefIdle
                                     : rendering::Image::AppraiserIdle,
                Vector2(left, top - 8.0F));
  const bool out = state.location == Location::City;
  const String where = out ? "in city" : "at base";
  drawing.text(canvas, left + (SLOT - drawing.text_width(where, 14)) / 2.0F,
               top + 82.0F, where, 14, out ? tones.cyan : tones.gold);
  const float column = left + CARD_COLUMN;
  const String name = actor_name(actor);
  drawing.text(canvas, column, top + 14.0F, name, 17, tones.white);
  drawing.text(canvas, column + drawing.text_width(name, 17) + 8.0F,
               top + 14.0F, keys ? "P" + number(*keys + 1) : String("AI"), 17,
               keys ? tones.cyan : tones.muted);
  paint_hearts(canvas, drawing, column - 2.0F, top + 18.0F, state.health);
  bar(canvas, column + 104.0F, top + 30.0F,
      CARD_WIDTH - 2.0F * INSET - CARD_COLUMN - 106.0F, 10.0F, stamina,
      color(96, 200, 250, 1.0F));
  const auto tool = role_tool_state(view, actor);
  paint_item(canvas, drawing, role_tool_name(actor),
             Vector2(column - 8.0F, top + 50.0F));
  drawing.text(canvas, column + CARD_TOOL_TEXT, top + 68.0F,
               role_tool_name(actor), CARD_TOOLS_SIZE, tones.white);
  drawing.text(canvas, column + CARD_TOOL_TEXT, top + 86.0F,
               tool_condition(tool), CARD_TOOLS_SIZE,
               tool == survival::ToolState::Intact
                   ? tones.muted
                   : color(255, 120, 110, 1.0F));
  paint_spare_tools(canvas, drawing, state, column + CARD_SPARES, top + 50.0F);
}

void paint_pack(godot::Node2D &canvas, const rendering::Art &drawing,
                const GameView &view, float x) {
  const auto tones = rendering::palette();
  drawing.panel(canvas, rect(x, CARD_Y, PACK_WIDTH, CARD_HEIGHT));
  const float left = x + INSET;
  const float top = CARD_Y + INSET;
  drawing.text(canvas, left, top + 14.0F, "PACK", 15, tones.gold);
  drawing.text(canvas, left + 50.0F, top + 14.0F, UNIDENTIFIED_MARK, 13,
               tones.muted);
  const auto pack = pack_of(view);
  if (pack.empty()) {
    drawing.text(canvas, left, top + 50.0F, "Empty", 15, tones.muted);
  }
  float column = left;
  for (const auto &[value, count] : pack) {
    const auto ingredient = IngredientId{value};
    const bool known = identified(view, ingredient);
    paint_item(canvas, drawing, survival::ingredient_name(ingredient),
               Vector2(column, top + 20.0F));
    const String label = number(count) + (known ? "" : "?");
    drawing.text(canvas,
                 column + (48.0F - drawing.text_width(label, 14)) / 2.0F,
                 top + 86.0F, label, 14, known ? tones.white : tones.gold);
    column += INGREDIENT_PITCH;
  }
}

void paint_events(godot::Node2D &canvas, const rendering::Art &drawing,
                  const SurvivalFrame &frame, godot::Rect2 area) {
  if (frame.events.empty()) {
    return;
  }
  drawing.panel(canvas, area);
  float line = area.position.y + INSET + 13.0F;
  for (const auto &event : frame.events) {
    drawing.text(canvas, area.position.x + INSET, line, describe(event),
                 EVENT_SIZE, rendering::palette().white);
    line += EVENT_LINE_HEIGHT;
  }
}

String key_legend(const SurvivalFrame &frame) {
  const bool city = frame.view.progression.phase == Phase::Exploration;
  const String collecting =
      city ? "    Each collection spends " + collection_clock(frame) : String();
  const auto legend = [city, &frame](const KeyLabels &keys, bool chef) {
    if (city) {
      // A human left at base sets out with the travel key.
      const auto actor = chef ? Actor::Chef : Actor::Appraiser;
      const bool out = state_of(frame.view, actor).location == Location::City;
      return from(keys.move) + " move  " + from(keys.run) + " run  " +
             from(keys.interact) + " use  " + from(keys.travel) +
             (out ? " return to base" : " set out");
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
           legend(KEY_LABELS.at(1), false) + collecting;
  }
  const auto &keys = KEY_LABELS.at(0);
  return legend(keys, chef.has_value()) + "   " + from(keys.switch_actor) +
         " swap actor" + collecting;
}

void paint_next_step(godot::Node2D &canvas, const rendering::Art &drawing,
                     const SurvivalFrame &frame) {
  const auto tones = rendering::palette();
  canvas.draw_rect(rect(0.0F, NEXT_Y, SCREEN_WIDTH, SCREEN_HEIGHT - NEXT_Y),
                   color(14, 12, 26, 1.0F));
  canvas.draw_rect(rect(0.0F, NEXT_Y, SCREEN_WIDTH, 2.0F),
                   color(88, 84, 102, 1.0F));
  drawing.text(canvas, LEGEND_X, NEXT_Y + 23.0F, "NEXT", NEXT_SIZE, tones.cyan);
  drawing.text(canvas, NEXT_TEXT_X, NEXT_Y + 23.0F, next_step(frame), NEXT_SIZE,
               tones.white);
  drawing.text(canvas, LEGEND_X, NEXT_Y + 41.0F, key_legend(frame), LEGEND_SIZE,
               tones.muted);
}

void paint_appraise_panel(godot::Node2D &canvas, const rendering::Art &drawing,
                          const SurvivalFrame &frame, float x) {
  const auto tones = rendering::palette();
  const auto &view = frame.view;
  drawing.panel(canvas, rect(x, PANEL_Y, APPRAISE_WIDTH, PANEL_HEIGHT));
  const float left = x + INSET;
  const float top = PANEL_Y + INSET;
  drawing.text(canvas, left, top + 26.0F, "APPRAISE", PANEL_TITLE_SIZE,
               tones.gold);
  paint_role_tool(canvas, drawing, view, Actor::Appraiser, left + 128.0F,
                  top - 8.0F);
  const auto pack = pack_of(view);
  if (pack.empty()) {
    drawing.text(canvas, left, top + 80.0F,
                 "Nothing carried. Find ingredients.", 16, tones.muted);
  }
  std::size_t index = 0;
  for (const auto &[value, count] : pack) {
    const auto ingredient = IngredientId{value};
    const std::size_t rows_above = index / SLOTS_PER_ROW;
    const Vector2 slot(
        left + static_cast<float>(index % SLOTS_PER_ROW) * SLOT_PITCH,
        top + 48.0F + static_cast<float>(rows_above) * SLOT_PITCH);
    drawing.image(canvas, rendering::Image::ItemSlot, slot);
    paint_item(canvas, drawing, survival::ingredient_name(ingredient),
               slot + Vector2(SLOT_ICON, SLOT_ICON));
    tag(canvas, drawing, slot.x + 38.0F, slot.y + 56.0F, "x" + number(count),
        13, tones.white);
    if (!identified(view, ingredient)) {
      tag(canvas, drawing, slot.x + 48.0F, slot.y + 20.0F, "?", 16, tones.gold);
    }
    ++index;
  }
  drawing.text(canvas, left, top + 206.0F, UNIDENTIFIED_MARK, 13, tones.muted);
  float line = top + 228.0F;
  for (const auto &text : eaten_lines(view.workshop.eaten_ingredients)) {
    drawing.text(canvas, left, line, text, EATEN_SIZE, tones.muted);
    line += 17.0F;
  }
  paint_button(canvas, drawing,
               rect(left - 4.0F, JOB_Y, APPRAISE_WIDTH - 2.0F * INSET + 8.0F,
                    BUTTON_HEIGHT),
               appraise_job(frame), JOB_SIZE,
               survival::key_set(frame.control, Actor::Appraiser) &&
                   has_unidentified(view));
}

/// What stands between a recipe and the pot, ingredient by ingredient.
struct RecipeNeeds {
  std::vector<IngredientId> missing;
  std::vector<IngredientId> unidentified;
};

RecipeNeeds recipe_needs(const GameView &view, const survival::Recipe &recipe) {
  auto pack = pack_of(view);
  RecipeNeeds needs;
  for (const auto ingredient : recipe.ingredients) {
    auto &count = pack[ingredient.value];
    if (count == 0) {
      needs.missing.push_back(ingredient);
    } else {
      --count;
      if (!identified(view, ingredient)) {
        needs.unidentified.push_back(ingredient);
      }
    }
  }
  return needs;
}

/// Appends "`verb` a, b" to `text`, when there are any.
void list_ingredients(String &text, const char *verb,
                      const std::vector<IngredientId> &ingredients) {
  if (ingredients.empty()) {
    return;
  }
  String names;
  for (const auto ingredient : ingredients) {
    names += (names.is_empty() ? "" : ", ") +
             from(survival::ingredient_name(ingredient));
  }
  text += (text.is_empty() ? "" : ", ") + String(verb) + " " + names;
}

/// Names what a recipe still lacks; empty when it can be cooked now.
String needs_text(const RecipeNeeds &needs) {
  String status;
  list_ingredients(status, "need", needs.missing);
  list_ingredients(status, "identify", needs.unidentified);
  return status;
}

/// The needs when the bits of `held` and `known` say which ingredients are
/// carried and identified.
RecipeNeeds needs_given(const std::vector<IngredientId> &ingredients,
                        std::size_t held, std::size_t known) {
  RecipeNeeds needs;
  for (std::size_t index = 0; index < ingredients.size(); ++index) {
    if ((held >> index & 1U) == 0) {
      needs.missing.push_back(ingredients.at(index));
    } else if ((known >> index & 1U) == 0) {
      needs.unidentified.push_back(ingredients.at(index));
    }
  }
  return needs;
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

/// "plain 85 falls short" and the like, for one quality of a dish.
String score_word(const SurvivalFrame &frame, survival::Dish dish) {
  const auto &tuning = frame.content.get().companion.workshop;
  const auto score = survival::dish_score(dish, frame.view.workshop, tuning);
  const auto total = score ? score->total : 0U;
  return number(total) +
         (total >= tuning.satisfying_score ? " passes" : " falls short");
}

/// What the offered dish would score, plain and fine, against the threshold.
String cook_forecast(const SurvivalFrame &frame, survival::FoodId food) {
  return "Plain " +
         score_word(frame,
                    {.food = food, .quality = survival::DishQuality::Plain}) +
         ", fine " +
         score_word(frame,
                    {.food = food, .quality = survival::DishQuality::Fine}) +
         ": needs " +
         number(frame.content.get().companion.workshop.satisfying_score);
}

/// The dish the chef's key would cook, if any.
std::optional<survival::FoodId> offered_dish(const SurvivalFrame &frame) {
  const auto &offer = frame.offers.at(index_of(Actor::Chef)).interact;
  const auto *cook = offer ? std::get_if<survival::Cook>(&*offer) : nullptr;
  return cook != nullptr ? std::optional{cook->food} : std::nullopt;
}

void paint_cook_panel(godot::Node2D &canvas, const rendering::Art &drawing,
                      const SurvivalFrame &frame, float x) {
  const auto tones = rendering::palette();
  const auto &view = frame.view;
  drawing.panel(canvas, rect(x, PANEL_Y, COOK_WIDTH, PANEL_HEIGHT));
  const float left = x + INSET;
  const float top = PANEL_Y + INSET;
  drawing.text(canvas, left, top + 26.0F, "COOK", PANEL_TITLE_SIZE, tones.gold);
  paint_role_tool(canvas, drawing, view, Actor::Chef, left + 80.0F, top - 8.0F);
  drawing.text(canvas, left + COOK_TASTE_X - 4.0F, top + 40.0F, "Taste", 13,
               tones.muted);
  float y = top + COOK_FIRST_ROW;
  const auto offered = offered_dish(frame);
  const auto &tuning = frame.content.get().companion.workshop;
  for (const auto &recipe : tuning.recipes) {
    const auto missing = needs_text(recipe_needs(view, recipe));
    if (offered == recipe.food) {
      drawing.text(canvas, left - 2.0F, y, ">", COOK_NAME_SIZE, tones.gold);
    }
    const auto &eaten = view.workshop.eaten_foods;
    const auto &rejected = view.workshop.rejected_foods;
    const bool repeat = std::ranges::find(eaten, recipe.food) != eaten.end();
    const bool hated =
        std::ranges::find(rejected, recipe.food) != rejected.end();
    drawing.text(canvas, left + 12.0F, y,
                 from(survival::food_name(recipe.food)), COOK_NAME_SIZE,
                 tones.white);
    drawing.text(canvas, left + COOK_STATUS_X, y,
                 missing.is_empty() ? String("ready") : missing,
                 COOK_STATUS_SIZE,
                 missing.is_empty() ? tones.cyan : tones.muted);
    if (repeat) {
      drawing.text(canvas, left + COOK_EATEN_X, y, hated ? "hated" : "eaten",
                   COOK_STATUS_SIZE, tones.gold);
    }
    drawing.text(canvas, left + COOK_TASTE_X, y,
                 number(survival::taste_of(recipe, tuning)), 14, tones.muted);
    y += COOK_ROW_HEIGHT;
  }
  const Vector2 slot(left, top + COOK_OFFER_Y);
  drawing.image(canvas, rendering::Image::ItemSlot, slot);
  if (offered) {
    paint_item(canvas, drawing, survival::food_name(*offered),
               slot + Vector2(SLOT_ICON, SLOT_ICON));
  }
  drawing.text(canvas, left + COOK_OFFER_TEXT, top + COOK_OFFER_Y + 14.0F,
               offered_forecast(frame), FORECAST_SIZE, tones.gold);
  paint_button(canvas, drawing,
               rect(left + COOK_OFFER_TEXT - 2.0F, JOB_Y,
                    COOK_WIDTH - 2.0F * INSET - COOK_OFFER_TEXT + 6.0F,
                    BUTTON_HEIGHT),
               cook_job(frame), JOB_SIZE,
               offered && survival::key_set(frame.control, Actor::Chef));
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

/// The feed panel's two keys: feed, and end the day unfed.
std::array<String, 2> feed_jobs(const SurvivalFrame &frame) {
  const auto &keys = KEY_LABELS.at(0);
  return {key(keys.feed) + "Feed",
          key(keys.end_day) + "End the day unfed: acid +" +
              number(frame.content.get().companion.unfed_penalty.value)};
}

String dish_label(const survival::Dish &dish) {
  return from(survival::food_name(dish.food)) +
         (dish.quality == survival::DishQuality::Fine ? String(" (fine)")
                                                      : String());
}

void paint_feed_panel(godot::Node2D &canvas, const rendering::Art &drawing,
                      const SurvivalFrame &frame, float x) {
  const auto tones = rendering::palette();
  const auto &workshop = frame.view.workshop;
  const auto &tuning = frame.content.get().companion.workshop;
  drawing.panel(canvas, rect(x, PANEL_Y, FEED_WIDTH, PANEL_HEIGHT));
  const float left = x + INSET;
  const float top = PANEL_Y + INSET;
  drawing.text(canvas, left, top + 26.0F, "FEED", PANEL_TITLE_SIZE, tones.gold);
  drawing.text(canvas, left + 76.0F, top + 26.0F,
               "Needs a score of " + number(tuning.satisfying_score), 14,
               tones.muted);
  drawing.text(canvas, left, top + 48.0F, FEED_RULES.at(0), 13, tones.muted);
  drawing.text(canvas, left, top + 64.0F, FEED_RULES.at(1), 13, tones.muted);
  float y = top + 76.0F;
  if (workshop.foods.empty()) {
    drawing.text(canvas, left, y + 24.0F, "No dish stored. The chef must cook.",
                 15, tones.muted);
  }
  for (const auto &dish : workshop.foods) {
    paint_item(canvas, drawing, survival::food_name(dish.food),
               Vector2(left, y));
    drawing.text(canvas, left + 54.0F, y + 20.0F, dish_label(dish),
                 FEED_NAME_SIZE, tones.cyan);
    drawing.text(canvas, left + 54.0F, y + 40.0F,
                 "Score " + score_word(frame, dish), 14, tones.gold);
    y += 50.0F;
  }
  const auto jobs = feed_jobs(frame);
  const float width = FEED_WIDTH - 2.0F * INSET + 8.0F;
  paint_button(canvas, drawing,
               rect(left - 4.0F, FEED_BUTTON_Y, width, BUTTON_HEIGHT - 4.0F),
               jobs.at(0), JOB_SIZE, dish_to_feed(frame));
  paint_button(canvas, drawing, rect(left - 4.0F, JOB_Y, width, BUTTON_HEIGHT),
               jobs.at(1), JOB_SIZE, false);
}

/// Draws over the kitchen and reads only snapshots, so no clock or key waits.
void paint_day_result(godot::Node2D &canvas, const rendering::Art &drawing,
                      const SurvivalFrame &frame) {
  const auto lines = day_result(frame);
  if (lines.empty()) {
    return;
  }
  const auto tones = rendering::palette();
  drawing.panel(canvas,
                rect(RESULT_X, RESULT_Y, RESULT_WIDTH,
                     RESULT_LINE_HEIGHT * static_cast<float>(lines.size()) +
                         2.0F * INSET + 6.0F));
  float y = RESULT_Y + INSET + static_cast<float>(RESULT_SIZE);
  for (const auto &line : lines) {
    drawing.text(canvas, RESULT_X + INSET, y, line, RESULT_SIZE, tones.white);
    y += RESULT_LINE_HEIGHT;
  }
}

void paint_workshop_screen(godot::Node2D &canvas, const rendering::Art &drawing,
                           const SurvivalFrame &frame) {
  rendering::paint_workshop(canvas, drawing,
                            !frame.view.workshop.foods.empty());
  paint_day_result(canvas, drawing, frame);
  paint_appraise_panel(canvas, drawing, frame, APPRAISE_X);
  paint_cook_panel(canvas, drawing, frame, COOK_X);
  paint_feed_panel(canvas, drawing, frame, FEED_X);
  paint_events(
      canvas, drawing, frame,
      rect(EVENT_X, EVENT_Y, SCREEN_WIDTH + HANG - EVENT_X, EVENT_HEIGHT));
}

void paint_city_screen(godot::Node2D &canvas, const rendering::Art &drawing,
                       const SurvivalFrame &frame) {
  rendering::paint_city(canvas, drawing, frame.content.get(), frame.view,
                        frame.control, out_of_reach(frame));
  paint_actor_card(canvas, drawing, frame, Actor::Chef, -HANG);
  paint_actor_card(canvas, drawing, frame, Actor::Appraiser, CARD_WIDTH - HANG);
  paint_pack(canvas, drawing, frame.view, PACK_X);
  paint_events(canvas, drawing, frame,
               rect(CITY_EVENT_X, CARD_Y, SCREEN_WIDTH + HANG - CITY_EVENT_X,
                    CARD_HEIGHT));
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
  case survival::Result::RunOver:
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

FittedText fitted(const String &text, int size, float width) {
  return {.text = text, .size = size, .width = width};
}

/// Every key of one player or of two, one row per action.
std::vector<KeyRow> rows_for(bool together) {
  const auto row = [together](const char *action, const char *first,
                              const char *second) {
    return KeyRow{.action = action,
                  .first = first,
                  .second = together ? String(second) : String()};
  };
  const auto &one = KEY_LABELS.at(0);
  const auto &two = KEY_LABELS.at(1);
  std::vector<KeyRow> rows;
  if (together) {
    rows.push_back(row("", "P1", "P2"));
  }
  rows.push_back(row("Move", one.every_move, two.every_move));
  rows.push_back(row("Run", one.run, two.run));
  rows.push_back(row("Set out or return to base", one.travel, two.travel));
  rows.push_back(row("Collect or repair; appraise or cook at base",
                     one.interact, two.interact));
  rows.push_back(row("Choose the dish, as the chef at base",
                     EVERY_CHOOSE_DISH_KEY, "none"));
  rows.push_back(
      row("Replace a damaged base tool", one.replace_tool, two.replace_tool));
  rows.push_back(row("Feed a stored dish", one.feed, two.feed));
  rows.push_back(row("End the day unfed", one.end_day, two.end_day));
  if (together) {
    rows.push_back(row("Hand your actor to AI", one.leave, two.leave));
    rows.push_back(
        row("Swap actor, once one player is left", one.switch_actor, "none"));
  } else {
    rows.push_back(row("Swap actor", one.switch_actor, "none"));
  }
  return rows;
}

/// The name and the line under it of one choice of players.
std::array<String, 2> setup_label(SetupEntry entry) {
  switch (entry) {
  case SetupEntry::Chef:
    return {"CHEF", "You cook. AI appraises."};
  case SetupEntry::Appraiser:
    return {"APPRAISER", "You appraise. AI cooks."};
  case SetupEntry::TwoPlayers:
    return {"TWO PLAYERS", "P1 chef (WASD), P2 appraiser (IJKL)."};
  }
  std::abort();
}

} // namespace

void paint_survival_setup(godot::Node2D &canvas, const rendering::Art &drawing,
                          SetupEntry selected) {
  const auto tones = rendering::palette();
  drawing.image(canvas, rendering::Image::TitleKeyArt, Vector2());
  canvas.draw_rect(rect(0.0F, 0.0F, SCREEN_WIDTH, SCREEN_HEIGHT),
                   color(14, 12, 26, 0.45F));
  drawing.panel(canvas, rect(SETUP_X, 30.0F, SETUP_WIDTH, 112.0F));
  drawing.text(canvas, SETUP_X + 40.0F, 104.0F, "WHO IS PLAYING?", 40,
               tones.white);
  float y = SETUP_Y;
  std::size_t number_key = 1;
  for (const SetupEntry entry : SETUP_ENTRIES) {
    const bool chosen = entry == selected;
    drawing.button(canvas, rect(SETUP_X, y, SETUP_WIDTH, SETUP_HEIGHT), chosen);
    const auto label = setup_label(entry);
    if (entry == SetupEntry::TwoPlayers) {
      drawing.image(canvas, rendering::Image::ChefIdle,
                    Vector2(SETUP_X + 16.0F, y + 28.0F));
      drawing.image(canvas, rendering::Image::AppraiserIdle,
                    Vector2(SETUP_X + 72.0F, y + 28.0F));
    } else {
      drawing.image(canvas,
                    entry == SetupEntry::Chef ? rendering::Image::ChefIdle
                                              : rendering::Image::AppraiserIdle,
                    Vector2(SETUP_X + 44.0F, y + 28.0F));
    }
    drawing.text(canvas, SETUP_X + 170.0F, y + 54.0F,
                 number(number_key) + "   " + label.at(0), 30,
                 chosen ? tones.ink : tones.white);
    drawing.text(canvas, SETUP_X + 170.0F, y + 92.0F, label.at(1), 20,
                 chosen ? tones.ink : tones.white);
    y += SETUP_PITCH;
    ++number_key;
  }
  drawing.panel(canvas, rect(-HANG, 572.0F, SCREEN_WIDTH + 2.0F * HANG,
                             SCREEN_HEIGHT - 572.0F + HANG));
  drawing.text(canvas, SETUP_X - 70.0F, 626.0F,
               "Gather by day, appraise and cook at base, and feed the monster "
               "before the acid wrecks the city.",
               20, tones.white);
  drawing.text(canvas, SETUP_X - 70.0F, 666.0F,
               "W/S or up/down: choose     Enter: select     1, 2, 3: jump "
               "straight in     M: main menu",
               18, tones.muted);
}

void paint_survival(godot::Node2D &canvas, const rendering::Art &drawing,
                    const SurvivalFrame &frame) {
  const auto &progression = frame.view.progression;
  rendering::paint_mouth(canvas, drawing, acid_ratio(frame));
  if (progression.phase == Phase::Exploration) {
    paint_city_screen(canvas, drawing, frame);
  } else {
    paint_workshop_screen(canvas, drawing, frame);
  }
  paint_top_bar(canvas, drawing, frame);
  paint_next_step(canvas, drawing, frame);
}

void paint_interruption(godot::Node2D &canvas, const rendering::Art &drawing,
                        Interruption interruption, const SurvivalFrame &frame) {
  const auto tones = rendering::palette();
  const auto text = interruption_text(interruption);
  canvas.draw_rect(rect(0.0F, 0.0F, SCREEN_WIDTH, SCREEN_HEIGHT),
                   color(14, 12, 26, 0.6F));
  if (interruption == Interruption::Paused) {
    drawing.panel(canvas, rect(PAUSE_BOX_X, PAUSE_BOX_Y, PAUSE_BOX_WIDTH,
                               PAUSE_BOX_HEIGHT));
    drawing.text(canvas, PAUSE_TEXT_X, PAUSE_BOX_Y + 70.0F, text.at(0),
                 INTERRUPTION_TITLE_SIZE, tones.gold);
    drawing.text(canvas, PAUSE_TEXT_X, PAUSE_BOX_Y + 105.0F, text.at(1),
                 INTERRUPTION_LINE_SIZE, tones.white);
    paint_key_rows(canvas, drawing, key_rows(frame), PAUSE_ROWS_Y,
                   PAUSE_ROW_HEIGHT);
    paint_button(canvas, drawing,
                 rect(PAUSE_TEXT_X - BUTTON_TEXT_X,
                      PAUSE_BOX_Y + PAUSE_BOX_HEIGHT - 66.0F,
                      drawing.text_width(text.at(2), INTERRUPTION_LINE_SIZE) +
                          2.0F * BUTTON_TEXT_X,
                      BUTTON_HEIGHT),
                 text.at(2), INTERRUPTION_LINE_SIZE, false);
    return;
  }
  drawing.panel(canvas, rect(NOTICE_BOX_X, NOTICE_BOX_Y, NOTICE_BOX_WIDTH,
                             NOTICE_BOX_HEIGHT));
  drawing.text(canvas, NOTICE_TEXT_X, NOTICE_BOX_Y + 70.0F, text.at(0),
               INTERRUPTION_TITLE_SIZE, tones.gold);
  drawing.text(canvas, NOTICE_TEXT_X, NOTICE_BOX_Y + 120.0F, text.at(1),
               INTERRUPTION_LINE_SIZE, tones.white);
  if (!text.at(2).is_empty()) {
    paint_button(canvas, drawing,
                 rect(NOTICE_TEXT_X - BUTTON_TEXT_X, NOTICE_BOX_Y + 160.0F,
                      drawing.text_width(text.at(2), INTERRUPTION_LINE_SIZE) +
                          2.0F * BUTTON_TEXT_X,
                      BUTTON_HEIGHT),
                 text.at(2), INTERRUPTION_LINE_SIZE, false);
  }
}

void paint_cinematic(godot::Node2D &canvas, const rendering::Art &drawing,
                     Cinematic cinematic, bool fallback,
                     const SurvivalFrame &frame) {
  canvas.draw_rect(rect(0.0F, 0.0F, 1280.0F, 720.0F), color(8, 6, 16, 1.0F));
  if (!fallback) {
    return;
  }
  const auto tones = rendering::palette();
  const auto &progression = frame.view.progression;
  rendering::paint_mouth(canvas, drawing, acid_ratio(frame));
  canvas.draw_rect(rect(0.0F, 0.0F, 1280.0F, 720.0F), color(14, 12, 26, 0.6F));
  drawing.image(canvas, rendering::Image::ChefIdle,
                godot::Vector2(130.0F, 300.0F), 2.0F);
  drawing.image(canvas, rendering::Image::AppraiserIdle,
                godot::Vector2(280.0F, 300.0F), 2.0F);
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
  case Cinematic::Loss:
    drawing.text(canvas, 130.0F, 170.0F, "DISSOLVED", 46,
                 color(150, 255, 120, 1.0F));
    drawing.text(canvas, 130.0F, 230.0F,
                 "The acid filled the monster's mouth after " +
                     number(progression.day - 1) + String(" days."),
                 24, tones.white);
    break;
  }
  drawing.text(canvas, 130.0F, 560.0F, "Placeholder card: no cinematic yet.",
               20, tones.muted);
  drawing.text(canvas, 130.0F, 640.0F, "E: skip   M: main menu", 22,
               tones.muted);
}

bool recipe_ready(const GameView &view, const survival::Recipe &recipe) {
  const auto needs = recipe_needs(view, recipe);
  return needs.missing.empty() && needs.unidentified.empty();
}

std::vector<KeyRow> key_rows(const SurvivalFrame &frame) {
  return rows_for(survival::key_set(frame.control, Actor::Chef) &&
                  survival::key_set(frame.control, Actor::Appraiser));
}

std::vector<KeyRow> control_rows() {
  auto rows = rows_for(true);
  rows.push_back({.action = "Pause or continue",
                  .first = MENU_KEYS.pause,
                  .second = MENU_KEYS.pause});
  rows.push_back({.action = "Leave the run; Esc cancels",
                  .first = MENU_KEYS.leave,
                  .second = MENU_KEYS.leave});
  return rows;
}

void paint_key_rows(godot::Node2D &canvas, const rendering::Art &drawing,
                    const std::vector<KeyRow> &rows, float top,
                    float row_height) {
  const auto tones = rendering::palette();
  float y = top;
  for (const auto &row : rows) {
    // The heading row names the players, not keys.
    if (row.action.is_empty()) {
      drawing.text(canvas, PAUSE_FIRST_X, y, row.first, PAUSE_ROW_SIZE,
                   tones.gold);
      drawing.text(canvas, PAUSE_SECOND_X, y, row.second, PAUSE_ROW_SIZE,
                   tones.gold);
      y += row_height;
      continue;
    }
    drawing.text(canvas, PAUSE_TEXT_X, y, row.action, PAUSE_ROW_SIZE,
                 tones.white);
    drawing.text(canvas, PAUSE_FIRST_X, y, row.first, PAUSE_ROW_SIZE,
                 tones.cyan);
    drawing.text(canvas, PAUSE_SECOND_X, y, row.second, PAUSE_ROW_SIZE,
                 tones.cyan);
    y += row_height;
  }
}

std::array<String, 3> interruption_text(Interruption interruption) {
  switch (interruption) {
  case Interruption::Paused:
    return {"PAUSED", "The clock is stopped.",
            "Esc: continue     M: leave the run"};
  case Interruption::ConfirmLeave:
  case Interruption::ConfirmLeaveFromPause:
    return {"LEAVE THIS RUN?", "Progress is not saved: the run ends here.",
            "Enter: leave for the main menu     Esc: cancel"};
  case Interruption::Resuming:
    return {"RELEASE EVERY KEY", "The run continues once all keys are up.", ""};
  }
  std::abort();
}

String hand_off_notice(const SurvivalFrame &frame) {
  const auto chef = survival::key_set(frame.control, Actor::Chef);
  const auto appraiser = survival::key_set(frame.control, Actor::Appraiser);
  if (!frame.started_together || (chef && appraiser)) {
    return {};
  }
  const auto &keys = KEY_LABELS.at(0);
  return "One player left, as the " +
         String(actor_name(chef ? Actor::Chef : Actor::Appraiser)) + ": " +
         from(keys.move) + " move  " + from(keys.run) + " run  " +
         from(keys.interact) + " use  " + from(keys.travel) + " travel  " +
         from(keys.replace_tool) + " tool  " + from(keys.feed) + " feed  " +
         from(keys.end_day) + " end day  " + from(keys.switch_actor) + " swap";
}

String next_step(const SurvivalFrame &frame) {
  const auto &view = frame.view;
  if (view.progression.phase == Phase::Escaped) {
    return "The party has escaped.";
  }
  if (view.progression.phase == Phase::Lost) {
    return "The acid has filled the mouth.";
  }
  if (view.progression.phase == Phase::Workshop) {
    return workshop_step(frame, KEY_LABELS.at(0));
  }
  const bool two_humans = std::ranges::all_of(ACTORS, [&frame](Actor actor) {
    return survival::key_set(frame.control, actor).has_value();
  });
  // A second human left at base is told to join while it has an errand.
  std::vector<std::pair<std::size_t, String>> steps;
  for (const auto actor : ACTORS) {
    const auto keys = survival::key_set(frame.control, actor);
    if (!keys) {
      continue;
    }
    const auto &labels = KEY_LABELS.at(*keys);
    if (state_of(frame.view, actor).location == Location::City) {
      steps.emplace_back(*keys, city_step(frame, actor, labels));
    } else if (two_humans && worth_setting_out(frame, actor)) {
      steps.emplace_back(*keys, key(labels.travel) + "Set out into the city.");
    }
  }
  String line;
  for (const auto &[keys, step] : steps) {
    const String player =
        steps.size() > 1 ? "P" + number(keys + 1) + ": " : String();
    line += (line.is_empty() ? String() : String("   ")) + player + step;
  }
  return line.is_empty() ? String("Waiting for the party to reach base.")
                         : line;
}

String appraise_job(const SurvivalFrame &frame) {
  const auto keys = survival::key_set(frame.control, Actor::Appraiser);
  if (!keys) {
    return "The AI appraiser works here.";
  }
  return has_unidentified(frame.view)
             ? key(KEY_LABELS.at(*keys).interact) +
                   "Appraise the next unknown ingredient"
             : String("Nothing to appraise.");
}

std::vector<String> eaten_lines(const std::vector<IngredientId> &eaten) {
  std::vector<String> lines{"Eaten:"};
  for (std::size_t index = 0; index < eaten.size(); ++index) {
    const bool wrap = index % EATEN_PER_LINE == 0;
    if (wrap && index > 0) {
      lines.emplace_back();
    }
    lines.back() += (wrap && index > 0 ? "" : " ") +
                    from(survival::ingredient_name(eaten.at(index)));
    if (index + 1 < eaten.size() && (index + 1) % EATEN_PER_LINE != 0) {
      lines.back() += ",";
    }
  }
  return lines;
}

String offered_forecast(const SurvivalFrame &frame) {
  const auto offered = offered_dish(frame);
  return offered ? cook_forecast(frame, *offered) : String();
}

String escape_goal() {
  return "Escape checks from day " +
         number(survival::TEMPORARY_ESCAPE.first_day);
}

std::vector<String> day_result(const SurvivalFrame &frame) {
  const auto &last = frame.view.workshop.last_day;
  if (!last) {
    return {};
  }
  String headline = "Day " + number(last->completed_day) + ": ";
  std::vector<String> lines;
  if (last->fed) {
    const auto &dish = last->fed->dish;
    const auto &score = last->fed->score;
    headline += from(survival::food_name(dish.food)) +
                (dish.quality == survival::DishQuality::Fine ? String(" (fine)")
                                                             : String()) +
                ". ";
    lines.emplace_back(headline + reaction_text(last->reaction));
    const auto &tuning = frame.content.get().companion.workshop;
    lines.push_back("New food " + number(score.new_food) + " + " +
                    number(score.new_ingredients) + " new ingredients " +
                    number(score.new_ingredient_points) + " + taste " +
                    number(score.taste) + " + fine " + number(score.fine) +
                    " = " + number(score.total) + ", needs " +
                    number(tuning.satisfying_score));
  } else {
    lines.emplace_back(headline + reaction_text(last->reaction));
  }
  const auto after = frame.view.progression.acid.value;
  const auto rise = std::min(after, last->acid_rise.value);
  lines.push_back("Acid " + number(after - rise) + " to " + number(after) +
                  ". Map changes: " + number(frame.last_day_map_changes));
  return lines;
}

std::vector<survival::ObjectId> out_of_reach(const SurvivalFrame &frame) {
  const auto &view = frame.view;
  const auto &exploration = frame.content.get().companion.exploration;
  Cells cells;
  for (const auto actor : ACTORS) {
    if (survival::key_set(frame.control, actor)) {
      add_walkable(cells, view.world,
                   state_of(view, actor).location == Location::City
                       ? position_of(view, actor)
                       : exploration.entry);
    }
  }
  std::vector<survival::ObjectId> stranded;
  const auto &objects = view.world.objects;
  for (const auto &stock : view.exploration.sources) {
    const auto object =
        std::ranges::find(objects, stock.id, &survival::ObjectSnapshot::id);
    if (stock.remaining == 0 || object == objects.end() ||
        !object->material_available) {
      continue;
    }
    const bool reached = std::ranges::any_of(cells, [&](const auto &cell) {
      return survival::distance({.x = cell.first, .y = cell.second},
                                object->area) <=
             exploration.collection_distance;
    });
    if (!reached) {
      stranded.push_back(stock.id);
    }
  }
  return stranded;
}

std::vector<FittedText> changing_text(const SurvivalFrame &frame) {
  const float appraise_inner = APPRAISE_WIDTH - 2.0F * INSET;
  const float feed_inner = FEED_WIDTH - 2.0F * INSET;
  std::vector<FittedText> lines;
  lines.push_back(fitted(escape_goal(), GOAL_SIZE, GOAL_WIDTH));
  for (const auto &line : day_result(frame)) {
    lines.push_back(fitted(line, RESULT_SIZE, RESULT_WIDTH - 2.0F * INSET));
  }
  std::vector<IngredientId> every_ingredient;
  for (const auto &flavor : frame.content.get().companion.workshop.flavors) {
    every_ingredient.push_back(flavor.ingredient);
  }
  for (const auto &line : eaten_lines(every_ingredient)) {
    lines.push_back(fitted(line, EATEN_SIZE, appraise_inner));
  }
  lines.push_back(
      fitted(next_step(frame), NEXT_SIZE, SCREEN_EDGE - NEXT_TEXT_X));
  lines.push_back(
      fitted(key_legend(frame), LEGEND_SIZE, SCREEN_EDGE - LEGEND_X));
  lines.push_back(
      fitted(hand_off_notice(frame), HAND_OFF_SIZE, HAND_OFF_WIDTH));
  // The city's log is the narrower of the two.
  for (const auto &event : frame.events) {
    lines.push_back(fitted(describe(event), EVENT_SIZE,
                           SCREEN_WIDTH + HANG - INSET - CITY_EVENT_X - INSET));
  }
  for (const auto &rows : {key_rows(frame), control_rows()}) {
    for (const auto &row : rows) {
      lines.push_back(fitted(row.action, PAUSE_ROW_SIZE,
                             PAUSE_FIRST_X - PAUSE_TEXT_X - 8.0F));
      lines.push_back(fitted(row.first, PAUSE_ROW_SIZE,
                             PAUSE_SECOND_X - PAUSE_FIRST_X - 8.0F));
      lines.push_back(
          fitted(row.second, PAUSE_ROW_SIZE,
                 PAUSE_BOX_X + PAUSE_BOX_WIDTH - INSET - PAUSE_SECOND_X));
    }
  }
  const float notice_width =
      NOTICE_BOX_X + NOTICE_BOX_WIDTH - INSET - NOTICE_TEXT_X;
  for (const auto interruption :
       {Interruption::ConfirmLeave, Interruption::Resuming}) {
    const auto text = interruption_text(interruption);
    lines.push_back(fitted(text.at(0), INTERRUPTION_TITLE_SIZE, notice_width));
    lines.push_back(fitted(text.at(1), INTERRUPTION_LINE_SIZE, notice_width));
    lines.push_back(fitted(text.at(2), INTERRUPTION_LINE_SIZE,
                           notice_width - BUTTON_TEXT_X));
  }
  // Words on a button stay inside its rim.
  const float button = 2.0F * BUTTON_TEXT_X;
  lines.push_back(
      fitted(appraise_job(frame), JOB_SIZE, appraise_inner + 8.0F - button));
  lines.push_back(
      fitted(cook_job(frame), JOB_SIZE,
             COOK_WIDTH - 2.0F * INSET - COOK_OFFER_TEXT + 6.0F - button));
  for (const auto &job : feed_jobs(frame)) {
    lines.push_back(fitted(job, JOB_SIZE, feed_inner + 8.0F - button));
  }
  for (const auto &rule : FEED_RULES) {
    lines.push_back(fitted(rule, 13, feed_inner));
  }
  const float card_tools =
      CARD_WIDTH - 2.0F * INSET - CARD_COLUMN - CARD_TOOL_TEXT;
  lines.push_back(fitted(NO_SPARE_TOOLS, CARD_TOOLS_SIZE,
                         card_tools - (CARD_SPARES - CARD_TOOL_TEXT)));
  for (const auto state :
       {survival::ToolState::Intact, survival::ToolState::Damaged}) {
    lines.push_back(fitted(tool_condition(state), CARD_TOOLS_SIZE,
                           CARD_SPARES - CARD_TOOL_TEXT - 2.0F));
  }
  const auto &tuning = frame.content.get().companion.workshop;
  for (const auto &recipe : tuning.recipes) {
    lines.push_back(fitted(cook_forecast(frame, recipe.food), FORECAST_SIZE,
                           COOK_WIDTH - 2.0F * INSET - COOK_OFFER_TEXT));
    lines.push_back(fitted(from(survival::food_name(recipe.food)),
                           COOK_NAME_SIZE, COOK_STATUS_X - 12.0F - 4.0F));
    lines.push_back(fitted(dish_label({.food = recipe.food,
                                       .quality = survival::DishQuality::Fine}),
                           FEED_NAME_SIZE, feed_inner - 54.0F));
    const auto subsets = std::size_t{1} << recipe.ingredients.size();
    for (std::size_t held = 0; held < subsets; ++held) {
      for (std::size_t known = 0; known < subsets; ++known) {
        lines.push_back(
            fitted(needs_text(needs_given(recipe.ingredients, held, known)),
                   COOK_STATUS_SIZE, COOK_EATEN_X - COOK_STATUS_X - 4.0F));
      }
    }
  }
  return lines;
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
        } else if constexpr (std::is_same_v<Happened, EscapeMissEvent>) {
          return "Day " + number(happened.day) + ": the way out stayed shut.";
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
