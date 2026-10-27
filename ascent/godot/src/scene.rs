use crate::{camera::AscentCamera, drawing::AscentDrawing, input};
use godot::classes::{Engine, INode2D, Node2D};
use godot::prelude::*;
use neocity_ascent_engine::{AscentEngine, PlayerCommand};

/// Godot composition root: wires the pure engine to input, camera, and canvas adapters.
#[derive(GodotClass)]
#[class(base=Node2D)]
pub struct AscentScene {
    base: Base<Node2D>,
    engine: AscentEngine,
    camera: AscentCamera,
    drawing: Option<AscentDrawing>,
    accumulator: f64,
    pending: PlayerCommand,
}

#[godot_api]
impl INode2D for AscentScene {
    fn init(base: Base<Node2D>) -> Self {
        let engine = AscentEngine::new();
        let camera = AscentCamera::new(&engine.snapshot());
        Self {
            base,
            engine,
            camera,
            drawing: None,
            accumulator: 0.0,
            pending: PlayerCommand::default(),
        }
    }

    fn ready(&mut self) {
        if Engine::singleton().is_editor_hint() {
            return;
        }
        self.drawing = Some(AscentDrawing::load());
        godot_print!("Ascent: Rust simulation and presentation ready.");
    }

    fn process(&mut self, delta: f64) {
        if Engine::singleton().is_editor_hint() {
            return;
        }
        let sampled = input::sample();
        self.pending.horizontal = sampled.horizontal;
        self.pending.jump_held = sampled.jump_held;
        self.pending.jump_pressed |= sampled.jump_pressed;
        self.pending.switch_character |= sampled.switch_character;
        self.pending.pause_pressed |= sampled.pause_pressed;
        self.pending.retry_pressed |= sampled.retry_pressed;
        self.accumulator = (self.accumulator + delta).min(0.1);
        while self.accumulator >= 1.0 / 120.0 {
            self.engine.step(self.pending);
            self.pending.jump_pressed = false;
            self.pending.switch_character = false;
            self.pending.pause_pressed = false;
            self.pending.retry_pressed = false;
            self.accumulator -= 1.0 / 120.0;
        }
        let snapshot = self.engine.snapshot();
        self.camera.follow(&snapshot, delta.min(0.1) as f32);
        self.base_mut().queue_redraw();
    }

    fn draw(&mut self) {
        let snapshot = self.engine.snapshot();
        let camera_y = self.camera.y;
        if let Some(drawing) = &self.drawing {
            let mut canvas = self.base().clone();
            drawing.paint(&mut canvas, &snapshot, camera_y);
        }
    }
}

#[godot_api]
impl AscentScene {
    /// Exposes the actual actor location for host integration assertions.
    #[func]
    fn actor_position(&self) -> Vector2 {
        let p = self.engine.snapshot().leader.feet;
        Vector2::new(p.x, p.y)
    }
}
