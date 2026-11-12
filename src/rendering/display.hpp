#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include <array>
#include <cstddef>
#include <cstdint>

namespace neocity::rendering {

/// A window size the player can choose.
struct Resolution {
  int width = 0;
  int height = 0;
};

/// The choosable window sizes, smallest first.
inline constexpr std::array<Resolution, 5> RESOLUTIONS{
    {{.width = 1280, .height = 720},
     {.width = 1600, .height = 900},
     {.width = 1920, .height = 1080},
     {.width = 2560, .height = 1440},
     {.width = 3840, .height = 2160}}};

/// The position of 1920x1080 in `RESOLUTIONS`, the default.
inline constexpr std::size_t FULL_HD = 2;
static_assert(RESOLUTIONS.at(FULL_HD).width == 1920 &&
              RESOLUTIONS.at(FULL_HD).height == 1080);

/// Where the player's choice is kept between runs.
inline constexpr const char *DISPLAY_FILE = "user://display.cfg";

enum class WindowMode : std::uint8_t { Windowed, Fullscreen };

/// What the player chose for the window: an index into `RESOLUTIONS`, the
/// size of the window while windowed, and whether it fills the display. The
/// canvas is drawn at the window's real output resolution either way; a full
/// screen window takes the display's size, whatever the resolution says.
struct Display {
  std::size_t resolution = FULL_HD;
  WindowMode mode = WindowMode::Fullscreen;
  friend bool operator==(const Display &, const Display &) = default;
};

/// Reads the choice kept in `file`; a missing or unreadable file, or a value
/// that is not one of the choices, gives the default for that part.
[[nodiscard]] Display load_display(const godot::String &file);
/// Keeps `display` in `file`; false when it cannot be written.
bool save_display(const Display &display, const godot::String &file);
/// Sizes and places the window `node` is in as `display` says.
void apply_display(godot::Node &node, const Display &display);
/// The size of the display the window is on; zero in both when the engine
/// reports none.
[[nodiscard]] godot::Vector2i display_size();
/// The last index of `RESOLUTIONS` that fits inside `screen`, the first when
/// none does and the last when `screen` is not positive.
[[nodiscard]] std::size_t largest_fit(const godot::Vector2i &screen);
/// `display` with its resolution held to `last` at most.
[[nodiscard]] Display fitted(Display display, std::size_t last);
/// `display` with its resolution moved `step` places, held between the first
/// and `last`.
[[nodiscard]] Display stepped_resolution(Display display, int step,
                                         std::size_t last);
/// `display` with the other window mode.
[[nodiscard]] Display toggled_mode(Display display);
/// The resolution at `index`, such as "1920x1080".
[[nodiscard]] godot::String resolution_label(std::size_t index);
/// The window mode's name on screen.
[[nodiscard]] godot::String mode_label(WindowMode mode);

} // namespace neocity::rendering
