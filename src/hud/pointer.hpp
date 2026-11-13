#pragma once

#include "../audio/sound.hpp"
#include "../rendering/drawing.hpp"

#include <survival/companion.hpp>

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

namespace neocity::hud {

/// A menu key a click stands for: Enter, Esc, or M.
enum class MenuKey : std::uint8_t { Confirm, Pause, Menu };

/// One of a human's keys a click stands for.
enum class PlayerKey : std::uint8_t { Interact, Feed, EndDay, Travel };

/// The menu entry its number key chooses, numbered from 1.
struct OptionPress {
  std::size_t number = 0;
};

/// `key` of the human playing `actor`; with no actor, of every human, as
/// when the party sets out together.
struct PlayerPress {
  std::optional<survival::Actor> actor;
  PlayerKey key = PlayerKey::Interact;
};

/// A human chef's up or down press, which moves the dish choice.
struct DishPress {
  survival::Direction direction = survival::Direction::Down;
};

/// The volume of `bus` set to `level`.
struct VolumePress {
  audio::Bus bus = audio::Bus::Effects;
  std::uint8_t level = 0;
};

/// A display setting the player changes.
enum class DisplaySetting : std::uint8_t { Resolution, Window };

/// `setting` moved one place, back for -1 and on for 1.
struct DisplayPress {
  DisplaySetting setting = DisplaySetting::Resolution;
  int step = 0;
};

/// The key press a click stands for.
using Press = std::variant<OptionPress, MenuKey, PlayerPress, DishPress,
                           VolumePress, DisplayPress>;

/// A place on the screen that a click acts on, the press it stands for, and
/// why it does nothing now; `refusal` is empty while it acts.
struct Target {
  godot::Rect2 area;
  Press press;
  godot::String refusal = godot::String();
};

/// The last of `targets` under `point`, since later targets are drawn over
/// earlier ones; null over none.
[[nodiscard]] const Target *target_at(const std::vector<Target> &targets,
                                      godot::Vector2 point);

/// Names `press` for the Godot checks, such as `option_2`,
/// `appraiser_interact`, `volume_effects_3` or `display_window_next`.
[[nodiscard]] godot::String press_name(const Press &press);

/// Lights the target under `pointer` and writes its refusal over it, and
/// leaves every target on `canvas` as the `pointer_targets` meta: an array of
/// dictionaries with its `rect`, `press`, `refusal` and whether it is
/// `hovered`.
void paint_pointer(godot::Node2D &canvas, const rendering::Art &drawing,
                   const std::vector<Target> &targets,
                   std::optional<godot::Vector2> pointer);

} // namespace neocity::hud
