use crate::collision::{landing, touches};
use crate::course::{CHECKPOINT_INDEX, GOAL, START, STEP, WIDTH, hazards, surfaces};
use crate::{Actor, Character, Point, RunPhase, WorldSnapshot};
use std::collections::VecDeque;

/// Platform-independent input sampled once per fixed simulation tick.
#[derive(Clone, Copy, Debug, Default)]
pub struct PlayerCommand {
    pub horizontal: f32,
    pub jump_pressed: bool,
    pub jump_held: bool,
    pub switch_character: bool,
    pub pause_pressed: bool,
    pub retry_pressed: bool,
}

/// Owns the expedition rules; no Godot, clock, filesystem, or rendering dependency.
pub struct AscentEngine {
    feet: Point,
    velocity: Point,
    leader: Character,
    facing_right: bool,
    grounded: Option<usize>,
    coyote: f32,
    jump_buffer: f32,
    time: f32,
    elapsed: f32,
    checkpoint: bool,
    retries: u32,
    invulnerability: f32,
    highest: f32,
    phase: RunPhase,
    trail: VecDeque<Actor>,
}

impl Default for AscentEngine {
    fn default() -> Self {
        Self::new()
    }
}

impl AscentEngine {
    /// Starts a new course at the market's lower service platform.
    pub fn new() -> Self {
        Self {
            feet: START,
            velocity: Point::default(),
            leader: Character::Chef,
            facing_right: true,
            grounded: Some(0),
            coyote: 0.1,
            jump_buffer: 0.0,
            time: 0.0,
            elapsed: 0.0,
            checkpoint: false,
            retries: 0,
            invulnerability: 1.0,
            highest: START.y,
            phase: RunPhase::Playing,
            trail: VecDeque::new(),
        }
    }

    /// Advances exactly one 120 Hz tick; adapters accumulate wall time outside the core.
    pub fn step(&mut self, command: PlayerCommand) {
        if command.retry_pressed {
            if self.phase == RunPhase::Complete {
                *self = Self::new();
            } else {
                self.respawn();
            }
            return;
        }
        if command.pause_pressed && self.phase != RunPhase::Complete {
            self.phase = if self.phase == RunPhase::Paused {
                RunPhase::Playing
            } else {
                RunPhase::Paused
            };
        }
        if self.phase != RunPhase::Playing {
            return;
        }
        if command.switch_character {
            self.leader = match self.leader {
                Character::Chef => Character::Detective,
                Character::Detective => Character::Chef,
            };
        }
        self.elapsed += STEP;
        self.invulnerability = (self.invulnerability - STEP).max(0.0);
        let old_surfaces = surfaces(self.time);
        self.time += STEP;
        let platforms = surfaces(self.time);
        if let Some(index) = self.grounded {
            self.feet.x += platforms[index].x - old_surfaces[index].x;
        }
        self.coyote = if self.grounded.is_some() {
            0.1
        } else {
            (self.coyote - STEP).max(0.0)
        };
        self.jump_buffer = if command.jump_pressed {
            0.12
        } else {
            (self.jump_buffer - STEP).max(0.0)
        };
        let horizontal = if command.horizontal.is_finite() {
            command.horizontal.clamp(-1.0, 1.0)
        } else {
            0.0
        };
        if horizontal.abs() > 0.01 {
            self.facing_right = horizontal > 0.0;
        }
        let target = horizontal * 285.0;
        let acceleration = if horizontal == 0.0 { 2400.0 } else { 1900.0 };
        self.velocity.x +=
            (target - self.velocity.x).clamp(-acceleration * STEP, acceleration * STEP);
        if self.jump_buffer > 0.0 && self.coyote > 0.0 {
            self.velocity.y = -680.0;
            self.grounded = None;
            self.coyote = 0.0;
            self.jump_buffer = 0.0;
        }
        let gravity = if self.velocity.y < 0.0 && !command.jump_held {
            2800.0
        } else {
            1500.0
        };
        self.velocity.y = (self.velocity.y + gravity * STEP).min(950.0);
        let previous = self.feet;
        self.feet.x = (self.feet.x + self.velocity.x * STEP).clamp(20.0, WIDTH - 20.0);
        self.feet.y += self.velocity.y * STEP;
        self.grounded = landing(previous, self.feet, &platforms);
        if let Some(index) = self.grounded {
            self.feet.y = platforms[index].y;
            self.velocity.y = 0.0;
            if index >= CHECKPOINT_INDEX {
                self.checkpoint = true;
            }
        }
        self.highest = self.highest.min(self.feet.y);
        if self.feet.y > self.highest + 650.0 || self.feet.y > START.y + 250.0 {
            self.respawn();
            return;
        }
        if self.invulnerability <= 0.0
            && hazards(self.time, &platforms)
                .iter()
                .any(|h| touches(self.feet, h))
        {
            self.respawn();
            return;
        }
        if (self.feet.x - GOAL.x).abs() < 45.0 && (self.feet.y - GOAL.y).abs() < 75.0 {
            self.phase = RunPhase::Complete;
        }
        self.trail.push_back(self.actor());
        if self.trail.len() > 26 {
            self.trail.pop_front();
        }
    }

    /// Returns immutable value data; the adapter chooses sprites, camera, UI, and light.
    pub fn snapshot(&self) -> WorldSnapshot {
        let leader = self.actor();
        let mut companion = self.trail.front().copied().unwrap_or(Actor {
            feet: Point {
                x: self.feet.x - 42.0,
                y: self.feet.y,
            },
            ..leader
        });
        companion.character = match self.leader {
            Character::Chef => Character::Detective,
            Character::Detective => Character::Chef,
        };
        if (companion.feet.x - leader.feet.x).abs() < 10.0
            && (companion.feet.y - leader.feet.y).abs() < 10.0
        {
            companion.feet.x -= if self.facing_right { 38.0 } else { -38.0 };
        }
        let platforms = surfaces(self.time);
        WorldSnapshot {
            leader,
            companion,
            hazards: hazards(self.time, &platforms),
            surfaces: platforms,
            checkpoint: Point {
                x: 170.0,
                y: START.y - CHECKPOINT_INDEX as f32 * 100.0,
            },
            checkpoint_reached: self.checkpoint,
            ingredient: GOAL,
            phase: self.phase,
            elapsed: self.elapsed,
            retries: self.retries,
            progress: ((START.y - self.highest) / (START.y - GOAL.y)).clamp(0.0, 1.0),
            invulnerable: self.invulnerability > 0.0,
        }
    }

    fn actor(&self) -> Actor {
        Actor {
            feet: self.feet,
            character: self.leader,
            airborne: self.grounded.is_none(),
            facing_right: self.facing_right,
        }
    }

    fn respawn(&mut self) {
        self.feet = if self.checkpoint {
            Point {
                x: 170.0,
                y: START.y - CHECKPOINT_INDEX as f32 * 100.0,
            }
        } else {
            START
        };
        self.velocity = Point::default();
        self.highest = self.feet.y;
        self.grounded = Some(if self.checkpoint { CHECKPOINT_INDEX } else { 0 });
        self.coyote = 0.1;
        self.jump_buffer = 0.0;
        self.invulnerability = 1.4;
        self.retries += 1;
        self.trail.clear();
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    fn hold(x: f32) -> PlayerCommand {
        PlayerCommand {
            horizontal: x,
            jump_held: true,
            ..PlayerCommand::default()
        }
    }

    #[test]
    fn jump_rises_and_lands_on_the_actual_start_surface() {
        let mut game = AscentEngine::new();
        game.step(PlayerCommand {
            jump_pressed: true,
            ..hold(0.0)
        });
        for _ in 0..25 {
            game.step(hold(0.0));
        }
        assert!(game.snapshot().leader.feet.y < START.y - 100.0);
        for _ in 0..110 {
            game.step(hold(0.0));
        }
        assert_eq!(game.snapshot().leader.feet.y, START.y);
        assert!(!game.snapshot().leader.airborne);
    }
    #[test]
    fn pause_freezes_rules_but_can_be_resumed() {
        let mut game = AscentEngine::new();
        game.step(PlayerCommand {
            pause_pressed: true,
            ..hold(1.0)
        });
        let before = game.snapshot();
        for _ in 0..100 {
            game.step(hold(1.0));
        }
        assert_eq!(game.snapshot().leader.feet, before.leader.feet);
        assert_eq!(game.snapshot().elapsed, before.elapsed);
        game.step(PlayerCommand {
            pause_pressed: true,
            ..hold(1.0)
        });
        assert!(game.snapshot().elapsed > before.elapsed);
    }
    #[test]
    fn holding_jump_does_not_automatically_jump_again_after_landing() {
        let mut game = AscentEngine::new();
        game.step(PlayerCommand {
            jump_pressed: true,
            ..hold(0.0)
        });
        for _ in 0..200 {
            game.step(hold(0.0));
        }
        assert!(!game.snapshot().leader.airborne);
    }
    #[test]
    fn switching_identity_preserves_the_shared_collision_position() {
        let mut game = AscentEngine::new();
        let before = game.snapshot().leader.feet;
        game.step(PlayerCommand {
            switch_character: true,
            ..PlayerCommand::default()
        });
        assert_eq!(game.snapshot().leader.character, Character::Detective);
        assert_eq!(game.snapshot().leader.feet, before);
    }
    #[test]
    fn retry_retains_the_reached_checkpoint() {
        let mut game = AscentEngine::new();
        game.checkpoint = true;
        game.feet = Point { x: 900.0, y: 300.0 };
        game.step(PlayerCommand {
            retry_pressed: true,
            ..PlayerCommand::default()
        });
        assert_eq!(game.snapshot().leader.feet, game.snapshot().checkpoint);
        assert_eq!(game.snapshot().retries, 1);
    }
    #[test]
    fn non_finite_input_cannot_poison_the_world() {
        let mut game = AscentEngine::new();
        game.step(hold(f32::NAN));
        assert!(game.snapshot().leader.feet.x.is_finite());
    }
    #[test]
    fn reaching_the_ingredient_completes_and_restart_resets_progress() {
        let mut game = AscentEngine::new();
        game.feet = GOAL;
        game.step(PlayerCommand::default());
        assert_eq!(game.snapshot().phase, RunPhase::Complete);
        game.step(PlayerCommand {
            retry_pressed: true,
            ..PlayerCommand::default()
        });
        assert_eq!(game.snapshot().phase, RunPhase::Playing);
        assert_eq!(game.snapshot().progress, 0.0);
    }
    #[test]
    fn walking_off_a_ledge_eventually_recovers_instead_of_softlocking() {
        let mut game = AscentEngine::new();
        for _ in 0..500 {
            game.step(hold(1.0));
        }
        assert!(game.snapshot().retries > 0);
    }
    #[test]
    fn every_adjacent_course_surface_is_reachable_with_the_shipped_jump() {
        for index in 0..17 {
            let mut game = AscentEngine::new();
            let platforms = surfaces(0.0);
            let source = platforms[index];
            let target = platforms[index + 1];
            let direction = if target.x > source.x { 1.0 } else { -1.0 };
            game.feet = Point {
                x: if direction > 0.0 {
                    source.x + source.width - 18.0
                } else {
                    source.x + 18.0
                },
                y: source.y,
            };
            game.grounded = Some(index);
            game.invulnerability = 999.0;
            game.highest = source.y;
            game.step(PlayerCommand {
                jump_pressed: true,
                ..hold(direction)
            });
            let mut reached = false;
            for _ in 0..115 {
                let target_now = surfaces(game.time)[index + 1];
                let desired = target_now.x + target_now.width * 0.5;
                let movement = ((desired - game.feet.x) * 0.06).clamp(-1.0, 1.0);
                game.step(hold(movement));
                if game.grounded == Some(index + 1) || game.phase == RunPhase::Complete {
                    reached = true;
                    break;
                }
            }
            assert!(reached, "unreachable course step {index} -> {}", index + 1);
        }
    }

    #[test]
    fn retry_after_a_high_fall_does_not_loop_at_the_checkpoint() {
        let mut game = AscentEngine::new();
        game.checkpoint = true;
        game.highest = GOAL.y;
        game.step(PlayerCommand {
            retry_pressed: true,
            ..PlayerCommand::default()
        });
        for _ in 0..20 {
            game.step(PlayerCommand::default());
        }
        assert_eq!(game.snapshot().retries, 1);
        assert_eq!(game.snapshot().leader.feet, game.snapshot().checkpoint);
    }
    #[test]
    fn monster_contact_recovers_once_and_protects_the_retry() {
        let mut game = AscentEngine::new();
        let platforms = surfaces(0.0);
        let pest = hazards(0.0, &platforms)[0];
        game.feet = Point {
            x: pest.center.x,
            y: platforms[3].y,
        };
        game.grounded = Some(3);
        game.invulnerability = 0.0;
        game.step(PlayerCommand::default());
        assert_eq!(game.snapshot().retries, 1);
        assert_eq!(game.snapshot().leader.feet, START);
        game.feet = Point {
            x: pest.center.x,
            y: platforms[3].y,
        };
        game.grounded = Some(3);
        game.step(PlayerCommand::default());
        assert_eq!(game.snapshot().retries, 1);
    }
}
