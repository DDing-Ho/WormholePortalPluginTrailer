#!/usr/bin/env python3
"""Generate the deterministic Energy Ball scorch decal mask as a 32-bit TGA."""

from __future__ import annotations

import argparse
import math
import struct
from pathlib import Path


DEFAULT_SIZE = 512


def smoothstep(edge0: float, edge1: float, value: float) -> float:
    if edge0 == edge1:
        return 0.0
    t = max(0.0, min(1.0, (value - edge0) / (edge1 - edge0)))
    return t * t * (3.0 - 2.0 * t)


def hash_noise(x: int, y: int, seed: int) -> float:
    value = (x * 0x1F123BB5) ^ (y * 0x05491333) ^ (seed * 0x45D9F3B)
    value ^= value >> 16
    value = (value * 0x45D9F3B) & 0xFFFFFFFF
    value ^= value >> 16
    return (value & 0xFFFF) / 65535.0


def value_noise(x: float, y: float, frequency: float, seed: int) -> float:
    sample_x = x * frequency
    sample_y = y * frequency
    x0 = math.floor(sample_x)
    y0 = math.floor(sample_y)
    tx = smoothstep(0.0, 1.0, sample_x - x0)
    ty = smoothstep(0.0, 1.0, sample_y - y0)
    a = hash_noise(x0, y0, seed)
    b = hash_noise(x0 + 1, y0, seed)
    c = hash_noise(x0, y0 + 1, seed)
    d = hash_noise(x0 + 1, y0 + 1, seed)
    top = a + (b - a) * tx
    bottom = c + (d - c) * tx
    return top + (bottom - top) * ty


def make_pixel(x: int, y: int, size: int) -> tuple[int, int, int, int]:
    nx = ((x + 0.5) / size) * 2.0 - 1.0
    ny = ((y + 0.5) / size) * 2.0 - 1.0
    radius = math.sqrt(nx * nx + ny * ny)
    angle = math.atan2(ny, nx)

    broad_noise = value_noise(nx + 2.0, ny + 2.0, 3.5, 17)
    detail_noise = value_noise(nx + 3.0, ny + 3.0, 11.0, 43)
    grit_noise = hash_noise(x, y, 91)
    irregular_edge = 0.82 + (broad_noise - 0.5) * 0.18 + math.sin(angle * 7.0 + broad_noise * 4.0) * 0.025
    edge_mask = 1.0 - smoothstep(irregular_edge - 0.12, irregular_edge + 0.035, radius)

    crater_ring = math.exp(-((radius - 0.43) / 0.16) ** 2)
    center_soot = 1.0 - smoothstep(0.0, 0.72, radius)
    fractured_ring = crater_ring * (0.55 + detail_noise * 0.65)
    soot = edge_mask * (0.32 + center_soot * 0.35 + fractured_ring * 0.60 + detail_noise * 0.20)

    if 0.48 < radius < irregular_edge and grit_noise > 0.965:
        soot = max(soot, (grit_noise - 0.965) * 18.0)
    alpha = int(round(max(0.0, min(1.0, soot)) * 255.0))
    return 9, 5, 3, alpha


def write_tga(path: Path, size: int) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    # Uncompressed BGRA, top-left origin, 8 alpha bits.
    header = struct.pack(
        "<BBBHHBHHHHBB",
        0, 0, 2, 0, 0, 0, 0, 0, size, size, 32, 0x28,
    )
    pixels = bytearray()
    for y in range(size):
        for x in range(size):
            red, green, blue, alpha = make_pixel(x, y, size)
            pixels.extend((blue, green, red, alpha))
    path.write_bytes(header + pixels)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output",
        type=Path,
        default=Path(__file__).resolve().parents[2]
        / "SourceArt"
        / "EnergyBallGimmicks"
        / "T_EnergyBallScorch.tga",
    )
    parser.add_argument("--size", type=int, default=DEFAULT_SIZE)
    arguments = parser.parse_args()
    if arguments.size < 32 or arguments.size > 4096:
        raise ValueError("Texture size must be between 32 and 4096 pixels")
    write_tga(arguments.output.resolve(), arguments.size)
    print(arguments.output.resolve())


if __name__ == "__main__":
    main()
