#include "menus.hpp"

#include "survival.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

namespace neocity::hud {
namespace {

using rendering::color;
using rendering::rect;

constexpr float SCREEN_WIDTH = 1280.0F;
constexpr float SCREEN_HEIGHT = 720.0F;
/// Panels keep this far from the screen's edge.
constexpr float MARGIN = 8.0F;
/// The logo's lettering starts 52 px into its picture.
constexpr float LOGO_X = 24.0F;
constexpr float LOGO_Y = -44.0F;
constexpr float ENTRY_X = 790.0F;
constexpr float ENTRY_Y = 300.0F;
constexpr float ENTRY_WIDTH = 460.0F;
constexpr float ENTRY_HEIGHT = 88.0F;
constexpr float ENTRY_PITCH = 96.0F;
constexpr float ENTRY_TEXT_X = 24.0F;
constexpr float FOOT_Y = 612.0F;
/// The controls page lines up with the pause screen, whose rows it shares.
constexpr float CONTROLS_X = 190.0F;
constexpr float CONTROLS_Y = 20.0F;
constexpr float CONTROLS_WIDTH = 900.0F;
constexpr float CONTROLS_HEIGHT = 680.0F;
constexpr float CONTROLS_TEXT_X = 230.0F;
constexpr float CONTROLS_ROW_Y = 140.0F;
constexpr float CONTROLS_ROW_HEIGHT = 38.0F;
constexpr const char *CONTROLS_BACK = "Esc or M: back to the title";

godot::Rect2 entry_area(std::size_t index) {
  return rect(ENTRY_X, ENTRY_Y + static_cast<float>(index) * ENTRY_PITCH,
              ENTRY_WIDTH, ENTRY_HEIGHT);
}

godot::Rect2 back_area(const rendering::Art &drawing) {
  return rect(CONTROLS_TEXT_X - 14.0F, CONTROLS_Y + CONTROLS_HEIGHT - 66.0F,
              drawing.text_width(CONTROLS_BACK, 24) + 28.0F, 46.0F);
}

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
                   color(9, 10, 20, veil));
}

/// The foot panel along the bottom edge, with its two lines.
void foot(godot::Node2D &canvas, const rendering::Art &drawing,
          const godot::String &line, const godot::String &keys) {
  const auto tones = rendering::palette();
  rendering::panel(canvas, rect(MARGIN, FOOT_Y, SCREEN_WIDTH - 2.0F * MARGIN,
                                SCREEN_HEIGHT - MARGIN - FOOT_Y));
  drawing.text(canvas, 32.0F, FOOT_Y + 42.0F, line, 24, tones.white);
  drawing.text(canvas, 32.0F, FOOT_Y + 76.0F, keys, 16, tones.muted);
}

} // namespace

void paint_title(godot::Node2D &canvas, const rendering::Art &drawing,
                 TitleEntry selected) {
  const auto tones = rendering::palette();
  drawing.image(canvas, rendering::Image::TitleKeyArt, godot::Vector2());
  drawing.image(canvas, rendering::Image::TitleLogo,
                godot::Vector2(LOGO_X, LOGO_Y));
  const auto chosen_at = static_cast<float>(
      std::ranges::find(TITLE_ENTRIES, selected) - TITLE_ENTRIES.begin());
  for (std::size_t index = 0; index < TITLE_ENTRIES.size(); ++index) {
    rendering::panel(canvas, entry_area(index));
  }
  // The highlight slides to the cursor's entry.
  rendering::button(
      canvas,
      rect(ENTRY_X,
           ENTRY_Y + drawing.glide("title_cursor", chosen_at) * ENTRY_PITCH,
           ENTRY_WIDTH, ENTRY_HEIGHT),
      true);
  float y = ENTRY_Y;
  std::size_t number = 1;
  for (const TitleEntry entry : TITLE_ENTRIES) {
    const bool chosen = entry == selected;
    drawing.text(canvas, ENTRY_X + ENTRY_TEXT_X, y + 38.0F,
                 godot::String::num_int64(static_cast<std::int64_t>(number)) +
                     "   " + entry_label(entry),
                 32, chosen ? tones.ink : tones.white, rendering::Weight::Bold);
    drawing.text(canvas, ENTRY_X + ENTRY_TEXT_X, y + 64.0F,
                 entry_summary(entry), 16, chosen ? tones.ink : tones.muted);
    y += ENTRY_PITCH;
    ++number;
  }
  foot(canvas, drawing,
       "Gather, cook, and feed the monster that swallowed the city.",
       "W/S or up/down: choose     Enter: select     1, 2, 3: jump straight "
       "in");
}

void paint_controls(godot::Node2D &canvas, const rendering::Art &drawing) {
  const auto tones = rendering::palette();
  backdrop(canvas, drawing, 0.6F);
  rendering::panel(
      canvas, rect(CONTROLS_X, CONTROLS_Y, CONTROLS_WIDTH, CONTROLS_HEIGHT));
  drawing.text(canvas, CONTROLS_TEXT_X, CONTROLS_Y + 66.0F, "Controls", 40,
               tones.white, rendering::Weight::Bold);
  paint_key_rows(canvas, drawing, control_rows(), CONTROLS_TEXT_X,
                 CONTROLS_ROW_Y, CONTROLS_ROW_HEIGHT);
  rendering::button(canvas, back_area(drawing), false);
  drawing.text(canvas, CONTROLS_TEXT_X, CONTROLS_Y + CONTROLS_HEIGHT - 36.0F,
               CONTROLS_BACK, 24, tones.white);
}

std::vector<Target> title_targets() {
  std::vector<Target> targets;
  for (std::size_t index = 0; index < TITLE_ENTRIES.size(); ++index) {
    targets.push_back(
        {.area = entry_area(index), .press = OptionPress{index + 1}});
  }
  return targets;
}

std::vector<Target> controls_targets(const rendering::Art &drawing) {
  return {{.area = back_area(drawing), .press = MenuKey::Menu}};
}

} // namespace neocity::hud
