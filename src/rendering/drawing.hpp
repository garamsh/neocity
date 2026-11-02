#pragma once

#include <survival/exploration.hpp>
#include <survival/simulation.hpp>

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <map>
#include <vector>

namespace neocity::rendering {

godot::Color color(std::uint8_t r, std::uint8_t g, std::uint8_t b, float a);
godot::Rect2 rect(float x, float y, float w, float h);
/// Fills `area` on `canvas` with `fill`, its corners rounded by `radius`.
void rounded(godot::Node2D &canvas, godot::Rect2 area, float radius,
             godot::Color fill);
/// Draws an opaque flat panel filling `area`: rounded, with a soft shadow and
/// a one-pixel edge.
void panel(godot::Node2D &canvas, godot::Rect2 area);
/// Draws a flat button filling `area`, in the accent when `marked`.
void button(godot::Node2D &canvas, godot::Rect2 area, bool marked);

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
  /// The one-pixel edge of a panel or an idle button.
  godot::Color outline;
  /// The soft shadow under a panel.
  godot::Color shadow;
  /// The least important text.
  godot::Color faint;
  godot::Color danger;
  godot::Color success;
};

/// How far a panel's words stay inside its edge.
inline constexpr float PANEL_INSET = 16.0F;

/// The interface font's weights, lightest first.
enum class Weight : std::uint8_t { Medium, SemiBold, Bold };

Palette palette();

/// One picture of the monster-mouth escape art set.
enum class Image : std::uint8_t {
  Throat,
  City,
  Acid,
  Kitchen,
  ChefIdle,
  ChefWalk1,
  ChefWalk2,
  ChefClimb,
  AppraiserIdle,
  AppraiserWalk1,
  AppraiserWalk2,
  AppraiserClimb,
  Road,
  CrackedRoad,
  Pothole,
  RoadEndLeft,
  RoadEndRight,
  TiltedSlab,
  Rubble,
  PlankBridge,
  BuriedCar,
  BuildingWall,
  Shop,
  VendingMachine,
  VendorCart,
  NoodleVendor,
  BagRoof,
  BagWashedUp,
  Ladder,
  LadderBroken,
  DoorColdStore,
  DoorBroken,
  AcidDrip,
  Kiosk,
  ColdStore,
  Ferry,
  Platform,
  Bridge,
  TrainCarLit,
  TrainCarWrecked,
  FacadeTall,
  FacadeLow,
  CablesSagging,
  CablesHanging,
  PipeBent,
  PipesValve,
  NeonBox,
  NeonBlade,
  FleshVines,
  FleshTendril,
  FleshTumor,
  FleshPolyps,
  RubbleLow,
  RubbleHigh,
  PillarSnapped,
  PillarDeck,
  LampStanding,
  LampBent,
  HeartFull,
  HeartEmpty,
  IconDay,
  IconAcid,
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
  survival::GridPosition cell;
  /// World time of the last change of cell.
  std::chrono::microseconds moved_at{};
  bool facing_left = false;
  /// Flips with every change of cell, so the walk alternates its frames.
  bool second_step = false;
};

/// Draws text and the monster-mouth escape art onto a Godot canvas, and
/// remembers how each actor moved between frames.
class Art {
public:
  /// Loads every picture of the art set and the interface font's weights.
  static Art load();

  void text(godot::Node2D &canvas, float x, float y, const godot::String &text,
            int size, godot::Color tint, Weight weight = Weight::Medium) const;
  /// How wide `text` draws at `size` in `weight`.
  [[nodiscard]] float text_width(const godot::String &text, int size,
                                 Weight weight = Weight::Medium) const;
  /// Draws `image` at its own size times a whole `scale`, its top-left corner
  /// on the pixel at `position`; `mirrored` flips it horizontally in place.
  /// Each colour is multiplied by `tint`.
  void image(godot::Node2D &canvas, Image image, godot::Vector2 position,
             float scale = 1.0F, bool mirrored = false,
             godot::Color tint = godot::Color(1.0F, 1.0F, 1.0F)) const;
  /// Draws the `source` part of `image` into `destination`, each colour
  /// multiplied by `tint`.
  void image_part(godot::Node2D &canvas, Image image, godot::Rect2 source,
                  godot::Rect2 destination,
                  godot::Color tint = godot::Color(1.0F, 1.0F, 1.0F)) const;
  /// Eases from the value `name` last showed toward `target` whenever the
  /// target changes; a value missing from the last drawn frame starts at its
  /// target.
  [[nodiscard]] float glide(const godot::String &name, float target) const;
  /// How far, from 0 to 1 and eased, `name` has come in since it appeared;
  /// one missing from the last drawn frame appears afresh.
  [[nodiscard]] float entrance(const godot::String &name) const;
  [[nodiscard]] godot::Vector2 size(Image image) const;
  /// Records `actor` at `cell` at `world_time` and returns how it has moved.
  /// An earlier world time than the last one starts the memory afresh.
  Motion track(survival::Actor actor, survival::GridPosition cell,
               std::chrono::microseconds world_time) const;

private:
  /// What `track` keeps for one actor.
  struct Trail {
    Motion motion;
    std::chrono::microseconds seen_at{-1};
  };

  /// What `glide` and `entrance` keep for one value: times in microseconds of
  /// the real clock, and the drawn frame that last showed it.
  struct Tween {
    float from = 0.0F;
    float to = 0.0F;
    std::uint64_t started = 0;
    std::uint64_t seen = 0;
    std::uint64_t frame = 0;
  };

  Art(std::vector<godot::Ref<godot::Texture2D>> pictures,
      std::array<godot::Ref<godot::Font>, 3> weights);

  /// The tween of `name`, restarted when it was not drawn for a moment.
  Tween &tween(const godot::String &name, float target) const;

  std::vector<godot::Ref<godot::Texture2D>> images;
  std::array<godot::Ref<godot::Font>, 3> fonts;
  /// Painting is const; the trails and tweens are what the screen saw, not
  /// art.
  mutable std::array<Trail, 2> trails{};
  mutable std::map<godot::String, Tween> tweens;
};

} // namespace neocity::rendering
