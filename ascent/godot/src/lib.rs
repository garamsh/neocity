//! Godot host adapter for the ascent domain; simulation remains in the engine package.
mod camera;
mod drawing;
mod hud;
mod input;
mod scene;

use godot::prelude::*;

struct NeonExtension;

// SAFETY: Godot calls this entry point; registration uses the binding's safe APIs.
#[gdextension]
unsafe impl ExtensionLibrary for NeonExtension {}
