#include "survival.hpp"

#include "../rendering/survival.hpp"

#include <survival/escape.hpp>

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <ranges>
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
using rendering::SCREEN_HEIGHT;
using rendering::SCREEN_WIDTH;
constexpr float INSET = rendering::PANEL_INSET;
/// Every panel keeps this far from the screen's edge and from its neighbour.
constexpr float MARGIN = 8.0F;
constexpr float GAP = 6.0F;
/// Words that change with play draw in this weight, the one the harness
/// measures them in.
constexpr Weight CHANGING = Weight::Regular;
constexpr float BUTTON_PAD = 14.0F;
/// The smallest words on the screen: the face at two screen pixels to each
/// of its own.
constexpr int SMALL_SIZE = 2 * rendering::FONT_GRID;
/// The face's capitals stand five of its eight pixels per em.
constexpr float CAP_HEIGHT = 0.625F;
/// The top corners: the day at the left, the acid in the middle, the time at
/// the right, and the goal line under the acid.
constexpr float BAND_Y = MARGIN;
constexpr float BAND_HEIGHT = 48.0F;
constexpr float BAND_BASELINE = BAND_Y + 32.0F;
constexpr int BIG_SIZE = 3 * rendering::FONT_GRID;
constexpr float DAY_WIDTH = 184.0F;
constexpr float TIME_WIDTH = 148.0F;
constexpr float TIME_X = SCREEN_WIDTH - MARGIN - TIME_WIDTH;
constexpr float ACID_WIDTH = 360.0F;
constexpr float ACID_X = (SCREEN_WIDTH - ACID_WIDTH) / 2.0F;
constexpr int ACID_SIZE = BIG_SIZE;
/// Where the acid's gauge starts inside its panel, after its words.
constexpr float ACID_GAUGE_X = 132.0F;
constexpr float GOAL_Y = BAND_Y + BAND_HEIGHT + GAP;
constexpr int GOAL_SIZE = SMALL_SIZE;
constexpr float GOAL_PAD = 14.0F;
/// The goal line may run the screen's width, between the corners above and
/// the pause chip and notes below.
constexpr float GOAL_WIDTH = SCREEN_WIDTH - 2.0F * MARGIN;
constexpr int HAND_OFF_SIZE = SMALL_SIZE;
constexpr float HINT_Y = GOAL_Y + 60.0F;
/// The log's notes, right-aligned under the pause chip; each shows this long,
/// then fades.
constexpr float NOTE_Y = HINT_Y + 12.0F;
constexpr float NOTE_WIDTH = 340.0F;
constexpr float NOTE_HEIGHT = 24.0F;
constexpr float NOTE_SECONDS = 4.0F;
constexpr float NOTE_FADE_SECONDS = 1.0F;
/// The bottom corners: the two actors at the left, the bag in the middle,
/// and the action at the right in the city or the dishes at base.
constexpr float FOOT_HEIGHT = 84.0F;
constexpr float FOOT_Y = SCREEN_HEIGHT - MARGIN - FOOT_HEIGHT;
constexpr float FRAME = 7.0F;
constexpr float ACTOR_WIDTH = 208.0F;
constexpr float PORTRAIT = 72.0F;
/// The head of each original cut, in scene pixels, shown at its native size.
constexpr float CHEF_FACE_X = 12.0F;
constexpr float CHEF_FACE_Y = 5.0F;
constexpr float APPRAISER_FACE_X = 24.0F;
constexpr float APPRAISER_FACE_Y = 27.0F;
constexpr float FACE = 32.0F;
constexpr float ACTOR_COLUMN = MARGIN + PORTRAIT + 12.0F;
constexpr float ACTOR_TEXT_WIDTH = ACTOR_WIDTH - ACTOR_COLUMN - FRAME;
constexpr float BAG_X = MARGIN + 2.0F * (ACTOR_WIDTH + GAP);
constexpr std::size_t BAG_SLOTS = 7;
constexpr float SLOT = 64.0F;
constexpr float SLOT_PITCH = 66.0F;
constexpr float BAG_WIDTH =
    static_cast<float>(BAG_SLOTS) * SLOT_PITCH - 2.0F + 2.0F * 10.0F;
constexpr float ICON = 56.0F;
constexpr float ACTION_X = BAG_X + BAG_WIDTH + GAP;
constexpr float ACTION_WIDTH = SCREEN_WIDTH - MARGIN - ACTION_X;
constexpr float ACTION_INNER = ACTION_WIDTH - 2.0F * 10.0F;
constexpr int ACTION_SIZE = SMALL_SIZE;
constexpr float SOLO_BUTTON_HEIGHT = 36.0F;
/// Two humans' rows, each a player tag and a button over a line of keys, in
/// a panel taller than the other corners'.
constexpr float PLAYER_TAG = 30.0F;
constexpr float DUO_BUTTON = ACTION_INNER - PLAYER_TAG;
constexpr float DUO_BUTTON_HEIGHT = 36.0F;
constexpr float DUO_ROW = 56.0F;
constexpr float DUO_HEIGHT = 2.0F * DUO_ROW + 24.0F;
constexpr int DUO_SIZE = SMALL_SIZE;
/// The base's task card over the bottom corners.
constexpr float TASK_WIDTH = 820.0F;
constexpr float TASK_X = (SCREEN_WIDTH - TASK_WIDTH) / 2.0F;
constexpr float TASK_HEIGHT = 104.0F;
constexpr float TASK_Y = FOOT_Y - GAP - TASK_HEIGHT;
constexpr int TASK_SIZE = BIG_SIZE;
constexpr int DETAIL_SIZE = SMALL_SIZE;
constexpr int TASK_NOTE_SIZE = SMALL_SIZE;
constexpr float TASK_INNER = TASK_WIDTH - 2.0F * INSET;
/// The baseline of the card's last line, the dishes and the end of the day,
/// under its top.
constexpr float TASK_NOTES = 89.0F;
/// The unidentified ingredients float over the appraiser's head in the
/// kitchen, copied from where `paint_workshop` in src/rendering/survival.cpp
/// draws the appraiser's idle frame: x 150, standing on the kitchen floor.
constexpr float APPRAISER_HEAD_X = 216.0F;
constexpr float APPRAISER_HEAD_Y = 206.0F;
/// Where the workshop scene stands the chef, copied from where
/// `paint_workshop` in src/rendering/survival.cpp draws the chef's idle frame:
/// its left edge, `KITCHEN_FLOOR` under its feet, and `WORKSHOP_SCALE`.
constexpr float CHEF_FIGURE_X = 560.0F;
constexpr float KITCHEN_FLOOR = 310.0F;
constexpr float FIGURE_SCALE = 2.0F;
/// The day's result, over the task card and clear of the appraiser's
/// ingredients, the kitchen's people and the notes.
constexpr float RESULT_WIDTH = TASK_WIDTH;
constexpr float RESULT_X = TASK_X;
constexpr float RESULT_BOTTOM = TASK_Y - GAP;
constexpr float RESULT_LINE_HEIGHT = 22.0F;
constexpr int RESULT_SIZE = SMALL_SIZE;
/// The pause screen: the keys at the left over the volumes, the recipe book
/// at the right.
constexpr float PAUSE_BOX_X = 40.0F;
constexpr float PAUSE_BOX_Y = 24.0F;
constexpr float PAUSE_BOX_WIDTH = SCREEN_WIDTH - 2.0F * PAUSE_BOX_X;
constexpr float PAUSE_BOX_HEIGHT = 672.0F;
constexpr float PAUSE_TEXT_X = 72.0F;
constexpr float PAUSE_ROWS_Y = 172.0F;
constexpr float PAUSE_ROW_HEIGHT = 28.0F;
constexpr float PAUSE_INSET = 32.0F;
/// The key rows' columns, from their left edge.
constexpr float KEY_FIRST = 330.0F;
constexpr float KEY_SECOND = 470.0F;
constexpr float KEY_ROWS_WIDTH = 600.0F;
constexpr int KEY_ROW_SIZE = SMALL_SIZE;
constexpr float BOOK_X = 680.0F;
constexpr float BOOK_RIGHT = PAUSE_BOX_X + PAUSE_BOX_WIDTH - PAUSE_INSET;
constexpr float BOOK_NEEDS = 150.0F;
constexpr float BOOK_MARK = 400.0F;
constexpr float BOOK_ROW = 24.0F;
constexpr int BOOK_SIZE = SMALL_SIZE;
constexpr std::size_t EATEN_PER_LINE = 6;
/// What feeding does, under the recipe book.
constexpr std::array FEED_RULES{
    "A dish reaching the needed score holds the acid back.",
    "A hated dish, a lower score or no meal raises it.",
    "Full acid ends the run."};
/// The volume rows' part at the bottom of the pause screen.
constexpr float VOLUME_AREA = 172.0F;
constexpr float VOLUME_ROW_HEIGHT = 34.0F;
constexpr float VOLUME_BLOCK_STEP = 18.0F;
constexpr float VOLUME_BLOCKS_X = 240.0F;
constexpr const char *VOLUME_KEYS =
    "SOUND AND DISPLAY   W/S or up/down: choose   A/D or left/right: change";
constexpr int PAUSE_ROW_SIZE = SMALL_SIZE;
constexpr float NOTICE_BOX_X = 290.0F;
constexpr float NOTICE_BOX_Y = 236.0F;
constexpr float NOTICE_BOX_WIDTH = 700.0F;
constexpr float NOTICE_BOX_HEIGHT = 240.0F;
constexpr float NOTICE_TEXT_X = 326.0F;
constexpr int INTERRUPTION_TITLE_SIZE = 4 * rendering::FONT_GRID;
constexpr int INTERRUPTION_LINE_SIZE = BIG_SIZE;
/// A panel's entrance lifts it this far into place.
constexpr float ENTRANCE_LIFT = 18.0F;
/// The pause chip under the time, where a first player looks for keys.
constexpr const char *PAUSE_HINT = "Pause, keys";
constexpr float CHIP_PAD = 8.0F;
constexpr const char *NO_DISH = "None stored yet";
/// A first-play tip: one line on a panel, and its gold caption.
constexpr float TIP_HEIGHT = 36.0F;
constexpr float TIP_PAD = 14.0F;
constexpr float TIP_BASELINE = 24.0F;
constexpr const char *TIP_CAPTION = "TIP";
/// The placeholder card's button, split into its two targets at "Main menu".
constexpr const char *CARD_BUTTON = "Skip     Main menu";
constexpr const char *CARD_LEAVE = "Main menu";
constexpr float CARD_BUTTON_X = 130.0F;
constexpr float CARD_BUTTON_Y = 610.0F;
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
                                          .feed = "[",
                                          .end_day = "]",
                                          .switch_actor = "B",
                                          .leave = "N",
                                          .jump = ";"}};

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

/// The collection holding this actor, if one is under way.
const std::optional<survival::Collecting> &collecting_of(const GameView &view,
                                                         Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return view.exploration.chef.collecting;
  case Actor::Appraiser:
    return view.exploration.appraiser.collecting;
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

/// A number with at least two digits, as the screen shows it.
String two_digits(std::uint64_t value) {
  return (value < 10 ? String("0") : String()) + number(value);
}

/// The time left as minutes and seconds of two digits each, as every
/// screen shows it: 04:32.
String corner_clock(std::chrono::microseconds remaining) {
  const auto seconds = std::chrono::ceil<std::chrono::seconds>(remaining);
  const auto whole = static_cast<std::uint64_t>(seconds.count());
  return two_digits(whole / 60) + ":" + two_digits(whole % 60);
}

/// A short span in seconds to a tenth, rounded up: 2.5s.
String seconds(std::chrono::microseconds span) {
  using Tenths = std::chrono::duration<std::int64_t, std::deci>;
  const auto tenths =
      static_cast<std::uint64_t>(std::chrono::ceil<Tenths>(span).count());
  return number(tenths / 10) + "." + number(tenths % 10) + "s";
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

/// Draws the 56 px icon of `name` with its top-left corner at `at`; a name the
/// art set has no icon for is written there instead.
void paint_item(godot::Node2D &canvas, const rendering::Art &drawing,
                std::string_view name, Vector2 at) {
  if (const auto icon = rendering::item_icon(name)) {
    drawing.image(canvas, *icon, at);
    return;
  }
  drawing.text(canvas, at.x, at.y + 28.0F, from(name), SMALL_SIZE,
               rendering::palette().white, CHANGING);
}

/// Words on a small dark chip, for counts and marks drawn over an icon.
void chip(godot::Node2D &canvas, const rendering::Art &drawing, float x,
          float baseline, const String &text, int size, godot::Color tint) {
  const auto height = static_cast<float>(size);
  canvas.draw_rect(
      rect(std::round(x - 3.0F), std::round(baseline - height + 1.0F),
           std::round(drawing.text_width(text, size, CHANGING)) + 6.0F,
           height + 3.0F),
      rendering::palette().outline);
  drawing.text(canvas, x, baseline, text, size, tint, CHANGING);
}

/// Adds `area` to what the HUD covers in this frame, the `hud_panels` meta.
void cover(godot::Node2D &canvas, godot::Rect2 area) {
  godot::Array panels = canvas.get_meta("hud_panels", godot::Array());
  panels.push_back(area);
  canvas.set_meta("hud_panels", panels);
}

/// Draws a panel of the HUD and counts it as covered.
void hud_panel(godot::Node2D &canvas, godot::Rect2 area) {
  rendering::panel(canvas, area);
  cover(canvas, area);
}

/// A dark button with its words centred on its height, gold when marked.
void paint_button(godot::Node2D &canvas, const rendering::Art &drawing,
                  godot::Rect2 area, const String &text, int size,
                  bool marked) {
  const auto tones = rendering::palette();
  rendering::button(canvas, area, marked);
  drawing.text(canvas, area.position.x + BUTTON_PAD,
               area.position.y +
                   (area.size.y + static_cast<float>(size) * CAP_HEIGHT) / 2.0F,
               text, size, marked ? tones.gold : tones.white, CHANGING);
}

/// How wide a button must be to hold `text` at `size`.
float button_width(const rendering::Art &drawing, const String &text,
                   int size) {
  return drawing.text_width(text, size, CHANGING) + 2.0F * BUTTON_PAD;
}

/// How far below its place the panel `name` still is as it comes in; its
/// painting and its targets both stand there.
float entrance_lift(const rendering::Art &drawing, const char *name) {
  return (1.0F - drawing.entrance(name)) * ENTRANCE_LIFT;
}

/// What `action` does, named by the action and not by its key.
String offered_step(const GameView &view, const survival::Action &action) {
  return std::visit(
      [&view](const auto &offer) -> String {
        using Offer = std::decay_t<decltype(offer)>;
        if constexpr (std::is_same_v<Offer, survival::Collect>) {
          return "Collect from " + rendering::place_name(view, offer.source);
        } else if constexpr (std::is_same_v<Offer, survival::Appraise>) {
          return "Appraise " +
                 from(survival::ingredient_name(offer.ingredient));
        } else if constexpr (std::is_same_v<Offer, survival::Cook>) {
          return "Cook " + from(survival::food_name(offer.food));
        } else if constexpr (std::is_same_v<Offer, survival::Feed>) {
          return "Feed " + from(survival::food_name(offer.food)) +
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
         survival::worth_entering(frame.content.get().companion, frame.view);
}

/// Whether setting out today gives any human something to do.
bool worth_setting_out(const SurvivalFrame &frame) {
  return std::ranges::any_of(ACTORS, [&frame](Actor actor) {
    return worth_setting_out(frame, actor);
  });
}

String collection_clock(const SurvivalFrame &frame) {
  return seconds(frame.content.get().companion.exploration.collection_time);
}

String city_step(const SurvivalFrame &frame, Actor actor,
                 const KeyLabels &keys) {
  const auto &offers = frame.offers.at(index_of(actor));
  const bool collecting =
      offers.interact &&
      std::holds_alternative<survival::Collect>(*offers.interact);
  const bool walking = offers.errand && offers.errand->step.has_value();
  if (const auto &held = collecting_of(frame.view, actor)) {
    return "Collecting from " +
           rendering::place_name(frame.view, held->source) + ": " +
           seconds(held->remaining) + " left.";
  }
  // A source passed on the way to what the party lacks would hold the actor.
  if (offers.interact && offers.errand && !walking) {
    return offered_step(frame.view, *offers.interact) +
           (collecting ? " (takes " + collection_clock(frame) + ")" : String());
  }
  if (!offers.errand) {
    return "Nothing is within reach for now: return to base.";
  }
  if (!offers.errand->step) {
    return "Walk over to " +
           rendering::place_name(frame.view, offers.errand->target) + ".";
  }
  return move_words(*offers.errand->step, keys) + " to " +
         rendering::place_name(frame.view, offers.errand->target) + ".";
}

/// A step the base names, and the press a click on it stands for; none while
/// it waits on AI.
struct Job {
  String text;
  std::optional<PlayerPress> press;
};

std::optional<Job> role_step(const SurvivalFrame &frame, Actor actor,
                             const char *busy) {
  const auto keys = survival::key_set(frame.control, actor);
  if (!keys) {
    return Job{.text = busy, .press = std::nullopt};
  }
  const auto &offer = frame.offers.at(index_of(actor)).interact;
  if (offer) {
    return Job{.text = offered_step(frame.view, *offer),
               .press =
                   PlayerPress{.actor = actor, .key = PlayerKey::Interact}};
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

std::optional<Job> workshop_craft_step(const SurvivalFrame &frame) {
  if (has_unidentified(frame.view)) {
    if (auto step = role_step(frame, Actor::Appraiser,
                              "The appraiser is identifying your finds.")) {
      return step;
    }
  }
  const auto chef_keys = survival::key_set(frame.control, Actor::Chef);
  if (chef_keys && !dish_to_feed(frame)) {
    const auto &offer = frame.offers.at(index_of(Actor::Chef)).interact;
    if (offer) {
      return Job{.text = offered_step(frame.view, *offer),
                 .press = PlayerPress{.actor = Actor::Chef,
                                      .key = PlayerKey::Interact}};
    }
  }
  // An AI chef cooks on its next step, so the day must not end unfed first.
  if (frame.ai_chef_working && !dish_to_feed(frame)) {
    return Job{.text = "The AI chef is cooking.", .press = std::nullopt};
  }
  // Feeding ends the day, and a stored dish keeps while the city still gives.
  if (worth_setting_out(frame)) {
    return std::nullopt;
  }
  for (const auto actor : ACTORS) {
    const auto keys = survival::key_set(frame.control, actor);
    const auto &offer = frame.offers.at(index_of(actor)).feed;
    if (keys && offer) {
      return Job{.text = offered_step(frame.view, *offer),
                 .press = PlayerPress{.actor = actor, .key = PlayerKey::Feed}};
    }
  }
  return std::nullopt;
}

/// The human whose keys are player one's.
Actor first_player(const SurvivalFrame &frame) {
  return survival::key_set(frame.control, Actor::Chef) == 0 ? Actor::Chef
                                                            : Actor::Appraiser;
}

Job workshop_step(const SurvivalFrame &frame) {
  if (auto step = workshop_craft_step(frame)) {
    return *step;
  }
  if (worth_setting_out(frame)) {
    return {.text = "Set out into the city: " +
                    corner_clock(frame.view.progression.remaining_exploration) +
                    " left today.",
            .press =
                PlayerPress{.actor = std::nullopt, .key = PlayerKey::Travel}};
  }
  return {.text = "End the day unfed: acid rises by " +
                  number(frame.content.get().companion.unfed_penalty.value) +
                  ".",
          .press = PlayerPress{.actor = first_player(frame),
                               .key = PlayerKey::EndDay}};
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

/// Ending the day, for the base's task card.
String end_day_job(const SurvivalFrame &frame) {
  return "End the day unfed: acid +" +
         number(frame.content.get().companion.unfed_penalty.value);
}

/// A small caption, as the pause screen's columns open.
void caption(godot::Node2D &canvas, const rendering::Art &drawing, float x,
             float baseline, const String &text, godot::Color tint) {
  drawing.text(canvas, x, baseline, text, SMALL_SIZE, tint);
}

/// A trough sunk into a panel with its eased fill, lit along its top.
void gauge(godot::Node2D &canvas, const rendering::Art &drawing,
           const String &name, godot::Rect2 area, float fill,
           godot::Color tint) {
  const float shown = drawing.glide(name, std::clamp(fill, 0.0F, 1.0F));
  rendering::inset(canvas, area);
  const auto inner = area.grow(-2.0F);
  const float width = std::round(inner.size.x * shown);
  if (width > 0.0F) {
    canvas.draw_rect(
        rect(inner.position.x, inner.position.y, width, inner.size.y), tint);
    canvas.draw_rect(rect(inner.position.x, inner.position.y, width, 2.0F),
                     tint.lightened(0.45F));
  }
}

/// The pause chip under the clock, which a click takes.
godot::Rect2 pause_chip(const rendering::Art &drawing) {
  const float width =
      drawing.text_width(PAUSE_HINT, SMALL_SIZE, CHANGING) + 2.0F * CHIP_PAD;
  return rect(SCREEN_WIDTH - MARGIN - width, HINT_Y - 15.0F, width, 21.0F);
}

/// The day at the top left, the acid in the middle and the time at the
/// right, with the pause key under the time.
void paint_corners(godot::Node2D &canvas, const rendering::Art &drawing,
                   const SurvivalFrame &frame) {
  const auto tones = rendering::palette();
  const auto &progression = frame.view.progression;
  hud_panel(canvas, rect(MARGIN, BAND_Y, DAY_WIDTH, BAND_HEIGHT));
  drawing.image(canvas, rendering::Image::IconDay,
                Vector2(MARGIN + 10.0F, BAND_Y + 8.0F));
  const String day = "DAY ";
  drawing.text(canvas, MARGIN + 52.0F, BAND_BASELINE, day, BIG_SIZE,
               tones.white, Weight::Bold);
  drawing.text(canvas,
               MARGIN + 52.0F + drawing.text_width(day, BIG_SIZE, Weight::Bold),
               BAND_BASELINE, two_digits(progression.day), BIG_SIZE, tones.gold,
               Weight::Bold);
  hud_panel(canvas, rect(ACID_X, BAND_Y, ACID_WIDTH, BAND_HEIGHT));
  const String acid = "ACID ";
  drawing.text(canvas, ACID_X + 16.0F, BAND_BASELINE - 2.0F, acid, ACID_SIZE,
               tones.success, Weight::Bold);
  drawing.text(canvas,
               ACID_X + 16.0F +
                   drawing.text_width(acid, ACID_SIZE, Weight::Bold),
               BAND_BASELINE - 2.0F,
               number(static_cast<std::uint64_t>(progression.acid.value) *
                      100U / frame.content.get().run.full_acid.value) +
                   "%",
               ACID_SIZE, tones.white, Weight::Bold);
  gauge(canvas, drawing, "acid",
        rect(ACID_X + ACID_GAUGE_X, BAND_Y + 15.0F,
             ACID_WIDTH - ACID_GAUGE_X - 16.0F, 18.0F),
        acid_ratio(frame), tones.success);
  hud_panel(canvas, rect(TIME_X, BAND_Y, TIME_WIDTH, BAND_HEIGHT));
  drawing.image(canvas, rendering::Image::IconClock,
                Vector2(TIME_X + 10.0F, BAND_Y + 8.0F));
  drawing.text(canvas, TIME_X + 52.0F, BAND_BASELINE,
               corner_clock(progression.remaining_exploration), BIG_SIZE,
               tones.white, Weight::Bold);
  const auto chip_area = pause_chip(drawing);
  canvas.draw_rect(chip_area, tones.outline);
  cover(canvas, chip_area);
  drawing.text(canvas, chip_area.position.x + CHIP_PAD, HINT_Y, PAUSE_HINT,
               SMALL_SIZE, tones.muted, CHANGING);
}

/// The goal line under the acid, sized to its words, with a hand-off notice
/// under it; nothing when both are empty.
void paint_goal(godot::Node2D &canvas, const rendering::Art &drawing,
                const String &line, const String &notice) {
  if (line.is_empty() && notice.is_empty()) {
    return;
  }
  const auto tones = rendering::palette();
  const float words =
      std::max(drawing.text_width(line, GOAL_SIZE, CHANGING),
               drawing.text_width(notice, HAND_OFF_SIZE, CHANGING));
  const float width = std::min(GOAL_WIDTH, words + 2.0F * GOAL_PAD);
  const bool both = !line.is_empty() && !notice.is_empty();
  const float x = std::round((SCREEN_WIDTH - width) / 2.0F);
  hud_panel(canvas, rect(x, GOAL_Y, width, both ? 52.0F : 36.0F));
  if (!line.is_empty()) {
    drawing.text(canvas, x + GOAL_PAD, GOAL_Y + 24.0F, line, GOAL_SIZE,
                 tones.white, CHANGING);
  }
  if (!notice.is_empty()) {
    drawing.text(canvas, x + GOAL_PAD, GOAL_Y + (both ? 41.0F : 23.0F), notice,
                 HAND_OFF_SIZE, tones.gold, CHANGING);
  }
}

/// The log as notes at the right edge, newest first, each fading a few
/// seconds after it appears; returns where it drew them.
std::vector<godot::Rect2> paint_notes(godot::Node2D &canvas,
                                      const rendering::Art &drawing,
                                      const SurvivalFrame &frame) {
  const auto tones = rendering::palette();
  // Each event is its own note by its number in the session's log, so a line
  // repeated after older ones drop out still appears afresh.
  std::vector<std::pair<String, String>> notes;
  notes.reserve(frame.events.size());
  std::uint64_t serial = frame.first_event;
  for (const auto &event : frame.events) {
    notes.emplace_back(describe(frame.view, event), "note" + number(serial++));
  }
  std::vector<godot::Rect2> drawn;
  float y = NOTE_Y;
  for (const auto &[text, name] : std::ranges::reverse_view(notes)) {
    const float age = drawing.shown_for(name);
    const float shown =
        std::clamp((NOTE_SECONDS + NOTE_FADE_SECONDS - age) / NOTE_FADE_SECONDS,
                   0.0F, 1.0F);
    if (shown <= 0.0F) {
      continue;
    }
    const float width = std::min(
        NOTE_WIDTH, drawing.text_width(text, SMALL_SIZE, CHANGING) + 20.0F);
    const float x = SCREEN_WIDTH - MARGIN - width;
    auto plate = tones.surface;
    plate.a = shown;
    auto edge = tones.cyan;
    edge.a = shown;
    auto ink = tones.white;
    ink.a = shown;
    canvas.draw_rect(rect(x, y, width, NOTE_HEIGHT), plate);
    canvas.draw_rect(rect(x, y, 3.0F, NOTE_HEIGHT), edge);
    drawing.text(canvas, x + 11.0F, y + 17.0F, text, SMALL_SIZE, ink, CHANGING);
    drawn.push_back(rect(x, y, width, NOTE_HEIGHT));
    y += NOTE_HEIGHT + 4.0F;
  }
  return drawn;
}

/// Every `Rect2` in the canvas meta `name`.
void add_areas(std::vector<godot::Rect2> &areas, const godot::Node2D &canvas,
               const char *name) {
  const godot::Array listed = canvas.get_meta(name, godot::Array());
  for (std::int64_t index = 0; index < listed.size(); ++index) {
    areas.push_back(listed.get(index));
  }
}

/// Where a tip `width` wide stands: the first place clear of `taken`, in rows
/// from just above the bottom cards up to the goal line, each tried at the
/// left, the middle and the right; none while every place is taken.
std::optional<godot::Rect2> tip_place(float width,
                                      const std::vector<godot::Rect2> &taken) {
  const std::array columns{MARGIN, std::round((SCREEN_WIDTH - width) / 2.0F),
                           SCREEN_WIDTH - MARGIN - width};
  const float bottom = FOOT_Y - GAP - TIP_HEIGHT;
  const auto rows =
      static_cast<int>((bottom - GOAL_Y) / (TIP_HEIGHT + GAP)) + 1;
  for (int row = 0; row < rows; ++row) {
    const float y = bottom - static_cast<float>(row) * (TIP_HEIGHT + GAP);
    for (const float x : columns) {
      const auto place = rect(x, y, width, TIP_HEIGHT);
      const auto apart = place.grow(GAP / 2.0F);
      if (std::ranges::none_of(taken, [&apart](const godot::Rect2 &area) {
            return area.intersects(apart);
          })) {
        return place;
      }
    }
  }
  return std::nullopt;
}

/// Draws `tip` where it covers no HUD panel, note or target, nor in the city
/// any of the map's words, and leaves it as the `hud_tip` meta.
void paint_tip(godot::Node2D &canvas, const rendering::Art &drawing,
               const SurvivalFrame &frame, Tip tip,
               const std::vector<godot::Rect2> &notes) {
  const bool city = frame.view.progression.phase == Phase::Exploration;
  std::vector<godot::Rect2> taken = notes;
  add_areas(taken, canvas, "hud_panels");
  // The map's words stay on the canvas after the city closes.
  if (city) {
    add_areas(taken, canvas, "map_plates");
  }
  for (const auto &target : survival_targets(drawing, frame)) {
    taken.push_back(target.area);
  }
  const auto tones = rendering::palette();
  const String text = tip_text(tip);
  const float caption_width =
      drawing.text_width(TIP_CAPTION, SMALL_SIZE, Weight::Bold) + TIP_PAD;
  const auto place =
      tip_place(caption_width + drawing.text_width(text, SMALL_SIZE, CHANGING) +
                    2.0F * TIP_PAD,
                taken);
  if (!place) {
    return;
  }
  rendering::panel(canvas, *place);
  const float x = place->position.x + TIP_PAD;
  const float baseline = place->position.y + TIP_BASELINE;
  drawing.text(canvas, x, baseline, TIP_CAPTION, SMALL_SIZE, tones.gold,
               Weight::Bold);
  drawing.text(canvas, x + caption_width, baseline, text, SMALL_SIZE,
               tones.white, CHANGING);
  godot::Dictionary drawn;
  drawn.set("rect", *place);
  drawn.set("text", text);
  drawn.set("city", city);
  canvas.set_meta("hud_tip", drawn);
}

/// The placeholder card's button, under its lines.
godot::Rect2 card_button(const rendering::Art &drawing) {
  return rect(CARD_BUTTON_X - BUTTON_PAD, CARD_BUTTON_Y,
              button_width(drawing, CARD_BUTTON, INTERRUPTION_LINE_SIZE),
              48.0F);
}

/// Who plays an actor: P1, P2 or AI.
String player_tag(const SurvivalFrame &frame, Actor actor) {
  const auto keys = survival::key_set(frame.control, actor);
  return keys ? "P" + number(*keys + 1) : String("AI");
}

/// Who plays an actor and where it is, as its block's top line shows.
String actor_status(const SurvivalFrame &frame, Actor actor) {
  return player_tag(frame, actor) + "  " +
         (state_of(frame.view, actor).location == Location::City ? "in city"
                                                                 : "at base");
}

/// One actor at the bottom left: its face in a frame, who plays it and where
/// it is.
void paint_actor(godot::Node2D &canvas, const rendering::Art &drawing,
                 const SurvivalFrame &frame, Actor actor, float x) {
  const auto tones = rendering::palette();
  const bool human = survival::key_set(frame.control, actor).has_value();
  canvas.draw_rect(rect(x, FOOT_Y + 8.0F, ACTOR_WIDTH, FOOT_HEIGHT - 8.0F),
                   tones.background);
  cover(canvas, rect(x, FOOT_Y, ACTOR_WIDTH, FOOT_HEIGHT));
  const float top = FOOT_Y + (FOOT_HEIGHT - PORTRAIT) / 2.0F;
  const auto face = rect(x + MARGIN + 2.0F, top, PORTRAIT, PORTRAIT);
  rendering::panel(canvas, face.grow(7.0F));
  drawing.image_part(canvas,
                     actor == Actor::Chef ? rendering::Image::ChefIdle
                                          : rendering::Image::AppraiserIdle,
                     rect(actor == Actor::Chef ? CHEF_FACE_X : APPRAISER_FACE_X,
                          actor == Actor::Chef ? CHEF_FACE_Y : APPRAISER_FACE_Y,
                          FACE, FACE),
                     face);
  const float column = x + ACTOR_COLUMN;
  const String status = actor_status(frame, actor);
  drawing.text(canvas, column, FOOT_Y + 22.0F, status, SMALL_SIZE,
               human ? tones.cyan : tones.muted, CHANGING);
}

/// One kind of ingredient in the bag.
struct BagItem {
  std::string_view name;
  std::uint32_t count = 0;
  bool known = true;
};

/// The ingredients both actors carry.
std::vector<BagItem> bag_of(const SurvivalFrame &frame) {
  const auto &view = frame.view;
  std::vector<BagItem> items;
  for (const auto &[value, count] : pack_of(view)) {
    const auto ingredient = IngredientId{value};
    items.push_back({.name = survival::ingredient_name(ingredient),
                     .count = count,
                     .known = identified(view, ingredient)});
  }
  return items;
}

/// The bag at the bottom middle: numbered slots with each item's icon and
/// count and a mark on an ingredient not yet identified. Leaves on the canvas
/// the drawn items as the `bag` meta.
void paint_bag(godot::Node2D &canvas, const rendering::Art &drawing,
               const SurvivalFrame &frame) {
  const auto tones = rendering::palette();
  cover(canvas, rect(BAG_X, FOOT_Y, BAG_WIDTH, FOOT_HEIGHT));
  const auto items = bag_of(frame);
  const std::size_t slots = std::max(BAG_SLOTS, items.size());
  // More kinds than slots share the bag's width, each slot showing its
  // left part under the next.
  const float pitch = slots > BAG_SLOTS ? (BAG_WIDTH - 20.0F - SLOT) /
                                              static_cast<float>(slots - 1)
                                        : SLOT_PITCH;
  const float shown = std::min(SLOT, pitch - 2.0F);
  const auto slot_at = [pitch](std::size_t index) {
    return Vector2(BAG_X + 10.0F + static_cast<float>(index) * pitch,
                   FOOT_Y + (FOOT_HEIGHT - SLOT) / 2.0F);
  };
  for (std::size_t index = 0; index < slots; ++index) {
    const auto at = slot_at(index);
    rendering::inset(canvas, rect(at.x, FOOT_Y, SLOT, FOOT_HEIGHT));
    if (index < items.size()) {
      paint_item(canvas, drawing, items.at(index).name,
                 at + Vector2(4.0F, 4.0F));
    }
  }
  // The marks go over every slot, so a crowded bag hides none of them.
  godot::Array drawn;
  for (std::size_t index = 0; index < slots; ++index) {
    const auto at = slot_at(index);
    drawing.text(canvas, at.x + 5.0F, at.y + 15.0F, number(index + 1),
                 SMALL_SIZE, tones.faint, Weight::Bold);
    if (index >= items.size()) {
      continue;
    }
    const auto &item = items.at(index);
    const String count = number(item.count);
    chip(canvas, drawing,
         at.x + shown - drawing.text_width(count, SMALL_SIZE, CHANGING) - 6.0F,
         at.y + SLOT - 5.0F, count, SMALL_SIZE, tones.white);
    if (!item.known) {
      chip(canvas, drawing, at.x + shown - 13.0F, at.y + 16.0F, "?", SMALL_SIZE,
           tones.gold);
    }
    godot::Dictionary entry;
    entry.set("name", from(item.name));
    entry.set("count", item.count);
    entry.set("known", item.known);
    drawn.push_back(entry);
  }
  canvas.set_meta("bag", drawn);
}

/// What one human's interact key does in the city and to what, or how long
/// the collection under way still holds the actor, for its button.
String interact_label(const SurvivalFrame &frame, Actor actor) {
  if (const auto &held = collecting_of(frame.view, actor)) {
    return "COLLECTING " + seconds(held->remaining);
  }
  const auto &offer = frame.offers.at(index_of(actor)).interact;
  if (const auto *collect =
          offer ? std::get_if<survival::Collect>(&*offer) : nullptr) {
    return "COLLECT " + rendering::place_name(frame.view, collect->source);
  }
  return "USE";
}

/// The movement, run and jump keys of one human.
String move_keys(const KeyLabels &keys) {
  return from(keys.move) + " move  " + from(keys.run) + " run  " +
         from(keys.jump) + " jump  ";
}

/// Where travel takes the actor, named by where it goes; none while a
/// collection holds the actor.
std::optional<String> travel_hint(const SurvivalFrame &frame, Actor actor) {
  if (collecting_of(frame.view, actor)) {
    return std::nullopt;
  }
  return state_of(frame.view, actor).location == Location::City ? "To base"
                                                                : "Set out";
}

/// One human's movement keys, then where travel takes it, which a click
/// takes.
String key_hint(const SurvivalFrame &frame, Actor actor,
                const KeyLabels &keys) {
  const auto travel = travel_hint(frame, actor);
  return travel ? move_keys(keys) + *travel : move_keys(keys).strip_edges();
}

/// The humans, chef first, with their key sets.
std::vector<std::pair<Actor, std::size_t>> humans(const SurvivalFrame &frame) {
  std::vector<std::pair<Actor, std::size_t>> found;
  for (const auto actor : ACTORS) {
    if (const auto keys = survival::key_set(frame.control, actor)) {
      found.emplace_back(actor, *keys);
    }
  }
  return found;
}

/// One human's row of the action panel: its button, and where its keys'
/// line starts on its baseline.
struct ActionRow {
  Actor actor = Actor::Chef;
  std::size_t set = 0;
  godot::Rect2 button;
  Vector2 hint;
};

/// The action panel's rows, one for a lone human, a smaller one for each of
/// two.
std::vector<ActionRow> action_rows(const SurvivalFrame &frame) {
  const float left = ACTION_X + 10.0F;
  const auto players = humans(frame);
  std::vector<ActionRow> rows;
  if (players.size() == 1) {
    const auto [actor, set] = players.front();
    rows.push_back(
        {.actor = actor,
         .set = set,
         .button = rect(left, FOOT_Y + 8.0F, ACTION_INNER, SOLO_BUTTON_HEIGHT),
         .hint = Vector2(left + 2.0F, FOOT_Y + FOOT_HEIGHT - 14.0F)});
    return rows;
  }
  float row = SCREEN_HEIGHT - MARGIN - DUO_HEIGHT + 12.0F;
  for (const auto &[actor, set] : players) {
    rows.push_back(
        {.actor = actor,
         .set = set,
         .button = rect(left + PLAYER_TAG, row, DUO_BUTTON, DUO_BUTTON_HEIGHT),
         .hint = Vector2(left + PLAYER_TAG + 2.0F,
                         row + DUO_BUTTON_HEIGHT + 14.0F)});
    row += DUO_ROW;
  }
  return rows;
}

/// The action at the bottom right: each human's interact key as a button
/// naming what it acts on, lit while it acts now, over that human's keys.
void paint_actions(godot::Node2D &canvas, const rendering::Art &drawing,
                   const SurvivalFrame &frame) {
  const auto tones = rendering::palette();
  const auto rows = action_rows(frame);
  const bool alone = rows.size() == 1;
  if (alone) {
    hud_panel(canvas, rect(ACTION_X, FOOT_Y, ACTION_WIDTH, FOOT_HEIGHT));
  } else {
    hud_panel(canvas, rect(ACTION_X, SCREEN_HEIGHT - MARGIN - DUO_HEIGHT,
                           ACTION_WIDTH, DUO_HEIGHT));
  }
  for (const auto &row : rows) {
    const auto &keys = KEY_LABELS.at(row.set);
    if (!alone) {
      drawing.text(canvas, ACTION_X + 10.0F, row.button.position.y + 23.0F,
                   "P" + number(row.set + 1), SMALL_SIZE, tones.cyan,
                   Weight::Bold);
    }
    paint_button(canvas, drawing, row.button, interact_label(frame, row.actor),
                 alone ? ACTION_SIZE : DUO_SIZE,
                 frame.offers.at(index_of(row.actor)).interact.has_value());
    drawing.text(canvas, row.hint.x, row.hint.y,
                 key_hint(frame, row.actor, keys), SMALL_SIZE, tones.muted,
                 CHANGING);
  }
}

/// The first human whose feed key would feed a stored dish.
std::optional<Actor> feeder(const SurvivalFrame &frame) {
  for (const auto actor : ACTORS) {
    const auto &offer = frame.offers.at(index_of(actor)).feed;
    if (survival::key_set(frame.control, actor) && offer &&
        std::holds_alternative<survival::Feed>(*offer)) {
      return actor;
    }
  }
  return std::nullopt;
}

/// Feeding the dish the first human's feed would feed: "Feed Fish Stew".
std::optional<String> feed_job(const SurvivalFrame &frame) {
  for (const auto actor : ACTORS) {
    const auto keys = survival::key_set(frame.control, actor);
    const auto &offer = frame.offers.at(index_of(actor)).feed;
    const auto *feed = offer ? std::get_if<survival::Feed>(&*offer) : nullptr;
    if (keys && feed != nullptr) {
      return "Feed " + from(survival::food_name(feed->food));
    }
  }
  return std::nullopt;
}

/// The base's second line: while a stored dish can be fed, its feed key
/// unless the task already names it, and what feeding asks; otherwise what
/// the offered dish would score.
String task_detail(const SurvivalFrame &frame) {
  if (!dish_to_feed(frame)) {
    return offered_forecast(frame);
  }
  const String needs =
      "A dish needs a score of " +
      number(frame.content.get().companion.workshop.satisfying_score) + ".";
  const auto feed = feed_job(frame);
  if (!feed || next_step(frame).begins_with(*feed)) {
    return "Feeding ends the day. " + needs;
  }
  return *feed + String(": this ends the day. ") + needs;
}

/// How many recipes can be cooked now, and where the rest are.
String cookable_note(const SurvivalFrame &frame) {
  const auto &recipes = frame.content.get().companion.workshop.recipes;
  const auto ready =
      std::ranges::count_if(recipes, [&frame](const auto &recipe) {
        return recipe_ready(frame.view, recipe);
      });
  const bool choose =
      ready > 1 && survival::key_set(frame.control, Actor::Chef).has_value();
  return "Can cook now: " + number(static_cast<std::uint64_t>(ready)) + " of " +
         number(recipes.size()) + ". Recipes on pause" +
         (choose ? String("   Next dish") : String());
}

/// The base's task card, risen into place as the workshop opens.
godot::Rect2 task_card(const rendering::Art &drawing) {
  return rect(TASK_X, TASK_Y + entrance_lift(drawing, "workshop"), TASK_WIDTH,
              TASK_HEIGHT);
}

/// The base's task card: the one thing to do now, large, what it would
/// score, how many dishes can be cooked, and the end-day key.
void paint_task(godot::Node2D &canvas, const rendering::Art &drawing,
                const SurvivalFrame &frame) {
  const auto tones = rendering::palette();
  const auto card = task_card(drawing);
  const float y = card.position.y;
  hud_panel(canvas, card);
  const float left = TASK_X + INSET;
  drawing.text(canvas, left, y + 38.0F, next_step(frame), TASK_SIZE,
               tones.white, CHANGING);
  drawing.text(canvas, left, y + 64.0F, task_detail(frame), DETAIL_SIZE,
               tones.white, CHANGING);
  drawing.text(canvas, left, y + TASK_NOTES, cookable_note(frame),
               TASK_NOTE_SIZE, tones.muted, CHANGING);
  const String end = end_day_job(frame);
  drawing.text(canvas,
               TASK_X + TASK_WIDTH - INSET -
                   drawing.text_width(end, TASK_NOTE_SIZE, CHANGING),
               y + TASK_NOTES, end, TASK_NOTE_SIZE, tones.muted, CHANGING);
}

/// The panel over the appraiser's head that holds `count` ingredients.
godot::Rect2 appraisal_area(std::size_t count) {
  const float width = static_cast<float>(count) * 52.0F + 14.0F;
  return rect(std::max(MARGIN, APPRAISER_HEAD_X - width / 2.0F),
              APPRAISER_HEAD_Y - 72.0F, width, 64.0F);
}

/// The ingredients not yet identified, over the appraiser's head, the one
/// its key would appraise ringed.
void paint_appraisal(godot::Node2D &canvas, const rendering::Art &drawing,
                     const SurvivalFrame &frame) {
  const auto tones = rendering::palette();
  std::vector<IngredientId> unknown;
  for (const auto &[value, count] : pack_of(frame.view)) {
    if (!identified(frame.view, IngredientId{value})) {
      unknown.push_back(IngredientId{value});
    }
  }
  if (unknown.empty()) {
    return;
  }
  const auto &offer = frame.offers.at(index_of(Actor::Appraiser)).interact;
  const auto *next = offer ? std::get_if<survival::Appraise>(&*offer) : nullptr;
  const auto area = appraisal_area(unknown.size());
  const float x = area.position.x;
  const float y = area.position.y;
  hud_panel(canvas, area);
  godot::PackedVector2Array pointer;
  pointer.push_back(Vector2(APPRAISER_HEAD_X - 7.0F, y + 62.0F));
  pointer.push_back(Vector2(APPRAISER_HEAD_X + 7.0F, y + 62.0F));
  pointer.push_back(Vector2(APPRAISER_HEAD_X, y + 70.0F));
  canvas.draw_colored_polygon(pointer, tones.rim);
  float left = x + 9.0F;
  for (const auto ingredient : unknown) {
    if (next != nullptr && next->ingredient == ingredient) {
      canvas.draw_rect(rect(left - 2.0F, y + 6.0F, ICON + 4.0F, ICON + 4.0F),
                       tones.gold, false, 2.0F);
    }
    paint_item(canvas, drawing, survival::ingredient_name(ingredient),
               Vector2(left, y + 8.0F));
    chip(canvas, drawing, left + ICON - 10.0F, y + 22.0F, "?", SMALL_SIZE,
         tones.gold);
    left += 52.0F;
  }
}

/// The stored dishes at the bottom right, each with its score counting up
/// as it appears, gold once it passes.
void paint_dishes(godot::Node2D &canvas, const rendering::Art &drawing,
                  const SurvivalFrame &frame) {
  const auto tones = rendering::palette();
  const auto &workshop = frame.view.workshop;
  const auto &tuning = frame.content.get().companion.workshop;
  hud_panel(canvas, rect(ACTION_X, FOOT_Y, ACTION_WIDTH, FOOT_HEIGHT));
  const float left = ACTION_X + 12.0F;
  drawing.text(canvas, left, FOOT_Y + 22.0F, "DISHES", SMALL_SIZE, tones.white,
               Weight::Bold);
  const String needs = "NEEDS " + number(tuning.satisfying_score);
  drawing.text(canvas,
               ACTION_X + ACTION_WIDTH - 12.0F -
                   drawing.text_width(needs, SMALL_SIZE, Weight::Bold),
               FOOT_Y + 22.0F, needs, SMALL_SIZE, tones.gold, Weight::Bold);
  if (workshop.foods.empty()) {
    drawing.text(canvas, left, FOOT_Y + 54.0F, NO_DISH, SMALL_SIZE, tones.faint,
                 CHANGING);
    return;
  }
  const float pitch =
      std::min(ICON + 8.0F,
               (ACTION_INNER - ICON) / static_cast<float>(std::max<std::size_t>(
                                           workshop.foods.size() - 1, 1)));
  float x = left;
  for (const auto &dish : workshop.foods) {
    paint_item(canvas, drawing, survival::food_name(dish.food),
               Vector2(x, FOOT_Y + 28.0F));
    const auto score = survival::dish_score(dish, workshop, tuning);
    const auto total = score ? score->total : 0U;
    // The score counts up to its value when the dish first shows.
    const auto shown = static_cast<std::uint64_t>(std::lround(drawing.glide(
        "score" + number(dish.food.value) +
            (dish.quality == survival::DishQuality::Fine ? "f" : "p"),
        static_cast<float>(total))));
    const String label =
        number(shown) +
        (dish.quality == survival::DishQuality::Fine ? String("+") : String());
    chip(canvas, drawing,
         x + ICON - drawing.text_width(label, SMALL_SIZE, CHANGING) - 2.0F,
         FOOT_Y + 74.0F, label, SMALL_SIZE,
         total >= tuning.satisfying_score ? tones.gold : tones.white);
    x += pitch;
  }
}

/// Draws over the kitchen and reads only snapshots, so no clock or key waits.
void paint_day_result(godot::Node2D &canvas, const rendering::Art &drawing,
                      const SurvivalFrame &frame) {
  const auto lines = day_result(frame);
  if (lines.empty()) {
    return;
  }
  const auto tones = rendering::palette();
  const float height =
      RESULT_LINE_HEIGHT * static_cast<float>(lines.size()) + 18.0F;
  const float top = RESULT_BOTTOM - height;
  hud_panel(canvas, rect(RESULT_X, top, RESULT_WIDTH, height));
  float y = top + 10.0F + static_cast<float>(RESULT_SIZE);
  bool headline = true;
  for (const auto &line : lines) {
    drawing.text(canvas, RESULT_X + INSET, y, line, RESULT_SIZE,
                 headline ? tones.gold : tones.white, CHANGING);
    headline = false;
    y += RESULT_LINE_HEIGHT;
  }
}

/// The two actors, the bag and the right corner's block along the bottom.
void paint_foot(godot::Node2D &canvas, const rendering::Art &drawing,
                const SurvivalFrame &frame) {
  paint_actor(canvas, drawing, frame, Actor::Chef, MARGIN);
  paint_actor(canvas, drawing, frame, Actor::Appraiser,
              MARGIN + ACTOR_WIDTH + GAP);
  paint_bag(canvas, drawing, frame);
}

void paint_workshop_screen(godot::Node2D &canvas, const rendering::Art &drawing,
                           const SurvivalFrame &frame) {
  rendering::paint_workshop(canvas, drawing,
                            !frame.view.workshop.foods.empty());
  paint_day_result(canvas, drawing, frame);
  paint_appraisal(canvas, drawing, frame);
  paint_task(canvas, drawing, frame);
  paint_foot(canvas, drawing, frame);
  paint_dishes(canvas, drawing, frame);
}

/// What each human in the city is told to go to, once each.
std::vector<survival::ObjectId> goals(const SurvivalFrame &frame) {
  std::vector<survival::ObjectId> found;
  for (const auto &offers : frame.offers) {
    if (offers.errand &&
        std::ranges::find(found, offers.errand->target) == found.end()) {
      found.push_back(offers.errand->target);
    }
  }
  return found;
}

String refusal_text(survival::Result result) {
  switch (result) {
  case survival::Result::Applied:
    return "";
  case survival::Result::NoExplorationTime:
    return "No exploration time left today.";
  case survival::Result::NotExploring:
    return "The way out is blocked.";
  case survival::Result::Busy:
    return "Finish collecting first.";
  case survival::Result::InvalidElapsed:
  case survival::Result::NotInWorkshop:
  case survival::Result::MissingIngredients:
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
  case survival::CollectionResult::Busy:
    return "Finish collecting first.";
  case survival::CollectionResult::NotExploring:
  case survival::CollectionResult::UnknownSource:
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
  case survival::WorkshopResult::NotCarried:
    return "The ingredients are not carried.";
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

String done_text(const GameView &view, const ActionEvent &event) {
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
                 rendering::place_name(view, action.source) + ".";
        } else if constexpr (std::is_same_v<Done, survival::Appraise>) {
          return from(survival::ingredient_name(action.ingredient)) +
                 (failed ? String(" appraisal failed: try again.")
                         : String(" identified."));
        } else if constexpr (std::is_same_v<Done, survival::Cook>) {
          return from(survival::food_name(action.food)) +
                 (failed ? String(" burned: ingredients lost.")
                         : String(" cooked."));
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
  rows.push_back(
      row("Collect; appraise or cook at base", one.interact, two.interact));
  rows.push_back(row("Choose the dish, as the chef at base",
                     EVERY_CHOOSE_DISH_KEY, "none"));
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

/// Every refusal a feature can answer with.
constexpr std::array<survival::ActionResult, 24> REFUSALS{
    survival::Result::NotExploring,
    survival::Result::NoExplorationTime,
    survival::Result::Busy,
    survival::Result::InvalidElapsed,
    survival::Result::NotInWorkshop,
    survival::Result::MissingIngredients,
    survival::Result::RunOver,
    survival::CollectionResult::Depleted,
    survival::CollectionResult::Unavailable,
    survival::CollectionResult::OutOfRange,
    survival::CollectionResult::Busy,
    survival::CollectionResult::NotExploring,
    survival::CollectionResult::UnknownSource,
    survival::WorkshopResult::NotInWorkshop,
    survival::WorkshopResult::NotCarried,
    survival::WorkshopResult::NoFood,
    survival::WorkshopResult::WrongRole,
    survival::WorkshopResult::AlreadyIdentified,
    survival::WorkshopResult::NotIdentified,
    survival::WorkshopResult::UnknownRecipe,
    survival::Result::Applied,
    survival::CollectionResult::Applied,
    survival::WorkshopResult::Applied,
    survival::WorkshopResult::Failed};

/// One object for each place name; a line names the place, so objects that
/// share a name print the same lines.
std::vector<survival::ObjectId> named_once(const GameView &view) {
  std::vector<String> names;
  std::vector<survival::ObjectId> objects;
  for (const auto &object : view.world.objects) {
    auto name = rendering::place_name(view, object.id);
    if (std::ranges::find(names, name) == names.end()) {
      names.push_back(std::move(name));
      objects.push_back(object.id);
    }
  }
  return objects;
}

/// Every sentence the log can show in this run: each actor's, place's,
/// ingredient's and dish's, and every refusal, with `places` from
/// `named_once`.
std::vector<String> every_note(const SurvivalFrame &frame,
                               const std::vector<survival::ObjectId> &places) {
  const auto &view = frame.view;
  const auto &tuning = frame.content.get().companion;
  // A day number of three digits stands for any day a run reaches.
  std::vector<Event> events{TimeUpEvent{}, EscapeMissEvent{.day = 100}};
  const auto acted = [&events](Actor actor, survival::Action action,
                               survival::ActionResult result) {
    events.emplace_back(
        ActionEvent{.actor = actor, .action = action, .result = result});
  };
  for (const auto actor : ACTORS) {
    events.emplace_back(FallEvent{.actor = actor});
    for (const auto result : REFUSALS) {
      acted(actor, survival::Enter{}, result);
      acted(actor, survival::ReturnToBase{}, result);
    }
    for (const auto object : places) {
      acted(actor, survival::Collect{.source = object},
            survival::CollectionResult::Applied);
    }
  }
  for (const auto object : places) {
    events.emplace_back(WorldEvent{.object = object,
                                   .state = survival::ObjectState::Destroyed});
    events.emplace_back(
        WorldEvent{.object = object, .state = survival::ObjectState::Intact});
  }
  for (const auto &flavor : tuning.workshop.flavors) {
    for (const auto result : {survival::WorkshopResult::Applied,
                              survival::WorkshopResult::Failed}) {
      acted(Actor::Appraiser,
            survival::Appraise{.ingredient = flavor.ingredient}, result);
    }
  }
  for (const auto &recipe : tuning.workshop.recipes) {
    for (const auto result : {survival::WorkshopResult::Applied,
                              survival::WorkshopResult::Failed}) {
      acted(Actor::Chef, survival::Cook{.food = recipe.food}, result);
    }
    acted(Actor::Chef, survival::Feed{.food = recipe.food},
          survival::WorkshopResult::Applied);
  }
  acted(Actor::Chef, survival::EndDayUnfed{.penalty = tuning.unfed_penalty},
        survival::WorkshopResult::Applied);
  std::vector<String> notes;
  notes.reserve(events.size());
  for (const auto &event : events) {
    notes.push_back(describe(view, event));
  }
  return notes;
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

/// The choice of players' entry `index`, counted from the top.
godot::Rect2 setup_area(std::size_t index) {
  return rect(SETUP_X, SETUP_Y + static_cast<float>(index) * SETUP_PITCH,
              SETUP_WIDTH, SETUP_HEIGHT);
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
    rendering::panel(canvas, setup_area(index));
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
    const auto ink = chosen ? tones.gold : tones.white;
    drawing.text(canvas, SETUP_X + 160.0F, y + 52.0F,
                 number(number_key) + "   " + label.at(0), 32, ink,
                 Weight::Bold);
    drawing.text(canvas, SETUP_X + 160.0F, y + 86.0F, label.at(1), 24,
                 chosen ? tones.white : tones.muted);
    y += SETUP_PITCH;
    ++number_key;
  }
  rendering::panel(canvas, rect(MARGIN, 600.0F, SCREEN_WIDTH - 2.0F * MARGIN,
                                SCREEN_HEIGHT - MARGIN - 600.0F));
  drawing.text(canvas, 30.0F, 642.0F,
               "Gather by day, appraise and cook at base, and feed the monster "
               "before the acid wrecks the city.",
               24, tones.white);
  drawing.text(canvas, 30.0F, 680.0F, "Click who is playing.", 16, tones.muted);
}

namespace {

/// "off" at level 0, otherwise the level as a percentage.
String volume_label(const VolumeRow &row) {
  if (row.level == 0 || row.steps == 0) {
    return "off";
  }
  return number(static_cast<std::uint32_t>(row.level) * 100U / row.steps) +
         String("%");
}

/// Block `step` of the pause screen's volume row `row`, `lift` below its
/// place.
godot::Rect2 volume_block(std::size_t row, std::uint8_t step, float lift) {
  const float baseline = PAUSE_BOX_Y + lift + PAUSE_BOX_HEIGHT - VOLUME_AREA +
                         static_cast<float>(row + 1) * VOLUME_ROW_HEIGHT;
  return rect(PAUSE_TEXT_X + VOLUME_BLOCKS_X +
                  static_cast<float>(step) * VOLUME_BLOCK_STEP,
              baseline - 16.0F, VOLUME_BLOCK_STEP - 4.0F, 18.0F);
}

/// The pause screen's volume rows under the keys: each a name, a row of
/// level blocks and a percentage, the row the left and right keys change
/// marked in the accent.
void paint_volumes(godot::Node2D &canvas, const rendering::Art &drawing,
                   const std::array<VolumeRow, 2> &volumes,
                   const std::vector<DisplayRow> &displays, float lift) {
  const auto tones = rendering::palette();
  const float top = PAUSE_BOX_Y + lift + PAUSE_BOX_HEIGHT - VOLUME_AREA;
  caption(canvas, drawing, PAUSE_TEXT_X, top, VOLUME_KEYS, tones.gold);
  float y = top + VOLUME_ROW_HEIGHT;
  for (std::size_t index = 0; index < volumes.size(); ++index) {
    const auto &row = volumes.at(index);
    const godot::Color lit = row.marked ? tones.gold : tones.cyan;
    if (row.marked) {
      canvas.draw_rect(rect(PAUSE_TEXT_X - 10.0F, y - VOLUME_ROW_HEIGHT + 10.0F,
                            KEY_ROWS_WIDTH, VOLUME_ROW_HEIGHT),
                       tones.raised);
    }
    drawing.text(canvas, PAUSE_TEXT_X, y, row.name, PAUSE_ROW_SIZE,
                 row.marked ? tones.gold : tones.white);
    const float blocks = PAUSE_TEXT_X + VOLUME_BLOCKS_X;
    for (std::uint8_t step = 0; step < row.steps; ++step) {
      const auto block = volume_block(index, step, lift);
      rendering::inset(canvas, block);
      if (step < row.level) {
        canvas.draw_rect(block.grow(-2.0F), lit);
      }
    }
    drawing.text(canvas,
                 blocks + static_cast<float>(row.steps) * VOLUME_BLOCK_STEP +
                     10.0F,
                 y, volume_label(row), PAUSE_ROW_SIZE, tones.white, CHANGING);
    y += VOLUME_ROW_HEIGHT;
  }
  paint_display_rows(canvas, drawing, displays, PAUSE_TEXT_X, y,
                     VOLUME_ROW_HEIGHT);
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

/// Every recipe with what it still needs, the monster's mark and its taste,
/// then the ingredients eaten so far and what feeding does, from `top`.
void paint_recipe_book(godot::Node2D &canvas, const rendering::Art &drawing,
                       const SurvivalFrame &frame, float top) {
  const auto tones = rendering::palette();
  caption(canvas, drawing, BOOK_X, top - 24.0F, "RECIPE BOOK", tones.gold);
  caption(canvas, drawing,
          BOOK_RIGHT - drawing.text_width("TASTE", SMALL_SIZE, CHANGING),
          top - 24.0F, "TASTE", tones.gold);
  float y = top;
  bool stripe = false;
  for (const auto &recipe : frame.content.get().companion.workshop.recipes) {
    if (stripe) {
      canvas.draw_rect(rect(BOOK_X - 10.0F, y - BOOK_ROW + 7.0F,
                            BOOK_RIGHT - BOOK_X + 20.0F, BOOK_ROW),
                       tones.raised);
    }
    stripe = !stripe;
    const bool ready = recipe_ready(frame.view, recipe);
    drawing.text(canvas, BOOK_X, y, from(survival::food_name(recipe.food)),
                 BOOK_SIZE, ready ? tones.white : tones.muted, CHANGING);
    const String needs = needs_text(recipe_needs(frame.view, recipe));
    drawing.text(canvas, BOOK_X + BOOK_NEEDS, y,
                 needs.is_empty() ? String("ready to cook") : needs, SMALL_SIZE,
                 needs.is_empty() ? tones.cyan : tones.faint, CHANGING);
    const String mark = eaten_mark(frame, recipe.food);
    drawing.text(canvas, BOOK_X + BOOK_MARK, y, mark, SMALL_SIZE,
                 mark == "hated" ? tones.danger : tones.muted, CHANGING);
    const String taste = number(
        survival::taste_of(recipe, frame.content.get().companion.workshop));
    drawing.text(canvas,
                 BOOK_RIGHT - drawing.text_width(taste, BOOK_SIZE, CHANGING), y,
                 taste, BOOK_SIZE, tones.white, CHANGING);
    y += BOOK_ROW;
  }
  y += 12.0F;
  for (const auto &line : eaten_lines(frame.view.workshop.eaten_ingredients)) {
    drawing.text(canvas, BOOK_X, y, line, SMALL_SIZE, tones.muted, CHANGING);
    y += 18.0F;
  }
  y += 12.0F;
  caption(canvas, drawing, BOOK_X, y, "FEEDING", tones.gold);
  for (const auto *rule : FEED_RULES) {
    y += 20.0F;
    drawing.text(canvas, BOOK_X, y, rule, SMALL_SIZE, tones.white);
  }
}

/// How far below its place the open interruption's panel still is as it
/// comes in.
float interruption_lift(const rendering::Art &drawing) {
  return entrance_lift(drawing, "interruption");
}

/// The pause screen's button at its top right, `lift` below its place.
godot::Rect2 continue_area(const rendering::Art &drawing, const String &text,
                           float lift) {
  const float width = button_width(drawing, text, INTERRUPTION_LINE_SIZE);
  return rect(PAUSE_BOX_X + PAUSE_BOX_WIDTH - PAUSE_INSET - width,
              PAUSE_BOX_Y + lift + 36.0F, width, 48.0F);
}

/// A notice's button under its line, `lift` below its place.
godot::Rect2 notice_button_area(const rendering::Art &drawing,
                                const String &text, float lift) {
  return rect(NOTICE_TEXT_X - BUTTON_PAD, NOTICE_BOX_Y + lift + 150.0F,
              button_width(drawing, text, INTERRUPTION_LINE_SIZE), 48.0F);
}

/// The pause screen's subtitle, with when the escape checks begin.
String pause_line(const std::array<String, 3> &text) {
  return text.at(1) + String(" ") + escape_goal() + String(".");
}

} // namespace

rendering::Shot shot_of(const rendering::Art &drawing,
                        const SurvivalFrame &frame) {
  if (frame.view.progression.phase != Phase::Exploration) {
    return {.camera = rendering::whole_scene(),
            .heading = rendering::whole_scene(),
            .out_of_reach = {},
            .goals = {}};
  }
  const auto heading =
      rendering::aim(frame.content.get(), frame.view, frame.control);
  return {.camera = rendering::follow(
              drawing,
              survival::map_of(frame.content.get(), frame.view.exploration.map),
              heading),
          .heading = heading,
          .out_of_reach = out_of_reach(frame),
          .goals = goals(frame)};
}

void paint_survival_world(godot::Node2D &canvas, const rendering::Art &drawing,
                          const SurvivalFrame &frame,
                          const rendering::Shot &shot,
                          const std::vector<rendering::Burst> &bursts) {
  drawing.shift(canvas, {});
  rendering::paint_mouth(canvas, drawing, acid_ratio(frame), shot.camera);
  if (frame.view.progression.phase == Phase::Exploration) {
    rendering::paint_city(canvas, drawing, frame.content.get(), frame.view,
                          shot.out_of_reach);
    rendering::paint_city_bursts(canvas, bursts, acid_ratio(frame));
  }
  drawing.shift(canvas, {});
}

void paint_survival_glow(godot::Node2D &canvas, const rendering::Art &drawing,
                         const SurvivalFrame &frame,
                         const rendering::Shot &shot) {
  drawing.shift(canvas, {});
  rendering::paint_glow(canvas, drawing, acid_ratio(frame), shot.camera);
  if (frame.view.progression.phase == Phase::Exploration) {
    rendering::paint_city_glow(
        canvas, drawing,
        survival::map_of(frame.content.get(), frame.view.exploration.map),
        shot.camera);
  }
  drawing.shift(canvas, {});
}

void paint_survival(godot::Node2D &canvas, const rendering::Art &drawing,
                    const SurvivalFrame &frame, const rendering::Shot &shot,
                    const std::vector<rendering::Burst> &bursts,
                    std::optional<Tip> tip) {
  canvas.set_meta("hud_panels", godot::Array());
  canvas.set_meta("interface_frames", godot::Dictionary());
  drawing.shift(canvas, {});
  const bool city = frame.view.progression.phase == Phase::Exploration;
  if (city) {
    paint_foot(canvas, drawing, frame);
    paint_actions(canvas, drawing, frame);
    paint_goal(canvas, drawing, next_step(frame), hand_off_notice(frame));
  } else {
    paint_workshop_screen(canvas, drawing, frame);
    rendering::paint_workshop_bursts(canvas, drawing, bursts);
    // The task card names the step at base; the goal line keeps the notice.
    paint_goal(canvas, drawing, {}, hand_off_notice(frame));
  }
  paint_corners(canvas, drawing, frame);
  const auto notes = paint_notes(canvas, drawing, frame);
  godot::Array note_areas;
  for (const auto &note : notes) {
    note_areas.push_back(note);
  }
  canvas.set_meta("hud_notes", note_areas);
  if (city) {
    // The map's words are placed once the HUD has taken its place, clear of
    // it.
    std::vector<godot::Rect2> covered = notes;
    add_areas(covered, canvas, "hud_panels");
    drawing.shift(canvas, {});
    rendering::paint_city_labels(canvas, drawing, frame.view, frame.control,
                                 shot, covered);
    drawing.shift(canvas, {});
  }
  if (tip) {
    paint_tip(canvas, drawing, frame, *tip, notes);
  }
  rendering::paint_screen_bursts(canvas, bursts);
}

void paint_interruption(godot::Node2D &canvas, const rendering::Art &drawing,
                        Interruption interruption, const SurvivalFrame &frame,
                        const std::array<VolumeRow, 2> &volumes,
                        const std::vector<DisplayRow> &displays) {
  const auto tones = rendering::palette();
  const auto text = interruption_text(interruption);
  const float shown = drawing.entrance("interruption");
  // The veil falls over the tip with the rest of the held screen.
  godot::Dictionary tip = canvas.get_meta("hud_tip", godot::Dictionary());
  if (!tip.is_empty()) {
    tip.set("held", true);
    canvas.set_meta("hud_tip", tip);
  }
  const float lift = interruption_lift(drawing);
  canvas.draw_rect(rect(0.0F, 0.0F, SCREEN_WIDTH, SCREEN_HEIGHT),
                   color(9, 10, 20, 0.72F * shown));
  if (interruption == Interruption::Paused) {
    rendering::panel(canvas, rect(PAUSE_BOX_X, PAUSE_BOX_Y + lift,
                                  PAUSE_BOX_WIDTH, PAUSE_BOX_HEIGHT));
    drawing.text(canvas, PAUSE_TEXT_X, PAUSE_BOX_Y + lift + 70.0F, text.at(0),
                 INTERRUPTION_TITLE_SIZE, tones.white, Weight::Bold);
    drawing.text(canvas, PAUSE_TEXT_X, PAUSE_BOX_Y + lift + 104.0F,
                 pause_line(text), PAUSE_ROW_SIZE, tones.muted, CHANGING);
    caption(canvas, drawing, PAUSE_TEXT_X, PAUSE_ROWS_Y + lift - 24.0F, "KEYS",
            tones.gold);
    paint_key_rows(canvas, drawing, key_rows(frame), PAUSE_TEXT_X,
                   PAUSE_ROWS_Y + lift, PAUSE_ROW_HEIGHT);
    paint_recipe_book(canvas, drawing, frame, PAUSE_ROWS_Y + lift);
    const auto button = continue_area(drawing, text.at(2), lift);
    paint_button(canvas, drawing, button, text.at(2), INTERRUPTION_LINE_SIZE,
                 false);
    canvas.set_meta("interruption_button", button);
    paint_volumes(canvas, drawing, volumes, displays, lift);
    return;
  }
  rendering::panel(canvas, rect(NOTICE_BOX_X, NOTICE_BOX_Y + lift,
                                NOTICE_BOX_WIDTH, NOTICE_BOX_HEIGHT));
  drawing.text(canvas, NOTICE_TEXT_X, NOTICE_BOX_Y + lift + 70.0F, text.at(0),
               INTERRUPTION_TITLE_SIZE, tones.white, Weight::Bold);
  drawing.text(canvas, NOTICE_TEXT_X, NOTICE_BOX_Y + lift + 116.0F, text.at(1),
               INTERRUPTION_LINE_SIZE, tones.muted, CHANGING);
  const auto button = notice_button_area(drawing, text.at(2), lift);
  canvas.set_meta("interruption_button",
                  text.at(2).is_empty() ? godot::Rect2() : button);
  if (!text.at(2).is_empty()) {
    paint_button(canvas, drawing, button, text.at(2), INTERRUPTION_LINE_SIZE,
                 false);
  }
}

void paint_cinematic(godot::Node2D &canvas, const rendering::Art &drawing,
                     Cinematic cinematic, bool fallback,
                     const SurvivalFrame &frame) {
  canvas.draw_rect(rect(0.0F, 0.0F, SCREEN_WIDTH, SCREEN_HEIGHT),
                   color(8, 6, 16, 1.0F));
  if (!fallback) {
    return;
  }
  const auto tones = rendering::palette();
  const auto &progression = frame.view.progression;
  rendering::paint_mouth(canvas, drawing, acid_ratio(frame),
                         rendering::whole_scene());
  drawing.shift(canvas, {});
  canvas.draw_rect(rect(0.0F, 0.0F, SCREEN_WIDTH, SCREEN_HEIGHT),
                   color(14, 12, 26, 0.6F));
  drawing.image(canvas, rendering::Image::ChefIdle,
                godot::Vector2(130.0F, 300.0F), 2.0F);
  drawing.image(canvas, rendering::Image::AppraiserIdle,
                godot::Vector2(280.0F, 300.0F), 2.0F);
  switch (cinematic) {
  case Cinematic::Intro:
    drawing.text(canvas, 130.0F, 170.0F, "SWALLOWED", 48, tones.white,
                 Weight::Bold);
    drawing.text(canvas, 130.0F, 230.0F,
                 "A monster ate the city. Keep it fed, and look for a way out.",
                 24, tones.white);
    break;
  case Cinematic::Ending:
    drawing.text(canvas, 130.0F, 170.0F, "ESCAPED", 48, tones.gold,
                 Weight::Bold);
    drawing.text(canvas, 130.0F, 230.0F,
                 "The party got out of the monster after " +
                     number(progression.day - 1) + String(" days."),
                 24, tones.white);
    break;
  case Cinematic::Loss:
    drawing.text(canvas, 130.0F, 170.0F, "DISSOLVED", 48, tones.success,
                 Weight::Bold);
    drawing.text(canvas, 130.0F, 230.0F,
                 "The acid filled the monster's mouth after " +
                     number(progression.day - 1) + String(" days."),
                 24, tones.white);
    break;
  }
  drawing.text(canvas, 130.0F, 560.0F, "Placeholder card: no cinematic yet.",
               24, tones.muted);
  paint_button(canvas, drawing, card_button(drawing), CARD_BUTTON,
               INTERRUPTION_LINE_SIZE, false);
}

String tip_text(Tip tip) {
  switch (tip) {
  case Tip::SetOut:
    return "Set out into the city to gather food. The day's clock runs only "
           "there.";
  case Tip::Move:
    return "Walk, run and jump with the keys under your action button; climb "
           "ladders.";
  case Tip::Collect:
    return "Collect with the action button. It holds you while the clock runs "
           "on.";
  case Tip::Appraise:
    return "A ? marks a find not yet identified: the appraiser does it at "
           "base.";
  case Tip::Cook:
    return "A recipe is ready: the chef cooks it into a dish that keeps until "
           "fed.";
  case Tip::Feed:
    return "Feeding ends the day. New foods and new ingredients score the "
           "most.";
  case Tip::Rejected:
    return "The monster hated that dish, and refuses that food from now on.";
  case Tip::Acid:
    return "The acid rises after a day unfed, a hated dish, or a low score.";
  case Tip::Escape:
    return "From day " + number(survival::TEMPORARY_ESCAPE.first_day) +
           " each finished day may open the way out. Hold on until then.";
  }
  std::abort();
}

String tip_name(Tip tip) {
  switch (tip) {
  case Tip::SetOut:
    return "set_out";
  case Tip::Move:
    return "move";
  case Tip::Collect:
    return "collect";
  case Tip::Appraise:
    return "appraise";
  case Tip::Cook:
    return "cook";
  case Tip::Feed:
    return "feed";
  case Tip::Rejected:
    return "rejected";
  case Tip::Acid:
    return "acid";
  case Tip::Escape:
    return "escape";
  }
  std::abort();
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
                    const std::vector<KeyRow> &rows, float left, float top,
                    float row_height) {
  const auto tones = rendering::palette();
  float y = top;
  bool stripe = false;
  for (const auto &row : rows) {
    // The heading row names the players, not keys.
    if (row.action.is_empty()) {
      caption(canvas, drawing, left + KEY_FIRST, y, row.first, tones.gold);
      caption(canvas, drawing, left + KEY_SECOND, y, row.second, tones.gold);
      y += row_height * 0.75F;
      continue;
    }
    if (stripe) {
      canvas.draw_rect(
          rect(left - 10.0F, y - row_height + 9.0F, KEY_ROWS_WIDTH, row_height),
          tones.raised);
    }
    stripe = !stripe;
    drawing.text(canvas, left, y, row.action, KEY_ROW_SIZE, tones.white,
                 CHANGING);
    drawing.text(canvas, left + KEY_FIRST, y, row.first, KEY_ROW_SIZE,
                 tones.cyan, CHANGING);
    drawing.text(canvas, left + KEY_SECOND, y, row.second, KEY_ROW_SIZE,
                 tones.cyan, CHANGING);
    y += row_height;
  }
}

std::array<String, 3> interruption_text(Interruption interruption) {
  switch (interruption) {
  case Interruption::Paused:
    return {"PAUSED", "The clock is stopped.", "Continue     Leave the run"};
  case Interruption::ConfirmLeave:
  case Interruption::ConfirmLeaveFromPause:
    return {"LEAVE THIS RUN?", "Progress is not saved: the run ends here.",
            "Leave for the main menu     Cancel"};
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
  return "One player left, as the " +
         String(actor_name(chef ? Actor::Chef : Actor::Appraiser)) + ": " +
         move_keys(KEY_LABELS.at(0)).strip_edges();
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
    return workshop_step(frame).text;
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
      steps.emplace_back(*keys, "Set out into the city.");
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
  return offer ? offered_step(frame.view, *offer)
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
  // Words on a button stay inside its padding.
  const float padding = 2.0F * BUTTON_PAD;
  const float goal_inner = GOAL_WIDTH - 2.0F * GOAL_PAD;
  const auto places = named_once(frame.view);
  std::vector<FittedText> lines;
  lines.push_back(fitted(PAUSE_HINT, SMALL_SIZE, TIME_WIDTH - 40.0F));
  // The NEXT step is the goal line in the city and the task card at base.
  if (frame.view.progression.phase == Phase::Workshop) {
    lines.push_back(fitted(next_step(frame), TASK_SIZE, TASK_INNER));
  } else {
    lines.push_back(fitted(next_step(frame), GOAL_SIZE, goal_inner));
  }
  lines.push_back(fitted(hand_off_notice(frame), HAND_OFF_SIZE, goal_inner));
  for (const auto &note : every_note(frame, places)) {
    lines.push_back(fitted(note, SMALL_SIZE, NOTE_WIDTH - 20.0F));
  }
  for (const auto &line : day_result(frame)) {
    lines.push_back(fitted(line, RESULT_SIZE, RESULT_WIDTH - 2.0F * INSET));
  }
  // Status words occupy their own lines.
  for (const auto actor : ACTORS) {
    lines.push_back(
        fitted(actor_status(frame, actor), SMALL_SIZE, ACTOR_TEXT_WIDTH));
  }
  lines.push_back(fitted("AI  at base", SMALL_SIZE, ACTOR_TEXT_WIDTH));
  // The city's action: a lone human's button and keys, or each of two
  // humans' rows, with every place its key can act on in the button, and
  // the hold's words in place of the verb.
  const auto players = humans(frame);
  const bool alone = players.size() == 1;
  const float button = (alone ? ACTION_INNER : DUO_BUTTON) - padding;
  const float hint = alone ? ACTION_INNER - 2.0F : DUO_BUTTON - 2.0F;
  std::vector<String> labels{"COLLECTING " + collection_clock(frame)};
  for (const auto &[actor, set] : players) {
    const auto &keys = KEY_LABELS.at(set);
    labels.push_back(interact_label(frame, actor));
    lines.push_back(fitted(key_hint(frame, actor, keys), SMALL_SIZE, hint));
    for (const auto object : places) {
      const String place = rendering::place_name(frame.view, object);
      labels.push_back("COLLECT " + place);
    }
  }
  for (const auto &label : labels) {
    lines.push_back(fitted(label, alone ? ACTION_SIZE : DUO_SIZE, button));
  }
  // The base's task card.
  const String end = end_day_job(frame);
  lines.push_back(fitted(task_detail(frame), DETAIL_SIZE, TASK_INNER));
  lines.push_back(fitted(end, TASK_NOTE_SIZE, TASK_INNER / 2.0F));
  lines.push_back(
      fitted(cookable_note(frame), TASK_NOTE_SIZE, TASK_INNER / 2.0F - 20.0F));
  lines.push_back(fitted(NO_DISH, SMALL_SIZE, ACTION_INNER));
  const auto &tuning = frame.content.get().companion.workshop;
  for (const auto &recipe : tuning.recipes) {
    lines.push_back(
        fitted(cook_forecast(frame, recipe.food), DETAIL_SIZE, TASK_INNER));
    lines.push_back(fitted("Feed " + from(survival::food_name(recipe.food)) +
                               ": this ends the day. A dish needs a score of " +
                               number(tuning.satisfying_score) + ".",
                           DETAIL_SIZE, TASK_INNER));
    lines.push_back(fitted(from(survival::food_name(recipe.food)), BOOK_SIZE,
                           BOOK_NEEDS - 8.0F));
    const auto subsets = std::size_t{1} << recipe.ingredients.size();
    for (std::size_t held = 0; held < subsets; ++held) {
      for (std::size_t known = 0; known < subsets; ++known) {
        lines.push_back(
            fitted(needs_text(needs_given(recipe.ingredients, held, known)),
                   SMALL_SIZE, BOOK_MARK - BOOK_NEEDS - 8.0F));
      }
    }
  }
  std::vector<IngredientId> every_ingredient;
  every_ingredient.reserve(tuning.flavors.size());
  for (const auto &flavor : tuning.flavors) {
    every_ingredient.push_back(flavor.ingredient);
  }
  for (const auto &line : eaten_lines(every_ingredient)) {
    lines.push_back(fitted(line, SMALL_SIZE, BOOK_RIGHT - BOOK_X));
  }
  for (const auto *rule : FEED_RULES) {
    lines.push_back(fitted(rule, SMALL_SIZE, BOOK_RIGHT - BOOK_X));
  }
  for (const auto &rows : {key_rows(frame), control_rows()}) {
    for (const auto &row : rows) {
      lines.push_back(fitted(row.action, KEY_ROW_SIZE, KEY_FIRST - 8.0F));
      lines.push_back(
          fitted(row.first, KEY_ROW_SIZE, KEY_SECOND - KEY_FIRST - 8.0F));
      lines.push_back(
          fitted(row.second, KEY_ROW_SIZE, KEY_ROWS_WIDTH - KEY_SECOND));
    }
  }
  lines.push_back(fitted(pause_line(interruption_text(Interruption::Paused)),
                         PAUSE_ROW_SIZE, BOOK_RIGHT - PAUSE_TEXT_X));
  const float notice_width =
      NOTICE_BOX_X + NOTICE_BOX_WIDTH - INSET - NOTICE_TEXT_X;
  for (const auto interruption :
       {Interruption::ConfirmLeave, Interruption::Resuming}) {
    const auto text = interruption_text(interruption);
    lines.push_back(fitted(text.at(1), INTERRUPTION_LINE_SIZE, notice_width));
    lines.push_back(fitted(text.at(2), INTERRUPTION_LINE_SIZE,
                           notice_width - padding + BUTTON_PAD));
  }
  return lines;
}

std::vector<String> action_labels(const SurvivalFrame &frame) {
  std::vector<String> labels;
  for (const auto &[actor, set] : humans(frame)) {
    labels.push_back(interact_label(frame, actor));
  }
  return labels;
}

String describe(const GameView &view, const Event &event) {
  return std::visit(
      [&view](const auto &happened) -> String {
        using Happened = std::decay_t<decltype(happened)>;
        if constexpr (std::is_same_v<Happened, ActionEvent>) {
          const String refusal =
              std::visit([](auto result) { return refusal_text(result); },
                         happened.result);
          return refusal.is_empty() ? done_text(view, happened) : refusal;
        } else if constexpr (std::is_same_v<Happened, FallEvent>) {
          return String(actor_name(happened.actor)) +
                 String(" fell and washed back to base.");
        } else if constexpr (std::is_same_v<Happened, TimeUpEvent>) {
          return "Time is up: everyone is back at base.";
        } else if constexpr (std::is_same_v<Happened, EscapeMissEvent>) {
          return "Day " + number(happened.day) + ": the way out stayed shut.";
        } else {
          const String name = rendering::place_name(view, happened.object);
          return happened.state == survival::ObjectState::Destroyed
                     ? name + String(" was destroyed.")
                     : name + String(" appeared.");
        }
      },
      event);
}

namespace {

/// A line of text drawn from `x` on `baseline`, as a target's area.
godot::Rect2 line_area(const rendering::Art &drawing, float x, float baseline,
                       const String &text, int size) {
  return rect(x - 4.0F, baseline - static_cast<float>(size) - 2.0F,
              drawing.text_width(text, size, CHANGING) + 8.0F,
              static_cast<float>(size) + 8.0F);
}

/// A button whose words name two keys, split where `second` begins, so
/// each key's words are a target of their own.
std::pair<godot::Rect2, godot::Rect2>
split_button(const rendering::Art &drawing, godot::Rect2 button,
             const String &text, const String &second, int size) {
  const float x =
      button.position.x + BUTTON_PAD +
      drawing.text_width(text.substr(0, text.find(second)), size, CHANGING) -
      6.0F;
  return {rect(button.position.x, button.position.y, x - button.position.x,
               button.size.y),
          rect(x, button.position.y, button.get_end().x - x, button.size.y)};
}

void city_targets(std::vector<Target> &targets, const rendering::Art &drawing,
                  const SurvivalFrame &frame) {
  for (const auto &row : action_rows(frame)) {
    const auto &keys = KEY_LABELS.at(row.set);
    String refusal;
    if (collecting_of(frame.view, row.actor)) {
      refusal = "Collecting: wait until it is done.";
    } else if (!frame.offers.at(index_of(row.actor)).interact) {
      refusal = "Nothing within reach to collect.";
    }
    targets.push_back(
        {.area = row.button,
         .press = PlayerPress{.actor = row.actor, .key = PlayerKey::Interact},
         .refusal = refusal});
    if (const auto travel = travel_hint(frame, row.actor)) {
      const float x = row.hint.x +
                      drawing.text_width(move_keys(keys), SMALL_SIZE, CHANGING);
      targets.push_back(
          {.area = line_area(drawing, x, row.hint.y, *travel, SMALL_SIZE),
           .press = PlayerPress{.actor = row.actor, .key = PlayerKey::Travel}});
    }
  }
}

void workshop_targets(std::vector<Target> &targets,
                      const rendering::Art &drawing,
                      const SurvivalFrame &frame) {
  const auto job = workshop_step(frame);
  const auto card = task_card(drawing);
  const float top = card.position.y;
  targets.push_back(
      {.area = rect(TASK_X, top, TASK_WIDTH, 48.0F),
       .press = job.press.value_or(PlayerPress{}),
       .refusal = job.press ? String() : String("The AI is working.")});
  if (const auto actor = feeder(frame);
      actor && job.press && job.press->key != PlayerKey::Feed) {
    targets.push_back(
        {.area = rect(TASK_X, top + 48.0F, TASK_WIDTH, 22.0F),
         .press = PlayerPress{.actor = actor, .key = PlayerKey::Feed}});
  }
  if (survival::key_set(frame.control, Actor::Chef)) {
    const auto &recipes = frame.content.get().companion.workshop.recipes;
    const auto ready =
        std::ranges::count_if(recipes, [&frame](const auto &recipe) {
          return recipe_ready(frame.view, recipe);
        });
    targets.push_back(
        {.area = line_area(drawing, TASK_X + INSET, top + TASK_NOTES,
                           cookable_note(frame), TASK_NOTE_SIZE),
         .press = DishPress{survival::Direction::Down},
         .refusal = ready > 1 ? String() : String("No other dish to choose.")});
  }
  const String end = end_day_job(frame);
  const float end_width = drawing.text_width(end, TASK_NOTE_SIZE, CHANGING);
  targets.push_back(
      {.area = line_area(drawing, TASK_X + TASK_WIDTH - INSET - end_width,
                         top + TASK_NOTES, end, TASK_NOTE_SIZE),
       .press = PlayerPress{.actor = first_player(frame),
                            .key = PlayerKey::EndDay}});
  if (survival::key_set(frame.control, Actor::Chef)) {
    const auto &offer = frame.offers.at(index_of(Actor::Chef)).interact;
    const auto figure = drawing.size(rendering::Image::ChefIdle) * FIGURE_SCALE;
    targets.push_back(
        {.area =
             rect(CHEF_FIGURE_X, KITCHEN_FLOOR - figure.y, figure.x, figure.y),
         .press = PlayerPress{.actor = Actor::Chef, .key = PlayerKey::Interact},
         .refusal = offer && std::holds_alternative<survival::Cook>(*offer)
                        ? String()
                        : String("No dish can be cooked now.")});
  }
  const auto unknown =
      std::ranges::count_if(pack_of(frame.view), [&frame](const auto &entry) {
        return !identified(frame.view, IngredientId{entry.first});
      });
  if (unknown > 0) {
    const bool human =
        survival::key_set(frame.control, Actor::Appraiser).has_value();
    const auto &offer = frame.offers.at(index_of(Actor::Appraiser)).interact;
    String refusal;
    if (!human) {
      refusal = "The AI appraiser works here.";
    } else if (!offer || !std::holds_alternative<survival::Appraise>(*offer)) {
      refusal = "Nothing to appraise now.";
    }
    targets.push_back(
        {.area = appraisal_area(static_cast<std::size_t>(unknown)),
         .press =
             PlayerPress{.actor = Actor::Appraiser, .key = PlayerKey::Interact},
         .refusal = refusal});
  }
}

} // namespace

std::vector<Target> setup_targets() {
  std::vector<Target> targets;
  targets.reserve(SETUP_ENTRIES.size());
  for (std::size_t index = 0; index < SETUP_ENTRIES.size(); ++index) {
    targets.push_back(
        {.area = setup_area(index), .press = OptionPress{index + 1}});
  }
  return targets;
}

std::vector<Target> survival_targets(const rendering::Art &drawing,
                                     const SurvivalFrame &frame) {
  std::vector<Target> targets;
  if (frame.view.progression.phase == Phase::Exploration) {
    city_targets(targets, drawing, frame);
  } else if (frame.view.progression.phase == Phase::Workshop) {
    workshop_targets(targets, drawing, frame);
  }
  targets.push_back({.area = pause_chip(drawing), .press = MenuKey::Pause});
  return targets;
}

std::vector<Target> cinematic_targets(const rendering::Art &drawing) {
  const auto [skip, leave] =
      split_button(drawing, card_button(drawing), CARD_BUTTON, CARD_LEAVE,
                   INTERRUPTION_LINE_SIZE);
  return {
      {.area = skip,
       .press = PlayerPress{.actor = std::nullopt, .key = PlayerKey::Interact}},
      {.area = leave, .press = MenuKey::Menu}};
}

std::vector<Target>
interruption_targets(const rendering::Art &drawing, Interruption interruption,
                     const std::array<VolumeRow, 2> &volumes,
                     const std::vector<DisplayRow> &displays) {
  const auto text = interruption_text(interruption);
  const float lift = interruption_lift(drawing);
  std::vector<Target> targets;
  switch (interruption) {
  case Interruption::Paused: {
    const auto [resume, leave] =
        split_button(drawing, continue_area(drawing, text.at(2), lift),
                     text.at(2), "Leave", INTERRUPTION_LINE_SIZE);
    targets.push_back({.area = resume, .press = MenuKey::Pause});
    targets.push_back({.area = leave, .press = MenuKey::Menu});
    // A row's name is no target, so a click there cannot mute it.
    for (std::size_t row = 0; row < volumes.size(); ++row) {
      for (std::uint8_t step = 0; step < volumes.at(row).steps; ++step) {
        // Each block's area reaches over the gap to the next.
        targets.push_back({.area = volume_block(row, step, lift)
                                       .grow_individual(0.0F, 6.0F, 4.0F, 4.0F),
                           .press = VolumePress{
                               .bus = volumes.at(row).bus,
                               .level = static_cast<std::uint8_t>(step + 1)}});
      }
    }
    const float display_top =
        PAUSE_BOX_Y + lift + PAUSE_BOX_HEIGHT - VOLUME_AREA +
        static_cast<float>(volumes.size() + 1) * VOLUME_ROW_HEIGHT;
    for (auto &target : display_row_targets(displays, PAUSE_TEXT_X, display_top,
                                            VOLUME_ROW_HEIGHT)) {
      targets.push_back(std::move(target));
    }
    return targets;
  }
  case Interruption::ConfirmLeave:
  case Interruption::ConfirmLeaveFromPause: {
    const auto [leave, cancel] =
        split_button(drawing, notice_button_area(drawing, text.at(2), lift),
                     text.at(2), "Cancel", INTERRUPTION_LINE_SIZE);
    targets.push_back({.area = leave, .press = MenuKey::Confirm});
    targets.push_back({.area = cancel, .press = MenuKey::Pause});
    return targets;
  }
  case Interruption::Resuming:
    return targets;
  }
  std::abort();
}

} // namespace neocity::hud
