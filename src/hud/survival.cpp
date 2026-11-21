#include "survival.hpp"

#include "../rendering/survival.hpp"
#include "title.hpp"

#include <survival/ending.hpp>

#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/core/error_macros.hpp>
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
#include <optional>
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
constexpr int SMALL_SIZE = 2 * rendering::TYPE_UNIT;
/// The face's capitals stand five of its eight pixels per em.
constexpr float CAP_HEIGHT = 0.625F;
/// The top bar: the day at the left, the acid in the middle, the time at the
/// right, each a plate of the art set at its own size, and the notice line
/// under the acid.
constexpr float BAND_Y = MARGIN;
constexpr float BAND_HEIGHT = 48.0F;
constexpr float BAND_BASELINE = BAND_Y + 32.0F;
constexpr int BIG_SIZE = 3 * rendering::TYPE_UNIT;
constexpr float DAY_WIDTH = 192.0F;
constexpr float TIME_WIDTH = 160.0F;
constexpr float TIME_X = SCREEN_WIDTH - MARGIN - TIME_WIDTH;
constexpr float ACID_WIDTH = 256.0F;
constexpr float ACID_X = (SCREEN_WIDTH - ACID_WIDTH) / 2.0F;
constexpr int ACID_SIZE = BIG_SIZE;
/// Where a plate's words and icon start, and the gauge's height and room from
/// the words and the plate's edge.
constexpr float PLATE_PAD = 18.0F;
constexpr float PLATE_ICON_X = 10.0F;
constexpr float PLATE_ICON_Y = 11.0F;
constexpr float PLATE_WORDS_X = 47.0F;
constexpr float GAUGE_HEIGHT = 8.0F;
constexpr float GAUGE_GAP = 10.0F;
/// The acid share from which the acid plate warns, and the room its triangle
/// takes.
constexpr float ACID_WARNING = 0.5F;
constexpr float WARNING_MARK = 22.0F;
constexpr float GOAL_Y = BAND_Y + BAND_HEIGHT + GAP;
constexpr int GOAL_SIZE = SMALL_SIZE;
constexpr float GOAL_PAD = 14.0F;
/// The goal line may run the screen's width, between the corners above and
/// the pause chip and notes below.
constexpr float GOAL_WIDTH = SCREEN_WIDTH - 2.0F * MARGIN;
constexpr int HAND_OFF_SIZE = SMALL_SIZE;
/// The action prompt at the bottom right, one for each human: the art's plate
/// at its own height, widened to its words.
constexpr float PROMPT_HEIGHT = 48.0F;
constexpr float PROMPT_MIN_WIDTH = 170.0F;
constexpr float PROMPT_MAX_WIDTH = 580.0F;
/// What a prompt's player tag and key cap take of its width, at most.
constexpr float PROMPT_KEYS_WIDTH = 120.0F;
constexpr float PROMPT_PAD = 22.0F;
constexpr int PROMPT_SIZE = BIG_SIZE;
constexpr const char *BASE_NOTICE = "Moving and jumping do not work at base.";
/// The base's deck along the bottom: the appraise, cook and feed panels under
/// the kitchen.
constexpr float DECK_Y = 556.0F;
constexpr float DECK_HEIGHT = 148.0F;
constexpr float APPRAISE_X = 20.0F;
constexpr float APPRAISE_WIDTH = 404.0F;
constexpr float COOK_X = 440.0F;
constexpr float COOK_WIDTH = 398.0F;
constexpr float FEED_X = 852.0F;
constexpr float FEED_WIDTH = 408.0F;
constexpr float DECK_PAD = 16.0F;
constexpr float ICON = 56.0F;
/// A slot of the deck, and its pitch.
constexpr float DECK_SLOT = 52.0F;
constexpr int DECK_TITLE_SIZE = 3 * rendering::TYPE_UNIT;
constexpr int DECK_SIZE = 2 * rendering::TYPE_UNIT;
/// The words under a panel's slots stand this far under its top.
constexpr float DECK_NOTE_Y = 110.0F;
constexpr float DECK_NOTE_STEP = 20.0F;
constexpr float DECK_SLOTS_Y = 40.0F;
constexpr float SERVE_WIDTH = 132.0F;
constexpr float SERVE_HEIGHT = 56.0F;
constexpr const char *SERVE = "SERVE";
constexpr const char *NEXT_DISH = "NEXT DISH";
constexpr const char *NO_DISH_OFFERED = "No dish";
constexpr const char *NOTHING_TO_APPRAISE = "Nothing to appraise.";
constexpr const char *AI_APPRAISES = "The AI appraiser works here.";
/// Where the chef stands in the kitchen art, as a target of its own.
constexpr float CHEF_AREA_X = 536.0F;
constexpr float CHEF_AREA_Y = 280.0F;
constexpr float CHEF_AREA_WIDTH = 110.0F;
constexpr float CHEF_AREA_HEIGHT = 170.0F;
/// A transfer screen: slots in rows of eight under a title, the container's
/// (or the fridge's) over the actor's own.
constexpr float SLOT = 58.0F;
constexpr float SLOT_PITCH = 60.0F;
constexpr std::size_t SLOT_COLUMNS = 8;
constexpr float TRANSFER_WIDTH =
    static_cast<float>(SLOT_COLUMNS) * SLOT_PITCH - 2.0F + 2.0F * INSET;
constexpr float TRANSFER_Y = 116.0F;
constexpr float DECK_INNER = COOK_WIDTH - 2.0F * DECK_PAD;
/// The day's result occupies the header, above the kitchen and its workers.
constexpr float RESULT_WIDTH = 800.0F;
constexpr float RESULT_X = APPRAISE_X;
constexpr float RESULT_TOP = 162.0F;
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
constexpr int INTERRUPTION_TITLE_SIZE = 4 * rendering::TYPE_UNIT;
constexpr int INTERRUPTION_LINE_SIZE = BIG_SIZE;
/// A panel's entrance lifts it this far into place.
constexpr float ENTRANCE_LIFT = 18.0F;
constexpr const char *NO_DISH = "None stored yet";
/// What the NEXT line says while a screen is open, since the screen takes the
/// interact key.
constexpr const char *CLOSE_FIRST = "Close the inventory first.";
constexpr const char *INVENTORY_OPEN = "Inventory open: search again to close.";
/// A first-play tip: one line on a panel, and its gold caption.
constexpr float TIP_HEIGHT = 36.0F;
constexpr float TIP_Y = 120.0F;
constexpr float TIP_PAD = 14.0F;
constexpr float TIP_BASELINE = 24.0F;
constexpr const char *TIP_CAPTION = "TIP";
/// The placeholder card's button, split into its two targets at "Main menu".
constexpr const char *CARD_BUTTON = "Skip     Main menu";
constexpr const char *CARD_LEAVE = "Main menu";
constexpr float CARD_BUTTON_X = 130.0F;
constexpr float CARD_BUTTON_Y = 610.0F;
/// The end screen: the picture scaled to the screen's width from the top, and
/// a fixed band over its foot holding the title, the description and the way
/// to the main menu.
constexpr float END_BAND_Y = 534.0F;
constexpr float END_TEXT_X = 85.0F;
constexpr float END_RIGHT = 1216.0F;
constexpr float END_BUTTON_Y = 614.0F;
constexpr float END_BUTTON_HEIGHT = 48.0F;
constexpr const char *END_LEAVE = "Main menu";
/// The choice of players: a panel at the right of the title art, holding the
/// heading, one row per entry and a hint line.
constexpr float SETUP_PANEL_X = 740.0F;
constexpr float SETUP_PANEL_Y = 110.0F;
constexpr float SETUP_PANEL_WIDTH = 500.0F;
constexpr float SETUP_PANEL_HEIGHT = 500.0F;
constexpr float SETUP_X = 764.0F;
constexpr float SETUP_Y = 200.0F;
constexpr float SETUP_WIDTH = 452.0F;
constexpr float SETUP_HEIGHT = 104.0F;
constexpr float SETUP_PITCH = 116.0F;
constexpr float SETUP_FIGURE_HEIGHT = 72.0F;

/// The key each action is bound to in project.godot, one entry per key set.
struct KeyLabels {
  const char *move;
  const char *every_move;
  const char *run;
  const char *travel;
  const char *interact;
  const char *search;
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
                                          .search = "R",
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
                                          .search = "P",
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

/// The container this actor has open, if any.
const std::optional<survival::Searching> &searching_of(const GameView &view,
                                                       Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return view.exploration.chef.searching;
  case Actor::Appraiser:
    return view.exploration.appraiser.searching;
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

/// The clock of the top bar: the base's own time at base while it has a
/// clock, and the day's exploration time otherwise.
String top_clock(const GameView &view) {
  const auto &base = view.workshop.remaining_base;
  return corner_clock(base && view.progression.phase == Phase::Workshop
                          ? *base
                          : view.progression.remaining_exploration);
}

bool identified(const GameView &view, IngredientId ingredient) {
  const auto &known = view.workshop.identified;
  // A part of the escape device is no find for the appraiser.
  return ingredient == survival::ESCAPE_PART ||
         std::ranges::find(known, ingredient) != known.end();
}

/// Ingredients the two actors carry and the fridge holds together, by
/// identity value: what base work reads.
std::map<std::uint32_t, std::uint32_t> pack_of(const GameView &view) {
  std::map<std::uint32_t, std::uint32_t> pack;
  for (const auto actor : ACTORS) {
    for (const auto ingredient : state_of(view, actor).inventory) {
      ++pack[ingredient.value];
    }
  }
  for (const auto ingredient : view.progression.fridge) {
    ++pack[ingredient.value];
  }
  return pack;
}

/// Draws the 56 px icon of `name` with its top-left corner at `at`; a name the
/// art set has no icon for is written there instead.
void paint_item(godot::Node2D &canvas, const rendering::Art &drawing,
                std::string_view name, Vector2 at) {
  if (const auto icon = rendering::item_icon(name)) {
    const bool ingredient = *icon >= rendering::Image::IngredientTomato &&
                            *icon <= rendering::Image::IngredientTruffle;
    const String path = "icons/" +
                        String(ingredient ? "ingredient_" : "dish_") +
                        from(name).to_lower().replace(" ", "_") + ".png";
    paint_hd_art(canvas, path, godot::Rect2(at, Vector2(ICON, ICON)));
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
  rendering::plate(canvas, area);
  cover(canvas, area);
}

/// The main action button with its words centred on its height, focused when
/// marked.
void paint_button(godot::Node2D &canvas, const rendering::Art &drawing,
                  godot::Rect2 area, const String &text, int size,
                  bool marked) {
  paint_action(canvas, drawing, area, text, size, marked);
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
        if constexpr (std::is_same_v<Offer, survival::Search>) {
          return "Search " + rendering::place_name(view, offer.source);
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

/// Adds to `seen` every cell movement reaches from `from`, never through
/// `obstacles`.
void add_walkable(Cells &seen, const survival::WorldSnapshot &world,
                  const std::vector<GridPosition> &obstacles,
                  GridPosition from) {
  for (const auto cell :
       survival::reachable_cells(survival::CellMap(world, obstacles), from)) {
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

String city_step(const SurvivalFrame &frame, Actor actor,
                 const KeyLabels &keys) {
  const auto &offers = frame.offers.at(index_of(actor));
  const bool walking = offers.errand && offers.errand->step.has_value();
  if (const auto &held = searching_of(frame.view, actor)) {
    return "Searching " + rendering::place_name(frame.view, held->source) +
           ": move food, then search again to close.";
  }
  if (frame.screens.at(index_of(actor)).open) {
    return INVENTORY_OPEN;
  }
  // A container passed on the way to what the party lacks can be searched.
  if (offers.search && offers.errand && !walking) {
    return offered_step(frame.view, *offers.search);
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
    // An open screen takes its human's interact key, so the step waits on
    // closing it.
    if (step->press && step->press->key == PlayerKey::Interact &&
        step->press->actor &&
        frame.screens.at(index_of(*step->press->actor)).open) {
      return {.text = INVENTORY_OPEN,
              .press = PlayerPress{.actor = step->press->actor,
                                   .key = PlayerKey::Search}};
    }
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

/// The acid's bar in the top bar, its fill eased to `fill`.
void gauge(godot::Node2D &canvas, const rendering::Art &drawing,
           const String &name, godot::Rect2 area, float fill) {
  const float shown = drawing.glide(name, std::clamp(fill, 0.0F, 1.0F));
  rendering::progress(canvas, area, shown);
}

/// The day at the top left, the acid in the middle and the time at the right,
/// each on a kit plate with its words drawn over it.
void paint_corners(godot::Node2D &canvas, const rendering::Art &drawing,
                   const SurvivalFrame &frame) {
  const auto tones = rendering::palette();
  const auto &progression = frame.view.progression;
  const auto plate = [&](float x, float width) {
    const auto area = rect(x, BAND_Y, width, BAND_HEIGHT);
    rendering::plate(canvas, area);
    cover(canvas, area);
  };
  plate(MARGIN, DAY_WIDTH);
  drawing.image(canvas, rendering::Image::HdSun,
                Vector2(MARGIN + PLATE_ICON_X, BAND_Y + PLATE_ICON_Y));
  const String day = "DAY ";
  drawing.text(canvas, MARGIN + PLATE_WORDS_X, BAND_BASELINE, day, BIG_SIZE,
               tones.white, Weight::Bold);
  drawing.text(canvas,
               MARGIN + PLATE_WORDS_X +
                   drawing.text_width(day, BIG_SIZE, Weight::Bold),
               BAND_BASELINE, two_digits(progression.day), BIG_SIZE, tones.lime,
               Weight::Bold);
  const bool rising = acid_ratio(frame) >= ACID_WARNING;
  const auto acid_plate = rect(ACID_X, BAND_Y, ACID_WIDTH, BAND_HEIGHT);
  if (rising) {
    rendering::warning(canvas, acid_plate);
    cover(canvas, acid_plate);
  } else {
    plate(ACID_X, ACID_WIDTH);
  }
  const int acid_size = rising ? SMALL_SIZE : ACID_SIZE;
  const String acid = rising ? "ACID RISING " : "ACID ";
  const String percent =
      number(static_cast<std::uint64_t>(progression.acid.value) * 100U /
             frame.content.get().run.full_acid.value) +
      "%";
  const float words = ACID_X + PLATE_PAD + (rising ? WARNING_MARK : 0.0F);
  const float acid_width = drawing.text_width(acid, acid_size, Weight::Bold);
  drawing.text(canvas, words, BAND_BASELINE, acid, acid_size, tones.magenta,
               Weight::Bold);
  drawing.text(canvas, words + acid_width, BAND_BASELINE, percent, acid_size,
               tones.white, Weight::Bold);
  const float gauge_x = words + acid_width +
                        drawing.text_width(percent, acid_size, Weight::Bold) +
                        GAUGE_GAP;
  gauge(canvas, drawing, "acid",
        rect(gauge_x, BAND_Y + (BAND_HEIGHT - GAUGE_HEIGHT) / 2.0F,
             ACID_X + ACID_WIDTH - PLATE_PAD - gauge_x, GAUGE_HEIGHT),
        acid_ratio(frame));
  plate(TIME_X, TIME_WIDTH);
  drawing.image(canvas, rendering::Image::HdClock,
                Vector2(TIME_X + PLATE_ICON_X, BAND_Y + PLATE_ICON_Y));
  drawing.text(canvas, TIME_X + PLATE_WORDS_X, BAND_BASELINE,
               top_clock(frame.view), BIG_SIZE, tones.white, Weight::Bold);
}

/// The notice line under the acid, sized to its words, with a hand-off notice
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
                 HAND_OFF_SIZE, tones.lime, CHANGING);
  }
}

/// Every `Rect2` in the canvas meta `name`.
void add_areas(std::vector<godot::Rect2> &areas, const godot::Node2D &canvas,
               const char *name) {
  const godot::Array listed = canvas.get_meta(name, godot::Array());
  for (std::int64_t index = 0; index < listed.size(); ++index) {
    areas.push_back(listed.get(index));
  }
}

/// The reserved tip row stays fixed; an inventory overlay can hide it.
std::optional<godot::Rect2> tip_place(float width,
                                      const std::vector<godot::Rect2> &taken) {
  const auto place = rect(APPRAISE_X, TIP_Y, width, TIP_HEIGHT);
  const auto apart = place.grow(GAP / 2.0F);
  if (std::ranges::none_of(taken, [&apart](const godot::Rect2 &area) {
        return area.intersects(apart);
      })) {
    return place;
  }
  return std::nullopt;
}

/// Draws `tip` at base where it covers no HUD panel or target, and leaves it
/// as the `hud_tip` meta.
void paint_tip(godot::Node2D &canvas, const rendering::Art &drawing,
               const SurvivalFrame &frame, Tip tip) {
  std::vector<godot::Rect2> taken;
  add_areas(taken, canvas, "hud_panels");
  for (const auto &target : survival_targets(drawing, frame)) {
    taken.push_back(target.area);
  }
  const auto tones = rendering::palette();
  const String text = tip_line(frame, tip);
  const float caption_width =
      drawing.text_width(TIP_CAPTION, SMALL_SIZE, Weight::Bold) + TIP_PAD;
  const auto place =
      tip_place(caption_width + drawing.text_width(text, SMALL_SIZE, CHANGING) +
                    2.0F * TIP_PAD,
                taken);
  if (!place) {
    return;
  }
  rendering::plate(canvas, *place);
  const float x = place->position.x + TIP_PAD;
  const float baseline = place->position.y + TIP_BASELINE;
  drawing.text(canvas, x, baseline, TIP_CAPTION, SMALL_SIZE, tones.lime,
               Weight::Bold);
  drawing.text(canvas, x + caption_width, baseline, text, SMALL_SIZE,
               tones.white, CHANGING);
  godot::Dictionary drawn;
  drawn.set("rect", *place);
  drawn.set("text", text);
  canvas.set_meta("hud_tip", drawn);
}

/// The end screen's Main menu button, at the band's right.
godot::Rect2 end_button(const rendering::Art &drawing) {
  const float width = button_width(drawing, END_LEAVE, INTERRUPTION_LINE_SIZE);
  return rect(END_RIGHT - width, END_BUTTON_Y, width, END_BUTTON_HEIGHT);
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

/// The container or fridge a human's transfer screen shows beside its own
/// slots, and where the screen stands.
struct TransferView {
  Actor actor = Actor::Chef;
  TransferScreen screen;
  /// The open container; null at base (the fridge) or with nothing open.
  const survival::SourceStock *container = nullptr;
  bool has_other = false;
  std::vector<IngredientId> other_items;
  std::size_t other_capacity = 0;
  std::size_t mine_capacity = 0;
  godot::Rect2 panel;
  Vector2 other_origin;
  Vector2 mine_origin;
  float other_title = 0.0F;
  float mine_title = 0.0F;
  float weight_line = 0.0F;
  float hint_line = 0.0F;
};

std::size_t rows_of(std::size_t slots) {
  return (slots + SLOT_COLUMNS - 1) / SLOT_COLUMNS;
}

/// Where slot `index` of a block of slots at `origin` stands.
godot::Rect2 slot_area(Vector2 origin, std::size_t index) {
  const std::size_t row = index / SLOT_COLUMNS;
  return rect(origin.x + static_cast<float>(index % SLOT_COLUMNS) * SLOT_PITCH,
              origin.y + static_cast<float>(row) * SLOT_PITCH, SLOT, SLOT);
}

/// Each open transfer screen, chef first: one centred, two side by side.
std::vector<TransferView> transfers(const SurvivalFrame &frame) {
  std::vector<TransferView> found;
  for (const auto actor : ACTORS) {
    const auto &screen = frame.screens.at(index_of(actor));
    if (!screen.open || !survival::key_set(frame.control, actor)) {
      continue;
    }
    TransferView view;
    view.actor = actor;
    view.screen = screen;
    const auto &held = searching_of(frame.view, actor);
    if (held) {
      const auto &sources = frame.view.exploration.sources;
      const auto stock =
          std::ranges::find(sources, held->source, &survival::SourceStock::id);
      if (stock != sources.end()) {
        view.container = &*stock;
      }
    }
    const bool at_base = state_of(frame.view, actor).location == Location::Base;
    view.has_other = view.container != nullptr || at_base;
    if (view.container != nullptr) {
      view.other_items = view.container->items;
      view.other_capacity = view.container->capacity;
    } else if (at_base) {
      view.other_items = frame.view.progression.fridge;
      view.other_capacity = frame.view.progression.fridge_slots;
    }
    view.mine_capacity = frame.view.progression.inventory_slots;
    found.push_back(std::move(view));
  }
  if (found.empty()) {
    return found;
  }
  const float gap = GAP;
  const float total = static_cast<float>(found.size()) * TRANSFER_WIDTH +
                      static_cast<float>(found.size() - 1) * gap;
  float left = (SCREEN_WIDTH - total) / 2.0F;
  for (auto &view : found) {
    const float x = left + INSET;
    float y = TRANSFER_Y + 12.0F;
    if (view.has_other) {
      view.other_title = y + static_cast<float>(SMALL_SIZE);
      y = view.other_title + 8.0F;
      view.other_origin = Vector2(x, y);
      y += static_cast<float>(rows_of(view.other_capacity)) * SLOT_PITCH + 6.0F;
    }
    view.mine_title = y + static_cast<float>(SMALL_SIZE);
    y = view.mine_title + 8.0F;
    view.mine_origin = Vector2(x, y);
    y += static_cast<float>(rows_of(view.mine_capacity)) * SLOT_PITCH + 4.0F;
    view.weight_line = y + static_cast<float>(SMALL_SIZE);
    view.hint_line = view.weight_line + static_cast<float>(SMALL_SIZE) + 6.0F;
    view.panel = rect(left, TRANSFER_Y, TRANSFER_WIDTH,
                      view.hint_line + 12.0F - TRANSFER_Y);
    left += TRANSFER_WIDTH + gap;
  }
  return found;
}

/// One block of slots with each item's icon and weight and a mark on an
/// ingredient not yet identified, and the cursor's ring on slot `cursor`.
void paint_slots(godot::Node2D &canvas, const rendering::Art &drawing,
                 const SurvivalFrame &frame,
                 const std::vector<IngredientId> &items, std::size_t capacity,
                 Vector2 origin, std::optional<std::size_t> cursor) {
  const auto tones = rendering::palette();
  const auto &carry = frame.content.get().companion.exploration.carry;
  for (std::size_t index = 0; index < capacity; ++index) {
    const auto area = slot_area(origin, index);
    rendering::card(canvas, area, rendering::State::Default);
    if (index < items.size()) {
      const auto item = items.at(index);
      paint_item(canvas, drawing, survival::ingredient_name(item),
                 area.position + Vector2(1.0F, 1.0F));
      const String weight = "w" + number(survival::weight_of(carry, item));
      chip(canvas, drawing, area.position.x + 4.0F, area.position.y + 16.0F,
           weight, SMALL_SIZE, tones.muted);
      chip(canvas, drawing, area.position.x + SLOT - 14.0F,
           area.position.y + SLOT - 5.0F, "1", SMALL_SIZE, tones.white);
      if (!identified(frame.view, item)) {
        chip(canvas, drawing, area.position.x + SLOT - 13.0F,
             area.position.y + 16.0F, "?", SMALL_SIZE, tones.lime);
      }
    }
  }
  if (cursor && *cursor < capacity) {
    canvas.draw_rect(slot_area(origin, *cursor), tones.lime, false, 2.0F);
  }
}

/// The title over the container's or fridge's slots.
String other_title(const SurvivalFrame &frame, const TransferView &view) {
  const String count =
      number(view.other_items.size()) + "/" + number(view.other_capacity);
  if (view.container != nullptr) {
    return rendering::place_name(frame.view, view.container->id) + "  " + count;
  }
  return "FRIDGE  " + count;
}

/// The weight line under an actor's slots: carried over the most it carries
/// at full speed, with the slowdown named while over.
String weight_text(const SurvivalFrame &frame, Actor actor) {
  const auto &carry = frame.content.get().companion.exploration.carry;
  const auto &items = state_of(frame.view, actor).inventory;
  String text = "Weight " + number(survival::carried_weight(carry, items)) +
                "/" + number(carry.max_weight);
  if (survival::overweight(carry, items)) {
    text += "  OVERWEIGHT: " +
            number(static_cast<std::uint64_t>(carry.overweight_percent)) +
            "% speed";
  }
  return text;
}

/// The keys a transfer screen answers to, for its hint line.
String transfer_hint(const SurvivalFrame &frame, Actor actor) {
  const auto keys = survival::key_set(frame.control, actor);
  const auto &labels = KEY_LABELS.at(keys.value_or(0));
  return from(labels.move) + " pick  " + from(labels.interact) + " move  " +
         from(labels.search) + " close";
}

/// The search and inventory screens over the scene: for each human who opened
/// one, the container's (or fridge's) slots over its own twelve.
void paint_transfers(godot::Node2D &canvas, const rendering::Art &drawing,
                     const SurvivalFrame &frame) {
  const auto tones = rendering::palette();
  godot::Array drawn;
  for (const auto &view : transfers(frame)) {
    rendering::plate(canvas, view.panel);
    cover(canvas, view.panel);
    const auto &state = state_of(frame.view, view.actor);
    const float x = view.panel.position.x + INSET;
    if (view.has_other) {
      drawing.text(canvas, x, view.other_title, other_title(frame, view),
                   SMALL_SIZE, tones.cyan, CHANGING);
      paint_slots(canvas, drawing, frame, view.other_items, view.other_capacity,
                  view.other_origin,
                  view.screen.mine ? std::nullopt
                                   : std::optional(view.screen.slot));
    }
    drawing.text(canvas, x, view.mine_title,
                 String(actor_name(view.actor)) + "  " +
                     number(state.inventory.size()) + "/" +
                     number(view.mine_capacity),
                 SMALL_SIZE, tones.cyan, CHANGING);
    paint_slots(canvas, drawing, frame, state.inventory, view.mine_capacity,
                view.mine_origin,
                view.screen.mine ? std::optional(view.screen.slot)
                                 : std::nullopt);
    const auto &carry = frame.content.get().companion.exploration.carry;
    drawing.text(
        canvas, x, view.weight_line, weight_text(frame, view.actor), SMALL_SIZE,
        survival::overweight(carry, state.inventory) ? tones.lime : tones.white,
        CHANGING);
    drawing.text(canvas, x, view.hint_line, transfer_hint(frame, view.actor),
                 SMALL_SIZE, tones.muted, CHANGING);
    godot::Dictionary entry;
    entry.set("actor", from(actor_name(view.actor)));
    entry.set("panel", view.panel);
    entry.set("mine", view.screen.mine);
    entry.set("slot", static_cast<std::int64_t>(view.screen.slot));
    drawn.push_back(entry);
  }
  // Recorded for the art checks only; the game itself records none of it.
  if (canvas.has_meta("measure_art")) {
    canvas.set_meta("transfers", drawn);
  }
}

/// The key one human's prompt names and what it does.
struct PromptAction {
  PlayerKey key = PlayerKey::Search;
  String text;
};

/// What one human's prompt offers now: the search key's action, closing the
/// screen it opened or searching the container in reach; with none, the
/// travel key's, named by where it goes; none while a search holds the actor.
std::optional<PromptAction> prompt_action(const SurvivalFrame &frame,
                                          Actor actor) {
  if (frame.screens.at(index_of(actor)).open) {
    return PromptAction{.key = PlayerKey::Search, .text = "CLOSE"};
  }
  const auto &offer = frame.offers.at(index_of(actor)).search;
  if (const auto *search =
          offer ? std::get_if<survival::Search>(&*offer) : nullptr) {
    return PromptAction{
        .key = PlayerKey::Search,
        .text = "SEARCH " + rendering::place_name(frame.view, search->source)};
  }
  const auto &use = frame.offers.at(index_of(actor)).interact;
  if (use && std::holds_alternative<survival::UseDevice>(*use)) {
    return PromptAction{.key = PlayerKey::Interact, .text = "USE DEVICE"};
  }
  if (!searching_of(frame.view, actor) &&
      state_of(frame.view, actor).location == Location::City) {
    return PromptAction{.key = PlayerKey::Travel, .text = "TO BASE"};
  }
  return std::nullopt;
}

/// What one human's prompts offer at base, the search key's first: the fridge
/// or closing the screen it opened, then the interact key's cooking or
/// appraising while the offer stands.
std::vector<PromptAction> base_actions(const SurvivalFrame &frame,
                                       Actor actor) {
  const bool open = frame.screens.at(index_of(actor)).open;
  std::vector<PromptAction> found{
      {.key = PlayerKey::Search,
       .text = open ? String("CLOSE") : String("SEARCH Fridge")}};
  const auto &offer = frame.offers.at(index_of(actor)).interact;
  if (offer && std::holds_alternative<survival::Cook>(*offer)) {
    found.push_back({.key = PlayerKey::Interact, .text = "COOK"});
  } else if (offer && std::holds_alternative<survival::Appraise>(*offer)) {
    found.push_back({.key = PlayerKey::Interact, .text = "APPRAISE"});
  }
  return found;
}

/// The movement, run and jump keys of one human.
String move_keys(const KeyLabels &keys) {
  return from(keys.move) + " move  " + from(keys.run) + " run  " +
         from(keys.jump) + " jump  ";
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

/// One human's action prompt at the bottom right: its player tag (with two
/// humans), the search key's cap and the action, on a plate widened to them.
struct Prompt {
  Actor actor = Actor::Chef;
  String tag;
  String cap;
  String action;
  PlayerKey key = PlayerKey::Search;
  godot::Rect2 area;
};

/// The prompts of the humans that have an action now, the chef's above, each
/// in the place it has when both humans have one.
std::vector<Prompt> prompts(const rendering::Art &drawing,
                            const SurvivalFrame &frame) {
  const auto players = humans(frame);
  const bool base = frame.view.progression.phase == Phase::Workshop;
  // Each place is a prompt row; at base a human may have two.
  std::vector<std::tuple<Actor, std::size_t, PromptAction>> entries;
  for (const auto &[actor, set] : players) {
    if (base) {
      for (auto &action : base_actions(frame, actor)) {
        entries.emplace_back(actor, set, std::move(action));
      }
    } else if (auto action = prompt_action(frame, actor)) {
      entries.emplace_back(actor, set, std::move(*action));
    }
  }
  const float bottom = base ? DECK_Y - GAP : SCREEN_HEIGHT - MARGIN;
  std::vector<Prompt> shown;
  for (std::size_t place = 0; place < entries.size(); ++place) {
    auto &[actor, set, action] = entries.at(place);
    const auto &labels = KEY_LABELS.at(set);
    const char *cap = labels.search;
    if (action.key == PlayerKey::Travel) {
      cap = labels.travel;
    } else if (action.key == PlayerKey::Interact) {
      cap = labels.interact;
    }
    Prompt prompt{.actor = actor,
                  .tag = players.size() > 1 ? "P" + number(set + 1) : String(),
                  .cap = "[" + from(cap) + "]",
                  .action = std::move(action.text),
                  .key = action.key,
                  .area = {}};
    const float words =
        drawing.text_width(prompt.tag + String("  "), PROMPT_SIZE,
                           Weight::Bold) +
        drawing.text_width(prompt.cap + String(" "), PROMPT_SIZE,
                           Weight::Bold) +
        drawing.text_width(prompt.action, PROMPT_SIZE, Weight::Bold);
    const float width = std::clamp(words + 2.0F * PROMPT_PAD, PROMPT_MIN_WIDTH,
                                   PROMPT_MAX_WIDTH);
    // In the city a human keeps its row whether or not it has a prompt.
    const auto rows = base ? entries.size() : players.size();
    const auto row =
        base ? place
             : static_cast<std::size_t>(
                   std::ranges::find(players, actor,
                                     &std::pair<Actor, std::size_t>::first) -
                   players.begin());
    const auto below = static_cast<float>(rows - 1 - row);
    prompt.area = rect(SCREEN_WIDTH - MARGIN - width,
                       bottom - PROMPT_HEIGHT - below * (PROMPT_HEIGHT + GAP),
                       width, PROMPT_HEIGHT);
    shown.push_back(std::move(prompt));
  }
  return shown;
}

/// The action prompts, each a plate over the screen's corner with its words.
void paint_prompts(godot::Node2D &canvas, const rendering::Art &drawing,
                   const SurvivalFrame &frame) {
  const auto tones = rendering::palette();
  for (const auto &prompt : prompts(drawing, frame)) {
    rendering::plate(canvas, prompt.area);
    cover(canvas, prompt.area);
    const float baseline =
        prompt.area.position.y +
        (PROMPT_HEIGHT + static_cast<float>(PROMPT_SIZE) * CAP_HEIGHT) / 2.0F;
    float x = prompt.area.position.x + PROMPT_PAD;
    if (!prompt.tag.is_empty()) {
      drawing.text(canvas, x, baseline, prompt.tag, PROMPT_SIZE, tones.cyan,
                   Weight::Bold);
      x += drawing.text_width(prompt.tag + String("  "), PROMPT_SIZE,
                              Weight::Bold);
    }
    drawing.text(canvas, x, baseline, prompt.cap, PROMPT_SIZE, tones.lime,
                 Weight::Bold);
    x +=
        drawing.text_width(prompt.cap + String(" "), PROMPT_SIZE, Weight::Bold);
    drawing.text(canvas, x, baseline, prompt.action, PROMPT_SIZE, tones.white,
                 Weight::Bold);
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

/// How many recipes can be cooked now.
std::size_t cookable_count(const SurvivalFrame &frame) {
  const auto &recipes = frame.content.get().companion.workshop.recipes;
  return static_cast<std::size_t>(
      std::ranges::count_if(recipes, [&frame](const auto &recipe) {
        return recipe_ready(frame.view, recipe);
      }));
}

/// How many recipes can be cooked now, of all of them.
String ready_note(const SurvivalFrame &frame) {
  return "READY RECIPES " + number(cookable_count(frame)) + "/" +
         number(frame.content.get().companion.workshop.recipes.size());
}

/// How many ingredient slots the cook panel draws for a dish of `needed`
/// ingredients: the kitchen's own, and at least three.
std::size_t cook_slots(const SurvivalFrame &frame, std::size_t needed) {
  return std::max<std::size_t>(
      {static_cast<std::size_t>(frame.view.workshop.ingredient_slots), needed,
       3});
}

/// The words over the cook bar: how far the cook has come.
String cooking_note(float fill) {
  return "COOKING " +
         number(static_cast<std::uint64_t>(std::lround(fill * 100.0F))) + "%";
}

/// The ingredients not yet identified, in the pack.
std::vector<IngredientId> unidentified(const SurvivalFrame &frame) {
  std::vector<IngredientId> unknown;
  for (const auto &[value, count] : pack_of(frame.view)) {
    if (!identified(frame.view, IngredientId{value})) {
      unknown.push_back(IngredientId{value});
    }
  }
  return unknown;
}

/// A panel of the base's deck, risen into place as the workshop opens.
godot::Rect2 deck_area(const rendering::Art &drawing, float x, float width) {
  return rect(x, DECK_Y + entrance_lift(drawing, "workshop"), width,
              DECK_HEIGHT);
}

/// The kit panel of the deck over `area`, counted as covered, with its title
/// on a tab at the top left.
void deck_panel(godot::Node2D &canvas, const rendering::Art &drawing,
                godot::Rect2 area, const String &title) {
  const auto tones = rendering::palette();
  rendering::plate(canvas, area);
  cover(canvas, area);
  canvas.draw_rect(
      rect(area.position.x + 12.0F, area.position.y + 8.0F,
           drawing.text_width(title, DECK_TITLE_SIZE, Weight::Bold) + 16.0F,
           30.0F),
      tones.background);
  drawing.text(canvas, area.position.x + 20.0F, area.position.y + 31.0F, title,
               DECK_TITLE_SIZE, tones.white, Weight::Title);
}

/// Words that end at the panel's right edge, on the title's line.
void deck_right(godot::Node2D &canvas, const rendering::Art &drawing,
                godot::Rect2 area, const String &text, godot::Color tint) {
  drawing.text(canvas,
               area.get_end().x - DECK_PAD -
                   drawing.text_width(text, DECK_SIZE, Weight::Bold),
               area.position.y + 28.0F, text, DECK_SIZE, tint, Weight::Bold);
}

/// A step longer than a panel's line, split at the space nearest its middle;
/// one line when short.
std::vector<String> halves(const String &text) {
  constexpr std::int64_t LINE = 30;
  const std::int64_t middle = text.length() / 2;
  if (text.length() <= LINE) {
    return {text};
  }
  std::int64_t at = -1;
  for (std::int64_t index = 0; index < text.length(); ++index) {
    if (text.unicode_at(index) == U' ' &&
        (at < 0 || std::llabs(index - middle) < std::llabs(at - middle))) {
      at = index;
    }
  }
  if (at < 0) {
    return {text};
  }
  return {text.substr(0, at), text.substr(at + 1)};
}

/// The ingredients of the dish the chef's key would cook; none when no dish
/// is offered.
std::vector<IngredientId> offered_ingredients(const SurvivalFrame &frame) {
  const auto offered = offered_dish(frame);
  if (!offered) {
    return {};
  }
  const auto &recipes = frame.content.get().companion.workshop.recipes;
  const auto recipe = std::ranges::find_if(recipes, [&](const auto &entry) {
    return entry.food.value == offered->value;
  });
  return recipe == recipes.end() ? std::vector<IngredientId>()
                                 : recipe->ingredients;
}

/// Where the appraise panel's slots stand for `count` ingredients.
godot::Rect2 appraisal_area(const rendering::Art &drawing, std::size_t count) {
  const auto panel = deck_area(drawing, APPRAISE_X, APPRAISE_WIDTH);
  return rect(panel.position.x + DECK_PAD, panel.position.y + DECK_SLOTS_Y,
              static_cast<float>(count) * DECK_SLOT, DECK_SLOT);
}

/// The serve button of the feed panel.
godot::Rect2 serve_area(const rendering::Art &drawing) {
  const auto panel = deck_area(drawing, FEED_X, FEED_WIDTH);
  return rect(panel.get_end().x - DECK_PAD - SERVE_WIDTH,
              panel.position.y + DECK_SLOTS_Y, SERVE_WIDTH, SERVE_HEIGHT);
}

/// The appraise panel: the ingredients not yet identified in slots, the one
/// its key would appraise ringed, and who works here.
void paint_appraise(godot::Node2D &canvas, const rendering::Art &drawing,
                    const SurvivalFrame &frame) {
  const auto tones = rendering::palette();
  const auto panel = deck_area(drawing, APPRAISE_X, APPRAISE_WIDTH);
  deck_panel(canvas, drawing, panel, "APPRAISE");
  const bool human =
      survival::key_set(frame.control, Actor::Appraiser).has_value();
  deck_right(canvas, drawing, panel, player_tag(frame, Actor::Appraiser),
             human ? tones.cyan : tones.muted);
  const auto unknown = unidentified(frame);
  const auto pack = pack_of(frame.view);
  const auto &offer = frame.offers.at(index_of(Actor::Appraiser)).interact;
  const auto *next = offer ? std::get_if<survival::Appraise>(&*offer) : nullptr;
  const auto slots = appraisal_area(drawing, unknown.size());
  float left = slots.position.x;
  for (const auto ingredient : unknown) {
    const auto area = rect(left, slots.position.y, DECK_SLOT, DECK_SLOT);
    rendering::card(canvas, area,
                    next != nullptr && next->ingredient == ingredient
                        ? rendering::State::Focus
                        : rendering::State::Default);
    paint_item(canvas, drawing, survival::ingredient_name(ingredient),
               area.position + Vector2(-2.0F, -2.0F));
    chip(canvas, drawing, left + DECK_SLOT - 14.0F, slots.position.y + 16.0F,
         "?", SMALL_SIZE, tones.lime);
    const String quantity = number(pack.at(ingredient.value));
    chip(canvas, drawing,
         left + DECK_SLOT - drawing.text_width(quantity, SMALL_SIZE, CHANGING) -
             4.0F,
         slots.position.y + DECK_SLOT - 4.0F, quantity, SMALL_SIZE,
         tones.white);
    left += DECK_SLOT;
  }
  const float words = panel.position.x + DECK_PAD;
  if (unknown.empty()) {
    drawing.text(canvas, words, panel.position.y + DECK_SLOTS_Y + 32.0F,
                 NOTHING_TO_APPRAISE, DECK_SIZE, tones.faint, CHANGING);
  } else if (!human) {
    drawing.text(canvas, words,
                 panel.position.y + DECK_NOTE_Y + DECK_NOTE_STEP + 4.0F,
                 AI_APPRAISES, DECK_SIZE, tones.muted, CHANGING);
  }
}

/// The cook panel: the dish on offer in its ingredient slots, how many dishes
/// can be cooked, and the step to take now.
void paint_cook(godot::Node2D &canvas, const rendering::Art &drawing,
                const SurvivalFrame &frame) {
  const auto tones = rendering::palette();
  const auto panel = deck_area(drawing, COOK_X, COOK_WIDTH);
  deck_panel(canvas, drawing, panel, "COOK");
  if (survival::key_set(frame.control, Actor::Chef)) {
    deck_right(canvas, drawing, panel, NEXT_DISH, tones.cyan);
  }
  const float left = panel.position.x + DECK_PAD;
  const float top = panel.position.y + DECK_SLOTS_Y;
  const auto &cooking = frame.view.workshop.cooking;
  // The slots hold the ingredients being cooked, or those of the dish on offer.
  const auto ingredients =
      cooking ? cooking->slots : offered_ingredients(frame);
  const auto count = cook_slots(frame, ingredients.size());
  const auto pack = pack_of(frame.view);
  for (std::size_t index = 0; index < count; ++index) {
    const auto area =
        rect(left + static_cast<float>(index) * (DECK_SLOT + 4.0F), top,
             DECK_SLOT, DECK_SLOT);
    rendering::card(canvas, area, rendering::State::Default);
    if (index < ingredients.size()) {
      paint_item(canvas, drawing,
                 survival::ingredient_name(ingredients.at(index)),
                 area.position + Vector2(-2.0F, -2.0F));
      const auto available = pack.find(ingredients.at(index).value);
      std::uint32_t amount = 1U;
      if (!cooking) {
        amount = available == pack.end() ? 0U : available->second;
      }
      const String quantity = number(amount);
      chip(canvas, drawing,
           area.get_end().x -
               drawing.text_width(quantity, SMALL_SIZE, CHANGING) - 4.0F,
           area.get_end().y - 4.0F, quantity, SMALL_SIZE, tones.white);
    }
  }
  const float column =
      left + static_cast<float>(count) * (DECK_SLOT + 4.0F) + 8.0F;
  const auto offered = offered_dish(frame);
  String name = NO_DISH_OFFERED;
  if (cooking) {
    name = from(survival::food_name(cooking->food));
  } else if (offered) {
    name = from(survival::food_name(*offered));
  }
  drawing.text(canvas, column, top + 22.0F, name, DECK_SIZE,
               cooking || offered ? tones.white : tones.faint, CHANGING);
  if (cooking) {
    const float fill = std::clamp(survival::progress_of(*cooking), 0.0F, 1.0F);
    drawing.text(canvas, column, top + 40.0F, cooking_note(fill), DECK_SIZE,
                 tones.lime, CHANGING);
    rendering::progress(
        canvas,
        rect(column, top + 44.0F, panel.get_end().x - DECK_PAD - column, 8.0F),
        fill);
  } else {
    drawing.text(canvas, column, top + 46.0F, ready_note(frame), DECK_SIZE,
                 tones.muted, CHANGING);
  }
  float y = panel.position.y + DECK_NOTE_Y + 4.0F;
  for (const auto &line : halves(next_step(frame))) {
    drawing.text(canvas, left, y, line, DECK_SIZE, tones.white, CHANGING);
    y += DECK_NOTE_STEP;
  }
}

/// The feed panel: the stored dishes with their scores, what feeding asks,
/// the serve button and the way to end the day unfed.
void paint_feed(godot::Node2D &canvas, const rendering::Art &drawing,
                const SurvivalFrame &frame) {
  const auto tones = rendering::palette();
  const auto &workshop = frame.view.workshop;
  const auto &tuning = frame.content.get().companion.workshop;
  const auto panel = deck_area(drawing, FEED_X, FEED_WIDTH);
  deck_panel(canvas, drawing, panel, "FEED");
  deck_right(canvas, drawing, panel, "NEEDS " + number(tuning.satisfying_score),
             tones.lime);
  const float left = panel.position.x + DECK_PAD;
  const float top = panel.position.y + DECK_SLOTS_Y;
  const auto serve = serve_area(drawing);
  const float room = serve.position.x - GAP - left;
  if (workshop.foods.empty()) {
    drawing.text(canvas, left, top + 32.0F, NO_DISH, DECK_SIZE, tones.faint,
                 CHANGING);
  }
  const float pitch = std::min(
      ICON + 8.0F, (room - ICON) / static_cast<float>(std::max<std::size_t>(
                                       workshop.foods.size() - 1, 1)));
  float x = left;
  for (const auto &dish : workshop.foods) {
    paint_item(canvas, drawing, survival::food_name(dish.food),
               Vector2(x, top - 2.0F));
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
         top + 50.0F, label, SMALL_SIZE,
         total >= tuning.satisfying_score ? tones.lime : tones.white);
    x += pitch;
  }
  const bool ready = feeder(frame).has_value();
  if (ready) {
    rendering::button(canvas, serve, rendering::State::Default);
  } else {
    rendering::card(canvas, serve, rendering::State::Default);
  }
  const auto words = static_cast<float>(DECK_TITLE_SIZE);
  drawing.text(canvas,
               serve.position.x +
                   (serve.size.x -
                    drawing.text_width(SERVE, DECK_TITLE_SIZE, Weight::Bold)) /
                       2.0F,
               serve.position.y + (serve.size.y + words * CAP_HEIGHT) / 2.0F,
               SERVE, DECK_TITLE_SIZE, ready ? tones.ink : tones.faint,
               Weight::Bold);
  drawing.text(canvas, left,
               panel.position.y + DECK_NOTE_Y + DECK_NOTE_STEP + 4.0F,
               end_day_job(frame), DECK_SIZE, tones.muted, CHANGING);
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
  const float top = RESULT_TOP;
  const auto area = rect(RESULT_X, top, RESULT_WIDTH, height);
  rendering::plate(canvas, area);
  cover(canvas, area);
  float y = top + 10.0F + static_cast<float>(RESULT_SIZE);
  bool headline = true;
  for (const auto &line : lines) {
    drawing.text(canvas, RESULT_X + INSET, y, line, RESULT_SIZE,
                 headline ? tones.lime : tones.white, CHANGING);
    headline = false;
    y += RESULT_LINE_HEIGHT;
  }
}

/// How far the feed crane has lowered its dish: 0 at rest, 1 at the monster's
/// mouth, rising and falling over the reaction to a dish that was fed.
float crane_lowering(const std::vector<rendering::Burst> &bursts) {
  float lowering = 0.0F;
  for (const auto &burst : bursts) {
    if (burst.kind != rendering::Flourish::Delight &&
        burst.kind != rendering::Flourish::Disgust) {
      continue;
    }
    const float done = burst.age / rendering::lifetime(burst.kind);
    lowering = std::max(lowering, 1.0F - std::abs(2.0F * done - 1.0F));
  }
  return lowering;
}

/// The dish the cradle carries: the dish that was fed for as long as the crane
/// is lowered for it, otherwise the stored dish the next feed would serve.
std::optional<std::string_view> cradle_dish(const SurvivalFrame &frame,
                                            float lowering) {
  const auto &workshop = frame.view.workshop;
  if (lowering > 0.0F && workshop.last_day && workshop.last_day->fed) {
    return survival::food_name(workshop.last_day->fed->dish.food);
  }
  if (!workshop.foods.empty()) {
    return survival::food_name(workshop.foods.front().food);
  }
  return std::nullopt;
}

/// What fills a layer of the base scene: a picture under `art_hd/`, or the
/// stored dish drawn with the item renderer.
enum class Fill : std::uint8_t { Picture, StoredDish };

/// One layer of the base scene, in 1920×1080 canvas pixels. The feed's travel
/// moves a layer by `follows` times that travel and grows its height by
/// `stretches` times it, so a fixed layer has both at zero.
struct BaseLayer {
  std::string_view name;
  Fill fill;
  std::string_view picture;
  Vector2 at;
  Vector2 size;
  Vector2 follows;
  float stretches;
  /// Whether the layer belongs to the feed crane, whose bounds are reported.
  bool crane;
};

constexpr Vector2 NO_MOTION(0.0F, 0.0F);
constexpr Vector2 DOWN(0.0F, 1.0F);
constexpr Vector2 ACTOR_SIZE(256.0F, 224.0F);
/// How far the cradle and its dish go down when fully lowered, in canvas
/// pixels: 140 layout units.
constexpr float CRANE_TRAVEL = 140.0F * rendering::ART_SCALE;
/// The stored dish's icon in canvas pixels, with its centre at x1666 and its
/// bottom at y685 when the cradle is at rest.
constexpr float DISH_SIZE = ICON * rendering::ART_SCALE;

/// The base scene from back to front. The interface is drawn over all of it.
constexpr std::array BASE_LAYERS{
    BaseLayer{.name = "background",
              .fill = Fill::Picture,
              .picture = "base/kitchen.png",
              .at = Vector2(0.0F, 0.0F),
              .size = Vector2(1920.0F, 1080.0F),
              .follows = NO_MOTION,
              .stretches = 0.0F,
              .crane = false},
    BaseLayer{.name = "appraiser",
              .fill = Fill::Picture,
              .picture = "characters/appraiser_idle.png",
              .at = Vector2(225.0F, 588.0F),
              .size = ACTOR_SIZE,
              .follows = NO_MOTION,
              .stretches = 0.0F,
              .crane = false},
    BaseLayer{.name = "chef",
              .fill = Fill::Picture,
              .picture = "characters/chef_idle.png",
              .at = Vector2(780.0F, 588.0F),
              .size = ACTOR_SIZE,
              .follows = NO_MOTION,
              .stretches = 0.0F,
              .crane = false},
    BaseLayer{.name = "front",
              .fill = Fill::Picture,
              .picture = "base/kitchen_front.png",
              .at = Vector2(0.0F, 0.0F),
              .size = Vector2(1920.0F, 1080.0F),
              .follows = NO_MOTION,
              .stretches = 0.0F,
              .crane = false},
    BaseLayer{.name = "mast",
              .fill = Fill::Picture,
              .picture = "base/feed_crane.png",
              .at = Vector2(1240.0F, 150.0F),
              .size = Vector2(660.0F, 642.0F),
              .follows = NO_MOTION,
              .stretches = 0.0F,
              .crane = true},
    BaseLayer{.name = "cable",
              .fill = Fill::Picture,
              .picture = "base/feed_crane_cable.png",
              .at = Vector2(1650.0F, 275.0F),
              .size = Vector2(40.0F, 230.0F),
              .follows = NO_MOTION,
              .stretches = 1.0F,
              .crane = true},
    BaseLayer{.name = "cradle",
              .fill = Fill::Picture,
              .picture = "base/feed_crane_cradle.png",
              .at = Vector2(1530.0F, 505.0F),
              .size = Vector2(280.0F, 220.0F),
              .follows = DOWN,
              .stretches = 0.0F,
              .crane = true},
    BaseLayer{.name = "dish",
              .fill = Fill::StoredDish,
              .picture = "",
              .at = Vector2(1666.0F - DISH_SIZE / 2.0F, 685.0F - DISH_SIZE),
              .size = Vector2(DISH_SIZE, DISH_SIZE),
              .follows = DOWN,
              .stretches = 0.0F,
              .crane = false},
};

/// Draws the base scene beneath its interface: every layer of `BASE_LAYERS` in
/// order, moved and stretched by how far the feed has lowered the crane.
/// Leaves each drawn layer's name and area as the `base_layers` meta, in draw
/// order, and the crane's bounds as `feed_crane`.
void paint_workshop_art(godot::Node2D &canvas, const rendering::Art &drawing,
                        const SurvivalFrame &frame, float lowering) {
  canvas.set_meta("character_layers", godot::Array());
  const float travel = std::clamp(lowering, 0.0F, 1.0F) * CRANE_TRAVEL;
  godot::Array drawn;
  godot::Rect2 crane;
  bool crane_started = false;
  const auto dish = cradle_dish(frame, lowering);
  for (const auto &layer : BASE_LAYERS) {
    if (layer.fill == Fill::StoredDish && !dish) {
      continue;
    }
    const auto area =
        godot::Rect2((layer.at + layer.follows * travel) / rendering::ART_SCALE,
                     (layer.size + Vector2(0.0F, layer.stretches * travel)) /
                         rendering::ART_SCALE);
    if (layer.fill == Fill::Picture) {
      paint_hd_art(canvas, from(layer.picture), area);
    } else if (dish) {
      paint_item(canvas, drawing, *dish, area.position);
    }
    godot::Dictionary note;
    note.set("name", from(layer.name));
    note.set("area", area);
    drawn.push_back(note);
    if (layer.crane) {
      crane = crane_started ? crane.merge(area) : area;
      crane_started = true;
    }
  }
  canvas.set_meta("base_layers", drawn);
  canvas.set_meta("feed_crane", crane);
}

void paint_workshop_screen(godot::Node2D &canvas, const rendering::Art &drawing,
                           const SurvivalFrame &frame,
                           const std::vector<rendering::Burst> &bursts) {
  paint_workshop_art(canvas, drawing, frame, crane_lowering(bursts));
  paint_day_result(canvas, drawing, frame);
  paint_appraise(canvas, drawing, frame);
  paint_cook(canvas, drawing, frame);
  paint_feed(canvas, drawing, frame);
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
    return "Close the search first.";
  case survival::Result::Full:
    return "Your slots are full.";
  case survival::Result::NotAtDevice:
    return "The device is out of reach.";
  case survival::Result::TooEarly:
    return "The device is not ready yet.";
  case survival::Result::MissingIngredients:
    return "The device needs three parts.";
  case survival::Result::InvalidElapsed:
  case survival::Result::NotInWorkshop:
  case survival::Result::RunOver:
    return "That cannot be done now.";
  }
  std::abort();
}

String refusal_text(survival::CollectionResult result) {
  switch (result) {
  case survival::CollectionResult::Applied:
    return "";
  case survival::CollectionResult::Occupied:
    return "The other actor is searching it.";
  case survival::CollectionResult::Unavailable:
    return "That container is gone.";
  case survival::CollectionResult::OutOfRange:
    return "Move closer.";
  case survival::CollectionResult::Busy:
    return "Close the search first.";
  case survival::CollectionResult::InventoryFull:
    return "Your slots are full.";
  case survival::CollectionResult::ContainerFull:
    return "That container is full.";
  case survival::CollectionResult::MissingItem:
    return "It is no longer there.";
  case survival::CollectionResult::NotSearching:
  case survival::CollectionResult::NotExploring:
  case survival::CollectionResult::UnknownSource:
    return "That cannot be done now.";
  }
  std::abort();
}

String refusal_text(survival::WorkshopResult result, const GameView &view) {
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
  case survival::WorkshopResult::Full:
    return view.workshop.cooking ? "A dish is still cooking."
                                 : "The fridge is full.";
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
        } else if constexpr (std::is_same_v<Done, survival::Search>) {
          return who + String(" searched ") +
                 rendering::place_name(view, action.source) + ".";
        } else if constexpr (std::is_same_v<Done, survival::CloseSearch>) {
          return who + String(" closed the search.");
        } else if constexpr (std::is_same_v<Done, survival::Take>) {
          return who + String(" took ") +
                 from(survival::ingredient_name(action.ingredient)) + ".";
        } else if constexpr (std::is_same_v<Done, survival::Put>) {
          return who + String(" put back ") +
                 from(survival::ingredient_name(action.ingredient)) + ".";
        } else if constexpr (std::is_same_v<Done, survival::Store>) {
          return who + String(" stored ") +
                 from(survival::ingredient_name(action.ingredient)) +
                 " in the fridge.";
        } else if constexpr (std::is_same_v<Done, survival::Withdraw>) {
          return who + String(" took ") +
                 from(survival::ingredient_name(action.ingredient)) +
                 " from the fridge.";
        } else if constexpr (std::is_same_v<Done, survival::Appraise>) {
          return from(survival::ingredient_name(action.ingredient)) +
                 (failed ? String(" appraisal failed: try again.")
                         : String(" identified."));
        } else if constexpr (std::is_same_v<Done, survival::Cook>) {
          return from(survival::food_name(action.food)) +
                 (failed ? String(" burned: ingredients lost.")
                         : String(" cooked."));
        } else if constexpr (std::is_same_v<Done, survival::UseDevice>) {
          return who + String(" used the escape device.");
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
  rows.push_back(row("Search or open the inventory", one.search, two.search));
  rows.push_back(
      row("Appraise, cook or move an item", one.interact, two.interact));
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

/// One object for each place name, over every map; a line names the place, so
/// objects that share a name print the same lines.
std::vector<survival::ObjectId> named_once(const SurvivalFrame &frame) {
  const auto &view = frame.view;
  std::vector<String> names;
  std::vector<survival::ObjectId> objects;
  for (const auto &object : frame.content.get().objects) {
    auto name = rendering::place_name(view, object.id);
    if (std::ranges::find(names, name) == names.end()) {
      names.push_back(std::move(name));
      objects.push_back(object.id);
    }
  }
  return objects;
}

/// The name and the one-line role of one choice of players.
std::array<String, 2> setup_label(SetupEntry entry) {
  switch (entry) {
  case SetupEntry::Chef:
    return {"Chef", "You cook. AI appraises."};
  case SetupEntry::Appraiser:
    return {"Appraiser", "You appraise. AI cooks."};
  case SetupEntry::TwoPlayers:
    return {"Two players", "Play together on one keyboard."};
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
  paint_title_art(canvas, 0.0F);
  rendering::fade(canvas,
                  rect(SETUP_PANEL_X - 120.0F, 0.0F, 120.0F, SCREEN_HEIGHT));
  paint_panel(canvas, rect(SETUP_PANEL_X, 0.0F, SCREEN_WIDTH - SETUP_PANEL_X,
                           SCREEN_HEIGHT));
  drawing.text(canvas, SETUP_X, SETUP_PANEL_Y + 60.0F, "Who is playing", 44,
               tones.white, Weight::Title);
  drawing.text(
      canvas, SETUP_X + drawing.text_width("Who is playing", 44, Weight::Title),
      SETUP_PANEL_Y + 60.0F, "?", 44, tones.lime, Weight::Title);
  canvas.draw_rect(rect(SETUP_X, SETUP_PANEL_Y + 76.0F, SETUP_WIDTH, 5.0F),
                   tones.lime);
  const auto chosen_at = static_cast<float>(
      std::ranges::find(SETUP_ENTRIES, selected) - SETUP_ENTRIES.begin());
  const float cursor =
      SETUP_Y + drawing.glide("setup_cursor", chosen_at) * SETUP_PITCH;
  for (std::size_t index = 0; index < SETUP_ENTRIES.size(); ++index) {
    paint_card(canvas, setup_area(index), false);
  }
  // The gold button slides to the cursor's entry.
  paint_card(canvas, rect(SETUP_X, cursor, SETUP_WIDTH, SETUP_HEIGHT), true);
  float y = SETUP_Y;
  std::size_t number_key = 1;
  const auto figure = [&](rendering::Image image, float x, float top) {
    const float scale = SETUP_FIGURE_HEIGHT / drawing.size(image).y;
    drawing.image(canvas, image, Vector2(x, top), scale);
  };
  for (const SetupEntry entry : SETUP_ENTRIES) {
    const bool chosen = entry == selected;
    const auto label = setup_label(entry);
    const float top = y + (SETUP_HEIGHT - SETUP_FIGURE_HEIGHT) / 2.0F;
    if (entry == SetupEntry::TwoPlayers) {
      figure(rendering::Image::ChefIdle, SETUP_X + 64.0F, top);
      figure(rendering::Image::AppraiserIdle, SETUP_X + 104.0F, top);
    } else {
      figure(entry == SetupEntry::Chef ? rendering::Image::ChefIdle
                                       : rendering::Image::AppraiserIdle,
             SETUP_X + 72.0F, top);
    }
    drawing.text(canvas, SETUP_X + 160.0F, y + 46.0F, label.at(0), 32,
                 chosen ? tones.ink : tones.white, Weight::Title);
    drawing.text(canvas, SETUP_X + 160.0F, y + 78.0F, label.at(1), 16,
                 chosen ? tones.ink : tones.muted);
    rendering::key_cap(
        canvas, drawing,
        rect(SETUP_X + 20.0F, y + (SETUP_HEIGHT - 36.0F) / 2.0F, 36.0F, 36.0F),
        number(number_key),
        chosen ? rendering::State::Focus : rendering::State::Default);
    y += SETUP_PITCH;
    ++number_key;
  }
  drawing.text(canvas, SETUP_X, SETUP_Y + 3.0F * SETUP_PITCH + 24.0F,
               "Click who is playing.", 16, tones.muted);
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
  caption(canvas, drawing, PAUSE_TEXT_X, top, VOLUME_KEYS, tones.lime);
  float y = top + VOLUME_ROW_HEIGHT;
  for (std::size_t index = 0; index < volumes.size(); ++index) {
    const auto &row = volumes.at(index);
    const godot::Color lit = row.marked ? tones.lime : tones.cyan;
    if (row.marked) {
      canvas.draw_rect(rect(PAUSE_TEXT_X - 10.0F, y - VOLUME_ROW_HEIGHT + 10.0F,
                            KEY_ROWS_WIDTH, VOLUME_ROW_HEIGHT),
                       tones.raised);
    }
    drawing.text(canvas, PAUSE_TEXT_X, y, row.name, PAUSE_ROW_SIZE,
                 row.marked ? tones.lime : tones.white);
    const float blocks = PAUSE_TEXT_X + VOLUME_BLOCKS_X;
    for (std::uint8_t step = 0; step < row.steps; ++step) {
      const auto block = volume_block(index, step, lift);
      rendering::card(canvas, block, rendering::State::Default);
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
  caption(canvas, drawing, BOOK_X, top - 24.0F, "RECIPE BOOK", tones.lime);
  caption(canvas, drawing,
          BOOK_RIGHT - drawing.text_width("TASTE", SMALL_SIZE, CHANGING),
          top - 24.0F, "TASTE", tones.lime);
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
                 mark == "hated" ? tones.magenta : tones.muted, CHANGING);
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
  caption(canvas, drawing, BOOK_X, y, "FEEDING", tones.lime);
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

/// The pause screen's subtitle, with what ends the run well.
String pause_line(const std::array<String, 3> &text) {
  return text.at(1) + String(" ") + ending_goal() + String(".");
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
  const auto heading = rendering::aim(frame.content.get(), frame.view);
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
                          frame.control, shot.out_of_reach);
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
  canvas.set_meta("interface_stamps", godot::Array());
  drawing.shift(canvas, {});
  const bool city = frame.view.progression.phase == Phase::Exploration;
  if (city) {
    paint_prompts(canvas, drawing, frame);
    paint_goal(canvas, drawing, {}, hand_off_notice(frame));
  } else {
    paint_workshop_screen(canvas, drawing, frame, bursts);
    rendering::paint_workshop_bursts(canvas, drawing, bursts);
    // The task card names the step at base; the notice line says what the
    // keys cannot do there.
    paint_goal(canvas, drawing, BASE_NOTICE, hand_off_notice(frame));
  }
  paint_transfers(canvas, drawing, frame);
  paint_corners(canvas, drawing, frame);
  if (city) {
    // The map's words are placed once the HUD has taken its place, clear of
    // it.
    std::vector<godot::Rect2> covered;
    add_areas(covered, canvas, "hud_panels");
    drawing.shift(canvas, {});
    rendering::paint_city_labels(canvas, drawing, frame.view, frame.control,
                                 shot, covered);
    drawing.shift(canvas, {});
  }
  if (tip && !city) {
    paint_tip(canvas, drawing, frame, *tip);
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
    rendering::plate(canvas, rect(PAUSE_BOX_X, PAUSE_BOX_Y + lift,
                                  PAUSE_BOX_WIDTH, PAUSE_BOX_HEIGHT));
    drawing.text(canvas, PAUSE_TEXT_X, PAUSE_BOX_Y + lift + 70.0F, text.at(0),
                 INTERRUPTION_TITLE_SIZE, tones.white, Weight::Title);
    drawing.text(canvas, PAUSE_TEXT_X, PAUSE_BOX_Y + lift + 104.0F,
                 pause_line(text), PAUSE_ROW_SIZE, tones.muted, CHANGING);
    caption(canvas, drawing, PAUSE_TEXT_X, PAUSE_ROWS_Y + lift - 24.0F, "KEYS",
            tones.lime);
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
  rendering::plate(canvas, rect(NOTICE_BOX_X, NOTICE_BOX_Y + lift,
                                NOTICE_BOX_WIDTH, NOTICE_BOX_HEIGHT));
  drawing.text(canvas, NOTICE_TEXT_X, NOTICE_BOX_Y + lift + 70.0F, text.at(0),
               INTERRUPTION_TITLE_SIZE, tones.white, Weight::Title);
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

namespace {

const char *ending_title(std::optional<survival::Ending> ending) {
  if (!ending.has_value()) {
    std::abort();
  }
  switch (*ending) {
  case survival::Ending::MonsterSatisfied:
    return "Monster satisfied";
  case survival::Ending::VomitedOut:
    return "Vomited out";
  case survival::Ending::Escaped:
    return "Escaped";
  }
  std::abort();
}

String ending_line(std::optional<survival::Ending> ending, std::uint64_t days) {
  if (!ending.has_value()) {
    std::abort();
  }
  switch (*ending) {
  case survival::Ending::MonsterSatisfied:
    return "The monster has had enough after " + number(days) +
           String(" days. Your feeding is over.");
  case survival::Ending::VomitedOut:
    return "After " + number(days) +
           String(" days, the monster spat the party back into the night.");
  case survival::Ending::Escaped:
    return "The party mended the roof device and escaped after " +
           number(days) + String(" days.");
  }
  std::abort();
}

const char *ending_image(std::optional<survival::Ending> ending) {
  if (!ending.has_value()) {
    std::abort();
  }
  switch (*ending) {
  case survival::Ending::MonsterSatisfied:
    return "res://levels/monster_mouth_escape/art_hd/endings/satisfied.png";
  case survival::Ending::VomitedOut:
    return "res://levels/monster_mouth_escape/art_hd/endings/vomited_out.png";
  case survival::Ending::Escaped:
    return "res://levels/monster_mouth_escape/art_hd/endings/escaped.png";
  }
  std::abort();
}

/// Draws the ending's picture at the screen's width from the top, its aspect
/// kept, and records where, so a check can read it. A missing picture aborts.
void paint_ending_art(godot::Node2D &canvas, const char *path) {
  const godot::Ref<godot::Texture2D> art =
      godot::ResourceLoader::get_singleton()->load(path);
  CRASH_COND_MSG(art.is_null(), "Ending art must load");
  // Keep the texture alive until Godot renders the canvas commands.
  canvas.set_meta("ending_art", art);
  const auto size = art->get_size();
  const auto area =
      rect(0.0F, 0.0F, SCREEN_WIDTH, SCREEN_WIDTH * size.y / size.x);
  canvas.draw_texture_rect(art, area, false);
  canvas.set_meta("ending_image", path);
  canvas.set_meta("ending_picture", area);
}

/// The band under the picture: a gold rule, the title on one line and the
/// description under it, with the Main menu button at the right.
void paint_end_band(godot::Node2D &canvas, const rendering::Art &drawing,
                    const char *title, const String &line) {
  const auto tones = rendering::palette();
  canvas.draw_rect(
      rect(0.0F, END_BAND_Y, SCREEN_WIDTH, SCREEN_HEIGHT - END_BAND_Y),
      tones.background);
  canvas.draw_rect(rect(0.0F, END_BAND_Y, SCREEN_WIDTH, 4.0F), tones.lime);
  const String words(title);
  const auto last = words.rfind(" ") + 1;
  const String lead = words.substr(0, last);
  drawing.text(canvas, END_TEXT_X, 614.0F, lead, 48, tones.white,
               Weight::Title);
  drawing.text(canvas, END_TEXT_X + drawing.text_width(lead, 48, Weight::Title),
               614.0F, words.substr(last), 48, tones.lime, Weight::Title);
  drawing.text(canvas, END_TEXT_X, 650.0F, line, 24, tones.white);
  paint_button(canvas, drawing, end_button(drawing), END_LEAVE,
               INTERRUPTION_LINE_SIZE, false);
}

} // namespace

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
  drawing.shift(canvas, {});
  if (cinematic == Cinematic::Ending) {
    paint_ending_art(canvas, ending_image(progression.ending));
    paint_end_band(canvas, drawing, ending_title(progression.ending),
                   ending_line(progression.ending, progression.day - 1));
    return;
  }
  if (cinematic == Cinematic::Loss) {
    paint_ending_art(canvas, "res://levels/monster_mouth_escape/art_hd/"
                             "endings/acid_failure.png");
    paint_end_band(canvas, drawing, "Dissolved",
                   "The acid filled the monster's mouth after " +
                       number(progression.day - 1) + String(" days."));
    return;
  }
  rendering::paint_mouth(canvas, drawing, acid_ratio(frame),
                         rendering::whole_scene());
  drawing.shift(canvas, {});
  canvas.draw_rect(rect(0.0F, 0.0F, SCREEN_WIDTH, SCREEN_HEIGHT),
                   color(14, 12, 26, 0.6F));
  drawing.image(canvas, rendering::Image::ChefIdle,
                godot::Vector2(130.0F, 300.0F), 2.0F);
  drawing.image(canvas, rendering::Image::AppraiserIdle,
                godot::Vector2(280.0F, 300.0F), 2.0F);
  rendering::plate(canvas, rect(100.0F, 440.0F, 1080.0F, 225.0F));
  drawing.text(canvas, 130.0F, 170.0F, "SWALLOWED", 48, tones.white,
               Weight::Bold);
  drawing.text(canvas, 130.0F, 230.0F,
               "A monster ate the city. Keep it fed, and look for a way out.",
               24, tones.white);
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
    return "Search a container with the search key: move food between it and "
           "your slots. The clock runs on.";
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
  case Tip::Ending:
    return "Days fed to satisfaction add up, in a row or not: after " +
           number(survival::TEMPORARY_ENDING.satisfied_days) +
           " of them the monster is done with the party.";
  }
  std::abort();
}

String tip_line(const SurvivalFrame &frame, Tip tip) {
  String text = tip_text(tip);
  if (tip != Tip::SetOut) {
    return text;
  }
  const auto players = humans(frame);
  if (players.size() == 1) {
    const auto &keys = KEY_LABELS.at(players.front().second);
    return text + String(" Set out with [") + from(keys.travel) + "].";
  }
  if (players.size() == 2) {
    const auto &first = KEY_LABELS.at(players.front().second);
    const auto &second = KEY_LABELS.at(players.back().second);
    return text + String(" Set out with [") + from(first.travel) + "] (P" +
           number(players.front().second + 1) + ") or [" + from(second.travel) +
           "] (P" + number(players.back().second + 1) + ").";
  }
  return text;
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
  case Tip::Ending:
    return "ending";
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
      caption(canvas, drawing, left + KEY_FIRST, y, row.first, tones.lime);
      caption(canvas, drawing, left + KEY_SECOND, y, row.second, tones.lime);
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
  if (view.progression.phase == Phase::Ended) {
    return ending_line(view.progression.ending, view.progression.day - 1);
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

String ending_goal() {
  return "Satisfy the monster on " +
         number(survival::TEMPORARY_ENDING.satisfied_days) + " days";
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
      add_walkable(cells, view.world, view.exploration.obstacles,
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
    if (stock.items.empty() || object == objects.end() ||
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
  const auto places = named_once(frame);
  std::vector<FittedText> lines;
  // The NEXT step is the cook panel's at base; the city draws none.
  if (frame.view.progression.phase == Phase::Workshop) {
    for (const auto &line : halves(next_step(frame))) {
      lines.push_back(fitted(line, DECK_SIZE, DECK_INNER));
    }
  }
  lines.push_back(fitted(BASE_NOTICE, GOAL_SIZE, goal_inner));
  lines.push_back(fitted(hand_off_notice(frame), HAND_OFF_SIZE, goal_inner));
  for (const auto &line : day_result(frame)) {
    lines.push_back(fitted(line, RESULT_SIZE, RESULT_WIDTH - 2.0F * INSET));
  }
  // The action prompt: every place its key can act on, the close of the
  // screen it opened, and what the base's keys do.
  const float prompt = PROMPT_MAX_WIDTH - 2.0F * PROMPT_PAD - PROMPT_KEYS_WIDTH;
  for (const char *label :
       {"CLOSE", "SEARCH Fridge", "COOK", "APPRAISE", "USE DEVICE"}) {
    lines.push_back(fitted(label, PROMPT_SIZE, prompt));
  }
  for (const auto object : places) {
    lines.push_back(
        fitted("SEARCH " + rendering::place_name(frame.view, object),
               PROMPT_SIZE, prompt));
  }
  // The base's deck.
  lines.push_back(fitted(end_day_job(frame), DECK_SIZE, DECK_INNER));
  lines.push_back(fitted(ready_note(frame), DECK_SIZE, DECK_INNER / 2.0F));
  lines.push_back(fitted(NO_DISH, DECK_SIZE, DECK_INNER));
  lines.push_back(fitted(NO_DISH_OFFERED, DECK_SIZE, DECK_INNER / 2.0F));
  lines.push_back(fitted(cooking_note(1.0F), DECK_SIZE, DECK_INNER / 2.0F));
  lines.push_back(fitted(NOTHING_TO_APPRAISE, DECK_SIZE, DECK_INNER));
  lines.push_back(fitted(AI_APPRAISES, DECK_SIZE, DECK_INNER));
  const auto &tuning = frame.content.get().companion.workshop;
  for (const auto &recipe : tuning.recipes) {
    // The dish's name stands right of its slots on the cook panel.
    const auto slots = cook_slots(frame, recipe.ingredients.size());
    lines.push_back(fitted(
        from(survival::food_name(recipe.food)), DECK_SIZE,
        DECK_INNER - static_cast<float>(slots) * (DECK_SLOT + 4.0F) - 8.0F));
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
    const auto action = prompt_action(frame, actor);
    labels.push_back(action ? action->text : String());
  }
  return labels;
}

String describe(const GameView &view, const Event &event) {
  return std::visit(
      [&view](const auto &happened) -> String {
        using Happened = std::decay_t<decltype(happened)>;
        if constexpr (std::is_same_v<Happened, ActionEvent>) {
          const String refusal = std::visit(
              [&view](auto result) -> String {
                if constexpr (std::is_same_v<decltype(result),
                                             survival::WorkshopResult>) {
                  return refusal_text(result, view);
                } else {
                  return refusal_text(result);
                }
              },
              happened.result);
          return refusal.is_empty() ? done_text(view, happened) : refusal;
        } else if constexpr (std::is_same_v<Happened, FallEvent>) {
          return String(actor_name(happened.actor)) +
                 String(" fell and washed back to base.");
        } else if constexpr (std::is_same_v<Happened, TouchEvent>) {
          return String(actor_name(happened.actor)) +
                 String(" touched danger and was sent away.");
        } else if constexpr (std::is_same_v<Happened, TimeUpEvent>) {
          return "Time is up: everyone is back at base.";
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
  for (const auto &prompt : prompts(drawing, frame)) {
    targets.push_back(
        {.area = prompt.area,
         .press = PlayerPress{.actor = prompt.actor, .key = prompt.key}});
  }
}

void workshop_targets(std::vector<Target> &targets,
                      const rendering::Art &drawing,
                      const SurvivalFrame &frame) {
  const auto job = workshop_step(frame);
  const auto cook = deck_area(drawing, COOK_X, COOK_WIDTH);
  // The step of the cook panel: the key that moves the base on.
  targets.push_back(
      {.area = rect(cook.position.x, cook.position.y + DECK_NOTE_Y - 8.0F,
                    COOK_WIDTH, DECK_HEIGHT - DECK_NOTE_Y + 8.0F),
       .press = job.press.value_or(PlayerPress{}),
       .refusal = job.press ? String() : String("The AI is working.")});
  if (survival::key_set(frame.control, Actor::Chef)) {
    const auto &recipes = frame.content.get().companion.workshop.recipes;
    const auto ready =
        std::ranges::count_if(recipes, [&frame](const auto &recipe) {
          return recipe_ready(frame.view, recipe);
        });
    const float width = drawing.text_width(NEXT_DISH, DECK_SIZE, Weight::Bold);
    targets.push_back(
        {.area = rect(cook.get_end().x - DECK_PAD - width - 6.0F,
                      cook.position.y + 6.0F, width + 12.0F, 32.0F),
         .press = DishPress{survival::Direction::Down},
         .refusal = ready > 1 ? String() : String("No other dish to choose.")});
  }
  const auto feed = deck_area(drawing, FEED_X, FEED_WIDTH);
  const auto actor = feeder(frame);
  targets.push_back(
      {.area = serve_area(drawing),
       .press = PlayerPress{.actor = actor, .key = PlayerKey::Feed},
       .refusal = actor ? String() : String("No dish can be fed now.")});
  const String end = end_day_job(frame);
  targets.push_back(
      {.area = line_area(drawing, feed.position.x + DECK_PAD,
                         feed.position.y + DECK_NOTE_Y + DECK_NOTE_STEP + 4.0F,
                         end, DECK_SIZE),
       .press = PlayerPress{.actor = first_player(frame),
                            .key = PlayerKey::EndDay}});
  if (survival::key_set(frame.control, Actor::Chef)) {
    const auto &offer = frame.offers.at(index_of(Actor::Chef)).interact;
    String refusal;
    if (frame.screens.at(index_of(Actor::Chef)).open) {
      refusal = CLOSE_FIRST;
    } else if (!offer || !std::holds_alternative<survival::Cook>(*offer)) {
      refusal = "No dish can be cooked now.";
    }
    targets.push_back(
        {.area =
             rect(CHEF_AREA_X, CHEF_AREA_Y, CHEF_AREA_WIDTH, CHEF_AREA_HEIGHT),
         .press = PlayerPress{.actor = Actor::Chef, .key = PlayerKey::Interact},
         .refusal = refusal});
  }
  const auto unknown = unidentified(frame).size();
  if (unknown > 0) {
    const bool human =
        survival::key_set(frame.control, Actor::Appraiser).has_value();
    const auto &offer = frame.offers.at(index_of(Actor::Appraiser)).interact;
    String refusal;
    if (!human) {
      refusal = AI_APPRAISES;
    } else if (frame.screens.at(index_of(Actor::Appraiser)).open) {
      refusal = CLOSE_FIRST;
    } else if (!offer || !std::holds_alternative<survival::Appraise>(*offer)) {
      refusal = "Nothing to appraise now.";
    }
    targets.push_back({.area = appraisal_area(drawing, unknown),
                       .press = PlayerPress{.actor = Actor::Appraiser,
                                            .key = PlayerKey::Interact},
                       .refusal = refusal});
  }
  // The prompts of a human at base open the fridge, or close the screen.
  city_targets(targets, drawing, frame);
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
  // A transfer screen is drawn over the rest, so its slots come later.
  for (const auto &view : transfers(frame)) {
    const auto slots = [&](bool mine, Vector2 origin, std::size_t capacity) {
      for (std::size_t index = 0; index < capacity; ++index) {
        targets.push_back(
            {.area = slot_area(origin, index),
             .press =
                 SlotPress{.actor = view.actor, .mine = mine, .index = index}});
      }
    };
    if (view.has_other) {
      slots(false, view.other_origin, view.other_capacity);
    }
    slots(true, view.mine_origin, view.mine_capacity);
  }
  return targets;
}

std::vector<Target> cinematic_targets(const rendering::Art &drawing,
                                      Cinematic cinematic) {
  if (cinematic != Cinematic::Intro) {
    return {{.area = end_button(drawing), .press = MenuKey::Menu}};
  }
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
