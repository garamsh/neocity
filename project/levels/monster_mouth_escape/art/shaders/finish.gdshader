// Finishes the drawn city before the interface goes over it: both acid
// surfaces heave under a rim of foam with light running along it, a web of
// light crawls over the acid near its surface, which sinks to a deeper shade
// below, bubbles rise and burst into foam, whatever stands over or in the
// acid takes its light in bands that fade away from the surface, bright parts
// spill light around them, shadows lean to magenta and the edges darken
// toward violet. Each effect is worked out once per pixel of the art it lies
// on, the band's own pixels behind the map and the scene's pixels elsewhere,
// and soft light steps through a few levels, so it reads as pixel art and not
// as a smooth blur.
shader_type canvas_item;
render_mode unshaded;

uniform sampler2D screen : hint_screen_texture, filter_linear_mipmap, repeat_disable;
// Seconds of the art's steady clock.
uniform float time = 0.0;
// The screen row of the acid band's surface and the screen column its art's
// pixel grid starts from.
uniform float band = 470.0;
uniform float band_left = 0.0;
// The screen row of the surface of the sea in the scene.
uniform float sea = 1000.0;
// The screen position of the scene's origin and the screen pixels one scene
// pixel spans.
uniform vec2 grid_origin = vec2(0.0);
uniform float grid = 1.0;

// The canvas size the scene is laid out in.
const vec2 CANVAS = vec2(1280.0, 720.0);
// The screen pixels of one of the band art's pixels.
const float BAND_PIXEL = 4.0;
// Rows near a surface that heave, and that light runs along.
const float HEAVE_DEPTH = 4.0;
const float RUN_DEPTH = 2.0;
// The web's cells, in pixels, and how deep under the surface it shows and
// how bright its strands are there.
const float WEB_CELL = 10.0;
const float WEB_DEPTH = 24.0;
const vec3 WEB_LIGHT = vec3(0.45, 0.6, 0.25);
// The acid sinks toward this shade over this many pixels under its surface.
const vec3 DEEP_SHADE = vec3(0.8, 0.86, 0.8);
const float DEEP_DEPTH = 48.0;
// The foam along the surface's top row, and the light running along it.
const vec3 FOAM = vec3(0.82, 1.0, 0.58);
const vec3 GLINT = vec3(0.95, 1.0, 0.8);
// Bubble lanes across the acid, in pixels, how deep a bubble starts, and the
// share of its rise after which it bursts at the surface.
const float LANE = 9.0;
const float BUBBLE_DEPTH = 14.0;
const float BURST = 0.82;
// The light the acid throws on what stands over and in it: bands this many
// screen pixels tall, this many of them, each fainter than the one nearer the
// surface, which pull a colour toward the acid's green at a brightness this
// much over its own; what stands in the acid is lit by this share of it.
const float GLOW_BAND = 8.0;
const float GLOW_BANDS = 6.0;
const vec3 ACID_GLOW = vec3(0.35, 1.0, 0.25);
const float ACID_LIFT = 0.45;
const float GLOW_MIX = 0.7;
const float WADING = 0.7;
// How much acid's green under the surface lights the bands over it in full.
const float GLOW_SHOWS = 8.0;
// How bright a part must be before it spills light, and how strongly.
const float BLOOM_FROM = 0.6;
const float BLOOM = 0.5;
// Levels the soft effects step through, and the 4x4 ordered dither between them.
const float STEPS = 4.0;
const int BAYER[16] = int[](0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5);

varying vec2 canvas_at;

void vertex() {
	canvas_at = VERTEX;
}

float hash(vec2 p) {
	return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

vec2 hash2(vec2 p) {
	return vec2(hash(p), hash(p + vec2(17.3, 41.9)));
}

// The ordered dither threshold of a pixel cell, from 0 to 1.
float bayer(vec2 cell) {
	vec2 p = mod(cell, 4.0);
	return (float(BAYER[int(p.x) + int(p.y) * 4]) + 0.5) / 16.0;
}

// `value` from 0 to 1 stepped to STEPS levels, dithered by the cell.
float stepped(float value, vec2 cell) {
	return clamp(floor(value * STEPS + bayer(cell)) / STEPS, 0.0, 1.0);
}

float luma(vec3 c) {
	return dot(c, vec3(0.299, 0.587, 0.114));
}

// How much a colour reads as glowing acid: green over red and blue.
float greenness(vec3 c) {
	return max(c.g - max(c.r, c.b), 0.0);
}

bool acid(vec3 c) {
	return greenness(c) > 0.1 && c.g > 0.3;
}

// Whether a point lies on a strand of a web of cells that drift and churn
// with time: the strands are where two cells meet.
bool strand(vec2 p) {
	vec2 base = floor(p);
	float first = 9.0;
	float second = 9.0;
	for (int y = -1; y <= 1; y++) {
		for (int x = -1; x <= 1; x++) {
			vec2 at = base + vec2(float(x), float(y));
			vec2 seed = hash2(at);
			vec2 centre = at + 0.5 + 0.38 * sin(time * (0.4 + 0.5 * seed) + 6.2831 * seed);
			float d = length(p - centre);
			if (d < first) {
				second = first;
				first = d;
			} else if (d < second) {
				second = d;
			}
		}
	}
	return second - first < 0.11;
}

void fragment() {
	// The screen UV of a canvas point, from this fragment's own.
	vec2 uv_per_unit = vec2(dFdx(SCREEN_UV.x) / dFdx(canvas_at.x), dFdy(SCREEN_UV.y) / dFdy(canvas_at.y));
	vec3 own = texture(screen, SCREEN_UV).rgb;

	// The pixel this point lies on: the scene's in the sea in the scene and
	// above the band, the band art's in the band.
	vec2 scene_cell = floor((canvas_at - grid_origin) / grid);
	vec2 scene_centre = grid_origin + (scene_cell + 0.5) * grid;
	float sea_depth = floor((scene_centre.y - sea) / grid);
	vec2 band_cell = floor((canvas_at - vec2(band_left, band)) / BAND_PIXEL);
	bool in_band = sea_depth < -HEAVE_DEPTH && band_cell.y >= -HEAVE_DEPTH;
	float pixel = in_band ? BAND_PIXEL : grid;
	vec2 cell = in_band ? band_cell : scene_cell;
	vec2 centre = in_band ? vec2(band_left, band) + (band_cell + 0.5) * BAND_PIXEL : scene_centre;
	float depth = in_band ? band_cell.y : sea_depth;

	vec3 colour = own;
	if (abs(depth) <= HEAVE_DEPTH) {
		// The surface heaves: a pixel near it takes the acid it rises or sinks
		// to, and anything else keeps its own.
		float heave = round(0.9 * sin(cell.x * 0.21 + time * 1.7) + 0.8 * sin(cell.x * 0.067 - time * 1.1));
		vec3 moved = texture(screen, SCREEN_UV + (centre + vec2(0.0, heave * pixel) - canvas_at) * uv_per_unit).rgb;
		if (acid(moved) && (acid(own) || depth < 0.0)) {
			colour = moved;
		}
	}

	if (acid(colour) && depth >= 0.0) {
		// Foam along the top row, light running along the rows under it, each
		// its own way.
		colour = mix(colour, FOAM, 0.75 * step(depth * pixel, 3.0));
		float way = mod(depth, 2.0) < 1.0 ? 1.0 : -1.0;
		float run = floor((cell.x + way * time * (6.0 + depth * 4.0)) / 5.0);
		float glint = step(0.72, hash(vec2(run, depth))) * step(depth, RUN_DEPTH);
		colour = mix(colour, GLINT, glint * 0.9);
		// A web of light crawling over the acid near its surface, which sinks
		// to a deeper shade below.
		float near = 1.0 - depth / WEB_DEPTH;
		if (near > 0.0 && strand(vec2(cell.x, cell.y * 1.7) / WEB_CELL + vec2(time * 0.05, 0.0))) {
			colour += WEB_LIGHT * stepped(near * near, cell);
		}
		colour *= mix(vec3(1.0), DEEP_SHADE, min(depth / DEEP_DEPTH, 1.0));
	}
	if (depth >= -3.0 && depth <= BUBBLE_DEPTH && (acid(colour) || depth < 0.0)) {
		// A bubble rising in its lane, bursting into a ring of foam at the
		// surface.
		float lane = floor(cell.x / LANE);
		float period = 1.6 + hash(vec2(lane, 3.0)) * 2.4;
		float rise = fract(time / period + hash(vec2(lane, 7.0)));
		if (hash(vec2(lane, 11.0)) < 0.6) {
			float x = lane * LANE + 2.0 + floor(hash(vec2(lane, 5.0)) * (LANE - 4.0));
			vec2 from = vec2(cell.x - x, depth);
			if (rise < BURST) {
				vec2 bubble = vec2(round(sin(rise * 12.0 + lane) * 0.8), round(BUBBLE_DEPTH * (1.0 - rise / BURST)));
				if (acid(colour) && max(abs(from.x - bubble.x), abs(from.y - bubble.y)) <= 1.0 - step(rise, 0.4)) {
					colour = GLINT;
				}
			} else {
				float size = 1.0 + floor((rise - BURST) / (1.0 - BURST) * 3.0);
				float ring = step(abs(length(from) - size), 0.6);
				colour = mix(colour, FOAM, ring);
			}
		}
	}

	// What stands over the acid, or in it, takes its light in bands that fade
	// away from the surface: the sea's where it shows, else the band's.
	float from_sea = sea - scene_centre.y;
	float from_band = band - scene_centre.y;
	float from_surface = sea <= band || from_sea >= 0.0 && from_sea < from_band ? from_sea : from_band;
	float glow_band = floor(abs(from_surface) / GLOW_BAND);
	if (!acid(colour) && glow_band < GLOW_BANDS) {
		// Only where acid shows under the surface, in a few steps across.
		vec2 under = vec2(scene_centre.x, scene_centre.y + from_surface);
		vec2 to_under = SCREEN_UV + (under - canvas_at) * uv_per_unit;
		float shows = max(greenness(textureLod(screen, to_under + vec2(0.0, 12.0) * uv_per_unit, 4.0).rgb), greenness(textureLod(screen, to_under + vec2(0.0, 32.0) * uv_per_unit, 4.0).rgb));
		float lit = floor(clamp(shows * GLOW_SHOWS, 0.0, 1.0) * 3.0) / 3.0;
		lit *= (GLOW_BANDS - glow_band) / GLOW_BANDS;
		lit *= from_surface < 0.0 ? WADING : 1.0;
		lit *= 0.92 + 0.08 * sin(time * 2.3 + glow_band);
		colour = mix(colour, ACID_GLOW * (ACID_LIFT + luma(colour)), lit * GLOW_MIX);
	}

	// Bright parts spill light over their neighbours, read from the blurred
	// screen at the middle of this point's scene pixel.
	vec2 at_uv = SCREEN_UV + (scene_centre - canvas_at) * uv_per_unit;
	vec3 near_blur = textureLod(screen, at_uv, 3.0).rgb;
	vec3 far_blur = textureLod(screen, at_uv, 5.0).rgb;
	vec3 spill = near_blur * max(luma(near_blur) - BLOOM_FROM, 0.0) + far_blur * max(luma(far_blur) - BLOOM_FROM, 0.0);
	float spill_level = stepped(clamp(luma(spill) * 5.0, 0.0, 1.0), scene_cell);
	if (luma(own) < BLOOM_FROM) {
		colour += normalize(spill + 0.0001) * spill_level * BLOOM;
	}

	// Shadows but the acid's lean to magenta, and the whole is a little more
	// saturated.
	float l = luma(colour);
	if (!acid(colour)) {
		colour += vec3(0.035, -0.01, 0.045) * (1.0 - smoothstep(0.0, 0.4, l));
	}
	colour = mix(vec3(l), colour, 1.12);

	// The edges darken toward deep violet in dithered steps.
	vec2 from_middle = (scene_centre / CANVAS - 0.5) * vec2(1.0, CANVAS.y / CANVAS.x) * 2.0;
	float edge = stepped(smoothstep(0.7, 1.3, length(from_middle)), scene_cell);
	colour = mix(colour, vec3(0.05, 0.01, 0.08), edge * 0.5);

	COLOR = vec4(clamp(colour, 0.0, 1.0), 1.0);
}
