#include "scene.hpp"

#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/cylinder_mesh.hpp>
#include <godot_cpp/classes/directional_light3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/sphere_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/torus_mesh.hpp>
#include <godot_cpp/classes/world_environment.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cmath>
#include <cstdlib>
#include <numbers>

namespace neocity::experiment {
namespace {

/// Metres per pan unit.
constexpr float SCALE = 0.01F;
constexpr float MAX_LEAN = 0.25F;

bool in_editor() { return godot::Engine::get_singleton()->is_editor_hint(); }

pan_dodge::Direction axis(float value) {
  if (value < 0.0F) {
    return pan_dodge::Direction::Negative;
  }
  return value > 0.0F ? pan_dodge::Direction::Positive
                      : pan_dodge::Direction::Neutral;
}

godot::Ref<godot::StandardMaterial3D> material(godot::Color albedo,
                                               bool glow = false) {
  godot::Ref<godot::StandardMaterial3D> surface;
  surface.instantiate();
  surface->set_albedo(albedo);
  if (albedo.a < 1.0F) {
    surface->set_transparency(godot::BaseMaterial3D::TRANSPARENCY_ALPHA);
  }
  if (glow) {
    surface->set_feature(godot::BaseMaterial3D::FEATURE_EMISSION, true);
    surface->set_emission(albedo);
  }
  return surface;
}

godot::MeshInstance3D *instance(godot::Node &parent,
                                const godot::Ref<godot::Mesh> &mesh,
                                const godot::Ref<godot::Material> &surface) {
  auto *node = memnew(godot::MeshInstance3D);
  node->set_mesh(mesh);
  node->set_material_override(surface);
  parent.add_child(node);
  return node;
}

godot::Ref<godot::CylinderMesh> cylinder(float radius, float height) {
  godot::Ref<godot::CylinderMesh> mesh;
  mesh.instantiate();
  mesh->set_top_radius(radius);
  mesh->set_bottom_radius(radius);
  mesh->set_height(height);
  return mesh;
}

godot::Label *label(godot::Node &parent, godot::Vector2 position,
                    const godot::String &text, int size, godot::Color tint) {
  auto *node = memnew(godot::Label);
  node->set_position(position);
  node->set_text(text);
  node->add_theme_font_size_override("font_size", size);
  node->add_theme_color_override("font_color", tint);
  parent.add_child(node);
  return node;
}

godot::Vector3 on_pan(pan_dodge::Point point, float height) {
  return {point.x * SCALE, height, point.y * SCALE};
}

} // namespace

void PanDodgeScene::_bind_methods() {
  godot::ClassDB::bind_method(godot::D_METHOD("outcome"),
                              &PanDodgeScene::outcome);
  godot::ClassDB::bind_method(godot::D_METHOD("player_position"),
                              &PanDodgeScene::player_position);
  godot::ClassDB::bind_method(godot::D_METHOD("exit_angle"),
                              &PanDodgeScene::exit_angle);
  godot::ClassDB::bind_method(godot::D_METHOD("pan_tilt"),
                              &PanDodgeScene::pan_tilt);
  godot::ClassDB::bind_method(godot::D_METHOD("hazard_count"),
                              &PanDodgeScene::hazard_count);
  godot::ClassDB::bind_method(godot::D_METHOD("run_tick"),
                              &PanDodgeScene::run_tick);
}

void PanDodgeScene::_ready() {
  if (in_editor()) {
    return;
  }
  build();
}

void PanDodgeScene::build() {
  godot::Ref<godot::Environment> sky;
  sky.instantiate();
  sky->set_background(godot::Environment::BG_COLOR);
  sky->set_bg_color(godot::Color(0.07F, 0.09F, 0.12F));
  sky->set_ambient_source(godot::Environment::AMBIENT_SOURCE_COLOR);
  sky->set_ambient_light_color(godot::Color(0.55F, 0.6F, 0.7F));
  auto *environment = memnew(godot::WorldEnvironment);
  environment->set_environment(sky);
  add_child(environment);

  auto *sun = memnew(godot::DirectionalLight3D);
  add_child(sun);
  sun->set_rotation(godot::Vector3(-1.0F, 0.5F, 0.0F));

  auto *camera = memnew(godot::Camera3D);
  add_child(camera);
  camera->set_position(godot::Vector3(1.1F, 4.6F, 3.7F));
  camera->look_at(godot::Vector3(1.1F, 0.0F, 0.3F));
  camera->set_current(true);

  const pan_dodge::Snapshot world = engine.snapshot();
  const float radius = world.pan_radius * SCALE;
  pan = memnew(godot::Node3D);
  add_child(pan);
  instance(*pan, cylinder(radius + 0.08F, 0.12F),
           material(godot::Color(0.17F, 0.19F, 0.24F)))
      ->set_position(godot::Vector3(0.0F, -0.06F, 0.0F));
  godot::Ref<godot::TorusMesh> rim;
  rim.instantiate();
  rim->set_inner_radius(radius);
  rim->set_outer_radius(radius + 0.16F);
  instance(*pan, rim, material(godot::Color(0.56F, 0.6F, 0.67F)))
      ->set_position(godot::Vector3(0.0F, 0.03F, 0.0F));
  godot::Ref<godot::BoxMesh> handle;
  handle.instantiate();
  handle->set_size(godot::Vector3(1.6F, 0.1F, 0.3F));
  instance(*pan, handle, material(godot::Color(0.41F, 0.43F, 0.49F)))
      ->set_position(godot::Vector3(-radius - 0.85F, 0.0F, 0.0F));

  godot::Ref<godot::BoxMesh> gate;
  gate.instantiate();
  gate->set_size(godot::Vector3(
      2.0F * (radius + 0.08F) * std::sin(world.exit_half_angle), 0.12F, 0.3F));
  exit =
      instance(*pan, gate, material(godot::Color(0.31F, 0.82F, 0.65F), true));
  player = instance(*pan, cylinder(world.player.radius * SCALE, 0.3F),
                    material(godot::Color(0.36F, 0.79F, 1.0F)));

  hazard_surface = material(godot::Color(0.96F, 0.44F, 0.36F));
  tracking_shadow = material(godot::Color(1.0F, 0.62F, 0.2F, 0.6F));
  locked_shadow = material(godot::Color(0.95F, 0.2F, 0.2F, 0.7F));

  auto *hud = memnew(godot::CanvasLayer);
  add_child(hud);
  const godot::Color white(0.92F, 0.94F, 0.97F);
  const godot::Color muted(0.6F, 0.65F, 0.72F);
  label(*hud, {40.0F, 24.0F}, "PAN DODGE 3D", 34, white);
  label(*hud, {900.0F, 120.0F},
        "WASD / ARROW KEYS: move\n\nThe pan sways and pushes you.\nShadows "
        "mark falling hazards:\norange follows you, red is locked.\nLanded "
        "red balls slide and\ndrop off the edge.\n\nEscape through the "
        "green gate\nas it travels around the rim.",
        20, white);
  label(*hud, {900.0F, 470.0F}, "R: fresh run\nM: developer page", 22,
        godot::Color(0.37F, 0.86F, 0.9F));
  label(*hud, {900.0F, 560.0F}, "Plain prototype: no fun or\nbalance claim.",
        18, muted);
  status = label(*hud, {40.0F, 660.0F}, "", 24, white);
}

void PanDodgeScene::_physics_process(double /*delta*/) {
  if (in_editor()) {
    return;
  }
  const godot::Input *input = godot::Input::get_singleton();
  if (input->is_action_just_pressed("retry")) {
    engine.reset();
    return;
  }
  engine.step({.horizontal = axis(input->get_axis("move_left", "move_right")),
               .vertical = axis(input->get_axis("move_up", "move_down"))});
}

void PanDodgeScene::_process(double /*delta*/) {
  if (pan == nullptr) {
    return;
  }
  const pan_dodge::Snapshot world = engine.snapshot();
  pan->set_rotation(
      godot::Vector3(world.tilt.y * MAX_LEAN, 0.0F, -world.tilt.x * MAX_LEAN));
  const float rim = (world.pan_radius + 8.0F) * SCALE;
  exit->set_position(godot::Vector3(rim * std::cos(world.exit_angle), 0.1F,
                                    rim * std::sin(world.exit_angle)));
  exit->set_rotation(godot::Vector3(
      0.0F, -(world.exit_angle + std::numbers::pi_v<float> / 2.0F), 0.0F));
  player->set_position(on_pan(world.player.center, 0.15F));
  show_hazards(world);
  switch (world.outcome) {
  case pan_dodge::Outcome::Playing:
    status->set_text("Blue: you    Red: hazard    Green: exit");
    return;
  case pan_dodge::Outcome::Escaped:
    status->set_text("ESCAPED - R to replay / M for the developer page");
    return;
  case pan_dodge::Outcome::Hit:
    status->set_text("HIT A HAZARD - R to retry / M for the developer page");
    return;
  }
  std::abort();
}

void PanDodgeScene::show_hazards(const pan_dodge::Snapshot &world) {
  while (hazards.size() < world.hazards.size()) {
    godot::Ref<godot::SphereMesh> ball;
    ball.instantiate();
    ball->set_radius(1.0F);
    ball->set_height(2.0F);
    hazards.push_back(
        {.body = instance(*pan, ball, hazard_surface),
         .shadow = instance(*pan, cylinder(1.0F, 0.01F), tracking_shadow)});
  }
  for (std::size_t i = 0; i < hazards.size(); ++i) {
    const HazardView view = hazards.at(i);
    const bool shown = i < world.hazards.size();
    view.body->set_visible(shown);
    view.shadow->set_visible(shown);
    if (!shown) {
      continue;
    }
    const pan_dodge::Hazard &hazard = world.hazards.at(i);
    const float size = hazard.body.radius * SCALE;
    view.body->set_scale(godot::Vector3(size, size, size));
    view.body->set_position(
        on_pan(hazard.body.center, hazard.height * SCALE + size));
    view.shadow->set_scale(godot::Vector3(size, 1.0F, size));
    view.shadow->set_position(on_pan(hazard.body.center, 0.01F));
    switch (hazard.phase) {
    case pan_dodge::Phase::Tracking:
      view.shadow->set_material_override(tracking_shadow);
      break;
    case pan_dodge::Phase::Falling:
      view.shadow->set_material_override(locked_shadow);
      break;
    case pan_dodge::Phase::Landed:
      view.shadow->set_visible(false);
      break;
    }
  }
}

godot::String PanDodgeScene::outcome() const {
  switch (engine.snapshot().outcome) {
  case pan_dodge::Outcome::Playing:
    return "playing";
  case pan_dodge::Outcome::Escaped:
    return "escaped";
  case pan_dodge::Outcome::Hit:
    return "hit";
  }
  std::abort();
}

godot::Vector2 PanDodgeScene::player_position() const {
  const auto p = engine.snapshot().player.center;
  return {p.x, p.y};
}

double PanDodgeScene::exit_angle() const {
  return engine.snapshot().exit_angle;
}

godot::Vector2 PanDodgeScene::pan_tilt() const {
  const auto tilt = engine.snapshot().tilt;
  return {tilt.x, tilt.y};
}

int64_t PanDodgeScene::hazard_count() const {
  return static_cast<int64_t>(engine.snapshot().hazards.size());
}

int64_t PanDodgeScene::run_tick() const { return engine.snapshot().tick; }

} // namespace neocity::experiment
