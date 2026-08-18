#!/usr/bin/env python3
"""Generate SBS Wars icon PNG and ICO (no third-party deps)."""
import struct
import zlib
from pathlib import Path


def chunk(tag: bytes, data: bytes) -> bytes:
    crc = zlib.crc32(tag + data) & 0xFFFFFFFF
    return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", crc)


def write_png(path: Path, w: int, h: int, rgba: list) -> None:
    raw = bytearray()
    for y in range(h):
        raw.append(0)
        raw.extend(rgba[y * w * 4 : (y + 1) * w * 4])
    ihdr = struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(bytes(raw), 9)) + chunk(b"IEND", b"")
    path.write_bytes(png)


def pixel(buf: bytearray, w: int, x: int, y: int, r: int, g: int, b: int, a: int = 255) -> None:
    if 0 <= x < w and 0 <= y < w:
        i = (y * w + x) * 4
        buf[i : i + 4] = bytes((r, g, b, a))


def fill_rect(buf, w, x0, y0, x1, y1, c):
    for y in range(y0, y1):
        for x in range(x0, x1):
            pixel(buf, w, x, y, *c)


def fill_circle(buf, w, cx, cy, rad, c):
    r2 = rad * rad
    for y in range(cy - rad, cy + rad + 1):
        for x in range(cx - rad, cx + rad + 1):
            if (x - cx) * (x - cx) + (y - cy) * (y - cy) <= r2:
                pixel(buf, w, x, y, *c)


def draw_icon(w: int = 256) -> bytearray:
    buf = bytearray(w * w * 4)
    # dark sci-fi plate
    for y in range(w):
        for x in range(w):
            t = y / (w - 1)
            r = int(12 + t * 18)
            g = int(14 + t * 10)
            b = int(20 + (1 - t) * 18)
            pixel(buf, w, x, y, r, g, b, 255)

    # shield
    fill_circle(buf, w, 128, 118, 96, (28, 32, 40, 255))
    fill_circle(buf, w, 128, 118, 88, (18, 22, 28, 255))
    # orange alliance wedge
    for y in range(40, 200):
        for x in range(40, 128):
            if (x - 128) ** 2 + (y - 118) ** 2 <= 80 ** 2:
                pixel(buf, w, x, y, 255, 140, 40, 255)
    # cyan dominion wedge
    for y in range(40, 200):
        for x in range(128, 216):
            if (x - 128) ** 2 + (y - 118) ** 2 <= 80 ** 2:
                pixel(buf, w, x, y, 40, 200, 255, 255)
    # center bar
    fill_rect(buf, w, 120, 48, 136, 188, (245, 245, 245, 255))
    # SBS block letters (simple 5x7)
    glyphs = {
        "S": ["01110", "10000", "01110", "00001", "01110"],
        "B": ["11110", "10001", "11110", "10001", "11110"],
        "W": ["10001", "10001", "10101", "11011", "10001"],
    }
    text = "SBS"
    ox, oy, s = 58, 208, 6
    for gi, ch in enumerate(text):
        g = glyphs[ch]
        for row, bits in enumerate(g):
            for col, bit in enumerate(bits):
                if bit == "1":
                    fill_rect(
                        buf,
                        w,
                        ox + gi * (s * 6) + col * s,
                        oy + row * s,
                        ox + gi * (s * 6) + col * s + s - 1,
                        oy + row * s + s - 1,
                        (255, 160, 50, 255),
                    )
    return buf


def write_ico(path: Path, png: bytes) -> None:
    # Vista+ ICO can embed a PNG
    count = 1
    header = struct.pack("<HHH", 0, 1, count)
    entry = struct.pack("<BBBBHHII", 0, 0, 0, 0, 1, 32, len(png), 6 + 16)
    path.write_bytes(header + entry + png)


def main() -> None:
    root = Path(__file__).resolve().parents[2]
    ui = root / "UI"
    linux = root / "Packaging" / "linux"
    windows = root / "Packaging" / "windows"
    ui.mkdir(parents=True, exist_ok=True)
    linux.mkdir(parents=True, exist_ok=True)
    windows.mkdir(parents=True, exist_ok=True)
    buf = draw_icon(256)
    png_path = ui / "sbswars.png"
    write_png(png_path, 256, 256, buf)
    png_bytes = png_path.read_bytes()
    linux.joinpath("sbswars.png").write_bytes(png_bytes)
    write_ico(linux / "sbswars.ico", png_bytes)
    write_ico(windows / "sbswars.ico", png_bytes)
    print("wrote", png_path)
    print("wrote", linux / "sbswars.ico")
    print("wrote", windows / "sbswars.ico")


if __name__ == "__main__":
    main()
