#pragma once

#include "audio/speaker.hpp"
#include "rendering/drawing.hpp"
#include "session/session.hpp"

#include <survival/companion.hpp>
#include <survival/content.hpp>
#include <survival/escape.hpp>

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include <cstdint>
#include <optional>

namespace neocity {

/// One survival run's rule objects. Members hold references to one another, so
/// a value stays where it was built.
class SurvivalRules {
public:
  explicit SurvivalRules(survival::Content game);
  SurvivalRules(const SurvivalRules &) = delete;
  SurvivalRules &operator=(const SurvivalRules &) = delete;
  SurvivalRules(SurvivalRules &&) = delete;
  SurvivalRules &operator=(SurvivalRules &&) = delete;
  ~SurvivalRules() = default;

  /// Hands the session references to this run's rule objects.
  [[nodiscard]] session::SurvivalFeatures features();

private:
  survival::Content content;
  survival::Simulation simulation;
  survival::World world;
  survival::Exploration exploration;
  survival::Workshop workshop;
  survival::Escape escape;
  survival::Companion companion;
};

/// Godot composition root: wires the title screen and the survival game to
/// input and canvas adapters.
// The suppressed diagnostics fire inside godot-cpp's GDCLASS expansion.
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class MainScene : public godot::Node2D {
  // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
  GDCLASS(MainScene, godot::Node2D)

public:
  MainScene();

  void _ready() override;
  void _exit_tree() override;
  void _process(double delta) override;
  void _draw() override;

  /// Reports the open screen.
  [[nodiscard]] godot::String screen_name() const;
  /// Reports the open survival run for host integration assertions.
  [[nodiscard]] godot::Dictionary survival_status() const;
  /// The controls page's rows, each as "action: first / second".
  [[nodiscard]] static godot::PackedStringArray controls_keys();
  /// Pins the seed of every later survival run, for repeatable host checks.
  void pin_survival_seed(std::int64_t seed);
  /// Pins the clock of the scene's ambient motion, its finishing pass and the
  /// screen's eased values at `seconds` from then on, so host checks read the
  /// screen at a moment they choose.
  void pin_scene_clock(double seconds);
  /// Reports what the speakers played and play now, for host checks; empty
  /// before the scene is ready.
  [[nodiscard]] godot::Dictionary sound_status() const;
  /// Reports the display choice: its `width`, `height`, `resolution` label and
  /// `mode`.
  [[nodiscard]] godot::Dictionary display_status() const;

protected:
  static void _bind_methods();

private:
  /// Starts a cinematic; false when it cannot load or breaks the scene
  /// contract, so the session shows its fallback.
  bool play_cinematic(const godot::String &scene);
  [[nodiscard]] bool cinematic_playing() const;
  void stop_cinematic();
  session::SurvivalFeatures start_survival();
  void draw_world();
  void draw_glow();
  void draw_finish();
  /// Takes the art's layout onto the canvas the window shows.
  void scale_canvas();

  std::optional<SurvivalRules> survival_rules;
  std::optional<std::uint32_t> pinned_seed;
  session::Session session;
  std::optional<rendering::Art> drawing;
  std::optional<audio::Speaker> speaker;
  /// The layers drawn behind this node's own drawing: the scene, the light
  /// added over it, and the pass that finishes both. The tree owns them.
  godot::Node2D *world = nullptr;
  godot::Node2D *glow = nullptr;
  godot::Node2D *finish = nullptr;
  /// The finishing pass's shader material.
  godot::Ref<godot::Resource> finishing;
};

} // namespace neocity
