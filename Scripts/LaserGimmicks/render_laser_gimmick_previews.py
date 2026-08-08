#!/usr/bin/env python3
"""Render quick software previews of the generated OBJ meshes for visual QA."""

from __future__ import annotations

import argparse
import math
from pathlib import Path

from PIL import Image, ImageDraw


COLORS = {
    "Shell": (210, 216, 211),
    "Mechanism": (25, 31, 39),
    "Optic": (255, 34, 18),
}


def add(a, b):
    return tuple(x + y for x, y in zip(a, b))


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def mul(a, scalar):
    return tuple(x * scalar for x in a)


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def normalized(value):
    length = math.sqrt(dot(value, value))
    return mul(value, 1.0 / max(length, 1.0e-8))


def parse_obj(path: Path):
    vertices = []
    triangles = []
    material = "Mechanism"
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith("v "):
            _, x, y, z = line.split()
            vertices.append((float(x), float(y), float(z)))
        elif line.startswith("usemtl "):
            material = line.split(maxsplit=1)[1]
        elif line.startswith("f "):
            indices = [int(token.split("/")[0]) - 1 for token in line.split()[1:]]
            triangles.append((indices, material))
    return vertices, triangles


def shade(color, amount):
    amount = max(0.22, min(1.15, amount))
    return tuple(max(0, min(255, int(channel * amount))) for channel in color)


def render_panel(draw, box, mesh_path, title, camera_vector):
    left, top, right_edge, bottom = box
    vertices, triangles = parse_obj(mesh_path)
    minimum = tuple(min(vertex[axis] for vertex in vertices) for axis in range(3))
    maximum = tuple(max(vertex[axis] for vertex in vertices) for axis in range(3))
    center = mul(add(minimum, maximum), 0.5)

    view = normalized(camera_vector)
    screen_right = normalized(cross((0.0, 0.0, 1.0), view))
    screen_up = normalized(cross(view, screen_right))
    projected = [
        (dot(sub(vertex, center), screen_right), dot(sub(vertex, center), screen_up), dot(sub(vertex, center), view))
        for vertex in vertices
    ]
    max_projected = max(max(abs(point[0]), abs(point[1])) for point in projected)
    scale = min(right_edge - left, bottom - top) * 0.38 / max(max_projected, 1.0)
    panel_center = ((left + right_edge) * 0.5, (top + bottom) * 0.5 + 8.0)

    light = normalized(add(view, (0.0, 0.0, 1.4)))
    draw_triangles = []
    for indices, material in triangles:
        points_3d = [vertices[index] for index in indices]
        normal = normalized(cross(sub(points_3d[1], points_3d[0]), sub(points_3d[2], points_3d[0])))
        lighting = 0.42 + 0.58 * abs(dot(normal, light))
        points_2d = [
            (panel_center[0] + projected[index][0] * scale, panel_center[1] - projected[index][1] * scale)
            for index in indices
        ]
        depth = sum(projected[index][2] for index in indices) / 3.0
        draw_triangles.append((depth, points_2d, shade(COLORS.get(material, COLORS["Mechanism"]), lighting)))

    for _, points, color in sorted(draw_triangles, key=lambda item: item[0]):
        draw.polygon(points, fill=color)

    draw.rectangle(box, outline=(62, 72, 82), width=2)
    draw.text((left + 16, top + 14), title, fill=(235, 240, 244))
    bounds_text = (
        f"{maximum[0] - minimum[0]:.0f} x {maximum[1] - minimum[1]:.0f} x "
        f"{maximum[2] - minimum[2]:.0f} cm"
    )
    draw.text((left + 16, bottom - 28), bounds_text, fill=(150, 161, 170))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    project_root = Path(__file__).resolve().parents[2]
    parser.add_argument("--source", type=Path, default=project_root / "SourceArt" / "LaserGimmicks")
    parser.add_argument("--output", type=Path, default=project_root / "Saved" / "LaserGimmickPreview.png")
    arguments = parser.parse_args()

    image = Image.new("RGB", (1536, 640), (17, 21, 25))
    draw = ImageDraw.Draw(image)
    panels = (
        ("SM_LaserEmitter", "Laser Emitter", (1.7, 2.1, 1.25)),
        ("SM_LaserReceiver", "Laser Receiver", (2.3, 1.2, 0.85)),
        ("SM_LaserRedirectorCube", "Laser Redirector", (1.8, 2.0, 1.45)),
    )
    for index, (filename, title, camera) in enumerate(panels):
        render_panel(
            draw,
            (16 + index * 507, 48, 498 + index * 507, 624),
            arguments.source / f"{filename}.obj",
            title,
            camera,
        )
    draw.text((18, 16), "Portal-inspired laser gimmick source-mesh preview", fill=(188, 199, 208))
    arguments.output.parent.mkdir(parents=True, exist_ok=True)
    image.save(arguments.output)
    print(arguments.output)


if __name__ == "__main__":
    main()
