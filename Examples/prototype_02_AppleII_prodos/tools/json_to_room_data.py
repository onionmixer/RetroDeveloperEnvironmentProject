#!/usr/bin/env python3
"""Convert prototype/01 room JSON files into static C room data for Apple II build."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any

ROOM_W = 100
ROOM_H = 100
ROOM_COUNT = 3
MAX_DOORS = 4
MAX_STAIRS = 2
MAX_BOXES = 10
MAX_MONSTERS = 10
MAX_ITEMS_PER_BOX = 5

SIDE_TO_ENUM = {
    "north": "SIDE_NORTH",
    "south": "SIDE_SOUTH",
    "west": "SIDE_WEST",
    "east": "SIDE_EAST",
}

ORI_TO_ENUM = {
    "horizontal": "ORIENT_HORIZONTAL",
    "vertical": "ORIENT_VERTICAL",
}

STAIR_TYPE_TO_ENUM = {
    "down": "STAIR_DOWN",
    "up": "STAIR_UP",
}


def c_escape(text: str) -> str:
    return text.replace("\\", "\\\\").replace('"', '\\"')


def rle_compress(data: bytes) -> bytes:
    """PackBits-style RLE compression.

    MSB=1: repeat run, count = (cmd & 0x7F) + 2 (range 2-129)
    MSB=0: literal run, count = cmd + 1 (range 1-128)
    """
    result = bytearray()
    i = 0
    n = len(data)
    while i < n:
        # Check for repeat of 3+ identical bytes
        j = i + 1
        while j < n and data[j] == data[i] and (j - i) < 129:
            j += 1
        run = j - i
        if run >= 3:
            result.append(0x80 | (run - 2))
            result.append(data[i])
            i = j
        else:
            # Accumulate literal bytes
            lit_start = i
            while i < n:
                if i + 2 < n and data[i] == data[i + 1] == data[i + 2]:
                    break
                i += 1
                if (i - lit_start) >= 128:
                    break
            lit_len = i - lit_start
            result.append(lit_len - 1)
            result.extend(data[lit_start : lit_start + lit_len])
    return bytes(result)


def tile_to_code(ch: str) -> int:
    if ch == ".":
        return 0
    if ch == "#":
        return 1
    if ch == "@":
        return 2
    if ch == "<":
        return 3
    if ch == ">":
        return 4
    if ch == "%":
        return 5
    raise ValueError(f"unsupported tile: {ch!r}")


def load_rooms(paths: list[Path]) -> list[dict[str, Any]]:
    rooms: list[dict[str, Any]] = []
    for p in paths:
        with p.open("r", encoding="utf-8") as f:
            rooms.append(json.load(f))
    return rooms


def validate_rooms(rooms: list[dict[str, Any]]) -> None:
    if len(rooms) != ROOM_COUNT:
        raise ValueError(f"expected {ROOM_COUNT} rooms, got {len(rooms)}")

    id_set = set()
    for room in rooms:
        rid = room["id"]
        if rid in id_set:
            raise ValueError(f"duplicate room id: {rid}")
        id_set.add(rid)

        grid = room["grid"]
        if len(grid) != ROOM_H:
            raise ValueError(f"{rid}: grid rows != {ROOM_H}")
        for y, row in enumerate(grid):
            if len(row) != ROOM_W:
                raise ValueError(f"{rid}: grid row {y} cols != {ROOM_W}")

        if len(room.get("doors", [])) > MAX_DOORS:
            raise ValueError(f"{rid}: too many doors")
        if len(room.get("stairs", [])) > MAX_STAIRS:
            raise ValueError(f"{rid}: too many stairs")
        if len(room.get("boxes", [])) > MAX_BOXES:
            raise ValueError(f"{rid}: too many boxes")
        if len(room.get("monsters", [])) > MAX_MONSTERS:
            raise ValueError(f"{rid}: too many monsters")

        ps = room["player_start"]
        if grid[ps["y"]][ps["x"]] != ".":
            raise ValueError(f"{rid}: player_start is not floor")

        for door in room.get("doors", []):
            x = door["position"]["x"]
            y = door["position"]["y"]
            ori = door["orientation"]
            if ori == "horizontal":
                if not (0 <= x < ROOM_W - 1 and 0 <= y < ROOM_H):
                    raise ValueError(f"{rid}: door out of bounds")
                if grid[y][x] != "@" or grid[y][x + 1] != "@":
                    raise ValueError(f"{rid}: door tiles mismatch")
            else:
                if not (0 <= x < ROOM_W and 0 <= y < ROOM_H - 1):
                    raise ValueError(f"{rid}: door out of bounds")
                if grid[y][x] != "@" or grid[y + 1][x] != "@":
                    raise ValueError(f"{rid}: door tiles mismatch")

        for stair in room.get("stairs", []):
            x = stair["position"]["x"]
            y = stair["position"]["y"]
            ch = grid[y][x]
            st = stair["type"]
            if st == "down" and ch != "<":
                raise ValueError(f"{rid}: stair down tile mismatch")
            if st == "up" and ch != ">":
                raise ValueError(f"{rid}: stair up tile mismatch")

        for box in room.get("boxes", []):
            x = box["position"]["x"]
            y = box["position"]["y"]
            ori = box["orientation"]
            if ori == "horizontal":
                if not (0 <= x < ROOM_W - 1 and 0 <= y < ROOM_H):
                    raise ValueError(f"{rid}: box out of bounds")
                if grid[y][x] != "%" or grid[y][x + 1] != "%":
                    raise ValueError(f"{rid}: box tiles mismatch")
            else:
                if not (0 <= x < ROOM_W and 0 <= y < ROOM_H - 1):
                    raise ValueError(f"{rid}: box out of bounds")
                if grid[y][x] != "%" or grid[y + 1][x] != "%":
                    raise ValueError(f"{rid}: box tiles mismatch")
            items = box.get("items", [])
            if not items:
                raise ValueError(f"{rid}: box has no items")
            if len(items) > MAX_ITEMS_PER_BOX:
                raise ValueError(f"{rid}: box too many items")

        for monster in room.get("monsters", []):
            x = monster["position"]["x"]
            y = monster["position"]["y"]
            if not (0 <= x < ROOM_W and 0 <= y < ROOM_H):
                raise ValueError(f"{rid}: monster out of bounds")
            if grid[y][x] != ".":
                raise ValueError(f"{rid}: monster must start on floor")


def resolve_links(rooms: list[dict[str, Any]]) -> tuple[dict[tuple[int, int], tuple[int, int]], dict[tuple[int, int], tuple[int, int]]]:
    door_links: dict[tuple[int, int], tuple[int, int]] = {}
    stair_links: dict[tuple[int, int], tuple[int, int]] = {}

    def opposite(a: str, b: str) -> bool:
        return (a == "north" and b == "south") or (a == "south" and b == "north") or (a == "west" and b == "east") or (a == "east" and b == "west")

    for i, room_a in enumerate(rooms):
        for di, door_a in enumerate(room_a.get("doors", [])):
            if (i, di) in door_links:
                continue
            for j, room_b in enumerate(rooms):
                if i == j:
                    continue
                for dj, door_b in enumerate(room_b.get("doors", [])):
                    if (j, dj) in door_links:
                        continue
                    if opposite(door_a["wall_side"], door_b["wall_side"]):
                        door_links[(i, di)] = (j, dj)
                        door_links[(j, dj)] = (i, di)
                        break
                if (i, di) in door_links:
                    break

    for i, room_a in enumerate(rooms):
        for si, stair_a in enumerate(room_a.get("stairs", [])):
            if stair_a["type"] != "down":
                continue
            if (i, si) in stair_links:
                continue
            for j, room_b in enumerate(rooms):
                if i == j:
                    continue
                for sj, stair_b in enumerate(room_b.get("stairs", [])):
                    if stair_b["type"] != "up":
                        continue
                    if (j, sj) in stair_links:
                        continue
                    stair_links[(i, si)] = (j, sj)
                    stair_links[(j, sj)] = (i, si)
                    break
                if (i, si) in stair_links:
                    break

    return door_links, stair_links


def generate(room_paths: list[Path], out_h: Path, out_c: Path,
             binary_dir: Path | None = None) -> None:
    rooms = load_rooms(room_paths)
    validate_rooms(rooms)
    door_links, stair_links = resolve_links(rooms)

    h_text = """#ifndef ROOM_DATA_H_INCLUDED
#define ROOM_DATA_H_INCLUDED

#include "engine.h"

extern const RoomDef g_rooms[ROOM_COUNT];

#endif
"""
    out_h.write_text(h_text, encoding="ascii")

    lines: list[str] = []
    lines.append('#include "room_data.h"')
    lines.append("")

    # Compress grids
    rle_arrays: list[str] = []
    for ri, room in enumerate(rooms):
        raw = bytearray()
        for row in room["grid"]:
            for i in range(0, ROOM_W, 2):
                hi = tile_to_code(row[i])
                lo = tile_to_code(row[i + 1])
                raw.append((hi << 4) | lo)
        compressed = rle_compress(bytes(raw))

        if binary_dir is not None:
            # Write RLE as binary file: ROOM00, ROOM01, ...
            bin_name = f"ROOM{ri:02d}"
            bin_path = binary_dir / bin_name
            bin_path.write_bytes(compressed)
            print(f"  {bin_name}: {len(compressed)} bytes")
        else:
            # Emit inline C array
            rle_arrays.append(f"room_{ri}_grid_rle")
            lines.append(f"static const unsigned char room_{ri}_grid_rle[] = {{")
            for off in range(0, len(compressed), 16):
                chunk = compressed[off : off + 16]
                nums = ", ".join(f"0x{b:02X}" for b in chunk)
                lines.append(f"    {nums},")
            lines.append("};")
            lines.append("")

    lines.append("const RoomDef g_rooms[ROOM_COUNT] = {")

    for ri, room in enumerate(rooms):
        tileset_id = int(room.get("tileset_id", 0))
        lines.append("    {")
        lines.append(f'        "{c_escape(room["id"])}",')
        lines.append(f'        "{c_escape(room["name"])}",')
        lines.append(f'        {int(room["z_level"])},')
        if binary_dir is not None:
            lines.append(f'        {tileset_id},')
        else:
            lines.append(f'        {rle_arrays[ri]},')
            lines.append(f'        sizeof({rle_arrays[ri]}),')

        # doors
        doors = room.get("doors", [])
        lines.append(f"        {len(doors)},")
        lines.append("        {")
        for di, d in enumerate(doors):
            tx, td = door_links.get((ri, di), (255, 255))
            lines.append("            {")
            lines.append(f"                {int(d['position']['x'])}, {int(d['position']['y'])},")
            lines.append(f"                {ORI_TO_ENUM[d['orientation']]},")
            lines.append(f"                {SIDE_TO_ENUM[d['wall_side']]},")
            lines.append(f"                {tx}, {td}")
            lines.append("            },")
        for _ in range(MAX_DOORS - len(doors)):
            lines.append("            {0, 0, ORIENT_HORIZONTAL, SIDE_NORTH, 255, 255},")
        lines.append("        },")

        # stairs
        stairs = room.get("stairs", [])
        lines.append(f"        {len(stairs)},")
        lines.append("        {")
        for si, s in enumerate(stairs):
            tx, ts = stair_links.get((ri, si), (255, 255))
            lines.append("            {")
            lines.append(f"                {int(s['position']['x'])}, {int(s['position']['y'])},")
            lines.append(f"                {STAIR_TYPE_TO_ENUM[s['type']]},")
            lines.append(f"                {tx}, {ts}")
            lines.append("            },")
        for _ in range(MAX_STAIRS - len(stairs)):
            lines.append("            {0, 0, STAIR_DOWN, 255, 255},")
        lines.append("        },")

        # boxes
        boxes = room.get("boxes", [])
        lines.append(f"        {len(boxes)},")
        lines.append("        {")
        for b in boxes:
            items = b.get("items", [])
            lines.append("            {")
            lines.append(f"                {int(b['position']['x'])}, {int(b['position']['y'])},")
            lines.append(f"                {ORI_TO_ENUM[b['orientation']]},")
            lines.append(f'                "{c_escape(b.get("placed_by", "default"))}",')
            lines.append(f'                "{c_escape(b.get("effect", ""))}",')
            lines.append(f"                {len(items)},")
            lines.append("                {")
            for it in items:
                lines.append(f'                    "{c_escape(it.get("name", ""))}",')
            for _ in range(MAX_ITEMS_PER_BOX - len(items)):
                lines.append('                    "",')
            lines.append("                }")
            lines.append("            },")
        for _ in range(MAX_BOXES - len(boxes)):
            lines.append('            {0, 0, ORIENT_HORIZONTAL, "", "", 0, {""}},')
        lines.append("        },")

        # monsters
        monsters = room.get("monsters", [])
        lines.append(f"        {len(monsters)},")
        lines.append("        {")
        for m in monsters:
            pos = m["position"]
            r = max(4, int(m.get("range", 4)))
            lines.append("            {")
            lines.append(f"                {int(pos['x'])}, {int(pos['y'])},")
            lines.append(f"                {r},")
            lines.append(f'                "{c_escape(m.get("name", "Monster"))}"')
            lines.append("            },")
        for _ in range(MAX_MONSTERS - len(monsters)):
            lines.append('            {0, 0, 4, ""},')
        lines.append("        },")

        ps = room["player_start"]
        lines.append(f"        {int(ps['x'])}, {int(ps['y'])}")
        lines.append("    },")

    lines.append("};")
    lines.append("")

    out_c.write_text("\n".join(lines), encoding="ascii")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path,
                        default=Path("/mnt/USERS/onion/DATA_ORIGN/Workspace/04_Game/prototype/01/data"))
    parser.add_argument("--out-dir", type=Path,
                        default=Path("./Examples/prototype_01_AppleII_prodos/src"))
    parser.add_argument("--binary-dir", type=Path, default=None,
                        help="Output RLE data as binary files (ROOMnn) to this directory")
    parser.add_argument("--validate", action="store_true")
    args = parser.parse_args()

    room_paths = [args.source / f"room_00{i}.json" for i in (1, 2, 3)]
    rooms = load_rooms(room_paths)
    validate_rooms(rooms)

    if args.validate:
        print("VALIDATION_OK")
        return 0

    out_dir = args.out_dir
    out_dir.mkdir(parents=True, exist_ok=True)

    binary_dir = args.binary_dir
    if binary_dir is not None:
        binary_dir.mkdir(parents=True, exist_ok=True)

    generate(room_paths, out_dir / "room_data.h", out_dir / "room_data.c",
             binary_dir=binary_dir)
    print(f"WROTE {out_dir / 'room_data.h'}")
    print(f"WROTE {out_dir / 'room_data.c'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
