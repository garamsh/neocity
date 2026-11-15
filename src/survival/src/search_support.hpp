#pragma once

#include <survival/exploration.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <vector>

namespace neocity::survival {

/// A container of `count` slots, all full of `item`.
inline SourceStock stock(ObjectId id, std::uint32_t count,
                         IngredientId item = IngredientId{70}) {
  return {.id = id,
          .capacity = count,
          .items = std::vector<IngredientId>(count, item)};
}

/// Opens `source`, takes its first item and closes it again: the old single
/// act of collecting. `MissingItem` when the container is empty.
inline CollectionResult collect(Exploration &exploration, Actor actor,
                                ObjectId source) {
  const auto opened = exploration.search(actor, source);
  if (opened != CollectionResult::Applied) {
    return opened;
  }
  const auto stocks = exploration.snapshot().sources;
  const auto found = std::ranges::find(stocks, source, &SourceStock::id);
  const auto taken =
      found->items.empty()
          ? CollectionResult::MissingItem
          : exploration.take(actor, source, found->items.front());
  EXPECT_EQ(exploration.close_search(actor), CollectionResult::Applied);
  return taken;
}

} // namespace neocity::survival
