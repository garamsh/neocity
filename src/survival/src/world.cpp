#include <survival/world.hpp>

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <stdexcept>

namespace neocity::survival {
namespace {

bool is_passage(ObjectKind kind) {
  switch (kind) {
  case ObjectKind::Ladder:
  case ObjectKind::Entrance:
    return true;
  case ObjectKind::Npc:
  case ObjectKind::Terrain:
  case ObjectKind::Building:
  case ObjectKind::MaterialSource:
    return false;
  }
  std::abort();
}

} // namespace

World::World(const std::vector<ObjectDefinition> &definitions) {
  std::vector<ObjectId> ids;
  for (const auto &definition : definitions) {
    if (std::ranges::find(ids, definition.id) != ids.end() ||
        definition.area.width <= 0 || definition.area.height <= 0 ||
        definition.exposed_from_day == 0U ||
        definition.destroyed_from_day == 0U ||
        (definition.ingredient.has_value() !=
         (definition.kind == ObjectKind::MaterialSource)) ||
        (definition.supported_by &&
         definition.kind != ObjectKind::MaterialSource)) {
      throw std::invalid_argument("invalid world object definition");
    }
    ids.push_back(definition.id);
    if (definition.supported_by &&
        !std::ranges::any_of(definitions, [&](const ObjectDefinition &owner) {
          return owner.id == *definition.supported_by &&
                 (owner.kind == ObjectKind::Npc ||
                  owner.kind == ObjectKind::Building ||
                  owner.kind == ObjectKind::Terrain);
        })) {
      throw std::invalid_argument("material source requires an existing owner");
    }
  }
  for (const auto &definition : definitions) {
    const auto state = definition.exposed_from_day ? ObjectState::Dormant
                                                   : ObjectState::Intact;
    objects.push_back({.definition = definition, .state = state});
  }
}

DamageStage damage_stage(std::uint32_t day) {
  if (day >= STAGE_FIRST_DAY.at(2)) {
    return DamageStage::Ruined;
  }
  return day >= STAGE_FIRST_DAY.at(1) ? DamageStage::Cracked
                                      : DamageStage::Standing;
}

WorldSnapshot on_map(const WorldSnapshot &snapshot, MapId map) {
  WorldSnapshot result{
      .day = snapshot.day, .stage = snapshot.stage, .objects = {}};
  std::ranges::copy_if(
      snapshot.objects, std::back_inserter(result.objects),
      [map](const ObjectSnapshot &object) { return object.map == map; });
  return result;
}

WorldUpdateResult World::update(std::uint32_t next) {
  if (next < day) {
    return WorldUpdateResult::DecreasingDay;
  }
  day = next;
  for (auto &object : objects) {
    if (object.state == ObjectState::Dormant &&
        object.definition.exposed_from_day &&
        day >= *object.definition.exposed_from_day) {
      object.state = ObjectState::Intact;
    }
    if (object.state == ObjectState::Intact &&
        object.definition.destroyed_from_day &&
        day >= *object.definition.destroyed_from_day) {
      object.state = ObjectState::Destroyed;
    }
  }
  return WorldUpdateResult::Applied;
}

WorldSnapshot World::snapshot() const {
  WorldSnapshot result{.day = day, .stage = damage_stage(day), .objects = {}};
  for (const auto &object : objects) {
    const auto &definition = object.definition;
    const bool intact = object.state == ObjectState::Intact;
    const bool supported =
        !definition.supported_by ||
        std::ranges::any_of(objects, [&](const Object &owner) {
          return owner.definition.id == *definition.supported_by &&
                 owner.state == ObjectState::Intact;
        });
    result.objects.push_back(
        {.id = definition.id,
         .kind = definition.kind,
         .area = definition.area,
         .map = definition.map,
         .state = object.state,
         .traversable = intact && (is_passage(definition.kind) ||
                                   definition.kind == ObjectKind::Terrain),
         .material_available =
             intact && supported && definition.ingredient.has_value(),
         .ingredient = definition.ingredient});
  }
  return result;
}

} // namespace neocity::survival
