#!/usr/bin/env python3
import json

paths = [
    "/mnt/USERS/onion/DATA_ORIGN/Workspace/04_Game/prototype/01/data/room_001.json",
    "/mnt/USERS/onion/DATA_ORIGN/Workspace/04_Game/prototype/01/data/room_002.json",
    "/mnt/USERS/onion/DATA_ORIGN/Workspace/04_Game/prototype/01/data/room_003.json",
]

rooms = [json.load(open(p, "r", encoding="utf-8")) for p in paths]

door_total = sum(len(r["doors"]) for r in rooms)
stairs_down = sum(1 for r in rooms for s in r["stairs"] if s["type"] == "down")
stairs_up = sum(1 for r in rooms for s in r["stairs"] if s["type"] == "up")

assert door_total % 2 == 0, "doors should be pairable"
assert stairs_down <= stairs_up, "up stairs must cover down stairs"

print("test_room_links: ok")
