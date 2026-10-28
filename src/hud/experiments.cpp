#include "experiments.hpp"

namespace neocity::hud {

void paint_menu(godot::Node2D &canvas,
                const rendering::AscentDrawing &drawing) {
  const auto colors = rendering::palette();
  canvas.draw_rect(rendering::rect(0.0F, 0.0F, 1280.0F, 720.0F),
                   rendering::color(19, 23, 30, 1.0F));
  drawing.text(canvas, 130.0F, 120.0F, "PAN EXPERIMENTS", 42, colors.white);
  drawing.text(canvas, 130.0F, 167.0F,
               "Two control ideas. One window. No final game selected.", 22,
               colors.muted);
  drawing.text(canvas, 150.0F, 270.0F, "1   DIRECT DODGE", 30, colors.cyan);
  drawing.text(canvas, 210.0F, 310.0F,
               "Move the blue player past fixed hazards to the green exit.", 22,
               colors.white);
  drawing.text(canvas, 150.0F, 400.0F, "2   TILT ESCAPE", 30, colors.gold);
  drawing.text(canvas, 210.0F, 440.0F,
               "Tilt the pan: you and the red hazards slide together.", 22,
               colors.white);
  drawing.text(canvas, 150.0F, 530.0F, "3   ASCENT REFERENCE", 26,
               colors.white);
  drawing.text(canvas, 210.0F, 568.0F,
               "The existing city climb. Original controls and art.", 22,
               colors.muted);
  drawing.text(canvas, 130.0F, 658.0F,
               "Press 1, 2, or 3 to begin.   M returns here from any run.", 22,
               colors.white);
}

void paint_experiment(godot::Node2D &canvas,
                      const rendering::AscentDrawing &drawing, bool tilt,
                      const godot::String &outcome) {
  const auto colors = rendering::palette();
  drawing.text(canvas, 70.0F, 65.0F,
               tilt ? "2 / TILT ESCAPE" : "1 / DIRECT DODGE", 34, colors.white);
  drawing.text(canvas, 378.0F, 110.0F, "EXIT", 22, colors.cyan);
  drawing.text(canvas, 790.0F, 170.0F, "WASD / ARROW KEYS", 25, colors.white);
  drawing.text(canvas, 790.0F, 214.0F,
               tilt ? "Tilt the pan, not the player."
                    : "Move the blue circle directly.",
               21, colors.white);
  drawing.text(canvas, 790.0F, 254.0F,
               tilt ? "All circles slide with gravity."
                    : "Red circles stay in place.",
               21, colors.muted);
  drawing.text(canvas, 790.0F, 294.0F,
               tilt ? "Reverse tilt early to brake." : "Release keys to stop.",
               21, colors.muted);
  drawing.text(canvas, 790.0F, 350.0F, "Avoid red. Escape through green.", 21,
               colors.white);
  drawing.text(canvas, 790.0F, 390.0F, "R: fresh run    M: menu", 23,
               colors.cyan);
  if (tilt) {
    drawing.text(canvas, 790.0F, 500.0F, "PAN TILT", 21, colors.gold);
    drawing.text(canvas, 790.0F, 590.0F,
                 "Release levels the pan; inertia remains.", 19, colors.muted);
  }
  const bool finished = outcome != "playing";
  godot::String status = "Blue: player    Red X: hazard    Green: escape";
  if (finished) {
    status = outcome == "escaped" ? "ESCAPED - R to replay / M for menu"
                                  : "HIT A HAZARD - R to retry / M for menu";
  }
  drawing.text(canvas, 70.0F, 700.0F, status, 23,
               finished ? colors.gold : colors.white);
}

} // namespace neocity::hud
