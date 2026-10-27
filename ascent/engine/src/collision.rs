use crate::{Hazard, Point, Surface};

pub(crate) const HALF_WIDTH: f32 = 14.0;
pub(crate) const BODY_HEIGHT: f32 = 42.0;

pub(crate) fn landing(previous: Point, next: Point, surfaces: &[Surface]) -> Option<usize> {
    if next.y < previous.y {
        return None;
    }
    surfaces
        .iter()
        .enumerate()
        .filter(|(_, p)| {
            previous.y <= p.y + 0.1
                && next.y >= p.y
                && next.x + HALF_WIDTH > p.x
                && next.x - HALF_WIDTH < p.x + p.width
        })
        .min_by(|(_, a), (_, b)| a.y.total_cmp(&b.y))
        .map(|(index, _)| index)
}

pub(crate) fn touches(feet: Point, hazard: &Hazard) -> bool {
    if !hazard.active {
        return false;
    }
    let nearest_x = hazard
        .center
        .x
        .clamp(feet.x - HALF_WIDTH, feet.x + HALF_WIDTH);
    let nearest_y = hazard.center.y.clamp(feet.y - BODY_HEIGHT, feet.y);
    let dx = nearest_x - hazard.center.x;
    let dy = nearest_y - hazard.center.y;
    dx * dx + dy * dy < hazard.radius * hazard.radius
}

#[cfg(test)]
mod tests {
    use super::*;
    fn ledge() -> Surface {
        Surface {
            x: 100.0,
            y: 200.0,
            width: 160.0,
            moving: false,
        }
    }

    #[test]
    fn descending_crossing_lands_even_when_one_tick_passes_the_surface() {
        assert_eq!(
            landing(
                Point { x: 150.0, y: 180.0 },
                Point { x: 150.0, y: 215.0 },
                &[ledge()]
            ),
            Some(0)
        );
    }
    #[test]
    fn rising_through_a_one_way_ledge_does_not_land() {
        assert_eq!(
            landing(
                Point { x: 150.0, y: 215.0 },
                Point { x: 150.0, y: 180.0 },
                &[ledge()]
            ),
            None
        );
    }
    #[test]
    fn falling_outside_the_platform_does_not_snap_sideways() {
        assert_eq!(
            landing(
                Point { x: 300.0, y: 180.0 },
                Point { x: 300.0, y: 215.0 },
                &[ledge()]
            ),
            None
        );
    }
}
