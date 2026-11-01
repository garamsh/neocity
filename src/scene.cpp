#include "scene.hpp"

#include "experiment/catalog.hpp"
#include "input/menu.hpp"

#include <godot_cpp/classes/canvas_item.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <cstdlib>
#include <utility>

namespace neocity {

namespace {

bool in_editor() { return godot::Engine::get_singleton()->is_editor_hint(); }

constexpr const char *EXPERIMENT_NODE = "Experiment";
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

AscentScene::AscentScene()
    : camera(engine.snapshot()),
      session(
          engine, camera,
          [this] {
            engine = gameplay::AscentEngine{};
            camera = camera::AscentCamera(engine.snapshot());
          },
          [this] { return start_survival(); }, experiment::CATALOG,
          [this](const std::optional<godot::String> &scene) { mount(scene); },
          session::CinematicStage{
              .play =
                  [this](const godot::String &scene) {
                    return play_cinematic(scene);
                  },
              .playing = [this] { return cinematic_playing(); },
              .stop = [this] { stop_cinematic(); }}) {}

void AscentScene::_bind_methods() {
  godot::ClassDB::bind_method(godot::D_METHOD("actor_position"),
                              &AscentScene::actor_position);
  godot::ClassDB::bind_method(godot::D_METHOD("screen_name"),
                              &AscentScene::screen_name);
  godot::ClassDB::bind_method(godot::D_METHOD("survival_status"),
                              &AscentScene::survival_status);
  godot::ClassDB::bind_method(godot::D_METHOD("pin_survival_seed", "seed"),
                              &AscentScene::pin_survival_seed);
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
  session.advance(delta,
                  input::sample_session_input(session.survival_control()));
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

godot::Dictionary AscentScene::survival_status() const {
  return session.survival_status();
}

void AscentScene::pin_survival_seed(std::int64_t seed) {
  pinned_seed = static_cast<std::uint32_t>(seed);
}

session::SurvivalFeatures AscentScene::start_survival() {
  const auto seed = pinned_seed.value_or(
      static_cast<std::uint32_t>(godot::UtilityFunctions::randi()));
  return survival_rules.emplace(survival::temporary_content(seed)).features();
}

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

bool AscentScene::play_cinematic(const godot::String &scene) {
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
  cinematic->connect(CINEMATIC_FINISHED,
                     callable_mp(this, &AscentScene::stop_cinematic));
  add_child(cinematic);
  return true;
}

bool AscentScene::cinematic_playing() const {
  return get_node_or_null(CINEMATIC_NODE) != nullptr;
}

void AscentScene::stop_cinematic() {
  if (godot::Node *open = get_node_or_null(CINEMATIC_NODE)) {
    remove_child(open);
    open->queue_free();
  }
}

} // namespace neocity
