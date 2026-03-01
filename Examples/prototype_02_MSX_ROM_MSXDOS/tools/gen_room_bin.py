#!/usr/bin/env python3
"""
gen_room_bin.py — Extract room grids from room_data.c into binary files.

Parses g_room_grids[][] string literals and writes each room as a flat
binary file (ROOM_W * ROOM_H = 720 bytes, no null terminators).

Output files: ROOM00, ROOM01, ROOM02, ...
"""

import argparse
import os
import re
import sys

ROOM_W = 30
ROOM_H = 24


def parse_room_grids(source_path):
    """Parse g_room_grids from room_data.c, return list of grid strings."""
    with open(source_path, 'r') as f:
        text = f.read()

    # Find the g_room_grids array
    match = re.search(
        r'g_room_grids\s*\[.*?\]\s*\[.*?\]\s*\[.*?\]\s*=\s*\{(.+?)\};',
        text, re.DOTALL
    )
    if not match:
        print("ERROR: g_room_grids not found in", source_path, file=sys.stderr)
        sys.exit(1)

    body = match.group(1)

    # Extract each room block: { "row0", "row1", ... }
    rooms = []
    # Split by room blocks (top-level braces within the outer array)
    room_blocks = re.findall(r'\{([^{}]+)\}', body)

    for block in room_blocks:
        # Extract all string literals
        rows = re.findall(r'"([^"]*)"', block)
        if len(rows) == ROOM_H:
            rooms.append(rows)

    return rooms


def write_room_binary(rooms, out_dir):
    """Write each room as ROOMnn binary file."""
    os.makedirs(out_dir, exist_ok=True)

    for idx, rows in enumerate(rooms):
        fname = "ROOM{:02d}".format(idx)
        path = os.path.join(out_dir, fname)

        data = bytearray()
        for row in rows:
            # Pad or truncate to ROOM_W
            row_bytes = row.encode('ascii')
            if len(row_bytes) < ROOM_W:
                row_bytes += b'.' * (ROOM_W - len(row_bytes))
            elif len(row_bytes) > ROOM_W:
                row_bytes = row_bytes[:ROOM_W]
            data.extend(row_bytes)

        assert len(data) == ROOM_W * ROOM_H, \
            "Room {} size {} != {}".format(idx, len(data), ROOM_W * ROOM_H)

        with open(path, 'wb') as f:
            f.write(data)
        print("  Generated: {} ({} bytes)".format(fname, len(data)))


def main():
    parser = argparse.ArgumentParser(description='Extract room grids to binary')
    parser.add_argument('--input', default='src/room_data.c',
                        help='Path to room_data.c')
    parser.add_argument('--out-dir', default='build_dos',
                        help='Output directory for ROOMnn files')
    args = parser.parse_args()

    rooms = parse_room_grids(args.input)
    if not rooms:
        print("ERROR: No rooms found", file=sys.stderr)
        sys.exit(1)

    print("[gen_room_bin] Found {} rooms".format(len(rooms)))
    write_room_binary(rooms, args.out_dir)


if __name__ == '__main__':
    main()
