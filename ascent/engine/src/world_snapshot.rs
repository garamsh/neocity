/// A location in the engine's world coordinate system, with positive Y downward.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Point {
    pub x: f32,
    pub y: f32,
}

/// The two selectable members of the expedition.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Character {
    Chef,
    Detective,
}

/// A one-way landing surface. The top edge is its collision boundary.
#[derive(Clone, Copy, Debug)]
pub struct Surface {
    pub x: f32,
    pub y: f32,
    pub width: f32,
    pub moving: bool,
}

/// A character pose supplied to a presentation adapter.
#[derive(Clone, Copy, Debug)]
pub struct Actor {
    pub feet: Point,
    pub character: Character,
    pub airborne: bool,
    pub facing_right: bool,
}

/// The visible obstacle pattern, independent of sprites and rendering resources.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum HazardKind {
    Patrol,
    Flyer,
    Projectile,
}

/// An obstacle's current collision volume and telegraph state.
#[derive(Clone, Copy, Debug)]
pub struct Hazard {
    pub center: Point,
    pub radius: f32,
    pub kind: HazardKind,
    pub warning: bool,
    pub active: bool,
}

/// Mutually exclusive expedition states.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum RunPhase {
    Playing,
    Paused,
    Complete,
}

/// Values exposed by the engine, without renderer objects or platform input codes.
#[derive(Clone, Debug)]
pub struct WorldSnapshot {
    pub leader: Actor,
    pub companion: Actor,
    pub surfaces: Vec<Surface>,
    pub hazards: Vec<Hazard>,
    pub checkpoint: Point,
    pub checkpoint_reached: bool,
    pub ingredient: Point,
    pub phase: RunPhase,
    pub elapsed: f32,
    pub retries: u32,
    pub progress: f32,
    pub invulnerable: bool,
}
