//! Framework-independent ascent rules, reached only through commands and value snapshots.
mod collision;
mod course;
mod simulation;
mod world_snapshot;

pub use simulation::{AscentEngine, PlayerCommand};
pub use world_snapshot::{
    Actor, Character, Hazard, HazardKind, Point, RunPhase, Surface, WorldSnapshot,
};
