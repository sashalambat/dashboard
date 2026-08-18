#!/usr/bin/env python3
"""Pack CC0 GitHub art into Content/art for the SBS Wars raycaster."""
from __future__ import annotations

import json
import math
import struct
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SRC = Path("/tmp/sbs-assets")
OUT = ROOT / "Content" / "art"


def ensure(p: Path) -> Path:
    p.mkdir(parents=True, exist_ok=True)
    return p


def save_rgba(im: Image.Image, path: Path) -> None:
    ensure(path.parent)
    im.convert("RGBA").save(path, "PNG", optimize=True)


def tile_to(src: Path, size: int) -> Image.Image:
    im = Image.open(src).convert("RGBA")
    if im.width < 2 or im.height < 2:
        canvas = Image.new("RGBA", (size, size), (40, 40, 40, 255))
        return canvas
    tile = im.resize((max(8, im.width * 4), max(8, im.height * 4)), Image.NEAREST)
    canvas = Image.new("RGBA", (size, size), (0, 0, 0, 255))
    for y in range(0, size, tile.height):
        for x in range(0, size, tile.width):
            canvas.paste(tile, (x, y))
    return canvas


def scale_sprite(src: Path, box: tuple[int, int], pad: int = 4) -> Image.Image:
    im = Image.open(src).convert("RGBA")
    # trim mostly-empty borders
    bbox = im.getbbox()
    if bbox:
        im = im.crop(bbox)
    w, h = box
    scale = min((w - pad * 2) / max(1, im.width), (h - pad * 2) / max(1, im.height))
    nw = max(1, int(im.width * scale))
    nh = max(1, int(im.height * scale))
    im = im.resize((nw, nh), Image.NEAREST)
    canvas = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    canvas.paste(im, ((w - nw) // 2, h - nh - pad), im)
    return canvas


def split_atlas(path: Path, tile: int) -> list[Image.Image]:
    im = Image.open(path).convert("RGBA")
    cols = im.width // tile
    rows = im.height // tile
    out = []
    for y in range(rows):
        for x in range(cols):
            out.append(im.crop((x * tile, y * tile, (x + 1) * tile, (y + 1) * tile)))
    return out


def read_glb(path: Path):
    data = path.read_bytes()
    magic, version, length = struct.unpack_from("<4sII", data, 0)
    if magic != b"glTF":
        raise RuntimeError(f"not glb: {path}")
    off = 12
    js = None
    blob = b""
    while off + 8 <= len(data):
        clen, ctype = struct.unpack_from("<I4s", data, off)
        chunk = data[off + 8 : off + 8 + clen]
        if ctype == b"JSON":
            js = json.loads(chunk)
        elif ctype.startswith(b"BIN"):
            blob = chunk
        off += 8 + clen
    return js, blob


COMP = {5120: 1, 5121: 1, 5122: 2, 5123: 2, 5125: 4, 5126: 4}
NCOMP = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4}
UNPACK = {5120: "b", 5121: "B", 5122: "h", 5123: "H", 5125: "I", 5126: "f"}


def accessor_data(js, blob, index):
    acc = js["accessors"][index]
    view = js["bufferViews"][acc["bufferView"]]
    off = view.get("byteOffset", 0) + acc.get("byteOffset", 0)
    n = acc["count"]
    ctype = acc["componentType"]
    ncomp = NCOMP[acc["type"]]
    stride = view.get("byteStride", COMP[ctype] * ncomp)
    fmt = UNPACK[ctype]
    out = []
    for i in range(n):
        base = off + i * stride
        tup = struct.unpack_from("<" + fmt * ncomp, blob, base)
        out.append(tup if ncomp > 1 else tup[0])
    return out


def quat_to_m(q):
    x, y, z, w = q
    return [
        [1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
        [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
        [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)],
    ]


def mulm(a, b):
    r = [[0.0] * 4 for _ in range(4)]
    for i in range(4):
        for j in range(4):
            r[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j] + a[i][3] * b[3][j]
    return r


def ident():
    return [[1.0 if i == j else 0.0 for j in range(4)] for i in range(4)]


def node_matrix(node):
    if "matrix" in node:
        m = node["matrix"]
        return [[m[c * 4 + r] for c in range(4)] for r in range(4)]
    t = node.get("translation", [0, 0, 0])
    s = node.get("scale", [1, 1, 1])
    q = node.get("rotation", [0, 0, 0, 1])
    rm = quat_to_m(q)
    m = ident()
    for i in range(3):
        for j in range(3):
            m[i][j] = rm[i][j] * s[j]
        m[i][3] = t[i]
    return m


def xform(m, p):
    x, y, z = p
    w = m[3][0] * x + m[3][1] * y + m[3][2] * z + m[3][3]
    if abs(w) < 1e-8:
        w = 1.0
    return (
        (m[0][0] * x + m[0][1] * y + m[0][2] * z + m[0][3]) / w,
        (m[1][0] * x + m[1][1] * y + m[1][2] * z + m[1][3]) / w,
        (m[2][0] * x + m[2][1] * y + m[2][2] * z + m[2][3]) / w,
    )


def collect_tris(js, blob):
    nodes = js.get("nodes", [])
    children = {i: node.get("children", []) for i, node in enumerate(nodes)}
    roots = set(range(len(nodes)))
    for ch in children.values():
        for c in ch:
            roots.discard(c)
    if not roots:
        roots = set(range(len(nodes)))
    tris = []

    def walk(i, parent):
        m = mulm(parent, node_matrix(nodes[i]))
        mesh_id = nodes[i].get("mesh")
        if mesh_id is not None:
            mesh = js["meshes"][mesh_id]
            for prim in mesh.get("primitives", []):
                attrs = prim["attributes"]
                if "POSITION" not in attrs:
                    continue
                pos = accessor_data(js, blob, attrs["POSITION"])
                uv = accessor_data(js, blob, attrs["TEXCOORD_0"]) if "TEXCOORD_0" in attrs else [(0.5, 0.5)] * len(pos)
                idx = accessor_data(js, blob, prim["indices"]) if "indices" in prim else list(range(len(pos)))
                world = [xform(m, p) for p in pos]
                for t in range(0, len(idx) - 2, 3):
                    a, b, c = idx[t], idx[t + 1], idx[t + 2]
                    tris.append((world[a], world[b], world[c], uv[a], uv[b], uv[c]))
        for c in children.get(i, []):
            walk(c, m)

    for r in sorted(roots):
        walk(r, ident())
    return tris


def raster_mesh(glb: Path, colormap: Image.Image, size: tuple[int, int], mode: str) -> Image.Image:
    js, blob = read_glb(glb)
    tris = collect_tris(js, blob)
    if not tris:
        return Image.new("RGBA", size, (0, 0, 0, 0))
    xs = [p[0] for tri in tris for p in tri[:3]]
    ys = [p[1] for tri in tris for p in tri[:3]]
    zs = [p[2] for tri in tris for p in tri[:3]]
    cx = (min(xs) + max(xs)) * 0.5
    cy = (min(ys) + max(ys)) * 0.5
    cz = (min(zs) + max(zs)) * 0.5
    span = max(max(xs) - min(xs), max(ys) - min(ys), max(zs) - min(zs), 0.001)

    w, h = size
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    px = img.load()
    zbuf = [1e9] * (w * h)
    cw, ch = colormap.size
    cmap = colormap.load()

    yaw = 0.55 if mode == "weapon" else 0.15
    pitch = 0.35 if mode == "weapon" else 0.05
    cyaw, syaw = math.cos(yaw), math.sin(yaw)
    cp, sp = math.cos(pitch), math.sin(pitch)

    def project(p):
        x, y, z = p[0] - cx, p[1] - cy, p[2] - cz
        # yaw around Y, then pitch
        xz = x * cyaw - z * syaw
        zz = x * syaw + z * cyaw
        y2 = y * cp - zz * sp
        z2 = y * sp + zz * cp
        scale = 0.82 * min(w, h) / span
        sx = w * (0.52 if mode == "weapon" else 0.5) + xz * scale
        sy = h * (0.62 if mode == "weapon" else 0.55) - y2 * scale
        return sx, sy, z2

    def sample(u, v):
        u = u - math.floor(u)
        v = v - math.floor(v)
        x = min(cw - 1, max(0, int(u * (cw - 1))))
        y = min(ch - 1, max(0, int((1.0 - v) * (ch - 1))))
        return cmap[x, y]

    def edge(ax, ay, bx, by, cx, cy):
        return (cx - ax) * (by - ay) - (cy - ay) * (bx - ax)

    for a, b, c, ua, ub, uc in tris:
        pa, pb, pc = project(a), project(b), project(c)
        minx = max(0, int(min(pa[0], pb[0], pc[0])))
        maxx = min(w - 1, int(max(pa[0], pb[0], pc[0])))
        miny = max(0, int(min(pa[1], pb[1], pc[1])))
        maxy = min(h - 1, int(max(pa[1], pb[1], pc[1])))
        area = edge(pa[0], pa[1], pb[0], pb[1], pc[0], pc[1])
        if abs(area) < 1e-6:
            continue
        for y in range(miny, maxy + 1):
            for x in range(minx, maxx + 1):
                w0 = edge(pb[0], pb[1], pc[0], pc[1], x + 0.5, y + 0.5)
                w1 = edge(pc[0], pc[1], pa[0], pa[1], x + 0.5, y + 0.5)
                w2 = edge(pa[0], pa[1], pb[0], pb[1], x + 0.5, y + 0.5)
                if area < 0:
                    w0, w1, w2, area = -w0, -w1, -w2, -area
                if w0 < 0 or w1 < 0 or w2 < 0:
                    continue
                w0 /= area
                w1 /= area
                w2 /= area
                z = pa[2] * w0 + pb[2] * w1 + pc[2] * w2
                di = y * w + x
                if z >= zbuf[di]:
                    continue
                u = ua[0] * w0 + ub[0] * w1 + uc[0] * w2
                v = ua[1] * w0 + ub[1] * w1 + uc[1] * w2
                col = sample(u, v)
                if len(col) > 3 and col[3] < 8:
                    continue
                zbuf[di] = z
                px[x, y] = col if len(col) == 4 else (col[0], col[1], col[2], 255)
    return img


def tint(im: Image.Image, rgb: tuple[int, int, int], amount: float) -> Image.Image:
    src = im.convert("RGBA")
    out = Image.new("RGBA", src.size)
    sp = src.load()
    dp = out.load()
    for y in range(src.height):
        for x in range(src.width):
            r, g, b, a = sp[x, y]
            if a == 0:
                continue
            dp[x, y] = (
                int(r * (1 - amount) + rgb[0] * amount),
                int(g * (1 - amount) + rgb[1] * amount),
                int(b * (1 - amount) + rgb[2] * amount),
                a,
            )
    return out


def main() -> int:
    if not SRC.exists():
        print("Missing /tmp/sbs-assets. Clone Kenney / sci-fi packs first.", file=sys.stderr)
        return 1
    walls = ensure(OUT / "walls")
    floors = ensure(OUT / "floors")
    chars = ensure(OUT / "chars")
    weapons = ensure(OUT / "weapons")
    fx = ensure(OUT / "fx")
    sky = ensure(OUT / "sky")
    pickups = ensure(OUT / "pickups")

    sci = SRC / "scifi"
    ken = SRC / "kenney-fps"
    tiny = SRC / "tinyraycaster"

    # Walls: sci-fi metal/pattern tiles + tinyraycaster atlas
    wall_src = sorted((sci / "Interior" / "interiorWalls").glob("textureMetal*.png"))
    wall_src += sorted((sci / "Interior" / "interiorWalls").glob("texturePattern*.png"))
    n = 0
    for src in wall_src[:10]:
        save_rgba(tile_to(src, 64), walls / f"{n:02d}.png")
        n += 1
    for i, tile in enumerate(split_atlas(tiny / "walltext.png", 64)):
        save_rgba(tile.resize((64, 64), Image.NEAREST), walls / f"{n:02d}.png")
        n += 1
        if n >= 16:
            break

    floor_src = [
        sci / "Interior" / "floor" / "floorMetal1.png",
        sci / "Interior" / "floor" / "floorTile1.png",
        sci / "Interior" / "floor" / "floorTile2.png",
        sci / "Interior" / "floor" / "floorVent.png",
        sci / "Interior" / "floor" / "floorWood1.png",
        sci / "Exterior" / "Foreground" / "floors" / "floorTextureIndustrial.png",
    ]
    for i, src in enumerate(floor_src):
        if src.exists():
            save_rgba(tile_to(src, 64), floors / f"{i:02d}.png")

    # Characters
    save_rgba(scale_sprite(sci / "NPCs" / "Guard" / "guardIdle1.png", (80, 128)), chars / "dominion.png")
    save_rgba(scale_sprite(sci / "NPCs" / "Guard" / "guardWarn1.png", (80, 128)), chars / "dominion_fire.png")
    save_rgba(scale_sprite(sci / "Player Character" / "walkRight8.png", (64, 128)), chars / "alliance.png")
    save_rgba(scale_sprite(sci / "Player Character" / "hack4.png", (64, 128)), chars / "alliance_fire.png")
    drone = sci / "NPCs" / "Drones" / "_0003s_0000_Drone-Basic-Forward.png"
    if drone.exists():
        save_rgba(scale_sprite(drone, (80, 80), pad=8), chars / "drone.png")

    # Kenney GLB weapons / enemy
    cmap = Image.open(ken / "models" / "Textures" / "colormap.png").convert("RGBA")
    blaster = raster_mesh(ken / "models" / "blaster.glb", cmap, (220, 140), "weapon")
    repeater = raster_mesh(ken / "models" / "blaster-repeater.glb", cmap, (220, 140), "weapon")
    enemy = raster_mesh(ken / "models" / "enemy-flying.glb", cmap, (96, 96), "char")
    save_rgba(blaster, weapons / "blaster.png")
    save_rgba(repeater, weapons / "repeater.png")
    save_rgba(enemy, chars / "kenney_enemy.png")
    tints = [
        (255, 150, 40),
        (180, 120, 50),
        (90, 90, 90),
        (40, 120, 60),
        (40, 200, 220),
        (200, 60, 40),
        (160, 80, 220),
        (220, 200, 60),
        (60, 160, 70),
    ]
    for i, col in enumerate(tints):
        base = blaster if i % 2 == 0 else repeater
        save_rgba(tint(base, col, 0.28), weapons / f"{i}.png")

    # FX / sky / pickups
    save_rgba(Image.open(ken / "sprites" / "crosshair.png").convert("RGBA").resize((32, 32), Image.NEAREST), fx / "crosshair.png")
    burst = Image.open(ken / "sprites" / "burst.png").convert("RGBA")
    save_rgba(burst.resize((96, 48), Image.BILINEAR), fx / "burst.png")
    skybox = Image.open(ken / "sprites" / "skybox.png").convert("RGBA")
    save_rgba(skybox.resize((512, 128), Image.BILINEAR), sky / "sky.png")
    for i, crate in enumerate(sorted((sci / "Interior" / "Crates").glob("crate*.png"))[:6]):
        save_rgba(scale_sprite(crate, (48, 48), pad=2), pickups / f"{i}.png")
    for i, vend in enumerate(
        [
            sci / "Interior" / "vending" / "colaMachine2.png",
            sci / "Interior" / "vending" / "atm1.png",
            sci / "Interior" / "vending" / "beerMachine.png",
        ]
    ):
        if vend.exists():
            save_rgba(scale_sprite(vend, (64, 96), pad=2), pickups / f"prop{i}.png")

    credits = ROOT / "Content" / "CREDITS.md"
    credits.write_text(
        """# Art credits

SBS Wars ships CC0 / free game art from these GitHub repositories:

- [KenneyNL/Starter-Kit-FPS](https://github.com/KenneyNL/Starter-Kit-FPS) — blaster/repeater weapons, flying enemy, skybox, crosshair, muzzle burst (CC0)
- [KMAlexander/ScifiGameAssetSet](https://github.com/KMAlexander/ScifiGameAssetSet) — station wall/floor tiles, guards, player, drones, crates
- [ssloy/tinyraycaster](https://github.com/ssloy/tinyraycaster) — classic raycaster wall atlas
- [nothings/stb](https://github.com/nothings/stb) — `stb_image` PNG loader (public domain)

Kenney assets are CC0 1.0. Sci-fi tiles are free for any use per the upstream readme.
""",
        encoding="utf-8",
    )
    print("Packed art into", OUT)
    for d in [walls, floors, chars, weapons, fx, sky, pickups]:
        print(f"  {d.name}: {len(list(d.glob('*.png')))} png")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
