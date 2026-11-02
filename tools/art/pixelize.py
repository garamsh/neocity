"""Turn Recraft generations into the monster-mouth escape runtime PNGs.

Subcommands, all driven by assets.json beside this file:
  fetch RAW_DIR          download every source and decode it to PNG with ImageMagick
  process RAW_DIR ART    write every runtime PNG under ART
  sheet ART CONCEPT_DIR OUT
                         write the review sheet: the concept images, a mock frame, every asset

fetch needs ImageMagick's magick to decode WebP; process and sheet use only the standard
library, so the same raw PNGs always give the same bytes.
"""

import json
import pathlib
import struct
import subprocess
import sys
import urllib.request
import zlib
from collections import deque

SPEC = pathlib.Path(__file__).with_name("assets.json")


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
    x, y, w, h = box
    return [row[x : x + w] for row in rows[y : y + h]]


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


def upscale(rows, factor):
    out = []
    for row in rows:
        line = [pixel for pixel in row for _ in range(factor)]
        out.extend(list(line) for _ in range(factor))
    return out


def render(asset, raw_dir, palette, cache):
    source = asset["source"]
    if source not in cache:
        cache[source] = read_png(pathlib.Path(raw_dir) / f"{source}.png")[2]
    rows = crop(cache[source], asset["crop"])
    mask = None
    key = asset.get("key")
    if key and key.get("alpha"):
        mask = [bytearray(1 if pixel[3] < 128 else 0 for pixel in row) for row in rows]
    elif key:
        mask = background_mask(rows, hex_colour(key["colour"]), key["tolerance"], key.get("enclosed", False))
    if mask is not None and asset.get("largest_only"):
        mask = keep_largest(mask)
    width, height = asset["size"]
    scale = asset.get("scale", 1)
    small = downscale(rows, mask, width // scale, height // scale)
    small = snap(small, palette)
    if mask is not None:
        small = drop_specks(small)
    return upscale(small, scale)


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
    for asset in spec["assets"]:
        write_png(pathlib.Path(art_dir) / asset["path"], render(asset, raw_dir, palette, cache))
    print(f"wrote {len(spec['assets'])} runtime PNGs listed in {SPEC.name} under {art_dir}")


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


def sheet(art_dir, concept_dir, out):
    """Concept images across the top, a mock frame from the assets, then every asset on a grid."""
    spec, _ = load_spec()
    width, gap, cell = 1920, 16, 144
    concepts = [read_png(pathlib.Path(concept_dir) / f"{name}.png")[2] for name in spec["review_concepts"]]
    concept_w = (width - gap * (len(concepts) + 1)) // len(concepts)
    scaled = [fit(rows, concept_w, concept_w) for rows in concepts]
    concept_h = max(len(rows) for rows in scaled)
    mock = mock_frame(spec, art_dir)
    mock_top = gap * 2 + concept_h
    grid_top = mock_top + len(mock) + gap
    columns = (width - gap) // (cell + gap)
    count = len(spec["assets"])
    grid_rows = -(-count // columns)
    height = grid_top + grid_rows * (cell + gap)
    background = (24, 18, 36, 255)
    canvas = [[background] * width for _ in range(height)]
    for i, rows in enumerate(scaled):
        paste(canvas, rows, gap + i * (concept_w + gap), gap)
    paste(canvas, mock, (width - len(mock[0])) // 2, mock_top)
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
    print(f"wrote review sheet {out}: {len(concepts)} concepts, a mock frame, {count} assets from {SPEC.name}")


def main(argv):
    commands = {"fetch": (fetch, 1), "process": (process, 2), "sheet": (sheet, 3)}
    if len(argv) < 2 or argv[1] not in commands or len(argv) - 2 != commands[argv[1]][1]:
        sys.exit(__doc__)
    command, _ = commands[argv[1]]
    command(*argv[2:])


if __name__ == "__main__":
    main(sys.argv)
