#include "survival.hpp"

#include "../rendering/survival.hpp"

#include <survival/escape.hpp>

#include <godot_cpp/variant/vector2.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
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
using rendering::Weight;
using survival::Actor;
using survival::GameView;
using survival::GridPosition;
using survival::IngredientId;
using survival::Location;
using survival::Phase;

constexpr std::array ACTORS{Actor::Chef, Actor::Appraiser};
constexpr float SCREEN_WIDTH = 1280.0F;
constexpr float SCREEN_HEIGHT = 720.0F;
constexpr float INSET = rendering::PANEL_INSET;
/// Every panel keeps this far from the screen's edge and from its neighbour.
constexpr float MARGIN = 8.0F;
constexpr float GAP = 6.0F;
/// Words that change with play draw in this weight, the one the harness
/// measures them in.
constexpr Weight CHANGING = Weight::SemiBold;
constexpr int TITLE_SIZE = 22;
constexpr int BUTTON_SIZE = 18;
constexpr float BUTTON_PAD = 14.0F;
/// The top band's cards end above the map at y 70.
constexpr float BAND_Y = MARGIN;
constexpr float BAND_HEIGHT = 54.0F;
constexpr float BAND_LABEL = 26.0F;
constexpr float BAND_VALUE = 48.0F;
constexpr int BIG_SIZE = 26;
constexpr int LABEL_SIZE = 13;
constexpr float DAY_X = MARGIN;
constexpr float DAY_WIDTH = 220.0F;
constexpr int GOAL_SIZE = 13;
constexpr float GOAL_WIDTH = DAY_WIDTH - 2.0F * 12.0F;
constexpr float ACID_X = DAY_X + DAY_WIDTH + MARGIN;
constexpr float ACID_WIDTH = 330.0F;
constexpr int ACID_SIZE = 18;
constexpr float TIME_X = ACID_X + ACID_WIDTH + MARGIN;
constexpr float TIME_WIDTH = 160.0F;
constexpr float HINT_X = TIME_X + TIME_WIDTH + MARGIN;
constexpr float HINT_WIDTH = 158.0F;
constexpr float LOG_X = HINT_X + HINT_WIDTH + MARGIN;
constexpr float LOG_WIDTH = SCREEN_WIDTH - MARGIN - LOG_X;
constexpr int EVENT_SIZE = 13;
constexpr float EVENT_LINE_HEIGHT = 17.0F;
/// The NEXT bar along the bottom edge; a hand-off notice takes its second line.
constexpr float NEXT_Y = 666.0F;
constexpr float NEXT_HEIGHT = SCREEN_HEIGHT - MARGIN - NEXT_Y;
constexpr float NEXT_TEXT_X = 84.0F;
constexpr int NEXT_SIZE = 20;
constexpr int HAND_OFF_SIZE = 13;
constexpr float NEXT_WIDTH = SCREEN_WIDTH - MARGIN - INSET - NEXT_TEXT_X;
/// The city's cards sit below the labels of the lowest map row.
constexpr float CARD_Y = 540.0F;
constexpr float CARD_HEIGHT = NEXT_Y - GAP - CARD_Y;
constexpr float CARD_WIDTH = 314.0F;
constexpr float CARD_PAD = 12.0F;
constexpr float PORTRAIT = 64.0F;
/// Where a card's words start, right of the portrait.
constexpr float CARD_COLUMN = CARD_PAD + PORTRAIT + 12.0F;
constexpr float CARD_TOOL_TEXT = 38.0F;
constexpr float CARD_SPARES = 150.0F;
constexpr int CARD_TOOLS_SIZE = 13;
constexpr float HEART_PITCH = 26.0F;
/// Tool icons are narrow inside their 48 px, so they sit closer than that.
constexpr float TOOL_PITCH = 24.0F;
constexpr float ICON = 48.0F;
constexpr float PACK_X = MARGIN + 2.0F * (CARD_WIDTH + GAP);
constexpr float PACK_WIDTH = 236.0F;
constexpr float ACTION_X = PACK_X + PACK_WIDTH + GAP;
constexpr float ACTION_WIDTH = SCREEN_WIDTH - MARGIN - ACTION_X;
constexpr float ACTION_BUTTON_HEIGHT = 50.0F;
constexpr int ACTION_SIZE = 22;
constexpr float TRAVEL_HEIGHT = 38.0F;
/// Button widths of the action card: a lone human's row, then each row of two
/// humans after its player tag.
constexpr float SOLO_INTERACT = 190.0F;
constexpr float SOLO_TRAVEL = 188.0F;
constexpr float PLAYER_TAG = 30.0F;
constexpr float DUO_INTERACT = 120.0F;
constexpr float DUO_TRAVEL = 188.0F;
/// The workshop's panels, under the kitchen floor at y 318.
constexpr float PANEL_Y = 324.0F;
constexpr float PANEL_HEIGHT = NEXT_Y - GAP - PANEL_Y;
constexpr float APPRAISE_X = MARGIN;
constexpr float APPRAISE_WIDTH = 312.0F;
constexpr float COOK_X = APPRAISE_X + APPRAISE_WIDTH + GAP;
constexpr float COOK_WIDTH = 608.0F;
constexpr float FEED_X = COOK_X + COOK_WIDTH + GAP;
constexpr float FEED_WIDTH = SCREEN_WIDTH - MARGIN - FEED_X;
/// A panel's entrance lifts it this far into place.
constexpr float ENTRANCE_LIFT = 18.0F;
constexpr float TILE = 56.0F;
constexpr float TILE_PITCH = 64.0F;
constexpr std::size_t TILES_PER_ROW = 4;
constexpr float JOB_HEIGHT = 46.0F;
constexpr float COOK_STATUS_X = 150.0F;
/// Where a ready recipe's eaten or hated mark starts.
constexpr float READY_EATEN_X = 180.0F;
constexpr float COOK_EATEN_X = 470.0F;
constexpr int COOK_STATUS_SIZE = 13;
constexpr int COOK_NAME_SIZE = 14;
constexpr int READY_SIZE = 18;
constexpr float READY_ROW = 28.0F;
constexpr float COOK_ROW_HEIGHT = 15.0F;
/// The offered dish's row: its icon, then its name over its forecast.
constexpr float OFFER_ROW = 50.0F;
constexpr float OFFER_TEXT = 62.0F;
constexpr int FORECAST_SIZE = 18;
constexpr int EATEN_SIZE = 14;
constexpr std::size_t EATEN_PER_LINE = 4;
constexpr float FEED_ROW = 44.0F;
constexpr int FEED_NAME_SIZE = 18;
constexpr float SERVE_HEIGHT = 54.0F;
constexpr int SERVE_SIZE = 24;
/// The day's result, over the kitchen and clear of the two actors' heads.
constexpr float RESULT_X = 16.0F;
constexpr float RESULT_Y = 72.0F;
constexpr float RESULT_WIDTH = 760.0F;
constexpr float RESULT_LINE_HEIGHT = 22.0F;
constexpr int RESULT_SIZE = 16;
constexpr float PAUSE_BOX_X = 190.0F;
constexpr float PAUSE_BOX_Y = 28.0F;
constexpr float PAUSE_BOX_WIDTH = 900.0F;
constexpr float PAUSE_BOX_HEIGHT = 664.0F;
constexpr float PAUSE_TEXT_X = 230.0F;
constexpr float PAUSE_FIRST_X = 700.0F;
constexpr float PAUSE_SECOND_X = 890.0F;
constexpr float PAUSE_ROWS_Y = 172.0F;
constexpr float PAUSE_ROW_HEIGHT = 38.0F;
constexpr int PAUSE_ROW_SIZE = 18;
constexpr float NOTICE_BOX_X = 290.0F;
constexpr float NOTICE_BOX_Y = 236.0F;
constexpr float NOTICE_BOX_WIDTH = 700.0F;
constexpr float NOTICE_BOX_HEIGHT = 240.0F;
constexpr float NOTICE_TEXT_X = 326.0F;
constexpr int INTERRUPTION_TITLE_SIZE = 34;
constexpr int INTERRUPTION_LINE_SIZE = 20;
constexpr const char *NO_SPARE_TOOLS = "No spares";
constexpr const char *UNIDENTIFIED_MARK = "? = not yet identified";
constexpr std::array FEED_RULES{"New and delicious food holds the acid back.",
                                "Anything else raises it.",
                                "Full acid ends the run."};
/// The pause key's hint in the top band, where a first player looks for keys.
constexpr std::array MENU_HINTS{"Pause, keys", "Main menu"};
constexpr float SETUP_X = 200.0F;
constexpr float SETUP_Y = 170.0F;
constexpr float SETUP_WIDTH = 880.0F;
constexpr float SETUP_HEIGHT = 116.0F;
constexpr float SETUP_PITCH = 128.0F;

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
  const char *jump;
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
                                          .leave = "X",
                                          .jump = "Space"},
                                KeyLabels{.move = "IJKL",
                                          .every_move = "IJKL",
                                          .run = "H",
                                          .travel = "U",
                                          .interact = "O",
                                          .replace_tool = "P",
                                          .feed = "[",
                                          .end_day = "]",
                                          .switch_actor = "B",
                                          .leave = "N",
                                          .jump = ";"}};

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
    return survival::cell_of(view.exploration.chef.body.position);
  case Actor::Appraiser:
    return survival::cell_of(view.exploration.appraiser.body.position);
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
               rendering::palette().white, CHANGING);
}

/// Words on a small rounded chip, for counts and marks drawn over an icon.
void chip(godot::Node2D &canvas, const rendering::Art &drawing, float x,
          float baseline, const String &text, int size, godot::Color tint) {
  const auto height = static_cast<float>(size);
  rendering::rounded(canvas,
                     rect(x - 4.0F, baseline - height + 1.0F,
                          drawing.text_width(text, size, CHANGING) + 8.0F,
                          height + 4.0F),
                     5.0F, rendering::palette().background);
  drawing.text(canvas, x, baseline, text, size, tint, CHANGING);
}

/// Draws `label` from `x`, its leading "[key]" in `key_tint` and the rest in
/// `tint`, so a key reads as a keycap ahead of its action.
void keyed_text(godot::Node2D &canvas, const rendering::Art &drawing, float x,
                float baseline, const String &label, int size,
                godot::Color tint, godot::Color key_tint) {
  const auto close = label.begins_with("[") ? label.find("] ") : -1;
  if (close < 0) {
    drawing.text(canvas, x, baseline, label, size, tint, CHANGING);
    return;
  }
  const String key_part = label.substr(0, close + 2);
  drawing.text(canvas, x, baseline, key_part, size, key_tint, CHANGING);
  drawing.text(canvas, x + drawing.text_width(key_part, size, CHANGING),
               baseline, label.substr(close + 2), size, tint, CHANGING);
}

/// A flat button with its words centred on its height, dark on the accent.
void paint_button(godot::Node2D &canvas, const rendering::Art &drawing,
                  godot::Rect2 area, const String &text, int size,
                  bool marked) {
  const auto tones = rendering::palette();
  rendering::button(canvas, area, marked);
  const float lip = marked ? 4.0F : 0.0F;
  keyed_text(canvas, drawing, area.position.x + BUTTON_PAD,
             area.position.y +
                 (area.size.y - lip + static_cast<float>(size) * 0.72F) / 2.0F,
             text, size, marked ? tones.ink : tones.white,
             marked ? color(110, 70, 10, 1.0F) : tones.gold);
}

/// How wide a button must be to hold `text` at `size`.
float button_width(const rendering::Art &drawing, const String &text,
                   int size) {
  return drawing.text_width(text, size, CHANGING) + 2.0F * BUTTON_PAD;
}

/// A rounded trough with its eased fill, for a value against its maximum.
void bar(godot::Node2D &canvas, const rendering::Art &drawing,
         const String &name, godot::Rect2 area, float fill, godot::Color tint) {
  const float shown = drawing.glide(name, std::clamp(fill, 0.0F, 1.0F));
  rendering::rounded(canvas, area, area.size.y / 2.0F,
                     rendering::palette().background);
  if (shown > 0.0F) {
    rendering::rounded(canvas,
                       rect(area.position.x, area.position.y,
                            std::max(area.size.y, area.size.x * shown),
                            area.size.y),
                       area.size.y / 2.0F, tint);
  }
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

/// Adds to `seen` every cell movement reaches from `from`.
void add_walkable(Cells &seen, const survival::WorldSnapshot &world,
                  GridPosition from) {
  for (const auto cell :
       survival::reachable_cells(survival::CellMap(world), from)) {
    seen.insert({cell.x, cell.y});
  }
}

/// The first move of an errand, worded for a human holding `keys`.
String move_words(survival::Move move, const KeyLabels &keys) {
  switch (move) {
  case survival::Move::Left:
    return "Walk left";
  case survival::Move::Right:
    return "Walk right";
  case survival::Move::ClimbUp:
    return "Climb up";
  case survival::Move::ClimbDown:
    return "Climb down";
  case survival::Move::JumpLeft:
    return "Jump left with " + from(keys.jump);
  case survival::Move::JumpRight:
    return "Jump right with " + from(keys.jump);
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
  const bool walking = offers.errand && offers.errand->step.has_value();
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
  if (!offers.errand->step) {
    return "Walk over to " +
           from(survival::object_name(offers.errand->target)) + ". " +
           from(keys.travel) + " returns.";
  }
  return move_words(*offers.errand->step, keys) + " to " +
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

/// A small upper-case caption over a value, as the top band's cards show.
void caption(godot::Node2D &canvas, const rendering::Art &drawing, float x,
             float baseline, const String &text, godot::Color tint) {
  drawing.text(canvas, x, baseline, text, LABEL_SIZE, tint, Weight::SemiBold);
}

/// A key on a chip followed by what it does, for the top band's hint.
void key_hint(godot::Node2D &canvas, const rendering::Art &drawing, float x,
              float baseline, const String &key_name, const String &action) {
  const auto tones = rendering::palette();
  const float width = drawing.text_width(key_name, LABEL_SIZE, Weight::Bold);
  rendering::rounded(canvas, rect(x, baseline - 14.0F, width + 12.0F, 19.0F),
                     5.0F, tones.raised);
  drawing.text(canvas, x + 6.0F, baseline, key_name, LABEL_SIZE, tones.white,
               Weight::Bold);
  drawing.text(canvas, x + 44.0F, baseline, action, LABEL_SIZE, tones.muted);
}

void paint_top_bar(godot::Node2D &canvas, const rendering::Art &drawing,
                   const SurvivalFrame &frame) {
  const auto tones = rendering::palette();
  const auto &progression = frame.view.progression;
  rendering::panel(canvas, rect(DAY_X, BAND_Y, DAY_WIDTH, BAND_HEIGHT));
  drawing.image(canvas, rendering::Image::IconDay,
                Vector2(DAY_X + 10.0F, BAND_Y + 4.0F));
  drawing.text(canvas, DAY_X + 50.0F, BAND_Y + 30.0F,
               "DAY " + number(progression.day), BIG_SIZE, tones.gold,
               Weight::Bold);
  drawing.text(canvas, DAY_X + 12.0F, BAND_Y + BAND_VALUE - 2.0F, escape_goal(),
               GOAL_SIZE, tones.muted, CHANGING);
  rendering::panel(canvas, rect(ACID_X, BAND_Y, ACID_WIDTH, BAND_HEIGHT));
  drawing.image(canvas, rendering::Image::IconAcid,
                Vector2(ACID_X + 10.0F, BAND_Y + 11.0F));
  const float acid_left = ACID_X + 50.0F;
  const float acid_right = ACID_X + ACID_WIDTH - INSET;
  drawing.text(canvas, acid_left, BAND_Y + BAND_LABEL, "ACID", ACID_SIZE,
               tones.success, Weight::Bold);
  const String level = number(progression.acid.value) + " / " +
                       number(frame.content.get().run.full_acid.value);
  drawing.text(
      canvas, acid_right - drawing.text_width(level, ACID_SIZE, Weight::Bold),
      BAND_Y + BAND_LABEL, level, ACID_SIZE, tones.white, Weight::Bold);
  bar(canvas, drawing, "acid",
      rect(acid_left, BAND_Y + 34.0F, acid_right - acid_left, 10.0F),
      acid_ratio(frame), tones.success);
  rendering::panel(canvas, rect(TIME_X, BAND_Y, TIME_WIDTH, BAND_HEIGHT));
  drawing.image(canvas, rendering::Image::IconClock,
                Vector2(TIME_X + 10.0F, BAND_Y + 11.0F));
  caption(canvas, drawing, TIME_X + 50.0F, BAND_Y + 20.0F, "TIME LEFT",
          tones.muted);
  drawing.text(canvas, TIME_X + 50.0F, BAND_Y + BAND_VALUE,
               clock(progression.remaining_exploration), BIG_SIZE, tones.white,
               Weight::Bold);
  rendering::panel(canvas, rect(HINT_X, BAND_Y, HINT_WIDTH, BAND_HEIGHT));
  key_hint(canvas, drawing, HINT_X + 12.0F, BAND_Y + 24.0F, "Esc",
           MENU_HINTS.at(0));
  key_hint(canvas, drawing, HINT_X + 12.0F, BAND_Y + 45.0F, "M",
           MENU_HINTS.at(1));
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
                 rendering::palette().faint, CHANGING);
    return;
  }
  float left = x;
  for (const auto &[value, count] : spares) {
    paint_item(canvas, drawing, survival::tool_name(survival::ToolId{value}),
               Vector2(left, y));
    if (count > 1) {
      chip(canvas, drawing, left + 28.0F, y + 46.0F, "x" + number(count),
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
  drawing.text(canvas, x + CARD_TOOL_TEXT + 8.0F, y + 20.0F,
               role_tool_name(actor), 14, tones.white, Weight::SemiBold);
  drawing.text(canvas, x + CARD_TOOL_TEXT + 8.0F, y + 38.0F,
               tool_condition(tool), CARD_TOOLS_SIZE,
               tool == survival::ToolState::Intact ? tones.muted : tones.danger,
               CHANGING);
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
  rendering::panel(canvas, rect(x, CARD_Y, CARD_WIDTH, CARD_HEIGHT));
  const float left = x + CARD_PAD;
  const float top = CARD_Y + CARD_PAD;
  rendering::rounded(canvas, rect(left, top, PORTRAIT, PORTRAIT), 10.0F,
                     tones.raised);
  drawing.image(canvas,
                actor == Actor::Chef ? rendering::Image::ChefIdle
                                     : rendering::Image::AppraiserIdle,
                Vector2(left, top - 4.0F));
  const bool out = state.location == Location::City;
  const String where = out ? "in city" : "at base";
  rendering::rounded(canvas, rect(left, top + PORTRAIT + 6.0F, PORTRAIT, 22.0F),
                     11.0F, tones.background);
  drawing.text(canvas,
               left +
                   (PORTRAIT - drawing.text_width(where, 13, CHANGING)) / 2.0F,
               top + PORTRAIT + 22.0F, where, 13, out ? tones.cyan : tones.gold,
               CHANGING);
  const float column = x + CARD_COLUMN;
  const String name = actor_name(actor);
  drawing.text(canvas, column, top + 16.0F, name, 18, tones.white,
               Weight::Bold);
  const String controller = keys ? "P" + number(*keys + 1) : String("AI");
  const float tag_x =
      column + drawing.text_width(name, 18, Weight::Bold) + 8.0F;
  rendering::rounded(
      canvas,
      rect(tag_x, top + 1.0F,
           drawing.text_width(controller, 13, Weight::Bold) + 12.0F, 19.0F),
      6.0F, keys ? tones.cyan : tones.raised);
  drawing.text(canvas, tag_x + 6.0F, top + 15.0F, controller, 13,
               keys ? tones.ink : tones.muted, Weight::Bold);
  paint_hearts(canvas, drawing, column - 4.0F, top + 20.0F, state.health);
  const float gauge = column + 3.0F * HEART_PITCH + 6.0F;
  bar(canvas, drawing, "stamina" + number(index_of(actor)),
      rect(gauge, top + 32.0F, x + CARD_WIDTH - CARD_PAD - gauge, 8.0F),
      stamina, tones.cyan);
  paint_role_tool(canvas, drawing, view, actor, column - 8.0F, top + 52.0F);
  paint_spare_tools(canvas, drawing, state, column + CARD_SPARES, top + 52.0F);
}

void paint_pack(godot::Node2D &canvas, const rendering::Art &drawing,
                const SurvivalFrame &frame) {
  const auto tones = rendering::palette();
  const auto &view = frame.view;
  rendering::panel(canvas, rect(PACK_X, CARD_Y, PACK_WIDTH, CARD_HEIGHT));
  const float left = PACK_X + INSET;
  const float top = CARD_Y + CARD_PAD;
  drawing.text(canvas, left, top + 18.0F, "PACK", 18, tones.white,
               Weight::Bold);
  drawing.text(canvas, left + 58.0F, top + 17.0F, UNIDENTIFIED_MARK, 13,
               tones.faint);
  const auto pack = pack_of(view);
  if (pack.empty()) {
    drawing.text(canvas, left, top + 56.0F, "Empty", 15, tones.faint, CHANGING);
  }
  const float room = PACK_WIDTH - 2.0F * INSET - ICON;
  const float pitch =
      pack.size() > 1
          ? std::min(ICON + 4.0F, room / static_cast<float>(pack.size() - 1))
          : 0.0F;
  float column = left;
  for (const auto &[value, count] : pack) {
    const auto ingredient = IngredientId{value};
    const bool known = identified(view, ingredient);
    paint_item(canvas, drawing, survival::ingredient_name(ingredient),
               Vector2(column, top + 22.0F));
    const String label = number(count) + (known ? "" : "?");
    chip(canvas, drawing,
         column + ICON - drawing.text_width(label, 13, CHANGING) - 2.0F,
         top + 70.0F, label, 13, known ? tones.white : tones.gold);
    column += pitch;
  }
  drawing.text(canvas, left, CARD_Y + CARD_HEIGHT - CARD_PAD - 2.0F,
               "Each collection spends " + collection_clock(frame), 13,
               tones.muted, CHANGING);
}

/// What one human's interact key does in the city, for its button.
String interact_label(const SurvivalFrame &frame, Actor actor,
                      const KeyLabels &keys) {
  const auto &offer = frame.offers.at(index_of(actor)).interact;
  const char *verb = "Use";
  if (offer && std::holds_alternative<survival::Collect>(*offer)) {
    verb = "Collect";
  } else if (offer && std::holds_alternative<survival::Repair>(*offer)) {
    verb = "Repair";
  }
  return key(keys.interact) + verb;
}

/// What the interact key would act on, under its button; empty with none.
String interact_target(const SurvivalFrame &frame, Actor actor) {
  const auto &offer = frame.offers.at(index_of(actor)).interact;
  if (!offer) {
    return {};
  }
  if (const auto *collect = std::get_if<survival::Collect>(&*offer)) {
    return from(survival::object_name(collect->source));
  }
  if (const auto *repair = std::get_if<survival::Repair>(&*offer)) {
    return from(survival::object_name(repair->target));
  }
  return {};
}

String travel_label(const SurvivalFrame &frame, Actor actor,
                    const KeyLabels &keys) {
  return key(keys.travel) +
         (state_of(frame.view, actor).location == Location::City
              ? "Return to base"
              : "Set out");
}

/// Whether the travel key is what NEXT asks of this human now: home with no
/// errand left, or out while one waits.
bool travel_asked(const SurvivalFrame &frame, Actor actor) {
  if (state_of(frame.view, actor).location == Location::City) {
    return !frame.offers.at(index_of(actor)).errand.has_value();
  }
  return worth_setting_out(frame, actor);
}

/// The movement keys a lone human reads beside its travel button.
String move_hint(const KeyLabels &keys) {
  return from(keys.move) + " move  " + from(keys.run) + " run  " +
         from(keys.jump) + " jump";
}

/// The humans in the city phase, chef first, with their key sets.
std::vector<std::pair<Actor, std::size_t>> humans(const SurvivalFrame &frame) {
  std::vector<std::pair<Actor, std::size_t>> found;
  for (const auto actor : ACTORS) {
    if (const auto keys = survival::key_set(frame.control, actor)) {
      found.emplace_back(actor, *keys);
    }
  }
  return found;
}

/// The city's action card: each human's interact key as a big button, lit
/// while it acts now, and the travel key beside it.
void paint_actions(godot::Node2D &canvas, const rendering::Art &drawing,
                   const SurvivalFrame &frame) {
  const auto tones = rendering::palette();
  rendering::panel(canvas, rect(ACTION_X, CARD_Y, ACTION_WIDTH, CARD_HEIGHT));
  const float left = ACTION_X + CARD_PAD;
  const float top = CARD_Y + CARD_PAD;
  const auto players = humans(frame);
  if (players.size() == 1) {
    const auto [actor, set] = players.front();
    const auto &keys = KEY_LABELS.at(set);
    const bool acts = frame.offers.at(index_of(actor)).interact.has_value();
    paint_button(canvas, drawing,
                 rect(left, top, SOLO_INTERACT, ACTION_BUTTON_HEIGHT),
                 interact_label(frame, actor, keys), ACTION_SIZE, acts);
    drawing.text(canvas, left + SOLO_INTERACT + 12.0F, top + 31.0F,
                 interact_target(frame, actor), 16, tones.white, CHANGING);
    const float row = CARD_Y + CARD_HEIGHT - CARD_PAD - TRAVEL_HEIGHT;
    paint_button(canvas, drawing, rect(left, row, SOLO_TRAVEL, TRAVEL_HEIGHT),
                 travel_label(frame, actor, keys), BUTTON_SIZE,
                 travel_asked(frame, actor));
    drawing.text(canvas, left + SOLO_TRAVEL + 10.0F, row + 25.0F,
                 move_hint(keys), 14, tones.muted, CHANGING);
    return;
  }
  const float height = (CARD_HEIGHT - 2.0F * CARD_PAD - GAP) / 2.0F;
  float row = top;
  for (const auto &[actor, set] : players) {
    const auto &keys = KEY_LABELS.at(set);
    drawing.text(canvas, left, row + 27.0F, "P" + number(set + 1), 16,
                 tones.cyan, Weight::Bold);
    paint_button(canvas, drawing,
                 rect(left + PLAYER_TAG, row, DUO_INTERACT, height),
                 interact_label(frame, actor, keys), BUTTON_SIZE,
                 frame.offers.at(index_of(actor)).interact.has_value());
    paint_button(
        canvas, drawing,
        rect(left + PLAYER_TAG + DUO_INTERACT + GAP, row, DUO_TRAVEL, height),
        travel_label(frame, actor, keys), BUTTON_SIZE,
        travel_asked(frame, actor));
    row += height + GAP;
  }
}

/// The log card at the top band's right end, sized to its lines.
void paint_events(godot::Node2D &canvas, const rendering::Art &drawing,
                  const SurvivalFrame &frame) {
  if (frame.events.empty()) {
    return;
  }
  const auto count = static_cast<float>(frame.events.size());
  rendering::panel(
      canvas, rect(LOG_X, BAND_Y, LOG_WIDTH,
                   std::max(BAND_HEIGHT, count * EVENT_LINE_HEIGHT + 18.0F)));
  float line = BAND_Y + 22.0F;
  std::size_t index = 0;
  for (const auto &event : frame.events) {
    // The newest line is bright; older ones step back.
    const bool newest = ++index == frame.events.size();
    drawing.text(canvas, LOG_X + CARD_PAD, line, describe(event), EVENT_SIZE,
                 newest ? rendering::palette().white
                        : rendering::palette().muted,
                 CHANGING);
    line += EVENT_LINE_HEIGHT;
  }
}

void paint_next_step(godot::Node2D &canvas, const rendering::Art &drawing,
                     const SurvivalFrame &frame) {
  const auto tones = rendering::palette();
  rendering::panel(
      canvas, rect(MARGIN, NEXT_Y, SCREEN_WIDTH - 2.0F * MARGIN, NEXT_HEIGHT));
  const String notice = hand_off_notice(frame);
  const float baseline =
      NEXT_Y + (notice.is_empty() ? NEXT_HEIGHT / 2.0F + 7.0F : 21.0F);
  rendering::rounded(canvas,
                     rect(MARGIN + 10.0F, baseline - 18.0F, 60.0F, 24.0F), 6.0F,
                     tones.cyan);
  drawing.text(canvas, MARGIN + 17.0F, baseline, "NEXT", 15, tones.ink,
               Weight::Bold);
  keyed_text(canvas, drawing, NEXT_TEXT_X, baseline, next_step(frame),
             NEXT_SIZE, tones.white, tones.gold);
  if (!notice.is_empty()) {
    drawing.text(canvas, NEXT_TEXT_X, baseline + 17.0F, notice, HAND_OFF_SIZE,
                 tones.gold, CHANGING);
  }
}

/// A panel's title, as every workshop panel opens.
void panel_title(godot::Node2D &canvas, const rendering::Art &drawing, float x,
                 float y, const String &title) {
  drawing.text(canvas, x, y + 28.0F, title, TITLE_SIZE,
               rendering::palette().white, Weight::Bold);
}

/// A rounded tile holding an icon, as the pack, the offer and the store show
/// them; `marked` rings it in the accent.
void paint_tile(godot::Node2D &canvas, const rendering::Art &drawing,
                Vector2 at, std::optional<std::string_view> item, bool marked) {
  const auto tones = rendering::palette();
  if (marked) {
    rendering::rounded(canvas,
                       rect(at.x - 2.0F, at.y - 2.0F, TILE + 4.0F, TILE + 4.0F),
                       11.0F, tones.gold);
  }
  rendering::rounded(canvas, rect(at.x, at.y, TILE, TILE), 9.0F, tones.raised);
  if (item) {
    paint_item(canvas, drawing, *item,
               at + Vector2((TILE - ICON) / 2.0F, (TILE - ICON) / 2.0F));
  }
}

void paint_appraise_panel(godot::Node2D &canvas, const rendering::Art &drawing,
                          const SurvivalFrame &frame, float lift) {
  const auto tones = rendering::palette();
  const auto &view = frame.view;
  const float y = PANEL_Y + lift;
  rendering::panel(canvas, rect(APPRAISE_X, y, APPRAISE_WIDTH, PANEL_HEIGHT));
  const float left = APPRAISE_X + INSET;
  const float top = y + INSET;
  panel_title(canvas, drawing, left, top - 6.0F, "APPRAISE");
  paint_role_tool(canvas, drawing, view, Actor::Appraiser, left + 116.0F,
                  top - 10.0F);
  const auto pack = pack_of(view);
  if (pack.empty()) {
    drawing.text(canvas, left, top + 80.0F, "Nothing carried.", 16, tones.muted,
                 CHANGING);
    drawing.text(canvas, left, top + 102.0F, "Find ingredients.", 16,
                 tones.muted, CHANGING);
  }
  const auto &offer = frame.offers.at(index_of(Actor::Appraiser)).interact;
  const auto *next = offer ? std::get_if<survival::Appraise>(&*offer) : nullptr;
  std::size_t index = 0;
  for (const auto &[value, count] : pack) {
    const auto ingredient = IngredientId{value};
    const std::size_t rows_above = index / TILES_PER_ROW;
    const Vector2 at(left +
                         static_cast<float>(index % TILES_PER_ROW) * TILE_PITCH,
                     top + 48.0F + static_cast<float>(rows_above) * TILE_PITCH);
    paint_tile(canvas, drawing, at, survival::ingredient_name(ingredient),
               next != nullptr && next->ingredient == ingredient);
    chip(canvas, drawing, at.x + 34.0F, at.y + TILE - 3.0F, "x" + number(count),
         13, tones.white);
    if (!identified(view, ingredient)) {
      chip(canvas, drawing, at.x + TILE - 10.0F, at.y + 16.0F, "?", 15,
           tones.gold);
    }
    ++index;
  }
  const float foot = y + PANEL_HEIGHT - INSET - JOB_HEIGHT;
  drawing.text(canvas, left, foot - 62.0F, UNIDENTIFIED_MARK, 13, tones.faint);
  float line = foot - 40.0F;
  for (const auto &text : eaten_lines(view.workshop.eaten_ingredients)) {
    drawing.text(canvas, left, line, text, EATEN_SIZE, tones.muted, CHANGING);
    line += 18.0F;
  }
  paint_button(canvas, drawing,
               rect(left, foot, APPRAISE_WIDTH - 2.0F * INSET, JOB_HEIGHT),
               appraise_job(frame), BUTTON_SIZE,
               survival::key_set(frame.control, Actor::Appraiser) &&
                   offer.has_value());
}

/// The eaten or hated mark of a recipe the monster has had; empty otherwise.
String eaten_mark(const SurvivalFrame &frame, survival::FoodId food) {
  const auto &eaten = frame.view.workshop.eaten_foods;
  const auto &rejected = frame.view.workshop.rejected_foods;
  if (std::ranges::find(eaten, food) == eaten.end()) {
    return {};
  }
  return std::ranges::find(rejected, food) != rejected.end() ? "hated"
                                                             : "eaten";
}

/// Draws a recipe's eaten or hated mark, the hated one in the danger tone.
void paint_eaten_mark(godot::Node2D &canvas, const rendering::Art &drawing,
                      const SurvivalFrame &frame, survival::FoodId food,
                      float x, float baseline) {
  const String mark = eaten_mark(frame, food);
  drawing.text(canvas, x, baseline, mark, COOK_STATUS_SIZE,
               mark == "hated" ? rendering::palette().danger
                               : rendering::palette().muted,
               CHANGING);
}

/// Draws a taste right-aligned to `right`.
void paint_taste(godot::Node2D &canvas, const rendering::Art &drawing,
                 const SurvivalFrame &frame, const survival::Recipe &recipe,
                 float right, float baseline, int size, godot::Color tint) {
  const String taste = number(
      survival::taste_of(recipe, frame.content.get().companion.workshop));
  drawing.text(canvas, right - drawing.text_width(taste, size, CHANGING),
               baseline, taste, size, tint, CHANGING);
}

/// The dish the chef's key would cook: a wide row with its icon, name, taste
/// and forecast, ahead of the other ready recipes.
void paint_offered_recipe(godot::Node2D &canvas, const rendering::Art &drawing,
                          const SurvivalFrame &frame,
                          const survival::Recipe &recipe, godot::Rect2 area) {
  const auto tones = rendering::palette();
  rendering::rounded(canvas, area, 8.0F, color(70, 54, 24, 1.0F));
  rendering::rounded(canvas,
                     rect(area.position.x, area.position.y, 4.0F, area.size.y),
                     2.0F, tones.gold);
  const float x = area.position.x;
  const float y = area.position.y;
  paint_item(canvas, drawing, survival::food_name(recipe.food),
             Vector2(x + 8.0F, y + 2.0F));
  drawing.text(canvas, x + OFFER_TEXT, y + 21.0F,
               from(survival::food_name(recipe.food)), READY_SIZE, tones.gold,
               CHANGING);
  paint_eaten_mark(canvas, drawing, frame, recipe.food,
                   x + OFFER_TEXT + READY_EATEN_X, y + 19.0F);
  paint_taste(canvas, drawing, frame, recipe, x + area.size.x - 12.0F,
              y + 21.0F, READY_SIZE, tones.white);
  drawing.text(canvas, x + OFFER_TEXT, y + 44.0F, offered_forecast(frame),
               FORECAST_SIZE, tones.white, CHANGING);
}

/// One other recipe the chef can cook now, in one of two columns.
void paint_ready_recipe(godot::Node2D &canvas, const rendering::Art &drawing,
                        const SurvivalFrame &frame,
                        const survival::Recipe &recipe, godot::Rect2 area) {
  const auto tones = rendering::palette();
  rendering::rounded(canvas, area, 8.0F, tones.raised);
  const float baseline = area.position.y + 20.0F;
  drawing.text(canvas, area.position.x + 12.0F, baseline,
               from(survival::food_name(recipe.food)), READY_SIZE, tones.white,
               CHANGING);
  paint_eaten_mark(canvas, drawing, frame, recipe.food,
                   area.position.x + READY_EATEN_X, baseline - 1.0F);
  paint_taste(canvas, drawing, frame, recipe,
              area.position.x + area.size.x - 12.0F, baseline, READY_SIZE,
              tones.white);
}

/// One recipe still short of ingredients, small, on its own line.
void paint_waiting_recipe(godot::Node2D &canvas, const rendering::Art &drawing,
                          const SurvivalFrame &frame,
                          const survival::Recipe &recipe, float left,
                          float right, float baseline) {
  const auto tones = rendering::palette();
  drawing.text(canvas, left, baseline, from(survival::food_name(recipe.food)),
               COOK_NAME_SIZE, tones.muted, CHANGING);
  drawing.text(canvas, left + COOK_STATUS_X, baseline,
               needs_text(recipe_needs(frame.view, recipe)), COOK_STATUS_SIZE,
               tones.faint, CHANGING);
  paint_eaten_mark(canvas, drawing, frame, recipe.food, left + COOK_EATEN_X,
                   baseline);
  paint_taste(canvas, drawing, frame, recipe, right, baseline, COOK_NAME_SIZE,
              tones.muted);
}

void paint_cook_panel(godot::Node2D &canvas, const rendering::Art &drawing,
                      const SurvivalFrame &frame, float lift) {
  const auto tones = rendering::palette();
  const auto &view = frame.view;
  const float y = PANEL_Y + lift;
  rendering::panel(canvas, rect(COOK_X, y, COOK_WIDTH, PANEL_HEIGHT));
  const float left = COOK_X + INSET;
  const float right = COOK_X + COOK_WIDTH - INSET;
  const float top = y + INSET;
  panel_title(canvas, drawing, left, top - 6.0F, "COOK");
  paint_role_tool(canvas, drawing, view, Actor::Chef, left + 84.0F,
                  top - 10.0F);
  caption(canvas, drawing,
          right - drawing.text_width("TASTE", LABEL_SIZE, Weight::SemiBold),
          top + 18.0F, "TASTE", tones.faint);
  const auto &tuning = frame.content.get().companion.workshop;
  const auto offered = offered_dish(frame);
  std::vector<const survival::Recipe *> ready;
  std::vector<const survival::Recipe *> waiting;
  float row = top + 36.0F;
  for (const auto &recipe : tuning.recipes) {
    if (offered == recipe.food) {
      paint_offered_recipe(canvas, drawing, frame, recipe,
                           rect(left, row, right - left, OFFER_ROW));
      row += OFFER_ROW + 4.0F;
    } else {
      (recipe_ready(view, recipe) ? ready : waiting).push_back(&recipe);
    }
  }
  const float column = (right - left - GAP) / 2.0F;
  for (std::size_t index = 0; index < ready.size(); ++index) {
    const std::size_t rows_above = index / 2;
    paint_ready_recipe(
        canvas, drawing, frame, *ready.at(index),
        rect(left + static_cast<float>(index % 2) * (column + GAP),
             row + static_cast<float>(rows_above) * READY_ROW, column,
             READY_ROW - 4.0F));
  }
  const std::size_t ready_rows = (ready.size() + 1) / 2;
  row += static_cast<float>(ready_rows) * READY_ROW + COOK_ROW_HEIGHT;
  for (const auto *recipe : waiting) {
    paint_waiting_recipe(canvas, drawing, frame, *recipe, left, right, row);
    row += COOK_ROW_HEIGHT;
  }
  paint_button(canvas, drawing,
               rect(left, y + PANEL_HEIGHT - INSET - JOB_HEIGHT, right - left,
                    JOB_HEIGHT),
               cook_job(frame), BUTTON_SIZE,
               offered && survival::key_set(frame.control, Actor::Chef));
}

void paint_feed_panel(godot::Node2D &canvas, const rendering::Art &drawing,
                      const SurvivalFrame &frame, float lift) {
  const auto tones = rendering::palette();
  const auto &workshop = frame.view.workshop;
  const auto &tuning = frame.content.get().companion.workshop;
  const float y = PANEL_Y + lift;
  rendering::panel(canvas, rect(FEED_X, y, FEED_WIDTH, PANEL_HEIGHT));
  const float left = FEED_X + INSET;
  const float width = FEED_WIDTH - 2.0F * INSET;
  const float top = y + INSET;
  panel_title(canvas, drawing, left, top - 6.0F, "FEED");
  const String needs = "Needs a score of " + number(tuning.satisfying_score);
  drawing.text(canvas,
               left + width - drawing.text_width(needs, 14, Weight::SemiBold),
               top + 20.0F, needs, 14, tones.gold, Weight::SemiBold);
  float line = top + 44.0F;
  for (const auto *rule : FEED_RULES) {
    drawing.text(canvas, left, line, rule, 13, tones.faint);
    line += 16.0F;
  }
  float row = line - 4.0F;
  if (workshop.foods.empty()) {
    drawing.text(canvas, left, row + 30.0F, "No dish stored.", 16, tones.muted,
                 CHANGING);
    drawing.text(canvas, left, row + 52.0F, "The chef must cook.", 16,
                 tones.muted, CHANGING);
  }
  for (const auto &dish : workshop.foods) {
    paint_item(canvas, drawing, survival::food_name(dish.food),
               Vector2(left - 4.0F, row - 2.0F));
    drawing.text(canvas, left + 50.0F, row + 18.0F, dish_label(dish),
                 FEED_NAME_SIZE, tones.cyan, CHANGING);
    const auto score = survival::dish_score(dish, workshop, tuning);
    const auto total = score ? score->total : 0U;
    // The score counts up to its value when the dish first shows.
    const auto shown = static_cast<std::uint64_t>(std::lround(drawing.glide(
        "score" + number(dish.food.value) +
            (dish.quality == survival::DishQuality::Fine ? "f" : "p"),
        static_cast<float>(total))));
    drawing.text(
        canvas, left + 50.0F, row + 39.0F,
        "Score " + number(shown) +
            (total >= tuning.satisfying_score ? " passes" : " falls short"),
        FEED_NAME_SIZE, tones.gold, CHANGING);
    row += FEED_ROW;
  }
  const auto jobs = feed_jobs(frame);
  const float foot = y + PANEL_HEIGHT - INSET - 40.0F;
  paint_button(canvas, drawing,
               rect(left, foot - SERVE_HEIGHT - GAP, width, SERVE_HEIGHT),
               jobs.at(0), SERVE_SIZE, dish_to_feed(frame));
  paint_button(canvas, drawing, rect(left, foot, width, 40.0F), jobs.at(1),
               BUTTON_SIZE, false);
}

/// Draws over the kitchen and reads only snapshots, so no clock or key waits.
void paint_day_result(godot::Node2D &canvas, const rendering::Art &drawing,
                      const SurvivalFrame &frame) {
  const auto lines = day_result(frame);
  if (lines.empty()) {
    return;
  }
  const auto tones = rendering::palette();
  rendering::panel(
      canvas,
      rect(RESULT_X, RESULT_Y, RESULT_WIDTH,
           RESULT_LINE_HEIGHT * static_cast<float>(lines.size()) + 22.0F));
  float y = RESULT_Y + 14.0F + static_cast<float>(RESULT_SIZE);
  bool headline = true;
  for (const auto &line : lines) {
    drawing.text(canvas, RESULT_X + INSET, y, line, RESULT_SIZE,
                 headline ? tones.gold : tones.white, CHANGING);
    headline = false;
    y += RESULT_LINE_HEIGHT;
  }
}

void paint_workshop_screen(godot::Node2D &canvas, const rendering::Art &drawing,
                           const SurvivalFrame &frame) {
  rendering::paint_workshop(canvas, drawing,
                            !frame.view.workshop.foods.empty());
  paint_day_result(canvas, drawing, frame);
  const float lift = (1.0F - drawing.entrance("workshop")) * ENTRANCE_LIFT;
  paint_appraise_panel(canvas, drawing, frame, lift);
  paint_cook_panel(canvas, drawing, frame, lift);
  paint_feed_panel(canvas, drawing, frame, lift);
}

void paint_city_screen(godot::Node2D &canvas, const rendering::Art &drawing,
                       const SurvivalFrame &frame) {
  rendering::paint_city(canvas, drawing, frame.content.get(), frame.view,
                        frame.control, out_of_reach(frame));
  paint_actor_card(canvas, drawing, frame, Actor::Chef, MARGIN);
  paint_actor_card(canvas, drawing, frame, Actor::Appraiser,
                   MARGIN + CARD_WIDTH + GAP);
  paint_pack(canvas, drawing, frame);
  paint_actions(canvas, drawing, frame);
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
  rows.push_back(row("Jump", one.jump, two.jump));
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
                   color(9, 10, 20, 0.55F));
  drawing.text(canvas, SETUP_X, 120.0F, "Who is playing?", 40, tones.white,
               Weight::Bold);
  const auto chosen_at = static_cast<float>(
      std::ranges::find(SETUP_ENTRIES, selected) - SETUP_ENTRIES.begin());
  const float cursor =
      SETUP_Y + drawing.glide("setup_cursor", chosen_at) * SETUP_PITCH;
  for (std::size_t index = 0; index < SETUP_ENTRIES.size(); ++index) {
    rendering::panel(
        canvas, rect(SETUP_X, SETUP_Y + static_cast<float>(index) * SETUP_PITCH,
                     SETUP_WIDTH, SETUP_HEIGHT));
  }
  // The highlight slides to the cursor's entry.
  rendering::button(canvas, rect(SETUP_X, cursor, SETUP_WIDTH, SETUP_HEIGHT),
                    true);
  float y = SETUP_Y;
  std::size_t number_key = 1;
  for (const SetupEntry entry : SETUP_ENTRIES) {
    const bool chosen = entry == selected;
    const auto label = setup_label(entry);
    if (entry == SetupEntry::TwoPlayers) {
      drawing.image(canvas, rendering::Image::ChefIdle,
                    Vector2(SETUP_X + 16.0F, y + 26.0F));
      drawing.image(canvas, rendering::Image::AppraiserIdle,
                    Vector2(SETUP_X + 72.0F, y + 26.0F));
    } else {
      drawing.image(canvas,
                    entry == SetupEntry::Chef ? rendering::Image::ChefIdle
                                              : rendering::Image::AppraiserIdle,
                    Vector2(SETUP_X + 44.0F, y + 26.0F));
    }
    const auto ink = chosen ? tones.ink : tones.white;
    drawing.text(canvas, SETUP_X + 160.0F, y + 52.0F,
                 number(number_key) + "   " + label.at(0), 28, ink,
                 Weight::Bold);
    drawing.text(canvas, SETUP_X + 160.0F, y + 86.0F, label.at(1), 19,
                 chosen ? tones.ink : tones.muted);
    y += SETUP_PITCH;
    ++number_key;
  }
  rendering::panel(canvas, rect(MARGIN, 600.0F, SCREEN_WIDTH - 2.0F * MARGIN,
                                SCREEN_HEIGHT - MARGIN - 600.0F));
  drawing.text(canvas, 30.0F, 642.0F,
               "Gather by day, appraise and cook at base, and feed the monster "
               "before the acid wrecks the city.",
               20, tones.white);
  drawing.text(canvas, 30.0F, 680.0F,
               "W/S or up/down: choose     Enter: select     1, 2, 3: jump "
               "straight in     M: main menu",
               17, tones.muted);
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
  paint_events(canvas, drawing, frame);
  paint_next_step(canvas, drawing, frame);
}

void paint_interruption(godot::Node2D &canvas, const rendering::Art &drawing,
                        Interruption interruption, const SurvivalFrame &frame) {
  const auto tones = rendering::palette();
  const auto text = interruption_text(interruption);
  const float shown = drawing.entrance("interruption");
  const float lift = (1.0F - shown) * ENTRANCE_LIFT;
  canvas.draw_rect(rect(0.0F, 0.0F, SCREEN_WIDTH, SCREEN_HEIGHT),
                   color(9, 10, 20, 0.72F * shown));
  if (interruption == Interruption::Paused) {
    rendering::panel(canvas, rect(PAUSE_BOX_X, PAUSE_BOX_Y + lift,
                                  PAUSE_BOX_WIDTH, PAUSE_BOX_HEIGHT));
    drawing.text(canvas, PAUSE_TEXT_X, PAUSE_BOX_Y + lift + 70.0F, text.at(0),
                 INTERRUPTION_TITLE_SIZE, tones.white, Weight::Bold);
    drawing.text(canvas, PAUSE_TEXT_X, PAUSE_BOX_Y + lift + 104.0F, text.at(1),
                 INTERRUPTION_LINE_SIZE, tones.muted);
    paint_key_rows(canvas, drawing, key_rows(frame), PAUSE_ROWS_Y + lift,
                   PAUSE_ROW_HEIGHT);
    paint_button(canvas, drawing,
                 rect(PAUSE_TEXT_X - BUTTON_PAD,
                      PAUSE_BOX_Y + lift + PAUSE_BOX_HEIGHT - 70.0F,
                      button_width(drawing, text.at(2), INTERRUPTION_LINE_SIZE),
                      48.0F),
                 text.at(2), INTERRUPTION_LINE_SIZE, false);
    return;
  }
  rendering::panel(canvas, rect(NOTICE_BOX_X, NOTICE_BOX_Y + lift,
                                NOTICE_BOX_WIDTH, NOTICE_BOX_HEIGHT));
  drawing.text(canvas, NOTICE_TEXT_X, NOTICE_BOX_Y + lift + 70.0F, text.at(0),
               INTERRUPTION_TITLE_SIZE, tones.white, Weight::Bold);
  drawing.text(canvas, NOTICE_TEXT_X, NOTICE_BOX_Y + lift + 116.0F, text.at(1),
               INTERRUPTION_LINE_SIZE, tones.muted, CHANGING);
  if (!text.at(2).is_empty()) {
    paint_button(canvas, drawing,
                 rect(NOTICE_TEXT_X - BUTTON_PAD, NOTICE_BOX_Y + lift + 150.0F,
                      button_width(drawing, text.at(2), INTERRUPTION_LINE_SIZE),
                      48.0F),
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
    drawing.text(canvas, 130.0F, 170.0F, "SWALLOWED", 46, tones.white,
                 Weight::Bold);
    drawing.text(canvas, 130.0F, 230.0F,
                 "A monster ate the city. Keep it fed, and look for a way out.",
                 24, tones.white);
    break;
  case Cinematic::Ending:
    drawing.text(canvas, 130.0F, 170.0F, "ESCAPED", 46, tones.gold,
                 Weight::Bold);
    drawing.text(canvas, 130.0F, 230.0F,
                 "The party got out of the monster after " +
                     number(progression.day - 1) + String(" days."),
                 24, tones.white);
    break;
  case Cinematic::Loss:
    drawing.text(canvas, 130.0F, 170.0F, "DISSOLVED", 46, tones.success,
                 Weight::Bold);
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
  bool stripe = false;
  for (const auto &row : rows) {
    // The heading row names the players, not keys.
    if (row.action.is_empty()) {
      caption(canvas, drawing, PAUSE_FIRST_X, y, row.first, tones.gold);
      caption(canvas, drawing, PAUSE_SECOND_X, y, row.second, tones.gold);
      y += row_height * 0.75F;
      continue;
    }
    if (stripe) {
      rendering::rounded(
          canvas,
          rect(PAUSE_TEXT_X - 14.0F, y - row_height + 12.0F,
               PAUSE_BOX_X + PAUSE_BOX_WIDTH - PAUSE_TEXT_X - 26.0F,
               row_height),
          8.0F, tones.raised);
    }
    stripe = !stripe;
    drawing.text(canvas, PAUSE_TEXT_X, y, row.action, PAUSE_ROW_SIZE,
                 tones.white, CHANGING);
    drawing.text(canvas, PAUSE_FIRST_X, y, row.first, PAUSE_ROW_SIZE,
                 tones.cyan, CHANGING);
    drawing.text(canvas, PAUSE_SECOND_X, y, row.second, PAUSE_ROW_SIZE,
                 tones.cyan, CHANGING);
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
         from(keys.jump) + " jump  " + from(keys.interact) + " use  " +
         from(keys.travel) + " travel  " + from(keys.replace_tool) + " tool  " +
         from(keys.feed) + " feed  " + from(keys.end_day) + " end day  " +
         from(keys.switch_actor) + " swap";
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
  // The key appraises the offered ingredient, so the button names it.
  const auto &offer = frame.offers.at(index_of(Actor::Appraiser)).interact;
  return offer ? offered_step(*offer, KEY_LABELS.at(*keys))
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
  const float cook_inner = COOK_WIDTH - 2.0F * INSET;
  // Words on a button stay inside its padding.
  const float padding = 2.0F * BUTTON_PAD;
  std::vector<FittedText> lines;
  lines.push_back(fitted(escape_goal(), GOAL_SIZE, GOAL_WIDTH));
  for (const auto *hint : MENU_HINTS) {
    lines.push_back(
        fitted(hint, LABEL_SIZE, HINT_WIDTH - 2.0F * 12.0F - 44.0F));
  }
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
  lines.push_back(fitted(next_step(frame), NEXT_SIZE, NEXT_WIDTH));
  lines.push_back(fitted(hand_off_notice(frame), HAND_OFF_SIZE, NEXT_WIDTH));
  for (const auto &event : frame.events) {
    lines.push_back(
        fitted(describe(event), EVENT_SIZE, LOG_WIDTH - 2.0F * CARD_PAD));
  }
  lines.push_back(fitted("Each collection spends " + collection_clock(frame),
                         13, PACK_WIDTH - 2.0F * INSET));
  // The city's action card: a lone human's row, or each of two humans' rows.
  const auto players = humans(frame);
  const float action_inner = ACTION_WIDTH - 2.0F * CARD_PAD;
  for (const auto &[actor, set] : players) {
    const auto &keys = KEY_LABELS.at(set);
    const String label = interact_label(frame, actor, keys);
    const String travel = travel_label(frame, actor, keys);
    if (players.size() == 1) {
      lines.push_back(fitted(label, ACTION_SIZE, SOLO_INTERACT - padding));
      lines.push_back(fitted(interact_target(frame, actor), 16,
                             action_inner - SOLO_INTERACT - 12.0F));
      lines.push_back(fitted(travel, BUTTON_SIZE, SOLO_TRAVEL - padding));
      lines.push_back(
          fitted(move_hint(keys), 14, action_inner - SOLO_TRAVEL - 10.0F));
    } else {
      lines.push_back(fitted(label, BUTTON_SIZE, DUO_INTERACT - padding));
      lines.push_back(fitted(travel, BUTTON_SIZE, DUO_TRAVEL - padding));
    }
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
    lines.push_back(fitted(text.at(1), INTERRUPTION_LINE_SIZE, notice_width));
    lines.push_back(fitted(text.at(2), INTERRUPTION_LINE_SIZE,
                           notice_width - padding + BUTTON_PAD));
  }
  lines.push_back(
      fitted(appraise_job(frame), BUTTON_SIZE, appraise_inner - padding));
  lines.push_back(fitted(cook_job(frame), BUTTON_SIZE, cook_inner - padding));
  const auto jobs = feed_jobs(frame);
  lines.push_back(fitted(jobs.at(0), SERVE_SIZE, feed_inner - padding));
  lines.push_back(fitted(jobs.at(1), BUTTON_SIZE, feed_inner - padding));
  lines.push_back(fitted(NO_SPARE_TOOLS, CARD_TOOLS_SIZE,
                         CARD_WIDTH - CARD_PAD - CARD_COLUMN - CARD_SPARES));
  for (const auto state :
       {survival::ToolState::Intact, survival::ToolState::Damaged}) {
    lines.push_back(fitted(tool_condition(state), CARD_TOOLS_SIZE,
                           CARD_SPARES - CARD_TOOL_TEXT - 4.0F));
  }
  const auto &tuning = frame.content.get().companion.workshop;
  for (const auto &recipe : tuning.recipes) {
    lines.push_back(fitted(cook_forecast(frame, recipe.food), FORECAST_SIZE,
                           cook_inner - OFFER_TEXT - 12.0F));
    const String name = from(survival::food_name(recipe.food));
    lines.push_back(fitted(name, COOK_NAME_SIZE, COOK_STATUS_X - 4.0F));
    lines.push_back(fitted(name, READY_SIZE, READY_EATEN_X - 12.0F - 4.0F));
    lines.push_back(fitted(dish_label({.food = recipe.food,
                                       .quality = survival::DishQuality::Fine}),
                           FEED_NAME_SIZE, feed_inner - 50.0F));
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
