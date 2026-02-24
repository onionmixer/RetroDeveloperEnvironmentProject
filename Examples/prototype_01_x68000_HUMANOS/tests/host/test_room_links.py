#!/usr/bin/env python3
import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "json_to_room_data_x68000.py"

spec = importlib.util.spec_from_file_location("gen", SCRIPT)
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

rooms = []
for p in mod.DEFAULT_INPUTS:
    with open(p, "r", encoding="utf-8") as f:
        import json
        room = json.load(f)
    mod.check_room(room)
    rooms.append(room)

door_links = mod.resolve_doors(rooms)
stair_links = mod.resolve_stairs(rooms)

for k, v in door_links.items():
    assert door_links.get(v) == k, f"door link not bidirectional: {k} -> {v}"

for k, v in stair_links.items():
    assert stair_links.get(v) == k, f"stair link not bidirectional: {k} -> {v}"

print("test_room_links: ok")
