#!/usr/bin/env python3
"""
gen_tileset_msx.py — Generate tileset binary files from tiles.h data.

Extracts map tile patterns and colors (tiles 0-8, 9 tiles * 8 bytes each)
from tiles.h and writes TILES0 binary file.

Output: TILES0 = 144 bytes (72B patterns + 72B colors)
"""

import argparse
import os
import re
import sys

TILES_COUNT = 9   # tiles 0-8 (TILE_EMPTY through TILE_MONSTER)
BYTES_PER_TILE = 8


def parse_hex_data(text, macro_name):
    """Extract hex byte values from a #define macro continuation."""
    # Find the macro definition
    pattern = r'#define\s+' + macro_name + r'\s*\\?\s*\n?((?:.*\\\s*\n)*.*?)(?:\n\n|\n#|\Z)'
    match = re.search(pattern, text, re.DOTALL)
    if not match:
        # Try simpler pattern: everything after #define until next #define or end
        pattern2 = r'#define\s+' + macro_name + r'\s+(.+?)(?=\n#define|\n#endif|\Z)'
        match = re.search(pattern2, text, re.DOTALL)
        if not match:
            return None

    body = match.group(1)
    # Remove line continuations and comments
    body = re.sub(r'/\*.*?\*/', '', body, flags=re.DOTALL)
    body = body.replace('\\', '')

    # Extract all hex and decimal values
    values = []
    for token in re.findall(r'0x[0-9A-Fa-f]+|\b\d+\b', body):
        if token.startswith('0x') or token.startswith('0X'):
            values.append(int(token, 16))
        else:
            values.append(int(token))

    return values


def main():
    parser = argparse.ArgumentParser(description='Generate tileset binary')
    parser.add_argument('--tiles-h', default='src/tiles.h',
                        help='Path to tiles.h')
    parser.add_argument('--out-dir', default='build_dos',
                        help='Output directory')
    parser.add_argument('--tileset-id', type=int, default=0,
                        help='Tileset ID (output filename TILESn)')
    args = parser.parse_args()

    with open(args.tiles_h, 'r') as f:
        text = f.read()

    patterns = parse_hex_data(text, 'TILE_PATTERNS_DATA')
    colors = parse_hex_data(text, 'TILE_COLORS_DATA')

    if patterns is None or colors is None:
        print("ERROR: Could not parse tile data from", args.tiles_h,
              file=sys.stderr)
        sys.exit(1)

    # Extract first 9 tiles (72 bytes each)
    need = TILES_COUNT * BYTES_PER_TILE
    if len(patterns) < need or len(colors) < need:
        print("ERROR: Not enough tile data. patterns={}, colors={}, need={}".format(
            len(patterns), len(colors), need), file=sys.stderr)
        sys.exit(1)

    pat_bytes = bytes(patterns[:need])
    col_bytes = bytes(colors[:need])

    os.makedirs(args.out_dir, exist_ok=True)
    fname = "TILES{}".format(args.tileset_id)
    path = os.path.join(args.out_dir, fname)

    with open(path, 'wb') as f:
        f.write(pat_bytes)
        f.write(col_bytes)

    print("[gen_tileset_msx] Generated: {} ({} bytes)".format(
        fname, len(pat_bytes) + len(col_bytes)))


if __name__ == '__main__':
    main()
