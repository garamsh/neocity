#pragma once

#include "../rendering/drawing.hpp"
#include "pointer.hpp"

#include <godot_cpp/classes/node2d.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace neocity::hud {

/// The title screen's entries.
enum class TitleEntry : std::uint8_t {
  Continue,
  NewGame,
  Controls,
  HowToPlay,
  Quit,
  Settings
};

/// The title entries top to bottom; number key n chooses entry n, as far as
/// the input has number keys, and the last entry takes the cursor and a click.
inline constexpr std::array TITLE_ENTRIES{
    TitleEntry::NewGame, TitleEntry::Controls, TitleEntry::HowToPlay,
    TitleEntry::Quit, TitleEntry::Settings};

/// The visible entries, with Continue first only when the slot can be read.
[[nodiscard]] inline const std::vector<TitleEntry> &title_entries(bool saved) {
  static const std::vector<TitleEntry> entries{
      TitleEntry::Continue,  TitleEntry::NewGame, TitleEntry::Controls,
      TitleEntry::HowToPlay, TitleEntry::Quit,    TitleEntry::Settings};
  static const std::vector<TitleEntry> fresh(TITLE_ENTRIES.begin(),
                                             TITLE_ENTRIES.end());
  return saved ? entries : fresh;
}

/// How many leading title entries have a number key.
inline constexpr std::size_t NUMBERED_ENTRIES = 4;

/// Draws the title screen with `selected` marked. Each frame of the UI kit it
/// draws is listed in the `title_frames` meta of `canvas`: an array of
/// dictionaries with the frame's `kind` ("art", "panel", "button" or
/// "selected") and its screen `area`.
void paint_title(godot::Node2D &canvas, const rendering::Art &drawing,
                 TitleEntry selected, bool saved = false);
/// The title screen's entries, each clicked as its number key.
[[nodiscard]] std::vector<Target> title_targets(bool saved = false);

/// Draws the title picture across the whole screen, under a dark `veil` of
/// 0 to 1 that lets a page's panels stand out on it.
void paint_title_art(godot::Node2D &canvas, float veil);
/// Draws a riveted dark metal panel with worn edges and a magenta drip at its
/// top right, filling `area`, the way the UI kit's panels are.
void kit_panel(godot::Node2D &canvas, godot::Rect2 area);
/// Draws a dark metal button filling `area`, or the gold one when `selected`.
void kit_button(godot::Node2D &canvas, godot::Rect2 area, bool selected);

} // namespace neocity::hud
