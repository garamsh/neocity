#include "drawing.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/error_macros.hpp>
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

/// A frame's layers, outside in: a dark line, a light rim, its shaded lower
/// part, and the inner dark line, each on whole pixels.
constexpr float FRAME_LINE = 2.0F;
constexpr float FRAME_RIM = 3.0F;
/// How far a frame's corners are cut back, so they read as pixel-rounded.
constexpr float FRAME_NOTCH = 2.0F;
/// The darker lip along a marked button's bottom edge.
constexpr float LIP = 4.0F;
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
  case Image::Deck:
    return "walkway/deck.png";
  case Image::DeckRivets:
    return "walkway/deck_rivets.png";
  case Image::DeckWorn:
    return "walkway/deck_worn.png";
  case Image::DeckCorroded:
    return "walkway/deck_corroded.png";
  case Image::FleshDrip:
    return "walkway/flesh_drip.png";
  case Image::EndLeft:
    return "walkway/end_left.png";
  case Image::EndRight:
    return "walkway/end_right.png";
  case Image::Pillar:
    return "walkway/pillar.png";
  case Image::Column:
    return "walkway/column.png";
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
  case Image::Kiosk:
    return "world/kiosk.png";
  case Image::ColdStore:
    return "world/cold_store.png";
  case Image::Ferry:
    return "world/ferry.png";
  case Image::Platform:
    return "world/platform.png";
  case Image::Bridge:
    return "world/bridge.png";
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
  case Image::HeartFull:
    return "ui/heart_full.png";
  case Image::HeartEmpty:
    return "ui/heart_empty.png";
  case Image::IconDay:
    return "ui/icon_day.png";
  case Image::IconAcid:
    return "ui/icon_acid.png";
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
  case Image::ToolMagnifier:
    return "icons/tool_magnifier.png";
  case Image::ToolSpatula:
    return "icons/tool_spatula.png";
  case Image::ToolWrench:
    return "icons/tool_wrench.png";
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

/// What a source is, as its name ends.
const char *source_word(survival::SourceKind kind) {
  switch (kind) {
  case survival::SourceKind::VendingMachine:
    return "machine";
  case survival::SourceKind::Npc:
    return "cart";
  case survival::SourceKind::Shop:
    return "shop";
  case survival::SourceKind::Bag:
    return "bag";
  }
  std::abort();
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

/// The dark line and light rim every frame shares, with `fill` inside them.
void frame(godot::Node2D &canvas, godot::Rect2 area, godot::Color rim,
           godot::Color fill) {
  const auto tones = palette();
  notched(canvas, area, FRAME_NOTCH, tones.outline);
  const auto lit = area.grow(-FRAME_LINE);
  notched(canvas, lit, FRAME_NOTCH, rim);
  // The rim's lower part is in shade, lit from above like the art.
  notched(canvas, lit.grow_individual(0.0F, -lit.size.y / 2.0F, 0.0F, 0.0F),
          FRAME_NOTCH, rim.darkened(0.25F));
  notched(canvas, lit.grow(-FRAME_RIM), 1.0F, tones.outline);
  canvas.draw_rect(lit.grow(-FRAME_RIM - 1.0F), fill);
}

} // namespace

void button(godot::Node2D &canvas, godot::Rect2 area, bool marked) {
  const auto tones = palette();
  if (!marked) {
    frame(canvas, area, tones.rim, tones.raised);
    return;
  }
  frame(canvas, area, tones.gold.lightened(0.3F), tones.gold);
  const auto inner = area.grow(-FRAME_LINE - FRAME_RIM - 1.0F);
  canvas.draw_rect(
      inner.grow_individual(0.0F, LIP - inner.size.y, 0.0F, 0.0F).abs(),
      color(204, 136, 28, 1.0F));
}

void panel(godot::Node2D &canvas, godot::Rect2 area) {
  const auto tones = palette();
  frame(canvas, area, tones.rim, tones.surface);
}

void inset(godot::Node2D &canvas, godot::Rect2 area) {
  const auto tones = palette();
  notched(canvas, area, 1.0F, tones.outline);
  canvas.draw_rect(area.grow_individual(-2.0F, -2.0F, -1.0F, -1.0F).round(),
                   tones.background);
}

godot::String place_name(const survival::GameView &view,
                         survival::ObjectId object) {
  const auto &objects = view.world.objects;
  const auto &sources = view.exploration.sources;
  const auto found =
      std::ranges::find(objects, object, &survival::ObjectSnapshot::id);
  const auto stock =
      std::ranges::find(sources, object, &survival::SourceStock::id);
  const auto named = [](std::string_view text) {
    return godot::String::utf8(text.data(),
                               static_cast<std::int64_t>(text.size()));
  };
  if (found == objects.end() || stock == sources.end()) {
    return named(survival::object_name(object));
  }
  if (const auto &tool = found->tool) {
    return named(survival::tool_name(*tool)) + " " + source_word(stock->kind);
  }
  if (const auto &ingredient = found->ingredient) {
    return named(survival::ingredient_name(*ingredient)) + " " +
           source_word(stock->kind);
  }
  return named(survival::object_name(object));
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
    : images(std::move(pictures)), font(std::move(face)) {}

Art Art::load() {
  const godot::String file =
      godot::String("res://levels/monster_mouth_escape/fonts/") + FONT_FILE;
  godot::Ref<godot::Font> font =
      godot::ResourceLoader::get_singleton()->load(file);
  CRASH_COND_MSG(font.is_null(), "Font must load: " + file);
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

void Art::image(godot::Node2D &canvas, Image image, Vector2 position,
                float scale, bool mirrored, godot::Color tint) const {
  const Vector2 drawn = size(image) * scale;
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

void Art::image_part(godot::Node2D &canvas, Image image, godot::Rect2 source,
                     godot::Rect2 destination, godot::Color tint) const {
  canvas.draw_texture_rect_region(images.at(slot(image)), destination, source,
                                  tint);
}

Art::Tween &Art::tween(const godot::String &name, float target) const {
  const auto time = now();
  const auto frame = static_cast<std::uint64_t>(
      godot::Engine::get_singleton()->get_frames_drawn());
  auto &kept = tweens[name];
  // A value missing from the last drawn frame was off screen: it starts over.
  if (kept.seen == 0 || frame > kept.frame + 1) {
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
  return images.at(slot(image))->get_size();
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
