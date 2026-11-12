#pragma once

namespace neocity::rendering {

/// The logical canvas, whatever the window's size: the window shows it at its
/// real output resolution, letterboxed where the aspect ratio differs.
inline constexpr float CANVAS_WIDTH = 1920.0F;
inline constexpr float CANVAS_HEIGHT = 1080.0F;

/// How many canvas pixels a pixel of the art laid out at 1280x720 covers,
/// until the art is redrawn for the canvas.
inline constexpr float ART_SCALE = 1.5F;

/// The screen the existing layouts and art are written in, in the units the
/// scene draws: the canvas over `ART_SCALE`.
inline constexpr float SCREEN_WIDTH = CANVAS_WIDTH / ART_SCALE;
inline constexpr float SCREEN_HEIGHT = CANVAS_HEIGHT / ART_SCALE;

} // namespace neocity::rendering
