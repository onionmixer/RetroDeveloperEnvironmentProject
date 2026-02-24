#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WORK="${WORK:-/tmp/bltoolc_phase4_mkrule_clean}"

CFG="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/examples/sharksym/0HELLO/0HELLO.CFG"

rm -rf "$WORK"
mkdir -p "$WORK"

cd "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool_c"
make >/dev/null
cd "$ROOT"

# prepare mk/manifest via existing stable path
./Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLMKRULE "$CFG" "$WORK/0HELLO.MK" >/dev/null
mkdir -p "$WORK/0HELLO.OUT"
touch "$WORK/0HELLO.OUT/dummy.bin"

Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool_c/bltoolc mkrule-clean --manifest "$WORK/0HELLO.MK.json" > "$WORK/clean.log"
[[ ! -f "$WORK/0HELLO.MK" && ! -f "$WORK/0HELLO.MK.json" ]]

# recreate manifest for cleanup test
./Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLMKRULE "$CFG" "$WORK/0HELLO.MK" >/dev/null
Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool_c/bltoolc mkrule-cleanup --manifest "$WORK/0HELLO.MK.json" > "$WORK/cleanup.log"
[[ ! -d "$WORK/0HELLO.OUT" ]]

echo "[test-bltoolc-phase4] PASS"
