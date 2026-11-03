#include "pointer.hpp"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <type_traits>

namespace neocity::hud {
namespace {

using godot::String;

constexpr int REFUSAL_SIZE = 2 * rendering::FONT_GRID;
constexpr float REFUSAL_PAD = 10.0F;
constexpr float REFUSAL_HEIGHT = 30.0F;
constexpr float SCREEN_WIDTH = 1280.0F;

const char *actor_word(survival::Actor actor) {
  switch (actor) {
  case survival::Actor::Chef:
    return "chef";
  case survival::Actor::Appraiser:
    return "appraiser";
  }
  std::abort();
}

const char *key_word(PlayerKey key) {
  switch (key) {
  case PlayerKey::Interact:
    return "interact";
  case PlayerKey::Feed:
    return "feed";
  case PlayerKey::EndDay:
    return "end_day";
  case PlayerKey::ReplaceTool:
    return "replace_tool";
  case PlayerKey::Travel:
    return "travel";
  }
  std::abort();
}

const char *menu_word(MenuKey key) {
  switch (key) {
  case MenuKey::Confirm:
    return "confirm";
  case MenuKey::Pause:
    return "pause";
  case MenuKey::Menu:
    return "menu";
  }
  std::abort();
}

} // namespace

const Target *target_at(const std::vector<Target> &targets,
                        godot::Vector2 point) {
  const Target *found = nullptr;
  for (const auto &target : targets) {
    if (target.area.has_point(point)) {
      found = &target;
    }
  }
  return found;
}

String press_name(const Press &press) {
  return std::visit(
      [](const auto &pressed) -> String {
        using Pressed = std::decay_t<decltype(pressed)>;
        if constexpr (std::is_same_v<Pressed, OptionPress>) {
          return "option_" +
                 String::num_int64(static_cast<std::int64_t>(pressed.number));
        } else if constexpr (std::is_same_v<Pressed, MenuKey>) {
          return menu_word(pressed);
        } else if constexpr (std::is_same_v<Pressed, PlayerPress>) {
          return String(pressed.actor ? actor_word(*pressed.actor) : "party") +
                 "_" + key_word(pressed.key);
        } else if constexpr (std::is_same_v<Pressed, DishPress>) {
          return pressed.direction == survival::Direction::Up ? "dish_up"
                                                              : "dish_down";
        } else {
          return "volume_" +
                 String::num_int64(static_cast<std::int64_t>(pressed.row)) +
                 "_" + String::num_int64(pressed.level);
        }
      },
      press);
}

void paint_pointer(godot::Node2D &canvas, const rendering::Art &drawing,
                   const std::vector<Target> &targets,
                   std::optional<godot::Vector2> pointer) {
  const auto tones = rendering::palette();
  const Target *hovered = pointer ? target_at(targets, *pointer) : nullptr;
  godot::Array listed;
  for (const auto &target : targets) {
    godot::Dictionary entry;
    entry.set("rect", target.area);
    entry.set("press", press_name(target.press));
    entry.set("refusal", target.refusal);
    entry.set("hovered", &target == hovered);
    listed.push_back(entry);
  }
  canvas.set_meta("pointer_targets", listed);
  if (hovered == nullptr) {
    return;
  }
  const auto &area = hovered->area;
  if (hovered->refusal.is_empty()) {
    canvas.draw_rect(area, godot::Color(tones.gold, 0.18F));
    canvas.draw_rect(area, tones.gold, false, 2.0F);
    return;
  }
  canvas.draw_rect(area, tones.faint, false, 2.0F);
  // The reason sits over the target, kept on the screen.
  const float width =
      drawing.text_width(hovered->refusal, REFUSAL_SIZE) + 2.0F * REFUSAL_PAD;
  const float x = std::clamp(area.position.x, 0.0F, SCREEN_WIDTH - width);
  const float y = std::max(0.0F, area.position.y - REFUSAL_HEIGHT - 4.0F);
  rendering::panel(canvas, godot::Rect2(x, y, width, REFUSAL_HEIGHT));
  drawing.text(canvas, x + REFUSAL_PAD, y + 21.0F, hovered->refusal,
               REFUSAL_SIZE, tones.white);
}

} // namespace neocity::hud
