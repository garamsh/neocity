#include "survival.hpp"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <optional>
#include <span>
#include <string_view>

namespace neocity::rendering {
namespace {

using namespace std::chrono_literals;
using godot::Vector2;
using survival::Actor;
using survival::DecorationKind;
using survival::GroundKind;
using survival::LandmarkKind;
using survival::ObjectKind;
using survival::ObjectSnapshot;
using survival::ObjectState;
using survival::SourceKind;

constexpr float CELL = 32.0F;
constexpr float MAP_X = 64.0F;
constexpr float MAP_Y = 70.0F;
constexpr float SCREEN_WIDTH = 1280.0F;
constexpr float SCREEN_HEIGHT = 720.0F;
/// The acid surface at acid 0: the top of row 14, below every ground row.
constexpr float ACID_LOW = MAP_Y + 14.0F * CELL;
/// How far below the acid picture's top edge its surface lies.
constexpr float ACID_SURFACE_DEPTH = 48.0F;
/// Below its surface the acid fades to its deep colour over this many steps
/// of this many pixels.
constexpr int ACID_FADE_STEPS = 6;
constexpr float ACID_FADE_STEP = 6.0F;
/// The rows of a ground tile's top edge, which a player walks on.
constexpr float GROUND_EDGE = 4.0F;
/// The distant city's bottom edge, under the acid at acid 0.
constexpr float CITY_BOTTOM = ACID_LOW + 48.0F;
/// The part of the kitchen picture the workshop shows between the top bar and
/// the panels, with the kitchen floor along its bottom.
constexpr float KITCHEN_SOURCE_Y = 364.0F;
constexpr float KITCHEN_Y = 62.0F;
constexpr float KITCHEN_HEIGHT = 256.0F;
constexpr float KITCHEN_FLOOR = 310.0F;
constexpr float WORKSHOP_SCALE = 2.0F;
constexpr Vector2 FEED_BUCKET(1205.0F, 202.0F);
/// The building wall and rubble pictures are two cells wide; the wall is two
/// cells tall.
constexpr std::int32_t TILE_CELLS = 2;
constexpr float DASH = 4.0F;
/// Clears the head of a character that rises a cell above its own.
constexpr float TAG_RISE = 38.0F;
/// Clears the chef's tag when the two actors share a cell.
constexpr float SHARED_CELL_TAG_SHIFT = 26.0F;
/// How long after a step an actor still shows a walking frame; a walking step
/// takes 100ms of world time.
constexpr auto STRIDE = 150ms;
/// Where the map's words may stand: below the top bar, above the actor cards.
constexpr float LABEL_TOP = 62.0F;
constexpr float LABEL_BOTTOM = 540.0F;
/// Clear space kept between two plates.
constexpr float PLATE_GAP = 2.0F;
/// How far a plate looks sideways per step when its spot is taken.
constexpr float PLATE_STEP = 16.0F;
constexpr int PLATE_RINGS = 12;

/// The scenery behind the play layer is darker and cooler than the play
/// layer, so the ground and what stands on it read first.
godot::Color throat_tint() { return color(80, 68, 114, 1.0F); }
godot::Color city_tint() { return color(66, 74, 128, 1.0F); }
godot::Color backdrop_tint() { return color(100, 92, 140, 1.0F); }
/// The acid below its glowing surface, so a high sea does not flood the
/// screen behind the map with bright green.
godot::Color deep_acid_tint() { return color(32, 52, 48, 1.0F); }
godot::Color untinted() { return {1.0F, 1.0F, 1.0F}; }
/// The ground is lifted above its own colours, and its walked-on top edge
/// most of all.
godot::Color ground_tint() { return {1.35F, 1.3F, 1.45F}; }
godot::Color ground_edge_tint() { return {2.4F, 2.3F, 2.6F}; }

float cell_x(std::int32_t x) { return MAP_X + static_cast<float>(x) * CELL; }
float cell_y(std::int32_t y) { return MAP_Y + static_cast<float>(y) * CELL; }

godot::Rect2 area_rect(const survival::WorldArea &area) {
  return rect(cell_x(area.x), cell_y(area.y),
              static_cast<float>(area.width) * CELL,
              static_cast<float>(area.height) * CELL);
}

bool covers(const survival::WorldArea &area, survival::GridPosition cell) {
  return cell.x >= area.x && cell.x < area.x + area.width && cell.y >= area.y &&
         cell.y < area.y + area.height;
}

godot::String from(std::string_view text) {
  return godot::String::utf8(text.data(),
                             static_cast<std::int64_t>(text.size()));
}

/// Draws `image` centred on `box` with its bottom edge on the box's floor.
void stand(godot::Node2D &canvas, const Art &art, Image image,
           const godot::Rect2 &box) {
  const Vector2 size = art.size(image);
  art.image(canvas, image,
            Vector2(box.position.x + (box.size.x - size.x) / 2.0F,
                    box.position.y + box.size.y - size.y));
}

/// Words on an opaque plate, whose contrast no acid level can change.
struct Plate {
  godot::Rect2 box;
  godot::String text;
  int size = 0;
  godot::Color tint;
};

/// The plate behind `text` at `size`, the left end of its baseline at `at`.
godot::Rect2 plate_box(const Art &art, Vector2 at, const godot::String &text,
                       int size) {
  const auto height = static_cast<float>(size);
  return {at.x - 2.0F, at.y - height, art.text_width(text, size) + 4.0F,
          height * 1.3F};
}

void paint_plate(godot::Node2D &canvas, const Art &art, const Plate &plate) {
  canvas.draw_rect(plate.box, color(14, 12, 26, 1.0F));
  art.text(canvas, plate.box.position.x + 2.0F,
           plate.box.position.y + static_cast<float>(plate.size), plate.text,
           plate.size, plate.tint);
}

/// The map's words, none covering another: each takes the spot it asks for,
/// or the nearest free one, looking farther away step by step.
class Plates {
public:
  /// Keeps words off `area`.
  void keep_clear(godot::Rect2 area) { clear.push_back(area); }

  /// Places `text` with the left end of its baseline at `at`, or nearby.
  void add(const Art &art, Vector2 at, const godot::String &text, int size,
           godot::Color tint) {
    const godot::Rect2 wanted = plate_box(art, at, text, size);
    static constexpr std::array DIRECTIONS{
        Vector2(0.0F, -1.0F), Vector2(0.0F, 1.0F),   Vector2(-1.0F, 0.0F),
        Vector2(1.0F, 0.0F),  Vector2(-1.0F, -1.0F), Vector2(1.0F, -1.0F)};
    const Vector2 pitch(PLATE_STEP, wanted.size.y + PLATE_GAP);
    godot::Rect2 box = wanted;
    for (int ring = 1; ring <= PLATE_RINGS && !vacant(box); ++ring) {
      for (const Vector2 direction : DIRECTIONS) {
        box = godot::Rect2(wanted.position +
                               direction * pitch * static_cast<float>(ring),
                           wanted.size);
        if (vacant(box)) {
          break;
        }
      }
    }
    if (!vacant(box)) {
      box = wanted;
    }
    placed.push_back({.box = box, .text = text, .size = size, .tint = tint});
  }

  void paint(godot::Node2D &canvas, const Art &art) const {
    for (const auto &plate : placed) {
      paint_plate(canvas, art, plate);
    }
  }

  [[nodiscard]] godot::Array boxes() const {
    godot::Array boxes;
    for (const auto &plate : placed) {
      boxes.push_back(plate.box);
    }
    return boxes;
  }

private:
  [[nodiscard]] bool vacant(const godot::Rect2 &box) const {
    const godot::Rect2 room(0.0F, LABEL_TOP, SCREEN_WIDTH,
                            LABEL_BOTTOM - LABEL_TOP);
    const godot::Rect2 spaced = box.grow(PLATE_GAP);
    return room.encloses(box) &&
           std::ranges::none_of(placed,
                                [&](const Plate &plate) {
                                  return spaced.intersects(plate.box);
                                }) &&
           std::ranges::none_of(clear, [&](const godot::Rect2 &area) {
             return box.intersects(area);
           });
  }

  std::vector<godot::Rect2> clear;
  std::vector<Plate> placed;
};

void dashed_frame(godot::Node2D &canvas, const godot::Rect2 &box,
                  godot::Color tint) {
  const Vector2 top_left = box.position;
  const Vector2 bottom_right = box.position + box.size;
  const Vector2 top_right(bottom_right.x, top_left.y);
  const Vector2 bottom_left(top_left.x, bottom_right.y);
  canvas.draw_dashed_line(top_left, top_right, tint, 2.0F, DASH);
  canvas.draw_dashed_line(top_right, bottom_right, tint, 2.0F, DASH);
  canvas.draw_dashed_line(bottom_right, bottom_left, tint, 2.0F, DASH);
  canvas.draw_dashed_line(bottom_left, top_left, tint, 2.0F, DASH);
}

void cross(godot::Node2D &canvas, const godot::Rect2 &box, godot::Color tint) {
  const Vector2 end = box.position + box.size;
  canvas.draw_line(box.position, end, tint, 3.0F);
  canvas.draw_line(Vector2(end.x, box.position.y),
                   Vector2(box.position.x, end.y), tint, 3.0F);
}

/// The red frame and cross of something the acid or time destroyed.
void wreck_mark(godot::Node2D &canvas, const godot::Rect2 &box) {
  const auto red = color(235, 64, 74, 1.0F);
  canvas.draw_rect(box, red, false, 2.0F);
  cross(canvas, box.grow(-4.0F), red);
}

Image ground_image(GroundKind kind) {
  switch (kind) {
  case GroundKind::Road:
    return Image::Road;
  case GroundKind::CrackedRoad:
    return Image::CrackedRoad;
  case GroundKind::Pothole:
    return Image::Pothole;
  case GroundKind::RoadEndLeft:
    return Image::RoadEndLeft;
  case GroundKind::RoadEndRight:
    return Image::RoadEndRight;
  case GroundKind::TiltedSlab:
    return Image::TiltedSlab;
  case GroundKind::Rubble:
    return Image::Rubble;
  case GroundKind::PlankBridge:
    return Image::PlankBridge;
  case GroundKind::BuriedCar:
    return Image::BuriedCar;
  }
  std::abort();
}

bool visible(const ObjectSnapshot &object) {
  return object.state != ObjectState::Dormant;
}

/// The landmark picture `object` shows, if it has one.
std::optional<LandmarkKind> landmark(const survival::Content &content,
                                     survival::ObjectId object) {
  const auto found =
      std::ranges::find(content.landmarks, object, &survival::Landmark::object);
  if (found == content.landmarks.end()) {
    return std::nullopt;
  }
  return found->kind;
}

/// Whether a ground kind is walked on along its tile's top edge, which the
/// screen lights; the taller tiles and the planks have no such edge.
bool lit_edge(GroundKind kind) {
  switch (kind) {
  case GroundKind::Road:
  case GroundKind::CrackedRoad:
  case GroundKind::Pothole:
  case GroundKind::RoadEndLeft:
  case GroundKind::RoadEndRight:
    return true;
  case GroundKind::TiltedSlab:
  case GroundKind::Rubble:
  case GroundKind::PlankBridge:
  case GroundKind::BuriedCar:
    return false;
  }
  std::abort();
}

/// Draws a ground tile brighter than the scenery behind it, its top edge
/// brightest when `edge`, so the walkable surface reads first.
void paint_tile(godot::Node2D &canvas, const Art &art, Image image, Vector2 at,
                bool edge) {
  art.image(canvas, image, at, 1.0F, false, ground_tint());
  if (edge) {
    art.image_part(canvas, image, rect(0.0F, 0.0F, CELL, GROUND_EDGE),
                   rect(at.x, at.y, CELL, GROUND_EDGE), ground_edge_tint());
  }
}

/// Draws one ground cell of standing terrain, its tile rising over the row
/// above when it is taller than its cell. A landmark's terrain shows its own
/// picture: the pier's iron deck in place of the road, the rotten bridge's
/// frame under its planks, and the ferry's hull, drawn whole, in place of
/// every cell it covers.
void paint_ground_cell(godot::Node2D &canvas, const Art &art,
                       const survival::GroundCell &ground,
                       std::optional<LandmarkKind> picture) {
  const float x = cell_x(ground.cell.x);
  const float top = cell_y(ground.cell.y);
  if (picture) {
    switch (*picture) {
    case LandmarkKind::Ferry:
      return;
    case LandmarkKind::Platform:
      paint_tile(canvas, art, Image::Platform, Vector2(x, top), true);
      return;
    case LandmarkKind::Bridge:
      art.image(canvas, Image::Bridge, Vector2(x, top));
      break;
    case LandmarkKind::Kiosk:
    case LandmarkKind::ColdStore:
      break;
    }
  }
  const Image image = ground_image(ground.kind);
  // Planks span a gap level with the road surface, at the top of the cell.
  paint_tile(canvas, art, image,
             Vector2(x, ground.kind == GroundKind::PlankBridge
                            ? top
                            : top + CELL - art.size(image).y),
             lit_edge(ground.kind));
}

/// Ground under standing terrain, row by row from the top, so a tile taller
/// than its cell rises over the row above it.
void paint_ground(godot::Node2D &canvas, const Art &art,
                  const survival::Content &content,
                  std::span<const ObjectSnapshot> objects) {
  for (const auto &object : objects) {
    if (object.kind == ObjectKind::Terrain &&
        object.state == ObjectState::Intact &&
        landmark(content, object.id) == LandmarkKind::Ferry) {
      stand(canvas, art, Image::Ferry, area_rect(object.area));
    }
  }
  for (const auto &ground : content.ground) {
    const auto standing =
        std::ranges::find_if(objects, [&](const auto &object) {
          return object.kind == ObjectKind::Terrain &&
                 object.state == ObjectState::Intact &&
                 covers(object.area, ground.cell);
        });
    if (standing != objects.end()) {
      paint_ground_cell(canvas, art, ground, landmark(content, standing->id));
    }
  }
  for (const auto &object : objects) {
    if (object.kind == ObjectKind::Terrain &&
        object.state == ObjectState::Destroyed) {
      wreck_mark(canvas, area_rect(object.area));
    }
  }
}

/// The pictures one decoration kind alternates between.
std::span<const Image> decoration_images(DecorationKind kind) {
  static constexpr std::array TRAINS{Image::TrainCarLit,
                                     Image::TrainCarWrecked};
  static constexpr std::array FACADES{Image::FacadeTall, Image::FacadeLow};
  static constexpr std::array CABLES{Image::CablesSagging, Image::PipesValve,
                                     Image::CablesHanging, Image::PipeBent};
  static constexpr std::array NEON{Image::NeonBox, Image::NeonBlade};
  static constexpr std::array FLESH{Image::FleshVines, Image::FleshPolyps,
                                    Image::FleshTendril, Image::FleshTumor};
  static constexpr std::array RUBBLE{Image::RubbleLow, Image::RubbleHigh};
  static constexpr std::array PILLARS{Image::PillarDeck, Image::PillarSnapped};
  static constexpr std::array LAMPS{Image::LampStanding, Image::LampBent};
  switch (kind) {
  case DecorationKind::TiltedTrain:
    return TRAINS;
  case DecorationKind::BuildingShell:
    return FACADES;
  case DecorationKind::HangingCables:
    return CABLES;
  case DecorationKind::DeadNeonSign:
    return NEON;
  case DecorationKind::FleshGrowth:
    return FLESH;
  case DecorationKind::RubblePile:
    return RUBBLE;
  case DecorationKind::BrokenPillar:
    return PILLARS;
  case DecorationKind::StreetLamp:
    return LAMPS;
  }
  std::abort();
}

/// Scenery that hangs from the top of its area rather than standing on a floor.
bool hangs(Image image) {
  return image == Image::CablesSagging || image == Image::CablesHanging ||
         image == Image::PipeBent || image == Image::PipesValve ||
         image == Image::NeonBox || image == Image::NeonBlade ||
         image == Image::FleshVines;
}

/// Whether a decoration kind stands behind the ground, as a backdrop, rather
/// than in front of it.
bool backdrop(DecorationKind kind) {
  switch (kind) {
  case DecorationKind::TiltedTrain:
  case DecorationKind::BuildingShell:
  case DecorationKind::RubblePile:
  case DecorationKind::BrokenPillar:
    return true;
  case DecorationKind::HangingCables:
  case DecorationKind::DeadNeonSign:
  case DecorationKind::FleshGrowth:
  case DecorationKind::StreetLamp:
    return false;
  }
  std::abort();
}

/// Fills a decoration's area with as many of its kind's pictures as fit side
/// by side, at least one, each centred in its share of the width.
void paint_decoration(godot::Node2D &canvas, const Art &art,
                      const survival::Decoration &decoration,
                      std::size_t ordinal, godot::Color tint) {
  const auto images = decoration_images(decoration.kind);
  const auto box = area_rect(decoration.area);
  const float widest = art.size(images.front()).x;
  const auto count =
      std::max<std::size_t>(1, static_cast<std::size_t>(box.size.x / widest));
  const float share = box.size.x / static_cast<float>(count);
  for (std::size_t piece = 0; piece < count; ++piece) {
    const Image image = *std::next(
        images.begin(),
        static_cast<std::ptrdiff_t>((ordinal + piece) % images.size()));
    const Vector2 size = art.size(image);
    const float x = box.position.x + share * static_cast<float>(piece) +
                    (share - size.x) / 2.0F;
    const float y =
        hangs(image) ? box.position.y : box.position.y + box.size.y - size.y;
    art.image(canvas, image, Vector2(x, y), 1.0F, false, tint);
  }
}

/// The backdrop scenery is darkened with the scenery behind the map.
void paint_decorations(godot::Node2D &canvas, const Art &art,
                       const survival::Content &content, bool back) {
  std::array<std::size_t,
             static_cast<std::size_t>(DecorationKind::StreetLamp) + 1>
      seen{};
  for (const auto &decoration : content.decorations) {
    auto &ordinal = seen.at(static_cast<std::size_t>(decoration.kind));
    if (backdrop(decoration.kind) == back) {
      paint_decoration(canvas, art, decoration, ordinal,
                       back ? backdrop_tint() : untinted());
    }
    ++ordinal;
  }
}

/// Whether a building's landmark picture stands in front of the ground: the
/// kiosk wreck blocks the arcade road it stands on, while the cold store's
/// floor runs into the store.
bool stands_in_front(LandmarkKind kind) {
  switch (kind) {
  case LandmarkKind::Kiosk:
    return true;
  case LandmarkKind::ColdStore:
  case LandmarkKind::Ferry:
  case LandmarkKind::Platform:
  case LandmarkKind::Bridge:
    return false;
  }
  std::abort();
}

/// A building is a wall of windows over its area, its landmark picture
/// standing in it, or rubble once destroyed. A building whose every cell is
/// rubble ground is that rubble heap already. Only what stands in front of
/// the ground is drawn when `front`, and only the rest otherwise.
void paint_building(godot::Node2D &canvas, const Art &art,
                    const survival::Content &content,
                    const ObjectSnapshot &object, bool front) {
  const auto box = area_rect(object.area);
  const auto &area = object.area;
  const auto picture = landmark(content, object.id);
  if (front != (object.state == ObjectState::Intact && picture &&
                stands_in_front(*picture))) {
    return;
  }
  if (object.state == ObjectState::Destroyed) {
    for (std::int32_t x = area.x; x < area.x + area.width; x += TILE_CELLS) {
      const auto cells = std::min(TILE_CELLS, area.x + area.width - x);
      stand(canvas, art, Image::RubbleLow,
            rect(cell_x(x), box.position.y, static_cast<float>(cells) * CELL,
                 box.size.y));
    }
    wreck_mark(canvas, box);
    return;
  }
  if (picture) {
    switch (*picture) {
    case LandmarkKind::Kiosk:
      stand(canvas, art, Image::Kiosk, box);
      return;
    case LandmarkKind::ColdStore:
      stand(canvas, art, Image::ColdStore, box);
      return;
    case LandmarkKind::Ferry:
    case LandmarkKind::Platform:
    case LandmarkKind::Bridge:
      break;
    }
  }
  const auto heap =
      std::ranges::count_if(content.ground, [&](const auto &cell) {
        return cell.kind == GroundKind::Rubble &&
               covers(object.area, cell.cell);
      });
  if (heap ==
      static_cast<std::int64_t>(object.area.width) * object.area.height) {
    return;
  }
  for (std::int32_t y = area.y; y < area.y + area.height; y += TILE_CELLS) {
    for (std::int32_t x = area.x; x < area.x + area.width; x += TILE_CELLS) {
      const float w =
          static_cast<float>(std::min(TILE_CELLS, area.x + area.width - x)) *
          CELL;
      const float h =
          static_cast<float>(std::min(TILE_CELLS, area.y + area.height - y)) *
          CELL;
      art.image_part(canvas, Image::BuildingWall, rect(0.0F, 0.0F, w, h),
                     rect(cell_x(x), cell_y(y), w, h));
    }
  }
  canvas.draw_rect(box, color(150, 112, 190, 1.0F), false, 2.0F);
}

void paint_passage(godot::Node2D &canvas, const Art &art,
                   const ObjectSnapshot &object, bool ladder) {
  const auto box = area_rect(object.area);
  const bool broken = object.state == ObjectState::Destroyed;
  if (ladder && !broken) {
    for (std::int32_t row = 0; row < object.area.height; ++row) {
      art.image(canvas, Image::Ladder,
                Vector2(box.position.x, cell_y(object.area.y + row)));
    }
    return;
  }
  Image image = Image::DoorColdStore;
  if (ladder) {
    image = Image::LadderBroken;
  } else if (broken) {
    image = Image::DoorBroken;
  }
  stand(canvas, art, image,
        rect(box.position.x, box.position.y, CELL, box.size.y));
}

void paint_npc(godot::Node2D &canvas, const Art &art,
               const ObjectSnapshot &object) {
  const auto box = area_rect(object.area);
  if (object.state == ObjectState::Destroyed) {
    cross(canvas, box.grow(-5.0F), color(235, 64, 74, 1.0F));
    return;
  }
  stand(canvas, art, Image::NoodleVendor, box);
}

const char *source_kind_name(SourceKind kind) {
  switch (kind) {
  case SourceKind::VendingMachine:
    return "VEND";
  case SourceKind::Npc:
    return "CART";
  case SourceKind::Shop:
    return "SHOP";
  case SourceKind::Bag:
    return "BAG";
  }
  std::abort();
}

/// A bag of tools is the roped duffel; a bag of food the washed-up backpack.
Image source_image(SourceKind kind, const ObjectSnapshot &object) {
  switch (kind) {
  case SourceKind::VendingMachine:
    return Image::VendingMachine;
  case SourceKind::Npc:
    return Image::VendorCart;
  case SourceKind::Shop:
    return Image::Shop;
  case SourceKind::Bag:
    return object.tool ? Image::BagRoof : Image::BagWashedUp;
  }
  std::abort();
}

godot::String supply_name(const ObjectSnapshot &source) {
  if (source.tool) {
    return from(survival::tool_name(*source.tool));
  }
  if (source.ingredient) {
    return from(survival::ingredient_name(*source.ingredient));
  }
  return "";
}

/// One source as the map shows it.
struct Marker {
  ObjectSnapshot object;
  survival::SourceStock stock;
  bool stocked_open = false;
  bool walkable = false;
  /// A gold frame is a promise that a player can walk there.
  bool promised = false;
};

/// A shop front is as tall as three cells, so it stands behind the ground. A
/// shop inside a standing building with a picture of its own has that
/// building for its front.
void paint_shop_front(godot::Node2D &canvas, const Art &art,
                      const survival::Content &content,
                      std::span<const ObjectSnapshot> objects,
                      const Marker &marker) {
  const survival::GridPosition cell{.x = marker.object.area.x,
                                    .y = marker.object.area.y};
  const bool pictured = std::ranges::any_of(objects, [&](const auto &object) {
    return object.kind == ObjectKind::Building &&
           object.state == ObjectState::Intact &&
           landmark(content, object.id) && covers(object.area, cell);
  });
  if (marker.stock.kind == SourceKind::Shop && !pictured) {
    stand(canvas, art, Image::Shop, area_rect(marker.object.area));
  }
}

void paint_source(godot::Node2D &canvas, const Art &art, const Marker &marker) {
  const auto cell = area_rect(marker.object.area);
  const auto box = cell.grow(-2.0F);
  const auto tones = palette();
  if (marker.stock.kind != SourceKind::Shop) {
    stand(canvas, art, source_image(marker.stock.kind, marker.object), cell);
  }
  if (marker.promised) {
    canvas.draw_rect(cell, tones.gold, false, 2.0F);
  } else if (marker.stocked_open) {
    dashed_frame(canvas, cell, tones.white);
  }
  if (!marker.object.material_available) {
    cross(canvas, box, color(235, 64, 74, 1.0F));
  }
}

void paint_hazards(godot::Node2D &canvas, const Art &art,
                   const survival::Content &content) {
  for (const auto &area : content.hazards) {
    const auto box = area_rect(area);
    for (std::int32_t column = 0; column < area.width; ++column) {
      stand(canvas, art, Image::AcidDrip,
            rect(cell_x(area.x + column), box.position.y, CELL, box.size.y));
    }
  }
}

Image figure(Actor actor, bool climbing, bool walking, bool second_step) {
  const bool chef = actor == Actor::Chef;
  if (climbing) {
    return chef ? Image::ChefClimb : Image::AppraiserClimb;
  }
  if (!walking) {
    return chef ? Image::ChefIdle : Image::AppraiserIdle;
  }
  if (second_step) {
    return chef ? Image::ChefWalk2 : Image::AppraiserWalk2;
  }
  return chef ? Image::ChefWalk1 : Image::AppraiserWalk1;
}

/// Where an actor's feet are drawn: its sub-cell position, eased across each
/// 100ms step so it moves smoothly between them.
Vector2 feet_of(const Art &art, Actor actor, const survival::Body &body) {
  const bool chef = actor == Actor::Chef;
  constexpr float PIXELS = CELL / static_cast<float>(survival::SUBCELLS);
  return {art.glide(chef ? "chef_feet_x" : "appraiser_feet_x",
                    MAP_X + static_cast<float>(body.position.x) * PIXELS),
          art.glide(chef ? "chef_feet_y" : "appraiser_feet_y",
                    MAP_Y + static_cast<float>(body.position.y) * PIXELS)};
}

/// Draws an actor taller than its cell, its frame's bottom edge on its feet:
/// climbing on a ladder, walking for a moment after each step and while in
/// the air, and facing the way it last moved sideways.
void paint_actor(godot::Node2D &canvas, const Art &art,
                 const survival::GameView &view, Actor actor,
                 const survival::Body &body, float offset) {
  const auto motion = art.track(actor, survival::cell_of(body.position),
                                view.progression.world_time);
  const bool climbing = body.stance == survival::Stance::Climbing;
  const bool walking =
      body.stance == survival::Stance::Airborne ||
      (motion.moved_at >= 0us &&
       view.progression.world_time - motion.moved_at <= STRIDE);
  const Image image = figure(actor, climbing, walking, motion.second_step);
  const Vector2 size = art.size(image);
  const Vector2 feet = feet_of(art, actor, body);
  art.image(canvas, image,
            Vector2(feet.x - size.x / 2.0F + offset, feet.y - size.y), 1.0F,
            body.heading == survival::Direction::Left && !climbing);
}

godot::String tag_of(survival::ControlMap control, Actor actor) {
  const auto keys = survival::key_set(control, actor);
  if (!keys) {
    return "AI";
  }
  return "P" + godot::String::num_int64(static_cast<std::int64_t>(*keys) + 1);
}

/// Who is out in the city, and where each tag goes.
struct Party {
  bool chef_out = false;
  bool appraiser_out = false;
  /// Both actors stand on one cell.
  bool shared = false;
};

/// Places the map's words in the order they claim a spot: each source's count
/// in its own cell, then what stays put, then the actors' tags, which move.
/// No word covers a source.
Plates label_city(const Art &art, const survival::Content &content,
                  const survival::GameView &view, survival::ControlMap control,
                  const std::vector<Marker> &markers, const Party &party) {
  const auto tones = palette();
  const auto red = color(255, 150, 150, 1.0F);
  Plates plates;
  for (const auto &marker : markers) {
    const auto box = area_rect(marker.object.area).grow(-2.0F);
    plates.add(art, Vector2(box.position.x + 9.0F, box.position.y + 21.0F),
               godot::String::num_int64(marker.stock.remaining), 15,
               marker.promised ? tones.white : tones.muted);
  }
  for (const auto &marker : markers) {
    plates.keep_clear(area_rect(marker.object.area));
  }
  const auto base = content.objects.front().area;
  plates.add(art, Vector2(cell_x(base.x) + 18.0F, cell_y(base.y) + 22.0F),
             "BASE", 18, tones.gold);
  for (const auto &marker : markers) {
    const auto box = area_rect(marker.object.area).grow(-2.0F);
    const float label_y =
        marker.object.area.y == 0 || marker.object.area.y == 13
            ? box.position.y + 44.0F
            : box.position.y - 4.0F;
    plates.add(art, Vector2(box.position.x - 8.0F, label_y),
               godot::String(source_kind_name(marker.stock.kind)) + " " +
                   supply_name(marker.object) +
                   (marker.stocked_open && !marker.walkable
                        ? godot::String(": no way there")
                        : godot::String()),
               13, marker.promised ? tones.white : tones.muted);
  }
  for (const auto &object : view.world.objects) {
    if (!visible(object)) {
      continue;
    }
    const auto box = area_rect(object.area);
    const bool broken = object.state == ObjectState::Destroyed;
    switch (object.kind) {
    case ObjectKind::Building:
      plates.add(art, Vector2(box.position.x + 4.0F, box.position.y + 14.0F),
                 from(survival::object_name(object.id)), 13, tones.muted);
      break;
    case ObjectKind::Npc:
      if (!broken) {
        plates.add(
            art, Vector2(box.position.x - 32.0F, box.position.y + CELL + 13.0F),
            from(survival::object_name(object.id)), 13, tones.white);
      }
      break;
    case ObjectKind::Ladder:
    case ObjectKind::Entrance:
      if (broken) {
        plates.add(art, Vector2(box.position.x + 36.0F, box.position.y + 21.0F),
                   "BROKEN", 13, red);
      }
      break;
    case ObjectKind::Terrain:
    case ObjectKind::MaterialSource:
      break;
    }
  }
  for (const auto &area : content.hazards) {
    const auto box = area_rect(area);
    plates.add(
        art,
        Vector2(box.position.x - 30.0F, box.position.y + box.size.y + 14.0F),
        "ACID DRIP: -1 health", 13, color(180, 255, 140, 1.0F));
  }
  const auto tag = [&](Actor actor, const survival::Body &body, float offset) {
    const Vector2 feet = feet_of(art, actor, body);
    plates.add(
        art,
        Vector2(feet.x - CELL / 2.0F + 6.0F + offset, feet.y - CELL - TAG_RISE),
        tag_of(control, actor), 14, tones.cyan);
  };
  if (party.chef_out) {
    tag(Actor::Chef, view.exploration.chef.body, 0.0F);
  }
  if (party.appraiser_out) {
    tag(Actor::Appraiser, view.exploration.appraiser.body,
        party.shared ? SHARED_CELL_TAG_SHIFT : 0.0F);
  }
  return plates;
}

} // namespace

void paint_mouth(godot::Node2D &canvas, const Art &art, float acid_ratio) {
  art.image(canvas, Image::Throat, Vector2(0.0F, 0.0F), 1.0F, false,
            throat_tint());
  art.image(canvas, Image::City,
            Vector2(0.0F, CITY_BOTTOM - art.size(Image::City).y), 1.0F, false,
            city_tint());
  const float ratio = std::clamp(acid_ratio, 0.0F, 1.0F);
  const float surface = ACID_LOW - ratio * (ACID_LOW - MAP_Y);
  const Vector2 acid = art.size(Image::Acid);
  const float top = std::floor(surface - ACID_SURFACE_DEPTH);
  const auto band = [&](float from, float height, godot::Color tint) {
    art.image_part(canvas, Image::Acid, rect(0.0F, from, acid.x, height),
                   rect(0.0F, top + from, acid.x, height), tint);
  };
  // The surface keeps the picture's glow; below it the sea sinks dark.
  band(0.0F, ACID_SURFACE_DEPTH, untinted());
  for (int step = 0; step < ACID_FADE_STEPS; ++step) {
    band(ACID_SURFACE_DEPTH + static_cast<float>(step) * ACID_FADE_STEP,
         ACID_FADE_STEP,
         untinted().lerp(deep_acid_tint(),
                         static_cast<float>(step + 1) /
                             static_cast<float>(ACID_FADE_STEPS + 1)));
  }
  const float deep =
      ACID_SURFACE_DEPTH + static_cast<float>(ACID_FADE_STEPS) * ACID_FADE_STEP;
  band(deep, acid.y - deep, deep_acid_tint());
  const float bottom = top + acid.y;
  if (bottom < SCREEN_HEIGHT) {
    art.image_part(canvas, Image::Acid, rect(0.0F, acid.y - 1.0F, acid.x, 1.0F),
                   rect(0.0F, bottom, SCREEN_WIDTH, SCREEN_HEIGHT - bottom),
                   deep_acid_tint());
  }
}

void paint_city(godot::Node2D &canvas, const Art &art,
                const survival::Content &content,
                const survival::GameView &view, survival::ControlMap control,
                const std::vector<survival::ObjectId> &out_of_reach) {
  const auto &objects = view.world.objects;
  const auto paint_kind = [&](ObjectKind kind, const auto &paint) {
    for (const auto &object : objects) {
      if (object.kind == kind && visible(object)) {
        paint(object);
      }
    }
  };
  std::vector<Marker> markers;
  for (const auto &stock : view.exploration.sources) {
    const auto object =
        std::ranges::find(objects, stock.id, &ObjectSnapshot::id);
    if (object != objects.end() && visible(*object)) {
      const bool stocked_open =
          object->material_available && stock.remaining > 0;
      const bool walkable =
          std::ranges::find(out_of_reach, stock.id) == out_of_reach.end();
      markers.push_back({.object = *object,
                         .stock = stock,
                         .stocked_open = stocked_open,
                         .walkable = walkable,
                         .promised = stocked_open && walkable});
    }
  }
  const auto &chef = view.exploration.chef.body;
  const auto &appraiser = view.exploration.appraiser.body;
  Party party{.chef_out =
                  view.progression.chef.location == survival::Location::City,
              .appraiser_out = view.progression.appraiser.location ==
                               survival::Location::City};
  party.shared =
      party.chef_out && party.appraiser_out &&
      survival::cell_of(chef.position) == survival::cell_of(appraiser.position);
  paint_decorations(canvas, art, content, true);
  const auto paint_buildings = [&](bool front) {
    paint_kind(ObjectKind::Building, [&](const ObjectSnapshot &object) {
      paint_building(canvas, art, content, object, front);
    });
  };
  paint_buildings(false);
  for (const auto &marker : markers) {
    paint_shop_front(canvas, art, content, objects, marker);
  }
  paint_ground(canvas, art, content, objects);
  paint_buildings(true);
  paint_hazards(canvas, art, content);
  paint_kind(ObjectKind::Entrance, [&](const ObjectSnapshot &object) {
    paint_passage(canvas, art, object, false);
  });
  paint_kind(ObjectKind::Ladder, [&](const ObjectSnapshot &object) {
    paint_passage(canvas, art, object, true);
  });
  paint_decorations(canvas, art, content, false);
  paint_kind(ObjectKind::Npc, [&](const ObjectSnapshot &object) {
    paint_npc(canvas, art, object);
  });
  for (const auto &marker : markers) {
    paint_source(canvas, art, marker);
  }
  if (party.chef_out) {
    paint_actor(canvas, art, view, Actor::Chef, chef, 0.0F);
  }
  if (party.appraiser_out) {
    paint_actor(canvas, art, view, Actor::Appraiser, appraiser,
                party.shared ? 10.0F : 0.0F);
  }
  const auto plates = label_city(art, content, view, control, markers, party);
  plates.paint(canvas, art);
  canvas.set_meta("map_plates", plates.boxes());
}

void paint_workshop(godot::Node2D &canvas, const Art &art, bool dish_ready) {
  art.image_part(canvas, Image::Kitchen,
                 rect(0.0F, KITCHEN_SOURCE_Y, SCREEN_WIDTH, KITCHEN_HEIGHT),
                 rect(0.0F, KITCHEN_Y, SCREEN_WIDTH, KITCHEN_HEIGHT));
  const float figure_top =
      KITCHEN_FLOOR - art.size(Image::ChefIdle).y * WORKSHOP_SCALE;
  art.image(canvas, Image::AppraiserIdle, Vector2(150.0F, figure_top),
            WORKSHOP_SCALE);
  art.image(canvas, Image::ChefIdle, Vector2(560.0F, figure_top),
            WORKSHOP_SCALE);
  if (dish_ready) {
    canvas.draw_circle(FEED_BUCKET, 18.0F, color(255, 142, 72, 1.0F));
    canvas.draw_circle(FEED_BUCKET + Vector2(-20.0F, -26.0F), 7.0F,
                       color(240, 240, 240, 0.5F));
    canvas.draw_circle(FEED_BUCKET + Vector2(18.0F, -36.0F), 9.0F,
                       color(240, 240, 240, 0.4F));
  }
  const godot::String feed = "MONSTER FEED";
  paint_plate(canvas, art,
              {.box = plate_box(art, Vector2(1110.0F, 300.0F), feed, 18),
               .text = feed,
               .size = 18,
               .tint = palette().gold});
}

} // namespace neocity::rendering
