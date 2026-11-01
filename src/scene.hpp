#pragma once

#include "camera/camera.hpp"
#include "rendering/drawing.hpp"
#include "session/session.hpp"

#include <gameplay/engine.hpp>
#include <survival/companion.hpp>
#include <survival/content.hpp>

#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2.hpp>

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
  survival::Companion companion;
};

/// Godot composition root: wires both games, the menus, and the registered
/// experiments to input, camera, and canvas adapters.
// The suppressed diagnostics fire inside godot-cpp's GDCLASS expansion.
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class AscentScene : public godot::Node2D {
  // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
  GDCLASS(AscentScene, godot::Node2D)

public:
  AscentScene();

  void _ready() override;
  void _process(double delta) override;
  void _draw() override;

  /// Exposes the ascent game's actor location for host integration assertions.
  [[nodiscard]] godot::Vector2 actor_position() const;
  /// Reports the open screen, or the id of the mounted experiment.
  [[nodiscard]] godot::String screen_name() const;
  /// Reports the open survival run for host integration assertions.
  [[nodiscard]] godot::Dictionary survival_status() const;
  /// Pins the seed of every later survival run, for repeatable host checks.
  void pin_survival_seed(std::int64_t seed);

protected:
  static void _bind_methods();

private:
  void mount(const std::optional<godot::String> &scene);
  session::SurvivalFeatures start_survival();

  gameplay::AscentEngine engine;
  camera::AscentCamera camera;
  std::optional<SurvivalRules> survival_rules;
  std::optional<std::uint32_t> pinned_seed;
  session::Session session;
  std::optional<rendering::AscentDrawing> drawing;
};

} // namespace neocity
