#include "menus.hpp"

namespace neocity::hud {
namespace {

void background(godot::Node2D &canvas) {
  canvas.draw_rect(rendering::rect(0.0F, 0.0F, 1280.0F, 720.0F),
                   rendering::color(19, 23, 30, 1.0F));
}

} // namespace

void paint_main_menu(godot::Node2D &canvas,
                     const rendering::AscentDrawing &drawing) {
  const auto colors = rendering::palette();
  background(canvas);
  drawing.text(canvas, 130.0F, 140.0F, "NEOCITY", 54, colors.white);
  drawing.text(canvas, 130.0F, 190.0F, "After Hours", 26, colors.muted);
  drawing.text(canvas, 150.0F, 290.0F, "1   MONSTER MOUTH ESCAPE", 34,
               colors.cyan);
  drawing.text(canvas, 230.0F, 330.0F,
               "Survive inside the monster: gather, cook, and feed it.", 22,
               colors.white);
  drawing.text(canvas, 150.0F, 410.0F, "2   DEVELOPER", 30, colors.gold);
  drawing.text(canvas, 230.0F, 450.0F,
               "Playable experiments contributors are trying out.", 22,
               colors.muted);
  drawing.text(canvas, 150.0F, 530.0F, "3   CITY CLIMB", 30, colors.gold);
  drawing.text(canvas, 230.0F, 570.0F,
               "The earlier prototype. Original controls and art.", 22,
               colors.muted);
  drawing.text(canvas, 130.0F, 650.0F,
               "Press 1, 2 or 3.   M returns here from a game.", 22,
               colors.white);
}

void paint_developer_menu(godot::Node2D &canvas,
                          const rendering::AscentDrawing &drawing,
                          std::span<const experiment::Entry> experiments) {
  const auto colors = rendering::palette();
  background(canvas);
  drawing.text(canvas, 130.0F, 120.0F, "DEVELOPER EXPERIMENTS", 42,
               colors.gold);
  drawing.text(canvas, 130.0F, 167.0F,
               "Plain prototypes for trying ideas. None is the main game.", 22,
               colors.muted);
  float y = 270.0F;
  int number = 1;
  for (const experiment::Entry &entry : experiments) {
    drawing.text(canvas, 150.0F, y,
                 godot::String::num_int64(number) + "   " +
                     godot::String::utf8(entry.title),
                 30, colors.cyan);
    drawing.text(canvas, 210.0F, y + 40.0F, godot::String::utf8(entry.summary),
                 22, colors.white);
    y += 120.0F;
    ++number;
  }
  drawing.text(canvas, 130.0F, 658.0F,
               "Press a number to play.   M: main menu (from an experiment: "
               "back here).",
               22, colors.white);
}

} // namespace neocity::hud
