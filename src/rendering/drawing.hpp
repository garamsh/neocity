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
#include <vector>

namespace neocity::rendering {

godot::Color color(std::uint8_t r, std::uint8_t g, std::uint8_t b, float a);
godot::Rect2 rect(float x, float y, float w, float h);

/// Text and accent colors shared by the world and the HUD.
struct Palette {
  godot::Color white;
  godot::Color muted;
  godot::Color cyan;
  godot::Color gold;
  /// Words on a highlighted button.
  godot::Color ink;
};

/// How far a panel's words stay inside its edge, clear of its steel frame.
inline constexpr float PANEL_INSET = 20.0F;

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
  PanelFrame,
  BarFrame,
  ItemSlot,
  Button,
  ButtonHighlight,
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
  /// Loads every picture of the art set and Godot's fallback font.
  static Art load();

  void text(godot::Node2D &canvas, float x, float y, const godot::String &text,
            int size, godot::Color tint) const;
  /// How wide `text` draws at `size`.
  [[nodiscard]] float text_width(const godot::String &text, int size) const;
  /// Draws `image` at its own size times a whole `scale`, its top-left corner
  /// on the pixel at `position`; `mirrored` flips it horizontally in place.
  void image(godot::Node2D &canvas, Image image, godot::Vector2 position,
             float scale = 1.0F, bool mirrored = false) const;
  /// Draws the `source` part of `image` into `destination`.
  void image_part(godot::Node2D &canvas, Image image, godot::Rect2 source,
                  godot::Rect2 destination) const;
  /// Draws `image` as a frame filling `destination`: its `border`-pixel
  /// corners at their own size, its edges repeated along each side, and the
  /// middle filled with `fill`, the colour of the picture's face.
  void frame(godot::Node2D &canvas, Image image, godot::Rect2 destination,
             float border, godot::Color fill) const;
  /// Draws the UI kit's opaque steel panel filling `area`.
  void panel(godot::Node2D &canvas, godot::Rect2 area) const;
  /// Draws a UI kit button filling `area`, yellow when `marked`.
  void button(godot::Node2D &canvas, godot::Rect2 area, bool marked) const;
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

  Art(std::vector<godot::Ref<godot::Texture2D>> pictures,
      godot::Ref<godot::Font> fallback_font);

  std::vector<godot::Ref<godot::Texture2D>> images;
  godot::Ref<godot::Font> font;
  /// Painting is const; the trails are what the screen saw, not art.
  mutable std::array<Trail, 2> trails{};
};

} // namespace neocity::rendering
