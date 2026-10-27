use crate::{Hazard, HazardKind, Point, Surface};

pub(crate) const STEP: f32 = 1.0 / 120.0;
pub(crate) const START: Point = Point {
    x: 160.0,
    y: 2280.0,
};
pub(crate) const WIDTH: f32 = 1280.0;
pub(crate) const CHECKPOINT_INDEX: usize = 8;
pub(crate) const GOAL: Point = Point { x: 448.0, y: 566.0 };

pub(crate) fn surfaces(time: f32) -> Vec<Surface> {
    let xs = [
        60.0, 310.0, 560.0, 810.0, 1020.0, 800.0, 550.0, 300.0, 80.0, 320.0, 560.0, 800.0, 1030.0,
        800.0, 550.0, 300.0, 80.0, 330.0,
    ];
    xs.into_iter()
        .enumerate()
        .map(|(i, x)| Surface {
            x: x + if i == 6 || i == 13 {
                (time * 0.7).sin() * 32.0
            } else {
                0.0
            },
            y: START.y - i as f32 * 100.0,
            width: if i == 0 { 300.0 } else { 190.0 },
            moving: i == 6 || i == 13,
        })
        .collect()
}

pub(crate) fn hazards(time: f32, platforms: &[Surface]) -> Vec<Hazard> {
    let mut result = Vec::new();
    for (index, phase) in [(3, 0.0), (11, 2.1)] {
        let p = platforms[index];
        result.push(Hazard {
            center: Point {
                x: p.x + p.width * 0.5 + (time * 1.6 + phase).sin() * 52.0,
                y: p.y - 17.0,
            },
            radius: 23.0,
            kind: HazardKind::Patrol,
            warning: false,
            active: true,
        });
    }
    result.push(Hazard {
        center: Point {
            x: 600.0 + (time * 1.1).sin() * 85.0,
            y: platforms[7].y - 65.0,
        },
        radius: 21.0,
        kind: HazardKind::Flyer,
        warning: false,
        active: true,
    });
    let cycle = time.rem_euclid(4.0);
    result.push(Hazard {
        center: Point {
            x: 1130.0 - (cycle - 1.0).max(0.0) * 175.0,
            y: platforms[14].y - 26.0,
        },
        radius: 10.0,
        kind: HazardKind::Projectile,
        warning: cycle < 1.0,
        active: (1.0..3.4).contains(&cycle),
    });
    result
}
