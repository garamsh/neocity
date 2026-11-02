#include "survival.hpp"

#include <godot_cpp/variant/vector2.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <span>
#include <string_view>

namespace neocity::rendering {
namespace {

using namespace std::chrono_literals;
using godot::Vector2;
using survival::Actor;
using survival::DecorationKind;
using survival::GroundKind;
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

/// Text on an opaque plate, whose contrast no acid level can change.
void plated_text(godot::Node2D &canvas, const Art &art, float x, float y,
                 const godot::String &text, int size, godot::Color tint) {
  const auto height = static_cast<float>(size);
  canvas.draw_rect(rect(x - 2.0F, y - height, art.text_width(text, size) + 4.0F,
                        height * 1.3F),
                   color(14, 12, 26, 1.0F));
  art.text(canvas, x, y, text, size, tint);
}

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

/// Ground under standing terrain, row by row from the top, so a tile taller
/// than its cell rises over the row above it.
void paint_ground(godot::Node2D &canvas, const Art &art,
                  const survival::Content &content,
                  std::span<const ObjectSnapshot> objects) {
  for (const auto &ground : content.ground) {
    const bool standing = std::ranges::any_of(objects, [&](const auto &object) {
      return object.kind == ObjectKind::Terrain &&
             object.state == ObjectState::Intact &&
             covers(object.area, ground.cell);
    });
    if (!standing) {
      continue;
    }
    const Image image = ground_image(ground.kind);
    const float x = cell_x(ground.cell.x);
    const float top = cell_y(ground.cell.y);
    // Planks span a gap level with the road surface, at the top of the cell.
    art.image(canvas, image,
              Vector2(x, ground.kind == GroundKind::PlankBridge
                             ? top
                             : top + CELL - art.size(image).y));
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
                      std::size_t ordinal) {
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
    art.image(canvas, image, Vector2(x, y));
  }
}

void paint_decorations(godot::Node2D &canvas, const Art &art,
                       const survival::Content &content, bool back) {
  std::array<std::size_t,
             static_cast<std::size_t>(DecorationKind::StreetLamp) + 1>
      seen{};
  for (const auto &decoration : content.decorations) {
    auto &ordinal = seen.at(static_cast<std::size_t>(decoration.kind));
    if (backdrop(decoration.kind) == back) {
      paint_decoration(canvas, art, decoration, ordinal);
    }
    ++ordinal;
  }
}

/// A building is a wall of windows over its area, or rubble once destroyed. A
/// building whose every cell is rubble ground is that rubble heap already.
void paint_building(godot::Node2D &canvas, const Art &art,
                    const survival::Content &content,
                    const ObjectSnapshot &object) {
  const auto box = area_rect(object.area);
  const auto &area = object.area;
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

void paint_building_name(godot::Node2D &canvas, const Art &art,
                         const ObjectSnapshot &object) {
  const auto box = area_rect(object.area);
  plated_text(canvas, art, box.position.x + 4.0F, box.position.y + 14.0F,
              from(survival::object_name(object.id)), 13, palette().muted);
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
  } else {
    Image image = Image::DoorColdStore;
    if (ladder) {
      image = Image::LadderBroken;
    } else if (broken) {
      image = Image::DoorBroken;
    }
    stand(canvas, art, image,
          rect(box.position.x, box.position.y, CELL, box.size.y));
  }
  if (broken) {
    plated_text(canvas, art, box.position.x + 36.0F, box.position.y + 21.0F,
                "BROKEN", 13, color(255, 150, 150, 1.0F));
  }
}

void paint_npc(godot::Node2D &canvas, const Art &art,
               const ObjectSnapshot &object) {
  const auto box = area_rect(object.area);
  if (object.state == ObjectState::Destroyed) {
    cross(canvas, box.grow(-5.0F), color(235, 64, 74, 1.0F));
    return;
  }
  stand(canvas, art, Image::NoodleVendor, box);
  plated_text(canvas, art, box.position.x - 32.0F,
              box.position.y + CELL + 13.0F,
              from(survival::object_name(object.id)), 13, palette().white);
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

/// A shop front is as tall as three cells, so it stands behind the ground.
void paint_shop_front(godot::Node2D &canvas, const Art &art,
                      const Marker &marker) {
  if (marker.stock.kind == SourceKind::Shop) {
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

void paint_source_name(godot::Node2D &canvas, const Art &art,
                       const Marker &marker) {
  const auto box = area_rect(marker.object.area).grow(-2.0F);
  const auto tones = palette();
  const float label_y = marker.object.area.y == 0 || marker.object.area.y == 13
                            ? box.position.y + 44.0F
                            : box.position.y - 4.0F;
  const godot::String kind = source_kind_name(marker.stock.kind);
  plated_text(canvas, art, box.position.x - 8.0F, label_y,
              kind + godot::String(" ") + supply_name(marker.object) +
                  (marker.stocked_open && !marker.walkable
                       ? godot::String(": no way there")
                       : godot::String()),
              13, marker.promised ? tones.white : tones.muted);
}

/// The count comes last of all, so an actor never hides what is left.
void paint_source_count(godot::Node2D &canvas, const Art &art,
                        const Marker &marker) {
  const auto box = area_rect(marker.object.area).grow(-2.0F);
  const auto tones = palette();
  plated_text(canvas, art, box.position.x + 9.0F, box.position.y + 21.0F,
              godot::String::num_int64(marker.stock.remaining), 15,
              marker.promised ? tones.white : tones.muted);
}

void paint_hazards(godot::Node2D &canvas, const Art &art,
                   const survival::Content &content) {
  for (const auto &area : content.hazards) {
    const auto box = area_rect(area);
    for (std::int32_t column = 0; column < area.width; ++column) {
      stand(canvas, art, Image::AcidDrip,
            rect(cell_x(area.x + column), box.position.y, CELL, box.size.y));
    }
    plated_text(canvas, art, box.position.x - 30.0F,
                box.position.y + box.size.y + 14.0F, "ACID DRIP: -1 health", 13,
                color(180, 255, 140, 1.0F));
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

/// Draws an actor taller than its cell, its frame's bottom edge on the cell's
/// floor: climbing on a ladder, walking for a moment after each step, and
/// facing the way it last walked.
void paint_actor(godot::Node2D &canvas, const Art &art,
                 const survival::GameView &view, Actor actor,
                 survival::GridPosition position, float offset) {
  const auto motion = art.track(actor, position, view.progression.world_time);
  const bool climbing = std::ranges::any_of(
      view.world.objects, [&](const ObjectSnapshot &object) {
        return object.kind == ObjectKind::Ladder &&
               object.state == ObjectState::Intact &&
               covers(object.area, position);
      });
  const bool walking = motion.moved_at >= 0us &&
                       view.progression.world_time - motion.moved_at <= STRIDE;
  const Image image = figure(actor, climbing, walking, motion.second_step);
  const Vector2 size = art.size(image);
  art.image(canvas, image,
            Vector2(cell_x(position.x) + (CELL - size.x) / 2.0F + offset,
                    cell_y(position.y) + CELL - size.y),
            1.0F, motion.facing_left && !climbing);
}

void paint_tag(godot::Node2D &canvas, const Art &art,
               survival::GridPosition position, const godot::String &tag,
               float offset) {
  plated_text(canvas, art, cell_x(position.x) + 6.0F + offset,
              cell_y(position.y) - TAG_RISE, tag, 14, palette().cyan);
}

godot::String tag_of(survival::ControlMap control, Actor actor) {
  const auto keys = survival::key_set(control, actor);
  if (!keys) {
    return "AI";
  }
  return "P" + godot::String::num_int64(static_cast<std::int64_t>(*keys) + 1);
}

} // namespace

void paint_mouth(godot::Node2D &canvas, const Art &art, float acid_ratio) {
  art.image(canvas, Image::Throat, Vector2(0.0F, 0.0F));
  art.image(canvas, Image::City,
            Vector2(0.0F, CITY_BOTTOM - art.size(Image::City).y));
  // Dims the throat and the city behind the map, so the map reads over them.
  canvas.draw_rect(rect(0.0F, MAP_Y, SCREEN_WIDTH, SCREEN_HEIGHT - MAP_Y),
                   color(10, 6, 24, 0.45F));
  const float ratio = std::clamp(acid_ratio, 0.0F, 1.0F);
  const float surface = ACID_LOW - ratio * (ACID_LOW - MAP_Y);
  const Vector2 acid = art.size(Image::Acid);
  const float top = std::floor(surface - ACID_SURFACE_DEPTH);
  art.image(canvas, Image::Acid, Vector2(0.0F, top));
  const float bottom = top + acid.y;
  if (bottom < SCREEN_HEIGHT) {
    art.image_part(canvas, Image::Acid, rect(0.0F, acid.y - 1.0F, acid.x, 1.0F),
                   rect(0.0F, bottom, SCREEN_WIDTH, SCREEN_HEIGHT - bottom));
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
  paint_decorations(canvas, art, content, true);
  paint_kind(ObjectKind::Building, [&](const ObjectSnapshot &object) {
    paint_building(canvas, art, content, object);
  });
  for (const auto &marker : markers) {
    paint_shop_front(canvas, art, marker);
  }
  paint_ground(canvas, art, content, objects);
  paint_hazards(canvas, art, content);
  paint_kind(ObjectKind::Entrance, [&](const ObjectSnapshot &object) {
    paint_passage(canvas, art, object, false);
  });
  paint_kind(ObjectKind::Ladder, [&](const ObjectSnapshot &object) {
    paint_passage(canvas, art, object, true);
  });
  paint_decorations(canvas, art, content, false);
  paint_kind(ObjectKind::Building, [&](const ObjectSnapshot &object) {
    paint_building_name(canvas, art, object);
  });
  paint_kind(ObjectKind::Npc, [&](const ObjectSnapshot &object) {
    paint_npc(canvas, art, object);
  });
  for (const auto &marker : markers) {
    paint_source(canvas, art, marker);
  }
  const auto tones = palette();
  const auto base = content.objects.front().area;
  plated_text(canvas, art, cell_x(base.x) + 18.0F, cell_y(base.y) + 22.0F,
              "BASE", 18, tones.gold);
  const auto &chef = view.exploration.chef.position;
  const auto &appraiser = view.exploration.appraiser.position;
  const bool chef_out =
      view.progression.chef.location == survival::Location::City;
  const bool appraiser_out =
      view.progression.appraiser.location == survival::Location::City;
  const bool shared = chef_out && appraiser_out && chef == appraiser;
  if (chef_out) {
    paint_actor(canvas, art, view, Actor::Chef, chef, 0.0F);
  }
  if (appraiser_out) {
    paint_actor(canvas, art, view, Actor::Appraiser, appraiser,
                shared ? 10.0F : 0.0F);
  }
  for (const auto &marker : markers) {
    paint_source_name(canvas, art, marker);
  }
  if (chef_out) {
    paint_tag(canvas, art, chef, tag_of(control, Actor::Chef), 0.0F);
  }
  if (appraiser_out) {
    paint_tag(canvas, art, appraiser, tag_of(control, Actor::Appraiser),
              shared ? SHARED_CELL_TAG_SHIFT : 0.0F);
  }
  for (const auto &marker : markers) {
    paint_source_count(canvas, art, marker);
  }
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
  plated_text(canvas, art, 1110.0F, 300.0F, "MONSTER FEED", 18, palette().gold);
}

} // namespace neocity::rendering
