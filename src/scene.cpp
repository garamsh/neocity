#include "scene.hpp"

#include "hud/survival.hpp"
#include "input/menu.hpp"

#include <godot_cpp/classes/canvas_item.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource.hpp>
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
/// The material that makes the glow layer add its light to what is under it.
constexpr const char *GLOW_MATERIAL =
    "res://levels/monster_mouth_escape/art/glow.tres";
/// The shader material of the pass that finishes the scene and its light.
constexpr const char *FINISH_MATERIAL =
    "res://levels/monster_mouth_escape/art/finish.tres";
/// The canvas the finishing pass covers.
constexpr godot::Vector2 SCREEN_SIZE(1280.0F, 720.0F);

/// Loads a layer's material and gives it to `layer`, stopping the host when
/// either fails.
godot::Ref<godot::Resource> dress(godot::Node2D &layer, const char *file) {
  const godot::Ref<godot::Resource> material =
      godot::ResourceLoader::get_singleton()->load(file);
  CRASH_COND_MSG(material.is_null(),
                 godot::String("Material must load: ") + file);
  // The trimmed bindings have no material setter; the property takes it.
  layer.set("material", material);
  CRASH_COND_MSG(layer.get("material") != godot::Variant(material),
                 godot::String("Layer must take its material: ") + file);
  return material;
}

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
    : session(
          [this] { return start_survival(); },
          session::CinematicStage{
              .play =
                  [this](const godot::String &scene) {
                    return play_cinematic(scene);
                  },
              .playing = [this] { return cinematic_playing(); },
              .stop = [this] { stop_cinematic(); }},
          session::SoundStage{.play =
                                  [this](audio::Cue cue) {
                                    if (speaker) {
                                      speaker->play(cue);
                                    }
                                  },
                              .music =
                                  [this](audio::Track track, float tension) {
                                    if (speaker) {
                                      speaker->music(track, tension);
                                    }
                                  },
                              .volume =
                                  [this](audio::Bus bus, float level) {
                                    if (speaker) {
                                      speaker->volume(bus, level);
                                    }
                                  }}) {}

void MainScene::_bind_methods() {
  godot::ClassDB::bind_method(godot::D_METHOD("screen_name"),
                              &MainScene::screen_name);
  godot::ClassDB::bind_method(godot::D_METHOD("survival_status"),
                              &MainScene::survival_status);
  godot::ClassDB::bind_static_method(
      "MainScene", godot::D_METHOD("controls_keys"), &MainScene::controls_keys);
  godot::ClassDB::bind_method(godot::D_METHOD("pin_survival_seed", "seed"),
                              &MainScene::pin_survival_seed);
  godot::ClassDB::bind_method(godot::D_METHOD("sound_status"),
                              &MainScene::sound_status);
}

void MainScene::_ready() {
  if (in_editor()) {
    return;
  }
  drawing = rendering::Art::load();
  speaker = audio::Speaker::attach(*this);
  world = memnew(godot::Node2D);
  glow = memnew(godot::Node2D);
  finish = memnew(godot::Node2D);
  dress(*glow, GLOW_MATERIAL);
  finishing = dress(*finish, FINISH_MATERIAL);
  world->set_name("World");
  finish->set_name("Finish");
  for (godot::Node2D *layer : {world, glow, finish}) {
    layer->set_draw_behind_parent(true);
    add_child(layer);
  }
  world->connect("draw", callable_mp(this, &MainScene::draw_world));
  glow->connect("draw", callable_mp(this, &MainScene::draw_glow));
  finish->connect("draw", callable_mp(this, &MainScene::draw_finish));
  godot::UtilityFunctions::print(
      "Host: title screen and Monster Mouth Escape ready.");
}

void MainScene::_exit_tree() {
  if (speaker) {
    speaker->stop();
  }
}

void MainScene::_process(double delta) {
  if (in_editor()) {
    return;
  }
  session.advance(delta,
                  input::sample_session_input(session.survival_control()));
  if (drawing) {
    session.aim(*drawing);
  }
  queue_redraw();
  // Finish redraws last: it reads the waterline the world just drew.
  if (world != nullptr && glow != nullptr && finish != nullptr) {
    world->queue_redraw();
    glow->queue_redraw();
    finish->queue_redraw();
  }
}

void MainScene::draw_world() {
  if (drawing && world != nullptr) {
    session.paint_world(*world, *drawing);
  }
}

void MainScene::draw_glow() {
  if (drawing && glow != nullptr) {
    session.paint_glow(*glow, *drawing);
  }
}

void MainScene::draw_finish() {
  if (!drawing || finish == nullptr) {
    return;
  }
  // Only a frame that drew the acid is finished.
  const auto line = drawing->take_waterline();
  if (!line) {
    return;
  }
  finishing->set("shader_parameter/time", drawing->seconds());
  finishing->set("shader_parameter/band", line->band);
  finishing->set("shader_parameter/band_left", line->band_left);
  finishing->set("shader_parameter/sea", line->sea);
  finishing->set("shader_parameter/grid_origin", line->origin);
  finishing->set("shader_parameter/grid", line->zoom);
  finish->draw_rect(godot::Rect2(0.0F, 0.0F, SCREEN_SIZE.x, SCREEN_SIZE.y),
                    godot::Color(1.0F, 1.0F, 1.0F));
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

godot::Dictionary MainScene::sound_status() const {
  return speaker ? speaker->status() : godot::Dictionary();
}

godot::PackedStringArray MainScene::controls_keys() {
  godot::PackedStringArray rows;
  for (const auto &row : hud::control_rows()) {
    rows.push_back(row.action + godot::String(": ") + row.first +
                   godot::String(" / ") + row.second);
  }
  return rows;
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
