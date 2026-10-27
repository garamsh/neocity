use godot::classes::{Font, Node2D, Texture2D, ThemeDb};
use godot::prelude::*;
use neocity_ascent_engine::{Actor, Character, HazardKind, WorldSnapshot};

pub(crate) struct AscentDrawing {
    background: Gd<Texture2D>,
    atlas: Gd<Texture2D>,
    font: Gd<Font>,
}

pub(crate) fn color(r: u8, g: u8, b: u8, a: f32) -> Color {
    Color::from_rgba(r as f32 / 255.0, g as f32 / 255.0, b as f32 / 255.0, a)
}
pub(crate) fn rect(x: f32, y: f32, w: f32, h: f32) -> Rect2 {
    Rect2::new(Vector2::new(x, y), Vector2::new(w, h))
}

impl AscentDrawing {
    pub(crate) fn load() -> Self {
        Self {
            background: load("res://ascent/godot/assets/neon_city.png"),
            atlas: load("res://ascent/godot/assets/ascent_atlas.png"),
            font: ThemeDb::singleton()
                .get_fallback_font()
                .expect("Godot fallback font must be available"),
        }
    }

    pub(crate) fn text(
        &self,
        canvas: &mut Gd<Node2D>,
        x: f32,
        y: f32,
        text: &str,
        size: i32,
        tint: Color,
    ) {
        canvas
            .draw_string_ex(&self.font, Vector2::new(x, y), text)
            .font_size(size)
            .modulate(tint)
            .done();
    }

    pub(crate) fn sprite(
        &self,
        canvas: &mut Gd<Node2D>,
        index: usize,
        destination: Rect2,
        flip: bool,
    ) {
        // Source rectangles are measured from the untouched atlas, which is not a uniform grid.
        let bounds = [
            (88.0, 90.0, 354.0, 408.0),
            (494.0, 64.0, 397.0, 364.0),
            (990.0, 156.0, 256.0, 342.0),
            (1396.0, 129.0, 275.0, 324.0),
            (58.0, 600.0, 360.0, 228.0),
            (453.0, 567.0, 422.0, 253.0),
            (890.0, 655.0, 437.0, 127.0),
            (1458.0, 598.0, 224.0, 238.0),
        ];
        let (x, y, w, h) = bounds[index];
        let source = rect(x, y, w, h);
        let mut target = destination;
        if flip {
            target.position.x += target.size.x;
            target.size.x = -target.size.x;
        }
        canvas.draw_texture_rect_region(&self.atlas, target, source);
    }

    fn actor(&self, canvas: &mut Gd<Node2D>, actor: Actor, camera: f32, leader: bool, time: f32) {
        let index = match actor.character {
            Character::Chef => 0,
            Character::Detective => 2,
        } + usize::from(actor.airborne);
        let bob = if actor.airborne {
            0.0
        } else {
            (time * 12.0).sin().abs() * 1.5
        };
        let x = actor.feet.x;
        let y = actor.feet.y - camera;
        canvas.draw_circle(Vector2::new(x, y - 3.0), 18.0, color(4, 8, 25, 0.5));
        self.sprite(
            canvas,
            index,
            rect(x - 31.0, y - 74.0 - bob, 62.0, 74.0),
            !actor.facing_right,
        );
        if leader {
            canvas.draw_circle(
                Vector2::new(x, y - 88.0 - bob),
                2.5,
                color(105, 251, 231, 1.0),
            );
        }
    }

    pub(crate) fn paint(&self, canvas: &mut Gd<Node2D>, world: &WorldSnapshot, camera: f32) {
        let white = color(228, 234, 255, 1.0);
        let muted = color(144, 159, 195, 1.0);
        let cyan = color(91, 244, 219, 1.0);
        let gold = color(255, 192, 83, 1.0);
        canvas.draw_rect(rect(0.0, 0.0, 1280.0, 720.0), color(7, 10, 30, 1.0));
        canvas.draw_texture_rect(
            &self.background,
            rect(-55.0, -90.0 - world.progress * 70.0, 1390.0, 920.0),
            false,
        );
        canvas.draw_rect(rect(0.0, 0.0, 1280.0, 720.0), color(8, 10, 32, 0.17));
        // Near structures scroll independently from the distant city texture.
        for (i, x) in [28.0, 1228.0].into_iter().enumerate() {
            canvas.draw_rect(rect(x, 0.0, 25.0, 720.0), color(9, 17, 34, 0.94));
            canvas.draw_rect(rect(x + 4.0, 0.0, 3.0, 720.0), color(43, 65, 85, 0.8));
            for n in 0..10 {
                let y = (n as f32 * 110.0 - camera * 0.7).rem_euclid(820.0) - 60.0;
                canvas.draw_line(
                    Vector2::new(x, y),
                    Vector2::new(x + 25.0, y + 70.0),
                    color(32, 47, 69, 1.0),
                );
                if (n + i) % 3 == 0 {
                    canvas.draw_rect(rect(x - 3.0, y + 12.0, 9.0, 18.0), color(249, 64, 100, 0.9));
                }
            }
        }
        for (i, p) in world.surfaces.iter().enumerate() {
            let y = p.y - camera;
            if !(-100.0..820.0).contains(&y) {
                continue;
            }
            let x = p.x;
            canvas.draw_rect(
                rect(x + 5.0, y + 22.0, p.width - 10.0, 8.0),
                color(3, 7, 17, 0.9),
            );
            canvas.draw_rect(rect(x + 18.0, y + 27.0, 9.0, 110.0), color(13, 24, 43, 1.0));
            canvas.draw_rect(
                rect(x + p.width - 27.0, y + 27.0, 9.0, 80.0),
                color(13, 24, 43, 1.0),
            );
            canvas
                .draw_line_ex(
                    Vector2::new(x + 22.0, y + 105.0),
                    Vector2::new(x + p.width - 22.0, y + 30.0),
                    color(34, 49, 68, 1.0),
                )
                .width(5.0)
                .done();
            // The atlas module is decorative; its bright flat rim coincides with the collision top.
            self.sprite(canvas, 6, rect(x, y - 1.0, p.width, 36.0), false);
            canvas.draw_rect(rect(x, y, p.width, 3.0), if p.moving { cyan } else { gold });
            if p.moving {
                canvas.draw_line(
                    Vector2::new(x + 30.0, 0.0),
                    Vector2::new(x + 30.0, y),
                    color(57, 65, 88, 0.8),
                );
                canvas.draw_line(
                    Vector2::new(x + p.width - 30.0, 0.0),
                    Vector2::new(x + p.width - 30.0, y),
                    color(57, 65, 88, 0.8),
                );
            }
            if i == 0 {
                self.text(canvas, x + 24.0, y + 45.0, "MARKET SERVICE / 01", 12, muted);
            }
        }
        let cp = world.checkpoint;
        let cy = cp.y - camera;
        if (-80.0..800.0).contains(&cy) {
            canvas.draw_rect(
                rect(cp.x - 5.0, cy - 42.0, 10.0, 42.0),
                color(21, 42, 62, 1.0),
            );
            canvas.draw_rect(rect(cp.x - 10.0, cy - 50.0, 20.0, 18.0), cyan);
            canvas.draw_circle(
                Vector2::new(cp.x, cy - 42.0),
                30.0,
                color(57, 240, 229, 0.09),
            );
            self.text(
                canvas,
                cp.x + 24.0,
                cy - 29.0,
                if world.checkpoint_reached {
                    "CHECKPOINT SAVED"
                } else {
                    "SAFE LANDING"
                },
                12,
                cyan,
            );
        }
        for h in &world.hazards {
            let y = h.center.y - camera;
            if !(-80.0..800.0).contains(&y) {
                continue;
            }
            match h.kind {
                HazardKind::Patrol => self.sprite(
                    canvas,
                    4,
                    rect(h.center.x - 29.0, y - 20.0, 58.0, 40.0),
                    false,
                ),
                HazardKind::Flyer => {
                    let flutter = (world.elapsed * 16.0).sin() * 6.0;
                    self.sprite(
                        canvas,
                        5,
                        rect(h.center.x - 40.0, y - 25.0 + flutter, 80.0, 50.0),
                        false,
                    );
                }
                HazardKind::Projectile => {
                    if h.warning {
                        canvas.draw_circle(
                            Vector2::new(1130.0, y),
                            18.0,
                            color(251, 115, 117, 0.35),
                        );
                        self.text(canvas, 1124.0, y + 5.0, "!", 20, gold);
                    } else if h.active {
                        canvas
                            .draw_line_ex(
                                Vector2::new(h.center.x + 20.0, y),
                                Vector2::new(h.center.x, y),
                                color(165, 255, 106, 0.5),
                            )
                            .width(5.0)
                            .done();
                        canvas.draw_circle(
                            Vector2::new(h.center.x, y),
                            7.0,
                            color(177, 255, 107, 1.0),
                        );
                    }
                }
            }
        }
        let goal = world.ingredient;
        let gy = goal.y - camera;
        if (-100.0..820.0).contains(&gy) {
            canvas.draw_circle(
                Vector2::new(goal.x, gy - 30.0),
                45.0,
                color(255, 176, 63, 0.08),
            );
            self.sprite(
                canvas,
                7,
                rect(
                    goal.x - 25.0,
                    gy - 58.0 + (world.elapsed * 2.0).sin() * 4.0,
                    50.0,
                    53.0,
                ),
                false,
            );
            self.text(
                canvas,
                goal.x - 65.0,
                gy - 85.0,
                "THE LAST INGREDIENT",
                12,
                gold,
            );
        }
        self.actor(canvas, world.companion, camera, false, world.elapsed);
        self.actor(canvas, world.leader, camera, true, world.elapsed);
        if world.invulnerable {
            canvas
                .draw_circle_ex(
                    Vector2::new(world.leader.feet.x, world.leader.feet.y - camera - 28.0),
                    38.0,
                    color(91, 244, 219, 0.3),
                )
                .filled(false)
                .width(1.5)
                .done();
        }
        self.hud(canvas, world, white, muted, cyan, gold);
    }
}
