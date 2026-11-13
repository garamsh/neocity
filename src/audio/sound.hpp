#pragma once

#include <array>
#include <cstdint>

namespace neocity::audio {

/// One sound effect, named for what it answers.
enum class Cue : std::uint8_t {
  Step,
  Jump,
  Land,
  Collect,
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
inline constexpr std::array CUES{
    Cue::Step,     Cue::Jump,   Cue::Land,       Cue::Collect,
    Cue::Fall,     Cue::Travel, Cue::Identified, Cue::AppraisalFailed,
    Cue::Cooked,   Cue::Burned, Cue::Satisfied,  Cue::Refused,
    Cue::AcidRise, Cue::NewDay, Cue::MenuMove,   Cue::MenuSelect,
    Cue::MenuBack};

/// The background music of a screen; `Silence` plays none.
enum class Track : std::uint8_t { Silence, Title, City, Workshop };

/// The two volumes a player sets.
enum class Bus : std::uint8_t { Effects, Music };

/// The cue's name, as the harness reads it.
[[nodiscard]] const char *cue_name(Cue cue);
/// The track's name, as the harness reads it.
[[nodiscard]] const char *track_name(Track track);

} // namespace neocity::audio
