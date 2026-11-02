#pragma once

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>

#include <cstddef>
#include <cstdint>

namespace neocity::rendering {

godot::Color color(std::uint8_t r, std::uint8_t g, std::uint8_t b, float a);
godot::Rect2 rect(float x, float y, float w, float h);

/// Text and accent colors shared by the world and the HUD.
struct Palette {
  godot::Color white;
  godot::Color muted;
  godot::Color cyan;
  godot::Color gold;
};

Palette palette();

/// Draws text and the ascent atlas's sprites onto a Godot canvas.
class AscentDrawing {
public:
  /// Loads the ascent atlas and Godot's fallback font.
  static AscentDrawing load();

  void text(godot::Node2D &canvas, float x, float y, const godot::String &text,
            int size, godot::Color tint) const;
  /// How wide `text` draws at `size`.
  [[nodiscard]] float text_width(const godot::String &text, int size) const;
  void sprite(godot::Node2D &canvas, std::size_t index,
              godot::Rect2 destination, bool flip) const;

private:
  AscentDrawing(godot::Ref<godot::Texture2D> sprites,
                godot::Ref<godot::Font> fallback_font);

  godot::Ref<godot::Texture2D> atlas;
  godot::Ref<godot::Font> font;
};

} // namespace neocity::rendering
