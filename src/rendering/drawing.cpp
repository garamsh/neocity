#include "drawing.hpp"

#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/core/error_macros.hpp>

#include <cstddef>
#include <cstdlib>
#include <utility>

namespace neocity::rendering {

namespace {

using godot::Vector2;

constexpr auto IMAGE_COUNT = static_cast<std::size_t>(Image::LampBent) + 1;

const char *path(Image image) {
  switch (image) {
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
  case Image::ChefClimb:
    return "characters/chef_climb.png";
  case Image::AppraiserIdle:
    return "characters/appraiser_idle.png";
  case Image::AppraiserWalk1:
    return "characters/appraiser_walk_1.png";
  case Image::AppraiserWalk2:
    return "characters/appraiser_walk_2.png";
  case Image::AppraiserClimb:
    return "characters/appraiser_climb.png";
  case Image::Road:
    return "ground/road.png";
  case Image::CrackedRoad:
    return "ground/road_cracked.png";
  case Image::Pothole:
    return "ground/pothole.png";
  case Image::RoadEndLeft:
    return "ground/road_end_left.png";
  case Image::RoadEndRight:
    return "ground/road_end_right.png";
  case Image::TiltedSlab:
    return "ground/slab_tilted.png";
  case Image::Rubble:
    return "ground/rubble.png";
  case Image::PlankBridge:
    return "ground/bridge_plank.png";
  case Image::BuriedCar:
    return "ground/car_buried.png";
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
  case Image::BagRoof:
    return "world/bag_roof.png";
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
  case Image::TrainCarLit:
    return "decor/train_car_lit.png";
  case Image::TrainCarWrecked:
    return "decor/train_car_wrecked.png";
  case Image::FacadeTall:
    return "decor/facade_tall.png";
  case Image::FacadeLow:
    return "decor/facade_low.png";
  case Image::CablesSagging:
    return "decor/cables_sagging.png";
  case Image::CablesHanging:
    return "decor/cables_hanging.png";
  case Image::PipeBent:
    return "decor/pipe_bent.png";
  case Image::PipesValve:
    return "decor/pipes_valve.png";
  case Image::NeonBox:
    return "decor/neon_box.png";
  case Image::NeonBlade:
    return "decor/neon_blade.png";
  case Image::FleshVines:
    return "decor/flesh_vines.png";
  case Image::FleshTendril:
    return "decor/flesh_tendril.png";
  case Image::FleshTumor:
    return "decor/flesh_tumor.png";
  case Image::FleshPolyps:
    return "decor/flesh_polyps.png";
  case Image::RubbleLow:
    return "decor/rubble_low.png";
  case Image::RubbleHigh:
    return "decor/rubble_high.png";
  case Image::PillarSnapped:
    return "decor/pillar_snapped.png";
  case Image::PillarDeck:
    return "decor/pillar_deck.png";
  case Image::LampStanding:
    return "decor/lamp_standing.png";
  case Image::LampBent:
    return "decor/lamp_bent.png";
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

Art::Art(std::vector<godot::Ref<godot::Texture2D>> pictures,
         godot::Ref<godot::Font> fallback_font)
    : images(std::move(pictures)), font(std::move(fallback_font)) {}

Art Art::load() {
  godot::Ref<godot::Font> font =
      godot::ThemeDB::get_singleton()->get_fallback_font();
  CRASH_COND_MSG(font.is_null(), "Godot fallback font must be available");
  std::vector<godot::Ref<godot::Texture2D>> pictures;
  pictures.reserve(IMAGE_COUNT);
  for (std::size_t image = 0; image < IMAGE_COUNT; ++image) {
    pictures.push_back(
        load_texture(godot::String("res://levels/monster_mouth_escape/art/") +
                     path(static_cast<Image>(image))));
  }
  return {std::move(pictures), std::move(font)};
}

void Art::text(godot::Node2D &canvas, float x, float y,
               const godot::String &text, int size, godot::Color tint) const {
  canvas.draw_string(font, Vector2(x, y), text,
                     godot::HORIZONTAL_ALIGNMENT_LEFT, -1, size, tint);
}

float Art::text_width(const godot::String &text, int size) const {
  return font->get_string_size(text, godot::HORIZONTAL_ALIGNMENT_LEFT, -1, size)
      .x;
}

void Art::image(godot::Node2D &canvas, Image image, Vector2 position,
                float scale, bool mirrored) const {
  const Vector2 drawn = size(image) * scale;
  if (!mirrored) {
    canvas.draw_texture_rect(images.at(slot(image)),
                             godot::Rect2(position.floor(), drawn), false);
    return;
  }
  // A mirroring transform flips the picture in place; nothing else on the
  // canvas draws under a transform, so it returns to the identity.
  canvas.draw_set_transform(position.floor() + Vector2(drawn.x, 0.0F), 0.0F,
                            Vector2(-1.0F, 1.0F));
  canvas.draw_texture_rect(images.at(slot(image)),
                           godot::Rect2(Vector2(), drawn), false);
  canvas.draw_set_transform(Vector2(), 0.0F, Vector2(1.0F, 1.0F));
}

void Art::image_part(godot::Node2D &canvas, Image image, godot::Rect2 source,
                     godot::Rect2 destination) const {
  canvas.draw_texture_rect_region(images.at(slot(image)), destination, source);
}

Vector2 Art::size(Image image) const {
  return images.at(slot(image))->get_size();
}

Motion Art::track(survival::Actor actor, survival::GridPosition cell,
                  std::chrono::microseconds world_time) const {
  auto &trail = trails.at(static_cast<std::size_t>(actor));
  if (world_time < trail.seen_at || trail.seen_at.count() < 0) {
    trail.motion = {.cell = cell, .moved_at = std::chrono::microseconds{-1}};
  } else if (cell != trail.motion.cell) {
    if (cell.x != trail.motion.cell.x) {
      trail.motion.facing_left = cell.x < trail.motion.cell.x;
    }
    trail.motion.second_step = !trail.motion.second_step;
    trail.motion.cell = cell;
    trail.motion.moved_at = world_time;
  }
  trail.seen_at = world_time;
  return trail.motion;
}

} // namespace neocity::rendering
