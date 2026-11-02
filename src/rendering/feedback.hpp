#pragma once

#include "drawing.hpp"

#include <survival/exploration.hpp>

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace neocity::rendering {

/// A short reaction on screen to something that happened in play. It reads
/// what happened and moves nothing in the rules.
enum class Flourish : std::uint8_t {
  /// Rings and specks bursting from a collection.
  Pop,
  /// Sparks from a repair.
  Spark,
  /// A red flash and a short shake when acid burns an actor.
  Hit,
  /// Acid thrown up where an actor fell in.
  Splash,
  /// Droplets thrown up all along the acid's surface as it rises.
  Surge,
  /// Steam rising from the pot.
  Steam,
  /// The monster's pleased reaction to a dish.
  Delight,
  /// The monster's refusal of a dish.
  Disgust,
  /// The screen fading through dark between days.
  NewDay
};

/// One flourish under way: where it happened, in the city's sub-cells, and
/// how many seconds of real time ago.
struct Burst {
  Flourish kind = Flourish::Pop;
  std::optional<survival::Position> at;
  float age = 0.0F;
};

/// The flourish's name, as the Godot checks read it.
[[nodiscard]] const char *flourish_name(Flourish kind);
/// Seconds a flourish lasts on screen.
[[nodiscard]] float lifetime(Flourish kind);
/// How far the world is shaken this frame, in pixels.
[[nodiscard]] godot::Vector2 shake(const std::vector<Burst> &bursts);
/// Draws the flourishes that happen in the city, over the map, with the acid's
/// surface at `acid_ratio` as `paint_mouth` puts it.
void paint_city_bursts(godot::Node2D &canvas, const std::vector<Burst> &bursts,
                       float acid_ratio);
/// Draws the flourishes that happen at base, over the kitchen.
void paint_workshop_bursts(godot::Node2D &canvas, const Art &art,
                           const std::vector<Burst> &bursts);
/// Draws the flourishes over the whole screen: the flash of a burn and the
/// fade between days.
void paint_screen_bursts(godot::Node2D &canvas,
                         const std::vector<Burst> &bursts);

} // namespace neocity::rendering
