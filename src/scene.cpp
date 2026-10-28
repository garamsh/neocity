#include "scene.hpp"

#include "experiment/catalog.hpp"
#include "input/menu.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <cstdlib>

namespace neocity {

namespace {

bool in_editor() { return godot::Engine::get_singleton()->is_editor_hint(); }

constexpr const char *EXPERIMENT_NODE = "Experiment";

} // namespace

AscentScene::AscentScene()
    : camera(engine.snapshot()),
      session(
          engine, camera,
          [this] {
            engine = gameplay::AscentEngine{};
            camera = camera::AscentCamera(engine.snapshot());
          },
          experiment::CATALOG,
          [this](const std::optional<godot::String> &scene) { mount(scene); }) {
}

void AscentScene::_bind_methods() {
  godot::ClassDB::bind_method(godot::D_METHOD("actor_position"),
                              &AscentScene::actor_position);
  godot::ClassDB::bind_method(godot::D_METHOD("screen_name"),
                              &AscentScene::screen_name);
}

void AscentScene::_ready() {
  if (in_editor()) {
    return;
  }
  drawing = rendering::AscentDrawing::load();
  godot::UtilityFunctions::print(
      "Host: main menu, main game, and developer experiments ready.");
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

void AscentScene::mount(const std::optional<godot::String> &scene) {
  if (godot::Node *open = get_node_or_null(EXPERIMENT_NODE)) {
    remove_child(open);
    open->queue_free();
  }
  if (!scene) {
    return;
  }
  const godot::Ref<godot::PackedScene> packed =
      godot::ResourceLoader::get_singleton()->load(*scene);
  if (packed.is_null()) {
    godot::UtilityFunctions::push_error("Experiment scene missing: ", *scene);
    std::abort();
  }
  godot::Node *experiment = packed->instantiate();
  if (experiment == nullptr) {
    godot::UtilityFunctions::push_error(
        "Experiment scene did not instantiate: ", *scene);
    std::abort();
  }
  experiment->set_name(EXPERIMENT_NODE);
  add_child(experiment);
}

} // namespace neocity
