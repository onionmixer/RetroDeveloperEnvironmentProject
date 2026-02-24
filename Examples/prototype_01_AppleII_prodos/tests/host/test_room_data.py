#!/usr/bin/env python3
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
cmd = ["python3", str(ROOT / "tools" / "json_to_room_data.py"), "--validate"]
res = subprocess.run(cmd, check=False, capture_output=True, text=True)
print(res.stdout.strip())
raise SystemExit(res.returncode)
