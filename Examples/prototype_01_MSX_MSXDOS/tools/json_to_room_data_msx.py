#!/usr/bin/env python3
"""Generate static C room data for prototype_01 MSX project."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Dict, List, Tuple

ROOM_W = 100
ROOM_H = 100
ROOM_COUNT = 3
MAX_DOORS = 4
MAX_STAIRS = 2
MAX_BOXES = 10
MAX_ITEMS_PER_BOX = 5

ORI_MAP = {"horizontal": 0, "vertical": 1}
WALL_MAP = {"north": 0, "south": 1, "west": 2, "east": 3}
STAIR_MAP = {"down": 0, "up": 1}


DEFAULT_INPUTS = [
    "/mnt/USERS/onion/DATA_ORIGN/Workspace/04_Game/prototype/01/data/room_001.json",
    "/mnt/USERS/onion/DATA_ORIGN/Workspace/04_Game/prototype/01/data/room_002.json",
    "/mnt/USERS/onion/DATA_ORIGN/Workspace/04_Game/prototype/01/data/room_003.json",
]


def c_escape(s: str) -> str:
    return s.replace("\\", "\\\\").replace('"', '\\"')


def check_room(room: Dict) -> None:
    if room.get("width") != ROOM_W or room.get("height") != ROOM_H:
        raise ValueError(f"{room.get('id')}: expected 100x100")

    grid = room.get("grid", [])
    if len(grid) != ROOM_H:
        raise ValueError(f"{room.get('id')}: grid rows must be 100")
    for row in grid:
        if len(row) != ROOM_W:
            raise ValueError(f"{room.get('id')}: grid row length must be 100")
        for ch in row:
            if ch not in ".#@<>%":
                raise ValueError(f"{room.get('id')}: invalid tile {ch!r}")

    sx = room["player_start"]["x"]
    sy = room["player_start"]["y"]
    if grid[sy][sx] != ".":
        raise ValueError(f"{room.get('id')}: player_start must be on floor")

    for d in room.get("doors", []):
        x = d["position"]["x"]
        y = d["position"]["y"]
        ori = d["orientation"]
        if ori == "horizontal":
            if not (0 <= x < ROOM_W - 1 and 0 <= y < ROOM_H):
                raise ValueError(f"{room.get('id')}: bad door position")
            if grid[y][x] != "@" or grid[y][x + 1] != "@":
                raise ValueError(f"{room.get('id')}: door tiles not contiguous")
        else:
            if not (0 <= x < ROOM_W and 0 <= y < ROOM_H - 1):
                raise ValueError(f"{room.get('id')}: bad door position")
            if grid[y][x] != "@" or grid[y + 1][x] != "@":
                raise ValueError(f"{room.get('id')}: door tiles not contiguous")

    for b in room.get("boxes", []):
        x = b["position"]["x"]
        y = b["position"]["y"]
        ori = b["orientation"]
        if ori == "horizontal":
            if not (0 <= x < ROOM_W - 1 and 0 <= y < ROOM_H):
                raise ValueError(f"{room.get('id')}: bad box position")
            if grid[y][x] != "%" or grid[y][x + 1] != "%":
                raise ValueError(f"{room.get('id')}: box tiles not contiguous")
        else:
            if not (0 <= x < ROOM_W and 0 <= y < ROOM_H - 1):
                raise ValueError(f"{room.get('id')}: bad box position")
            if grid[y][x] != "%" or grid[y + 1][x] != "%":
                raise ValueError(f"{room.get('id')}: box tiles not contiguous")

    for s in room.get("stairs", []):
        x = s["position"]["x"]
        y = s["position"]["y"]
        typ = s["type"]
        expected = "<" if typ == "down" else ">"
        if grid[y][x] != expected:
            raise ValueError(f"{room.get('id')}: stair tile mismatch at {x},{y}")


def resolve_doors(rooms: List[Dict]) -> Dict[Tuple[int, int], Tuple[int, int]]:
    links: Dict[Tuple[int, int], Tuple[int, int]] = {}
    opposite = {"east": "west", "west": "east", "north": "south", "south": "north"}

    for i, room in enumerate(rooms):
        for di, d in enumerate(room["doors"]):
            if (i, di) in links:
                continue
            for j, other in enumerate(rooms):
                if i == j:
                    continue
                for dj, od in enumerate(other["doors"]):
                    if (j, dj) in links:
                        continue
                    if opposite[d["wall_side"]] == od["wall_side"]:
                        links[(i, di)] = (j, dj)
                        links[(j, dj)] = (i, di)
                        break
                if (i, di) in links:
                    break
    return links


def resolve_stairs(rooms: List[Dict]) -> Dict[Tuple[int, int], Tuple[int, int]]:
    links: Dict[Tuple[int, int], Tuple[int, int]] = {}
    downs: List[Tuple[int, int]] = []
    ups: List[Tuple[int, int]] = []

    for i, room in enumerate(rooms):
        for si, s in enumerate(room["stairs"]):
            if s["type"] == "down":
                downs.append((i, si))
            elif s["type"] == "up":
                ups.append((i, si))

    up_idx = 0
    for d in downs:
        while up_idx < len(ups) and ups[up_idx][0] == d[0]:
            up_idx += 1
        if up_idx >= len(ups):
            break
        u = ups[up_idx]
        up_idx += 1
        links[d] = u
        links[u] = d

    return links


def emit_h(path: Path) -> None:
    path.write_text(
        """#ifndef ROOM_DATA_H\n#define ROOM_DATA_H\n\n#define ROOM_COUNT 3\n#define ROOM_W 100\n#define ROOM_H 100\n#define VIEW_W 12\n#define VIEW_H 7\n\n#define MAX_DOORS 4\n#define MAX_STAIRS 2\n#define MAX_BOXES 10\n#define MAX_ITEMS_PER_BOX 5\n\ntypedef struct {\n    unsigned char x;\n    unsigned char y;\n    unsigned char orientation; /* 0=horizontal,1=vertical */\n    unsigned char wall_side;   /* 0=north,1=south,2=west,3=east */\n    unsigned char target_room;\n    unsigned char target_index;\n} DoorMsx;\n\ntypedef struct {\n    unsigned char x;\n    unsigned char y;\n    unsigned char type;        /* 0=down,1=up */\n    unsigned char target_room;\n    unsigned char target_index;\n} StairMsx;\n\ntypedef struct {\n    unsigned char x;\n    unsigned char y;\n    unsigned char orientation; /* 0=horizontal,1=vertical */\n    unsigned char placed_by_id;\n    unsigned char effect_id;\n    unsigned char item_count;\n    unsigned char item_ids[MAX_ITEMS_PER_BOX];\n} BoxMsx;\n\nextern const char g_room_grids[ROOM_COUNT][ROOM_H][ROOM_W + 1];\nextern const char g_room_names[ROOM_COUNT][24];\nextern const unsigned char g_room_z[ROOM_COUNT];\nextern const unsigned char g_player_start_x[ROOM_COUNT];\nextern const unsigned char g_player_start_y[ROOM_COUNT];\n\nextern const unsigned char g_door_count[ROOM_COUNT];\nextern const DoorMsx g_doors[ROOM_COUNT][MAX_DOORS];\n\nextern const unsigned char g_stair_count[ROOM_COUNT];\nextern const StairMsx g_stairs[ROOM_COUNT][MAX_STAIRS];\n\nextern const unsigned char g_box_count[ROOM_COUNT];\nextern const BoxMsx g_boxes[ROOM_COUNT][MAX_BOXES];\n\nextern const char * const g_placed_by[];\nextern const unsigned char g_placed_by_count;\nextern const char * const g_box_effects[];\nextern const unsigned char g_box_effect_count;\nextern const char * const g_item_names[];\nextern const unsigned char g_item_count;\n\n#endif\n""",
        encoding="utf-8",
    )


def emit_c(path: Path, rooms: List[Dict], door_links, stair_links) -> None:
    placed_by = ["default"]
    effects = [""]
    items = [""]

    def idx_of(table: List[str], s: str) -> int:
        if s not in table:
            table.append(s)
        return table.index(s)

    box_rows = []
    for ri, room in enumerate(rooms):
        row = []
        for bi, b in enumerate(room["boxes"]):
            pb = idx_of(placed_by, b["placed_by"])
            ef = idx_of(effects, b["effect"])
            item_ids = [idx_of(items, it["name"]) for it in b["items"]]
            item_ids += [0] * (MAX_ITEMS_PER_BOX - len(item_ids))
            row.append((
                b["position"]["x"],
                b["position"]["y"],
                ORI_MAP[b["orientation"]],
                pb,
                ef,
                len(b["items"]),
                item_ids,
            ))
        box_rows.append(row)

    lines: List[str] = []
    lines.append('#include "room_data.h"')
    lines.append("")

    lines.append("const char g_room_names[ROOM_COUNT][24] = {")
    for r in rooms:
        name = c_escape(r["name"])
        lines.append(f'    "{name}",')
    lines.append("};")
    lines.append("")

    lines.append("const unsigned char g_room_z[ROOM_COUNT] = {")
    lines.append("    " + ", ".join(str(r["z_level"]) for r in rooms))
    lines.append("};")

    lines.append("const unsigned char g_player_start_x[ROOM_COUNT] = {")
    lines.append("    " + ", ".join(str(r["player_start"]["x"]) for r in rooms))
    lines.append("};")

    lines.append("const unsigned char g_player_start_y[ROOM_COUNT] = {")
    lines.append("    " + ", ".join(str(r["player_start"]["y"]) for r in rooms))
    lines.append("};")
    lines.append("")

    lines.append("const char g_room_grids[ROOM_COUNT][ROOM_H][ROOM_W + 1] = {")
    for room in rooms:
        lines.append("    {")
        for row in room["grid"]:
            lines.append(f'        "{row}",')
        lines.append("    },")
    lines.append("};")
    lines.append("")

    lines.append("const unsigned char g_door_count[ROOM_COUNT] = {")
    lines.append("    " + ", ".join(str(len(r["doors"])) for r in rooms))
    lines.append("};")

    lines.append("const DoorMsx g_doors[ROOM_COUNT][MAX_DOORS] = {")
    for ri, room in enumerate(rooms):
        lines.append("    {")
        for di in range(MAX_DOORS):
            if di < len(room["doors"]):
                d = room["doors"][di]
                tgt = door_links.get((ri, di), (255, 255))
                lines.append(
                    "        { %d, %d, %d, %d, %d, %d }," % (
                        d["position"]["x"],
                        d["position"]["y"],
                        ORI_MAP[d["orientation"]],
                        WALL_MAP[d["wall_side"]],
                        tgt[0],
                        tgt[1],
                    )
                )
            else:
                lines.append("        { 0, 0, 0, 0, 255, 255 },")
        lines.append("    },")
    lines.append("};")
    lines.append("")

    lines.append("const unsigned char g_stair_count[ROOM_COUNT] = {")
    lines.append("    " + ", ".join(str(len(r["stairs"])) for r in rooms))
    lines.append("};")

    lines.append("const StairMsx g_stairs[ROOM_COUNT][MAX_STAIRS] = {")
    for ri, room in enumerate(rooms):
        lines.append("    {")
        for si in range(MAX_STAIRS):
            if si < len(room["stairs"]):
                s = room["stairs"][si]
                tgt = stair_links.get((ri, si), (255, 255))
                lines.append(
                    "        { %d, %d, %d, %d, %d }," % (
                        s["position"]["x"],
                        s["position"]["y"],
                        STAIR_MAP[s["type"]],
                        tgt[0],
                        tgt[1],
                    )
                )
            else:
                lines.append("        { 0, 0, 0, 255, 255 },")
        lines.append("    },")
    lines.append("};")
    lines.append("")

    lines.append("const unsigned char g_box_count[ROOM_COUNT] = {")
    lines.append("    " + ", ".join(str(len(r["boxes"])) for r in rooms))
    lines.append("};")

    lines.append("const BoxMsx g_boxes[ROOM_COUNT][MAX_BOXES] = {")
    for ri in range(ROOM_COUNT):
        lines.append("    {")
        for bi in range(MAX_BOXES):
            if bi < len(box_rows[ri]):
                x, y, ori, pb, ef, cnt, ids = box_rows[ri][bi]
                lines.append(
                    "        { %d, %d, %d, %d, %d, %d, { %d, %d, %d, %d, %d } }," % (
                        x, y, ori, pb, ef, cnt, ids[0], ids[1], ids[2], ids[3], ids[4]
                    )
                )
            else:
                lines.append("        { 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0, 0 } },")
        lines.append("    },")
    lines.append("};")
    lines.append("")

    lines.append("const char * const g_placed_by[] = {")
    for s in placed_by:
        lines.append(f'    "{c_escape(s)}",')
    lines.append("};")
    lines.append(f"const unsigned char g_placed_by_count = {len(placed_by)};")
    lines.append("")

    lines.append("const char * const g_box_effects[] = {")
    for s in effects:
        lines.append(f'    "{c_escape(s)}",')
    lines.append("};")
    lines.append(f"const unsigned char g_box_effect_count = {len(effects)};")
    lines.append("")

    lines.append("const char * const g_item_names[] = {")
    for s in items:
        lines.append(f'    "{c_escape(s)}",')
    lines.append("};")
    lines.append(f"const unsigned char g_item_count = {len(items)};")

    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--inputs", nargs="*", default=DEFAULT_INPUTS)
    parser.add_argument("--output-dir", default="src")
    parser.add_argument("--validate", action="store_true")
    args = parser.parse_args()

    inputs = [Path(p) for p in args.inputs]
    if len(inputs) != ROOM_COUNT:
        raise ValueError("expected exactly 3 room input files")

    rooms: List[Dict] = []
    for p in inputs:
        with p.open("r", encoding="utf-8") as f:
            room = json.load(f)
        check_room(room)
        rooms.append(room)

    if args.validate:
        print("validate: ok")
        return 0

    door_links = resolve_doors(rooms)
    stair_links = resolve_stairs(rooms)

    out_dir = Path(args.output_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    emit_h(out_dir / "room_data.h")
    emit_c(out_dir / "room_data.c", rooms, door_links, stair_links)
    print(f"generated: {out_dir / 'room_data.h'}")
    print(f"generated: {out_dir / 'room_data.c'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
