#include "scene.hpp"

#include "input/experiments.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace neocity {

namespace {

bool in_editor() { return godot::Engine::get_singleton()->is_editor_hint(); }

} // namespace

AscentScene::AscentScene()
    : camera(engine.snapshot()), session(dodge, tilt, engine, camera, [this] {
        engine = gameplay::AscentEngine{};
        camera = camera::AscentCamera(engine.snapshot());
      }) {}

void AscentScene::_bind_methods() {
  godot::ClassDB::bind_method(godot::D_METHOD("actor_position"),
                              &AscentScene::actor_position);
  godot::ClassDB::bind_method(godot::D_METHOD("screen_name"),
                              &AscentScene::screen_name);
  godot::ClassDB::bind_method(godot::D_METHOD("outcome"),
                              &AscentScene::outcome);
  godot::ClassDB::bind_method(godot::D_METHOD("actor_velocity"),
                              &AscentScene::actor_velocity);
  godot::ClassDB::bind_method(godot::D_METHOD("hazard_position"),
                              &AscentScene::hazard_position);
  godot::ClassDB::bind_method(godot::D_METHOD("pan_tilt"),
                              &AscentScene::pan_tilt);
}

void AscentScene::_ready() {
  if (in_editor()) {
    return;
  }
  drawing = rendering::AscentDrawing::load();
  godot::UtilityFunctions::print(
      "Experiments: menu and three independent rule engines ready.");
}

void AscentScene::_process(double delta) {
  if (in_editor()) {
    return;
  }
  session.advance(delta, input::sample_session_input());
  queue_redraw();
}

void AscentScene::_draw() {
  if (drawing) {
    session.paint(*this, *drawing);
  }
}

godot::Vector2 AscentScene::actor_position() const {
  return session.actor_position();
}
godot::String AscentScene::screen_name() const { return session.screen_name(); }
godot::String AscentScene::outcome() const { return session.outcome(); }
godot::Vector2 AscentScene::actor_velocity() const {
  return session.actor_velocity();
}
godot::Vector2 AscentScene::hazard_position() const {
  return session.hazard_position();
}
godot::Vector2 AscentScene::pan_tilt() const { return session.pan_tilt(); }

} // namespace neocity
