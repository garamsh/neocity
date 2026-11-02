#include "scene.hpp"

#include "input/menu.hpp"

#include <godot_cpp/classes/canvas_item.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <utility>

namespace neocity {

namespace {

bool in_editor() { return godot::Engine::get_singleton()->is_editor_hint(); }

constexpr const char *CINEMATIC_NODE = "Cinematic";
constexpr const char *CINEMATIC_FINISHED = "finished";

} // namespace

SurvivalRules::SurvivalRules(survival::Content game)
    : content(std::move(game)), simulation(content.run), world(content.objects),
      exploration(simulation, world, content.companion.exploration),
      workshop(simulation, content.companion.workshop),
      escape(simulation, survival::TEMPORARY_ESCAPE),
      companion(content.companion) {}

session::SurvivalFeatures SurvivalRules::features() {
  return {.content = content,
          .simulation = simulation,
          .world = world,
          .exploration = exploration,
          .workshop = workshop,
          .escape = escape,
          .companion = companion};
}

MainScene::MainScene()
    : session([this] { return start_survival(); },
              session::CinematicStage{
                  .play =
                      [this](const godot::String &scene) {
                        return play_cinematic(scene);
                      },
                  .playing = [this] { return cinematic_playing(); },
                  .stop = [this] { stop_cinematic(); }}) {}

void MainScene::_bind_methods() {
  godot::ClassDB::bind_method(godot::D_METHOD("screen_name"),
                              &MainScene::screen_name);
  godot::ClassDB::bind_method(godot::D_METHOD("survival_status"),
                              &MainScene::survival_status);
  godot::ClassDB::bind_method(godot::D_METHOD("pin_survival_seed", "seed"),
                              &MainScene::pin_survival_seed);
}

void MainScene::_ready() {
  if (in_editor()) {
    return;
  }
  drawing = rendering::AscentDrawing::load();
  godot::UtilityFunctions::print(
      "Host: title screen and Monster Mouth Escape ready.");
}

void MainScene::_process(double delta) {
  if (in_editor()) {
    return;
  }
  session.advance(delta,
                  input::sample_session_input(session.survival_control()));
  queue_redraw();
}

void MainScene::_draw() {
  if (drawing) {
    session.paint(*this, *drawing);
  }
}

godot::String MainScene::screen_name() const { return session.screen_name(); }

godot::Dictionary MainScene::survival_status() const {
  return session.survival_status();
}

void MainScene::pin_survival_seed(std::int64_t seed) {
  pinned_seed = static_cast<std::uint32_t>(seed);
}

session::SurvivalFeatures MainScene::start_survival() {
  const auto seed = pinned_seed.value_or(
      static_cast<std::uint32_t>(godot::UtilityFunctions::randi()));
  return survival_rules.emplace(survival::temporary_content(seed)).features();
}

bool MainScene::play_cinematic(const godot::String &scene) {
  auto *loader = godot::ResourceLoader::get_singleton();
  if (!loader->exists(scene)) {
    return false;
  }
  const godot::Ref<godot::PackedScene> packed = loader->load(scene);
  godot::Node *cinematic = packed.is_valid() ? packed->instantiate() : nullptr;
  if (cinematic == nullptr) {
    godot::UtilityFunctions::push_warning("Cinematic did not load: ", scene);
    return false;
  }
  if (godot::Object::cast_to<godot::CanvasItem>(cinematic) == nullptr ||
      !cinematic->has_signal(CINEMATIC_FINISHED)) {
    godot::UtilityFunctions::push_warning("Cinematic breaks the contract: ",
                                          scene);
    cinematic->queue_free();
    return false;
  }
  cinematic->set_name(CINEMATIC_NODE);
  // Connected before add_child to hear a scene that finishes while entering the
  // tree, and deferred because add_child cannot survive removing that node.
  cinematic->connect(CINEMATIC_FINISHED,
                     callable_mp(this, &MainScene::stop_cinematic),
                     godot::Object::CONNECT_DEFERRED);
  add_child(cinematic);
  return true;
}

bool MainScene::cinematic_playing() const {
  return get_node_or_null(CINEMATIC_NODE) != nullptr;
}

void MainScene::stop_cinematic() {
  if (godot::Node *open = get_node_or_null(CINEMATIC_NODE)) {
    remove_child(open);
    open->queue_free();
  }
}

} // namespace neocity
