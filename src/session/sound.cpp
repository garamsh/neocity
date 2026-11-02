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
std::optional<audio::Cue> action_cue(const hud::ActionEvent &event,
                                     const survival::GameView &after) {
  if (!applied(event.result)) {
    return std::nullopt;
  }
  const bool missed = failed(event.result);
  return std::visit(
      [&](const auto &action) -> std::optional<audio::Cue> {
        using Done = std::decay_t<decltype(action)>;
        if constexpr (std::is_same_v<Done, survival::Enter> ||
                      std::is_same_v<Done, survival::ReturnToBase>) {
          return audio::Cue::Travel;
        } else if constexpr (std::is_same_v<Done, survival::Collect>) {
          return audio::Cue::Collect;
        } else if constexpr (std::is_same_v<Done, survival::Repair> ||
                             std::is_same_v<Done, survival::ReplaceTool>) {
          return audio::Cue::Repair;
        } else if constexpr (std::is_same_v<Done, survival::Appraise>) {
          return missed ? audio::Cue::AppraisalFailed : audio::Cue::Identified;
        } else if constexpr (std::is_same_v<Done, survival::Cook>) {
          return missed ? audio::Cue::Burned : audio::Cue::Cooked;
        } else if constexpr (std::is_same_v<Done, survival::Feed>) {
          const auto &day = after.workshop.last_day;
          return day && day->reaction == survival::MonsterReaction::Satisfied
                     ? audio::Cue::Satisfied
                     : audio::Cue::Refused;
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
    found.push_back({.cue = audio::Cue::Jump, .at = now.position});
  }
  if (then.stance == survival::Stance::Airborne &&
      now.stance == survival::Stance::Standing) {
    found.push_back({.cue = audio::Cue::Land, .at = now.position});
    stride = 0;
  }
  if (then.stance == survival::Stance::Standing &&
      now.stance == survival::Stance::Standing) {
    stride += std::abs(now.position.x - then.position.x);
    if (stride >= FOOTSTEP) {
      stride %= FOOTSTEP;
      found.push_back({.cue = audio::Cue::Step, .at = now.position});
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
      found.push_back({.cue = audio::Cue::Hurt,
                       .at = explorer(before, hazard->actor).body.position});
    } else if (const auto *fall = std::get_if<hud::FallEvent>(&event)) {
      found.push_back({.cue = audio::Cue::Fall,
                       .at = explorer(before, fall->actor).body.position});
    }
  }
  if (after.progression.acid.value > before.progression.acid.value) {
    found.push_back({.cue = audio::Cue::AcidRise, .at = std::nullopt});
  }
  if (after.progression.day > before.progression.day) {
    found.push_back({.cue = audio::Cue::NewDay, .at = std::nullopt});
  }
  return found;
}

std::optional<rendering::Flourish> flourish_of(audio::Cue cue) {
  switch (cue) {
  case audio::Cue::Collect:
    return rendering::Flourish::Pop;
  case audio::Cue::Repair:
    return rendering::Flourish::Spark;
  case audio::Cue::Hurt:
    return rendering::Flourish::Hit;
  case audio::Cue::Fall:
    return rendering::Flourish::Splash;
  case audio::Cue::AcidRise:
    return rendering::Flourish::Surge;
  case audio::Cue::Cooked:
  case audio::Cue::Burned:
    return rendering::Flourish::Steam;
  case audio::Cue::Satisfied:
    return rendering::Flourish::Delight;
  case audio::Cue::Refused:
    return rendering::Flourish::Disgust;
  case audio::Cue::NewDay:
    return rendering::Flourish::NewDay;
  case audio::Cue::Step:
  case audio::Cue::Jump:
  case audio::Cue::Land:
  case audio::Cue::Travel:
  case audio::Cue::Identified:
  case audio::Cue::AppraisalFailed:
  case audio::Cue::MenuMove:
  case audio::Cue::MenuSelect:
  case audio::Cue::MenuBack:
    return std::nullopt;
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
                         .marked = volume.marked == audio::Bus::Effects},
          hud::VolumeRow{.name = "Music",
                         .level = volume.music,
                         .steps = VOLUME_STEPS,
                         .marked = volume.marked == audio::Bus::Music}};
}

} // namespace neocity::session
