#pragma once

#include <pan_dodge/engine.hpp>

#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <vector>

namespace neocity::experiment {

/// Plays the pan dodge rules as a 3D scene; runs alone or mounted by the
/// developer page.
// The suppressed diagnostics fire inside godot-cpp's GDCLASS expansion.
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class PanDodgeScene : public godot::Node3D {
  // NOLINTNEXTLINE(misc-const-correctness,modernize-use-auto)
  GDCLASS(PanDodgeScene, godot::Node3D)

public:
  void _ready() override;
  void _physics_process(double delta) override;
  void _process(double delta) override;

  /// Reports "playing", "escaped", or "hit" for host integration checks.
  [[nodiscard]] godot::String outcome() const;
  /// Reports the player centre in pan units.
  [[nodiscard]] godot::Vector2 player_position() const;
  /// Reports the rim opening's centre angle in radians.
  [[nodiscard]] double exit_angle() const;
  /// Reports the pan's own sway on each axis.
  [[nodiscard]] godot::Vector2 pan_tilt() const;
  /// Reports how many hazards are currently spawned.
  [[nodiscard]] int64_t hazard_count() const;
  /// Reports the rule ticks run since the last reset.
  [[nodiscard]] int64_t run_tick() const;

protected:
  static void _bind_methods();

private:
  /// One pooled hazard's body and drop shadow, children of the pan.
  struct HazardView {
    godot::MeshInstance3D *body = nullptr;
    godot::MeshInstance3D *shadow = nullptr;
  };

  void build();
  void show_hazards(const pan_dodge::Snapshot &world);

  pan_dodge::Engine engine;
  // Scene-tree children owned by Godot; null until _ready builds them.
  godot::Node3D *pan = nullptr;
  godot::MeshInstance3D *player = nullptr;
  godot::MeshInstance3D *exit = nullptr;
  godot::Label *status = nullptr;
  godot::Ref<godot::StandardMaterial3D> hazard_surface;
  godot::Ref<godot::StandardMaterial3D> tracking_shadow;
  godot::Ref<godot::StandardMaterial3D> locked_shadow;
  std::vector<HazardView> hazards;
};

} // namespace neocity::experiment
