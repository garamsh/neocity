#pragma once

#include <array>

namespace neocity::experiment {

/// One developer-page entry: what to show and which scene to play.
struct Entry {
  const char *id = "";
  const char *title = "";
  const char *summary = "";
  /// Directly runnable Godot scene; the developer page instantiates the same
  /// one.
  const char *scene = "";
};

/// Every registered experiment, in developer-page order.
inline constexpr std::array CATALOG{
    Entry{.id = "pan_dodge",
          .title = "PAN DODGE 3D",
          .summary = "Dodge dropped hazards on a swaying pan and escape "
                     "through its moving exit.",
          .scene = "res://experiment/pan_dodge/pan_dodge.tscn"},
};

} // namespace neocity::experiment
