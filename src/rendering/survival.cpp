#include "survival.hpp"

#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string_view>

namespace neocity::rendering {
namespace {

using godot::Vector2;
using survival::Actor;
using survival::ObjectKind;
using survival::ObjectSnapshot;
using survival::ObjectState;
using survival::SourceKind;

constexpr float CELL = 32.0F;
constexpr float MAP_X = 64.0F;
constexpr float MAP_Y = 70.0F;
constexpr float MAP_BOTTOM = 582.0F;
constexpr float TOOTH_WIDTH = 88.0F;
constexpr int TEETH = 15;
constexpr std::size_t CHEF_SPRITE = 0;
constexpr std::size_t APPRAISER_SPRITE = 2;
constexpr float DASH = 4.0F;
constexpr float TAG_RISE = 24.0F;
/// Clears the chef's tag when the two actors share a cell.
constexpr float SHARED_CELL_TAG_SHIFT = 26.0F;

float cell_x(std::int32_t x) { return MAP_X + static_cast<float>(x) * CELL; }
float cell_y(std::int32_t y) { return MAP_Y + static_cast<float>(y) * CELL; }

godot::Rect2 area_rect(const survival::WorldArea &area) {
  return rect(cell_x(area.x), cell_y(area.y),
              static_cast<float>(area.width) * CELL,
              static_cast<float>(area.height) * CELL);
}

godot::String from(std::string_view text) {
  return godot::String::utf8(text.data(),
                             static_cast<std::int64_t>(text.size()));
}

void triangle(godot::Node2D &canvas, Vector2 a, Vector2 b, Vector2 c,
              godot::Color tint) {
  godot::PackedVector2Array points;
  points.push_back(a);
  points.push_back(b);
  points.push_back(c);
  canvas.draw_colored_polygon(points, tint);
}

/// Text on an opaque plate, whose contrast no acid level can change.
void plated_text(godot::Node2D &canvas, const AscentDrawing &drawing, float x,
                 float y, const godot::String &text, int size,
                 godot::Color tint) {
  const auto height = static_cast<float>(size);
  canvas.draw_rect(rect(x - 2.0F, y - height,
                        drawing.text_width(text, size) + 4.0F, height * 1.3F),
                   color(14, 12, 26, 1.0F));
  drawing.text(canvas, x, y, text, size, tint);
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

void paint_towers(godot::Node2D &canvas) {
  constexpr std::array<std::array<float, 3>, 9> towers{
      {{80.0F, 54.0F, 180.0F},
       {180.0F, 70.0F, 240.0F},
       {320.0F, 44.0F, 150.0F},
       {470.0F, 80.0F, 210.0F},
       {640.0F, 56.0F, 260.0F},
       {790.0F, 66.0F, 170.0F},
       {930.0F, 48.0F, 230.0F},
       {1050.0F, 72.0F, 190.0F},
       {1170.0F, 60.0F, 150.0F}}};
  for (const auto &[x, width, height] : towers) {
    const float top = 620.0F - height;
    canvas.draw_rect(rect(x, top, width, height), color(44, 20, 74, 1.0F));
    const int floors = static_cast<int>((586.0F - top) / 26.0F);
    for (int floor = 0; floor < floors; ++floor) {
      const float y = top + 14.0F + static_cast<float>(floor) * 26.0F;
      canvas.draw_rect(rect(x + 10.0F, y, 8.0F, 10.0F),
                       color(255, 176, 88, 0.45F));
      canvas.draw_rect(rect(x + width - 20.0F, y, 8.0F, 10.0F),
                       color(255, 176, 88, 0.3F));
    }
  }
}

void paint_tile(godot::Node2D &canvas, const godot::Rect2 &box,
                godot::Color fill, godot::Color seam) {
  canvas.draw_rect(box, fill);
  const int columns = static_cast<int>(box.size.x / CELL);
  for (int column = 0; column < columns; ++column) {
    const float x = box.position.x + static_cast<float>(column) * CELL;
    canvas.draw_rect(rect(x, box.position.y, 1.0F, box.size.y), seam);
  }
  const int rows = static_cast<int>(box.size.y / CELL);
  for (int row = 0; row < rows; ++row) {
    const float y = box.position.y + static_cast<float>(row) * CELL;
    canvas.draw_rect(rect(box.position.x, y, box.size.x, 1.0F), seam);
  }
}

void paint_rubble(godot::Node2D &canvas, const godot::Rect2 &box) {
  constexpr std::array<std::array<float, 3>, 5> stones{{{0.15F, 0.3F, 6.0F},
                                                        {0.4F, 0.7F, 5.0F},
                                                        {0.6F, 0.25F, 7.0F},
                                                        {0.8F, 0.6F, 5.0F},
                                                        {0.3F, 0.5F, 4.0F}}};
  for (const auto &[u, v, radius] : stones) {
    canvas.draw_circle(box.position + Vector2(box.size.x * u, box.size.y * v),
                       radius, color(70, 58, 66, 1.0F));
  }
  canvas.draw_rect(box, color(235, 64, 74, 1.0F), false, 2.0F);
}

void paint_terrain(godot::Node2D &canvas, const ObjectSnapshot &object) {
  const auto box = area_rect(object.area);
  if (object.state == ObjectState::Destroyed) {
    paint_rubble(canvas, box);
    return;
  }
  const bool bridge = object.area.height == 1;
  paint_tile(canvas, box,
             bridge ? color(92, 66, 48, 1.0F) : color(58, 56, 72, 1.0F),
             color(30, 28, 42, 1.0F));
  canvas.draw_rect(rect(box.position.x, box.position.y, box.size.x, 2.0F),
                   color(255, 160, 96, 0.6F));
}

void paint_building(godot::Node2D &canvas, const AscentDrawing &drawing,
                    const ObjectSnapshot &object) {
  const auto box = area_rect(object.area);
  const auto tones = palette();
  if (object.state == ObjectState::Destroyed) {
    paint_rubble(canvas, box);
    return;
  }
  canvas.draw_rect(box, color(40, 26, 62, 1.0F));
  canvas.draw_rect(box, color(150, 112, 190, 1.0F), false, 2.0F);
  plated_text(canvas, drawing, box.position.x + 4.0F, box.position.y + 14.0F,
              from(survival::object_name(object.id)), 13, tones.muted);
}

void paint_passage(godot::Node2D &canvas, const AscentDrawing &drawing,
                   const ObjectSnapshot &object, bool ladder) {
  const auto box = area_rect(object.area);
  if (object.state == ObjectState::Destroyed) {
    canvas.draw_rect(box, color(120, 24, 34, 1.0F));
    cross(canvas, box, color(255, 110, 110, 1.0F));
    plated_text(canvas, drawing, box.position.x + 36.0F, box.position.y + 21.0F,
                "BROKEN", 13, color(255, 150, 150, 1.0F));
    return;
  }
  canvas.draw_rect(box,
                   ladder ? color(120, 84, 44, 1.0F) : color(82, 72, 58, 1.0F));
  if (ladder) {
    for (const float y : {6.0F, 14.0F, 22.0F, 30.0F}) {
      canvas.draw_rect(rect(box.position.x + 4.0F, box.position.y + y,
                            box.size.x - 8.0F, 3.0F),
                       color(226, 178, 104, 1.0F));
    }
  }
}

void paint_npc(godot::Node2D &canvas, const AscentDrawing &drawing,
               const ObjectSnapshot &object) {
  const auto box = area_rect(object.area);
  const Vector2 head = box.position + Vector2(CELL / 2.0F, 9.0F);
  if (object.state == ObjectState::Destroyed) {
    canvas.draw_circle(head + Vector2(0.0F, 12.0F), 9.0F,
                       color(70, 58, 66, 1.0F));
    cross(canvas, box.grow(-5.0F), color(235, 64, 74, 1.0F));
    return;
  }
  canvas.draw_rect(rect(head.x - 7.0F, head.y + 6.0F, 14.0F, 15.0F),
                   color(212, 104, 148, 1.0F));
  canvas.draw_circle(head, 7.0F, color(244, 208, 170, 1.0F));
  plated_text(canvas, drawing, box.position.x - 32.0F,
              box.position.y + CELL + 13.0F,
              from(survival::object_name(object.id)), 13, palette().white);
}

godot::Color source_tint(SourceKind kind) {
  switch (kind) {
  case SourceKind::VendingMachine:
    return color(214, 58, 64, 1.0F);
  case SourceKind::Npc:
    return color(212, 104, 148, 1.0F);
  case SourceKind::Shop:
    return color(226, 150, 56, 1.0F);
  case SourceKind::Bag:
    return color(170, 118, 62, 1.0F);
  }
  std::abort();
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

void paint_source(godot::Node2D &canvas, const Marker &marker) {
  const auto box = area_rect(marker.object.area).grow(-2.0F);
  const auto tones = palette();
  canvas.draw_rect(box, marker.promised ? source_tint(marker.stock.kind)
                                        : color(60, 58, 66, 1.0F));
  if (marker.promised) {
    canvas.draw_rect(box.grow(2.0F), tones.gold, false, 2.0F);
  } else if (marker.stocked_open) {
    dashed_frame(canvas, box.grow(2.0F), tones.white);
  }
  if (!marker.object.material_available) {
    cross(canvas, box, color(235, 64, 74, 1.0F));
  }
}

void paint_source_name(godot::Node2D &canvas, const AscentDrawing &drawing,
                       const Marker &marker) {
  const auto box = area_rect(marker.object.area).grow(-2.0F);
  const auto tones = palette();
  const float label_y = marker.object.area.y == 0 || marker.object.area.y == 13
                            ? box.position.y + 44.0F
                            : box.position.y - 4.0F;
  const godot::String kind = source_kind_name(marker.stock.kind);
  plated_text(canvas, drawing, box.position.x - 8.0F, label_y,
              kind + godot::String(" ") + supply_name(marker.object) +
                  (marker.stocked_open && !marker.walkable
                       ? godot::String(": no way there")
                       : godot::String()),
              13, marker.promised ? tones.white : tones.muted);
}

/// The count comes last of all, so an actor never hides what is left.
void paint_source_count(godot::Node2D &canvas, const AscentDrawing &drawing,
                        const Marker &marker) {
  const auto box = area_rect(marker.object.area).grow(-2.0F);
  const auto tones = palette();
  plated_text(canvas, drawing, box.position.x + 9.0F, box.position.y + 21.0F,
              godot::String::num_int64(marker.stock.remaining), 15,
              marker.promised ? tones.white : tones.muted);
}

void paint_hazards(godot::Node2D &canvas, const AscentDrawing &drawing,
                   const survival::Content &content) {
  for (const auto &area : content.hazards) {
    const auto box = area_rect(area);
    canvas.draw_rect(box, color(120, 255, 90, 0.35F));
    for (std::int32_t row = 0; row < area.height; ++row) {
      const float y = box.position.y + 8.0F + static_cast<float>(row) * CELL;
      canvas.draw_circle(Vector2(box.position.x + CELL / 2.0F, y + 8.0F), 6.0F,
                         color(160, 255, 120, 1.0F));
      triangle(canvas, Vector2(box.position.x + CELL / 2.0F - 6.0F, y + 6.0F),
               Vector2(box.position.x + CELL / 2.0F + 6.0F, y + 6.0F),
               Vector2(box.position.x + CELL / 2.0F, y - 8.0F),
               color(160, 255, 120, 1.0F));
    }
    plated_text(canvas, drawing, box.position.x - 30.0F,
                box.position.y + box.size.y + 14.0F, "ACID DRIP: -1 health", 13,
                color(180, 255, 140, 1.0F));
  }
}

void paint_actor(godot::Node2D &canvas, const AscentDrawing &drawing,
                 survival::GridPosition position, std::size_t sprite,
                 float offset) {
  const float x = cell_x(position.x) + offset;
  const float y = cell_y(position.y);
  canvas.draw_circle(Vector2(x + CELL / 2.0F, y + CELL - 2.0F), 13.0F,
                     color(4, 8, 25, 0.5F));
  drawing.sprite(canvas, sprite, rect(x - 6.0F, y - 20.0F, 44.0F, 52.0F),
                 false);
}

void paint_tag(godot::Node2D &canvas, const AscentDrawing &drawing,
               survival::GridPosition position, const godot::String &tag,
               float offset) {
  plated_text(canvas, drawing, cell_x(position.x) + 6.0F + offset,
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

void paint_mouth(godot::Node2D &canvas, float acid_ratio) {
  canvas.draw_rect(rect(0.0F, 0.0F, 1280.0F, 720.0F), color(96, 20, 78, 1.0F));
  const Vector2 throat(880.0F, 120.0F);
  canvas.draw_circle(throat, 340.0F, color(136, 28, 98, 1.0F));
  canvas.draw_circle(throat, 230.0F, color(182, 42, 120, 1.0F));
  canvas.draw_circle(throat, 120.0F, color(232, 78, 150, 1.0F));
  paint_towers(canvas);
  canvas.draw_rect(rect(0.0F, MAP_Y, 1280.0F, 720.0F - MAP_Y),
                   color(10, 6, 24, 0.55F));
  const float ratio = std::clamp(acid_ratio, 0.0F, 1.0F);
  const float span = MAP_BOTTOM - MAP_Y;
  const float surface = MAP_BOTTOM - (0.12F + 0.88F * ratio) * span;
  canvas.draw_rect(rect(0.0F, surface, 1280.0F, 720.0F - surface),
                   color(62, 214, 76, 0.42F));
  canvas.draw_rect(rect(0.0F, surface, 1280.0F, 3.0F),
                   color(170, 255, 130, 0.9F));
  for (int tooth = 0; tooth < TEETH; ++tooth) {
    const float x = static_cast<float>(tooth) * TOOTH_WIDTH;
    triangle(canvas, Vector2(x, 62.0F), Vector2(x + TOOTH_WIDTH, 62.0F),
             Vector2(x + TOOTH_WIDTH / 2.0F, 112.0F),
             color(238, 214, 180, 1.0F));
  }
}

void paint_city(godot::Node2D &canvas, const AscentDrawing &drawing,
                const survival::Content &content,
                const survival::GameView &view, survival::ControlMap control,
                const std::vector<survival::ObjectId> &out_of_reach) {
  const auto &objects = view.world.objects;
  const auto paint_kind = [&](ObjectKind kind, const auto &paint) {
    for (const auto &object : objects) {
      if (object.kind == kind && object.state != ObjectState::Dormant) {
        paint(object);
      }
    }
  };
  paint_kind(ObjectKind::Terrain, [&](const ObjectSnapshot &object) {
    paint_terrain(canvas, object);
  });
  paint_hazards(canvas, drawing, content);
  paint_kind(ObjectKind::Building, [&](const ObjectSnapshot &object) {
    paint_building(canvas, drawing, object);
  });
  paint_kind(ObjectKind::Entrance, [&](const ObjectSnapshot &object) {
    paint_passage(canvas, drawing, object, false);
  });
  paint_kind(ObjectKind::Ladder, [&](const ObjectSnapshot &object) {
    paint_passage(canvas, drawing, object, true);
  });
  paint_kind(ObjectKind::Npc, [&](const ObjectSnapshot &object) {
    paint_npc(canvas, drawing, object);
  });
  std::vector<Marker> markers;
  for (const auto &stock : view.exploration.sources) {
    const auto object =
        std::ranges::find(objects, stock.id, &ObjectSnapshot::id);
    if (object != objects.end() && object->state != ObjectState::Dormant) {
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
  for (const auto &marker : markers) {
    paint_source(canvas, marker);
  }
  const auto tones = palette();
  const auto base = content.objects.front().area;
  plated_text(canvas, drawing, cell_x(base.x) + 18.0F, cell_y(base.y) + 22.0F,
              "BASE", 18, tones.gold);
  const auto &chef = view.exploration.chef.position;
  const auto &appraiser = view.exploration.appraiser.position;
  const bool chef_out =
      view.progression.chef.location == survival::Location::City;
  const bool appraiser_out =
      view.progression.appraiser.location == survival::Location::City;
  const bool shared = chef_out && appraiser_out && chef == appraiser;
  if (chef_out) {
    paint_actor(canvas, drawing, chef, CHEF_SPRITE, 0.0F);
  }
  if (appraiser_out) {
    paint_actor(canvas, drawing, appraiser, APPRAISER_SPRITE,
                shared ? 10.0F : 0.0F);
  }
  for (const auto &marker : markers) {
    paint_source_name(canvas, drawing, marker);
  }
  if (chef_out) {
    paint_tag(canvas, drawing, chef, tag_of(control, Actor::Chef), 0.0F);
  }
  if (appraiser_out) {
    paint_tag(canvas, drawing, appraiser, tag_of(control, Actor::Appraiser),
              shared ? SHARED_CELL_TAG_SHIFT : 0.0F);
  }
  for (const auto &marker : markers) {
    paint_source_count(canvas, drawing, marker);
  }
}

void paint_workshop(godot::Node2D &canvas, const AscentDrawing &drawing,
                    bool dish_ready) {
  canvas.draw_rect(rect(30.0F, 110.0F, 760.0F, 200.0F),
                   color(58, 34, 28, 1.0F));
  canvas.draw_rect(rect(30.0F, 104.0F, 760.0F, 8.0F), color(36, 22, 22, 1.0F));
  canvas.draw_rect(rect(30.0F, 296.0F, 760.0F, 14.0F), color(36, 22, 22, 1.0F));
  for (const float x : {180.0F, 400.0F, 620.0F}) {
    canvas.draw_line(Vector2(x, 112.0F), Vector2(x, 138.0F),
                     color(36, 22, 22, 1.0F), 2.0F);
    canvas.draw_circle(Vector2(x, 146.0F), 26.0F, color(255, 176, 88, 0.2F));
    canvas.draw_circle(Vector2(x, 146.0F), 9.0F, color(255, 206, 120, 1.0F));
  }
  canvas.draw_rect(rect(70.0F, 250.0F, 210.0F, 46.0F), color(96, 62, 40, 1.0F));
  canvas.draw_rect(rect(430.0F, 262.0F, 150.0F, 34.0F),
                   color(30, 28, 36, 1.0F));
  canvas.draw_circle(Vector2(505.0F, 262.0F), 22.0F, color(255, 112, 48, 0.8F));
  drawing.sprite(canvas, APPRAISER_SPRITE, rect(130.0F, 172.0F, 104.0F, 124.0F),
                 false);
  drawing.sprite(canvas, CHEF_SPRITE, rect(350.0F, 172.0F, 108.0F, 124.0F),
                 false);
  const auto ivory = color(232, 224, 214, 1.0F);
  canvas.draw_circle(Vector2(960.0F, 212.0F), 50.0F, ivory, true);
  canvas.draw_rect(rect(900.0F, 160.0F, 120.0F, 52.0F),
                   color(96, 20, 78, 1.0F));
  canvas.draw_rect(rect(906.0F, 206.0F, 108.0F, 8.0F), ivory);
  canvas.draw_line(Vector2(960.0F, 70.0F), Vector2(960.0F, 206.0F),
                   color(48, 34, 30, 1.0F), 4.0F);
  if (dish_ready) {
    canvas.draw_circle(Vector2(960.0F, 204.0F), 26.0F,
                       color(255, 142, 72, 1.0F));
    canvas.draw_circle(Vector2(936.0F, 176.0F), 7.0F,
                       color(240, 240, 240, 0.5F));
    canvas.draw_circle(Vector2(980.0F, 166.0F), 9.0F,
                       color(240, 240, 240, 0.4F));
  }
  drawing.text(canvas, 908.0F, 288.0F, "MONSTER FEED", 18, palette().gold);
}

} // namespace neocity::rendering
