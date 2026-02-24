#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WORK="${WORK:-/tmp/bltoolc_phase2}"

rm -rf "$WORK"
mkdir -p "$WORK"

printf 'abc\n' > "$WORK/a.bin"
printf 'DEF' > "$WORK/b.bin"

cd "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool_c"
make >/dev/null

python3 "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool.py" list "$WORK/a.bin" > "$WORK/py_list.txt"
./bltoolc list "$WORK/a.bin" > "$WORK/c_list.txt"

python3 "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool.py" merge -o "$WORK/py_merge.bin" "$WORK/a.bin" "$WORK/b.bin" > "$WORK/py_merge.log"
./bltoolc merge -o "$WORK/c_merge.bin" "$WORK/a.bin" "$WORK/b.bin" > "$WORK/c_merge.log"

cmp -s "$WORK/py_merge.bin" "$WORK/c_merge.bin"
cmp -s "$WORK/py_merge.bin.tbl" "$WORK/c_merge.bin.tbl"

python3 - "$WORK/py_list.txt" "$WORK/c_list.txt" <<'PY'
import re,sys
py = open(sys.argv[1], 'r', encoding='utf-8').read()
c = open(sys.argv[2], 'r', encoding='utf-8').read()
for key in ("type:", "size:", "md5 :", "head:"):
    if key not in py or key not in c:
        raise SystemExit(f"missing field: {key}")
def field(text, name):
    m = re.search(rf"^\s*{re.escape(name)}\s*(.+)$", text, re.MULTILINE)
    return m.group(1).strip() if m else None
for name in ("type:", "size:", "md5 :", "head:"):
    if field(py, name) != field(c, name):
        raise SystemExit(f"field mismatch: {name} -> py={field(py,name)} c={field(c,name)}")
print("[test-bltoolc-phase2] list fields OK")
PY

echo "[test-bltoolc-phase2] merge/bin/tbl OK"
echo "[test-bltoolc-phase2] PASS"
