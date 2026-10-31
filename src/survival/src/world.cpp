#include <survival/world.hpp>

#include <algorithm>
#include <cstdlib>
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

bool valid_threshold(const std::optional<WorldThreshold> &threshold) {
  if (!threshold) {
    return true;
  }
  return (threshold->elapsed || threshold->acid) &&
         (!threshold->elapsed ||
          *threshold->elapsed > std::chrono::microseconds::zero()) &&
         (!threshold->acid || threshold->acid->value > 0);
}

bool reached(const WorldThreshold &threshold, WorldProgression progression) {
  return (threshold.elapsed && progression.elapsed >= *threshold.elapsed) ||
         (threshold.acid && progression.acid.value >= threshold.acid->value);
}

} // namespace

World::World(const std::vector<ObjectDefinition> &definitions) {
  std::vector<ObjectId> ids;
  for (const auto &definition : definitions) {
    if (std::ranges::find(ids, definition.id) != ids.end() ||
        definition.area.width <= 0 || definition.area.height <= 0 ||
        !valid_threshold(definition.exposed_at) ||
        !valid_threshold(definition.destroyed_after) ||
        (definition.repair_with && !is_passage(definition.kind)) ||
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
    const auto state =
        definition.exposed_at ? ObjectState::Dormant : ObjectState::Intact;
    objects.push_back(
        {.definition = definition, .state = state, .repaired_at = {}});
  }
}

WorldUpdateResult World::update(WorldProgression progression) {
  if (progression.elapsed < current.elapsed) {
    return WorldUpdateResult::DecreasingTime;
  }
  current = progression;
  peak_acid.value = std::max(peak_acid.value, current.acid.value);
  for (auto &object : objects) {
    if (object.state == ObjectState::Dormant && object.definition.exposed_at &&
        reached(*object.definition.exposed_at,
                {.elapsed = current.elapsed, .acid = peak_acid})) {
      object.state = ObjectState::Intact;
    }
    if (object.state == ObjectState::Intact &&
        object.definition.destroyed_after &&
        reached(*object.definition.destroyed_after,
                {.elapsed = current.elapsed - object.repaired_at.elapsed,
                 .acid = {peak_acid.value - object.repaired_at.acid.value}})) {
      object.state = ObjectState::Destroyed;
    }
  }
  return WorldUpdateResult::Applied;
}

RepairResult World::repair(ObjectId id, ToolId tool) {
  const auto object = std::ranges::find_if(
      objects, [id](const Object &entry) { return entry.definition.id == id; });
  if (object == objects.end()) {
    return RepairResult::UnknownObject;
  }
  if (object->state != ObjectState::Destroyed) {
    return RepairResult::NotDamaged;
  }
  if (!object->definition.repair_with) {
    return RepairResult::NotRepairable;
  }
  if (tool != *object->definition.repair_with) {
    return RepairResult::WrongTool;
  }
  object->state = ObjectState::Intact;
  object->repaired_at = {.elapsed = current.elapsed, .acid = peak_acid};
  return RepairResult::Applied;
}

WorldSnapshot World::snapshot() const {
  WorldSnapshot result{
      .progression = current, .peak_acid = peak_acid, .objects = {}};
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
         .state = object.state,
         .traversable = intact && (is_passage(definition.kind) ||
                                   definition.kind == ObjectKind::Terrain),
         .material_available = intact && supported && definition.ingredient,
         .ingredient = definition.ingredient,
         .repair_with = definition.repair_with});
  }
  return result;
}

} // namespace neocity::survival
