#include "drawing.hpp"

#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

#include <survival/content.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <numbers>
#include <string_view>
#include <utility>

namespace neocity::rendering {

namespace {

using godot::Vector2;

/// Points on each rounded corner.
constexpr int CORNER_STEPS = 6;
constexpr std::uint64_t GLIDE_MICROSECONDS = 280000;
constexpr std::uint64_t ENTRANCE_MICROSECONDS = 220000;

/// The interface's pixel face, drawn without smoothing on its own grid.
constexpr const char *FONT_FILE = "Tiny5-Regular.ttf";

/// `size` at the nearest whole multiple of the face's grid, one at least.
int on_grid(int size) {
  return std::max(1, (size + FONT_GRID / 2) / FONT_GRID) * FONT_GRID;
}

constexpr auto IMAGE_COUNT = static_cast<std::size_t>(Image::TitleLogo) + 1;

const char *path(Image image) {
  switch (image) {
  case Image::AquariumBackdrop:
    return "backgrounds/aquarium.png";
  case Image::TerminalBackdrop:
    return "world/terminal.png";
  case Image::AquariumDeck:
    return "walkway/aquarium_deck.png";
  case Image::InteriorKitchen:
    return "world/interior_kitchen.png";
  case Image::InteriorNoodle:
    return "world/interior_noodle.png";
  case Image::InteriorColdStore:
    return "world/interior_cold_store.png";
  case Image::SpanWreck:
    return "world/span_wreck.png";
  case Image::KioskRuined:
    return "world/kiosk_ruined.png";
  case Image::FireEscape:
    return "world/fire_escape.png";
  case Image::Throat:
    return "backgrounds/throat.png";
  case Image::City:
    return "backgrounds/city.png";
  case Image::Acid:
    return "backgrounds/acid.png";
  case Image::Kitchen:
    return "workshop/kitchen.png";
  case Image::ChefIdle:
    return "characters/chef_idle.png";
  case Image::ChefWalk1:
    return "characters/chef_walk_1.png";
  case Image::ChefWalk2:
    return "characters/chef_walk_2.png";
  case Image::ChefWalk3:
    return "characters/chef_walk_3.png";
  case Image::ChefWalk4:
    return "characters/chef_walk_4.png";
  case Image::ChefRun1:
    return "characters/chef_run_1.png";
  case Image::ChefRun2:
    return "characters/chef_run_2.png";
  case Image::ChefJump:
    return "characters/chef_jump.png";
  case Image::ChefFall:
    return "characters/chef_fall.png";
  case Image::ChefLand:
    return "characters/chef_land.png";
  case Image::ChefClimb:
    return "characters/chef_climb.png";
  case Image::AppraiserIdle:
    return "characters/appraiser_idle.png";
  case Image::AppraiserWalk1:
    return "characters/appraiser_walk_1.png";
  case Image::AppraiserWalk2:
    return "characters/appraiser_walk_2.png";
  case Image::AppraiserWalk3:
    return "characters/appraiser_walk_3.png";
  case Image::AppraiserWalk4:
    return "characters/appraiser_walk_4.png";
  case Image::AppraiserRun1:
    return "characters/appraiser_run_1.png";
  case Image::AppraiserRun2:
    return "characters/appraiser_run_2.png";
  case Image::AppraiserJump:
    return "characters/appraiser_jump.png";
  case Image::AppraiserFall:
    return "characters/appraiser_fall.png";
  case Image::AppraiserLand:
    return "characters/appraiser_land.png";
  case Image::AppraiserClimb:
    return "characters/appraiser_climb.png";
  case Image::HdChefIdle:
    return "characters/chef_idle.png";
  case Image::HdChefWalk1:
    return "characters/chef_walk_1.png";
  case Image::HdChefWalk2:
    return "characters/chef_walk_2.png";
  case Image::HdChefWalk3:
    return "characters/chef_walk_3.png";
  case Image::HdChefWalk4:
    return "characters/chef_walk_4.png";
  case Image::HdChefRun1:
    return "characters/chef_run_1.png";
  case Image::HdChefRun2:
    return "characters/chef_run_2.png";
  case Image::HdChefJump:
    return "characters/chef_jump.png";
  case Image::HdChefFall:
    return "characters/chef_fall.png";
  case Image::HdChefLand:
    return "characters/chef_land.png";
  case Image::HdChefClimb:
    return "characters/chef_climb.png";
  case Image::HdAppraiserIdle:
    return "characters/appraiser_idle.png";
  case Image::HdAppraiserWalk1:
    return "characters/appraiser_walk_1.png";
  case Image::HdAppraiserWalk2:
    return "characters/appraiser_walk_2.png";
  case Image::HdAppraiserWalk3:
    return "characters/appraiser_walk_3.png";
  case Image::HdAppraiserWalk4:
    return "characters/appraiser_walk_4.png";
  case Image::HdAppraiserRun1:
    return "characters/appraiser_run_1.png";
  case Image::HdAppraiserRun2:
    return "characters/appraiser_run_2.png";
  case Image::HdAppraiserJump:
    return "characters/appraiser_jump.png";
  case Image::HdAppraiserFall:
    return "characters/appraiser_fall.png";
  case Image::HdAppraiserLand:
    return "characters/appraiser_land.png";
  case Image::HdAppraiserClimb:
    return "characters/appraiser_climb.png";
  case Image::Deck:
    return "walkway/deck.png";
  case Image::Structure:
    return "walkway/structure.png";
  case Image::Rubble:
    return "ground/rubble.png";
  case Image::BuildingWall:
    return "world/building_wall.png";
  case Image::Shop:
    return "world/shop.png";
  case Image::VendingMachine:
    return "world/vending_machine.png";
  case Image::VendorCart:
    return "world/vendor_cart.png";
  case Image::NoodleVendor:
    return "world/noodle_vendor.png";
  case Image::BagWashedUp:
    return "world/bag_washed_up.png";
  case Image::Ladder:
    return "world/ladder.png";
  case Image::LadderBroken:
    return "world/ladder_broken.png";
  case Image::DoorColdStore:
    return "world/door_cold_store.png";
  case Image::DoorBroken:
    return "world/door_broken.png";
  case Image::AcidDrip:
    return "world/acid_drip.png";
  case Image::Kiosk:
    return "world/kiosk.png";
  case Image::ColdStore:
    return "world/cold_store.png";
  case Image::Ferry:
    return "world/ferry.png";
  case Image::IconDay:
    return "ui/icon_day.png";
  case Image::IconClock:
    return "ui/icon_clock.png";
  case Image::IngredientTomato:
    return "icons/ingredient_tomato.png";
  case Image::IngredientFish:
    return "icons/ingredient_fish.png";
  case Image::IngredientKelp:
    return "icons/ingredient_kelp.png";
  case Image::IngredientMushroom:
    return "icons/ingredient_mushroom.png";
  case Image::IngredientPickles:
    return "icons/ingredient_pickles.png";
  case Image::IngredientMeat:
    return "icons/ingredient_meat.png";
  case Image::IngredientTruffle:
    return "icons/ingredient_truffle.png";
  case Image::DishMushroomSoup:
    return "icons/dish_mushroom_soup.png";
  case Image::DishFishStew:
    return "icons/dish_fish_stew.png";
  case Image::DishKelpWrap:
    return "icons/dish_kelp_wrap.png";
  case Image::DishMeatSkewer:
    return "icons/dish_meat_skewer.png";
  case Image::DishTruffleRoast:
    return "icons/dish_truffle_roast.png";
  case Image::DishPickleSurprise:
    return "icons/dish_pickle_surprise.png";
  case Image::DishSeaSalad:
    return "icons/dish_sea_salad.png";
  case Image::DishFishPie:
    return "icons/dish_fish_pie.png";
  case Image::DishMeatStew:
    return "icons/dish_meat_stew.png";
  case Image::DishTruffleSoup:
    return "icons/dish_truffle_soup.png";
  case Image::DishSurfAndTurf:
    return "icons/dish_surf_and_turf.png";
  case Image::FramePanel:
    return "ui/frame_panel.png";
  case Image::FrameButton:
    return "ui/frame_button.png";
  case Image::FrameSlot:
    return "ui/frame_slot.png";
  case Image::TitleKeyArt:
    return "title/key_art.png";
  case Image::TitleLogo:
    return "title/logo.png";
  }
  std::abort();
}

godot::Ref<godot::Texture2D> load_texture(const godot::String &path) {
  const godot::Ref<godot::Texture2D> texture =
      godot::ResourceLoader::get_singleton()->load(path);
  CRASH_COND_MSG(texture.is_null(), "Art must load: " + path);
  return texture;
}

std::size_t slot(Image image) { return static_cast<std::size_t>(image); }

/// HD counterparts keep the old layout size while retiring the canvas upscale.
bool hd(Image image) {
  const godot::String folder = godot::String(path(image)).get_slice("/", 0);
  return folder == "backgrounds" || folder == "walkway" || folder == "world" ||
         folder == "ground" ||
         (image >= Image::HdChefIdle && image <= Image::HdAppraiserClimb);
}

/// Native picture pixels per layout unit, before the camera and canvas. The
/// noodle vendor was drawn at the layout size and keeps it.
float density(Image image) {
  const godot::String folder = godot::String(path(image)).get_slice("/", 0);
  const bool world = folder == "walkway" || folder == "world" ||
                     folder == "ground" || folder == "characters";
  const bool cell_dense = world && image != Image::NoodleVendor;
  return (cell_dense ? 2.0F : 1.0F) * (hd(image) ? ART_SCALE : 1.0F);
}

/// The outline of `area` with its corners rounded by `radius`, clockwise from
/// the top-left corner.
godot::PackedVector2Array rounded_outline(godot::Rect2 area, float radius) {
  const float r = std::min(radius, std::min(area.size.x, area.size.y) / 2.0F);
  const Vector2 far = area.position + area.size;
  constexpr float QUARTER = std::numbers::pi_v<float> / 2.0F;
  const std::array<std::pair<Vector2, float>, 4> corners{
      {{Vector2(area.position.x + r, area.position.y + r), 2.0F * QUARTER},
       {Vector2(far.x - r, area.position.y + r), 3.0F * QUARTER},
       {Vector2(far.x - r, far.y - r), 0.0F},
       {Vector2(area.position.x + r, far.y - r), QUARTER}}};
  godot::PackedVector2Array points;
  for (const auto &[centre, start] : corners) {
    for (int step = 0; step <= CORNER_STEPS; ++step) {
      const float angle = start + QUARTER * static_cast<float>(step) /
                                      static_cast<float>(CORNER_STEPS);
      points.push_back(centre + Vector2(std::cos(angle), std::sin(angle)) * r);
    }
  }
  return points;
}

/// Ease-out cubic: fast at first, settling at 1.
float ease_out(float progress) {
  const float rest = 1.0F - std::clamp(progress, 0.0F, 1.0F);
  return 1.0F - rest * rest * rest;
}

/// Microseconds of a steady real clock; it drives only how the screen moves.
std::uint64_t now() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
}

} // namespace

godot::Color color(std::uint8_t r, std::uint8_t g, std::uint8_t b, float a) {
  return {static_cast<float>(r) / 255.0F, static_cast<float>(g) / 255.0F,
          static_cast<float>(b) / 255.0F, a};
}

godot::Rect2 rect(float x, float y, float w, float h) { return {x, y, w, h}; }

void rounded(godot::Node2D &canvas, godot::Rect2 area, float radius,
             godot::Color fill) {
  canvas.draw_colored_polygon(rounded_outline(area, radius), fill);
}

namespace {

/// Fills `area` with its corners cut back by `notch`, on whole pixels.
void notched(godot::Node2D &canvas, godot::Rect2 area, float notch,
             godot::Color fill) {
  const godot::Rect2 whole(area.position.round(), area.size.round());
  canvas.draw_rect(whole.grow_individual(-notch, 0.0F, -notch, 0.0F), fill);
  canvas.draw_rect(whole.grow_individual(0.0F, -notch, 0.0F, -notch), fill);
}

/// Repeats a cut frame's edge pixels between its native nine-pixel corners.
void frame(godot::Node2D &canvas, godot::Rect2 area, std::size_t kind) {
  area = area.round();
  static constexpr std::array frames{Image::FramePanel, Image::FrameButton,
                                     Image::FrameSlot};
  const auto texture =
      load_texture(godot::String("res://levels/monster_mouth_escape/art/") +
                   path(frames.at(kind)));
  constexpr float EDGE = 9.0F;
  const Vector2 native = texture->get_size();
  const std::array source_x{0.0F, EDGE, native.x - EDGE};
  const std::array source_y{0.0F, EDGE, native.y - EDGE};
  const std::array source_width{EDGE, 1.0F, EDGE};
  const std::array source_height{EDGE, 1.0F, EDGE};
  const std::array target_x{area.position.x, area.position.x + EDGE,
                            area.get_end().x - EDGE};
  const std::array target_y{area.position.y, area.position.y + EDGE,
                            area.get_end().y - EDGE};
  const std::array target_width{EDGE, area.size.x - 2.0F * EDGE, EDGE};
  const std::array target_height{EDGE, area.size.y - 2.0F * EDGE, EDGE};
  for (std::size_t y = 0; y < 3; ++y) {
    for (std::size_t x = 0; x < 3; ++x) {
      canvas.draw_texture_rect_region(
          texture,
          rect(target_x.at(x), target_y.at(y), target_width.at(x),
               target_height.at(y)),
          rect(source_x.at(x), source_y.at(y), source_width.at(x),
               source_height.at(y)));
    }
  }
  godot::Dictionary drawn =
      canvas.get_meta("interface_frames", godot::Dictionary());
  godot::Dictionary observation;
  observation.set("texture", texture);
  observation.set("area", area);
  drawn.set(texture->get_path(), observation);
  canvas.set_meta("interface_frames", drawn);
}

} // namespace

void button(godot::Node2D &canvas, godot::Rect2 area, bool marked) {
  frame(canvas, area, 1);
  if (marked) {
    canvas.draw_rect(rect(area.position.x + 9.0F, area.position.y + 12.0F, 2.0F,
                          std::max(2.0F, area.size.y - 24.0F)),
                     palette().gold);
  }
}

void panel(godot::Node2D &canvas, godot::Rect2 area) { frame(canvas, area, 0); }

void inset(godot::Node2D &canvas, godot::Rect2 area) {
  const auto tones = palette();
  if (area.size.x >= 32.0F && area.size.y >= 32.0F) {
    frame(canvas, area, 2);
    return;
  }
  notched(canvas, area, 1.0F, tones.outline);
  canvas.draw_rect(area.grow_individual(-2.0F, -2.0F, -1.0F, -1.0F).round(),
                   tones.background);
}

godot::String place_name(const survival::GameView &view,
                         survival::ObjectId object) {
  return godot::String::utf8(
      survival::place_display_name(view, object).c_str());
}

Palette palette() {
  return {.white = color(240, 241, 250, 1.0F),
          .muted = color(166, 174, 204, 1.0F),
          .cyan = color(91, 226, 238, 1.0F),
          .gold = color(255, 196, 70, 1.0F),
          .ink = color(28, 22, 12, 1.0F),
          .background = color(9, 10, 20, 1.0F),
          .surface = color(24, 24, 38, 1.0F),
          .raised = color(42, 44, 66, 1.0F),
          .outline = color(8, 8, 16, 1.0F),
          .rim = color(150, 154, 178, 1.0F),
          .faint = color(122, 130, 164, 1.0F),
          .danger = color(255, 104, 104, 1.0F),
          .success = color(140, 235, 92, 1.0F)};
}

Art::Art(std::vector<godot::Ref<godot::Texture2D>> pictures,
         godot::Ref<godot::Font> face)
    : images(std::move(pictures)), font(std::move(face)), loaded(now()) {}

Art Art::load() {
  const godot::String file =
      godot::String("res://levels/monster_mouth_escape/fonts/") + FONT_FILE;
  godot::Ref<godot::Font> font =
      godot::ResourceLoader::get_singleton()->load(file);
  CRASH_COND_MSG(font.is_null(), "Font must load: " + file);
  std::vector<godot::Ref<godot::Texture2D>> pictures;
  pictures.reserve(IMAGE_COUNT);
  for (std::size_t image = 0; image < IMAGE_COUNT; ++image) {
    const auto picture = static_cast<Image>(image);
    pictures.push_back(load_texture(
        godot::String(hd(picture) ? "res://levels/monster_mouth_escape/art_hd/"
                                  : "res://levels/monster_mouth_escape/art/") +
        path(picture)));
  }
  return {std::move(pictures), std::move(font)};
}

void Art::text(godot::Node2D &canvas, float x, float y,
               const godot::String &text, int size, godot::Color tint,
               Weight weight) const {
  const Vector2 at = Vector2(x, y).round();
  const int drawn = on_grid(size);
  sizes.push_back(drawn);
  canvas.draw_string(font, at, text, godot::HORIZONTAL_ALIGNMENT_LEFT, -1,
                     drawn, tint);
  if (weight == Weight::Bold) {
    canvas.draw_string(font, at + Vector2(1.0F, 0.0F), text,
                       godot::HORIZONTAL_ALIGNMENT_LEFT, -1, drawn, tint);
  }
  const Vector2 end = at + Vector2(text_width(text, size, weight), 0.0F);
  if (!written.empty() && written.back().end.is_equal_approx(at)) {
    written.back() = {.text = written.back().text + text, .end = end};
  } else {
    written.push_back({.text = text, .end = end});
  }
}

float Art::text_width(const godot::String &text, int size,
                      Weight weight) const {
  const float width =
      font->get_string_size(text, godot::HORIZONTAL_ALIGNMENT_LEFT, -1,
                            on_grid(size))
          .x;
  return weight == Weight::Bold ? width + 1.0F : width;
}

std::vector<godot::String> Art::take_written() const {
  std::vector<godot::String> lines;
  lines.reserve(written.size());
  for (auto &line : written) {
    lines.push_back(std::move(line.text));
  }
  written.clear();
  return lines;
}

std::vector<int> Art::take_sizes() const { return std::exchange(sizes, {}); }

void Art::note_foreground(godot::Node2D &canvas, Image image,
                          godot::Rect2 source, godot::Rect2 destination,
                          bool mirrored) const {
  const godot::String folder = godot::String(path(image)).get_slice("/", 0);
  if (!canvas.has_meta("foreground_layers") ||
      (folder != "world" && folder != "ground" && folder != "walkway")) {
    return;
  }
  godot::Array layers = canvas.get_meta("foreground_layers", godot::Array());
  godot::Dictionary layer;
  layer.set("texture", images.at(slot(image)));
  layer.set("source", source);
  layer.set("destination", destination);
  layer.set("transform", placed);
  layer.set("mirrored", mirrored);
  layers.push_back(layer);
  canvas.set_meta("foreground_layers", layers);
}

void Art::image(godot::Node2D &canvas, Image image, Vector2 position,
                float scale, bool mirrored, godot::Color tint) const {
  const Vector2 drawn = size(image) * scale;
  note_foreground(canvas, image,
                  godot::Rect2(Vector2(), images.at(slot(image))->get_size()),
                  godot::Rect2(position.floor(), drawn), mirrored);
  if (canvas.has_meta("character_layers") &&
      godot::String(path(image)).get_slice("/", 0) == "characters") {
    godot::Array layers = canvas.get_meta("character_layers", godot::Array());
    godot::Dictionary layer;
    layer.set("role", godot::String(path(image)).get_file().begins_with("chef_")
                          ? "chef"
                          : "appraiser");
    layer.set("texture", images.at(slot(image)));
    const Vector2 zoom = placed.get_scale();
    layer.set("area",
              godot::Rect2(placed.get_origin() + position.floor() * zoom,
                           drawn * zoom));
    layer.set("mirrored", mirrored);
    layers.push_back(layer);
    canvas.set_meta("character_layers", layers);
  }
  if (!mirrored) {
    canvas.draw_texture_rect(images.at(slot(image)),
                             godot::Rect2(position.floor(), drawn), false,
                             tint);
    return;
  }
  // A mirroring transform flips the picture in place, then the canvas returns
  // to the last shift or look.
  canvas.draw_set_transform_matrix(
      placed * godot::Transform2D(0.0F, Vector2(-1.0F, 1.0F), 0.0F,
                                  position.floor() + Vector2(drawn.x, 0.0F)));
  canvas.draw_texture_rect(images.at(slot(image)),
                           godot::Rect2(Vector2(), drawn), false, tint);
  canvas.draw_set_transform_matrix(placed);
}

void Art::shift(godot::Node2D &canvas, Vector2 offset) const {
  shaken = offset.round();
  placed = godot::Transform2D(0.0F, shaken);
  canvas.draw_set_transform_matrix(placed);
}

void Art::look(godot::Node2D &canvas, Vector2 origin, float zoom) const {
  placed = godot::Transform2D(0.0F, Vector2(zoom, zoom), 0.0F, shaken + origin);
  canvas.draw_set_transform_matrix(placed);
}

Look Art::looking() const {
  return {.origin = placed.get_origin() - shaken, .zoom = placed.get_scale().x};
}

void Art::image_part(godot::Node2D &canvas, Image image, godot::Rect2 source,
                     godot::Rect2 destination, godot::Color tint) const {
  const float scale = density(image);
  note_foreground(canvas, image,
                  godot::Rect2(source.position * scale, source.size * scale),
                  destination);
  if ((image == Image::Deck || image == Image::AquariumDeck) &&
      canvas.has_meta("deck_layers")) {
    godot::Array layers = canvas.get_meta("deck_layers", godot::Array());
    godot::Dictionary layer;
    layer.set("texture", images.at(slot(image)));
    layer.set("source",
              godot::Rect2(source.position * scale, source.size * scale));
    layer.set("destination", destination);
    layer.set("transform", placed);
    layers.push_back(layer);
    canvas.set_meta("deck_layers", layers);
  }
  const bool mirrored = destination.size.x < 0.0F || destination.size.y < 0.0F;
  if (mirrored) {
    canvas.draw_set_transform_matrix(
        placed *
        godot::Transform2D(0.0F,
                           Vector2(destination.size.x < 0.0F ? -1.0F : 1.0F,
                                   destination.size.y < 0.0F ? -1.0F : 1.0F),
                           0.0F, destination.position));
    destination = godot::Rect2(Vector2(), destination.size.abs());
  }
  canvas.draw_texture_rect_region(
      images.at(slot(image)), destination,
      godot::Rect2(source.position * scale, source.size * scale), tint);
  if (mirrored) {
    canvas.draw_set_transform_matrix(placed);
  }
}

void Art::next_frame() { ++frame; }

Art::Tween &Art::tween(const godot::String &name, float target) const {
  const auto time = clock();
  auto [found, fresh] = tweens.try_emplace(name);
  auto &kept = found->second;
  // A value missing from the frame before was off screen: it starts over.
  if (fresh || kept.frame + 1 < frame) {
    kept = {.from = target, .to = target, .started = time};
  }
  kept.seen = time;
  kept.frame = frame;
  return kept;
}

float Art::glide(const godot::String &name, float target) const {
  auto &kept = tween(name, target);
  const auto progress = [&kept] {
    return ease_out(static_cast<float>(kept.seen - kept.started) /
                    static_cast<float>(GLIDE_MICROSECONDS));
  };
  if (target != kept.to) {
    kept.from += (kept.to - kept.from) * progress();
    kept.to = target;
    kept.started = kept.seen;
  }
  return kept.from + (kept.to - kept.from) * progress();
}

float Art::entrance(const godot::String &name) const {
  const auto &kept = tween(name, 0.0F);
  return ease_out(static_cast<float>(kept.seen - kept.started) /
                  static_cast<float>(ENTRANCE_MICROSECONDS));
}

float Art::shown_for(const godot::String &name) const {
  const auto &kept = tween(name, 0.0F);
  return static_cast<float>(kept.seen - kept.started) / 1.0e6F;
}

Vector2 Art::size(Image image) const {
  return images.at(slot(image))->get_size() / density(image);
}

float Art::seconds() const {
  return pinned.value_or(static_cast<float>(now() - loaded) / 1.0e6F);
}

void Art::pin(float held) { pinned = held; }

std::uint64_t Art::clock() const {
  if (pinned) {
    return static_cast<std::uint64_t>(
        std::llround(static_cast<double>(std::max(*pinned, 0.0F)) * 1.0e6));
  }
  return now() - loaded;
}

void Art::note_waterline(float band, float band_left, float sea) const {
  const float zoom = placed.get_scale().x;
  waterline = Waterline{.band = shaken.y + band,
                        .band_left = shaken.x + band_left,
                        .sea = placed.get_origin().y + sea * zoom,
                        .origin = placed.get_origin(),
                        .zoom = zoom};
}

std::optional<Waterline> Art::take_waterline() const {
  return std::exchange(waterline, std::nullopt);
}

Motion Art::track(survival::Actor actor, const survival::Body &body,
                  std::chrono::microseconds world_time) const {
  auto &trail = trails.at(static_cast<std::size_t>(actor));
  if (world_time < trail.seen_at || trail.seen_at.count() < 0) {
    trail = {.motion = {},
             .x = body.position.x,
             .x_at = world_time,
             .stance = body.stance,
             .seen_at = world_time};
    return trail.motion;
  }
  // Movement comes in whole units of several physics steps, so the speed is
  // the distance of a change of position over the time since the last one.
  if (body.position.x != trail.x && world_time > trail.x_at) {
    const auto steps =
        static_cast<float>((world_time - trail.x_at).count()) /
        static_cast<float>(
            std::chrono::microseconds(survival::PHYSICS.tick).count());
    trail.motion.speed =
        static_cast<float>(std::abs(body.position.x - trail.x)) / steps;
    trail.motion.moved_at = world_time;
    trail.x = body.position.x;
    trail.x_at = world_time;
  }
  if (trail.stance == survival::Stance::Airborne &&
      body.stance == survival::Stance::Standing) {
    trail.motion.landed_at = world_time;
  }
  trail.stance = body.stance;
  trail.seen_at = world_time;
  return trail.motion;
}

} // namespace neocity::rendering
