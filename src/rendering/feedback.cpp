#include "feedback.hpp"

#include "survival.hpp"

#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/string.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <numbers>

namespace neocity::rendering {
namespace {

using godot::Vector2;

constexpr float SCREEN_WIDTH = 1280.0F;
constexpr float SCREEN_HEIGHT = 720.0F;
constexpr float SHAKE_PIXELS = 6.0F;
/// Shakes per second while a burn shakes the world.
constexpr float SHAKE_RATE = 38.0F;
constexpr float HIT_SECONDS = 0.35F;
constexpr float POP_SECONDS = 0.6F;
constexpr float SPLASH_SECONDS = 0.8F;
constexpr float STEAM_SECONDS = 1.6F;
constexpr float REACTION_SECONDS = 1.4F;
constexpr float NEW_DAY_SECONDS = 0.9F;
/// The pot on the kitchen's stove, and the monster's feeding bucket.
constexpr Vector2 POT(700.0F, 236.0F);
constexpr Vector2 BUCKET(1205.0F, 202.0F);
/// Where the rising acid throws up its drops across the screen.
constexpr int SURGE_COLUMNS = 8;
constexpr float SURGE_FIRST = 60.0F;
constexpr float SURGE_GAP = 160.0F;
/// Where the actor's body is drawn above its feet.
constexpr float BODY_RISE = 26.0F;

/// 0 at the start of a flourish and 1 at its end.
float progress(const Burst &burst) {
  return std::clamp(burst.age / lifetime(burst.kind), 0.0F, 1.0F);
}

godot::Color fade(godot::Color tint, float alpha) {
  tint.a = alpha;
  return tint;
}

/// A ring of `count` specks thrown out from `centre` and falling back.
void specks(godot::Node2D &canvas, Vector2 centre, float done, int count,
            float reach, float radius, godot::Color tint) {
  constexpr float TAU = 2.0F * std::numbers::pi_v<float>;
  for (int index = 0; index < count; ++index) {
    const float angle =
        TAU * (static_cast<float>(index) + 0.5F) / static_cast<float>(count);
    const Vector2 out(std::cos(angle), std::sin(angle));
    const Vector2 drop(0.0F, 30.0F * done * done);
    canvas.draw_circle(centre + out * reach * done + drop,
                       radius * (1.0F - done) + 1.0F, fade(tint, 1.0F - done));
  }
}

void paint_pop(godot::Node2D &canvas, Vector2 centre, float done) {
  const auto tones = palette();
  canvas.draw_arc(centre, 6.0F + 28.0F * done, 0.0F,
                  2.0F * std::numbers::pi_v<float>, 24,
                  fade(tones.gold, 1.0F - done), 3.0F);
  specks(canvas, centre, done, 8, 34.0F, 3.0F, tones.white);
}

void paint_spark(godot::Node2D &canvas, Vector2 centre, float done) {
  specks(canvas, centre, done, 10, 30.0F, 2.5F, color(255, 220, 120, 1.0F));
}

void paint_hit(godot::Node2D &canvas, Vector2 centre, float done) {
  canvas.draw_circle(centre, 22.0F * (1.0F - done) + 4.0F,
                     fade(palette().danger, 0.55F * (1.0F - done)));
}

/// Drops of acid thrown up from `surface` at `x` and falling back into it.
void paint_splash(godot::Node2D &canvas, float x, float surface, float done,
                  float height) {
  const auto acid = palette().success;
  constexpr int DROPS = 7;
  for (int index = 0; index < DROPS; ++index) {
    const float spread =
        (static_cast<float>(index) - static_cast<float>(DROPS - 1) / 2.0F) *
        7.0F;
    const float rise =
        height *
        (1.0F - static_cast<float>(std::abs(index - DROPS / 2)) / 5.0F);
    // A drop rises and falls back on a parabola over the flourish.
    const float lift = 4.0F * rise * done * (1.0F - done);
    canvas.draw_circle(Vector2(x + spread * (0.5F + done), surface - lift),
                       3.5F * (1.0F - done) + 1.0F, fade(acid, 1.0F - done));
  }
  canvas.draw_arc(Vector2(x, surface), 8.0F + 30.0F * done, 0.0F,
                  -std::numbers::pi_v<float>, 16,
                  fade(acid, 0.8F * (1.0F - done)), 2.0F);
}

void paint_steam(godot::Node2D &canvas, float done) {
  constexpr int PUFFS = 5;
  for (int index = 0; index < PUFFS; ++index) {
    const float own = std::fmod(done + static_cast<float>(index) * 0.2F, 1.0F);
    const float sway =
        std::sin((own + static_cast<float>(index)) * 6.0F) * 8.0F;
    canvas.draw_circle(
        POT +
            Vector2(sway + static_cast<float>(index - 2) * 9.0F, -own * 90.0F),
        6.0F + 12.0F * own,
        color(240, 240, 248, 0.45F * (1.0F - own) * (1.0F - done)));
  }
}

void paint_reaction(godot::Node2D &canvas, const Art &art, bool pleased,
                    float done) {
  const auto tones = palette();
  const godot::String word = pleased ? "YUM!" : "BLEH!";
  const godot::Color tint = pleased ? tones.success : tones.danger;
  const float rise = 50.0F * done;
  const float alpha = done < 0.7F ? 1.0F : (1.0F - done) / 0.3F;
  const Vector2 at = BUCKET + Vector2(-40.0F, -40.0F - rise);
  const float width = art.text_width(word, 26, Weight::Bold);
  rounded(canvas,
          godot::Rect2(at.x - 10.0F, at.y - 28.0F, width + 20.0F, 38.0F), 8.0F,
          fade(tones.background, 0.85F * alpha));
  art.text(canvas, at.x, at.y, word, 26, fade(tint, alpha), Weight::Bold);
  if (pleased) {
    specks(canvas, BUCKET, done, 8, 40.0F, 3.0F, tones.gold);
  }
}

} // namespace

const char *flourish_name(Flourish kind) {
  switch (kind) {
  case Flourish::Pop:
    return "pop";
  case Flourish::Spark:
    return "spark";
  case Flourish::Hit:
    return "hit";
  case Flourish::Splash:
    return "splash";
  case Flourish::Surge:
    return "surge";
  case Flourish::Steam:
    return "steam";
  case Flourish::Delight:
    return "delight";
  case Flourish::Disgust:
    return "disgust";
  case Flourish::NewDay:
    return "new_day";
  }
  std::abort();
}

float lifetime(Flourish kind) {
  switch (kind) {
  case Flourish::Pop:
  case Flourish::Spark:
    return POP_SECONDS;
  case Flourish::Hit:
    return HIT_SECONDS;
  case Flourish::Splash:
  case Flourish::Surge:
    return SPLASH_SECONDS;
  case Flourish::Steam:
    return STEAM_SECONDS;
  case Flourish::Delight:
  case Flourish::Disgust:
    return REACTION_SECONDS;
  case Flourish::NewDay:
    return NEW_DAY_SECONDS;
  }
  std::abort();
}

Vector2 shake(const std::vector<Burst> &bursts) {
  float strength = 0.0F;
  float age = 0.0F;
  for (const auto &burst : bursts) {
    if (burst.kind == Flourish::Hit && 1.0F - progress(burst) > strength) {
      strength = 1.0F - progress(burst);
      age = burst.age;
    }
  }
  if (strength <= 0.0F) {
    return {};
  }
  const float phase = age * SHAKE_RATE;
  return Vector2(std::sin(phase * 2.0F * std::numbers::pi_v<float>),
                 std::cos(phase * 1.3F * std::numbers::pi_v<float>)) *
         SHAKE_PIXELS * strength;
}

void paint_city_bursts(godot::Node2D &canvas, const std::vector<Burst> &bursts,
                       float acid_ratio) {
  const float surface = acid_surface(acid_ratio);
  for (const auto &burst : bursts) {
    const float done = progress(burst);
    if (burst.kind == Flourish::Surge) {
      for (int column = 0; column < SURGE_COLUMNS; ++column) {
        paint_splash(canvas,
                     SURGE_FIRST + static_cast<float>(column) * SURGE_GAP,
                     surface, done, 28.0F);
      }
      continue;
    }
    if (!burst.at) {
      continue;
    }
    const Vector2 feet = map_point(*burst.at);
    const Vector2 body = feet - Vector2(0.0F, BODY_RISE);
    switch (burst.kind) {
    case Flourish::Pop:
      paint_pop(canvas, body, done);
      break;
    case Flourish::Spark:
      paint_spark(canvas, body, done);
      break;
    case Flourish::Hit:
      paint_hit(canvas, body, done);
      break;
    case Flourish::Splash:
      paint_splash(canvas, feet.x, surface, done, 60.0F);
      break;
    case Flourish::Surge:
    case Flourish::Steam:
    case Flourish::Delight:
    case Flourish::Disgust:
    case Flourish::NewDay:
      break;
    }
  }
}

void paint_workshop_bursts(godot::Node2D &canvas, const Art &art,
                           const std::vector<Burst> &bursts) {
  for (const auto &burst : bursts) {
    const float done = progress(burst);
    switch (burst.kind) {
    case Flourish::Steam:
      paint_steam(canvas, done);
      break;
    case Flourish::Delight:
    case Flourish::Disgust:
      paint_reaction(canvas, art, burst.kind == Flourish::Delight, done);
      break;
    case Flourish::Pop:
    case Flourish::Spark:
    case Flourish::Hit:
    case Flourish::Splash:
    case Flourish::Surge:
    case Flourish::NewDay:
      break;
    }
  }
}

void paint_screen_bursts(godot::Node2D &canvas,
                         const std::vector<Burst> &bursts) {
  const godot::Rect2 screen(0.0F, 0.0F, SCREEN_WIDTH, SCREEN_HEIGHT);
  for (const auto &burst : bursts) {
    const float done = progress(burst);
    if (burst.kind == Flourish::Hit) {
      canvas.draw_rect(screen, fade(palette().danger, 0.28F * (1.0F - done)));
    } else if (burst.kind == Flourish::NewDay) {
      // Dark at the middle of the fade, clear at both ends.
      const float dark = 1.0F - std::abs(2.0F * done - 1.0F);
      canvas.draw_rect(screen, fade(palette().background, 0.9F * dark));
    }
  }
}

} // namespace neocity::rendering
