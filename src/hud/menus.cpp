#include "menus.hpp"

#include "../rendering/survival.hpp"

#include <cstddef>
#include <cstdlib>

namespace neocity::hud {
namespace {

using rendering::color;
using rendering::rect;

constexpr float ENTRY_X = 150.0F;
constexpr float ENTRY_Y = 370.0F;
constexpr float ENTRY_HEIGHT = 80.0F;
constexpr float CONTROLS_ROW_Y = 160.0F;
constexpr float CONTROLS_ROW_HEIGHT = 36.0F;
constexpr int CONTROLS_ROW_SIZE = 20;

/// One action and its key for each player, as project.godot binds them.
struct ControlRow {
  const char *action;
  const char *first;
  const char *second;
};

constexpr std::array CONTROL_ROWS{
    ControlRow{.action = "", .first = "Player 1", .second = "Player 2"},
    ControlRow{.action = "Move", .first = "WASD or arrows", .second = "IJKL"},
    ControlRow{.action = "Run", .first = "Shift", .second = "H"},
    ControlRow{
        .action = "Set out or return to base", .first = "Q", .second = "U"},
    ControlRow{.action = "Collect or repair; appraise or cook at base",
               .first = "E",
               .second = "O"},
    ControlRow{.action = "Choose the dish, as the chef at base",
               .first = "W/S or up/down",
               .second = "none"},
    ControlRow{
        .action = "Replace a damaged base tool", .first = "R", .second = "P"},
    ControlRow{.action = "Feed a stored dish", .first = "F", .second = "["},
    ControlRow{.action = "End the day unfed", .first = "G", .second = "]"},
    ControlRow{.action = "Swap actor, playing alone",
               .first = "Tab",
               .second = "none"},
    ControlRow{.action = "Hand your actor to AI, playing together",
               .first = "X",
               .second = "N"},
    ControlRow{.action = "Pause or continue", .first = "Esc", .second = "Esc"},
    ControlRow{.action = "Leave the run; Esc cancels",
               .first = "M, then Enter",
               .second = "M, then Enter"},
};

const char *entry_label(TitleEntry entry) {
  switch (entry) {
  case TitleEntry::NewGame:
    return "NEW GAME";
  case TitleEntry::Controls:
    return "CONTROLS";
  case TitleEntry::Quit:
    return "QUIT";
  }
  std::abort();
}

const char *entry_summary(TitleEntry entry) {
  switch (entry) {
  case TitleEntry::NewGame:
    return "Play the chef, the appraiser, or both with a friend.";
  case TitleEntry::Controls:
    return "Every key for player one and player two.";
  case TitleEntry::Quit:
    return "Close the game.";
  }
  std::abort();
}

/// The monster's mouth behind a dark veil, so menu text reads over it.
void backdrop(godot::Node2D &canvas, const rendering::Art &drawing,
              float veil) {
  rendering::paint_mouth(canvas, drawing, 0.1F);
  canvas.draw_rect(rect(0.0F, 0.0F, 1280.0F, 720.0F), color(14, 12, 26, veil));
}

} // namespace

void paint_title(godot::Node2D &canvas, const rendering::Art &drawing,
                 TitleEntry selected) {
  const auto tones = rendering::palette();
  backdrop(canvas, drawing, 0.6F);
  drawing.text(canvas, 130.0F, 200.0F, "MONSTER MOUTH ESCAPE", 64, tones.white);
  drawing.text(canvas, 134.0F, 252.0F,
               "Gather, cook, and feed the monster that swallowed the city.",
               24, tones.muted);
  drawing.image(canvas, rendering::Image::ChefIdle,
                godot::Vector2(880.0F, 330.0F), 2.0F);
  drawing.image(canvas, rendering::Image::AppraiserIdle,
                godot::Vector2(1040.0F, 330.0F), 2.0F);
  float y = ENTRY_Y;
  std::size_t number = 1;
  for (const TitleEntry entry : TITLE_ENTRIES) {
    const bool chosen = entry == selected;
    if (chosen) {
      canvas.draw_rect(rect(ENTRY_X - 30.0F, y - 42.0F, 640.0F, 72.0F),
                       color(14, 12, 26, 0.85F));
      canvas.draw_rect(rect(ENTRY_X - 30.0F, y - 42.0F, 6.0F, 72.0F),
                       tones.cyan);
    }
    drawing.text(canvas, ENTRY_X, y,
                 godot::String::num_int64(static_cast<std::int64_t>(number)) +
                     "   " + entry_label(entry),
                 34, chosen ? tones.cyan : tones.white);
    drawing.text(canvas, ENTRY_X + 48.0F, y + 22.0F, entry_summary(entry), 18,
                 chosen ? tones.white : tones.muted);
    y += ENTRY_HEIGHT;
    ++number;
  }
  drawing.text(canvas, 130.0F, 680.0F,
               "W/S or up/down: choose     Enter: select     1, 2, 3: jump "
               "straight in",
               20, tones.white);
}

void paint_controls(godot::Node2D &canvas, const rendering::Art &drawing) {
  const auto tones = rendering::palette();
  backdrop(canvas, drawing, 0.9F);
  drawing.text(canvas, 190.0F, 110.0F, "CONTROLS", 46, tones.white);
  float y = CONTROLS_ROW_Y;
  for (const ControlRow &row : CONTROL_ROWS) {
    drawing.text(canvas, 190.0F, y, row.action, CONTROLS_ROW_SIZE, tones.white);
    drawing.text(canvas, 700.0F, y, row.first, CONTROLS_ROW_SIZE, tones.cyan);
    drawing.text(canvas, 930.0F, y, row.second, CONTROLS_ROW_SIZE, tones.cyan);
    y += CONTROLS_ROW_HEIGHT;
  }
  drawing.text(canvas, 190.0F, 680.0F, "Esc or M: back to the title", 22,
               tones.gold);
}

} // namespace neocity::hud
