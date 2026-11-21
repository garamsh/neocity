#include "menus.hpp"

#include "survival.hpp"

#include <utility>

namespace neocity::hud {
namespace {

using rendering::rect;

/// The controls page lines up with the pause screen, whose rows it shares.
constexpr float CONTROLS_X = 220.0F;
constexpr float CONTROLS_Y = 20.0F;
constexpr float CONTROLS_WIDTH = 840.0F;
constexpr float CONTROLS_HEIGHT = 680.0F;
constexpr float CONTROLS_TEXT_X = 260.0F;
constexpr float CONTROLS_ROW_Y = 124.0F;
constexpr float CONTROLS_ROW_HEIGHT = 32.0F;
constexpr const char *CONTROLS_BACK = "Back to the title";
/// The How to play page shares the controls page's panel and way back.
constexpr float HOW_LEAD_Y = 136.0F;
constexpr float HOW_ROW_Y = 190.0F;
constexpr float HOW_ROW_HEIGHT = 42.0F;
/// The settings page's rows under its title.
constexpr float SETTINGS_ROW_Y = 170.0F;
constexpr float SETTINGS_ROW_HEIGHT = 56.0F;
constexpr float SETTINGS_HINT_Y = 330.0F;

godot::Rect2 back_area(const rendering::Art &drawing) {
  return rect(CONTROLS_TEXT_X - 14.0F, CONTROLS_Y + CONTROLS_HEIGHT - 76.0F,
              drawing.text_width(CONTROLS_BACK, 24) + 28.0F, 44.0F);
}

/// The title picture under a dark veil, so a page's panels stand out on it.
void backdrop(godot::Node2D &canvas, float veil) {
  paint_title_art(canvas, veil);
}

} // namespace

void paint_controls(godot::Node2D &canvas, const rendering::Art &drawing) {
  const auto tones = rendering::palette();
  backdrop(canvas, 0.6F);
  paint_panel(canvas,
              rect(CONTROLS_X, CONTROLS_Y, CONTROLS_WIDTH, CONTROLS_HEIGHT));
  drawing.text(canvas, CONTROLS_TEXT_X, CONTROLS_Y + 66.0F, "Controls", 40,
               tones.white, rendering::Weight::Title);
  paint_key_rows(canvas, drawing, control_rows(), CONTROLS_TEXT_X,
                 CONTROLS_ROW_Y, CONTROLS_ROW_HEIGHT);
  paint_action(canvas, drawing, back_area(drawing), CONTROLS_BACK, 24, false);
}

void paint_how_to_play(godot::Node2D &canvas, const rendering::Art &drawing) {
  const auto tones = rendering::palette();
  backdrop(canvas, 0.6F);
  paint_panel(canvas,
              rect(CONTROLS_X, CONTROLS_Y, CONTROLS_WIDTH, CONTROLS_HEIGHT));
  drawing.text(canvas, CONTROLS_TEXT_X, CONTROLS_Y + 66.0F, "How to play", 40,
               tones.white, rendering::Weight::Title);
  drawing.text(canvas, CONTROLS_TEXT_X, HOW_LEAD_Y,
               "Keep the monster fed until the way out opens.", 24, tones.lime);
  float y = HOW_ROW_Y;
  for (const Tip tip : TIPS) {
    drawing.text(canvas, CONTROLS_TEXT_X, y, tip_text(tip), 16, tones.white);
    y += HOW_ROW_HEIGHT;
  }
  paint_action(canvas, drawing, back_area(drawing), CONTROLS_BACK, 24, false);
}

void paint_settings(godot::Node2D &canvas, const rendering::Art &drawing,
                    const std::vector<DisplayRow> &rows) {
  const auto tones = rendering::palette();
  backdrop(canvas, 0.6F);
  paint_panel(canvas,
              rect(CONTROLS_X, CONTROLS_Y, CONTROLS_WIDTH, CONTROLS_HEIGHT));
  drawing.text(canvas, CONTROLS_TEXT_X, CONTROLS_Y + 66.0F, "Settings", 40,
               tones.white, rendering::Weight::Title);
  paint_display_rows(canvas, drawing, rows, CONTROLS_TEXT_X, SETTINGS_ROW_Y,
                     SETTINGS_ROW_HEIGHT);
  drawing.text(canvas, CONTROLS_TEXT_X, SETTINGS_HINT_Y,
               "W/S or up/down: choose   A/D or left/right: change", 16,
               tones.muted);
  drawing.text(canvas, CONTROLS_TEXT_X, SETTINGS_HINT_Y + 28.0F,
               "The picture is drawn at the window's own resolution.", 16,
               tones.muted);
  paint_action(canvas, drawing, back_area(drawing), CONTROLS_BACK, 24, false);
}

std::vector<Target> settings_targets(const rendering::Art &drawing,
                                     const std::vector<DisplayRow> &rows) {
  auto targets = controls_targets(drawing);
  for (auto &target : display_row_targets(rows, CONTROLS_TEXT_X, SETTINGS_ROW_Y,
                                          SETTINGS_ROW_HEIGHT)) {
    targets.push_back(std::move(target));
  }
  return targets;
}

std::vector<Target> controls_targets(const rendering::Art &drawing) {
  return {{.area = back_area(drawing), .press = MenuKey::Menu}};
}

} // namespace neocity::hud
