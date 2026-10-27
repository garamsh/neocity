use neocity_ascent_engine::WorldSnapshot;

pub(crate) struct AscentCamera {
    pub(crate) y: f32,
}

impl AscentCamera {
    pub(crate) fn new(snapshot: &WorldSnapshot) -> Self {
        Self {
            y: snapshot.leader.feet.y - 560.0,
        }
    }

    pub(crate) fn follow(&mut self, snapshot: &WorldSnapshot, delta: f32) {
        let target = (snapshot.leader.feet.y - 540.0).clamp(0.0, 1740.0);
        if (target - self.y).abs() > 500.0 {
            self.y = target;
        } else {
            self.y += (target - self.y) * (1.0 - (-5.0 * delta).exp());
        }
    }
}
