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
#include <numbers>
#include <optional>
#include <span>
#include <string_view>

namespace neocity::rendering {
namespace {

using namespace std::chrono_literals;
using godot::Vector2;
using survival::Actor;
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
constexpr float ACID_SURFACE_DEPTH = 61.0F;
/// Below its surface the acid fades to its deep colour over this many steps
/// of this many pixels.
constexpr int ACID_FADE_STEPS = 6;
constexpr float ACID_FADE_STEP = 6.0F;
/// The rows of a ground tile's top edge, which a player walks on, drawn lit.
constexpr float GROUND_EDGE = 4.0F;
/// The dark top outline of the walkway deck pictures, which their lit top edge
/// is drawn from under, so the walked-on edge is the top of the deck. The
/// pier's picture has none: its top row is its brightest.
constexpr float OUTLINE = 1.0F;
/// The rise of each step of stairs, cut from a walkway deck, and the first row
/// of the deck's face, which stairs show under their steps.
constexpr float STAIR_STEP = 8.0F;
constexpr float STAIR_FACE = 10.0F;
/// The deck's warm lit rows under its outline, which each stair's tread is
/// stretched from.
constexpr float STAIR_TREAD = 2.0F;
/// Where each backdrop layer's top-left stands on the screen while the
/// camera rests. Each is cut from the search mockup, and they stand so that
/// the first city screen of the capture, 4-city-collect-prompt, which slides
/// them 70, 164 and 281 pixels right of here, shows each where the mockup
/// does with the mockup's column 196 at the screen's left edge: the throat,
/// cut from the mockup's column 560 and top row, its fangs hanging from the
/// screen's top edge; the city, cut from its column 640 and row 60; and the
/// acid band, cut from its column 0.
constexpr Vector2 THROAT_AT(333.0F, 0.0F);
constexpr Vector2 CITY_AT(266.0F, 36.0F);
constexpr float BAND_LEFT = -384.0F;
/// The part of the kitchen picture the workshop shows between the top bar and
/// the panels, with the kitchen floor along its bottom.
constexpr float KITCHEN_SOURCE_Y = 364.0F;
constexpr float KITCHEN_Y = 62.0F;
constexpr float KITCHEN_HEIGHT = 256.0F;
constexpr float KITCHEN_FLOOR = 310.0F;
constexpr float WORKSHOP_SCALE = 2.0F;
constexpr Vector2 FEED_BUCKET(1205.0F, 202.0F);
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
/// How far the ferry's hull sinks below its floor line, so its deck is there.
constexpr float FERRY_SINK = 64.0F;
/// Where the map's words may stand: below the top bar, above the actor cards.
constexpr float LABEL_TOP = 62.0F;
constexpr float LABEL_BOTTOM = 540.0F;
/// How much of its light an out-of-reach source's icon keeps.
constexpr float OUT_OF_REACH_SHADE = 0.6F;
/// The sizes asked for a source's count and a goal's name on the map.
constexpr int COUNT_SIZE = 16;
constexpr int NAME_SIZE = 16;
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
/// full acid; at acid 0 its surface is the mockup's, at screen row 489.
constexpr float BAND_LOW = 51.0F;
constexpr float BAND_HIGH = 300.0F;
/// A halo's rings, each adding the same light.
constexpr int HALO_RINGS = 6;
constexpr float DRIP_GLOW = 30.0F;
constexpr float DRIP_LIGHT = 0.06F;

/// The throat breathes over this many seconds, lifting this many pixels and
/// brightening by this share as it draws in.
constexpr float BREATH_SECONDS = 5.0F;
constexpr float BREATH_LIFT = 4.0F;
constexpr float BREATH_GLOW = 0.08F;
/// Spores drifting through the air, far behind the map and near in front of
/// it: how many in each, how fast they rise in pixels per second, and how
/// far the camera moves them, as for the backdrop's layers.
constexpr int SPORES = 28;
constexpr float SPORE_RISE = 7.0F;
constexpr float FAR_SPORE_DEPTH = 0.25F;
constexpr float NEAR_SPORE_DEPTH = 0.8F;
/// Steam over an acid surface: a puff every this many screen pixels along
/// it, each rising this many over about this many seconds.
constexpr float STEAM_SPACING = 36.0F;
constexpr float STEAM_RISE = 80.0F;
constexpr float STEAM_SECONDS = 3.4F;
/// Seeds of the ambient motion's scatter, one per kind of thing moving.
constexpr std::uint32_t FAR_SPORE_SEED = 0x5B0EU;
constexpr std::uint32_t NEAR_SPORE_SEED = 0x5B1EU;
constexpr std::uint32_t STEAM_SEED = 0x57EAU;

/// A deck the acid has risen around is stained toward its green.
godot::Color corroded_tint() { return color(150, 200, 140, 1.0F); }
/// The contact shadow a prop casts on the floor it stands on.
godot::Color floor_shadow() { return color(8, 4, 12, 0.6F); }
/// The acid glows above its own colours at its surface. Below it the band
/// behind the map stays a little less bright, the brightest layer on the
/// screen, while the sea in the scene, drawn at the camera's size over
/// everything behind the map, sinks to a deeper green.
godot::Color acid_tint() { return {1.1F, 1.2F, 1.1F}; }
godot::Color band_deep_tint() { return color(200, 240, 200, 1.0F); }
godot::Color sea_deep_tint() { return color(90, 150, 100, 1.0F); }
godot::Color untinted() { return {1.0F, 1.0F, 1.0F}; }
/// The stairs' stringer, under their treads, faces away from the light.
godot::Color stringer_tint() { return {0.7F, 0.7F, 0.75F}; }
/// The walked-on top edge of the ground is lit warm and bright.
godot::Color ground_edge_tint() { return {2.4F, 2.0F, 1.6F}; }

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

/// What a plate shows besides its words: an icon before them, and, for stock
/// no human can walk to, the icon faded inside a dashed rim, so the
/// difference is not colour alone.
struct Mark {
  std::optional<Image> icon;
  bool out_of_reach = false;
};

/// Words on an opaque plate, whose contrast no acid level can change, after
/// the mark's icon if it has one.
struct Plate {
  godot::Rect2 box;
  godot::String text;
  int size = 0;
  godot::Color tint;
  Mark mark;
};

/// The plate behind `text` at `size`, and the mark's icon before it, the left
/// end of its baseline, or the icon's bottom-left corner, at `at`.
godot::Rect2 plate_box(const Art &art, Vector2 at, const godot::String &text,
                       int size, const Mark &mark = {}) {
  const auto height = static_cast<float>(size);
  if (mark.icon) {
    const Vector2 icon = art.size(*mark.icon);
    return {at.x - 2.0F, at.y - icon.y - 2.0F,
            icon.x + art.text_width(text, size) + 6.0F, icon.y + 4.0F};
  }
  return {at.x - 2.0F, at.y - height, art.text_width(text, size) + 4.0F,
          height * 1.3F};
}

void paint_plate(godot::Node2D &canvas, const Art &art, const Plate &plate) {
  canvas.draw_rect(plate.box, color(14, 12, 26, 1.0F));
  const Vector2 corner = plate.box.position + Vector2(2.0F, 2.0F);
  if (const auto icon = plate.mark.icon) {
    const Vector2 size = art.size(*icon);
    const float shade = plate.mark.out_of_reach ? OUT_OF_REACH_SHADE : 1.0F;
    art.image(canvas, *icon, corner, 1.0F, false,
              godot::Color(shade, shade, shade));
    art.text(canvas, corner.x + size.x + 2.0F, corner.y + size.y - 4.0F,
             plate.text, plate.size, plate.tint);
  } else {
    art.text(canvas, plate.box.position.x + 2.0F,
             plate.box.position.y + static_cast<float>(plate.size), plate.text,
             plate.size, plate.tint);
  }
  if (plate.mark.out_of_reach) {
    dashed_frame(canvas, plate.box.grow(-1.0F), plate.tint);
  }
}

/// The map's words, none covering another: each takes the spot it asks for,
/// or the nearest free one, looking farther away step by step, and is left
/// out where none is free.
class Plates {
public:
  /// Keeps words off `area`.
  void keep_clear(godot::Rect2 area) { clear.push_back(area); }

  /// Places `text`, after the mark's icon if it has one, with the left end of
  /// its baseline, or the icon's bottom-left corner, at `at`, or nearby, and
  /// tells whether it did: words for a spot off the play area, or with no
  /// free spot near it, are left out.
  bool add(const Art &art, Vector2 at, const godot::String &text, int size,
           godot::Color tint, const Mark &mark = {}) {
    const godot::Rect2 wanted = plate_box(art, at, text, size, mark);
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
      return false;
    }
    placed.push_back(
        {.box = box, .text = text, .size = size, .tint = tint, .mark = mark});
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

  /// The words of every plate placed, in the order they were placed.
  [[nodiscard]] godot::Array words() const {
    godot::Array words;
    for (const auto &plate : placed) {
      words.push_back(plate.text);
    }
    return words;
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

/// The map's columns one screen of the mockup spans: the walkways, buildings
/// and shops of a column show the mockup's at that column of its screen.
constexpr std::int32_t MOCKUP_COLUMNS = 20;
/// How deep a band of the deck picture is, and the rows whose floor line each
/// band shows, top to bottom; any other row shows the last.
constexpr float DECK_DEPTH = 28.0F;
constexpr std::array DECK_ROWS{4, 5, 6, 8, 9, 12};
/// The rows whose building fronts each band of the wall picture shows, a
/// cell deep, top to bottom; any other row shows the last.
constexpr std::array WALL_ROWS{5, 6, 7};
/// The deck of this row is the kitchen loft's, two rows over the walkway under
/// it where the mockup's is a cell nearer: the wall picture's last band, the
/// mockup's loft face and the dark under it, fills the rows between.
constexpr std::int32_t LOFT_ROW = 6;
constexpr float UNDERSIDE_TOP = 3.0F * CELL;
constexpr float UNDERSIDE_DEPTH = 2.0F * CELL - DECK_DEPTH;
/// The floor rows whose shop fronts each band of the shop picture shows, two
/// cells deep, top to bottom, and how many cells across a shop front shows.
constexpr std::array SHOP_ROWS{6, 8};
constexpr float SHOP_DEPTH = 2.0F * CELL;
constexpr std::int32_t SHOP_CELLS = 3;
/// The posts, gauge pillar, cable and lamp of a screen of the mockup stand on
/// the floor line of this row, their picture's top this far above it.
constexpr std::int32_t STRUCTURE_ROW = 8;
constexpr float STRUCTURE_RISE = 65.0F;

/// Where column `x` falls on the screen of the mockup the pictures are cut
/// from.
float mockup_x(std::int32_t x) {
  return static_cast<float>(x % MOCKUP_COLUMNS) * CELL;
}

/// The band of a picture cut in bands, one per row of `rows`, that `row` shows.
template <std::size_t N>
float band(const std::array<int, N> &rows, std::int32_t row) {
  const auto found = std::ranges::find(rows, row);
  return static_cast<float>(
      found == rows.end() ? N - 1
                          : static_cast<std::size_t>(found - rows.begin()));
}

/// The part of the deck picture a cell of ground in `cell` shows.
godot::Rect2 deck_source(survival::GridPosition cell) {
  return rect(mockup_x(cell.x), band(DECK_ROWS, cell.y) * DECK_DEPTH, CELL,
              DECK_DEPTH);
}

/// A walkway deck under the floor line of `cell`, stained by the acid when
/// `corroded`, under its top edge lit warm and bright, so the surface a player
/// walks on reads first.
void paint_deck(godot::Node2D &canvas, const Art &art,
                survival::GridPosition cell, bool corroded) {
  const godot::Rect2 source = deck_source(cell);
  const godot::Color tint = corroded ? corroded_tint() : untinted();
  const Vector2 at(cell_x(cell.x), floor_y(cell.y));
  art.image_part(canvas, Image::Deck, source,
                 rect(at.x, at.y, CELL, DECK_DEPTH), tint);
  art.image_part(
      canvas, Image::Deck,
      rect(source.position.x, source.position.y + OUTLINE, CELL, GROUND_EDGE),
      rect(at.x, at.y, CELL, GROUND_EDGE), tint * ground_edge_tint());
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

/// Stairs cut from a walkway deck, one cell wide, falling to the right from
/// the floor line at `at` to the row below in steps of STAIR_STEP. Each step
/// is a tread of the deck's warm lit rows, stretched down to the diagonal the
/// stairs span, where a player walks on them, over the deck's face below that
/// in its own colours shaded, a stringer facing away from the light.
void paint_stairs(godot::Node2D &canvas, const Art &art,
                  survival::GridPosition cell, godot::Color tint) {
  const godot::Rect2 deck = deck_source(cell);
  const Vector2 at(cell_x(cell.x), floor_y(cell.y));
  for (int column = 0; column < static_cast<int>(CELL); ++column) {
    const auto diagonal = static_cast<float>(column);
    const float drop = std::floor(diagonal / STAIR_STEP) * STAIR_STEP;
    const float lit = std::max(GROUND_EDGE, diagonal - drop + 1.0F);
    const float x = at.x + diagonal;
    const float y = at.y + drop;
    const float source = deck.position.x + diagonal;
    art.image_part(canvas, Image::Deck,
                   rect(source, deck.position.y + OUTLINE, 1.0F, STAIR_TREAD),
                   rect(x, y, 1.0F, lit), tint * ground_edge_tint());
    art.image_part(canvas, Image::Deck,
                   rect(source, deck.position.y + STAIR_FACE, 1.0F,
                        DECK_DEPTH - STAIR_FACE),
                   rect(x, y + lit, 1.0F, DECK_DEPTH - STAIR_FACE),
                   tint * stringer_tint());
  }
}

/// Tilted ground: stairs cut from the deck, one cell wide, where it rises to
/// the left, spanning from the floor line of its own cell down to the row
/// below; any other tilted cell is drawn as a level deck. A deck the acid has
/// reached is `corroded`.
void paint_tilted(godot::Node2D &canvas, const Art &art,
                  std::span<const survival::GroundCell> cells,
                  const survival::GroundCell &ground, bool corroded) {
  const auto [x, y] = ground.cell;
  if (rise(cells, x, x, y) == Rise::Left) {
    paint_stairs(canvas, art, ground.cell,
                 corroded ? corroded_tint() : untinted());
  } else {
    paint_deck(canvas, art, ground.cell, corroded);
  }
}

/// Whether the acid surface at `surface` has risen over the bottom of the
/// walkway deck whose top is at `top`.
bool corroded(float top, float surface) { return surface < top + DECK_DEPTH; }

/// Draws one ground cell of standing terrain under the floor line of its
/// cell, the band of the mockup's walkway at its row and column, stained once
/// the acid at `surface` reaches the deck, under the kitchen loft's deck its
/// underside down to the walkway below, and a rubble heap standing on it
/// where the ground is rubble. The ferry's hull is drawn whole in place of
/// every cell it covers.
void paint_ground_cell(godot::Node2D &canvas, const Art &art,
                       std::span<const survival::GroundCell> cells,
                       const survival::GroundCell &ground,
                       std::optional<LandmarkKind> picture, float surface) {
  if (picture == LandmarkKind::Ferry) {
    return;
  }
  const Vector2 at(cell_x(ground.cell.x), floor_y(ground.cell.y));
  const bool eaten = corroded(at.y, surface);
  if (ground.kind == GroundKind::TiltedSlab) {
    paint_tilted(canvas, art, cells, ground, eaten);
    return;
  }
  paint_deck(canvas, art, ground.cell, eaten);
  if (ground.cell.y == LOFT_ROW) {
    art.image_part(
        canvas, Image::BuildingWall,
        rect(mockup_x(ground.cell.x), UNDERSIDE_TOP, CELL, UNDERSIDE_DEPTH),
        rect(at.x, at.y + DECK_DEPTH, CELL, UNDERSIDE_DEPTH));
  }
  if (ground.kind == GroundKind::Rubble) {
    art.image(canvas, Image::Rubble,
              Vector2(at.x, at.y - art.size(Image::Rubble).y));
  }
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

/// The posts, chain and gauge pillar under the walkways, and the cable and
/// lamp on them, where the mockup has them: its picture of a screen's worth
/// once for each MOCKUP_COLUMNS of the map, on the floor line of
/// STRUCTURE_ROW.
void paint_structure(godot::Node2D &canvas, const Art &art,
                     const survival::Content &content) {
  for (std::int32_t x = 0; x < content.map.width; x += MOCKUP_COLUMNS) {
    art.image(canvas, Image::Structure,
              Vector2(cell_x(x), floor_y(STRUCTURE_ROW) - STRUCTURE_RISE));
  }
}

/// `value` wrapped into [0, `period`).
float wrapped(float value, float period) {
  const float rest = std::fmod(value, period);
  return rest < 0.0F ? rest + period : rest;
}

/// A spread of `seed` and `index` over [0, 1).
float unit(std::uint32_t seed, std::int64_t index) {
  constexpr std::uint32_t SPREAD = 1024U;
  const std::uint32_t spread =
      (seed + static_cast<std::uint32_t>(index)) * 2654435761U >> 16U;
  return static_cast<float>(spread % SPREAD) / static_cast<float>(SPREAD);
}

/// How far, from 0 to 1, a cycle of `period` seconds that starts `offset` of
/// the way in has run at `seconds`.
float cycle(float seconds, float period, float offset) {
  return wrapped(seconds / period + offset, 1.0F);
}

/// The throat's breath now, from 0 drawn out to 1 drawn in.
float breath(const Art &art) {
  return 0.5F - 0.5F * std::cos(art.seconds() * 2.0F *
                                std::numbers::pi_v<float> / BREATH_SECONDS);
}

/// Spores rising through the air over the whole screen, moved by `camera`
/// as a layer `depth` of the way to the map is: squares of `size` screen
/// pixels on whole screen pixels, in the first of `tints` and every other one
/// in the second, each twinkling.
void paint_spores(godot::Node2D &canvas, const Art &art, const View &camera,
                  float depth, float size, std::array<godot::Color, 2> tints,
                  std::uint32_t seed) {
  const Vector2 slide = drift(camera, depth);
  const float seconds = art.seconds();
  for (int spore = 0; spore < SPORES; ++spore) {
    const auto at = static_cast<float>(spore);
    const float rise = SPORE_RISE * (0.6F + unit(seed, spore));
    const float x = wrapped(unit(seed + 1U, spore) * SCREEN_WIDTH + slide.x +
                                6.0F * std::sin(seconds * 0.7F + at),
                            SCREEN_WIDTH);
    const float y = wrapped(unit(seed + 2U, spore) * SCREEN_HEIGHT + slide.y -
                                seconds * rise,
                            SCREEN_HEIGHT);
    godot::Color tint = tints.at(static_cast<std::size_t>(spore % 2));
    const float twinkle = 1.0F + 2.0F * unit(seed + 3U, spore);
    tint.a *= 0.55F + 0.45F * std::sin(seconds * twinkle + at);
    canvas.draw_rect(rect(std::floor(x), std::floor(y), size, size), tint);
  }
}

/// Steam rising off an acid surface at screen row `surface`, across the
/// screen's width from 0, in puffs on whole screen pixels that swell and fade
/// as they rise.
void paint_steam(godot::Node2D &canvas, const Art &art, float surface,
                 godot::Color tint) {
  const float seconds = art.seconds();
  const auto puffs = static_cast<int>(SCREEN_WIDTH / STEAM_SPACING);
  for (int puff = 0; puff <= puffs; ++puff) {
    const float run =
        cycle(seconds, STEAM_SECONDS * (0.8F + 0.5F * unit(STEAM_SEED, puff)),
              unit(STEAM_SEED + 1U, puff));
    const float size = std::floor(4.0F + 8.0F * run);
    const float x = (static_cast<float>(puff) + unit(STEAM_SEED + 2U, puff)) *
                        STEAM_SPACING +
                    6.0F * std::sin(run * 4.0F + static_cast<float>(puff));
    const float y = surface - run * STEAM_RISE;
    godot::Color fading = tint;
    fading.a *= std::sin(run * std::numbers::pi_v<float>);
    canvas.draw_rect(
        rect(std::floor(x - size / 2.0F), std::floor(y - size), size, size),
        fading);
  }
}

/// The ferry's hull, then each walkway of standing terrain on its floor line,
/// eaten where the acid at `surface` has reached it.
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
  for (const auto &object : objects) {
    if (object.kind == ObjectKind::Terrain &&
        object.state == ObjectState::Destroyed) {
      wreck_mark(canvas, area_rect(object.area));
    }
  }
}

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

/// A building is the mockup's building front at its rows and columns, its
/// landmark picture standing in it, or rubble once destroyed. A building
/// whose every cell is rubble ground is that rubble heap already. Only what
/// stands in front of the ground is drawn when `front`, and only the rest
/// otherwise.
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
    for (std::int32_t x = area.x; x < area.x + area.width; ++x) {
      stand(canvas, art, Image::Rubble,
            rect(cell_x(x), box.position.y, CELL, box.size.y));
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
  for (std::int32_t y = area.y; y < area.y + area.height; ++y) {
    for (std::int32_t x = area.x; x < area.x + area.width; ++x) {
      art.image_part(canvas, Image::BuildingWall,
                     rect(mockup_x(x), band(WALL_ROWS, y) * CELL, CELL, CELL),
                     rect(cell_x(x), cell_y(y), CELL, CELL));
    }
  }
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

/// Every bag is the mockup's backpack.
Image source_image(SourceKind kind) {
  switch (kind) {
  case SourceKind::VendingMachine:
    return Image::VendingMachine;
  case SourceKind::Npc:
    return Image::VendorCart;
  case SourceKind::Shop:
    return Image::Shop;
  case SourceKind::Bag:
    return Image::BagWashedUp;
  }
  std::abort();
}

/// The icon of what `source` holds, if the art set has one.
std::optional<Image> supply_icon(const ObjectSnapshot &source) {
  std::string_view name;
  if (source.tool) {
    name = survival::tool_name(*source.tool);
  } else if (source.ingredient) {
    name = survival::ingredient_name(*source.ingredient);
  }
  return item_icon(name);
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

/// A lit shop front is the mockup's, two cells deep over its floor and
/// SHOP_CELLS across around its column, kept inside the mockup's screen, and
/// stands behind the ground. A shop inside a building is drawn with that
/// building, or its rubble.
void paint_shop_front(godot::Node2D &canvas, const Art &art,
                      std::span<const ObjectSnapshot> objects,
                      const Marker &marker) {
  const bool housed = std::ranges::any_of(objects, [&](const auto &object) {
    return object.kind == ObjectKind::Building && shop_inside(object, marker);
  });
  if (marker.stock.kind != SourceKind::Shop || housed) {
    return;
  }
  const std::int32_t x = marker.object.area.x;
  const std::int32_t y = marker.object.area.y;
  const float span = static_cast<float>(SHOP_CELLS) * CELL;
  const float left =
      std::clamp(mockup_x(x) - CELL, 0.0F,
                 static_cast<float>(MOCKUP_COLUMNS) * CELL - span);
  art.image_part(canvas, Image::Shop,
                 rect(left, band(SHOP_ROWS, y) * SHOP_DEPTH, span, SHOP_DEPTH),
                 rect(cell_x(x) + left - mockup_x(x), floor_y(y) - SHOP_DEPTH,
                      span, SHOP_DEPTH));
}

void paint_source(godot::Node2D &canvas, const Art &art, const Marker &marker) {
  const auto cell = area_rect(marker.object.area);
  const auto box = cell.grow(-2.0F);
  const auto tones = palette();
  if (marker.stock.kind != SourceKind::Shop) {
    stand_on_floor(canvas, art, source_image(marker.stock.kind), cell);
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
  survival::ObjectId goal;
  /// The goal's screen name.
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
                     .goal = goal,
                     .name = place_name(view, goal)});
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
/// source's icon and count over its own cell, the pointers' names, the names
/// of the goals on the play area, then what stays put, then the actors' tags,
/// which move. Only a goal is named. No word covers a pointer, a source or
/// `covered`, and words for a spot off the play area, or with no free spot
/// near it, are left out.
Labels label_city(const Art &art, const survival::Content &content,
                  const survival::GameView &view, survival::ControlMap control,
                  const std::vector<Marker> &markers, const Shot &shot,
                  std::span<const godot::Rect2> covered) {
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
  for (const auto &area : covered) {
    plates.keep_clear(area);
  }
  for (const auto &pointer : labels.pointers) {
    const Vector2 reach(POINTER_SIZE + 6.0F, POINTER_SIZE + 6.0F);
    plates.keep_clear(godot::Rect2(pointer.at - reach, reach * 2.0F));
  }
  for (const auto &marker : markers) {
    plates.keep_clear(on_screen(marker.object.area));
  }
  for (const auto &marker : markers) {
    const auto cell = on_screen(marker.object.area);
    const auto count = godot::String::num_int64(marker.stock.remaining);
    const bool unreachable = marker.stocked_open && !marker.walkable;
    const Mark mark{.icon = supply_icon(marker.object),
                    .out_of_reach = unreachable};
    const float width =
        plate_box(art, Vector2(), count, COUNT_SIZE, mark).size.x;
    auto tint = tones.muted;
    if (marker.promised) {
      tint = tones.white;
    } else if (unreachable) {
      tint = tones.faint;
    }
    if (plates.add(art,
                   Vector2(cell.get_center().x - width / 2.0F + 2.0F,
                           cell.position.y - 2.0F - PLATE_GAP),
                   count, COUNT_SIZE, tint, mark)) {
      labels.labelled.push_back(from(survival::object_name(marker.object.id)));
      labels.counts.push_back(plates.last());
    }
  }
  for (const auto &pointer : labels.pointers) {
    const float width = art.text_width(pointer.name, NAME_SIZE);
    const float side =
        pointer.toward.x > 0.5F ? -POINTER_INSET - width : POINTER_INSET;
    plates.add(art, pointer.at + Vector2(side, 5.0F), pointer.name, NAME_SIZE,
               tones.cyan);
  }
  for (const auto goal : shot.goals) {
    const auto object =
        std::ranges::find(view.world.objects, goal, &ObjectSnapshot::id);
    if (object == view.world.objects.end()) {
      continue;
    }
    const auto box = on_screen(object->area);
    if (Plates::room().intersects(box)) {
      // The name asks for a spot inside the play area even where the goal
      // stands at its edge, so it is left out only when no spot is free.
      const auto name = place_name(view, goal);
      const godot::Rect2 room = Plates::room();
      const auto size = static_cast<float>(NAME_SIZE);
      const float width = art.text_width(name, NAME_SIZE);
      plates.add(
          art,
          Vector2(std::clamp(box.position.x, room.position.x + 2.0F,
                             room.get_end().x - width - 2.0F),
                  std::clamp(box.get_end().y + size, room.position.y + size,
                             room.get_end().y - size * 0.3F)),
          name, NAME_SIZE, tones.cyan);
    }
  }
  for (const auto &object : view.world.objects) {
    const bool passage = object.kind == ObjectKind::Ladder ||
                         object.kind == ObjectKind::Entrance;
    if (visible(object) && passage && object.state == ObjectState::Destroyed) {
      const auto box = on_screen(object.area);
      plates.add(art, Vector2(box.position.x + 36.0F, box.position.y + 21.0F),
                 "BROKEN", 13, red);
    }
  }
  for (const auto &area : content.hazards) {
    const auto box = on_screen(area);
    plates.add(
        art,
        Vector2(box.position.x - 8.0F, box.position.y + box.size.y + 36.0F),
        "-1", COUNT_SIZE, tones.danger, {.icon = Image::HeartFull});
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
/// that blends by adding, unless `camera` shows none of them. Each ring is
/// drawn in runs of whole pixel rows of one width, so its edge steps on the
/// pixel grid.
void halo(godot::Node2D &canvas, const View &camera, Vector2 centre,
          float radius, godot::Color tint) {
  const godot::Rect2 reach(centre - Vector2(radius, radius),
                           Vector2(radius, radius) * 2.0F);
  if (!to_screen(camera, reach)
           .intersects(rect(0.0F, 0.0F, SCREEN_WIDTH, SCREEN_HEIGHT))) {
    return;
  }
  const Vector2 middle = centre.floor();
  const auto width = [](int edge, int row) {
    const auto down = static_cast<float>(row) + 0.5F;
    const auto outer = static_cast<float>(edge);
    return std::round(std::sqrt(outer * outer - down * down));
  };
  for (int ring = HALO_RINGS; ring >= 1; --ring) {
    const auto edge = static_cast<int>(radius * static_cast<float>(ring) /
                                       static_cast<float>(HALO_RINGS));
    int row = -edge;
    while (row < edge) {
      const float half = width(edge, row);
      int rows = 1;
      while (row + rows < edge && width(edge, row + rows) == half) {
        ++rows;
      }
      canvas.draw_rect(rect(middle.x - half, middle.y + static_cast<float>(row),
                            2.0F * half, static_cast<float>(rows)),
                       tint);
      row += rows;
    }
  }
}

/// The acid band's surface on the screen at `acid_ratio`.
float band_surface(float acid_ratio) {
  return LABEL_BOTTOM - (BAND_LOW + std::clamp(acid_ratio, 0.0F, 1.0F) *
                                        (BAND_HIGH - BAND_LOW));
}

/// The screen row of the surface of the sea in the scene at `acid_ratio`, as
/// `camera` shows it, on a whole pixel.
float sea_row(const View &camera, float acid_ratio) {
  return std::floor(
      to_screen(camera, Vector2(0.0F, acid_surface(acid_ratio))).y);
}

/// Draws the acid picture with its top edge at `top`, from `left` across
/// `width`, mirrored where `width` is negative: its surface glows, and below
/// it the sea fades over a few steps to the `deep` tint.
void paint_sea(godot::Node2D &canvas, const Art &art, float left, float top,
               float width, godot::Color deep_tint) {
  const Vector2 acid = art.size(Image::Acid);
  const auto band = [&](float from, float height, godot::Color tint) {
    art.image_part(canvas, Image::Acid, rect(0.0F, from, acid.x, height),
                   rect(left, top + from, width, height), tint);
  };
  band(0.0F, ACID_SURFACE_DEPTH, acid_tint());
  for (int step = 0; step < ACID_FADE_STEPS; ++step) {
    band(ACID_SURFACE_DEPTH + static_cast<float>(step) * ACID_FADE_STEP,
         ACID_FADE_STEP,
         acid_tint().lerp(deep_tint,
                          static_cast<float>(step + 1) /
                              static_cast<float>(ACID_FADE_STEPS + 1)));
  }
  const float deep =
      ACID_SURFACE_DEPTH + static_cast<float>(ACID_FADE_STEPS) * ACID_FADE_STEP;
  band(deep, acid.y - deep, deep_tint);
  const float bottom = top + acid.y;
  if (bottom < SCREEN_HEIGHT) {
    art.image_part(canvas, Image::Acid, rect(0.0F, acid.y - 1.0F, acid.x, 1.0F),
                   rect(left, bottom, width, SCREEN_HEIGHT - bottom),
                   deep_tint);
  }
}

/// Draws a backdrop layer at its own size, its colours multiplied by `tint`,
/// its top-left at `at` on the screen moved by `drift`, beside mirrored copies
/// of itself that meet it without a seam, so it covers the screen's width
/// wherever it slides.
void paint_layer(godot::Node2D &canvas, const Art &art, Image image, Vector2 at,
                 Vector2 drift, godot::Color tint) {
  const float width = art.size(image).x;
  for (int copy = -1; copy <= 1; ++copy) {
    art.image(canvas, image,
              at + drift + Vector2(static_cast<float>(copy) * width, 0.0F),
              1.0F, copy != 0, tint);
  }
}

/// The acid picture at its own size with its surface at screen row
/// `surface`, slid sideways by `slide` beside mirrored copies of itself so it
/// covers the screen's width, fading below to the `deep` tint.
void paint_acid(godot::Node2D &canvas, const Art &art, float surface,
                float slide, godot::Color deep_tint) {
  const float width = art.size(Image::Acid).x;
  for (int copy = -1; copy <= 1; ++copy) {
    const float left = slide + static_cast<float>(copy) * width;
    // Odd copies are mirrored, so neighbours meet without a seam.
    if (copy % 2 != 0) {
      paint_sea(canvas, art, left + width, surface - ACID_SURFACE_DEPTH, -width,
                deep_tint);
    } else {
      paint_sea(canvas, art, left, surface - ACID_SURFACE_DEPTH, width,
                deep_tint);
    }
  }
}

/// The acid band nearest behind the map: the mockup's acid lake of sunken
/// trains and broken pillars glowing at its surface, pinned to the bottom of
/// the play area and rising with the acid, its picture's left edge at
/// `left`.
void paint_band(godot::Node2D &canvas, const Art &art, float acid_ratio,
                float left) {
  paint_acid(canvas, art, std::floor(band_surface(acid_ratio)), left,
             band_deep_tint());
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
  const float drawn_in = breath(art);
  // The throat breathes: it lifts and brightens as it draws in.
  const Vector2 breathing(0.0F, -std::round(drawn_in * BREATH_LIFT));
  const float flush = 1.0F + BREATH_GLOW * (drawn_in - 0.5F);
  art.look(canvas, Vector2(), 1.0F);
  paint_layer(canvas, art, Image::Throat, THROAT_AT + breathing, throat,
              godot::Color(flush, flush, flush));
  paint_layer(canvas, art, Image::City, CITY_AT, city, untinted());
  const float band_left = BAND_LEFT + band.x;
  paint_band(canvas, art, acid_ratio, band_left);
  // The sea in the scene is drawn at its own size too, moving with the map.
  const float sea = sea_row(camera, acid_ratio);
  paint_acid(canvas, art, sea, drift(camera, 1.0F).x, sea_deep_tint());
  godot::Array layers;
  layers.push_back(throat);
  layers.push_back(city);
  layers.push_back(band);
  canvas.set_meta("backdrop_layers", layers);
  canvas.set_meta("throat_at", THROAT_AT + breathing + throat);
  art.look(canvas, origin(camera), camera.zoom);
  art.note_waterline(std::floor(band_surface(acid_ratio)), band_left,
                     (sea - origin(camera).y) / camera.zoom);
}

void paint_glow(godot::Node2D &canvas, const Art &art, float acid_ratio,
                const View &camera) {
  const godot::Color steam = color(150, 255, 140, 0.16F);
  art.look(canvas, Vector2(), 1.0F);
  paint_steam(canvas, art, std::floor(band_surface(acid_ratio)), steam);
  paint_steam(canvas, art, sea_row(camera, acid_ratio), steam);
  paint_spores(canvas, art, camera, FAR_SPORE_DEPTH, 2.0F,
               {color(255, 150, 220, 0.5F), color(170, 255, 150, 0.4F)},
               FAR_SPORE_SEED);
  paint_spores(canvas, art, camera, NEAR_SPORE_DEPTH, 4.0F,
               {color(255, 80, 200, 0.45F), color(140, 255, 120, 0.4F)},
               NEAR_SPORE_SEED);
}

void paint_city_glow(godot::Node2D &canvas, const Art &art,
                     const survival::Content &content, const View &camera) {
  art.look(canvas, origin(camera), camera.zoom);
  for (const auto &area : content.hazards) {
    const auto box = area_rect(area);
    halo(canvas, camera, box.get_center(), DRIP_GLOW,
         color(110, 255, 90, DRIP_LIGHT));
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
  canvas.set_texture_filter(
      godot::CanvasItem::TEXTURE_FILTER_NEAREST_WITH_MIPMAPS);
  paint_structure(canvas, art, content);
  const auto paint_buildings = [&](bool front) {
    paint_kind(ObjectKind::Building, [&](const ObjectSnapshot &object) {
      paint_building(canvas, art, content, object, front);
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

std::optional<Image> item_icon(std::string_view name) {
  struct NamedIcon {
    std::string_view name;
    Image image;
  };
  static constexpr std::array ICONS{
      NamedIcon{.name = "Tomato", .image = Image::IngredientTomato},
      NamedIcon{.name = "Fish", .image = Image::IngredientFish},
      NamedIcon{.name = "Kelp", .image = Image::IngredientKelp},
      NamedIcon{.name = "Mushroom", .image = Image::IngredientMushroom},
      NamedIcon{.name = "Pickles", .image = Image::IngredientPickles},
      NamedIcon{.name = "Meat", .image = Image::IngredientMeat},
      NamedIcon{.name = "Truffle", .image = Image::IngredientTruffle},
      NamedIcon{.name = "Magnifier", .image = Image::ToolMagnifier},
      NamedIcon{.name = "Spatula", .image = Image::ToolSpatula},
      NamedIcon{.name = "Wrench", .image = Image::ToolWrench},
      NamedIcon{.name = "Mushroom Soup", .image = Image::DishMushroomSoup},
      NamedIcon{.name = "Fish Stew", .image = Image::DishFishStew},
      NamedIcon{.name = "Kelp Wrap", .image = Image::DishKelpWrap},
      NamedIcon{.name = "Meat Skewer", .image = Image::DishMeatSkewer},
      NamedIcon{.name = "Truffle Roast", .image = Image::DishTruffleRoast},
      NamedIcon{.name = "Pickle Surprise", .image = Image::DishPickleSurprise},
      NamedIcon{.name = "Sea Salad", .image = Image::DishSeaSalad},
      NamedIcon{.name = "Fish Pie", .image = Image::DishFishPie},
      NamedIcon{.name = "Meat Stew", .image = Image::DishMeatStew},
      NamedIcon{.name = "Truffle Soup", .image = Image::DishTruffleSoup},
      NamedIcon{.name = "Surf and Turf", .image = Image::DishSurfAndTurf}};
  const auto *found = std::ranges::find(ICONS, name, &NamedIcon::name);
  if (found == ICONS.end()) {
    return std::nullopt;
  }
  return found->image;
}

void paint_city_labels(godot::Node2D &canvas, const Art &art,
                       const survival::Content &content,
                       const survival::GameView &view,
                       survival::ControlMap control, const Shot &shot,
                       std::span<const godot::Rect2> covered) {
  const auto markers = markers_of(view, shot.out_of_reach);
  const auto labels =
      label_city(art, content, view, control, markers, shot, covered);
  godot::Array pointed;
  for (const auto &pointer : labels.pointers) {
    paint_pointer(canvas, pointer);
    pointed.push_back(from(survival::object_name(pointer.goal)));
  }
  labels.plates.paint(canvas, art);
  godot::Array drawn;
  godot::Array areas;
  for (const auto &marker : markers) {
    drawn.push_back(from(survival::object_name(marker.object.id)));
    areas.push_back(to_screen(shot.camera, area_rect(marker.object.area)));
  }
  canvas.set_meta("map_plates", labels.plates.boxes());
  canvas.set_meta("map_words", labels.plates.words());
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
               .tint = palette().gold,
               .mark = {}});
}

} // namespace neocity::rendering
