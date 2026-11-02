#pragma once

#include "sound.hpp"

#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

#include <array>
#include <cstdint>
#include <deque>

namespace neocity::audio {

/// Plays the game's sound through Godot's audio players, on the Effects and
/// Music buses of `project/default_bus_layout.tres`, and records what it was
/// asked to play.
class Speaker {
public:
  /// Loads every sound of the game and adds its players to `host`, which
  /// owns them.
  static Speaker attach(godot::Node &host);

  /// Starts `cue` on its own player.
  void play(Cue cue);
  /// Keeps `track` playing, starting it over only when it changes; `level`
  /// of tension raises its pitch and, in the city, the drone under it.
  void music(Track track, float level);
  /// Sets the loudness of `bus`; 0 mutes it.
  void volume(Bus bus, float level);
  /// What the speakers did and do now, for the Godot checks.
  [[nodiscard]] godot::Dictionary status() const;

private:
  Speaker(std::array<godot::AudioStreamPlayer *, CUES.size()> players,
          godot::AudioStreamPlayer *tune_player,
          godot::AudioStreamPlayer *drone_player,
          std::array<godot::Ref<godot::AudioStream>, 4> streams);

  /// Not owned: the host's tree frees them.
  std::array<godot::AudioStreamPlayer *, CUES.size()> effects;
  godot::AudioStreamPlayer *tune;
  godot::AudioStreamPlayer *drone;
  /// Indexed by `Track`; `Silence` holds none.
  std::array<godot::Ref<godot::AudioStream>, 4> tracks;
  Track playing = Track::Silence;
  float tension = 0.0F;
  std::array<float, 2> levels{};
  std::uint64_t started = 0;
  std::deque<godot::String> recent;
};

} // namespace neocity::audio
