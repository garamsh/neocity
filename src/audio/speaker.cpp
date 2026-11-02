#include "speaker.hpp"

#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <cstddef>
#include <cstdlib>
#include <utility>

namespace neocity::audio {
namespace {

constexpr const char *FOLDER = "res://levels/monster_mouth_escape/audio/";
constexpr const char *EFFECTS_BUS = "Effects";
constexpr const char *MUSIC_BUS = "Music";
/// How many of one effect may sound at once, so quick footsteps overlap.
constexpr int POLYPHONY = 4;
/// How far full acid raises the music's pitch.
constexpr float TENSION_PITCH = 0.12F;
/// The drone's loudness under the city's music at full acid.
constexpr float DRONE_LEVEL = 0.7F;
/// Recent cues kept for the Godot checks.
constexpr std::size_t RECENT = 32;
/// Quieter than any audible level, for a silent player.
constexpr float SILENT_DB = -80.0F;

const char *effect_file(Cue cue) {
  switch (cue) {
  case Cue::Step:
    return "effects/step.ogg";
  case Cue::Jump:
    return "effects/jump.mp3";
  case Cue::Land:
    return "effects/land.ogg";
  case Cue::Collect:
    return "effects/collect.ogg";
  case Cue::Repair:
    return "effects/repair.ogg";
  case Cue::Hurt:
  case Cue::Fall:
    return "effects/hurt.ogg";
  case Cue::Travel:
    return "effects/travel.ogg";
  case Cue::Identified:
    return "effects/identified.ogg";
  case Cue::AppraisalFailed:
  case Cue::Burned:
    return "effects/failed.ogg";
  case Cue::Cooked:
    return "effects/cook.ogg";
  case Cue::Satisfied:
    return "effects/satisfied.ogg";
  case Cue::Refused:
    return "effects/refused.ogg";
  case Cue::AcidRise:
    return "effects/acid.mp3";
  case Cue::NewDay:
    return "effects/day.ogg";
  case Cue::MenuMove:
    return "effects/menu_move.ogg";
  case Cue::MenuSelect:
    return "effects/menu_select.ogg";
  case Cue::MenuBack:
    return "effects/menu_back.ogg";
  }
  std::abort();
}

const char *track_file(Track track) {
  switch (track) {
  case Track::Silence:
    return nullptr;
  case Track::Title:
    return "music/title.ogg";
  case Track::City:
    return "music/city.ogg";
  case Track::Workshop:
    return "music/workshop.ogg";
  }
  std::abort();
}

godot::Ref<godot::AudioStream> load_sound(const char *file) {
  const godot::String path = godot::String(FOLDER) + file;
  godot::Ref<godot::AudioStream> sound =
      godot::ResourceLoader::get_singleton()->load(path);
  CRASH_COND_MSG(sound.is_null(), "Sound must load: " + path);
  return sound;
}

godot::AudioStreamPlayer *
add_player(godot::Node &host, const godot::Ref<godot::AudioStream> &sound,
           const char *bus) {
  auto *player = memnew(godot::AudioStreamPlayer);
  player->set_stream(sound);
  player->set_bus(bus);
  host.add_child(player);
  return player;
}

int bus_index(Bus bus) {
  const char *name = nullptr;
  switch (bus) {
  case Bus::Effects:
    name = EFFECTS_BUS;
    break;
  case Bus::Music:
    name = MUSIC_BUS;
    break;
  }
  const int index = godot::AudioServer::get_singleton()->get_bus_index(name);
  CRASH_COND_MSG(index < 0, godot::String("Audio bus must exist: ") + name);
  return index;
}

float decibels(float level) {
  return level > 0.0F
             ? static_cast<float>(godot::UtilityFunctions::linear_to_db(level))
             : SILENT_DB;
}

} // namespace

Speaker::Speaker(std::array<godot::AudioStreamPlayer *, CUES.size()> players,
                 godot::AudioStreamPlayer *tune_player,
                 godot::AudioStreamPlayer *drone_player,
                 std::array<godot::Ref<godot::AudioStream>, 4> streams)
    : effects(players), tune(tune_player), drone(drone_player),
      tracks(std::move(streams)) {}

Speaker Speaker::attach(godot::Node &host) {
  std::array<godot::AudioStreamPlayer *, CUES.size()> effects{};
  for (const auto cue : CUES) {
    auto *player = add_player(host, load_sound(effect_file(cue)), EFFECTS_BUS);
    player->set_max_polyphony(POLYPHONY);
    effects.at(static_cast<std::size_t>(cue)) = player;
  }
  std::array<godot::Ref<godot::AudioStream>, 4> tracks;
  for (const auto track : {Track::Title, Track::City, Track::Workshop}) {
    tracks.at(static_cast<std::size_t>(track)) = load_sound(track_file(track));
  }
  auto *tune = add_player(host, {}, MUSIC_BUS);
  auto *drone = add_player(host, load_sound("music/tension.ogg"), MUSIC_BUS);
  drone->set_volume_db(SILENT_DB);
  return {effects, tune, drone, std::move(tracks)};
}

void Speaker::play(Cue cue) {
  effects.at(static_cast<std::size_t>(cue))->play();
  ++started;
  recent.emplace_back(cue_name(cue));
  while (recent.size() > RECENT) {
    recent.pop_front();
  }
}

void Speaker::music(Track track, float level) {
  tension = level;
  if (track != playing) {
    playing = track;
    tune->stop();
    drone->stop();
    const auto &stream = tracks.at(static_cast<std::size_t>(track));
    if (stream.is_valid()) {
      tune->set_stream(stream);
      tune->play();
    }
    if (track == Track::City) {
      drone->play();
    }
  }
  tune->set_pitch_scale(1.0F + TENSION_PITCH * tension);
  drone->set_volume_db(decibels(DRONE_LEVEL * tension));
}

void Speaker::volume(Bus bus, float level) {
  levels.at(static_cast<std::size_t>(bus)) = level;
  auto *server = godot::AudioServer::get_singleton();
  const int index = bus_index(bus);
  server->set_bus_volume_db(index, decibels(level));
  server->set_bus_mute(index, level <= 0.0F);
}

godot::Dictionary Speaker::status() const {
  godot::Dictionary status;
  status.set("started", static_cast<std::int64_t>(started));
  godot::Array names;
  for (const auto &name : recent) {
    names.push_back(name);
  }
  status.set("recent", names);
  status.set("track", track_name(playing));
  status.set("music_playing", tune->is_playing());
  status.set("drone_playing", drone->is_playing());
  status.set("tension", tension);
  status.set("pitch", tune->get_pitch_scale());
  status.set("drone_db", drone->get_volume_db());
  const auto *server = godot::AudioServer::get_singleton();
  for (const auto bus : {Bus::Effects, Bus::Music}) {
    const bool effects_bus = bus == Bus::Effects;
    const int index = bus_index(bus);
    status.set(effects_bus ? "effects_level" : "music_level",
               levels.at(static_cast<std::size_t>(bus)));
    status.set(effects_bus ? "effects_db" : "music_db",
               server->get_bus_volume_db(index));
    status.set(effects_bus ? "effects_muted" : "music_muted",
               server->is_bus_mute(index));
  }
  return status;
}

} // namespace neocity::audio
