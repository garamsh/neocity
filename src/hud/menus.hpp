#pragma once

#include "../rendering/drawing.hpp"

#include <godot_cpp/classes/node2d.hpp>

#include <array>
#include <cstdint>

namespace neocity::hud {

/// The title screen's entries.
enum class TitleEntry : std::uint8_t { NewGame, Controls, Quit };

/// The title entries top to bottom; number key n chooses entry n.
inline constexpr std::array TITLE_ENTRIES{
    TitleEntry::NewGame, TitleEntry::Controls, TitleEntry::Quit};

/// Draws the title screen with `selected` marked.
void paint_title(godot::Node2D &canvas, const rendering::Art &drawing,
                 TitleEntry selected);
/// Draws every key of both players and the way back to the title.
void paint_controls(godot::Node2D &canvas, const rendering::Art &drawing);

} // namespace neocity::hud
