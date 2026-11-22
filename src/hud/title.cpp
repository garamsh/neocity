#include "title.hpp"

#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>

namespace neocity::hud {
namespace {

using rendering::color;
using rendering::rect;
using rendering::SCREEN_HEIGHT;
using rendering::SCREEN_WIDTH;

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
constexpr float KEY_SIZE = 32.0F;

/// Notes a frame of the kit on `canvas`, for the checks that read the screen.
void note(godot::Node2D &canvas, const char *kind, godot::Rect2 area) {
  godot::Array frames = canvas.get_meta("title_frames", godot::Array());
  godot::Dictionary frame;
  frame.set("kind", kind);
  frame.set("area", area);
  frames.push_back(frame);
  canvas.set_meta("title_frames", frames);
}

} // namespace

void paint_title_art(godot::Node2D &canvas, float veil) {
  canvas.set_meta("title_frames", godot::Array());
  canvas.set_meta("interface_stamps", godot::Array());
  const godot::Ref<godot::Texture2D> art =
      godot::ResourceLoader::get_singleton()->load(ART);
  CRASH_COND_MSG(art.is_null(), "Title art must load");
  // The canvas keeps the picture alive: a texture freed before the frame is
  // rendered draws white.
  canvas.set_meta("title_art", art);
  const godot::Rect2 screen = rect(0.0F, 0.0F, SCREEN_WIDTH, SCREEN_HEIGHT);
  canvas.draw_texture_rect(art, screen, false);
  paint_hd_art(canvas, "characters/chef_idle.png",
               rect(90.0F, 430.0F, 256.0F / rendering::ART_SCALE,
                    224.0F / rendering::ART_SCALE));
  paint_hd_art(canvas, "characters/appraiser_idle.png",
               rect(260.0F, 430.0F, 256.0F / rendering::ART_SCALE,
                    224.0F / rendering::ART_SCALE));
  if (veil > 0.0F) {
    canvas.draw_rect(screen, color(9, 10, 20, veil));
  }
  note(canvas, "art", screen);
}

void paint_hd_art(godot::Node2D &canvas, const godot::String &path,
                  godot::Rect2 area) {
  const godot::Ref<godot::Texture2D> texture =
      godot::ResourceLoader::get_singleton()->load(
          "res://levels/monster_mouth_escape/art_hd/" + path);
  CRASH_COND_MSG(texture.is_null(), "HUD art must load: " + path);
  godot::Dictionary textures = canvas.get_meta("hud_art", godot::Dictionary());
  textures.set(path, texture);
  canvas.set_meta("hud_art", textures);
  canvas.draw_texture_rect(texture, area, false);
}

void paint_panel(godot::Node2D &canvas, godot::Rect2 area) {
  rendering::plate(canvas, area);
  note(canvas, "panel", area);
}

void paint_action(godot::Node2D &canvas, const rendering::Art &drawing,
                  godot::Rect2 area, const godot::String &text, int size,
                  bool focused) {
  const auto state = rendering::stand(canvas, area, focused);
  rendering::button(canvas, area, state);
  note(canvas, focused ? "selected" : "button", area);
  const float width = drawing.text_width(text, size, rendering::Weight::Bold);
  drawing.text(
      canvas,
      area.position.x + (area.size.x - rendering::ARROW_ROOM - width) / 2.0F,
      area.position.y + (area.size.y + static_cast<float>(size) * 0.7F) / 2.0F +
          (state == rendering::State::Pressed ? rendering::PRESS_DROP : 0.0F),
      text, size, rendering::palette().ink, rendering::Weight::Bold);
}

void paint_card(godot::Node2D &canvas, godot::Rect2 area, bool focused) {
  rendering::card(canvas, area, rendering::stand(canvas, area, focused));
  note(canvas, focused ? "selected" : "button", area);
}

namespace {

float entry_pitch(bool saved) { return saved ? 62.0F : ENTRY_PITCH; }

godot::Rect2 entry_area(std::size_t index, bool saved) {
  return rect(ENTRY_X, ENTRY_Y + static_cast<float>(index) * entry_pitch(saved),
              ENTRY_WIDTH, saved ? 58.0F : ENTRY_HEIGHT);
}

const char *entry_label(TitleEntry entry) {
  switch (entry) {
  case TitleEntry::Continue:
    return "CONTINUE";
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
  case TitleEntry::Continue:
    return "Resume from the start of the saved day.";
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
                 TitleEntry selected, bool saved) {
  const auto &entries = title_entries(saved);
  const auto tones = rendering::palette();
  paint_title_art(canvas, 0.0F);
  rendering::fade(canvas, rect(MENU_X - 120.0F, 0.0F, 120.0F, SCREEN_HEIGHT));
  const godot::Rect2 menu =
      rect(MENU_X, 0.0F, SCREEN_WIDTH - MENU_X, SCREEN_HEIGHT);
  canvas.draw_rect(menu, tones.background);
  note(canvas, "panel", menu);
  canvas.draw_rect(rect(ENTRY_X, MENU_Y, ENTRY_WIDTH, 5.0F), tones.lime);
  const auto chosen_at = static_cast<float>(
      std::ranges::find(entries, selected) - entries.begin());
  for (std::size_t index = 0; index < entries.size(); ++index) {
    paint_card(canvas, entry_area(index, saved), false);
  }
  // The lime card slides to the cursor's entry.
  paint_card(canvas,
             rect(ENTRY_X,
                  ENTRY_Y + drawing.glide("title_cursor", chosen_at) *
                                entry_pitch(saved),
                  ENTRY_WIDTH, saved ? 58.0F : ENTRY_HEIGHT),
             true);
  float y = ENTRY_Y;
  std::size_t number = 1;
  for (const TitleEntry entry : entries) {
    const bool chosen = entry == selected;
    const float height = saved ? 58.0F : ENTRY_HEIGHT;
    if (number <= NUMBERED_ENTRIES) {
      rendering::key_cap(
          canvas, drawing,
          rect(ENTRY_X + ENTRY_KEY_X, y + (height - KEY_SIZE) / 2.0F, KEY_SIZE,
               KEY_SIZE),
          godot::String::num_int64(static_cast<std::int64_t>(number)),
          chosen ? rendering::State::Focus : rendering::State::Default);
    }
    drawing.text(canvas, ENTRY_X + ENTRY_TEXT_X, y + (saved ? 26.0F : 30.0F),
                 entry_label(entry), 26, chosen ? tones.ink : tones.white,
                 rendering::Weight::Title);
    drawing.text(canvas, ENTRY_X + ENTRY_TEXT_X, y + (saved ? 47.0F : 53.0F),
                 entry_summary(entry), 15, chosen ? tones.ink : tones.muted);
    y += entry_pitch(saved);
    ++number;
  }
  paint_panel(canvas, rect(MARGIN, FOOT_Y, SCREEN_WIDTH - 2.0F * MARGIN,
                           SCREEN_HEIGHT - MARGIN - FOOT_Y));
  drawing.text(canvas, 32.0F, FOOT_Y + 42.0F,
               "Gather, cook, and feed the monster that swallowed the city.",
               24, tones.white);
  drawing.text(canvas, 32.0F, FOOT_Y + 76.0F,
               "Click an entry to choose it. Controls lists every key.", 16,
               tones.muted);
}

std::vector<Target> title_targets(bool saved) {
  const auto &entries = title_entries(saved);
  std::vector<Target> targets;
  targets.reserve(entries.size());
  for (std::size_t index = 0; index < entries.size(); ++index) {
    targets.push_back(
        {.area = entry_area(index, saved), .press = OptionPress{index + 1}});
  }
  return targets;
}

} // namespace neocity::hud
