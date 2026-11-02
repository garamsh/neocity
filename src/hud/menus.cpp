#include "menus.hpp"

#include "survival.hpp"

#include <cstddef>
#include <cstdlib>

namespace neocity::hud {
namespace {

using rendering::color;
using rendering::rect;

constexpr float SCREEN_WIDTH = 1280.0F;
constexpr float SCREEN_HEIGHT = 720.0F;
/// Plates along the screen's edges run this far past it.
constexpr float HANG = 14.0F;
/// The logo's lettering starts 52 px into its picture.
constexpr float LOGO_X = 24.0F;
constexpr float LOGO_Y = -44.0F;
constexpr float ENTRY_X = 790.0F;
constexpr float ENTRY_Y = 300.0F;
constexpr float ENTRY_WIDTH = 460.0F;
constexpr float ENTRY_HEIGHT = 88.0F;
constexpr float ENTRY_PITCH = 96.0F;
constexpr float ENTRY_TEXT_X = 24.0F;
constexpr float FOOT_Y = 604.0F;
constexpr float CONTROLS_X = 150.0F;
constexpr float CONTROLS_Y = 20.0F;
constexpr float CONTROLS_WIDTH = 980.0F;
constexpr float CONTROLS_HEIGHT = 680.0F;
constexpr float CONTROLS_TEXT_X = 200.0F;
constexpr float CONTROLS_ROW_Y = 140.0F;
constexpr float CONTROLS_ROW_HEIGHT = 38.0F;

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

/// The title key art under a dark veil, so a page's panels stand out on it.
void backdrop(godot::Node2D &canvas, const rendering::Art &drawing,
              float veil) {
  drawing.image(canvas, rendering::Image::TitleKeyArt, godot::Vector2());
  canvas.draw_rect(rect(0.0F, 0.0F, SCREEN_WIDTH, SCREEN_HEIGHT),
                   color(14, 12, 26, veil));
}

} // namespace

void paint_title(godot::Node2D &canvas, const rendering::Art &drawing,
                 TitleEntry selected) {
  const auto tones = rendering::palette();
  drawing.image(canvas, rendering::Image::TitleKeyArt, godot::Vector2());
  drawing.image(canvas, rendering::Image::TitleLogo,
                godot::Vector2(LOGO_X, LOGO_Y));
  float y = ENTRY_Y;
  std::size_t number = 1;
  for (const TitleEntry entry : TITLE_ENTRIES) {
    const bool chosen = entry == selected;
    drawing.button(canvas, rect(ENTRY_X, y, ENTRY_WIDTH, ENTRY_HEIGHT), chosen);
    drawing.text(canvas, ENTRY_X + ENTRY_TEXT_X, y + 40.0F,
                 godot::String::num_int64(static_cast<std::int64_t>(number)) +
                     "   " + entry_label(entry),
                 30, chosen ? tones.ink : tones.white);
    drawing.text(canvas, ENTRY_X + ENTRY_TEXT_X, y + 64.0F,
                 entry_summary(entry), 15, chosen ? tones.ink : tones.white);
    y += ENTRY_PITCH;
    ++number;
  }
  drawing.panel(canvas, rect(-HANG, FOOT_Y, SCREEN_WIDTH + 2.0F * HANG,
                             SCREEN_HEIGHT - FOOT_Y + HANG));
  drawing.text(canvas, 30.0F, FOOT_Y + 52.0F,
               "Gather, cook, and feed the monster that swallowed the city.",
               22, tones.white);
  drawing.text(canvas, 30.0F, FOOT_Y + 90.0F,
               "W/S or up/down: choose     Enter: select     1, 2, 3: jump "
               "straight in",
               18, tones.muted);
}

void paint_controls(godot::Node2D &canvas, const rendering::Art &drawing) {
  const auto tones = rendering::palette();
  backdrop(canvas, drawing, 0.5F);
  drawing.panel(canvas,
                rect(CONTROLS_X, CONTROLS_Y, CONTROLS_WIDTH, CONTROLS_HEIGHT));
  drawing.text(canvas, CONTROLS_TEXT_X, CONTROLS_Y + 70.0F, "CONTROLS", 40,
               tones.gold);
  paint_key_rows(canvas, drawing, control_rows(), CONTROLS_ROW_Y,
                 CONTROLS_ROW_HEIGHT);
  const godot::String back = "Esc or M: back to the title";
  drawing.button(canvas,
                 rect(CONTROLS_TEXT_X - 12.0F,
                      CONTROLS_Y + CONTROLS_HEIGHT - 66.0F,
                      drawing.text_width(back, 22) + 24.0F, 44.0F),
                 false);
  drawing.text(canvas, CONTROLS_TEXT_X, CONTROLS_Y + CONTROLS_HEIGHT - 36.0F,
               back, 22, tones.gold);
}

} // namespace neocity::hud
