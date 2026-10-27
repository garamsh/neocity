use crate::drawing::{AscentDrawing, color, rect};
use godot::classes::Node2D;
use godot::prelude::*;
use neocity_ascent_engine::{Character, RunPhase, WorldSnapshot};

impl AscentDrawing {
    pub(crate) fn hud(
        &self,
        canvas: &mut Gd<Node2D>,
        world: &WorldSnapshot,
        white: Color,
        muted: Color,
        cyan: Color,
        gold: Color,
    ) {
        canvas.draw_rect(rect(0.0, 0.0, 1280.0, 98.0), color(5, 8, 23, 0.92));
        canvas.draw_rect(rect(0.0, 97.0, 1280.0, 1.0), color(69, 81, 116, 0.6));
        self.text(canvas, 30.0, 32.0, "NEOCITY", 22, white);
        self.text(
            canvas,
            31.0,
            54.0,
            "AFTER HOURS / INGREDIENT RUN",
            10,
            muted,
        );
        self.text(canvas, 31.0, 79.0, "01  THE SERVICE TOWERS", 12, gold);
        for (i, character) in [Character::Chef, Character::Detective]
            .into_iter()
            .enumerate()
        {
            let x = 352.0 + i as f32 * 136.0;
            let active = world.leader.character == character;
            canvas.draw_rect(
                rect(x, 16.0, 126.0, 62.0),
                if active {
                    color(23, 54, 67, 1.0)
                } else {
                    color(13, 21, 40, 1.0)
                },
            );
            canvas.draw_rect(
                rect(x, 76.0, 126.0, 2.0),
                if active { cyan } else { color(47, 57, 82, 1.0) },
            );
            self.sprite(
                canvas,
                if i == 0 { 0 } else { 2 },
                rect(x - 5.0, 7.0, 62.0, 75.0),
                false,
            );
            self.text(
                canvas,
                x + 54.0,
                43.0,
                if i == 0 { "CHEF" } else { "DETECTIVE" },
                10,
                white,
            );
            self.text(
                canvas,
                x + 54.0,
                61.0,
                if active { "LEADING" } else { "PARTNER" },
                9,
                if active { cyan } else { muted },
            );
        }
        self.text(canvas, 708.0, 34.0, "ASCENT", 10, muted);
        canvas.draw_rect(rect(708.0, 47.0, 230.0, 5.0), color(29, 40, 65, 1.0));
        canvas.draw_rect(rect(708.0, 47.0, 230.0 * world.progress, 5.0), cyan);
        self.text(
            canvas,
            708.0,
            74.0,
            &format!(
                "{:02}%   /   {} RETRIES",
                (world.progress * 100.0) as u32,
                world.retries
            ),
            11,
            white,
        );
        self.text(
            canvas,
            1050.0,
            43.0,
            &format!(
                "{:02}:{:02}",
                world.elapsed as u32 / 60,
                world.elapsed as u32 % 60
            ),
            28,
            white,
        );
        self.text(
            canvas,
            1052.0,
            68.0,
            if world.phase == RunPhase::Complete {
                "SPICE FRUIT  /  1 OF 1"
            } else {
                "SPICE FRUIT  /  0 OF 1"
            },
            10,
            gold,
        );
        canvas.draw_rect(rect(280.0, 670.0, 720.0, 36.0), color(5, 9, 25, 0.88));
        self.text(
            canvas,
            302.0,
            693.0,
            "A D / ARROWS  MOVE     SPACE  JUMP     TAB  LEAD     R  RETRY     ESC  PAUSE",
            12,
            white,
        );
        if world.elapsed < 9.0 && world.progress < 0.05 {
            canvas.draw_rect(rect(385.0, 132.0, 510.0, 75.0), color(8, 15, 34, 0.92));
            self.text(
                canvas,
                410.0,
                160.0,
                "A dangerous shortcut. An exceptional dinner.",
                18,
                white,
            );
            self.text(
                canvas,
                410.0,
                185.0,
                "Climb the towers. Find the glowing spice fruit.",
                13,
                muted,
            );
        }
        if world.phase != RunPhase::Playing {
            canvas.draw_rect(rect(0.0, 98.0, 1280.0, 622.0), color(3, 5, 19, 0.72));
            canvas.draw_rect(rect(360.0, 240.0, 560.0, 230.0), color(10, 19, 39, 0.98));
            canvas.draw_rect(rect(360.0, 240.0, 560.0, 3.0), cyan);
            let complete = world.phase == RunPhase::Complete;
            self.text(
                canvas,
                405.0,
                298.0,
                if complete {
                    "THE INGREDIENT IS OURS."
                } else {
                    "TAKE A BREATH."
                },
                27,
                white,
            );
            self.text(
                canvas,
                405.0,
                340.0,
                if complete {
                    "Now, let's see what the chef can do."
                } else {
                    "The city can wait a moment."
                },
                17,
                muted,
            );
            self.text(
                canvas,
                405.0,
                399.0,
                if complete {
                    "R  ANOTHER RUN"
                } else {
                    "ESC  CONTINUE    /    R  CHECKPOINT"
                },
                16,
                gold,
            );
        }
    }
}
