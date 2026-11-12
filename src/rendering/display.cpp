#include "display.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/vector2i.hpp>

#include <algorithm>
#include <cstdlib>

namespace neocity::rendering {
namespace {

using godot::Variant;

constexpr const char *SECTION = "display";
constexpr const char *WINDOWED = "windowed";
constexpr const char *FULLSCREEN = "fullscreen";
/// Godot's `Window.MODE_WINDOWED` and `Window.MODE_FULLSCREEN`.
constexpr int GODOT_WINDOWED = 0;
constexpr int GODOT_FULLSCREEN = 3;

/// A config file the trimmed bindings have no class for, made through the
/// engine's `ClassDB`; empty when the engine cannot make one.
Variant new_config() {
  godot::Object *classes =
      godot::Engine::get_singleton()->get_singleton("ClassDB");
  return classes == nullptr ? Variant()
                            : classes->call("instantiate", "ConfigFile");
}

} // namespace

Display load_display(const godot::String &file) {
  Display display;
  const Variant config = new_config();
  godot::Object *object = config;
  if (object == nullptr ||
      static_cast<int>(object->call("load", file)) != godot::OK) {
    return display;
  }
  const Variant width = object->call("get_value", SECTION, "width", -1);
  const Variant height = object->call("get_value", SECTION, "height", -1);
  if (width.get_type() == Variant::INT && height.get_type() == Variant::INT) {
    for (std::size_t index = 0; index < RESOLUTIONS.size(); ++index) {
      const auto &choice = RESOLUTIONS.at(index);
      if (choice.width == static_cast<int>(width) &&
          choice.height == static_cast<int>(height)) {
        display.resolution = index;
      }
    }
  }
  const Variant mode = object->call("get_value", SECTION, "mode", "");
  if (mode.get_type() == Variant::STRING) {
    const godot::String name = mode;
    if (name == WINDOWED) {
      display.mode = WindowMode::Windowed;
    } else if (name == FULLSCREEN) {
      display.mode = WindowMode::Fullscreen;
    }
  }
  return display;
}

bool save_display(const Display &display, const godot::String &file) {
  const Variant config = new_config();
  godot::Object *object = config;
  if (object == nullptr) {
    return false;
  }
  const auto &size = RESOLUTIONS.at(display.resolution);
  object->call("set_value", SECTION, "width", size.width);
  object->call("set_value", SECTION, "height", size.height);
  object->call("set_value", SECTION, "mode", mode_label(display.mode));
  return static_cast<int>(object->call("save", file)) == godot::OK;
}

void apply_display(godot::Node &node, const Display &display) {
  const Variant found = node.call("get_window");
  godot::Object *window = found;
  if (window == nullptr) {
    return;
  }
  if (display.mode == WindowMode::Fullscreen) {
    window->set("mode", GODOT_FULLSCREEN);
    return;
  }
  const auto &size = RESOLUTIONS.at(display.resolution);
  window->set("mode", GODOT_WINDOWED);
  window->set("size", godot::Vector2i(size.width, size.height));
  window->call("move_to_center");
}

Display stepped_resolution(Display display, int step) {
  const auto last = static_cast<int>(RESOLUTIONS.size()) - 1;
  display.resolution = static_cast<std::size_t>(
      std::clamp(static_cast<int>(display.resolution) + step, 0, last));
  return display;
}

Display toggled_mode(Display display) {
  display.mode = display.mode == WindowMode::Windowed ? WindowMode::Fullscreen
                                                      : WindowMode::Windowed;
  return display;
}

godot::String resolution_label(std::size_t index) {
  const auto &size = RESOLUTIONS.at(index);
  return godot::String::num_int64(size.width) + "x" +
         godot::String::num_int64(size.height);
}

godot::String mode_label(WindowMode mode) {
  switch (mode) {
  case WindowMode::Windowed:
    return WINDOWED;
  case WindowMode::Fullscreen:
    return FULLSCREEN;
  }
  std::abort();
}

} // namespace neocity::rendering
