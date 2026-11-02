#pragma once

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

/// One sound effect, named for what it answers.
enum class Cue : std::uint8_t {
  Step,
  Jump,
  Land,
  Collect,
  Repair,
  Hurt,
  Fall,
  Travel,
  Identified,
  AppraisalFailed,
  Cooked,
  Burned,
  Satisfied,
  Refused,
  AcidRise,
  NewDay,
  MenuMove,
  MenuSelect,
  MenuBack
};

/// Every cue, in declaration order.
inline constexpr std::array CUES{Cue::Step,       Cue::Jump,
                                 Cue::Land,       Cue::Collect,
                                 Cue::Repair,     Cue::Hurt,
                                 Cue::Fall,       Cue::Travel,
                                 Cue::Identified, Cue::AppraisalFailed,
                                 Cue::Cooked,     Cue::Burned,
                                 Cue::Satisfied,  Cue::Refused,
                                 Cue::AcidRise,   Cue::NewDay,
                                 Cue::MenuMove,   Cue::MenuSelect,
                                 Cue::MenuBack};

/// The background music of a screen; `Silence` plays none.
enum class Track : std::uint8_t { Silence, Title, City, Workshop };

/// The two volumes a player sets.
enum class Bus : std::uint8_t { Effects, Music };

/// Highest volume level; level 0 plays nothing on that bus.
inline constexpr std::uint8_t VOLUME_STEPS = 10;

/// What the pause screen sets: each bus's level from 0 to `VOLUME_STEPS`, and
/// the row its left and right keys change.
struct Volume {
  std::uint8_t effects = 8;
  std::uint8_t music = 6;
  Bus marked = Bus::Effects;
};

/// Plays sound for the session; the composition root supplies it.
struct SoundStage {
  /// Starts one effect.
  std::function<void(Cue cue)> play;
  /// Keeps `track` playing, its tension from 0, calm, to 1, the acid full.
  std::function<void(Track track, float tension)> music;
  /// Sets a bus's loudness, from 0, silent, to 1.
  std::function<void(Bus bus, float level)> volume;
};

/// Something play did that the screen and the speakers answer: a cue, and
/// where in the city it happened when it happened there.
struct Moment {
  Cue cue = Cue::Step;
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
[[nodiscard]] std::optional<rendering::Flourish> flourish_of(Cue cue);
/// The cue's name, as the harness reads it.
[[nodiscard]] const char *cue_name(Cue cue);
/// The track's name, as the harness reads it.
[[nodiscard]] const char *track_name(Track track);
/// `level` of `VOLUME_STEPS` as loudness from 0 to 1.
[[nodiscard]] float loudness(std::uint8_t level);
/// The pause screen's volume rows, effects first.
[[nodiscard]] std::array<hud::VolumeRow, 2> volume_rows(const Volume &volume);

} // namespace neocity::session
