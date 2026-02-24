#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
WORK="${WORK:-/tmp/bltoolc_phase4_mkrule}"
CFG0="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/examples/sharksym/0HELLO/0HELLO.CFG"
CFG2="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/examples/sharksym/2HELLO/2HELLO.CFG"

rm -rf "$WORK"
mkdir -p "$WORK"

cd "$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool_c"
make >/dev/null
cd "$ROOT"

python3 Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool.py mkrule "$CFG0" "$WORK/py0.MK" >/dev/null
python3 Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool.py mkrule "$CFG2" "$WORK/py2.MK" >/dev/null
Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool_c/bltoolc mkrule "$CFG0" "$WORK/c0.MK" >/dev/null
Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool_c/bltoolc mkrule "$CFG2" "$WORK/c2.MK" >/dev/null

python3 - "$WORK/py0.MK.json" "$WORK/c0.MK.json" "$WORK/py2.MK.json" "$WORK/c2.MK.json" <<'PY'
import json,sys
pairs=[(sys.argv[1],sys.argv[2]),(sys.argv[3],sys.argv[4])]
for pyf,cf in pairs:
    py=json.load(open(pyf,'r',encoding='utf-8'))
    c=json.load(open(cf,'r',encoding='utf-8'))
    keys=['target','tool_mode','app_mode','rom_mode','tsr_mode','lib_float','lib_r800','cflags_derived','psect_cfg_derived','banks']
    for k in keys:
        assert py[k]==c[k], f'mismatch {k}: {pyf}'
    assert py['bank_layout']==c['bank_layout'], f'bank_layout mismatch: {pyf}'
print('[test-bltoolc-phase4-mkrule] manifest parity OK')
PY

make -f "$WORK/c0.MK" build >/dev/null
make -f "$WORK/c2.MK" build >/dev/null

echo "[test-bltoolc-phase4-mkrule] build path OK"
echo "[test-bltoolc-phase4-mkrule] PASS"
