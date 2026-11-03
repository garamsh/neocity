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
/// The throat's top edge on the screen, raised so its teeth and uvula hang
/// over the distant city; and the city's bottom edge, under the acid band at
/// acid 0, so its towers stand clear below the teeth.
constexpr float THROAT_TOP = -200.0F;
constexpr float CITY_BOTTOM = 700.0F;
/// The monster's upper teeth hang from the screen's top edge, behind the top
/// bar, and slide sideways this many times as far as the map when the camera
/// moves.
constexpr float JAW_TOP = 0.0F;
constexpr float JAW_DEPTH = 1.25F;
/// The teeth picture's pixels, drawn on the scene's pixel grid at twice its
/// size, so the finishing pass reads them whole.
constexpr float JAW_PIXEL = 2.0F;
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
/// A support stands down to the walkway below or into the acid only where
/// that is at most this far under the deck, the acid read at its surface at
/// acid 0; a higher walkway stands on short braces this long under the deck,
/// ending on a girder of this many of the deck picture's bottom rows that
/// runs along the walkway between them.
constexpr float SUPPORT_REACH = 4.5F * CELL;
constexpr float BRACE = CELL;
constexpr float GIRDER = 8.0F;
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
constexpr float BAND_LOW = 70.0F;
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
/// The acid band's art is drawn at its own size, four screen pixels to each
/// of its pixels.
constexpr float BAND_PIXEL = 4.0F;

/// The throat breathes over this many seconds, lifting this many pixels and
/// brightening by this share as it draws in.
constexpr float BREATH_SECONDS = 5.0F;
constexpr float BREATH_LIFT = 4.0F;
constexpr float BREATH_GLOW = 0.08F;
/// Where saliva gathers at the end of the strands under the fangs, in the
/// throat picture's pixels, and the size of that picture's pixels.
constexpr std::array SALIVA_TIPS{
    Vector2(4.0F, 428.0F),    Vector2(228.0F, 424.0F), Vector2(252.0F, 384.0F),
    Vector2(528.0F, 416.0F),  Vector2(752.0F, 408.0F), Vector2(1028.0F, 372.0F),
    Vector2(1052.0F, 436.0F), Vector2(1240.0F, 384.0F)};
constexpr float THROAT_PIXEL = 4.0F;
/// A drop swells at its tip for the first part of its cycle, then falls
/// with this pull, in pixels per second squared, until it has fallen its
/// reach.
constexpr float DROP_SWELL = 0.55F;
constexpr float DROP_PULL = 700.0F;
/// A drop under the flesh on a walkway hangs from this far below the
/// flesh picture's top, and falls this far at most.
constexpr float FLESH_DROP_TIP = 24.0F;
constexpr float FLESH_FALL = 160.0F;
/// Spores drifting through the air, far behind the map and near in front of
/// it: how many in each, how fast they rise in pixels per second, and how
/// far the camera moves them, as for the backdrop's layers.
constexpr int SPORES = 28;
constexpr float SPORE_RISE = 7.0F;
constexpr float FAR_SPORE_DEPTH = 0.25F;
constexpr float NEAR_SPORE_DEPTH = 0.8F;
/// Steam over an acid surface: a puff every this many pixels along it, each
/// rising this many of its pixels over about this many seconds.
constexpr float STEAM_SPACING = 36.0F;
constexpr float STEAM_RISE = 40.0F;
constexpr float STEAM_SECONDS = 3.4F;
/// Seeds of the ambient motion's scatter, one per kind of thing moving.
constexpr std::uint32_t SALIVA_SEED = 0x5A11U;
constexpr std::uint32_t FLESH_SEED = 0xF1E5U;
constexpr std::uint32_t FAR_SPORE_SEED = 0x5B0EU;
constexpr std::uint32_t NEAR_SPORE_SEED = 0x5B1EU;
constexpr std::uint32_t STEAM_SEED = 0x57EAU;

/// The supports under the walkways, and the footing under a shop, are dark
/// foreground in front of the bright backdrop.
godot::Color support_tint() { return color(110, 100, 140, 1.0F); }
/// A support the acid has risen around is stained toward its green.
godot::Color corroded_support_tint() { return color(90, 125, 85, 1.0F); }
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
/// The ground is dark foreground, a cooler shade of its own colours, under a
/// walked-on top edge lit warm and bright.
godot::Color ground_tint() { return {0.8F, 0.8F, 1.0F}; }
godot::Color ground_edge_tint() { return {2.4F, 2.0F, 1.6F}; }
/// The throat keeps its saturated magenta while its near-white teeth and
/// uvula are pulled toward it, so no part of the backdrop outshines the
/// walkways' lit edge.
godot::Color throat_tint() { return {1.0F, 0.8F, 0.95F}; }

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

/// Draws a walkway's picture dark, in front of the bright backdrop, under a
/// top edge lit brightest from the picture's rows below `outline`, so the
/// surface a player walks on reads first.
void paint_surface(godot::Node2D &canvas, const Art &art, Image image,
                   Vector2 at, float outline) {
  art.image(canvas, image, at, 1.0F, false, ground_tint());
  art.image_part(canvas, image, rect(0.0F, outline, CELL, GROUND_EDGE),
                 rect(at.x, at.y, CELL, GROUND_EDGE), ground_edge_tint());
}

/// A walkway deck with its top at `at`, eaten by the acid when `corroded`; a
/// snapped end keeps its torn shape.
void paint_deck(godot::Node2D &canvas, const Art &art, Image deck, Vector2 at,
                bool corroded) {
  const bool end = deck == Image::EndLeft || deck == Image::EndRight;
  paint_surface(canvas, art, corroded && !end ? Image::DeckCorroded : deck, at,
                OUTLINE);
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
    art.image_part(canvas, deck, rect(source, OUTLINE, 1.0F, GROUND_EDGE),
                   rect(x, y, 1.0F, GROUND_EDGE), ground_edge_tint());
  }
}

/// Stairs cut from a walkway deck, one cell wide, falling to the right from
/// the floor line at `at` to the row below in steps of STAIR_STEP. Each step
/// is a tread of the deck's warm lit rows, stretched down to the diagonal the
/// stairs span, where a player walks on them, over the deck's face below that
/// in its own colours, a stringer facing away from the light.
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
    art.image_part(canvas, deck, rect(source, OUTLINE, 1.0F, STAIR_TREAD),
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
      paint_surface(canvas, art, Image::Platform, at, 0.0F);
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

/// A support under column `x` from `top`: down to the next walkway below or
/// out of the map, into the acid, where that is within SUPPORT_REACH, and
/// otherwise a brace, whose foot it returns for the girder it stands on.
std::optional<godot::Rect2> paint_leg(godot::Node2D &canvas, const Art &art,
                                      const std::vector<Standing> &ground,
                                      Image image, std::int32_t x, float top,
                                      float map_bottom, float surface) {
  const float bottom = support_bottom(ground, x, top, map_bottom);
  if (std::min(bottom, ACID_LOW) - top <= SUPPORT_REACH) {
    paint_support(canvas, art, image, x, top, bottom, surface);
    return std::nullopt;
  }
  paint_support(canvas, art, image, x, top, top + BRACE, surface);
  return rect(cell_x(x), top + BRACE, CELL, GIRDER);
}

/// A girder of the deck picture's bottom rows along `span`, the braces' feet
/// from the first to the last.
void paint_girder(godot::Node2D &canvas, const Art &art,
                  const std::optional<godot::Rect2> &span) {
  if (!span) {
    return;
  }
  const Vector2 deck = art.size(Image::Deck);
  const auto cells = static_cast<int>(std::round(span->size.x / CELL));
  for (int cell = 0; cell < cells; ++cell) {
    art.image_part(canvas, Image::Deck,
                   rect(0.0F, deck.y - GIRDER, deck.x, GIRDER),
                   rect(span->position.x + static_cast<float>(cell) * CELL,
                        span->position.y, deck.x, GIRDER),
                   support_tint());
  }
}

/// Grows `span` to take in `brace`, when there is one.
void join(std::optional<godot::Rect2> &span,
          const std::optional<godot::Rect2> &brace) {
  if (brace) {
    span = span ? span->merge(*brace) : *brace;
  }
}

/// The supports under every walkway deck and the pier, but the planks, the
/// tilted ground and the other landmarks: a support under each end of a run,
/// down to the next walkway below or out of the map, into the acid, where
/// that is within SUPPORT_REACH; a run too high for that stands on a brace
/// under each end, on a girder between them. Each run takes the next support
/// picture in turn.
void paint_supports(godot::Node2D &canvas, const Art &art,
                    const std::vector<Standing> &ground, float map_bottom,
                    float surface) {
  static constexpr std::array SUPPORTS{Image::Pillar, Image::Column};
  const auto walkway = [&](const Standing &cell) {
    const auto kind = cell.ground.kind;
    return cell.picture ? cell.picture == LandmarkKind::Platform
                        : kind != GroundKind::PlankBridge &&
                              kind != GroundKind::TiltedSlab;
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
  // The span of the braces under the run being drawn.
  std::optional<godot::Rect2> girder;
  std::size_t run = 0;
  for (std::size_t index = 0; index < ground.size(); ++index) {
    const auto &cell = ground.at(index);
    if (!walkway(cell)) {
      continue;
    }
    if (!beside(index, -1, walkway)) {
      paint_girder(canvas, art, girder);
      girder.reset();
      ++run;
    }
    const auto [x, y] = cell.ground.cell;
    const bool ends = !beside(index, -1, solid) || !beside(index, 1, solid);
    if (!solid(cell) || !ends) {
      continue;
    }
    const float top =
        floor_y(y) + art.size(cell.picture ? Image::Platform : Image::Deck).y;
    join(girder,
         paint_leg(canvas, art, ground, SUPPORTS.at(run % SUPPORTS.size()), x,
                   top, map_bottom, surface));
  }
  paint_girder(canvas, art, girder);
}

/// Whether flesh has grown over the front edge of a cell of walkway deck.
bool overgrown(const survival::GroundCell &ground) {
  const bool plain = ground.kind == GroundKind::Road ||
                     ground.kind == GroundKind::CrackedRoad ||
                     ground.kind == GroundKind::Pothole;
  return plain && (ground.cell.x * 5 + ground.cell.y * 3) % 7 == 3;
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

/// `value` down to the grid of `pixel`.
float snapped(float value, float pixel) {
  return std::floor(value / pixel) * pixel;
}

/// What a drop drips from and how: a cycle of `period` seconds on the grid
/// of `pixel`, as wide as one pixel and `length` long once it lets go, in
/// `tint`, falling `reach` at most.
struct Dripping {
  float period = 1.0F;
  float pixel = 1.0F;
  float length = 1.0F;
  float reach = 0.0F;
  godot::Color tint;
};

/// A drop hanging from `tip`, on the grid of its pixel: it swells there for
/// the first DROP_SWELL of its `run` from 0 to 1, then falls until it has
/// fallen its reach, after which it is not drawn.
void paint_drop(godot::Node2D &canvas, Vector2 tip, float run,
                const Dripping &drop) {
  if (run < DROP_SWELL) {
    const float swell = snapped(drop.length * run / DROP_SWELL, drop.pixel);
    canvas.draw_rect(
        rect(tip.x, tip.y, drop.pixel, std::max(drop.pixel, swell)), drop.tint);
    return;
  }
  const float falling = (run - DROP_SWELL) * drop.period;
  const float fallen =
      snapped(DROP_PULL * falling * falling / 2.0F, drop.pixel);
  if (fallen + drop.length > drop.reach) {
    return;
  }
  canvas.draw_rect(rect(tip.x, tip.y + fallen, drop.pixel, drop.length),
                   drop.tint);
}

/// The throat's breath now, from 0 drawn out to 1 drawn in.
float breath(const Art &art) {
  return 0.5F - 0.5F * std::cos(art.seconds() * 2.0F *
                                std::numbers::pi_v<float> / BREATH_SECONDS);
}

/// Spores rising through the air over the whole screen, moved by `camera`
/// as a layer `depth` of the way to the map is: squares of `size` screen
/// pixels on their own grid, in the first of `tints` and every other one in
/// the second, each twinkling.
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
    canvas.draw_rect(rect(snapped(x, size), snapped(y, size), size, size),
                     tint);
  }
}

/// Steam rising off an acid surface at row `surface`, across the screen's
/// width from 0, in puffs on the grid of `pixel` that swell and fade as they
/// rise.
void paint_steam(godot::Node2D &canvas, const Art &art, float surface,
                 float pixel, godot::Color tint) {
  const float seconds = art.seconds();
  const auto puffs = static_cast<int>(SCREEN_WIDTH / STEAM_SPACING);
  for (int puff = 0; puff <= puffs; ++puff) {
    const float run =
        cycle(seconds, STEAM_SECONDS * (0.8F + 0.5F * unit(STEAM_SEED, puff)),
              unit(STEAM_SEED + 1U, puff));
    const float size = std::floor(2.0F + 4.0F * run) * pixel;
    const float x =
        (static_cast<float>(puff) + unit(STEAM_SEED + 2U, puff)) *
            STEAM_SPACING +
        3.0F * pixel * std::sin(run * 4.0F + static_cast<float>(puff));
    const float y = surface - run * STEAM_RISE * pixel;
    godot::Color fading = tint;
    fading.a *= std::sin(run * std::numbers::pi_v<float>);
    canvas.draw_rect(rect(snapped(x - size / 2.0F, pixel),
                          snapped(y - size, pixel), size, size),
                     fading);
  }
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
  const float seconds = art.seconds();
  for (const auto &cell : ground) {
    if (!cell.picture && overgrown(cell.ground)) {
      const Vector2 at(cell_x(cell.ground.cell.x) + (CELL - drip) / 2.0F,
                       floor_y(cell.ground.cell.y) + GROUND_EDGE);
      art.image(canvas, Image::FleshDrip, at);
      const auto index = static_cast<std::int64_t>(cell.ground.cell.x) * 64 +
                         cell.ground.cell.y;
      const Vector2 tip = at + Vector2(std::floor(drip / 2.0F), FLESH_DROP_TIP);
      const Dripping drop{.period = 1.8F + 2.2F * unit(FLESH_SEED, index),
                          .pixel = 1.0F,
                          .length = 3.0F,
                          .reach = std::min(FLESH_FALL, surface - tip.y),
                          .tint = color(214, 72, 150, 1.0F)};
      paint_drop(canvas, tip,
                 cycle(seconds, drop.period, unit(FLESH_SEED + 1U, index)),
                 drop);
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
/// otherwise.
void paint_decorations(godot::Node2D &canvas, const Art &art,
                       const survival::Content &content, bool back) {
  for (const auto &piece : decoration_pieces(art, content, back)) {
    art.image(canvas, piece.image, piece.at);
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

/// A lit shop in a building under a walkway: its roof under that walkway's
/// deck, its floor on a footing of deck across the building's width, which
/// stands on a support under each end as a walkway does.
void paint_shop_room(godot::Node2D &canvas, const Art &art,
                     const survival::WorldArea &area,
                     const std::vector<Standing> &ground, float map_bottom,
                     float surface) {
  const auto box = area_rect(area);
  const Vector2 shop = art.size(Image::Shop);
  const Vector2 deck = art.size(Image::Deck);
  const float roof = box.position.y + deck.y;
  art.image(canvas, Image::Shop,
            Vector2(box.get_center().x - shop.x / 2.0F, roof));
  const float floor = roof + shop.y;
  for (std::int32_t x = area.x; x < area.x + area.width; ++x) {
    art.image(canvas, Image::Deck, Vector2(cell_x(x), floor), 1.0F, false,
              support_tint());
  }
  std::optional<godot::Rect2> girder;
  for (const std::int32_t x : {area.x, area.x + area.width - 1}) {
    join(girder, paint_leg(canvas, art, ground, Image::Pillar, x,
                           floor + deck.y, map_bottom, surface));
  }
  paint_girder(canvas, art, girder);
}

/// A building is a wall of windows over its area, a lit shop room where a
/// shop is inside it, its landmark picture standing in it, or rubble once
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
    paint_shop_room(canvas, art, area, ground, cell_y(content.map.height),
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

/// A lit shop is taller than three cells, so it stands behind the
/// ground. A shop inside a building is drawn with that building, or its
/// rubble.
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

/// Saliva swelling at the strands under the throat's fangs and falling, on
/// the throat drawn at `at` beside its mirrored copies as `paint_layer` draws
/// it.
void paint_saliva(godot::Node2D &canvas, const Art &art, Vector2 at) {
  const float width = art.size(Image::Throat).x;
  const float seconds = art.seconds();
  for (std::size_t tip = 0; tip < SALIVA_TIPS.size(); ++tip) {
    const auto index = static_cast<std::int64_t>(tip);
    const Dripping drop{.period = 2.4F + 2.0F * unit(SALIVA_SEED, index),
                        .pixel = THROAT_PIXEL,
                        .length = 2.0F * THROAT_PIXEL,
                        .reach = SCREEN_HEIGHT,
                        .tint = color(250, 214, 226, 0.9F)};
    const float run =
        cycle(seconds, drop.period, unit(SALIVA_SEED + 1U, index));
    const Vector2 local = SALIVA_TIPS.at(tip);
    for (int copy = -1; copy <= 1; ++copy) {
      // A mirrored copy flips each pixel column in place.
      const float x = copy == 0 ? local.x : width - local.x - THROAT_PIXEL;
      paint_drop(canvas,
                 at + Vector2(x + static_cast<float>(copy) * width, local.y),
                 run, drop);
    }
  }
}

/// The monster's upper teeth along the top of the screen whatever the camera
/// shows, breathing with the throat and sliding sideways with the map, beside
/// mirrored copies of themselves that meet without a seam, the copies at odd
/// steps of its width from the one it leaves on the canvas as the `teeth_at`
/// meta mirrored. The canvas goes back to the scene afterwards.
void paint_jaw(godot::Node2D &canvas, const Art &art) {
  const Look scene = art.looking();
  const float width = art.size(Image::Jaw).x;
  const auto on_grid = [](float value, float grid) {
    return grid + snapped(value - grid, JAW_PIXEL);
  };
  const float slide = on_grid(wrapped(scene.origin.x * JAW_DEPTH, 2.0F * width),
                              scene.origin.x);
  const float top =
      on_grid(JAW_TOP - breath(art) * BREATH_LIFT, scene.origin.y);
  canvas.set_meta("teeth_at", Vector2(slide, top));
  art.look(canvas, Vector2(), 1.0F);
  for (int copy = -2; copy <= 2; ++copy) {
    art.image(canvas, Image::Jaw,
              Vector2(slide + static_cast<float>(copy) * width, top), 1.0F,
              copy % 2 != 0);
  }
  art.look(canvas, scene.origin, scene.zoom);
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
                -acid.x, band_deep_tint());
    } else {
      paint_sea(canvas, art, left, surface - ACID_SURFACE_DEPTH, acid.x,
                band_deep_tint());
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
  const float drawn_in = breath(art);
  // The throat breathes: it lifts and brightens as it draws in.
  const Vector2 breathing(0.0F,
                          THROAT_TOP - std::round(drawn_in * BREATH_LIFT));
  const float flush = 1.0F + BREATH_GLOW * (drawn_in - 0.5F);
  art.look(canvas, Vector2(), 1.0F);
  paint_layer(canvas, art, Image::Throat, breathing, throat,
              throat_tint() * godot::Color(flush, flush, flush));
  paint_saliva(canvas, art, breathing + throat);
  paint_layer(canvas, art, Image::City,
              Vector2(0.0F, CITY_BOTTOM - art.size(Image::City).y), city,
              untinted());
  paint_band(canvas, art, acid_ratio, band);
  godot::Array layers;
  layers.push_back(throat);
  layers.push_back(city);
  layers.push_back(band);
  canvas.set_meta("backdrop_layers", layers);
  art.look(canvas, origin(camera), camera.zoom);
  paint_sea(canvas, art, 0.0F,
            std::floor(acid_surface(acid_ratio) - ACID_SURFACE_DEPTH),
            art.size(Image::Acid).x, sea_deep_tint());
  art.note_waterline(std::floor(band_surface(acid_ratio)),
                     wrapped(std::floor(band.x), BAND_PIXEL),
                     acid_surface(acid_ratio));
}

void paint_glow(godot::Node2D &canvas, const Art &art, float acid_ratio,
                const View &camera) {
  const godot::Color steam = color(150, 255, 140, 0.16F);
  art.look(canvas, Vector2(), 1.0F);
  paint_steam(canvas, art, std::floor(band_surface(acid_ratio)),
              THROAT_PIXEL / 2.0F, steam);
  paint_spores(canvas, art, camera, FAR_SPORE_DEPTH, 2.0F,
               {color(255, 150, 220, 0.5F), color(170, 255, 150, 0.4F)},
               FAR_SPORE_SEED);
  paint_spores(canvas, art, camera, NEAR_SPORE_DEPTH, 4.0F,
               {color(255, 80, 200, 0.45F), color(140, 255, 120, 0.4F)},
               NEAR_SPORE_SEED);
  art.look(canvas, origin(camera), camera.zoom);
  paint_steam(canvas, art, acid_surface(acid_ratio), 1.0F, steam);
}

void paint_city_glow(godot::Node2D &canvas, const Art &art,
                     const survival::Content &content, const View &camera) {
  art.look(canvas, origin(camera), camera.zoom);
  for (const auto &piece : decoration_pieces(art, content, false)) {
    if (piece.image == Image::LampStanding || piece.image == Image::LampBent) {
      halo(canvas, camera,
           piece.at + Vector2(art.size(piece.image).x / 2.0F, LAMP_HEAD),
           LAMP_GLOW, color(255, 170, 80, LAMP_LIGHT));
    }
  }
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
  paint_decorations(canvas, art, content, true);
  paint_supports(canvas, art, ground, cell_y(content.map.height), surface);
  paint_jaw(canvas, art);
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
  paint_decorations(canvas, art, content, false);
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
