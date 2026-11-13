#include "sound.hpp"

#include <cstdlib>

namespace neocity::audio {

const char *cue_name(Cue cue) {
  switch (cue) {
  case Cue::Step:
    return "step";
  case Cue::Jump:
    return "jump";
  case Cue::Land:
    return "land";
  case Cue::Collect:
    return "collect";
  case Cue::Fall:
    return "fall";
  case Cue::Travel:
    return "travel";
  case Cue::Identified:
    return "identified";
  case Cue::AppraisalFailed:
    return "appraisal_failed";
  case Cue::Cooked:
    return "cooked";
  case Cue::Burned:
    return "burned";
  case Cue::Satisfied:
    return "satisfied";
  case Cue::Refused:
    return "refused";
  case Cue::AcidRise:
    return "acid_rise";
  case Cue::NewDay:
    return "new_day";
  case Cue::MenuMove:
    return "menu_move";
  case Cue::MenuSelect:
    return "menu_select";
  case Cue::MenuBack:
    return "menu_back";
  }
  std::abort();
}

const char *track_name(Track track) {
  switch (track) {
  case Track::Silence:
    return "silence";
  case Track::Title:
    return "title";
  case Track::City:
    return "city";
  case Track::Workshop:
    return "workshop";
  }
  std::abort();
}

} // namespace neocity::audio
