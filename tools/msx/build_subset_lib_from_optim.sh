#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
LIBR3="${LIBR3:-$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin/libr3}"

OPTIM_JSON="${1:-}"
OUT_DIR="${2:-}"

if [[ -z "$OPTIM_JSON" || -z "$OUT_DIR" ]]; then
  echo "usage: $0 <optim_graph.json> <out_dir>" >&2
  exit 1
fi
if [[ ! -f "$OPTIM_JSON" ]]; then
  echo "[subset-lib] missing optim json: $OPTIM_JSON" >&2
  exit 1
fi
if [[ ! -x "$LIBR3" ]]; then
  echo "[subset-lib] missing libr3 executable: $LIBR3" >&2
  exit 1
fi

mkdir -p "$OUT_DIR"
TMP_DIR="$OUT_DIR/.subset_tmp"
rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR"

python3 - "$OPTIM_JSON" "$OUT_DIR" "$TMP_DIR" "$LIBR3" <<'PY'
import json
import os
import pathlib
import subprocess
import sys

optim_json = pathlib.Path(sys.argv[1])
out_dir = pathlib.Path(sys.argv[2])
tmp_dir = pathlib.Path(sys.argv[3])
libr3 = pathlib.Path(sys.argv[4])

obj = json.load(open(optim_json, "r", encoding="utf-8"))
virt = obj.get("kept_virtual_modules", [])
if not isinstance(virt, list):
    raise SystemExit("[subset-lib] invalid kept_virtual_modules")

groups = {}
for item in virt:
    if not isinstance(item, str):
        continue
    if ":" not in item:
        continue
    lib, mod = item.rsplit(":", 1)
    lib = lib.strip()
    mod = mod.strip()
    if not lib or not mod:
        continue
    groups.setdefault(lib, []).append(mod)

manifest = {
    "source_optim_json": str(optim_json),
    "subset_libs": [],
}

for lib_path, mods in sorted(groups.items()):
    src = pathlib.Path(lib_path)
    if not src.exists():
        raise SystemExit(f"[subset-lib] source lib missing: {src}")

    requested = []
    seen = set()
    for m in mods:
        if m not in seen:
            seen.add(m)
            requested.append(m)

    # Preserve original module order from source library to keep linker behavior stable.
    list_out = subprocess.run([str(libr3), "m", str(src)], check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True).stdout
    src_order = [ln.split()[0] for ln in list_out.splitlines() if ln.strip()]
    req_set = set(requested)
    uniq_mods = [m for m in src_order if m in req_set]
    if not uniq_mods:
        raise SystemExit(f"[subset-lib] no matched modules in source order for {src}")

    work = tmp_dir / src.stem
    work.mkdir(parents=True, exist_ok=True)
    cmd_extract = [str(libr3), "x", str(src), *uniq_mods]
    subprocess.run(cmd_extract, cwd=work, check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

    missing = [m for m in uniq_mods if not (work / m).exists()]
    if missing:
        raise SystemExit(f"[subset-lib] extraction missing modules for {src}: {missing[:5]}")

    out_name = f"{src.stem}.subset{src.suffix.lower()}"
    out_lib = out_dir / out_name
    if out_lib.exists():
        out_lib.unlink()

    cmd_replace = [str(libr3), "r", str(out_lib), *uniq_mods]
    subprocess.run(cmd_replace, cwd=work, check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

    list_out = subprocess.run([str(libr3), "m", str(out_lib)], check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True).stdout
    built_mods = [ln.split()[0] for ln in list_out.splitlines() if ln.strip()]

    manifest["subset_libs"].append(
        {
            "source_lib": str(src),
            "subset_lib": str(out_lib),
            "requested_modules": len(uniq_mods),
            "built_modules": len(built_mods),
            "modules": built_mods,
            "source_size": src.stat().st_size,
            "subset_size": out_lib.stat().st_size,
        }
    )

manifest_path = out_dir / "subset_lib_manifest.json"
with open(manifest_path, "w", encoding="utf-8") as f:
    json.dump(manifest, f, indent=2, ensure_ascii=True)
    f.write("\n")

print(f"[subset-lib] manifest={manifest_path}")
for item in manifest["subset_libs"]:
    print(
        f"[subset-lib] {pathlib.Path(item['subset_lib']).name}: "
        f"{item['built_modules']} modules, "
        f"{item['subset_size']} bytes (src {item['source_size']} bytes)"
    )
PY

rm -rf "$TMP_DIR"
