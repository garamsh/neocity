#include "survival.hpp"

#include "camera.hpp"

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

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
/// The rise of each step of stairs, cut from a walkway deck, and the first row
/// of the deck's face, which stairs show under their steps.
constexpr float STAIR_STEP = 8.0F;
constexpr float STAIR_FACE = 10.0F;
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
/// How long an actor shows its landing frame after it comes down.
constexpr auto LANDING = 120ms;
/// The walk and the run show their next frame every this many sub-cells, so
/// the feet keep pace with the ground; the climb every this many upward.
constexpr std::int32_t WALK_STRIDE = 16;
constexpr std::int32_t RUN_STRIDE = 24;
constexpr std::int32_t CLIMB_STRIDE = 16;
/// Faster than this, in sub-cells per physics step, is running.
constexpr float RUN_SPEED =
    static_cast<float>(survival::PHYSICS.walk + survival::PHYSICS.run) / 2.0F;
/// A walkway has a support under every this many cells between its ends.
constexpr std::int32_t SUPPORT_SPACING = 4;
/// How far the ferry's hull sinks below its floor line, so its deck is there.
constexpr float FERRY_SINK = 64.0F;
/// Where the map's words may stand: below the top bar, above the actor cards.
constexpr float LABEL_TOP = 62.0F;
constexpr float LABEL_BOTTOM = 540.0F;
/// Clear space kept between two plates.
constexpr float PLATE_GAP = 2.0F;
/// How far a plate looks sideways per step when its spot is taken.
constexpr float PLATE_STEP = 16.0F;
constexpr int PLATE_RINGS = 12;
/// How far inside the play area's edge a pointer to a goal off the screen
/// stands, and its arrow's length from its middle.
constexpr float POINTER_INSET = 28.0F;
constexpr float POINTER_SIZE = 12.0F;
/// How far each backdrop layer moves with the camera, from the back.
constexpr float THROAT_DEPTH = 0.15F;
constexpr float CITY_DEPTH = 0.35F;
constexpr float BAND_DEPTH = 0.6F;
/// The acid band's height above the bottom of the play area at acid 0 and at
/// full acid.
constexpr float BAND_LOW = 140.0F;
constexpr float BAND_HIGH = 300.0F;
/// Where the band's broken pillars stand along it, and how far they sink.
constexpr std::array BAND_PILLARS{150.0F, 470.0F, 800.0F, 1120.0F};
constexpr float BAND_SINK = 28.0F;
/// A halo's rings, each adding the same light.
constexpr int HALO_RINGS = 6;
/// A lamp's light: how far below the picture's top its bulb is, its reach,
/// and what each ring adds.
constexpr float LAMP_HEAD = 12.0F;
constexpr float LAMP_GLOW = 46.0F;
constexpr float LAMP_LIGHT = 0.07F;
constexpr float DRIP_GLOW = 30.0F;
constexpr float DRIP_LIGHT = 0.06F;
/// Light over an acid surface: its rows of steps, and what the lowest adds.
constexpr int GLOW_STEPS = 6;
constexpr float GLOW_STEP = 5.0F;
constexpr float ACID_GLOW = 0.16F;

/// Translucent hazes laid over the backdrop: the first over the throat, the
/// second over the throat and the city, so each layer fades toward the
/// haze's colour the farther back it is and the walkways read first.
godot::Color far_haze() { return color(40, 22, 48, 0.15F); }
godot::Color middle_haze() { return color(40, 22, 48, 0.12F); }
/// The supports under the walkways, and the footing under a shop, stand
/// between the backdrop and the play layer.
godot::Color support_tint() { return color(150, 140, 190, 1.0F); }
/// A support the acid has risen around is stained toward its green.
godot::Color corroded_support_tint() { return color(90, 125, 85, 1.0F); }
/// The contact shadow a prop casts on the floor it stands on.
godot::Color floor_shadow() { return color(8, 4, 12, 0.6F); }
/// The acid below its glowing surface, so a high sea does not flood the
/// screen behind the map with bright green.
godot::Color deep_acid_tint() { return color(32, 52, 48, 1.0F); }
godot::Color untinted() { return {1.0F, 1.0F, 1.0F}; }
/// The ground is lifted above its own colours, and its walked-on top edge
/// most of all.
godot::Color ground_tint() { return {1.4F, 1.32F, 1.45F}; }
godot::Color ground_edge_tint() { return {1.95F, 1.8F, 2.05F}; }

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

/// Draws `image` like `stand`, over a contact shadow along the box's floor, so
/// it sits on the floor rather than floating in front of it.
void stand_on_floor(godot::Node2D &canvas, const Art &art, Image image,
                    const godot::Rect2 &box) {
  const float width = art.size(image).x + 6.0F;
  canvas.draw_rect(rect(box.get_center().x - width / 2.0F,
                        box.position.y + box.size.y - 2.0F, width, 4.0F),
                   floor_shadow());
  stand(canvas, art, image, box);
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

  /// Places `text` with the left end of its baseline at `at`, or nearby, and
  /// tells whether it did: words for a spot off the play area are left out.
  bool add(const Art &art, Vector2 at, const godot::String &text, int size,
           godot::Color tint) {
    const godot::Rect2 wanted = plate_box(art, at, text, size);
    if (!room().intersects(wanted)) {
      return false;
    }
    static constexpr std::array DIRECTIONS{
        Vector2(0.0F, -1.0F), Vector2(0.0F, 1.0F),   Vector2(-1.0F, 0.0F),
        Vector2(1.0F, 0.0F),  Vector2(-1.0F, -1.0F), Vector2(1.0F, -1.0F),
        Vector2(-1.0F, 1.0F), Vector2(1.0F, 1.0F)};
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
    return true;
  }

  /// The play area between the top bar and the cards, where words may stand.
  [[nodiscard]] static godot::Rect2 room() {
    return {0.0F, LABEL_TOP, SCREEN_WIDTH, LABEL_BOTTOM - LABEL_TOP};
  }

  void paint(godot::Node2D &canvas, const Art &art) const {
    for (const auto &plate : placed) {
      paint_plate(canvas, art, plate);
    }
  }

  /// The plate the last successful `add` placed.
  [[nodiscard]] godot::Rect2 last() const {
    return placed.at(placed.size() - 1).box;
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
    const godot::Rect2 spaced = box.grow(PLATE_GAP);
    return room().encloses(box) &&
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

bool visible(const ObjectSnapshot &object) {
  return object.state != ObjectState::Dormant;
}

/// A bag emptied or carried off leaves the map; the next bag may land on its
/// cell. Every other source keeps showing its count.
bool gone(const survival::SourceStock &stock, const ObjectSnapshot &object) {
  return stock.kind == SourceKind::Bag &&
         (stock.remaining == 0 || object.state == ObjectState::Destroyed);
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

/// The floor line of a cell's row: the feet of an actor standing in it, and
/// the top of the walkway under it.
float floor_y(std::int32_t y) { return cell_y(y + 1); }

/// Whether the map has ground at `cell`.
bool has_ground(std::span<const survival::GroundCell> cells,
                survival::GridPosition cell) {
  return std::ranges::any_of(
      cells, [&](const auto &ground) { return ground.cell == cell; });
}

/// The deck picture of a whole cell of plain road, varied along the row so no
/// block repeats across the screen.
Image plain_deck(survival::GridPosition cell) {
  static constexpr std::array PLAIN{Image::Deck, Image::DeckRivets, Image::Deck,
                                    Image::DeckWorn};
  return PLAIN.at(static_cast<std::size_t>(cell.x * 3 + cell.y) % PLAIN.size());
}

/// Draws a walkway's lit picture: brighter than the scenery behind it, its
/// top edge brightest, so the surface a player walks on reads first.
void paint_surface(godot::Node2D &canvas, const Art &art, Image image,
                   Vector2 at) {
  art.image(canvas, image, at, 1.0F, false, ground_tint());
  art.image_part(canvas, image, rect(0.0F, 0.0F, CELL, GROUND_EDGE),
                 rect(at.x, at.y, CELL, GROUND_EDGE), ground_edge_tint());
}

/// A walkway deck with its top at `at`, eaten by the acid when `corroded`; a
/// snapped end keeps its torn shape.
void paint_deck(godot::Node2D &canvas, const Art &art, Image deck, Vector2 at,
                bool corroded) {
  const bool end = deck == Image::EndLeft || deck == Image::EndRight;
  paint_surface(canvas, art, corroded && !end ? Image::DeckCorroded : deck, at);
}

/// Which way a step or a slab rises, or that it lies level.
enum class Rise : std::uint8_t { Level, Right, Left };

/// How the tilted ground from column `first` to `last` of row `y` rises: to
/// the right where ground meets its left end a row down or its right end a
/// row up, to the left the other way round, and level where neither does.
Rise rise(std::span<const survival::GroundCell> cells, std::int32_t first,
          std::int32_t last, std::int32_t y) {
  if (has_ground(cells, {.x = first - 1, .y = y + 1}) ||
      has_ground(cells, {.x = last + 1, .y = y - 1})) {
    return Rise::Right;
  }
  if (has_ground(cells, {.x = last + 1, .y = y + 1}) ||
      has_ground(cells, {.x = first - 1, .y = y - 1})) {
    return Rise::Left;
  }
  return Rise::Level;
}

/// A tilted walkway deck, `width` pixels long from `at`, its high end on that
/// floor line and its low end a row down. Each column of pixels is lowered
/// along the tilt on its own, so the deck is one straight piece on whole
/// pixels.
void paint_slab(godot::Node2D &canvas, const Art &art, Image deck, Vector2 at,
                int width, Rise rises) {
  const Vector2 size = art.size(deck);
  for (int column = 0; column < width; ++column) {
    const int along = rises == Rise::Right ? width - 1 - column : column;
    const float drop = rises == Rise::Level
                           ? 0.0F
                           : std::floor(static_cast<float>(along) * CELL /
                                        static_cast<float>(width));
    const float x = at.x + static_cast<float>(column);
    const float y = at.y + drop;
    const float source = std::fmod(static_cast<float>(column), size.x);
    art.image_part(canvas, deck, rect(source, 0.0F, 1.0F, size.y),
                   rect(x, y, 1.0F, size.y), ground_tint());
    art.image_part(canvas, deck, rect(source, 0.0F, 1.0F, GROUND_EDGE),
                   rect(x, y, 1.0F, GROUND_EDGE), ground_edge_tint());
  }
}

/// Stairs cut from a walkway deck, one cell wide, falling to the right from
/// the floor line at `at` to the row below in steps of STAIR_STEP. Each step
/// is the deck's top, drawn brightest down to the diagonal the stairs span,
/// where a player walks on them, over the deck's face below that in its own
/// colours, a stringer facing away from the light.
void paint_stairs(godot::Node2D &canvas, const Art &art, Image deck,
                  Vector2 at) {
  const Vector2 size = art.size(deck);
  for (int column = 0; column < static_cast<int>(CELL); ++column) {
    const auto diagonal = static_cast<float>(column);
    const float drop = std::floor(diagonal / STAIR_STEP) * STAIR_STEP;
    const float lit = std::max(GROUND_EDGE, diagonal - drop + 1.0F);
    const float x = at.x + diagonal;
    const float y = at.y + drop;
    const float source = std::fmod(diagonal, size.x);
    art.image_part(canvas, deck, rect(source, 0.0F, 1.0F, lit),
                   rect(x, y, 1.0F, lit), ground_edge_tint());
    art.image_part(canvas, deck,
                   rect(source, STAIR_FACE, 1.0F, size.y - STAIR_FACE),
                   rect(x, y + lit, 1.0F, size.y - STAIR_FACE), untinted());
  }
}

/// Tilted ground. A run of it along one row is a fallen slab, drawn whole from
/// its first cell. A single cell is a step between floors: a ramp, one cell
/// of deck tilted, where it rises to the right, stairs cut from the deck where
/// it rises to the left, each spanning from the floor line of its own cell
/// down to the row below, and a level deck with no step or floor on a
/// diagonal. A deck the acid has reached is `corroded`.
void paint_tilted(godot::Node2D &canvas, const Art &art,
                  std::span<const survival::GroundCell> cells,
                  const survival::GroundCell &ground, bool corroded) {
  const auto [x, y] = ground.cell;
  const auto tilted = [&](std::int32_t column) {
    return std::ranges::any_of(cells, [&](const auto &other) {
      return other.kind == GroundKind::TiltedSlab &&
             other.cell == survival::GridPosition{.x = column, .y = y};
    });
  };
  if (tilted(x - 1)) {
    return;
  }
  std::int32_t last = x;
  while (tilted(last + 1)) {
    ++last;
  }
  const Vector2 at(cell_x(x), floor_y(y));
  const Rise rises = rise(cells, x, last, y);
  if (last == x && rises == Rise::Left) {
    paint_stairs(canvas, art, corroded ? Image::DeckCorroded : Image::Deck, at);
  } else {
    paint_slab(canvas, art, corroded ? Image::DeckCorroded : Image::DeckWorn,
               at, static_cast<int>(cell_x(last + 1) - at.x), rises);
  }
}

/// Whether the acid surface at `surface` has risen over the bottom of the
/// walkway deck whose top is at `top`.
bool corroded(const Art &art, float top, float surface) {
  return surface < top + art.size(Image::Deck).y;
}

/// Draws one ground cell of standing terrain under the floor line of its
/// cell, eaten by the acid once its surface at `surface` reaches the deck. A
/// landmark's terrain shows its own picture: the pier's iron deck in place of
/// the walkway, the rotten bridge's frame under its planks, and the ferry's
/// hull, drawn whole, in place of every cell it covers.
void paint_ground_cell(godot::Node2D &canvas, const Art &art,
                       std::span<const survival::GroundCell> cells,
                       const survival::GroundCell &ground,
                       std::optional<LandmarkKind> picture, float surface) {
  const Vector2 at(cell_x(ground.cell.x), floor_y(ground.cell.y));
  const bool eaten = corroded(art, at.y, surface);
  if (picture) {
    switch (*picture) {
    case LandmarkKind::Ferry:
      return;
    case LandmarkKind::Platform:
      paint_surface(canvas, art, Image::Platform, at);
      return;
    case LandmarkKind::Bridge:
      art.image(canvas, Image::Bridge, at);
      break;
    case LandmarkKind::Kiosk:
    case LandmarkKind::ColdStore:
      break;
    }
  }
  const auto standing_on = [&](Image image, float sink) {
    art.image(canvas, image, Vector2(at.x, at.y + sink - art.size(image).y),
              1.0F, false, ground_tint());
  };
  switch (ground.kind) {
  case GroundKind::TiltedSlab:
    paint_tilted(canvas, art, cells, ground, eaten);
    return;
  case GroundKind::PlankBridge:
    art.image(canvas, Image::PlankBridge, at, 1.0F, false, ground_tint());
    return;
  case GroundKind::Rubble:
    paint_deck(canvas, art, plain_deck(ground.cell), at, eaten);
    standing_on(Image::Rubble, 0.0F);
    return;
  case GroundKind::BuriedCar:
    paint_deck(canvas, art, plain_deck(ground.cell), at, eaten);
    standing_on(Image::BuriedCar, 0.0F);
    return;
  case GroundKind::Road:
    paint_deck(canvas, art, plain_deck(ground.cell), at, eaten);
    return;
  case GroundKind::CrackedRoad:
  case GroundKind::Pothole:
    paint_deck(canvas, art, Image::DeckWorn, at, eaten);
    return;
  case GroundKind::RoadEndLeft:
    paint_deck(canvas, art, Image::EndLeft, at, eaten);
    return;
  case GroundKind::RoadEndRight:
    paint_deck(canvas, art, Image::EndRight, at, eaten);
    return;
  }
  std::abort();
}

/// The cells of standing terrain whose ground is drawn, with the landmark
/// picture each shows.
struct Standing {
  survival::GroundCell ground;
  std::optional<LandmarkKind> picture;
};

std::vector<Standing> standing_ground(const survival::Content &content,
                                      std::span<const ObjectSnapshot> objects) {
  std::vector<Standing> found;
  found.reserve(content.ground.size());
  for (const auto &ground : content.ground) {
    const auto standing =
        std::ranges::find_if(objects, [&](const auto &object) {
          return object.kind == ObjectKind::Terrain &&
                 object.state == ObjectState::Intact &&
                 covers(object.area, ground.cell);
        });
    if (standing != objects.end()) {
      found.push_back(
          {.ground = ground, .picture = landmark(content, standing->id)});
    }
  }
  return found;
}

/// The lowest a support under column `x` reaches from `top`: the floor line
/// of the first walkway below it, or `map_bottom`, in the acid.
float support_bottom(const std::vector<Standing> &ground, std::int32_t x,
                     float top, float map_bottom) {
  float bottom = map_bottom;
  for (const auto &below : ground) {
    const float floor = floor_y(below.ground.cell.y);
    if (below.ground.cell.x == x && floor > top) {
      bottom = std::min(bottom, floor);
    }
  }
  return bottom;
}

/// One support picture repeated down from `top` to `bottom`, centred on
/// column `x`; each repeat the acid at `surface` has risen over is stained.
void paint_support(godot::Node2D &canvas, const Art &art, Image image,
                   std::int32_t x, float top, float bottom, float surface) {
  const Vector2 size = art.size(image);
  const float left = cell_x(x) + (CELL - size.x) / 2.0F;
  const auto pieces = static_cast<int>(std::ceil((bottom - top) / size.y));
  for (int piece = 0; piece < pieces; ++piece) {
    const float at = top + static_cast<float>(piece) * size.y;
    const float height = std::min(size.y, bottom - at);
    art.image_part(canvas, image, rect(0.0F, 0.0F, size.x, height),
                   rect(left, at, size.x, height),
                   at >= surface ? corroded_support_tint() : support_tint());
  }
}

/// The supports under the pier, which no building holds up: a support under
/// each end of its deck and under every SUPPORT_SPACING-th cell between,
/// down to the next walkway below or out of the map, into the acid. Each run
/// takes the next support picture in turn.
void paint_supports(godot::Node2D &canvas, const Art &art,
                    const std::vector<Standing> &ground, float map_bottom,
                    float surface) {
  static constexpr std::array SUPPORTS{Image::Pillar, Image::Column};
  const auto walkway = [&](const Standing &cell) {
    return cell.picture == LandmarkKind::Platform;
  };
  const auto solid = [&](const Standing &cell) {
    return walkway(cell) && cell.ground.kind != GroundKind::RoadEndLeft &&
           cell.ground.kind != GroundKind::RoadEndRight;
  };
  const auto beside = [&](std::size_t index, std::int32_t step,
                          const auto &kind) {
    const auto &cell = ground.at(index).ground.cell;
    return std::ranges::any_of(ground, [&](const Standing &other) {
      return other.ground.cell ==
                 survival::GridPosition{.x = cell.x + step, .y = cell.y} &&
             kind(other);
    });
  };
  std::size_t run = 0;
  for (std::size_t index = 0; index < ground.size(); ++index) {
    const auto &cell = ground.at(index);
    if (!walkway(cell)) {
      continue;
    }
    if (!beside(index, -1, walkway)) {
      ++run;
    }
    const auto [x, y] = cell.ground.cell;
    const bool ends = !beside(index, -1, solid) || !beside(index, 1, solid);
    if (!solid(cell) || (!ends && x % SUPPORT_SPACING != 0)) {
      continue;
    }
    const Image deck = cell.picture ? Image::Platform : Image::Deck;
    const float top = floor_y(y) + art.size(deck).y;
    paint_support(canvas, art, SUPPORTS.at(run % SUPPORTS.size()), x, top,
                  support_bottom(ground, x, top, map_bottom), surface);
  }
}

/// One run of walkway along a row, the roof or a floor of the sunken building
/// under it; `torn_left` and `torn_right` where the road is snapped off.
struct Run {
  std::int32_t first = 0;
  std::int32_t last = 0;
  std::int32_t y = 0;
  bool torn_left = false;
  bool torn_right = false;
};

/// The runs of walkway that are a building's roof or floor, top row first and
/// each row left to right: every walkway but the planks, the tilted ground and
/// the landmarks.
std::vector<Run> building_runs(const std::vector<Standing> &ground) {
  std::vector<survival::GroundCell> cells;
  for (const auto &cell : ground) {
    const auto kind = cell.ground.kind;
    if (!cell.picture && kind != GroundKind::PlankBridge &&
        kind != GroundKind::TiltedSlab) {
      cells.push_back(cell.ground);
    }
  }
  std::ranges::sort(cells, {}, [](const survival::GroundCell &cell) {
    return std::pair(cell.cell.y, cell.cell.x);
  });
  std::vector<Run> runs;
  for (const auto &cell : cells) {
    const auto [x, y] = cell.cell;
    if (runs.empty() || runs.back().y != y || runs.back().last + 1 != x) {
      runs.push_back({.first = x,
                      .last = x,
                      .y = y,
                      .torn_left = cell.kind == GroundKind::RoadEndLeft});
    }
    runs.back().last = x;
    runs.back().torn_right = cell.kind == GroundKind::RoadEndRight;
  }
  return runs;
}

/// `value` wrapped into [0, `period`).
float wrapped(float value, float period) {
  const float rest = std::fmod(value, period);
  return rest < 0.0F ? rest + period : rest;
}

/// How each row of repeats of a wall shifts sideways: by a whole number of
/// its `columns` of windows, picked by `seed`, so the windows stay in columns
/// while the rows do not repeat one another. One column never shifts.
struct Stagger {
  std::int32_t columns = 1;
  std::uint32_t seed = 0;
};

/// A spread of `seed` and `index` over the 32-bit values.
std::uint32_t scatter(std::uint32_t seed, std::int64_t index) {
  return (seed + static_cast<std::uint32_t>(index)) * 2654435761U >> 16U;
}

/// Repeats `image` over `box`, its pattern anchored at `anchor`, so a wall
/// drawn in pieces lines up across them; each row of repeats shifts by
/// `stagger`.
void tile(godot::Node2D &canvas, const Art &art, Image image,
          const godot::Rect2 &box, Vector2 anchor, godot::Color tint,
          Stagger stagger = {}) {
  const Vector2 size = art.size(image);
  const float right = box.position.x + box.size.x;
  const float bottom = box.position.y + box.size.y;
  for (float y = box.position.y; y < bottom;) {
    const float top = wrapped(y - anchor.y, size.y);
    const float height = std::min(size.y - top, bottom - y);
    const auto row =
        static_cast<std::int64_t>(std::floor((y - anchor.y) / size.y));
    const float shift =
        size.x / static_cast<float>(stagger.columns) *
        static_cast<float>(scatter(stagger.seed, row) %
                           static_cast<std::uint32_t>(stagger.columns));
    for (float x = box.position.x; x < right;) {
      const float left = wrapped(x - anchor.x - shift, size.x);
      const float width = std::min(size.x - left, right - x);
      art.image_part(canvas, image, rect(left, top, width, height),
                     rect(x, y, width, height), tint);
      x += width;
    }
    y += height;
  }
}

/// A wall picture of the sunken buildings and the columns of windows across
/// one repeat of it.
struct WallPicture {
  Image image{};
  std::int32_t columns = 1;
};
constexpr std::array WALLS{
    WallPicture{Image::FacadeConcrete, 2}, WallPicture{Image::FacadeOffice, 3},
    WallPicture{Image::FacadeBrick, 2}, WallPicture{Image::FacadeTenement, 2}};
/// The tints that set neighbouring buildings apart.
const std::array<godot::Color, 3> &wall_tints() {
  static const std::array TINTS{color(105, 82, 120, 1.0F),
                                color(120, 84, 108, 1.0F),
                                color(96, 82, 126, 1.0F)};
  return TINTS;
}
/// How much of its tint the wall of a building at the top of the scene keeps;
/// one whose roof is nearer the acid keeps more.
constexpr float FAR_SHADE = 0.6F;
/// The rows a leaning wall draws at one sideways offset; it leans a pixel
/// further each time.
constexpr float LEAN_ROWS = 20.0F;
/// The shadow a deck casts on the wall under it.
constexpr float DECK_SHADOW = 6.0F;
/// How far a broken corner falls away from the deck at a snapped end, and the
/// jagged steps along its line.
constexpr float BREAK_DROP = 40.0F;
constexpr float BREAK_JAG = 2.0F;
constexpr int BREAK_JAGS = 5;
/// The strips a broken corner is drawn in, across a cell.
constexpr float BREAK_STRIP = 2.0F;
constexpr int BREAK_STRIPS = static_cast<int>(CELL / BREAK_STRIP);
/// The flesh on a wall's foot, a little darker than its own colours so it
/// does not outshine the walkways, and the places along its picture a cell of
/// it starts at.
godot::Color waterline_tint() { return color(200, 170, 190, 1.0F); }
constexpr float WATERLINE_STEP = 8.0F;
constexpr std::uint32_t WATERLINE_PLACES = 8;
/// The acid's green light on a wall, laid in steps up from its surface.
constexpr int ACID_LIGHT_STEPS = 5;
constexpr float ACID_LIGHT_STEP = 8.0F;
godot::Color acid_light(int step) {
  return color(110, 255, 120,
               0.2F * static_cast<float>(ACID_LIGHT_STEPS - step) /
                   static_cast<float>(ACID_LIGHT_STEPS));
}

/// A building sunk under one run of walkway, from under the deck down to the
/// acid or the map's bottom. Its wall, tint and lean follow from where the run
/// starts, so it keeps its look whatever else is destroyed.
struct SunkenBuilding {
  Run run;
  /// The top row its deck hides, the deck's top unless the acid has eaten
  /// into it; the wall's top, under the deck; and the wall's bottom.
  float roof = 0.0F;
  float top = 0.0F;
  float bottom = 0.0F;
  /// The run's width, and the part of it no broken corner cuts into.
  float left = 0.0F;
  float right = 0.0F;
  float body_left = 0.0F;
  float body_right = 0.0F;
  /// The rows a broken corner cuts into end here.
  float broken_bottom = 0.0F;
  WallPicture wall;
  godot::Color tint;
  /// A pixel to the right, to the left, or none, every LEAN_ROWS down.
  int lean = 0;
  Vector2 anchor;
  /// Picks the shift of each row of the wall's repeats and where each cell of
  /// flesh at its foot starts.
  std::uint32_t seed = 0;

  /// How far the wall has leant at row `y`.
  [[nodiscard]] float shift(float y) const {
    return static_cast<float>(lean) * std::floor((y - top) / LEAN_ROWS);
  }
  /// Where the broken corner's edge falls `from_edge` pixels in from a
  /// snapped end.
  [[nodiscard]] float broken_top(float from_edge) const {
    const auto jag =
        static_cast<float>(static_cast<int>(from_edge) * 7 % BREAK_JAGS);
    return top + BREAK_DROP * (1.0F - from_edge / CELL) + BREAK_JAG * jag;
  }
  /// The columns the deck and the wall hide in every row of `box`, when
  /// they span its rows: the deck's whole cells, and under them the wall as
  /// it leans.
  [[nodiscard]] std::optional<std::pair<float, float>>
  hidden_columns(const godot::Rect2 &box) const {
    const float box_top = box.position.y;
    const float box_bottom = box.position.y + box.size.y;
    if (box_top < roof || box_bottom > bottom) {
      return std::nullopt;
    }
    float from = std::numeric_limits<float>::lowest();
    float to = std::numeric_limits<float>::max();
    if (box_top < top) {
      from = body_left;
      to = body_right;
    }
    if (box_bottom > top) {
      const float wall_top = std::max(box_top, top);
      const bool whole = wall_top >= broken_bottom;
      const float first = shift(wall_top);
      const float last = shift(box_bottom - 1.0F);
      from = std::max(from, (whole ? left : body_left) + std::max(first, last));
      to = std::min(to, (whole ? right : body_right) + std::min(first, last));
    }
    return std::pair(from, to);
  }
};

/// Whether the walls of `buildings`, one alone or side by side, hide all of
/// `box`.
bool hidden(std::span<const SunkenBuilding> buildings,
            const godot::Rect2 &box) {
  std::vector<std::pair<float, float>> spans;
  for (const auto &building : buildings) {
    if (const auto columns = building.hidden_columns(box)) {
      spans.push_back(*columns);
    }
  }
  std::ranges::sort(spans);
  float reach = box.position.x;
  for (const auto &[from, to] : spans) {
    if (from > reach) {
      break;
    }
    reach = std::max(reach, to);
  }
  return reach >= box.position.x + box.size.x;
}

/// The sunken building under `run`, with the acid at `surface`, unless the
/// acid has risen over the run's deck.
std::optional<SunkenBuilding> sunken_building(const Art &art, const Run &run,
                                              float surface, float map_bottom) {
  const float top = floor_y(run.y) + art.size(Image::Deck).y - GROUND_EDGE;
  const float bottom = std::min(surface, map_bottom);
  if (bottom <= top) {
    return std::nullopt;
  }
  const auto place = [&](std::int32_t across, std::int32_t down,
                         std::size_t count) {
    return static_cast<std::size_t>(run.first * across + run.y * down) % count;
  };
  // A higher building stands farther back, in deeper shade.
  const float depth =
      FAR_SHADE + (1.0F - FAR_SHADE) * std::min(1.0F, top / ACID_LOW);
  const godot::Color base = wall_tints().at(place(1, 2, wall_tints().size()));
  const float left = cell_x(run.first);
  const float right = cell_x(run.last + 1);
  const float floor = floor_y(run.y);
  return SunkenBuilding{
      .run = run,
      .roof = corroded(art, floor, surface) ? floor + 1.0F : floor,
      .top = top,
      .bottom = bottom,
      .left = left,
      .right = right,
      .body_left = run.torn_left ? left + CELL : left,
      .body_right = run.torn_right ? right - CELL : right,
      .broken_bottom =
          run.torn_left || run.torn_right
              ? std::min(bottom,
                         top + BREAK_DROP +
                             BREAK_JAG * static_cast<float>(BREAK_JAGS - 1))
              : top,
      .wall = WALLS.at(place(1, 3, WALLS.size())),
      .tint = godot::Color(base.r * depth, base.g * depth, base.b * depth),
      .lean = static_cast<int>(place(1, 1, 3)) - 1,
      .anchor = Vector2(static_cast<float>(place(23, 11, 64)), top),
      .seed = static_cast<std::uint32_t>(place(31, 17, 1U << 16U)),
  };
}

/// The sunken buildings under the walkways, the top rows' first, with the acid
/// at `surface`.
std::vector<SunkenBuilding>
sunken_buildings(const Art &art, const std::vector<Standing> &ground,
                 float surface, float map_bottom) {
  std::vector<SunkenBuilding> buildings;
  for (const auto &run : building_runs(ground)) {
    if (auto building = sunken_building(art, run, surface, map_bottom)) {
      buildings.push_back(*building);
    }
  }
  return buildings;
}

/// Draws a building's wall rows from `from` to `to` between `x_from` and
/// `x_to`: at once where it stands straight, and a band of LEAN_ROWS at a time
/// where it leans.
void paint_wall(godot::Node2D &canvas, const Art &art,
                const SunkenBuilding &building, float from, float to,
                float x_from, float x_to) {
  const Stagger stagger{.columns = building.wall.columns,
                        .seed = building.seed};
  for (float y = from; y < to;) {
    const float end =
        building.lean == 0
            ? to
            : std::min(to,
                       building.top +
                           (std::floor((y - building.top) / LEAN_ROWS) + 1.0F) *
                               LEAN_ROWS);
    const float offset = building.shift(y);
    tile(canvas, art, building.wall.image,
         rect(x_from + offset, y, x_to - x_from, end - y),
         building.anchor + Vector2(offset, 0.0F), building.tint, stagger);
    y = end;
  }
}

/// A sunken building: its wall, broken away at a snapped end; windows,
/// shutters, air conditioners, a sign and a drainpipe on it; the deck's
/// shadow; and where the acid at `surface` meets it, flesh grown over its
/// foot and the acid's green light on it.
void paint_sunken_building(godot::Node2D &canvas, const Art &art,
                           const SunkenBuilding &building, float surface) {
  const auto &run = building.run;
  const float top = building.top;
  const float bottom = building.bottom;
  if (building.body_right > building.body_left) {
    paint_wall(canvas, art, building, top, building.broken_bottom,
               building.body_left, building.body_right);
  }
  paint_wall(canvas, art, building, building.broken_bottom, bottom,
             building.left, building.right);
  for (int column = 0; column < BREAK_STRIPS; ++column) {
    const float strip = static_cast<float>(column) * BREAK_STRIP;
    const float strip_top = building.broken_top(strip + BREAK_STRIP / 2.0F);
    if (run.torn_left && building.left + strip < building.body_right) {
      paint_wall(canvas, art, building, strip_top, building.broken_bottom,
                 building.left + strip, building.left + strip + BREAK_STRIP);
    }
    if (run.torn_right &&
        building.right - strip - BREAK_STRIP >= building.body_left) {
      paint_wall(canvas, art, building, strip_top, building.broken_bottom,
                 building.right - strip - BREAK_STRIP, building.right - strip);
    }
  }
  const auto fixture = [&](Image image, float x, float y, godot::Color lit) {
    if (y + art.size(image).y <= bottom) {
      art.image(canvas, image, Vector2(std::round(x + building.shift(y)), y),
                1.0F, false, lit);
    }
  };
  const auto storeys =
      static_cast<std::int32_t>(std::floor((bottom - top) / CELL));
  for (std::int32_t x = run.first; x <= run.last; ++x) {
    if ((x == run.first && run.torn_left) ||
        (x == run.last && run.torn_right)) {
      continue;
    }
    for (std::int32_t storey = 0; storey < storeys; ++storey) {
      const float y = top + static_cast<float>(storey) * CELL;
      const auto pick = (x * 7 + storey * 13 + run.y * 5) % 11;
      if (storey == 0 && x == run.first + 1 + run.last % 2) {
        fixture(Image::WallSign, cell_x(x), y + 6.0F, untinted());
      } else if (storey == 0 && pick % 4 == 1) {
        fixture(Image::WallShopWindow, cell_x(x) + 1.0F, y + 2.0F, untinted());
      } else if (pick == 6 || pick == 9) {
        fixture(Image::WallAc, cell_x(x) + 3.0F, y + 12.0F, building.tint);
      } else if (storey == 1 && pick == 2) {
        fixture(Image::WallShutter, cell_x(x) + 2.0F, y + 10.0F, building.tint);
      }
    }
  }
  canvas.draw_rect(rect(building.left, top + GROUND_EDGE,
                        building.right - building.left, DECK_SHADOW),
                   floor_shadow());
  if (run.last - run.first >= 2) {
    fixture(Image::WallPipe, cell_x(run.last - (run.torn_right ? 1 : 0)) - 6.0F,
            top, building.tint);
  }
  if (bottom < surface) {
    return;
  }
  for (int step = 0; step < ACID_LIGHT_STEPS; ++step) {
    const float y = bottom - static_cast<float>(step + 1) * ACID_LIGHT_STEP;
    canvas.draw_rect(
        rect(building.body_left + building.shift(y), std::max(top, y),
             building.body_right - building.body_left, ACID_LIGHT_STEP),
        acid_light(step));
  }
  // The flesh is laid a cell at a time, each from its own place along the
  // picture, so it does not repeat along the wall.
  const float foot = bottom - art.size(Image::Waterline).y + 2.0F;
  const float flesh_top = std::max(top, foot);
  for (std::int32_t x = run.first; x <= run.last; ++x) {
    const float from =
        WATERLINE_STEP *
        static_cast<float>(scatter(building.seed, x) % WATERLINE_PLACES);
    const float at = cell_x(x) + building.shift(foot);
    tile(canvas, art, Image::Waterline,
         rect(at, flesh_top, CELL, bottom + 2.0F - flesh_top),
         Vector2(at - from, foot), waterline_tint());
  }
}

/// Draws the sunken buildings, the top rows' first, so a lower building stands
/// in front of a higher one.
void paint_sunken_buildings(godot::Node2D &canvas, const Art &art,
                            std::span<const SunkenBuilding> buildings,
                            float surface) {
  for (const auto &building : buildings) {
    paint_sunken_building(canvas, art, building, surface);
  }
}

/// Whether flesh has grown over the front edge of a cell of walkway deck.
bool overgrown(const survival::GroundCell &ground) {
  const bool plain = ground.kind == GroundKind::Road ||
                     ground.kind == GroundKind::CrackedRoad ||
                     ground.kind == GroundKind::Pothole;
  return plain && (ground.cell.x * 5 + ground.cell.y * 3) % 7 == 3;
}

/// The ferry's hull, then each walkway of standing terrain on its floor line,
/// eaten where the acid at `surface` has reached it, with flesh dripping over
/// the front edge of a few of them.
void paint_ground(godot::Node2D &canvas, const Art &art,
                  const survival::Content &content,
                  std::span<const ObjectSnapshot> objects,
                  const std::vector<Standing> &ground, float surface) {
  for (const auto &object : objects) {
    if (object.kind == ObjectKind::Terrain &&
        object.state == ObjectState::Intact &&
        landmark(content, object.id) == LandmarkKind::Ferry) {
      const auto box = area_rect(object.area);
      stand(canvas, art, Image::Ferry,
            godot::Rect2(box.position.x, box.position.y, box.size.x,
                         box.size.y + FERRY_SINK));
    }
  }
  for (const auto &cell : ground) {
    paint_ground_cell(canvas, art, content.ground, cell.ground, cell.picture,
                      surface);
  }
  const float drip = art.size(Image::FleshDrip).x;
  for (const auto &cell : ground) {
    if (!cell.picture && overgrown(cell.ground)) {
      art.image(canvas, Image::FleshDrip,
                Vector2(cell_x(cell.ground.cell.x) + (CELL - drip) / 2.0F,
                        floor_y(cell.ground.cell.y) + GROUND_EDGE));
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

/// One picture of the scenery and where its top-left corner goes.
struct Piece {
  Image image{};
  Vector2 at;
};

/// Fills a decoration's area with as many of its kind's pictures as fit side
/// by side, at least one, each centred in its share of the width.
void add_pieces(std::vector<Piece> &pieces, const Art &art,
                const survival::Decoration &decoration, std::size_t ordinal) {
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
    pieces.push_back({.image = image, .at = Vector2(x, y)});
  }
}

/// The scenery behind the ground when `back`, and in front of it otherwise.
std::vector<Piece>
decoration_pieces(const Art &art, const survival::Content &content, bool back) {
  std::array<std::size_t,
             static_cast<std::size_t>(DecorationKind::StreetLamp) + 1>
      seen{};
  std::vector<Piece> pieces;
  for (const auto &decoration : content.decorations) {
    auto &ordinal = seen.at(static_cast<std::size_t>(decoration.kind));
    if (backdrop(decoration.kind) == back) {
      add_pieces(pieces, art, decoration, ordinal);
    }
    ++ordinal;
  }
  return pieces;
}

/// Draws the scenery behind the ground when `back`, and in front of it
/// otherwise, but for a piece the walls of `hiding` hide whole.
void paint_decorations(godot::Node2D &canvas, const Art &art,
                       const survival::Content &content, bool back,
                       std::span<const SunkenBuilding> hiding) {
  for (const auto &piece : decoration_pieces(art, content, back)) {
    if (!hidden(hiding,
                godot::Rect2(piece.at.floor(), art.size(piece.image)))) {
      art.image(canvas, piece.image, piece.at);
    }
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

/// A shop front hung from the walkway above a building with a shop inside:
/// its roof under the deck, which is drawn over it, and its floor on a
/// footing of deck across the building's width, held up by a support under
/// each end down to the walkway below or into the acid at `surface`.
void paint_storefront(godot::Node2D &canvas, const Art &art,
                      const survival::WorldArea &area,
                      const std::vector<Standing> &ground, float map_bottom,
                      float surface) {
  const auto box = area_rect(area);
  const Vector2 shop = art.size(Image::Shop);
  const float floor = box.position.y + shop.y;
  art.image(canvas, Image::Shop,
            Vector2(box.get_center().x - shop.x / 2.0F, box.position.y));
  const float footing = art.size(Image::Deck).y;
  for (std::int32_t x = area.x; x < area.x + area.width; ++x) {
    art.image(canvas, Image::Deck, Vector2(cell_x(x), floor), 1.0F, false,
              support_tint());
  }
  for (const std::int32_t x : {area.x, area.x + area.width - 1}) {
    paint_support(canvas, art, Image::Pillar, x, floor + footing,
                  support_bottom(ground, x, floor, map_bottom), surface);
  }
}

/// A building is a wall of windows over its area, a shop front where a shop
/// is inside it, its landmark picture standing in it, or rubble once
/// destroyed. A building whose every cell is rubble ground is that rubble
/// heap already. Only what stands in front of the ground is drawn when
/// `front`, and only the rest otherwise.
void paint_building(godot::Node2D &canvas, const Art &art,
                    const survival::Content &content,
                    const ObjectSnapshot &object, bool front, bool shop,
                    const std::vector<Standing> &ground, float surface) {
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
      stand_on_floor(canvas, art, Image::Kiosk, box);
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
  if (shop) {
    paint_storefront(canvas, art, area, ground, cell_y(content.map.height),
                     surface);
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
  stand_on_floor(canvas, art, Image::NoodleVendor, box);
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

/// Whether a shop's source stands inside `building`.
bool shop_inside(const ObjectSnapshot &building, const Marker &marker) {
  return marker.stock.kind == SourceKind::Shop &&
         covers(building.area,
                {.x = marker.object.area.x, .y = marker.object.area.y});
}

/// A shop front is as tall as three cells, so it stands behind the ground. A
/// shop inside a building has that building for its front, or its rubble.
void paint_shop_front(godot::Node2D &canvas, const Art &art,
                      std::span<const ObjectSnapshot> objects,
                      const Marker &marker) {
  const bool housed = std::ranges::any_of(objects, [&](const auto &object) {
    return object.kind == ObjectKind::Building && shop_inside(object, marker);
  });
  if (marker.stock.kind == SourceKind::Shop && !housed) {
    stand_on_floor(canvas, art, Image::Shop, area_rect(marker.object.area));
  }
}

void paint_source(godot::Node2D &canvas, const Art &art, const Marker &marker) {
  const auto cell = area_rect(marker.object.area);
  const auto box = cell.grow(-2.0F);
  const auto tones = palette();
  if (marker.stock.kind != SourceKind::Shop) {
    stand_on_floor(canvas, art, source_image(marker.stock.kind, marker.object),
                   cell);
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

/// One actor's animation frames.
struct Frames {
  Image idle;
  std::array<Image, 4> walk;
  std::array<Image, 2> run;
  Image jump;
  Image fall;
  Image land;
  Image climb;
};

const Frames &frames(Actor actor) {
  static const Frames chef{.idle = Image::ChefIdle,
                           .walk = {Image::ChefWalk1, Image::ChefWalk2,
                                    Image::ChefWalk3, Image::ChefWalk4},
                           .run = {Image::ChefRun1, Image::ChefRun2},
                           .jump = Image::ChefJump,
                           .fall = Image::ChefFall,
                           .land = Image::ChefLand,
                           .climb = Image::ChefClimb};
  static const Frames appraiser{
      .idle = Image::AppraiserIdle,
      .walk = {Image::AppraiserWalk1, Image::AppraiserWalk2,
               Image::AppraiserWalk3, Image::AppraiserWalk4},
      .run = {Image::AppraiserRun1, Image::AppraiserRun2},
      .jump = Image::AppraiserJump,
      .fall = Image::AppraiserFall,
      .land = Image::AppraiserLand,
      .climb = Image::AppraiserClimb};
  switch (actor) {
  case Actor::Chef:
    return chef;
  case Actor::Appraiser:
    return appraiser;
  }
  std::abort();
}

/// The frame an actor shows and whether it is mirrored.
struct Pose {
  Image image{};
  bool mirrored = false;
};

/// On the ground: landing for a moment after it comes down, then running,
/// walking or standing, the stride frames advancing with distance.
Pose ground_pose(const Frames &set, const survival::Body &body,
                 const Motion &motion, std::chrono::microseconds now) {
  const bool left = body.heading == survival::Direction::Left;
  if (motion.landed_at >= 0us && now - motion.landed_at <= LANDING) {
    return {.image = set.land, .mirrored = left};
  }
  if (motion.moved_at < 0us || now - motion.moved_at > STRIDE) {
    return {.image = set.idle, .mirrored = left};
  }
  const auto x = static_cast<std::size_t>(std::abs(body.position.x));
  if (motion.speed > RUN_SPEED) {
    return {.image = set.run.at(x / RUN_STRIDE % set.run.size()),
            .mirrored = left};
  }
  return {.image = set.walk.at(x / WALK_STRIDE % set.walk.size()),
          .mirrored = left};
}

/// Faces the way the actor last moved sideways; a climber, seen from
/// behind, swaps its reaching arm every CLIMB_STRIDE sub-cells.
Pose pose(Actor actor, const survival::Body &body, const Motion &motion,
          std::chrono::microseconds now) {
  const auto &set = frames(actor);
  switch (body.stance) {
  case survival::Stance::Climbing:
    return {.image = set.climb,
            .mirrored = std::abs(body.position.y) / CLIMB_STRIDE % 2 != 0};
  case survival::Stance::Airborne:
    return {.image = body.fall_speed < 0 ? set.jump : set.fall,
            .mirrored = body.heading == survival::Direction::Left};
  case survival::Stance::Standing:
    return ground_pose(set, body, motion, now);
  }
  std::abort();
}

/// Where an actor's feet are drawn: its sub-cell position, eased across each
/// 100ms step so it moves smoothly between them.
Vector2 feet_of(const Art &art, Actor actor, const survival::Body &body) {
  const bool chef = actor == Actor::Chef;
  const Vector2 point = map_point(body.position);
  return {art.glide(chef ? "chef_feet_x" : "appraiser_feet_x", point.x),
          art.glide(chef ? "chef_feet_y" : "appraiser_feet_y", point.y)};
}

/// Draws an actor taller than its cell, its frame's bottom edge on its feet,
/// in the pose its body and its motion call for.
void paint_actor(godot::Node2D &canvas, const Art &art,
                 const survival::GameView &view, Actor actor,
                 const survival::Body &body, float offset) {
  const auto now = view.progression.world_time;
  const auto shown = pose(actor, body, art.track(actor, body, now), now);
  const Vector2 size = art.size(shown.image);
  const Vector2 feet = feet_of(art, actor, body);
  art.image(canvas, shown.image,
            Vector2(feet.x - size.x / 2.0F + offset, feet.y - size.y), 1.0F,
            shown.mirrored);
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

Party party_of(const survival::GameView &view) {
  Party party{.chef_out =
                  view.progression.chef.location == survival::Location::City,
              .appraiser_out = view.progression.appraiser.location ==
                               survival::Location::City};
  party.shared =
      party.chef_out && party.appraiser_out &&
      survival::cell_of(view.exploration.chef.body.position) ==
          survival::cell_of(view.exploration.appraiser.body.position);
  return party;
}

/// The sources the map draws, each with what its frame promises.
std::vector<Marker>
markers_of(const survival::GameView &view,
           const std::vector<survival::ObjectId> &out_of_reach) {
  const auto &objects = view.world.objects;
  std::vector<Marker> markers;
  for (const auto &stock : view.exploration.sources) {
    const auto object =
        std::ranges::find(objects, stock.id, &ObjectSnapshot::id);
    if (object != objects.end() && visible(*object) && !gone(stock, *object)) {
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
  return markers;
}

/// A mark at the edge of the play area pointing at a goal off the screen.
struct Pointer {
  Vector2 at;
  /// The unit direction from the mark to the goal.
  Vector2 toward;
  godot::String name;
};

/// The pointers to every goal whose area lies wholly off the play area.
std::vector<Pointer> pointers_to(const survival::GameView &view,
                                 std::span<const survival::ObjectId> goals,
                                 const View &camera) {
  const godot::Rect2 room = Plates::room();
  const godot::Rect2 inner = room.grow(-POINTER_INSET);
  std::vector<Pointer> found;
  for (const auto goal : goals) {
    const auto object =
        std::ranges::find(view.world.objects, goal, &ObjectSnapshot::id);
    if (object == view.world.objects.end()) {
      continue;
    }
    const auto box = to_screen(camera, area_rect(object->area));
    if (room.intersects(box)) {
      continue;
    }
    const Vector2 centre = box.get_center();
    const Vector2 at = centre.clamp(inner.position, inner.get_end());
    found.push_back({.at = at,
                     .toward = (centre - at).normalized(),
                     .name = from(survival::object_name(goal))});
  }
  return found;
}

void paint_pointer(godot::Node2D &canvas, const Pointer &pointer) {
  canvas.draw_circle(pointer.at, POINTER_SIZE + 6.0F, color(14, 12, 26, 1.0F));
  const Vector2 across(-pointer.toward.y, pointer.toward.x);
  const Vector2 back = pointer.at - pointer.toward * (POINTER_SIZE * 0.6F);
  godot::PackedVector2Array arrow;
  arrow.push_back(pointer.at + pointer.toward * POINTER_SIZE);
  arrow.push_back(back + across * (POINTER_SIZE * 0.8F));
  arrow.push_back(back - across * (POINTER_SIZE * 0.8F));
  canvas.draw_colored_polygon(arrow, palette().cyan);
}

/// The map's words and the pointers to goals off the screen.
struct Labels {
  Plates plates;
  std::vector<Pointer> pointers;
  /// The names of the sources whose count is on the play area, and the
  /// plates of those counts.
  godot::Array labelled;
  godot::Array counts;
};

/// Places the map's words on the screen in the order they claim a spot: each
/// source's count in its own cell, the pointers' names, then what stays put,
/// then the actors' tags, which move. No word covers a pointer or a source
/// other than its own count, and words for a spot off the play area are left
/// out.
Labels label_city(const Art &art, const survival::Content &content,
                  const survival::GameView &view, survival::ControlMap control,
                  const std::vector<Marker> &markers, const Shot &shot) {
  const auto &camera = shot.camera;
  const auto tones = palette();
  const auto red = color(255, 150, 150, 1.0F);
  const auto party = party_of(view);
  const auto on_screen = [&](const survival::WorldArea &area) {
    return to_screen(camera, area_rect(area));
  };
  Labels labels{.plates = {},
                .pointers = pointers_to(view, shot.goals, camera),
                .labelled = {},
                .counts = {}};
  auto &plates = labels.plates;
  for (const auto &pointer : labels.pointers) {
    const Vector2 reach(POINTER_SIZE + 6.0F, POINTER_SIZE + 6.0F);
    plates.keep_clear(godot::Rect2(pointer.at - reach, reach * 2.0F));
  }
  for (const auto &marker : markers) {
    const auto box = on_screen(marker.object.area).grow(-2.0F);
    if (plates.add(art, Vector2(box.position.x + 9.0F, box.position.y + 21.0F),
                   godot::String::num_int64(marker.stock.remaining), 15,
                   marker.promised ? tones.white : tones.muted)) {
      labels.labelled.push_back(from(survival::object_name(marker.object.id)));
      labels.counts.push_back(plates.last());
    }
  }
  // Only a source's own count may stand on it, so the sources are kept clear
  // once the counts are placed and before any other word.
  for (const auto &marker : markers) {
    plates.keep_clear(on_screen(marker.object.area));
  }
  for (const auto &pointer : labels.pointers) {
    const float width = art.text_width(pointer.name, 14);
    const float side =
        pointer.toward.x > 0.5F ? -POINTER_INSET - width : POINTER_INSET;
    plates.add(art, pointer.at + Vector2(side, 5.0F), pointer.name, 14,
               tones.cyan);
  }
  const auto base = on_screen(content.objects.front().area);
  plates.add(art, base.position + Vector2(18.0F, 22.0F), "BASE", 18,
             tones.gold);
  for (const auto &marker : markers) {
    const auto box = on_screen(marker.object.area).grow(-2.0F);
    const float label_y =
        marker.object.area.y == 0 || marker.object.area.y == 13
            ? box.position.y + box.size.y + 16.0F
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
    const auto box = on_screen(object.area);
    const bool broken = object.state == ObjectState::Destroyed;
    switch (object.kind) {
    case ObjectKind::Building:
      plates.add(art, Vector2(box.position.x + 4.0F, box.position.y + 14.0F),
                 from(survival::object_name(object.id)), 13, tones.muted);
      break;
    case ObjectKind::Npc:
      if (!broken) {
        plates.add(art,
                   Vector2(box.position.x - 32.0F,
                           box.position.y + box.size.y + 13.0F),
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
    const auto box = on_screen(area);
    plates.add(
        art,
        Vector2(box.position.x - 30.0F, box.position.y + box.size.y + 14.0F),
        "ACID DRIP: -1 health", 13, color(180, 255, 140, 1.0F));
  }
  const auto tag = [&](Actor actor, const survival::Body &body, float offset) {
    const Vector2 feet = to_screen(camera, feet_of(art, actor, body));
    plates.add(art,
               feet + Vector2((6.0F - CELL / 2.0F + offset) * camera.zoom,
                              -(CELL + TAG_RISE) * camera.zoom),
               tag_of(control, actor), 14, tones.cyan);
  };
  if (party.chef_out) {
    tag(Actor::Chef, view.exploration.chef.body, 0.0F);
  }
  if (party.appraiser_out) {
    tag(Actor::Appraiser, view.exploration.appraiser.body,
        party.shared ? SHARED_CELL_TAG_SHIFT : 0.0F);
  }
  return labels;
}

/// Rings of light, each adding a little more toward the middle, for a canvas
/// that blends by adding.
void halo(godot::Node2D &canvas, Vector2 centre, float radius,
          godot::Color tint) {
  for (int ring = HALO_RINGS; ring >= 1; --ring) {
    canvas.draw_circle(centre,
                       radius * static_cast<float>(ring) /
                           static_cast<float>(HALO_RINGS),
                       tint);
  }
}

/// Light rising off an acid surface at `surface` across the scene, brightest
/// at the surface.
void surface_glow(godot::Node2D &canvas, float surface) {
  for (int step = 0; step < GLOW_STEPS; ++step) {
    const float fade =
        1.0F - static_cast<float>(step) / static_cast<float>(GLOW_STEPS);
    canvas.draw_rect(rect(0.0F,
                          surface - static_cast<float>(step + 1) * GLOW_STEP,
                          SCREEN_WIDTH, GLOW_STEP),
                     color(110, 255, 90, ACID_GLOW * fade));
  }
}

/// The acid band's surface on the screen at `acid_ratio`.
float band_surface(float acid_ratio) {
  return LABEL_BOTTOM - (BAND_LOW + std::clamp(acid_ratio, 0.0F, 1.0F) *
                                        (BAND_HIGH - BAND_LOW));
}

/// Draws the acid picture with its top edge at `top`, from `left` across
/// `width`, mirrored where `width` is negative: its surface keeps the
/// picture's glow, and below it the sea fades dark over a few steps, so what
/// stands in front of it reads.
void paint_sea(godot::Node2D &canvas, const Art &art, float left, float top,
               float width) {
  const Vector2 acid = art.size(Image::Acid);
  const auto band = [&](float from, float height, godot::Color tint) {
    art.image_part(canvas, Image::Acid, rect(0.0F, from, acid.x, height),
                   rect(left, top + from, width, height), tint);
  };
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
                   rect(left, bottom, width, SCREEN_HEIGHT - bottom),
                   deep_acid_tint());
  }
}

/// Draws a backdrop layer at its own size, its top-left at `at` on the screen
/// moved by `drift`, beside mirrored copies of itself that meet it without a
/// seam, so it covers the screen's width wherever it slides.
void paint_layer(godot::Node2D &canvas, const Art &art, Image image, Vector2 at,
                 Vector2 drift) {
  const float width = art.size(image).x;
  for (int copy = -1; copy <= 1; ++copy) {
    art.image(canvas, image,
              at + drift + Vector2(static_cast<float>(copy) * width, 0.0F),
              1.0F, copy != 0);
  }
}

/// The acid band nearest behind the map: the silhouettes of sunken trains
/// and broken pillars in a sea glowing at its surface, pinned to the bottom of
/// the play area and rising with the acid, slid sideways by `slide`.
void paint_band(godot::Node2D &canvas, const Art &art, float acid_ratio,
                Vector2 slide) {
  const float surface = std::floor(band_surface(acid_ratio));
  const Vector2 acid = art.size(Image::Acid);
  for (int copy = -1; copy <= 1; ++copy) {
    const float left = slide.x + static_cast<float>(copy) * acid.x;
    for (std::size_t pillar = 0; pillar < BAND_PILLARS.size(); ++pillar) {
      const Image image =
          pillar % 2 == 0 ? Image::PillarSnapped : Image::PillarDeck;
      art.image(canvas, image,
                Vector2(left + BAND_PILLARS.at(pillar),
                        surface + BAND_SINK - art.size(image).y));
    }
  }
  for (int copy = -1; copy <= 1; ++copy) {
    const float left = slide.x + static_cast<float>(copy) * acid.x;
    // Odd copies are mirrored, so neighbours meet without a seam.
    if (copy % 2 != 0) {
      paint_sea(canvas, art, left + acid.x, surface - ACID_SURFACE_DEPTH,
                -acid.x);
    } else {
      paint_sea(canvas, art, left, surface - ACID_SURFACE_DEPTH, acid.x);
    }
  }
}

} // namespace

float acid_surface(float acid_ratio) {
  return ACID_LOW - std::clamp(acid_ratio, 0.0F, 1.0F) * (ACID_LOW - MAP_Y);
}

Vector2 map_point(survival::Position position) {
  constexpr float PIXELS = CELL / static_cast<float>(survival::SUBCELLS);
  return {MAP_X + static_cast<float>(position.x) * PIXELS,
          MAP_Y + static_cast<float>(position.y) * PIXELS};
}

void paint_mouth(godot::Node2D &canvas, const Art &art, float acid_ratio,
                 const View &camera) {
  const Vector2 throat(drift(camera, THROAT_DEPTH).x, 0.0F);
  const Vector2 city = drift(camera, CITY_DEPTH);
  const Vector2 band(drift(camera, BAND_DEPTH).x, 0.0F);
  // Wider than the screen, so a shaken screen shows no unhazed edge.
  const godot::Rect2 hazed =
      rect(0.0F, 0.0F, SCREEN_WIDTH, SCREEN_HEIGHT).grow(CELL);
  art.look(canvas, Vector2(), 1.0F);
  paint_layer(canvas, art, Image::Throat, Vector2(), throat);
  canvas.draw_rect(hazed, far_haze());
  paint_layer(canvas, art, Image::City,
              Vector2(0.0F, CITY_BOTTOM - art.size(Image::City).y), city);
  canvas.draw_rect(hazed, middle_haze());
  paint_band(canvas, art, acid_ratio, band);
  godot::Array layers;
  layers.push_back(throat);
  layers.push_back(city);
  layers.push_back(band);
  canvas.set_meta("backdrop_layers", layers);
  art.look(canvas, origin(camera), camera.zoom);
  paint_sea(canvas, art, 0.0F,
            std::floor(acid_surface(acid_ratio) - ACID_SURFACE_DEPTH),
            art.size(Image::Acid).x);
}

void paint_glow(godot::Node2D &canvas, const Art &art, float acid_ratio,
                const View &camera) {
  art.look(canvas, Vector2(), 1.0F);
  surface_glow(canvas, std::floor(band_surface(acid_ratio)));
  art.look(canvas, origin(camera), camera.zoom);
  surface_glow(canvas, acid_surface(acid_ratio));
}

void paint_city_glow(godot::Node2D &canvas, const Art &art,
                     const survival::Content &content, const View &camera) {
  art.look(canvas, origin(camera), camera.zoom);
  for (const auto &piece : decoration_pieces(art, content, false)) {
    if (piece.image == Image::LampStanding || piece.image == Image::LampBent) {
      halo(canvas,
           piece.at + Vector2(art.size(piece.image).x / 2.0F, LAMP_HEAD),
           LAMP_GLOW, color(255, 170, 80, LAMP_LIGHT));
    }
  }
  for (const auto &area : content.hazards) {
    const auto box = area_rect(area);
    halo(canvas, box.get_center(), DRIP_GLOW, color(110, 255, 90, DRIP_LIGHT));
  }
}

void paint_city(godot::Node2D &canvas, const Art &art,
                const survival::Content &content,
                const survival::GameView &view,
                const std::vector<survival::ObjectId> &out_of_reach) {
  const auto &objects = view.world.objects;
  const auto paint_kind = [&](ObjectKind kind, const auto &paint) {
    for (const auto &object : objects) {
      if (object.kind == kind && visible(object)) {
        paint(object);
      }
    }
  };
  const auto markers = markers_of(view, out_of_reach);
  const auto party = party_of(view);
  const auto ground = standing_ground(content, objects);
  const float surface =
      acid_surface(static_cast<float>(view.progression.acid.value) /
                   static_cast<float>(content.run.full_acid.value));
  const auto buildings =
      sunken_buildings(art, ground, surface, cell_y(content.map.height));
  paint_decorations(canvas, art, content, true, buildings);
  paint_sunken_buildings(canvas, art, buildings, surface);
  paint_supports(canvas, art, ground, cell_y(content.map.height), surface);
  const auto paint_buildings = [&](bool front) {
    paint_kind(ObjectKind::Building, [&](const ObjectSnapshot &object) {
      const bool shop = std::ranges::any_of(markers, [&](const auto &marker) {
        return shop_inside(object, marker);
      });
      paint_building(canvas, art, content, object, front, shop, ground,
                     surface);
    });
  };
  paint_buildings(false);
  for (const auto &marker : markers) {
    paint_shop_front(canvas, art, objects, marker);
  }
  paint_ground(canvas, art, content, objects, ground, surface);
  paint_buildings(true);
  paint_hazards(canvas, art, content);
  paint_kind(ObjectKind::Entrance, [&](const ObjectSnapshot &object) {
    paint_passage(canvas, art, object, false);
  });
  paint_kind(ObjectKind::Ladder, [&](const ObjectSnapshot &object) {
    paint_passage(canvas, art, object, true);
  });
  paint_decorations(canvas, art, content, false, {});
  paint_kind(ObjectKind::Npc, [&](const ObjectSnapshot &object) {
    paint_npc(canvas, art, object);
  });
  for (const auto &marker : markers) {
    paint_source(canvas, art, marker);
  }
  if (party.chef_out) {
    paint_actor(canvas, art, view, Actor::Chef, view.exploration.chef.body,
                0.0F);
  }
  if (party.appraiser_out) {
    paint_actor(canvas, art, view, Actor::Appraiser,
                view.exploration.appraiser.body, party.shared ? 10.0F : 0.0F);
  }
}

void paint_city_labels(godot::Node2D &canvas, const Art &art,
                       const survival::Content &content,
                       const survival::GameView &view,
                       survival::ControlMap control, const Shot &shot) {
  const auto markers = markers_of(view, shot.out_of_reach);
  const auto labels = label_city(art, content, view, control, markers, shot);
  godot::Array pointed;
  for (const auto &pointer : labels.pointers) {
    paint_pointer(canvas, pointer);
    pointed.push_back(pointer.name);
  }
  labels.plates.paint(canvas, art);
  godot::Array drawn;
  godot::Array areas;
  for (const auto &marker : markers) {
    drawn.push_back(from(survival::object_name(marker.object.id)));
    areas.push_back(to_screen(shot.camera, area_rect(marker.object.area)));
  }
  canvas.set_meta("map_plates", labels.plates.boxes());
  canvas.set_meta("map_sources", drawn);
  canvas.set_meta("map_source_areas", areas);
  canvas.set_meta("map_labelled", labels.labelled);
  canvas.set_meta("map_counts", labels.counts);
  canvas.set_meta("map_pointers", pointed);
  canvas.set_meta("map_view", transform(shot.camera));
  canvas.set_meta("map_heading", transform(shot.heading));
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
