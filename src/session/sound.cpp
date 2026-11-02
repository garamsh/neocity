#include "sound.hpp"

#include <cstdlib>
#include <type_traits>
#include <variant>

namespace neocity::session {
namespace {

using survival::Actor;

/// Sub-cells walked between two footsteps: a cell and a half.
constexpr std::int32_t FOOTSTEP = survival::SUBCELLS * 3 / 2;

const survival::ExplorerSnapshot &explorer(const survival::GameView &view,
                                           Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return view.exploration.chef;
  case Actor::Appraiser:
    return view.exploration.appraiser;
  }
  std::abort();
}

survival::Location location(const survival::GameView &view, Actor actor) {
  switch (actor) {
  case Actor::Chef:
    return view.progression.chef.location;
  case Actor::Appraiser:
    return view.progression.appraiser.location;
  }
  std::abort();
}

bool applied(const survival::ActionResult &result) {
  return std::visit(
      [](auto answer) {
        using Answer = decltype(answer);
        if constexpr (std::is_same_v<Answer, survival::Result>) {
          return answer == survival::Result::Applied;
        } else if constexpr (std::is_same_v<Answer,
                                            survival::CollectionResult>) {
          return answer == survival::CollectionResult::Applied;
        } else if constexpr (std::is_same_v<Answer, survival::RepairResult>) {
          return answer == survival::RepairResult::Applied;
        } else {
          return answer == survival::WorkshopResult::Applied ||
                 answer == survival::WorkshopResult::Failed;
        }
      },
      result);
}

bool failed(const survival::ActionResult &result) {
  const auto *workshop = std::get_if<survival::WorkshopResult>(&result);
  return workshop != nullptr && *workshop == survival::WorkshopResult::Failed;
}

/// The cue of a command the rules carried out; refusals have none.
std::optional<Cue> action_cue(const hud::ActionEvent &event,
                              const survival::GameView &after) {
  if (!applied(event.result)) {
    return std::nullopt;
  }
  const bool missed = failed(event.result);
  return std::visit(
      [&](const auto &action) -> std::optional<Cue> {
        using Done = std::decay_t<decltype(action)>;
        if constexpr (std::is_same_v<Done, survival::Enter> ||
                      std::is_same_v<Done, survival::ReturnToBase>) {
          return Cue::Travel;
        } else if constexpr (std::is_same_v<Done, survival::Collect>) {
          return Cue::Collect;
        } else if constexpr (std::is_same_v<Done, survival::Repair> ||
                             std::is_same_v<Done, survival::ReplaceTool>) {
          return Cue::Repair;
        } else if constexpr (std::is_same_v<Done, survival::Appraise>) {
          return missed ? Cue::AppraisalFailed : Cue::Identified;
        } else if constexpr (std::is_same_v<Done, survival::Cook>) {
          return missed ? Cue::Burned : Cue::Cooked;
        } else if constexpr (std::is_same_v<Done, survival::Feed>) {
          const auto &day = after.workshop.last_day;
          return day && day->reaction == survival::MonsterReaction::Satisfied
                     ? Cue::Satisfied
                     : Cue::Refused;
        } else {
          return std::nullopt;
        }
      },
      event.action);
}

/// A jump, a landing, or a footstep of one actor in the city.
void movement_moments(const survival::GameView &before,
                      const survival::GameView &after, Actor actor,
                      std::int32_t &stride, std::vector<Moment> &found) {
  if (location(before, actor) != survival::Location::City ||
      location(after, actor) != survival::Location::City) {
    stride = 0;
    return;
  }
  const auto &then = explorer(before, actor).body;
  const auto &now = explorer(after, actor).body;
  if (then.stance != survival::Stance::Airborne &&
      now.stance == survival::Stance::Airborne && now.fall_speed < 0) {
    found.push_back({.cue = Cue::Jump, .at = now.position});
  }
  if (then.stance == survival::Stance::Airborne &&
      now.stance == survival::Stance::Standing) {
    found.push_back({.cue = Cue::Land, .at = now.position});
    stride = 0;
  }
  if (then.stance == survival::Stance::Standing &&
      now.stance == survival::Stance::Standing) {
    stride += std::abs(now.position.x - then.position.x);
    if (stride >= FOOTSTEP) {
      stride %= FOOTSTEP;
      found.push_back({.cue = Cue::Step, .at = now.position});
    }
  }
}

} // namespace

std::vector<Moment> moments(const survival::GameView &before,
                            const survival::GameView &after,
                            const std::vector<hud::Event> &events,
                            Strides &strides) {
  std::vector<Moment> found;
  movement_moments(before, after, Actor::Chef, strides.at(0), found);
  movement_moments(before, after, Actor::Appraiser, strides.at(1), found);
  for (const auto &event : events) {
    if (const auto *action = std::get_if<hud::ActionEvent>(&event)) {
      if (const auto cue = action_cue(*action, after)) {
        found.push_back(
            {.cue = *cue, .at = explorer(after, action->actor).body.position});
      }
    } else if (const auto *hazard = std::get_if<hud::HazardEvent>(&event)) {
      found.push_back({.cue = Cue::Hurt,
                       .at = explorer(before, hazard->actor).body.position});
    } else if (const auto *fall = std::get_if<hud::FallEvent>(&event)) {
      found.push_back({.cue = Cue::Fall,
                       .at = explorer(before, fall->actor).body.position});
    }
  }
  if (after.progression.acid.value > before.progression.acid.value) {
    found.push_back({.cue = Cue::AcidRise, .at = std::nullopt});
  }
  if (after.progression.day > before.progression.day) {
    found.push_back({.cue = Cue::NewDay, .at = std::nullopt});
  }
  return found;
}

std::optional<rendering::Flourish> flourish_of(Cue cue) {
  switch (cue) {
  case Cue::Collect:
    return rendering::Flourish::Pop;
  case Cue::Repair:
    return rendering::Flourish::Spark;
  case Cue::Hurt:
    return rendering::Flourish::Hit;
  case Cue::Fall:
    return rendering::Flourish::Splash;
  case Cue::AcidRise:
    return rendering::Flourish::Surge;
  case Cue::Cooked:
  case Cue::Burned:
    return rendering::Flourish::Steam;
  case Cue::Satisfied:
    return rendering::Flourish::Delight;
  case Cue::Refused:
    return rendering::Flourish::Disgust;
  case Cue::NewDay:
    return rendering::Flourish::NewDay;
  case Cue::Step:
  case Cue::Jump:
  case Cue::Land:
  case Cue::Travel:
  case Cue::Identified:
  case Cue::AppraisalFailed:
  case Cue::MenuMove:
  case Cue::MenuSelect:
  case Cue::MenuBack:
    return std::nullopt;
  }
  std::abort();
}

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
  case Cue::Repair:
    return "repair";
  case Cue::Hurt:
    return "hurt";
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

float loudness(std::uint8_t level) {
  return static_cast<float>(level) / static_cast<float>(VOLUME_STEPS);
}

std::array<hud::VolumeRow, 2> volume_rows(const Volume &volume) {
  return {hud::VolumeRow{.name = "Effects",
                         .level = volume.effects,
                         .steps = VOLUME_STEPS,
                         .marked = volume.marked == Bus::Effects},
          hud::VolumeRow{.name = "Music",
                         .level = volume.music,
                         .steps = VOLUME_STEPS,
                         .marked = volume.marked == Bus::Music}};
}

} // namespace neocity::session
