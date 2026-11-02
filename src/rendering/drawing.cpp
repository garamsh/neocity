#include "drawing.hpp"

#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <array>
#include <utility>

namespace neocity::rendering {

namespace {

using godot::Vector2;

godot::Ref<godot::Texture2D> load_texture(const godot::String &path) {
  const godot::Ref<godot::Texture2D> texture =
      godot::ResourceLoader::get_singleton()->load(path);
  CRASH_COND_MSG(texture.is_null(), "Ascent texture must load: " + path);
  return texture;
}

} // namespace

godot::Color color(std::uint8_t r, std::uint8_t g, std::uint8_t b, float a) {
  return {static_cast<float>(r) / 255.0F, static_cast<float>(g) / 255.0F,
          static_cast<float>(b) / 255.0F, a};
}

godot::Rect2 rect(float x, float y, float w, float h) { return {x, y, w, h}; }

Palette palette() {
  return {.white = color(228, 234, 255, 1.0F),
          .muted = color(144, 159, 195, 1.0F),
          .cyan = color(91, 244, 219, 1.0F),
          .gold = color(255, 192, 83, 1.0F)};
}

AscentDrawing::AscentDrawing(godot::Ref<godot::Texture2D> sprites,
                             godot::Ref<godot::Font> fallback_font)
    : atlas(std::move(sprites)), font(std::move(fallback_font)) {}

AscentDrawing AscentDrawing::load() {
  godot::Ref<godot::Font> font =
      godot::ThemeDB::get_singleton()->get_fallback_font();
  CRASH_COND_MSG(font.is_null(), "Godot fallback font must be available");
  return {load_texture("res://levels/ascent/ascent_atlas.png"),
          std::move(font)};
}

void AscentDrawing::text(godot::Node2D &canvas, float x, float y,
                         const godot::String &text, int size,
                         godot::Color tint) const {
  canvas.draw_string(font, Vector2(x, y), text,
                     godot::HORIZONTAL_ALIGNMENT_LEFT, -1, size, tint);
}

float AscentDrawing::text_width(const godot::String &text, int size) const {
  return font->get_string_size(text, godot::HORIZONTAL_ALIGNMENT_LEFT, -1, size)
      .x;
}

void AscentDrawing::sprite(godot::Node2D &canvas, std::size_t index,
                           godot::Rect2 destination, bool flip) const {
  // Source rectangles are measured from the untouched atlas, which is not a
  // uniform grid.
  constexpr std::array<std::array<float, 4>, 8> bounds{{
      {88.0F, 90.0F, 354.0F, 408.0F},
      {494.0F, 64.0F, 397.0F, 364.0F},
      {990.0F, 156.0F, 256.0F, 342.0F},
      {1396.0F, 129.0F, 275.0F, 324.0F},
      {58.0F, 600.0F, 360.0F, 228.0F},
      {453.0F, 567.0F, 422.0F, 253.0F},
      {890.0F, 655.0F, 437.0F, 127.0F},
      {1458.0F, 598.0F, 224.0F, 238.0F},
  }};
  const auto &[x, y, w, h] = bounds.at(index);
  const godot::Rect2 source = rect(x, y, w, h);
  godot::Rect2 target = destination;
  if (flip) {
    target.position.x += target.size.x;
    target.size.x = -target.size.x;
  }
  canvas.draw_texture_rect_region(atlas, target, source);
}

} // namespace neocity::rendering
