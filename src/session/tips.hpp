#pragma once

#include "../hud/survival.hpp"

#include <optional>
#include <vector>

namespace neocity::session {

/// The first-play tips of one run. Each opens once, the first time the run
/// shows its concept, one at a time, and never holds the run: it closes on a
/// click, on the next action a human takes, or once it has shown a while.
class Tips {
public:
  /// Closes the open tip when `acted` or once it has shown long enough,
  /// counting `delta` seconds of play, then opens the first tip `frame`
  /// shows the concept of that has not opened yet.
  void advance(const hud::SurvivalFrame &frame, double delta, bool acted);
  /// The tip on the screen, if any.
  [[nodiscard]] std::optional<hud::Tip> open() const;
  /// The tips opened so far, in the order they opened.
  [[nodiscard]] const std::vector<hud::Tip> &opened() const;

private:
  std::optional<hud::Tip> current;
  /// How long `current` has shown, in seconds of play.
  double shown_for = 0.0;
  std::vector<hud::Tip> seen;
};

} // namespace neocity::session
