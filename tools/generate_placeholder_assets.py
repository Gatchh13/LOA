#!/usr/bin/env python3
"""
generate_placeholder_assets.py

Generates the three placeholder PNG images required by the bootstrap:
  - grass.png   (16x16 green tile)
  - wall.png    (16x16 brown tile)
  - player.png  (16x16 red sprite with white dot)

Output: assets/gfx/  (relative to project root)

Requirements:
  pip install Pillow

Run from the project root:
  python3 tools/generate_placeholder_assets.py
"""

import os
from pathlib import Path

try:
    from PIL import Image, ImageDraw
except ImportError:
    print("ERROR: Pillow not installed. Run:  pip install Pillow")
    raise SystemExit(1)


OUTPUT_DIR = Path(__file__).parent.parent / "assets" / "gfx"
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)


def make_grass():
    img = Image.new("RGBA", (16, 16), (80, 140, 60, 255))
    draw = ImageDraw.Draw(img)
    # Subtle texture lines
    for x in range(0, 16, 4):
        draw.line([(x, 0), (x, 15)], fill=(70, 120, 50, 100))
    img.save(OUTPUT_DIR / "grass.png")
    print("  grass.png")


def make_wall():
    img = Image.new("RGBA", (16, 16), (100, 80, 60, 255))
    draw = ImageDraw.Draw(img)
    # Simple brick pattern
    draw.rectangle([0, 0, 15, 7],   fill=(110, 88, 66, 255))
    draw.rectangle([0, 8, 15, 15],  fill=(95,  75, 55, 255))
    draw.line([(8, 0), (8, 7)],   fill=(70, 55, 40, 255))
    draw.line([(0, 8), (16, 8)],  fill=(70, 55, 40, 255))
    draw.line([(4, 8), (4, 15)],  fill=(70, 55, 40, 255))
    draw.line([(12, 8), (12, 15)],fill=(70, 55, 40, 255))
    img.save(OUTPUT_DIR / "wall.png")
    print("  wall.png")


def make_player():
    img = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    # Body
    draw.rectangle([1, 1, 14, 14], fill=(200, 80, 80, 255))
    # Eyes
    draw.rectangle([3, 4, 5, 6],   fill=(255, 255, 255, 255))
    draw.rectangle([10, 4, 12, 6], fill=(255, 255, 255, 255))
    # Outline
    draw.rectangle([1, 1, 14, 14], outline=(140, 40, 40, 255))
    img.save(OUTPUT_DIR / "player.png")
    print("  player.png")


if __name__ == "__main__":
    print(f"Generating placeholder assets in: {OUTPUT_DIR}")
    make_grass()
    make_wall()
    make_player()
    print()
    print("Done.")
    print()
    print("Next step: pack into tiles.t3x using tex3ds:")
    print()
    print("  tex3ds -f rgba8888 -z auto \\")
    print("    --atlas assets/gfx/grass.png \\")
    print("           assets/gfx/wall.png \\")
    print("           assets/gfx/player.png \\")
    print("    -o romfs/gfx/tiles.t3x")
    print()
    print("Or run:  make assets   (if the Makefile asset target is added)")
