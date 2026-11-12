"""Turn Recraft generations and the concept sheets into the monster-mouth escape runtime PNGs.

Subcommands; generation uses assets.json beside this file:
  fetch RAW_DIR          download every URL source and decode it to PNG with ImageMagick, and
                         read every "commit" source from the repository's history; a "file"
                         source is read from the repository instead
  process RAW_DIR ART    write every runtime PNG and manifest.json under ART. Each output pixel
                         is one block of the source: the generation's own grid where one is
                         detected, otherwise an even division of the crop. A seamless asset's
                         crop is a third wider or taller than the tile it makes. A "cut" asset
                         keeps its source's own pixels, filled and cleared where it says, at the
                         source's size or resized to the screen's. A pose articulates a cut's
                         masked limbs with nearest sampling and the original colours
  sheet RAW_DIR ART CONCEPT_DIR OUT
                         write the review sheet: the concept images, a mock frame, 8x close-ups
                         (sheet poses beside sprites, plain renders beside processed), every asset
  check ART              measure every runtime PNG in ART's manifest and exit 1 when one breaks a
                         clean-art rule: lone pixels, its folder's colour limit, and for character
                         sprites the lit outline and the light from above, but for the folders cut
                         from the concept and the chef/appraiser cuts; those keep their own
                         colours and light, and one picture pixel to a screen pixel.
                         HD manifests instead check native size, hash and edge detail

fetch needs ImageMagick's magick to decode WebP; process and sheet use only the standard
library, so the same raw PNGs always give the same bytes.
"""

import cmath
import colorsys
import hashlib
import json
import math
import pathlib
import struct
import subprocess
import sys
import urllib.request
import zlib
from collections import deque

SPEC = pathlib.Path(__file__).with_name("assets.json")
REPOSITORY = pathlib.Path(__file__).resolve().parents[2]
GRID_CONFIDENCE = 4.0
NOISE_DISTANCE = 80**2 * 3
# The share of an asset's opaque pixels that may be lone: of a colour no 8-neighbour shares.
# The check allows LONE_LIMIT; merge_lone keeps LONE_KEEP of contrasting ones such as eyes.
LONE_LIMIT = 0.01
LONE_KEEP = 0.004
# The most colours an asset may use, by its folder: characters 16, every other sprite 12, the
# kitchen and the key art 32 and the logo 16.
COLOUR_LIMITS = {
    "characters": 16,
    "world": 12,
    "walkway": 12,
    "ground": 12,
    "decor": 12,
    "icons": 12,
    "ui": 12,
    "workshop": 32,
    "title": 32,
}
LOGO_COLOURS = 16
# Sprite folders whose non-cut assets keep the lit outline and the light from above.
SPRITE_FOLDERS = ("characters", "world", "walkway", "ground", "decor")
# The share of side and bottom edge pixels that must be no lighter than the pixel inside them.
EDGE_SHARE = 0.95
# Folders drawn one picture pixel to a screen pixel at the default camera, and the least share of
# their edge pixels, those that differ from a neighbour along a row or a column, that differ from
# both, so stand one pixel wide or tall. Art drawn in blocks of two or more pixels holds none.
SCREEN_PIXEL_FOLDERS = ("backgrounds", "walkway", "world", "ground", "decor")
THIN_SHARE = 0.1
# Folders cut from the concept with its own pixels, which keep its colours and its own light:
# neither the colour limit, the lone-pixel limit nor the sprite rules apply to them.
CONCEPT_CUT_FOLDERS = ("backgrounds", "walkway", "world", "ground", "decor")
CHARACTER_FRAMES = ("idle", "walk_1", "walk_2", "walk_3", "walk_4", "run_1", "run_2", "jump", "fall", "land", "climb")
CONCEPT_INTERFACE = ("ui/frame_panel.png", "ui/frame_button.png", "ui/frame_slot.png",
                     "ui/icon_day.png", "ui/icon_clock.png",
                     "icons/ingredient_mushroom.png", "icons/ingredient_tomato.png",
                     "icons/ingredient_fish.png", "icons/ingredient_pickles.png")
CONCEPT_CHARACTERS = tuple(f"characters/{role}_{frame}.png"
                           for role in ("chef", "appraiser") for frame in CHARACTER_FRAMES)


def read_png(path):
    """Returns (width, height, rows) with rows of RGBA tuples; 8-bit RGB/RGBA, non-interlaced."""
    data = pathlib.Path(path).read_bytes()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"{path} is not a PNG")
    pos, idat, header = 8, [], None
    while pos < len(data):
        length, kind = struct.unpack(">I4s", data[pos : pos + 8])
        body = data[pos + 8 : pos + 8 + length]
        if kind == b"IHDR":
            header = struct.unpack(">IIBBBBB", body)
        elif kind == b"IDAT":
            idat.append(body)
        pos += 12 + length
    width, height, depth, color, _, _, interlace = header
    if depth != 8 or color not in (2, 6) or interlace:
        raise ValueError(f"{path}: unsupported PNG (depth {depth}, color {color})")
    channels = 4 if color == 6 else 3
    raw = zlib.decompress(b"".join(idat))
    stride = width * channels
    prev = bytearray(stride)
    rows, pos = [], 0
    for _ in range(height):
        kind = raw[pos]
        line = bytearray(raw[pos + 1 : pos + 1 + stride])
        pos += 1 + stride
        unfilter(kind, line, prev, channels)
        if channels == 4:
            rows.append([tuple(line[i : i + 4]) for i in range(0, stride, 4)])
        else:
            rows.append([(*line[i : i + 3], 255) for i in range(0, stride, 3)])
        prev = line
    return width, height, rows


def unfilter(kind, line, prev, bpp):
    for i in range(len(line)):
        left = line[i - bpp] if i >= bpp else 0
        up = prev[i]
        if kind == 1:
            line[i] = (line[i] + left) & 255
        elif kind == 2:
            line[i] = (line[i] + up) & 255
        elif kind == 3:
            line[i] = (line[i] + ((left + up) >> 1)) & 255
        elif kind == 4:
            corner = prev[i - bpp] if i >= bpp else 0
            p = left + up - corner
            pa, pb, pc = abs(p - left), abs(p - up), abs(p - corner)
            pred = left if pa <= pb and pa <= pc else up if pb <= pc else corner
            line[i] = (line[i] + pred) & 255


def write_png(path, rows):
    """Writes RGBA rows as an 8-bit RGBA PNG with no ancillary chunks."""
    height, width = len(rows), len(rows[0])
    raw = bytearray()
    for row in rows:
        raw.append(0)
        for pixel in row:
            raw.extend(pixel)

    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body))

    pathlib.Path(path).parent.mkdir(parents=True, exist_ok=True)
    pathlib.Path(path).write_bytes(
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(bytes(raw), 9))
        + chunk(b"IEND", b"")
    )


def crop(rows, box):
    """The box of rows; what lies outside the image is clear white, which every key treats as background."""
    x, y, w, h = box
    height, width = len(rows), len(rows[0])
    clear = (255, 255, 255, 0)
    out = []
    for yy in range(y, y + h):
        if not 0 <= yy < height:
            out.append([clear] * w)
            continue
        row = rows[yy]
        out.append([clear] * max(0, -x) + row[max(0, x) : min(width, x + w)] + [clear] * max(0, x + w - width))
    return out


def wrap_x(rows):
    """Cross-fades the last quarter of each row into the first and drops it, so the result tiles horizontally."""
    width = len(rows[0])
    overlap = width // 4
    out = []
    for row in rows:
        line = list(row[: width - overlap])
        for x in range(overlap):
            t = (x + 0.5) / overlap
            tail = row[width - overlap + x]
            line[x] = tuple(round(tail[c] * (1 - t) + row[x][c] * t) for c in range(4))
        out.append(line)
    return out


def transpose(rows):
    return [list(column) for column in zip(*rows)]


def distance(a, b):
    return max(abs(a[0] - b[0]), abs(a[1] - b[1]), abs(a[2] - b[2]))


def background_mask(rows, key, tolerance, enclosed=False):
    """Marks border-connected pixels within tolerance of the key colour; with enclosed, every such pixel."""
    height, width = len(rows), len(rows[0])
    if enclosed:
        return [bytearray(distance(pixel, key) <= tolerance for pixel in row) for row in rows]
    mask = [bytearray(width) for _ in range(height)]
    queue = deque()
    for x in range(width):
        queue.extend(((x, 0), (x, height - 1)))
    for y in range(height):
        queue.extend(((0, y), (width - 1, y)))
    while queue:
        x, y = queue.popleft()
        if mask[y][x] or distance(rows[y][x], key) > tolerance:
            continue
        mask[y][x] = 1
        if x > 0:
            queue.append((x - 1, y))
        if x < width - 1:
            queue.append((x + 1, y))
        if y > 0:
            queue.append((x, y - 1))
        if y < height - 1:
            queue.append((x, y + 1))
    return mask


def keep_largest(mask):
    """Adds every foreground pixel outside the largest 8-connected foreground region to the mask."""
    height, width = len(mask), len(mask[0])
    label = [[0] * width for _ in range(height)]
    sizes = [0]
    for y in range(height):
        for x in range(width):
            if mask[y][x] or label[y][x]:
                continue
            sizes.append(0)
            current = len(sizes) - 1
            label[y][x] = current
            queue = deque([(x, y)])
            while queue:
                cx, cy = queue.popleft()
                sizes[current] += 1
                for nx in (cx - 1, cx, cx + 1):
                    for ny in (cy - 1, cy, cy + 1):
                        if 0 <= nx < width and 0 <= ny < height and not mask[ny][nx] and not label[ny][nx]:
                            label[ny][nx] = current
                            queue.append((nx, ny))
    largest = max(range(1, len(sizes)), key=lambda i: sizes[i])
    return [bytearray(0 if label[y][x] == largest else 1 for x in range(width)) for y in range(height)]


def downscale(rows, mask, width, height):
    """Nearest-neighbour sample at each target block's centre; a block is opaque when most of it is."""
    src_h, src_w = len(rows), len(rows[0])
    out = []
    for ty in range(height):
        y0, y1 = ty * src_h // height, max(ty * src_h // height + 1, (ty + 1) * src_h // height)
        line = []
        for tx in range(width):
            x0, x1 = tx * src_w // width, max(tx * src_w // width + 1, (tx + 1) * src_w // width)
            cy, cx = (y0 + y1) // 2, (x0 + x1) // 2
            if mask is None:
                line.append(rows[cy][cx])
                continue
            covered = sum(mask[y][x] for y in range(y0, y1) for x in range(x0, x1))
            if covered * 2 >= (y1 - y0) * (x1 - x0):
                line.append((0, 0, 0, 0))
            elif not mask[cy][cx]:
                line.append(rows[cy][cx])
            else:
                line.append(nearest_opaque(rows, mask, cx, cy, x0, y0, x1, y1))
        out.append(line)
    return out


def nearest_opaque(rows, mask, cx, cy, x0, y0, x1, y1):
    best = None
    for y in range(y0, y1):
        for x in range(x0, x1):
            if not mask[y][x]:
                d = (x - cx) ** 2 + (y - cy) ** 2
                if best is None or d < best[0]:
                    best = (d, rows[y][x])
    return best[1]


def recolour(rows, rules):
    """Applies each rule to opaque pixels whose hue in degrees lies in its "hues" range.

    A rule sets the hue to "hue" and multiplies saturation and lightness by "saturation" and
    "lightness"; each is optional.
    """
    out = []
    for row in rows:
        line = []
        for r, g, b, a in row:
            if a:
                h, l, s = colorsys.rgb_to_hls(r / 255, g / 255, b / 255)
                for rule in rules:
                    low, high = rule["hues"]
                    if low <= h * 360 <= high:
                        h = rule.get("hue", h * 360) / 360
                        s = min(1.0, s * rule.get("saturation", 1.0))
                        l = min(1.0, l * rule.get("lightness", 1.0))
                        break
                r, g, b = (round(c * 255) for c in colorsys.hls_to_rgb(h, l, s))
            line.append((r, g, b, a))
        out.append(line)
    return out


def snap(rows, palette):
    """Replaces every opaque pixel by its nearest palette colour (redmean distance)."""
    cache = {}
    out = []
    for row in rows:
        line = []
        for r, g, b, a in row:
            if a == 0:
                line.append((0, 0, 0, 0))
                continue
            key = (r, g, b)
            if key not in cache:
                cache[key] = min(palette, key=lambda p: redmean(p, key))
            line.append((*cache[key], 255))
        out.append(line)
    return out


def redmean(p, q):
    mean = (p[0] + q[0]) / 2
    dr, dg, db = p[0] - q[0], p[1] - q[1], p[2] - q[2]
    return (2 + mean / 256) * dr * dr + 4 * dg * dg + (2 + (255 - mean) / 256) * db * db


def drop_specks(rows):
    """Clears opaque pixels with no opaque 4-neighbour."""
    height, width = len(rows), len(rows[0])
    out = [list(row) for row in rows]
    for y in range(height):
        for x in range(width):
            if rows[y][x][3] == 0:
                continue
            neighbours = [
                rows[ny][nx][3]
                for nx, ny in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1))
                if 0 <= nx < width and 0 <= ny < height
            ]
            if not any(neighbours):
                out[y][x] = (0, 0, 0, 0)
    return out


def edge_profile(rows):
    """Sums the colour change between each column and the one before it."""
    width = len(rows[0])
    edge = [0.0] * width
    for row in rows:
        for x in range(1, width):
            a, b = row[x - 1], row[x]
            edge[x] += abs(a[0] - b[0]) + abs(a[1] - b[1]) + abs(a[2] - b[2])
    return edge


def detect_pitch(rows):
    """Returns (pitch, phase, confidence) of the strongest regular column grid, pitch 3 to 16 px.

    Confidence is the spectral peak over the median; among pitches within 80% of the peak the
    largest wins, because a grid also peaks at its half and third.
    """
    edge = edge_profile(rows)
    total = sum(edge) or 1.0
    spectrum = []
    for step in range(260):
        pitch = 3 + step * 0.05
        z = sum(e * cmath.exp(-2j * math.pi * i / pitch) for i, e in enumerate(edge))
        spectrum.append((abs(z) / total, pitch, (cmath.phase(z) / (2 * math.pi) * pitch) % pitch))
    peak = max(s[0] for s in spectrum)
    median = sorted(s[0] for s in spectrum)[len(spectrum) // 2] or 1e-9
    strength, pitch, phase = max((s for s in spectrum if s[0] >= 0.8 * peak), key=lambda s: s[1])
    return pitch, phase, peak / median


def grid_crop(rows, box, logical):
    """Moves box onto the generation's own pixel grid when one is found.

    Returns (box, pitch_x, pitch_y): box covers logical blocks of the detected pitch, centred on
    the given box and aligned to the grid, or the given box with pitches that divide it evenly.
    """
    x, y, w, h = box
    region = crop(rows, box)
    px, phase_x, conf_x = detect_pitch(region)
    py, phase_y, conf_y = detect_pitch(transpose(region))
    even = (box, w / logical[0], h / logical[1])
    if min(conf_x, conf_y) < GRID_CONFIDENCE:
        return even
    # An asset drawn coarser than the generation's grid takes a whole multiple of it, so the box
    # still covers the figure instead of shrinking to its centre.
    px *= max(1, round(w / (logical[0] * px)))
    py *= max(1, round(h / (logical[1] * py)))
    new_w, new_h = logical[0] * px, logical[1] * py
    left = x + phase_x + round((w - new_w) / 2 / px) * px
    top = y + phase_y + round((h - new_h) / 2 / py) * py
    if left < 0 or top < 0 or left + new_w > len(rows[0]) or top + new_h > len(rows):
        return even
    return [round(left), round(top), round(new_w), round(new_h)], px, py


def nearest_index(colour, palette, cache):
    """Index of the palette colour nearest to colour, keyed on 6-bit channels so the cache stays small."""
    key = (colour[0] >> 2, colour[1] >> 2, colour[2] >> 2)
    if key not in cache:
        centre = tuple(c * 4 + 2 for c in key)
        cache[key] = min(range(len(palette)), key=lambda i: (redmean(palette[i], centre), i))
    return cache[key]


def sample_blocks(rows, mask, logical, pitch, palette):
    """One palette index per block: the most frequent in its centre, or None when mostly clear."""
    cache = {}
    width, height = logical
    out = []
    for ty in range(height):
        y0 = int(ty * pitch[1])
        y1 = max(y0 + 1, min(len(rows), int((ty + 1) * pitch[1])))
        line = []
        for tx in range(width):
            x0 = int(tx * pitch[0])
            x1 = max(x0 + 1, min(len(rows[0]), int((tx + 1) * pitch[0])))
            if mask is not None:
                clear = sum(mask[yy][xx] for yy in range(y0, y1) for xx in range(x0, x1))
                if clear * 2 > (y1 - y0) * (x1 - x0):
                    line.append(None)
                    continue
            # The vote uses the block's central half, so a one-pixel line in the source keeps the
            # block it is centred in instead of losing to the colour around it.
            cx0, cx1 = x0 + (x1 - x0) // 4, x1 - (x1 - x0) // 4
            cy0, cy1 = y0 + (y1 - y0) // 4, y1 - (y1 - y0) // 4
            counts = {}
            for yy in range(cy0, max(cy1, cy0 + 1)):
                for xx in range(cx0, max(cx1, cx0 + 1)):
                    if mask is None or not mask[yy][xx]:
                        index = nearest_index(rows[yy][xx], palette, cache)
                        counts[index] = counts.get(index, 0) + 1
            if not counts:
                for yy in range(y0, y1):
                    for xx in range(x0, x1):
                        if not mask[yy][xx]:
                            index = nearest_index(rows[yy][xx], palette, cache)
                            counts[index] = counts.get(index, 0) + 1
            line.append(max(counts, key=lambda i: (counts[i], -i)))
        out.append(line)
    return out


def limit_colours(cells, palette, count, keep, fixed=()):
    """Keeps count palette indices (keep and fixed among them) and maps every other cell to the nearest kept one.

    After the most used colour, each pick maximises usage times distance to the colours already
    kept, so a small accent such as an earring survives against shades of the large areas.
    """
    usage = {}
    for row in cells:
        for index in row:
            if index is not None:
                usage[index] = usage.get(index, 0) + 1
    kept = [keep] if keep is not None and keep in usage else []
    kept += [i for i in fixed if i not in kept]
    candidates = sorted(usage, key=lambda i: (-usage[i], i))
    while len(kept) < count and any(i not in kept for i in candidates):
        kept.append(
            max(
                (i for i in candidates if i not in kept),
                key=lambda i: (usage[i] * min((redmean(palette[i], palette[k]) for k in kept), default=1), -i),
            )
        )
    if keep is not None and keep not in kept:
        kept = kept[: count - 1] + [keep]
    nearest = {i: min(kept, key=lambda k: (redmean(palette[k], palette[i]), k)) for i in usage}
    return [[None if i is None else nearest[i] for i in row] for row in cells]


def neighbours(cells, x, y, diagonal):
    steps = ((-1, 0), (1, 0), (0, -1), (0, 1))
    if diagonal:
        steps += ((-1, -1), (1, -1), (-1, 1), (1, 1))
    for dx, dy in steps:
        nx, ny = x + dx, y + dy
        yield cells[ny][nx] if 0 <= ny < len(cells) and 0 <= nx < len(cells[0]) else None


def most_common(values):
    counts = {}
    for value in values:
        if value is not None:
            counts[value] = counts.get(value, 0) + 1
    return max(counts, key=lambda v: (counts[v], -v)) if counts else None


def clean(cells, palette):
    """Clears thin alpha spurs, fills alpha notches, and replaces lone pixels that are noise.

    A pixel touching no neighbour of its own colour is noise when it is close to the colour
    around it; a lone pixel that contrasts, such as an eye, is kept.
    """
    height, width = len(cells), len(cells[0])
    shaped = [list(row) for row in cells]
    for y in range(height):
        for x in range(width):
            around = list(neighbours(cells, x, y, False))
            opaque = [v for v in around if v is not None]
            if cells[y][x] is not None and len(opaque) <= 1:
                shaped[y][x] = None
            elif cells[y][x] is None and len(opaque) >= 3:
                shaped[y][x] = most_common(opaque)
    out = [list(row) for row in shaped]
    for y in range(height):
        for x in range(width):
            value = shaped[y][x]
            around = list(neighbours(shaped, x, y, True))
            common = most_common(around)
            if value is None or value in around or common is None:
                continue
            if redmean(palette[value], palette[common]) < NOISE_DISTANCE:
                out[y][x] = common
    return out


def outline(cells, index):
    """Sets every opaque cell with a clear or off-edge 4-neighbour to index."""
    return [
        [
            index if value is not None and None in neighbours(cells, x, y, False) else value
            for x, value in enumerate(row)
        ]
        for y, row in enumerate(cells)
    ]


def luminance(colour):
    return 0.299 * colour[0] + 0.587 * colour[1] + 0.114 * colour[2]


def lone(cells, x, y):
    """Whether the opaque cell at x, y has no 8-neighbour of its own colour."""
    return cells[y][x] is not None and cells[y][x] not in neighbours(cells, x, y, True)


def merge_lone(cells, palette):
    """Replaces each lone pixel by the colour most common around it, so the asset is drawn in clusters.

    The lone pixels that contrast most with what is around them, such as an eye, are kept, up to
    LONE_KEEP of the opaque pixels.
    """
    opaque = sum(value is not None for row in cells for value in row)
    found = []
    for y, row in enumerate(cells):
        for x, value in enumerate(row):
            if lone(cells, x, y):
                common = most_common(neighbours(cells, x, y, True))
                if common is not None:
                    found.append((redmean(palette[value], palette[common]), x, y, common))
    found.sort(key=lambda item: (-item[0], item[2], item[1]))
    allowed = int(opaque * LONE_KEEP)
    out = [list(row) for row in cells]
    for rank, (contrast, x, y, common) in enumerate(found):
        if rank >= allowed or contrast < NOISE_DISTANCE * 4:
            out[y][x] = common
    return out


def facing(cells, x, y, tile):
    """The sides of the opaque cell at x, y that face clear space: "up", or "side" for any other.

    A tile continues past its own border, so its border faces nothing.
    """
    height, width = len(cells), len(cells[0])

    def clear(nx, ny):
        if 0 <= nx < width and 0 <= ny < height:
            return cells[ny][nx] is None
        return not tile

    up = clear(x, y - 1)
    side = clear(x - 1, y) or clear(x + 1, y) or clear(x, y + 1)
    return up, side


def lit_edges(cells, palette, tile):
    """The one outline rule: a selective outline lit from above.

    An edge facing up shows the colour under it, without a dark line; every other edge takes a
    darker colour of the asset's own, near 45% of the darker of itself and the pixel inside it.
    """
    kept = sorted({i for row in cells for i in row if i is not None})
    height, width = len(cells), len(cells[0])

    def shade(index):
        darker = [k for k in kept if luminance(palette[k]) <= luminance(palette[index])]
        target = tuple(c * 0.45 for c in palette[index])
        return min(darker, key=lambda k: (redmean(palette[k], target), k))

    def inner(x, y):
        """The opaque pixel opposite the first clear 4-neighbour of a side or bottom edge."""
        for dx, dy in ((-1, 0), (1, 0), (0, 1)):
            nx, ny, ix, iy = x + dx, y + dy, x - dx, y - dy
            if 0 <= nx < width and 0 <= ny < height and cells[ny][nx] is None:
                if 0 <= ix < width and 0 <= iy < height:
                    return cells[iy][ix]
                return None
        return None

    out = [list(row) for row in cells]
    for y, row in enumerate(cells):
        for x, value in enumerate(row):
            if value is None:
                continue
            up, side = facing(cells, x, y, tile)
            below = cells[y + 1][x] if y + 1 < height else None
            if up and below is not None and luminance(palette[below]) > luminance(palette[value]):
                out[y][x] = below
            elif side and not up:
                inside = inner(x, y)
                darkest = value
                if inside is not None and luminance(palette[inside]) < luminance(palette[value]):
                    darkest = inside
                out[y][x] = shade(darkest)
    return out


def match_edges(cells, base, sides, width, top=False):
    """Gives cells the outer columns of base on each side named, so the two tiles meet without a seam.

    base is aligned to the bottom of cells, or to the top with top. Each row copies base up to a cut
    that varies from row to row between half of width and width, so the join is ragged rather than a
    straight line.
    """
    height, tile_w, base_w = len(cells), len(cells[0]), len(base[0])
    out = [list(row) for row in cells]
    for by, base_row in enumerate(base):
        y = by if top else height - len(base) + by
        if y < 0 or y >= height:
            continue
        cut = width // 2 + (by * 5) % (width - width // 2 + 1)
        for c in range(cut):
            if "l" in sides:
                out[y][c] = base_row[c]
            if "r" in sides:
                out[y][tile_w - 1 - c] = base_row[base_w - 1 - c]
    return out


def recolour_rows(rows, rules):
    """recolour with a per-colour cache, for full-resolution crops."""
    cache = {}
    out = []
    for row in rows:
        line = []
        for pixel in row:
            if pixel not in cache:
                cache[pixel] = recolour([[pixel]], rules)[0][0]
            line.append(cache[pixel])
        out.append(line)
    return out


def upscale(rows, factor):
    out = []
    for row in rows:
        line = [pixel for pixel in row for _ in range(factor)]
        out.extend(list(line) for _ in range(factor))
    return out


def load(source, raw_dir, cache, sources):
    """Reads a source into cache once: a "file" source from the repository, any other from raw_dir."""
    if source not in cache:
        if "file" in sources[source]:
            path = REPOSITORY / sources[source]["file"]
        else:
            path = pathlib.Path(raw_dir) / f"{source}.png"
        cache[source] = read_png(path)[2]
    return cache[source]


def spans(source, target):
    """For each of target cells along an axis of source cells, the (index, weight) pairs of the
    source cells it covers, weighted by how much of each it covers."""
    out = []
    for t in range(target):
        low, high = t * source / target, (t + 1) * source / target
        cells = []
        for s in range(int(low), min(source, math.ceil(high))):
            weight = min(high, s + 1) - max(low, s)
            if weight > 0:
                cells.append((s, weight))
        out.append(cells)
    return out


def resample(rows, width, height):
    """Resizes rows to width x height, each pixel the average of the source area it covers.

    Colours are weighted by their alpha, so a clear pixel lends no colour to an opaque one beside it.
    """

    def along(lines, size):
        weights = spans(len(lines[0]), size)
        out = []
        for line in lines:
            premultiplied = [(r * a, g * a, b * a, a) for r, g, b, a in line]
            resized = []
            for cells in weights:
                total = [0.0, 0.0, 0.0, 0.0]
                for index, weight in cells:
                    pixel = premultiplied[index]
                    for c in range(4):
                        total[c] += pixel[c] * weight
                area = sum(weight for _, weight in cells)
                alpha = total[3] / area
                if alpha <= 0:
                    resized.append((0, 0, 0, 0))
                    continue
                resized.append(tuple(round(total[c] / total[3]) for c in range(3)) + (round(alpha),))
            out.append(resized)
        return out

    return transpose(along(transpose(along(rows, width)), height))


def polygon_mask(rows, polygons):
    """Keeps pixel centres inside the union of the source-coordinate polygons."""
    kept = [[(0, 0, 0, 0)] * len(rows[0]) for _ in rows]
    for polygon in polygons:
        edges = list(zip(polygon, polygon[1:] + polygon[:1]))
        for y in range(max(0, min(p[1] for p in polygon)), min(len(rows), max(p[1] for p in polygon))):
            scan = y + 0.5
            crossings = sorted(
                x1 + (scan - y1) * (x2 - x1) / (y2 - y1)
                for (x1, y1), (x2, y2) in edges
                if min(y1, y2) <= scan < max(y1, y2)
            )
            for left, right in zip(crossings[::2], crossings[1::2]):
                start, end = max(0, math.ceil(left - 0.5)), min(len(rows[0]), math.ceil(right - 0.5))
                if start < end:
                    kept[y][start:end] = rows[y][start:end]
    return kept


def feather_flesh(rows, depth):
    """Blends only pink flesh in each column's top depth rows; blue city and dark outlines stay opaque."""
    out = [list(row) for row in rows]
    for x in range(len(rows[0])):
        top = next((y for y, row in enumerate(rows) if row[x][3]), len(rows))
        for y in range(top, min(len(rows), top + depth)):
            r, g, b, a = rows[y][x]
            if r > g + 20 and b > g + 20 and r >= b and max(r, g, b) >= 80:
                out[y][x] = (r, g, b, round(a * (y - top) / depth))
    return out


def cut(asset, raw_dir, cache, sources):
    """Returns (rgba rows, report) for an asset cut from its source with the source's own pixels.

    Each fill, in order, replaces the pixels of its rects, [x, y, w, h], and of its columns,
    [x, w, y], from row y down, with the same pixels of its fill source, whose top-left lies at
    at. A fill may instead name source-coordinate polygons, keeping every pixel outside
    their union unchanged; the pixels above each clear column [x, w, y] are then made clear. Those boxes are in the
    source's pixels. With screen, [w, h], the source is then resized to the screen, and every
    later box is in the screen's pixels: each patch [x, y, w, h, dx, dy] takes the pixels dx, dy
    away, each clear rect is made clear, and outside the keep rects, when there are any, every
    pixel is made clear. Each clear colour rule then clears the pixels whose hue in degrees lies
    in its hues and whose saturation and lightness reach its own, inside its within rects when it
    has any. A mask keeps only the union of its polygons, independent of colour. The crop is
    cut out last; a list of crops is cut and stacked top to bottom. feather_flesh blends only
    pink tissue in the mask's first rows, retaining blue architecture and dark outlines.
    """
    spec = asset["cut"]
    plain = not spec.get("fills") and not spec.get("clear_above")
    resized = f"{asset['source']}@{spec.get('screen')}"
    if plain and "screen" in spec and resized in cache:
        rows = [list(row) for row in cache[resized]]
    else:
        rows = cut_source(asset, raw_dir, cache, sources)
        if "screen" in spec:
            rows = resample(rows, *spec["screen"])
            if plain:
                cache[resized] = [list(row) for row in rows]
    if "interior" in spec:
        x, y, w, h = spec["interior"]
        sx, sy = spec["fill_at"]
        fill = rows[sy][sx]
        for yy in range(y, y + h):
            rows[yy][x:x + w] = [fill] * w
    taken = [list(row) for row in rows]
    for x, y, w, h, dx, dy in spec.get("patches", []):
        for yy in range(y, y + h):
            rows[yy][x : x + w] = taken[yy + dy][x + dx : x + dx + w]
    for x, y, w, h in spec.get("clear", []):
        for yy in range(max(0, y), min(len(rows), y + h)):
            rows[yy][max(0, x) : x + w] = [(0, 0, 0, 0)] * len(rows[yy][max(0, x) : x + w])
    if spec.get("keep"):
        kept = [[(0, 0, 0, 0)] * len(rows[0]) for _ in rows]
        for x, y, w, h in spec["keep"]:
            for yy in range(max(0, y), min(len(rows), y + h)):
                kept[yy][max(0, x) : x + w] = rows[yy][max(0, x) : x + w]
        rows = kept
    for rule in spec.get("clear_colours", []):
        low, high = rule["hues"]
        within = rule.get("within", [[0, 0, len(rows[0]), len(rows)]])
        for y, row in enumerate(rows):
            for x, (r, g, b, a) in enumerate(row):
                if not any(wx <= x < wx + ww and wy <= y < wy + wh for wx, wy, ww, wh in within):
                    continue
                h, l, sat = colorsys.rgb_to_hls(r / 255, g / 255, b / 255)
                if a and low <= h * 360 <= high and sat >= rule["saturation"] and l >= rule["lightness"]:
                    row[x] = (0, 0, 0, 0)
    if "mask" in spec:
        rows = polygon_mask(rows, spec["mask"])
    if "feather_flesh" in spec:
        rows = feather_flesh(rows, spec["feather_flesh"])
    boxes = asset["crop"] if isinstance(asset["crop"][0], list) else [asset["crop"]]
    rows = [row for box in boxes for row in crop(rows, box)]
    if [len(rows[0]), len(rows)] != asset["size"]:
        raise ValueError(f"{asset['path']}: crop {asset['crop']} does not give size {asset['size']}")
    used = {pixel[:3] for row in rows for pixel in row if pixel[3]}
    return rows, {"colours": len(used), "pitch": [1, 1], "grid": "cut"}


def cut_source(asset, raw_dir, cache, sources):
    """The source of a cut asset with its fills and its clear columns, in the source's pixels."""
    rows = [list(row) for row in cache[asset["source"]]]
    spec = asset["cut"]
    for fill in spec.get("fills", []):
        filled = load(fill["source"], raw_dir, cache, sources)
        left, top = fill["at"]
        if "polygons" in fill:
            masked = polygon_mask(rows, fill["polygons"])
            for yy, row in enumerate(masked):
                for xx, pixel in enumerate(row):
                    if pixel[3]:
                        rows[yy][xx] = filled[yy - top][xx - left]
        boxes = list(fill.get("rects", []))
        boxes += [[x, y, w, top + len(filled) - y] for x, w, y in fill.get("columns", [])]
        for x, y, w, h in boxes:
            for yy in range(y, y + h):
                rows[yy][x : x + w] = filled[yy - top][x - left : x - left + w]
    for x, w, y in spec.get("clear_above", []):
        for yy in range(y):
            rows[yy][x : x + w] = [(0, 0, 0, 0)] * w
    return rows


def articulate(base, rig, pose):
    """Moves masked limbs with nearest sampling, retaining the idle's colours and body pixels."""
    height, width = len(base), len(base[0])
    clear = (0, 0, 0, 0)
    body = [list(row) for row in base]
    layers = []
    for name, part in rig.items():
        limb = polygon_mask(base, part["mask"])
        for y, row in enumerate(limb):
            for x, pixel in enumerate(row):
                if pixel[3]:
                    body[y][x] = clear
        move = pose["limbs"].get(name, {})
        angle = math.radians(move.get("angle", 0))
        cosine, sine = math.cos(angle), math.sin(angle)
        px, py = part["pivot"]
        dy = 0
        if "floor" in move:
            bottom = max(round(py + (x - px) * sine + (y - py) * cosine)
                         for y, row in enumerate(limb) for x, pixel in enumerate(row) if pixel[3])
            dy += move["floor"] - bottom
        moved = [[clear] * width for _ in range(height)]
        for y in range(height):
            for x in range(width):
                sx = round(px + (x - px) * cosine + (y - dy - py) * sine)
                sy = round(py - (x - px) * sine + (y - dy - py) * cosine)
                if 0 <= sx < width and 0 <= sy < height:
                    moved[y][x] = limb[sy][sx]
        layers.append((part.get("front", False), moved))
    canvas = [[clear] * width for _ in range(height)]
    for front, rows in layers:
        if not front:
            paste(canvas, rows, 0, 0)
    dx, dy = pose.get("body", [0, 0])
    paste(canvas, crop(body, [-dx, -dy, width, height]), 0, 0)
    for front, rows in layers:
        if front:
            paste(canvas, rows, 0, 0)
    return canvas


def render(asset, raw_dir, palette, cache, sources, plain=False, done=None):
    """Returns (rgba rows, report). plain renders the earlier nearest-neighbour pipeline for review.

    done maps the path of each asset already rendered to its palette-index cells; an asset that
    matches another reads that one's cells from it. Cut rows and rigs are cached separately
    so a pose can retain the base cut's colours without palette processing.
    """
    if "pose" in asset:
        pose = asset["pose"]
        rows = articulate(cache["rendered:" + pose["base"]], cache["rig:" + pose["base"]], pose)
        return rows, {"colours": len({pixel[:3] for row in rows for pixel in row if pixel[3]}),
                      "pitch": [1, 1], "grid": "articulated cut"}
    source = asset["source"]
    load(source, raw_dir, cache, sources)
    if "cut" in asset:
        rows, report = cut(asset, raw_dir, cache, sources)
        cache["rendered:" + asset["path"]] = rows
        if "rig" in asset:
            cache["rig:" + asset["path"]] = asset["rig"]
        return rows, report
    width, height = asset["size"]
    scale = asset.get("scale", 1)
    logical = (width // scale, height // scale)
    seamless = asset.get("seamless", "")
    box, pitch = asset["crop"], None
    if not plain and not seamless and "match" not in asset:
        box, pitch_x, pitch_y = grid_crop(cache[source], asset["crop"], logical)
        pitch = (pitch_x, pitch_y)
    rows = crop(cache[source], box)
    if "x" in seamless:
        rows = wrap_x(rows)
    if "y" in seamless:
        rows = transpose(wrap_x(transpose(rows)))
    mask = None
    key = asset.get("key")
    if key and key.get("alpha"):
        mask = [bytearray(1 if pixel[3] < 128 else 0 for pixel in row) for row in rows]
    elif key:
        mask = background_mask(rows, hex_colour(key["colour"]), key["tolerance"], key.get("enclosed", False))
    if mask is not None and asset.get("largest_only"):
        mask = keep_largest(mask)
    if plain:
        small = downscale(rows, mask, *logical)
        if "recolour" in asset:
            small = recolour(small, asset["recolour"])
        small = snap(small, palette)
        if mask is not None:
            small = drop_specks(small)
        return upscale(small, scale), None
    if "recolour" in asset:
        rows = recolour_rows(rows, asset["recolour"])
    if pitch is None:
        pitch = (len(rows[0]) / logical[0], len(rows) / logical[1])
    cells = sample_blocks(rows, mask, logical, pitch, palette)
    edge = palette.index(hex_colour(asset["outline"])) if "outline" in asset else None
    match = asset.get("match")
    base = done[match["path"]] if match else None
    fixed = sorted({i for row in base for i in row if i is not None}) if base else ()
    cells = merge_lone(clean(limit_colours(cells, palette, asset["colours"], edge, fixed), palette), palette)
    if edge is not None:
        cells = outline(cells, edge)
    elif asset.get("edge") == "lit":
        tile = bool(seamless or match or asset.get("tile"))
        cells = lit_edges(cells, palette, tile)
    if match:
        cells = match_edges(cells, base, match["sides"], match["width"], match.get("top", False))
    # The outline and the joins leave some pixels lone again.
    cells = merge_lone(cells, palette)
    if done is not None:
        done[asset["path"]] = cells
    used = {i for row in cells for i in row if i is not None}
    rgba = [[(0, 0, 0, 0) if i is None else (*palette[i], 255) for i in row] for row in cells]
    grid = "detected" if box != asset["crop"] else "even"
    report = {"colours": len(used), "pitch": [round(p, 2) for p in pitch], "grid": grid}
    return upscale(rgba, scale), report


def hex_colour(text):
    return tuple(int(text[i : i + 2], 16) for i in (1, 3, 5))


def load_spec():
    spec = json.loads(SPEC.read_text())
    palette = [hex_colour(h) for h in spec["palette"]]
    return spec, palette


def fetch(raw_dir):
    spec, _ = load_spec()
    raw_dir = pathlib.Path(raw_dir)
    raw_dir.mkdir(parents=True, exist_ok=True)
    for name, source in spec["sources"].items():
        if "file" in source:
            continue
        if "commit" in source:
            shown = subprocess.run(
                ["git", "-C", str(REPOSITORY), "show", f"{source['commit']}:{source['path']}"],
                check=True,
                capture_output=True,
            ).stdout
            subprocess.run(["magick", "png:-", "-strip", f"PNG24:{raw_dir / name}.png"], input=shown, check=True)
            print(f"fetched {name}")
            continue
        url = source["url"]
        if not url.startswith("https://img.recraft.ai/"):
            raise ValueError(f"{name}: unexpected host in {url}")
        webp = raw_dir / f"{name}.webp"
        webp.write_bytes(urllib.request.urlopen(url, timeout=60).read())
        depth = "PNG32" if source.get("alpha") else "PNG24"
        subprocess.run(["magick", str(webp), "-strip", f"{depth}:{raw_dir / name}.png"], check=True)
        print(f"fetched {name}")


def process(raw_dir, art_dir):
    spec, palette = load_spec()
    cache = {}
    done = {}
    manifest = []
    for asset in spec["assets"]:
        rows, report = render(asset, raw_dir, palette, cache, spec["sources"], done=done)
        write_png(pathlib.Path(art_dir) / asset["path"], rows)
        manifest.append(
            {
                "path": asset["path"],
                "size": asset["size"],
                "scale": asset.get("scale", 1),
                "colours": report["colours"],
                "depicts": asset["depicts"],
                **({"provenance": asset["provenance"]} if "provenance" in asset else {}),
            }
        )
        print(f"{asset['path']}: {report['colours']} colours, block {report['pitch']} px, {report['grid']} grid")
    text = json.dumps({"generated_by": "tools/art/pixelize.py process", "files": manifest}, indent=1)
    (pathlib.Path(art_dir) / "manifest.json").write_text(text + "\n")
    print(f"wrote {len(manifest)} runtime PNGs listed in {SPEC.name} and manifest.json under {art_dir}")


def measure(rows, scale):
    """The clean-art measures of one runtime PNG, read on its own pixel grid.

    Returns the share of lone pixels, the colour count, the share of side and bottom edge pixels
    no lighter than the pixel inside them (None without such edges), and the mean luminance of
    what faces up and what faces down: the up- and down-facing edges, or for an asset with no
    clear pixel its top and bottom quarter.
    """
    cells_rgba = [row[::scale] for row in rows[::scale]]
    colours = {}
    cells = [[None if a == 0 else colours.setdefault((r, g, b), len(colours)) for r, g, b, a in row] for row in cells_rgba]
    palette = list(colours)
    height, width = len(cells), len(cells[0])
    opaque = [(x, y) for y in range(height) for x in range(width) if cells[y][x] is not None]
    lone_count = sum(lone(cells, x, y) for x, y in opaque)
    dark, judged, up_lums, down_lums = 0, 0, [], []
    for x, y in opaque:
        up, side = facing(cells, x, y, True)
        lum = luminance(palette[cells[y][x]])
        if up:
            up_lums.append(lum)
        if y + 1 < height and cells[y + 1][x] is None:
            down_lums.append(lum)
        if not side or up:
            continue
        for dx, dy in ((-1, 0), (1, 0), (0, 1)):
            nx, ny, ix, iy = x + dx, y + dy, x - dx, y - dy
            if 0 <= nx < width and 0 <= ny < height and cells[ny][nx] is None:
                if 0 <= ix < width and 0 <= iy < height and cells[iy][ix] is not None:
                    judged += 1
                    dark += lum <= luminance(palette[cells[iy][ix]])
                break
    if not up_lums or not down_lums:
        quarter = max(1, height // 4)
        up_lums = [luminance(palette[cells[y][x]]) for x, y in opaque if y < quarter]
        down_lums = [luminance(palette[cells[y][x]]) for x, y in opaque if y >= height - quarter]
    return {
        "lone": lone_count / max(1, len(opaque)),
        "colours": len(colours),
        "edge": dark / judged if judged else None,
        "lit": sum(up_lums) / max(1, len(up_lums)),
        "shaded": sum(down_lums) / max(1, len(down_lums)),
    }


def thin_share(rows):
    """Of the opaque pixels that differ from an opaque neighbour along a row, the share that differ
    from both, and the same along a column; the smaller of the two, or None with no such pixel."""
    shares = []
    for lines in (rows, transpose(rows)):
        edges = thin = 0
        for line in lines:
            for x in range(1, len(line) - 1):
                left, pixel, right = line[x - 1], line[x], line[x + 1]
                if not (left[3] and pixel[3] and right[3]):
                    continue
                differs = (left[:3] != pixel[:3], right[:3] != pixel[:3])
                edges += any(differs)
                thin += all(differs)
        if not edges:
            return None
        shares.append(thin / edges)
    return min(shares)


def problems(path, rows, scale):
    """The clean-art rules one runtime PNG at path breaks, each as a sentence."""
    folder = path.split("/")[0]
    limit = LOGO_COLOURS if path == "title/logo.png" else COLOUR_LIMITS.get(folder)
    found = []
    m = measure(rows, scale)
    cut = folder in CONCEPT_CUT_FOLDERS or path in CONCEPT_CHARACTERS or path in CONCEPT_INTERFACE
    if not cut and m["lone"] > LONE_LIMIT:
        found.append(f"{m['lone']:.2%} lone pixels, over {LONE_LIMIT:.0%}")
    if not cut and (limit is None or m["colours"] > limit):
        found.append(f"{m['colours']} colours, over the {folder} limit of {limit}")
    if folder in SCREEN_PIXEL_FOLDERS or path in CONCEPT_CHARACTERS:
        thin = thin_share(rows)
        if thin is None or thin < THIN_SHARE:
            share = "no" if thin is None else f"{thin:.0%} of"
            found.append(f"drawn in blocks: {share} edge pixels one pixel wide, under {THIN_SHARE:.0%}")
    if folder in SPRITE_FOLDERS and not cut:
        if m["edge"] is not None and m["edge"] < EDGE_SHARE:
            found.append(f"{m['edge']:.0%} of side and bottom edges darker than inside, under {EDGE_SHARE:.0%}")
        if m["lit"] < m["shaded"]:
            found.append(f"lit from below: up-facing {m['lit']:.0f} under down-facing {m['shaded']:.0f}")
    return found, m


def hd_problems(rows, size):
    """Native HD art keeps fine edges and the dimensions promised by its manifest."""
    found = []
    if [len(rows[0]), len(rows)] != size:
        found.append(f"actual size {[len(rows[0]), len(rows)]} differs from {size}")
    thin = thin_share(rows)
    if thin is None or thin < THIN_SHARE:
        found.append(f"native edge detail {thin} is under {THIN_SHARE}")
    return found


def check_hd(art_dir):
    """Check both HD owners without applying the legacy palette or cut-source rules."""
    failed = count = 0
    for manifest in (art_dir / "manifest.json", art_dir / "characters/manifest.json"):
        spec = json.loads(manifest.read_text())
        entries = spec.get("files", spec.get("frames", []))
        if not entries:
            raise ValueError(f"empty HD manifest: {manifest}")
        for entry in entries:
            path = manifest.parent / entry.get("path", entry.get("file"))
            rows = read_png(path)[2]
            found = hd_problems(rows, entry["size"])
            if hashlib.sha256(path.read_bytes()).hexdigest() != entry["sha256"]:
                found.append("PNG hash differs from the asset owner's manifest")
            print(f"{path}: " + ("FAILS: " + "; ".join(found) if found else "native size, hash and edge detail pass"))
            failed += bool(found)
            count += 1
    print(f"checked {count} HD PNGs listed in both manifests under {art_dir}: {failed} break a rule")
    return failed


def check(art_dir):
    """Measures every runtime PNG in the manifest and exits 1 when one breaks a clean-art rule."""
    art_dir = pathlib.Path(art_dir)
    if art_dir.name == "art_hd":
        if check_hd(art_dir):
            sys.exit(1)
        return
    files = json.loads((art_dir / "manifest.json").read_text())["files"]
    failed = 0
    for entry in files:
        found, m = problems(entry["path"], read_png(art_dir / entry["path"])[2], entry["scale"])
        edge = "-" if m["edge"] is None else f"{m['edge']:.0%}"
        print(
            f"{entry['path']}: lone {m['lone']:.2%}, {m['colours']} colours, edges darker {edge}, "
            f"up {m['lit']:.0f} / down {m['shaded']:.0f}" + ("" if not found else " FAILS: " + "; ".join(found))
        )
        failed += bool(found)
    print(f"checked {len(files)} PNGs listed in {art_dir / 'manifest.json'}: {failed} break a rule")
    hd = art_dir.with_name("art_hd")
    if hd.exists():
        failed += check_hd(hd)
    if failed:
        sys.exit(1)


def fit(rows, max_w, max_h):
    """Nearest-neighbour resize that keeps the aspect ratio inside max_w x max_h."""
    height, width = len(rows), len(rows[0])
    factor = min(max_w / width, max_h / height)
    new_w, new_h = max(1, int(width * factor)), max(1, int(height * factor))
    return [[rows[y * height // new_h][x * width // new_w] for x in range(new_w)] for y in range(new_h)]


def paste(canvas, rows, left, top):
    for y, row in enumerate(rows):
        for x, (r, g, b, a) in enumerate(row):
            if a:
                canvas[top + y][left + x] = (r, g, b, 255)


def mock_frame(spec, art_dir):
    """Composes review_mock placements on a 1280x720 canvas: [path, x, y, repeat_x, repeat_y]."""
    canvas = [[(0, 0, 0, 255)] * 1280 for _ in range(720)]
    for path, x, y, repeat_x, repeat_y in spec["review_mock"]:
        rows = read_png(pathlib.Path(art_dir) / path)[2]
        for j in range(repeat_y):
            for i in range(repeat_x):
                left, top = x + i * len(rows[0]), y + j * len(rows)
                clipped = [row[: max(0, 1280 - left)] for row in rows[: max(0, 720 - top)]]
                paste(canvas, clipped, left, top)
    return canvas


def closeups(spec, art_dir):
    """Pairs each asset cut from a repository image with its source crop, both 8x the sprite."""
    pairs = []
    for asset in spec["assets"]:
        source = spec["sources"][asset["source"]]
        if "file" not in source:
            continue
        sprite = upscale(read_png(pathlib.Path(art_dir) / asset["path"])[2], 8)
        side = len(sprite)
        original = fit(crop(read_png(REPOSITORY / source["file"])[2], asset["crop"]), side, side)
        pairs.append((original, sprite))
    return pairs


def before_after(spec, palette, raw_dir, art_dir):
    """Pairs the plain nearest-neighbour render of each review_before_after asset with its committed PNG, at 8x."""
    by_path = {asset["path"]: asset for asset in spec["assets"]}
    cache = {}
    pairs = []
    for path in spec["review_before_after"]:
        before, _ = render(by_path[path], raw_dir, palette, cache, spec["sources"], plain=True)
        after = read_png(pathlib.Path(art_dir) / path)[2]
        pairs.append((upscale(before, 8), upscale(after, 8)))
    return pairs


def sheet(raw_dir, art_dir, concept_dir, out):
    """Concept images, a mock frame, 8x close-ups (sheet poses beside sprites, then before beside
    after), then every asset on a grid."""
    spec, palette = load_spec()
    width, gap, cell = 1920, 16, 144
    concepts = [read_png(pathlib.Path(concept_dir) / f"{name}.png")[2] for name in spec["review_concepts"]]
    concept_w = (width - gap * (len(concepts) + 1)) // len(concepts)
    scaled = [fit(rows, concept_w, concept_w) for rows in concepts]
    concept_h = max(len(rows) for rows in scaled)
    mock = mock_frame(spec, art_dir)
    mock_top = gap * 2 + concept_h
    pairs = closeups(spec, art_dir) + before_after(spec, palette, raw_dir, art_dir)
    pair_w = max((len(a[0]) + len(b[0]) + gap for a, b in pairs), default=0)
    pair_h = max((len(b) for _, b in pairs), default=0)
    per_row = 2
    width = max(width, gap + per_row * (pair_w + gap * 3))
    closeup_top = mock_top + len(mock) + gap
    grid_top = closeup_top + -(-len(pairs) // per_row) * (pair_h + gap)
    columns = (width - gap) // (cell + gap)
    count = len(spec["assets"])
    grid_rows = -(-count // columns)
    height = grid_top + grid_rows * (cell + gap)
    background = (24, 18, 36, 255)
    canvas = [[background] * width for _ in range(height)]
    for i, rows in enumerate(scaled):
        paste(canvas, rows, gap + i * (concept_w + gap), gap)
    paste(canvas, mock, (width - len(mock[0])) // 2, mock_top)
    for i, (original, sprite) in enumerate(pairs):
        left = gap + (i % per_row) * (pair_w + gap * 3)
        top = closeup_top + (i // per_row) * (pair_h + gap)
        paste(canvas, original, left, top)
        paste(canvas, sprite, left + len(original[0]) + gap, top)
    tile = (40, 32, 56, 255)
    for i, asset in enumerate(spec["assets"]):
        left = gap + (i % columns) * (cell + gap)
        top = grid_top + (i // columns) * (cell + gap)
        for y in range(cell):
            canvas[top + y][left : left + cell] = [tile] * cell
        rows = read_png(pathlib.Path(art_dir) / asset["path"])[2]
        height_px, width_px = len(rows), len(rows[0])
        if width_px <= cell // 2 and height_px <= cell // 2:
            factor = min(cell // width_px, cell // height_px)
            rows = upscale(rows, factor)
        elif width_px > cell or height_px > cell:
            rows = fit(rows, cell, cell)
        paste(canvas, rows, left + (cell - len(rows[0])) // 2, top + (cell - len(rows)) // 2)
    write_png(out, canvas)
    print(
        f"wrote review sheet {out}: {len(concepts)} concepts, a mock frame, "
        f"{len(pairs)} close-ups, {count} assets from {SPEC.name}"
    )


def main(argv):
    commands = {"fetch": (fetch, 1), "process": (process, 2), "sheet": (sheet, 4), "check": (check, 1)}
    if len(argv) < 2 or argv[1] not in commands or len(argv) - 2 != commands[argv[1]][1]:
        sys.exit(__doc__)
    command, _ = commands[argv[1]]
    command(*argv[2:])


if __name__ == "__main__":
    main(sys.argv)
