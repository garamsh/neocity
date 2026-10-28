#pragma once

#include <gameplay/world_snapshot.hpp>

#include <cstddef>
#include <optional>
#include <span>

namespace neocity::gameplay {

inline constexpr float HALF_WIDTH = 14.0F;
inline constexpr float BODY_HEIGHT = 42.0F;

/// Returns the highest one-way surface crossed while descending from
/// `previous` to `next`.
std::optional<std::size_t> landing(Point previous, Point next,
                                   std::span<const Surface> surfaces);

/// Whether the party's body, standing on `feet`, overlaps an active hazard.
bool touches(Point feet, const Hazard &hazard);

} // namespace neocity::gameplay
