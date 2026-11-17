#include "title.hpp"

#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>

namespace neocity::hud {
namespace {

using rendering::color;
using rendering::rect;
using rendering::SCREEN_HEIGHT;
using rendering::SCREEN_WIDTH;

/// One pixel of the kit, in layout units: three screen pixels.
constexpr float PX = 2.0F;
constexpr const char *ART =
    "res://levels/monster_mouth_escape/art_hd/title/key_art.png";

constexpr float MENU_X = 770.0F;
constexpr float MENU_Y = 140.0F;
constexpr float MENU_WIDTH = 480.0F;
constexpr float MENU_HEIGHT = 444.0F;
constexpr float ENTRY_X = 794.0F;
constexpr float ENTRY_Y = 190.0F;
constexpr float ENTRY_WIDTH = 432.0F;
constexpr float ENTRY_HEIGHT = 66.0F;
constexpr float ENTRY_PITCH = 74.0F;
constexpr float ENTRY_KEY_X = 20.0F;
constexpr float ENTRY_TEXT_X = 72.0F;
constexpr float FOOT_Y = 612.0F;
constexpr float MARGIN = 8.0F;

/// The metal, paint and goo of the UI kit.
struct Kit {
  godot::Color outline = color(12, 11, 18, 1.0F);
  godot::Color fill = color(21, 20, 32, 1.0F);
  godot::Color idle = color(30, 29, 48, 1.0F);
  godot::Color rim_light = color(124, 118, 140, 1.0F);
  godot::Color rim = color(78, 73, 92, 1.0F);
  godot::Color rim_dark = color(40, 37, 52, 1.0F);
  godot::Color rivet = color(168, 162, 178, 1.0F);
  godot::Color rust = color(184, 108, 38, 1.0F);
  godot::Color goo = color(214, 58, 154, 1.0F);
  godot::Color goo_light = color(255, 140, 208, 1.0F);
  godot::Color gold_light = color(255, 236, 150, 1.0F);
  godot::Color gold_top = color(255, 206, 64, 1.0F);
  godot::Color gold_bottom = color(236, 150, 16, 1.0F);
  godot::Color gold_edge = color(122, 70, 8, 1.0F);
  godot::Color lamp_glass = color(250, 168, 38, 1.0F);
  godot::Color lamp_brass = color(150, 96, 30, 1.0F);
};

void fill(godot::Node2D &canvas, float x, float y, float width, float height,
          godot::Color tone) {
  canvas.draw_rect(rect(x, y, width, height), tone);
}

/// Notes a frame of the kit on `canvas`, for the checks that read the screen.
void note(godot::Node2D &canvas, const char *kind, godot::Rect2 area) {
  godot::Array frames = canvas.get_meta("title_frames", godot::Array());
  godot::Dictionary frame;
  frame.set("kind", kind);
  frame.set("area", area);
  frames.push_back(frame);
  canvas.set_meta("title_frames", frames);
}

/// A rivet of two by two pixels with a lit corner.
void rivet(godot::Node2D &canvas, const Kit &kit, float x, float y) {
  fill(canvas, x, y, PX, PX, kit.rivet);
  fill(canvas, x, y, PX / 2.0F, PX / 2.0F, kit.rim_light);
}

/// Flesh dripping over the top right of `area`: a blob and three drips of
/// fixed lengths, each ending in a drop.
void goo(godot::Node2D &canvas, const Kit &kit, godot::Rect2 area) {
  const float right = area.get_end().x;
  const float top = area.position.y;
  fill(canvas, right - 7.0F * PX, top - PX, 6.0F * PX, 3.0F * PX, kit.goo);
  fill(canvas, right - 7.0F * PX, top - PX, 6.0F * PX, PX / 2.0F,
       kit.goo_light);
  const std::array<float, 3> lengths{5.0F, 9.0F, 3.0F};
  float x = right - 6.0F * PX;
  for (const float length : lengths) {
    fill(canvas, x, top + 2.0F * PX, PX, length * PX, kit.goo);
    fill(canvas, x - PX / 2.0F, top + (2.0F + length) * PX, 2.0F * PX, PX,
         kit.goo);
    fill(canvas, x, top + 2.0F * PX, PX / 2.0F, length * PX, kit.goo_light);
    x += 2.0F * PX;
  }
}

/// A lit lantern hanging from the top left corner of `area`.
void lantern(godot::Node2D &canvas, const Kit &kit, godot::Rect2 area) {
  const float x = area.position.x - 2.0F * PX;
  const float y = area.position.y - 3.0F * PX;
  rendering::rounded(
      canvas, rect(x - 8.0F, y - 4.0F, 7.0F * PX + 16.0F, 12.0F * PX + 8.0F),
      14.0F, color(255, 160, 40, 0.14F));
  rendering::rounded(canvas, rect(x - 2.0F, y, 7.0F * PX + 4.0F, 10.0F * PX),
                     8.0F, color(255, 170, 50, 0.22F));
  fill(canvas, x + PX, y, 5.0F * PX, PX, kit.lamp_brass);
  fill(canvas, x, y + PX, 7.0F * PX, PX, kit.lamp_brass);
  fill(canvas, x, y + 2.0F * PX, 7.0F * PX, 6.0F * PX, kit.outline);
  fill(canvas, x + PX, y + 2.0F * PX, 5.0F * PX, 6.0F * PX, kit.lamp_glass);
  fill(canvas, x + 2.0F * PX, y + 3.0F * PX, 3.0F * PX, 4.0F * PX,
       kit.gold_light);
  fill(canvas, x, y + 8.0F * PX, 7.0F * PX, PX, kit.lamp_brass);
}

/// Worn rust on the rim of `area`: a few pixels placed by its size alone, so
/// the same panel is worn the same way every frame.
void wear(godot::Node2D &canvas, const Kit &kit, godot::Rect2 area) {
  const auto steps = static_cast<int>(area.size.x / (14.0F * PX));
  for (int step = 0; step < steps; ++step) {
    const float along = static_cast<float>((step * 37 + 11) % 97) / 97.0F;
    const float x = area.position.x + 4.0F * PX +
                    PX * std::floor(along * (area.size.x - 8.0F * PX) / PX);
    const float y =
        step % 2 == 0 ? area.position.y + PX : area.get_end().y - 2.0F * PX;
    fill(canvas, x, y, 2.0F * PX, PX, kit.rust);
  }
}

/// The metal frame of a panel or a button: a dark outline, a bevelled rim of
/// `rim_width` pixels, light above and left, dark below and right.
void bevel(godot::Node2D &canvas, const Kit &kit, godot::Rect2 area,
           float rim_width) {
  const float rim_size = rim_width * PX;
  fill(canvas, area.position.x, area.position.y, area.size.x, area.size.y,
       kit.outline);
  fill(canvas, area.position.x + PX, area.position.y + PX,
       area.size.x - 2.0F * PX, area.size.y - 2.0F * PX, kit.rim);
  fill(canvas, area.position.x + PX, area.position.y + PX,
       area.size.x - 2.0F * PX, PX, kit.rim_light);
  fill(canvas, area.position.x + PX, area.position.y + PX, PX,
       area.size.y - 2.0F * PX, kit.rim_light);
  fill(canvas, area.position.x + PX, area.get_end().y - 2.0F * PX,
       area.size.x - 2.0F * PX, PX, kit.rim_dark);
  fill(canvas, area.get_end().x - 2.0F * PX, area.position.y + PX, PX,
       area.size.y - 2.0F * PX, kit.rim_dark);
  const float inner = PX + rim_size;
  fill(canvas, area.position.x + inner, area.position.y + inner,
       area.size.x - 2.0F * inner, area.size.y - 2.0F * inner, kit.outline);
}

/// The gold of a marked button, lit along its top.
void gold(godot::Node2D &canvas, const Kit &kit, godot::Rect2 area) {
  fill(canvas, area.position.x, area.position.y, area.size.x, area.size.y,
       kit.gold_edge);
  const godot::Rect2 inner(area.position + godot::Vector2(PX, PX),
                           area.size - godot::Vector2(2.0F * PX, 2.0F * PX));
  const float half = std::floor(inner.size.y / PX / 2.0F) * PX;
  fill(canvas, inner.position.x, inner.position.y, inner.size.x, half,
       kit.gold_top);
  fill(canvas, inner.position.x, inner.position.y + half, inner.size.x,
       inner.size.y - half, kit.gold_bottom);
  fill(canvas, inner.position.x, inner.position.y, inner.size.x, PX,
       kit.gold_light);
  fill(canvas, inner.position.x + 3.0F * PX, area.get_end().y - 2.0F * PX,
       inner.size.x - 6.0F * PX, PX / 2.0F, kit.gold_edge);
}

} // namespace

void paint_title_art(godot::Node2D &canvas, float veil) {
  canvas.set_meta("title_frames", godot::Array());
  const godot::Ref<godot::Texture2D> art =
      godot::ResourceLoader::get_singleton()->load(ART);
  CRASH_COND_MSG(art.is_null(), "Title art must load");
  // The canvas keeps the picture alive: a texture freed before the frame is
  // rendered draws white.
  canvas.set_meta("title_art", art);
  const godot::Rect2 screen = rect(0.0F, 0.0F, SCREEN_WIDTH, SCREEN_HEIGHT);
  canvas.draw_texture_rect(art, screen, false);
  if (veil > 0.0F) {
    canvas.draw_rect(screen, color(9, 10, 20, veil));
  }
  note(canvas, "art", screen);
}

void kit_panel(godot::Node2D &canvas, godot::Rect2 area) {
  const Kit kit;
  bevel(canvas, kit, area, 2.0F);
  fill(canvas, area.position.x + 4.0F * PX, area.position.y + 4.0F * PX,
       area.size.x - 8.0F * PX, area.size.y - 8.0F * PX, kit.fill);
  wear(canvas, kit, area);
  for (const float x :
       {area.position.x + 2.0F * PX, area.get_end().x - 4.0F * PX}) {
    for (const float y :
         {area.position.y + 2.0F * PX, area.get_end().y - 4.0F * PX}) {
      rivet(canvas, kit, x, y);
    }
  }
  goo(canvas, kit, area);
  note(canvas, "panel", area);
}

void kit_button(godot::Node2D &canvas, godot::Rect2 area, bool selected) {
  const Kit kit;
  if (selected) {
    gold(canvas, kit, area);
  } else {
    bevel(canvas, kit, area, 1.0F);
    fill(canvas, area.position.x + 2.0F * PX, area.position.y + 2.0F * PX,
         area.size.x - 4.0F * PX, area.size.y - 4.0F * PX, kit.idle);
  }
  if (area.size.x >= 20.0F * PX) {
    for (const float x :
         {area.position.x + 2.0F * PX, area.get_end().x - 4.0F * PX}) {
      const float y =
          area.position.y + std::floor(area.size.y / PX / 2.0F) * PX - PX;
      rivet(canvas, kit, x, y);
    }
  }
  note(canvas, selected ? "selected" : "button", area);
}

namespace {

godot::Rect2 entry_area(std::size_t index) {
  return rect(ENTRY_X, ENTRY_Y + static_cast<float>(index) * ENTRY_PITCH,
              ENTRY_WIDTH, ENTRY_HEIGHT);
}

const char *entry_label(TitleEntry entry) {
  switch (entry) {
  case TitleEntry::NewGame:
    return "NEW GAME";
  case TitleEntry::Controls:
    return "CONTROLS";
  case TitleEntry::HowToPlay:
    return "HOW TO PLAY";
  case TitleEntry::Quit:
    return "QUIT";
  case TitleEntry::Settings:
    return "SETTINGS";
  }
  std::abort();
}

const char *entry_summary(TitleEntry entry) {
  switch (entry) {
  case TitleEntry::NewGame:
    return "Play the chef, the appraiser, or both with a friend.";
  case TitleEntry::Controls:
    return "Every key for player one and player two.";
  case TitleEntry::HowToPlay:
    return "What a day asks of you, in a line each.";
  case TitleEntry::Quit:
    return "Close the game.";
  case TitleEntry::Settings:
    return "Window size and full screen.";
  }
  std::abort();
}

} // namespace

void paint_title(godot::Node2D &canvas, const rendering::Art &drawing,
                 TitleEntry selected) {
  const auto tones = rendering::palette();
  const Kit kit;
  paint_title_art(canvas, 0.0F);
  const godot::Rect2 menu = rect(MENU_X, MENU_Y, MENU_WIDTH, MENU_HEIGHT);
  kit_panel(canvas, menu);
  lantern(canvas, kit, menu);
  const auto chosen_at = static_cast<float>(
      std::ranges::find(TITLE_ENTRIES, selected) - TITLE_ENTRIES.begin());
  for (std::size_t index = 0; index < TITLE_ENTRIES.size(); ++index) {
    kit_button(canvas, entry_area(index), false);
  }
  // The gold button slides to the cursor's entry.
  kit_button(
      canvas,
      rect(ENTRY_X,
           ENTRY_Y + drawing.glide("title_cursor", chosen_at) * ENTRY_PITCH,
           ENTRY_WIDTH, ENTRY_HEIGHT),
      true);
  float y = ENTRY_Y;
  std::size_t number = 1;
  for (const TitleEntry entry : TITLE_ENTRIES) {
    const bool chosen = entry == selected;
    if (number <= NUMBERED_ENTRIES) {
      drawing.text(
          canvas, ENTRY_X + ENTRY_KEY_X, y + 40.0F,
          "[" + godot::String::num_int64(static_cast<std::int64_t>(number)) +
              "]",
          24, chosen ? tones.ink : tones.gold, rendering::Weight::Bold);
    }
    drawing.text(canvas, ENTRY_X + ENTRY_TEXT_X, y + 30.0F, entry_label(entry),
                 32, chosen ? tones.ink : tones.white, rendering::Weight::Bold);
    drawing.text(canvas, ENTRY_X + ENTRY_TEXT_X, y + 54.0F,
                 entry_summary(entry), 16, chosen ? tones.ink : tones.muted);
    y += ENTRY_PITCH;
    ++number;
  }
  kit_panel(canvas, rect(MARGIN, FOOT_Y, SCREEN_WIDTH - 2.0F * MARGIN,
                         SCREEN_HEIGHT - MARGIN - FOOT_Y));
  drawing.text(canvas, 32.0F, FOOT_Y + 42.0F,
               "Gather, cook, and feed the monster that swallowed the city.",
               24, tones.white);
  drawing.text(canvas, 32.0F, FOOT_Y + 76.0F,
               "Click an entry to choose it. Controls lists every key.", 16,
               tones.muted);
}

std::vector<Target> title_targets() {
  std::vector<Target> targets;
  targets.reserve(TITLE_ENTRIES.size());
  for (std::size_t index = 0; index < TITLE_ENTRIES.size(); ++index) {
    targets.push_back(
        {.area = entry_area(index), .press = OptionPress{index + 1}});
  }
  return targets;
}

} // namespace neocity::hud
