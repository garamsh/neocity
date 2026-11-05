#pragma once

#include <survival/companion.hpp>
#include <survival/exploration.hpp>
#include <survival/simulation.hpp>

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <map>
#include <optional>
#include <vector>

namespace neocity::rendering {

godot::Color color(std::uint8_t r, std::uint8_t g, std::uint8_t b, float a);
godot::Rect2 rect(float x, float y, float w, float h);
/// Fills `area` on `canvas` with `fill`, its corners rounded by `radius`.
void rounded(godot::Node2D &canvas, godot::Rect2 area, float radius,
             godot::Color fill);
/// Draws an opaque pixel panel filling `area`: a dark fill inside a thick
/// original mockup rim, repeating native edge pixels between cut corners.
void panel(godot::Node2D &canvas, godot::Rect2 area);
/// Draws a pixel button filling `area` in a panel's frame, filled in the
/// accent rule inside its original mockup rim when `marked`.
void button(godot::Node2D &canvas, godot::Rect2 area, bool marked);
/// Draws a dark slot sunk into a panel, filling `area`.
void inset(godot::Node2D &canvas, godot::Rect2 area);
/// The one name the screen gives a city object, so the map, the goal line,
/// the action button and the log agree: a source by what it holds and what it
/// is, "Fish cart", anything else by `survival::object_name`.
[[nodiscard]] godot::String place_name(const survival::GameView &view,
                                       survival::ObjectId object);

/// The colour tokens of the world's labels and the interface.
struct Palette {
  /// Primary text.
  godot::Color white;
  /// Secondary text.
  godot::Color muted;
  /// Information: a human's controller, a place, a ready dish.
  godot::Color cyan;
  /// The accent: titles' marks, a marked button, a promise of stock.
  godot::Color gold;
  /// Words on the accent.
  godot::Color ink;
  /// The veil behind an overlay.
  godot::Color background;
  /// A panel.
  godot::Color surface;
  /// A row, slot, chip or idle button on a panel.
  godot::Color raised;
  /// The dark lines around a frame and a slot.
  godot::Color outline;
  /// The light rim of a panel or an idle button.
  godot::Color rim;
  /// The least important text.
  godot::Color faint;
  godot::Color danger;
  godot::Color success;
};

/// How far a panel's words stay inside its edge.
inline constexpr float PANEL_INSET = 16.0F;

/// How words are drawn in the interface's one face: `Bold` draws them again
/// one pixel to the right.
enum class Weight : std::uint8_t { Regular, Bold };

/// The interface face's pixels per em when one of its design pixels is one
/// screen pixel; words are drawn at whole multiples of it.
inline constexpr int FONT_GRID = 8;

Palette palette();

/// The origin and the zoom of a look.
struct Look {
  godot::Vector2 origin;
  float zoom = 1.0F;
};

/// One picture of the monster-mouth escape art set.
enum class Image : std::uint8_t {
  Throat,
  City,
  Acid,
  Kitchen,
  ChefIdle,
  ChefWalk1,
  ChefWalk2,
  ChefWalk3,
  ChefWalk4,
  ChefRun1,
  ChefRun2,
  ChefJump,
  ChefFall,
  ChefLand,
  ChefClimb,
  AppraiserIdle,
  AppraiserWalk1,
  AppraiserWalk2,
  AppraiserWalk3,
  AppraiserWalk4,
  AppraiserRun1,
  AppraiserRun2,
  AppraiserJump,
  AppraiserFall,
  AppraiserLand,
  AppraiserClimb,
  Deck,
  Structure,
  Rubble,
  BuildingWall,
  Shop,
  VendingMachine,
  VendorCart,
  NoodleVendor,
  BagWashedUp,
  Ladder,
  LadderBroken,
  DoorColdStore,
  DoorBroken,
  AcidDrip,
  Kiosk,
  ColdStore,
  Ferry,
  HeartFull,
  HeartEmpty,
  IconDay,
  IconClock,
  IngredientTomato,
  IngredientFish,
  IngredientKelp,
  IngredientMushroom,
  IngredientPickles,
  IngredientMeat,
  IngredientTruffle,
  ToolMagnifier,
  ToolSpatula,
  ToolWrench,
  DishMushroomSoup,
  DishFishStew,
  DishKelpWrap,
  DishMeatSkewer,
  DishTruffleRoast,
  DishPickleSurprise,
  DishSeaSalad,
  DishFishPie,
  DishMeatStew,
  DishTruffleSoup,
  DishSurfAndTurf,
  TitleKeyArt,
  TitleLogo
};

/// How an actor has moved, as the screen last saw it.
struct Motion {
  /// Sideways speed of the last move, in sub-cells per physics step.
  float speed = 0.0F;
  /// World time of the last sideways move; negative before the first.
  std::chrono::microseconds moved_at{-1};
  /// World time the actor last came down from the air; negative before it
  /// has.
  std::chrono::microseconds landed_at{-1};
};

/// Where the last backdrop drawn put the acid's surfaces and the scene's
/// pixels on the screen, for the pass that finishes the scene.
struct Waterline {
  /// The screen row of the acid band's surface, and the screen column its
  /// art's pixel grid starts from.
  float band = 0.0F;
  float band_left = 0.0F;
  /// The screen row of the surface of the sea in the scene.
  float sea = 0.0F;
  /// The screen position of the scene's origin.
  godot::Vector2 origin;
  /// How many screen pixels one scene pixel spans.
  float zoom = 1.0F;
};

/// Draws text and the monster-mouth escape art onto a Godot canvas, and
/// remembers how each actor moved between frames.
class Art {
public:
  /// Loads every picture of the art set and the interface font.
  static Art load();

  /// Draws `text` with the left end of its baseline on the whole pixel
  /// nearest (`x`, `y`), at the whole multiple of `FONT_GRID` nearest `size`.
  void text(godot::Node2D &canvas, float x, float y, const godot::String &text,
            int size, godot::Color tint, Weight weight = Weight::Regular) const;
  /// How wide `text` draws at `size` in `weight`.
  [[nodiscard]] float text_width(const godot::String &text, int size,
                                 Weight weight = Weight::Regular) const;
  /// The lines drawn since the last call, in drawing order; words drawn on a
  /// line's baseline where it ends continue that line.
  [[nodiscard]] std::vector<godot::String> take_written() const;
  /// The sizes `text` actually drew at, one per call since the last call, in
  /// drawing order: each is the grid size, not the one asked for.
  [[nodiscard]] std::vector<int> take_sizes() const;
  /// Draws `image` at its own size times a whole `scale`, its top-left corner
  /// on the pixel at `position`; `mirrored` flips it horizontally in place.
  /// Each colour is multiplied by `tint`. A canvas with `character_layers`
  /// receives the drawn character texture, screen area and reflection there.
  void image(godot::Node2D &canvas, Image image, godot::Vector2 position,
             float scale = 1.0F, bool mirrored = false,
             godot::Color tint = godot::Color(1.0F, 1.0F, 1.0F)) const;
  /// Draws the `source` part of `image`, in the units `size` gives, into
  /// `destination`, each colour multiplied by `tint`; a negative destination
  /// width or height reflects that axis about its position.
  void image_part(godot::Node2D &canvas, Image image, godot::Rect2 source,
                  godot::Rect2 destination,
                  godot::Color tint = godot::Color(1.0F, 1.0F, 1.0F)) const;
  /// Moves everything drawn on `canvas` after it by `offset`, until the next
  /// shift or look; a mirrored image keeps it.
  void shift(godot::Node2D &canvas, godot::Vector2 offset) const;
  /// Draws everything after it on `canvas` `zoom` times its size with its
  /// origin at `origin`, moved by the last shift's offset, until the next
  /// shift or look; a mirrored image keeps it.
  void look(godot::Node2D &canvas, godot::Vector2 origin, float zoom) const;
  /// Where the last look put the scene's origin, without the last shift's
  /// offset, and its zoom, so drawing can return to it with `look`.
  [[nodiscard]] Look looking() const;
  /// Eases from the value `name` last showed toward `target` whenever the
  /// target changes, on the clock `seconds` reads; a value missing from the
  /// frame before starts at its target.
  [[nodiscard]] float glide(const godot::String &name, float target) const;
  /// How far, from 0 to 1 and eased, `name` has come in since it appeared;
  /// one missing from the frame before appears afresh.
  [[nodiscard]] float entrance(const godot::String &name) const;
  /// How many seconds of the clock `seconds` reads `name` has been on the
  /// screen; one missing from the frame before appears afresh.
  [[nodiscard]] float shown_for(const godot::String &name) const;
  /// How large `image` is drawn in the scene: a picture cut from the mockup
  /// for the map's cells holds two of its pixels to a scene pixel each way.
  [[nodiscard]] godot::Vector2 size(Image image) const;
  /// Seconds of a steady real clock since the art loaded, or where `pin`
  /// holds it; the scene's ambient motion, its finishing pass and every eased
  /// value run on it and move nothing in the rules.
  [[nodiscard]] float seconds() const;
  /// Holds `seconds()` at `held` from then on, for repeatable host checks.
  void pin(float held);
  /// Starts the next frame, before anything in it is eased or drawn.
  void next_frame();
  /// Notes the acid band's surface at row `band` with its art's grid from
  /// column `band_left`, both drawn through the last shift, and the sea's at
  /// scene row `sea`, drawn through the last look.
  void note_waterline(float band, float band_left, float sea) const;
  /// The waterline noted since the last call, if any.
  [[nodiscard]] std::optional<Waterline> take_waterline() const;
  /// Records `actor`'s `body` at `world_time` and returns how it has moved.
  /// An earlier world time than the last one starts the memory afresh.
  Motion track(survival::Actor actor, const survival::Body &body,
               std::chrono::microseconds world_time) const;

private:
  /// What `track` keeps for one actor.
  struct Trail {
    Motion motion;
    /// The last sideways position and the world time it was reached.
    std::int32_t x = 0;
    std::chrono::microseconds x_at{};
    survival::Stance stance = survival::Stance::Standing;
    std::chrono::microseconds seen_at{-1};
  };

  /// What `glide` and `entrance` keep for one value: times in microseconds of
  /// `clock`, and the frame that last showed it.
  struct Tween {
    float from = 0.0F;
    float to = 0.0F;
    std::uint64_t started = 0;
    std::uint64_t seen = 0;
    std::uint64_t frame = 0;
  };

  /// One line `text` has drawn, and where its baseline ends.
  struct Written {
    godot::String text;
    godot::Vector2 end;
  };

  Art(std::vector<godot::Ref<godot::Texture2D>> pictures,
      godot::Ref<godot::Font> face);

  /// The tween of `name`, restarted when it was not drawn for a moment.
  Tween &tween(const godot::String &name, float target) const;
  /// `seconds()` in microseconds.
  [[nodiscard]] std::uint64_t clock() const;

  std::vector<godot::Ref<godot::Texture2D>> images;
  godot::Ref<godot::Font> font;
  /// Painting is const; the trails, tweens and written lines are what the
  /// screen saw, not art.
  mutable std::array<Trail, 2> trails{};
  mutable std::map<godot::String, Tween> tweens;
  mutable std::vector<Written> written;
  mutable std::vector<int> sizes;
  mutable std::optional<Waterline> waterline;
  /// The steady clock's reading when the art loaded.
  std::uint64_t loaded;
  std::optional<float> pinned;
  /// How many frames `next_frame` has started.
  std::uint64_t frame = 0;
  /// The offset of the last `shift`, and the canvas transform of the last
  /// `shift` or `look`.
  mutable godot::Vector2 shaken;
  mutable godot::Transform2D placed;
};

} // namespace neocity::rendering
