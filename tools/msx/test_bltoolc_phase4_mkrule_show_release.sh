#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WORK="${WORK:-/tmp/bltoolc_phase4_show_release}"
CFG="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/examples/sharksym/2HELLO/2HELLO.CFG"

rm -rf "$WORK"
mkdir -p "$WORK"

cd "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool_c"
make >/dev/null
cd "$ROOT"

# prepare real manifest for show parity
./Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLMKRULE "$CFG" "$WORK/2HELLO.MK" >/dev/null
python3 Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool.py mkrule-show --manifest "$WORK/2HELLO.MK.json" > "$WORK/py_show.txt"
Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool_c/bltoolc mkrule-show --manifest "$WORK/2HELLO.MK.json" > "$WORK/c_show.txt"
diff -u "$WORK/py_show.txt" "$WORK/c_show.txt"

# synthetic manifest for release success parity
mkdir -p "$WORK/samplecfg"
cat > "$WORK/m.MK.json" <<'JSON'
{
  "cfg": "/tmp/bltoolc_phase4_show_release/samplecfg/SAMPLE.CFG",
  "target": "SAMPLE"
}
JSON
mkdir -p "$WORK/SAMPLE.OUT"
printf x > "$WORK/SAMPLE.OUT/a.com"
printf x > "$WORK/SAMPLE.OUT/b.map"
printf x > "$WORK/SAMPLE.OUT/c.txt"

python3 Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool.py mkrule-release --manifest "$WORK/m.MK.json" >/dev/null
find "$WORK/samplecfg/RELEASE" -maxdepth 1 -type f -printf '%f\n' | sort > "$WORK/py_rel_files.txt"
rm -rf "$WORK/samplecfg/RELEASE"

Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool_c/bltoolc mkrule-release --manifest "$WORK/m.MK.json" >/dev/null
find "$WORK/samplecfg/RELEASE" -maxdepth 1 -type f -printf '%f\n' | sort > "$WORK/c_rel_files.txt"

diff -u "$WORK/py_rel_files.txt" "$WORK/c_rel_files.txt"

echo "[test-bltoolc-phase4-show-release] PASS"
