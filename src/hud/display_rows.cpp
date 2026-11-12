#include "display_rows.hpp"

#include <cstddef>

namespace neocity::hud {
namespace {

using rendering::rect;

constexpr int ROW_SIZE = 2 * rendering::FONT_GRID;
constexpr float ARROWS_X = 240.0F;
constexpr float ARROW_WIDTH = 30.0F;
constexpr float ARROW_HEIGHT = 26.0F;
constexpr float VALUE_GAP = 10.0F;
constexpr float VALUE_WIDTH = 140.0F;
constexpr float ROW_PAD = 10.0F;
constexpr float ROW_BACK = 20.0F;

godot::Rect2 arrow(float left, float baseline, bool forward) {
  const float x =
      left + ARROWS_X +
      (forward ? ARROW_WIDTH + 2.0F * VALUE_GAP + VALUE_WIDTH : 0.0F);
  return rect(x, baseline - ROW_BACK, ARROW_WIDTH, ARROW_HEIGHT);
}

} // namespace

void paint_display_rows(godot::Node2D &canvas, const rendering::Art &drawing,
                        const std::vector<DisplayRow> &rows, float left,
                        float top, float pitch) {
  const auto tones = rendering::palette();
  float y = top;
  for (const auto &row : rows) {
    if (row.marked) {
      canvas.draw_rect(rect(left - ROW_PAD, y - pitch + ROW_PAD + 4.0F,
                            ARROWS_X + 2.0F * ARROW_WIDTH + 2.0F * VALUE_GAP +
                                VALUE_WIDTH + 2.0F * ROW_PAD,
                            pitch),
                       tones.raised);
    }
    drawing.text(canvas, left, y, row.name, ROW_SIZE,
                 row.marked ? tones.gold : tones.white);
    for (const bool forward : {false, true}) {
      const auto area = arrow(left, y, forward);
      rendering::button(canvas, area, false);
      drawing.text(canvas, area.position.x + 10.0F, y - 1.0F,
                   forward ? ">" : "<", ROW_SIZE, tones.white);
    }
    drawing.text(canvas, left + ARROWS_X + ARROW_WIDTH + VALUE_GAP, y,
                 row.value, ROW_SIZE, tones.white);
    y += pitch;
  }
}

std::vector<Target> display_row_targets(const std::vector<DisplayRow> &rows,
                                        float left, float top, float pitch) {
  std::vector<Target> targets;
  float y = top;
  for (const auto &row : rows) {
    for (const bool forward : {false, true}) {
      targets.push_back({.area = arrow(left, y, forward),
                         .press = DisplayPress{.setting = row.setting,
                                               .step = forward ? 1 : -1}});
    }
    y += pitch;
  }
  return targets;
}

} // namespace neocity::hud
