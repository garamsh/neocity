#pragma once

#include "../audio/sound.hpp"
#include "../hud/survival.hpp"
#include "../rendering/feedback.hpp"

#include <survival/companion.hpp>

#include <godot_cpp/variant/string.hpp>

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace neocity::session {

/// Highest volume level; level 0 plays nothing on that bus.
inline constexpr std::uint8_t VOLUME_STEPS = 10;

/// What the pause screen sets: each bus's level from 0 to `VOLUME_STEPS`, and
/// the row its left and right keys change.
struct Volume {
  std::uint8_t effects = 8;
  std::uint8_t music = 6;
  audio::Bus marked = audio::Bus::Effects;
};

/// Plays sound for the session; the composition root supplies it.
struct SoundStage {
  /// Starts one effect.
  std::function<void(audio::Cue cue)> play;
  /// Keeps `track` playing, its tension from 0, calm, to 1, the acid full.
  std::function<void(audio::Track track, float tension)> music;
  /// Sets a bus's loudness, from 0, silent, to 1.
  std::function<void(audio::Bus bus, float level)> volume;
};

/// Something play did that the screen and the speakers answer: a cue, and
/// where in the city it happened when it happened there.
struct Moment {
  audio::Cue cue = audio::Cue::Step;
  std::optional<survival::Position> at;
};

/// How far each actor has walked since its last footstep, in sub-cells.
using Strides = std::array<std::int32_t, 2>;

/// The moments between two views of a run, with the events recorded between
/// them; `strides` carries each actor's walking across calls.
[[nodiscard]] std::vector<Moment> moments(const survival::GameView &before,
                                          const survival::GameView &after,
                                          const std::vector<hud::Event> &events,
                                          Strides &strides);
/// The flourish the screen shows for `cue`, if it shows one.
[[nodiscard]] std::optional<rendering::Flourish> flourish_of(audio::Cue cue);
/// `level` of `VOLUME_STEPS` as loudness from 0 to 1.
[[nodiscard]] float loudness(std::uint8_t level);
/// The pause screen's volume rows, effects first.
[[nodiscard]] std::array<hud::VolumeRow, 2> volume_rows(const Volume &volume);

} // namespace neocity::session
