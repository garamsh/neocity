#pragma once

#include <gameplay/world_snapshot.hpp>

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

/// Paints world snapshots onto a Godot canvas with the ascent's resources.
class AscentDrawing {
public:
  /// Loads the ascent textures and Godot's fallback font.
  static AscentDrawing load();

  void paint(godot::Node2D &canvas, const gameplay::WorldSnapshot &world,
             float camera) const;

  void text(godot::Node2D &canvas, float x, float y, const godot::String &text,
            int size, godot::Color tint) const;
  void sprite(godot::Node2D &canvas, std::size_t index,
              godot::Rect2 destination, bool flip) const;

private:
  AscentDrawing(godot::Ref<godot::Texture2D> city,
                godot::Ref<godot::Texture2D> sprites,
                godot::Ref<godot::Font> fallback_font);

  void actor(godot::Node2D &canvas, gameplay::Actor actor, float camera,
             bool leader, float time) const;

  godot::Ref<godot::Texture2D> background;
  godot::Ref<godot::Texture2D> atlas;
  godot::Ref<godot::Font> font;
};

} // namespace neocity::rendering
