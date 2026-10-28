#include "scene.hpp"

#include "input.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>

namespace neocity::ascent {

namespace {

constexpr double TICK = 1.0 / 120.0;

bool in_editor() { return godot::Engine::get_singleton()->is_editor_hint(); }

} // namespace

AscentScene::AscentScene() : camera(engine.snapshot()) {}

void AscentScene::_bind_methods() {
  godot::ClassDB::bind_method(godot::D_METHOD("actor_position"),
                              &AscentScene::actor_position);
}

void AscentScene::_ready() {
  if (in_editor()) {
    return;
  }
  drawing = AscentDrawing::load();
  godot::UtilityFunctions::print(
      "Ascent: C++ simulation and presentation ready.");
}

void AscentScene::_process(double delta) {
  if (in_editor()) {
    return;
  }
  const PlayerCommand sampled = sample_input();
  pending.horizontal = sampled.horizontal;
  pending.jump_held = sampled.jump_held;
  pending.jump_pressed = pending.jump_pressed || sampled.jump_pressed;
  pending.switch_character =
      pending.switch_character || sampled.switch_character;
  pending.pause_pressed = pending.pause_pressed || sampled.pause_pressed;
  pending.retry_pressed = pending.retry_pressed || sampled.retry_pressed;
  accumulator = std::min(accumulator + delta, 0.1);
  while (accumulator >= TICK) {
    engine.step(pending);
    pending.jump_pressed = false;
    pending.switch_character = false;
    pending.pause_pressed = false;
    pending.retry_pressed = false;
    accumulator -= TICK;
  }
  camera.follow(engine.snapshot(), static_cast<float>(std::min(delta, 0.1)));
  queue_redraw();
}

void AscentScene::_draw() {
  if (drawing) {
    drawing->paint(*this, engine.snapshot(), camera.y());
  }
}

godot::Vector2 AscentScene::actor_position() const {
  const Point p = engine.snapshot().leader.feet;
  return {p.x, p.y};
}

} // namespace neocity::ascent
