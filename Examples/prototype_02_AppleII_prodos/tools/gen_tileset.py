#!/usr/bin/env python3
"""Generate tileset binary files for prototype_02_AppleII_prodos.

Output: 72-byte binary (9 tiles x 8 bytes each).
Tile order: FLOOR, WALL, DOOR_H, DOOR_V, STAIR_DN, STAIR_UP, BOX, EMPTY, reserved

Usage:
    python3 gen_tileset.py [--out-dir DIR]
"""

import argparse
import os

# Default tileset 0 — matches original render.c static const arrays
TILESET_0 = bytes([
    # FLOOR
    0x00, 0x00, 0x04, 0x00, 0x00, 0x20, 0x00, 0x00,
    # WALL
    0x7F, 0x63, 0x63, 0x7F, 0x1C, 0x1C, 0x7F, 0x00,
    # DOOR_H
    0x00, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x00,
    # DOOR_V
    0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14,
    # STAIR_DN
    0x00, 0x00, 0x3E, 0x1C, 0x08, 0x00, 0x3E, 0x00,
    # STAIR_UP
    0x00, 0x3E, 0x00, 0x08, 0x1C, 0x3E, 0x00, 0x00,
    # BOX
    0x00, 0x3E, 0x22, 0x22, 0x2A, 0x22, 0x3E, 0x00,
    # EMPTY
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    # reserved
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
])

def main():
    parser = argparse.ArgumentParser(description="Generate tileset binary")
    parser.add_argument("--out-dir", default="build",
                        help="Output directory (default: build)")
    args = parser.parse_args()

    os.makedirs(args.out_dir, exist_ok=True)
    out_path = os.path.join(args.out_dir, "TILES0")
    with open(out_path, "wb") as f:
        f.write(TILESET_0)
    print(f"Generated: {out_path} ({len(TILESET_0)} bytes)")

if __name__ == "__main__":
    main()
