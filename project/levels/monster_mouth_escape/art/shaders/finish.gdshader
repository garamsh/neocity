// Finishes the drawn scene before the interface goes over it: the acid sea
// ripples, flows and bubbles below its surface and lights the air above it,
// bright parts spill light around them, and the edges darken toward violet.
// Every effect is worked out once per scene pixel, on the grid the camera
// draws the scene on, and steps through a few levels with ordered dithering,
// so it reads as pixel art and not as a smooth blur.
shader_type canvas_item;
render_mode unshaded;

uniform sampler2D screen : hint_screen_texture, filter_linear_mipmap, repeat_disable;
// Seconds of the art's steady clock.
uniform float time = 0.0;
// The screen position of the scene's origin and the screen pixels one scene
// pixel spans; the screen's own pixels when no scene is drawn.
uniform vec2 grid_origin = vec2(0.0);
uniform float grid = 1.0;
// The screen row of the acid sea's surface; below the screen when none shows.
uniform float surface = 100000.0;

// The canvas size the scene is laid out in.
const vec2 CANVAS = vec2(1280.0, 720.0);
// Scene pixels of acid light above the surface, and its strength there.
const float LIGHT_REACH = 72.0;
const vec3 ACID_LIGHT = vec3(0.10, 0.34, 0.08);
// Scene pixels below the surface that ripple, and that flow with light.
const float RIPPLE_DEPTH = 160.0;
const float FLOW_DEPTH = 14.0;
// Bubble lanes across the sea, in scene pixels, and how deep a bubble starts.
const float LANE = 14.0;
const float BUBBLE_DEPTH = 46.0;
// How bright a part must be before it spills light.
const float BLOOM_FROM = 0.52;
// Levels the soft effects step through, and the 4x4 ordered dither between them.
const float STEPS = 5.0;
const int BAYER[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};

varying vec2 canvas_at;

void vertex() {
	canvas_at = VERTEX;
}

float hash(vec2 p) {
	return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// The ordered dither threshold of a scene pixel, from 0 to 1.
float bayer(vec2 cell) {
	vec2 p = mod(cell, 4.0);
	return (float(BAYER[int(p.x) + int(p.y) * 4]) + 0.5) / 16.0;
}

// `value` from 0 to 1 stepped to STEPS levels, dithered by the scene pixel.
float stepped(float value, vec2 cell) {
	return clamp(floor(value * STEPS + bayer(cell)) / STEPS, 0.0, 1.0);
}

float luma(vec3 c) {
	return dot(c, vec3(0.299, 0.587, 0.114));
}

void fragment() {
	// The screen UV of a canvas point, from this fragment's own.
	vec2 uv_per_unit = vec2(dFdx(SCREEN_UV.x) / dFdx(canvas_at.x), dFdy(SCREEN_UV.y) / dFdy(canvas_at.y));
	vec2 cell = floor((canvas_at - grid_origin) / grid);
	vec2 centre = grid_origin + (cell + 0.5) * grid;
	// Scene pixels below the surface; negative above it.
	float depth = floor((centre.y - surface) / grid);
	vec2 source = canvas_at;
	if (depth > -3.0 && depth < RIPPLE_DEPTH) {
		// The surface heaves, and the sea under it sways sideways a pixel.
		float heave = round(sin(cell.x * 0.21 + time * 1.7) + sin(cell.x * 0.067 - time * 1.1));
		float sway = depth < 0.0 ? 0.0 : round(sin(cell.y * 0.45 + time * 2.3 + cell.x * 0.03) * 0.9);
		source = centre + vec2(sway, -heave) * grid;
		depth -= heave;
	}
	vec3 colour = texture(screen, SCREEN_UV + (source - canvas_at) * uv_per_unit).rgb;
	vec2 at_uv = SCREEN_UV + (centre - canvas_at) * uv_per_unit;

	if (depth >= 0.0 && depth < RIPPLE_DEPTH) {
		// Light flowing along the top of the sea, each row its own way.
		float row_way = mod(depth, 2.0) < 1.0 ? 1.0 : -1.0;
		float run = floor((cell.x + row_way * time * (7.0 + mod(depth, 3.0) * 3.0)) / 6.0);
		float flow = step(0.83, hash(vec2(run, depth))) * step(depth, FLOW_DEPTH) * (1.0 - depth / FLOW_DEPTH);
		colour += vec3(0.55, 0.85, 0.45) * stepped(flow, cell) * 0.7;
		// A bubble rising in its lane, bursting into a ring at the surface.
		float lane = floor(cell.x / LANE);
		float period = 2.2 + hash(vec2(lane, 3.0)) * 2.6;
		float rise = fract(time / period + hash(vec2(lane, 7.0)));
		if (hash(vec2(lane, 11.0)) < 0.6) {
			vec2 bubble = vec2(lane * LANE + 3.0 + floor(hash(vec2(lane, 5.0)) * (LANE - 6.0)), BUBBLE_DEPTH * (1.0 - rise));
			bubble.x += round(sin(rise * 12.0 + lane) * 0.8);
			vec2 offset = vec2(cell.x, depth) - bubble;
			float size = rise < 0.9 ? 1.0 + rise : 1.0 + (rise - 0.9) * 30.0;
			float far = length(offset);
			float ring = step(abs(far - size), 0.75);
			float fade = rise < 0.9 ? 1.0 : 1.0 - (rise - 0.9) * 10.0;
			colour = mix(colour, vec3(0.78, 1.0, 0.62), ring * fade * 0.85);
		}
	}

	// Acid light rising off the surface, the higher the fainter.
	float above = -depth;
	if (above > 0.0 && above < LIGHT_REACH) {
		float flicker = 0.85 + 0.15 * sin(time * 2.0 + cell.x * 0.05);
		float near = 1.0 - above / LIGHT_REACH;
		float lit = stepped(near * near * flicker, cell);
		colour += ACID_LIGHT * lit + colour * vec3(0.0, 0.35, 0.05) * lit;
	}

	// Bright parts spill light over their neighbours, from a blurred screen.
	vec3 spill = max(textureLod(screen, at_uv, 3.0).rgb - BLOOM_FROM, 0.0) * 1.3 + max(textureLod(screen, at_uv, 5.0).rgb - BLOOM_FROM, 0.0) * 1.8;
	float spill_level = stepped(clamp(luma(spill) * 1.6, 0.0, 1.0), cell);
	colour += normalize(spill + 0.0001) * spill_level * 0.55;

	// Shadows lean to magenta, the whole a little more saturated.
	float l = luma(colour);
	colour = mix(vec3(l), colour, 1.12);
	colour += vec3(0.05, -0.01, 0.06) * (1.0 - smoothstep(0.0, 0.45, l));

	// The edges darken toward deep violet in dithered steps.
	vec2 from_middle = (centre / CANVAS - 0.5) * vec2(1.0, CANVAS.y / CANVAS.x) * 2.0;
	float edge = stepped(smoothstep(0.62, 1.25, length(from_middle)), cell);
	colour = mix(colour, vec3(0.05, 0.01, 0.08), edge * 0.55);

	COLOR = vec4(clamp(colour, 0.0, 1.0), 1.0);
}
