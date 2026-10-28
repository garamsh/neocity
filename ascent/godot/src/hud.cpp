#include "drawing.hpp"

#include <array>
#include <cstdint>
#include <string>

namespace neocity::ascent {

namespace {

/// Formats `value` with at least two digits.
std::string two_digits(std::uint32_t value) {
  return (value < 10 ? "0" : "") + std::to_string(value);
}

godot::String from(const std::string &value) {
  return godot::String::utf8(value.c_str());
}

} // namespace

void AscentDrawing::hud(godot::Node2D &canvas, const WorldSnapshot &world,
                        godot::Color white, godot::Color muted,
                        godot::Color cyan, godot::Color gold) const {
  canvas.draw_rect(rect(0.0F, 0.0F, 1280.0F, 98.0F), color(5, 8, 23, 0.92F));
  canvas.draw_rect(rect(0.0F, 97.0F, 1280.0F, 1.0F), color(69, 81, 116, 0.6F));
  text(canvas, 30.0F, 32.0F, "NEOCITY", 22, white);
  text(canvas, 31.0F, 54.0F, "AFTER HOURS / INGREDIENT RUN", 10, muted);
  text(canvas, 31.0F, 79.0F, "01  THE SERVICE TOWERS", 12, gold);
  constexpr std::array characters{Character::Chef, Character::Detective};
  for (std::size_t i = 0; i < characters.size(); ++i) {
    const float x = 352.0F + static_cast<float>(i) * 136.0F;
    const bool active = world.leader.character == characters.at(i);
    canvas.draw_rect(rect(x, 16.0F, 126.0F, 62.0F),
                     active ? color(23, 54, 67, 1.0F)
                            : color(13, 21, 40, 1.0F));
    canvas.draw_rect(rect(x, 76.0F, 126.0F, 2.0F),
                     active ? cyan : color(47, 57, 82, 1.0F));
    sprite(canvas, i == 0 ? 0 : 2, rect(x - 5.0F, 7.0F, 62.0F, 75.0F), false);
    text(canvas, x + 54.0F, 43.0F, i == 0 ? "CHEF" : "DETECTIVE", 10, white);
    text(canvas, x + 54.0F, 61.0F, active ? "LEADING" : "PARTNER", 9,
         active ? cyan : muted);
  }
  text(canvas, 708.0F, 34.0F, "ASCENT", 10, muted);
  canvas.draw_rect(rect(708.0F, 47.0F, 230.0F, 5.0F), color(29, 40, 65, 1.0F));
  canvas.draw_rect(rect(708.0F, 47.0F, 230.0F * world.progress, 5.0F), cyan);
  text(canvas, 708.0F, 74.0F,
       from(two_digits(static_cast<std::uint32_t>(world.progress * 100.0F)) +
            "%   /   " + std::to_string(world.retries) + " RETRIES"),
       11, white);
  const auto seconds = static_cast<std::uint32_t>(world.elapsed);
  text(canvas, 1050.0F, 43.0F,
       from(two_digits(seconds / 60) + ":" + two_digits(seconds % 60)), 28,
       white);
  text(canvas, 1052.0F, 68.0F,
       world.phase == RunPhase::Complete ? "SPICE FRUIT  /  1 OF 1"
                                         : "SPICE FRUIT  /  0 OF 1",
       10, gold);
  canvas.draw_rect(rect(280.0F, 670.0F, 720.0F, 36.0F), color(5, 9, 25, 0.88F));
  text(canvas, 302.0F, 693.0F,
       "A D / ARROWS  MOVE     SPACE  JUMP     TAB  LEAD     R  RETRY     ESC  "
       "PAUSE",
       12, white);
  if (world.elapsed < 9.0F && world.progress < 0.05F) {
    canvas.draw_rect(rect(385.0F, 132.0F, 510.0F, 75.0F),
                     color(8, 15, 34, 0.92F));
    text(canvas, 410.0F, 160.0F, "A dangerous shortcut. An exceptional dinner.",
         18, white);
    text(canvas, 410.0F, 185.0F,
         "Climb the towers. Find the glowing spice fruit.", 13, muted);
  }
  if (world.phase != RunPhase::Playing) {
    canvas.draw_rect(rect(0.0F, 98.0F, 1280.0F, 622.0F),
                     color(3, 5, 19, 0.72F));
    canvas.draw_rect(rect(360.0F, 240.0F, 560.0F, 230.0F),
                     color(10, 19, 39, 0.98F));
    canvas.draw_rect(rect(360.0F, 240.0F, 560.0F, 3.0F), cyan);
    const bool complete = world.phase == RunPhase::Complete;
    text(canvas, 405.0F, 298.0F,
         complete ? "THE INGREDIENT IS OURS." : "TAKE A BREATH.", 27, white);
    text(canvas, 405.0F, 340.0F,
         complete ? "Now, let's see what the chef can do."
                  : "The city can wait a moment.",
         17, muted);
    text(canvas, 405.0F, 399.0F,
         complete ? "R  ANOTHER RUN" : "ESC  CONTINUE    /    R  CHECKPOINT",
         16, gold);
  }
}

} // namespace neocity::ascent
