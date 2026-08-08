"""Render a deterministic software preview of the generated Energy Ball devices."""

from __future__ import annotations

import argparse
import importlib.util
import sys
from pathlib import Path

from PIL import Image, ImageDraw


def load_preview_tools():
    helper_path = Path(__file__).resolve().parents[1] / "LaserGimmicks" / "render_laser_gimmick_previews.py"
    specification = importlib.util.spec_from_file_location("laser_gimmick_preview_tools", helper_path)
    if specification is None or specification.loader is None:
        raise RuntimeError(f"Could not load preview helpers: {helper_path}")
    module = importlib.util.module_from_spec(specification)
    sys.modules[specification.name] = module
    specification.loader.exec_module(module)
    return module


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    project_root = Path(__file__).resolve().parents[2]
    parser.add_argument("--source", type=Path, default=project_root / "SourceArt" / "EnergyBallGimmicks")
    parser.add_argument("--output", type=Path, default=project_root / "Saved" / "EnergyBallGimmickPreview.png")
    arguments = parser.parse_args()

    tools = load_preview_tools()
    image = Image.new("RGB", (1040, 650), (17, 21, 25))
    draw = ImageDraw.Draw(image)
    panels = (
        ("SM_EnergyBallEmitter", "Energy Ball Emitter", (1.7, 2.1, 1.2)),
        ("SM_EnergyBallReceiver", "Energy Ball Receiver", (1.9, 2.0, 1.15)),
    )
    for index, (filename, title, camera) in enumerate(panels):
        tools.render_panel(
            draw,
            (16 + index * 507, 48, 498 + index * 507, 634),
            arguments.source / f"{filename}.obj",
            title,
            camera,
        )
    draw.text((18, 16), "Portal-inspired Energy Ball device source-mesh preview", fill=(188, 199, 208))
    arguments.output.parent.mkdir(parents=True, exist_ok=True)
    image.save(arguments.output)
    print(arguments.output)


if __name__ == "__main__":
    main()
