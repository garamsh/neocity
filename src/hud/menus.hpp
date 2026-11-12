#pragma once

#include "../rendering/drawing.hpp"
#include "display_rows.hpp"
#include "pointer.hpp"

#include <godot_cpp/classes/node2d.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace neocity::hud {

/// The title screen's entries.
enum class TitleEntry : std::uint8_t {
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

/// How many leading title entries have a number key.
inline constexpr std::size_t NUMBERED_ENTRIES = 4;

/// Draws the title screen with `selected` marked.
void paint_title(godot::Node2D &canvas, const rendering::Art &drawing,
                 TitleEntry selected);
/// Draws every key of both players and the way back to the title.
void paint_controls(godot::Node2D &canvas, const rendering::Art &drawing);
/// Draws the How to play page: what the game asks, each first-play tip's
/// line, and the way back to the title.
void paint_how_to_play(godot::Node2D &canvas, const rendering::Art &drawing);
/// Draws the settings page: the display `rows` and the way back to the title.
void paint_settings(godot::Node2D &canvas, const rendering::Art &drawing,
                    const std::vector<DisplayRow> &rows);
/// The title screen's entries, each clicked as its number key.
[[nodiscard]] std::vector<Target> title_targets();
/// The settings page's arrows and its way back to the title.
[[nodiscard]] std::vector<Target>
settings_targets(const rendering::Art &drawing,
                 const std::vector<DisplayRow> &rows);
/// The way back to the title from the controls page or the How to play page,
/// which share its place.
[[nodiscard]] std::vector<Target>
controls_targets(const rendering::Art &drawing);

} // namespace neocity::hud
