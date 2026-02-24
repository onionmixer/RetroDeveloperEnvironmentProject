#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BLTOOLC="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/source/BANKING/bltool_c/bltoolc"
TMP="/tmp/bltoolc_phase6_mkrule_unsupported"

rm -rf "$TMP"
mkdir -p "$TMP"

cat > "$TMP/UNSUP.CFG" <<CFG
###
# target
#
target = UNSUP

tool-mode = 1
app-mode = 1
rom-mode = 0

src-bank00 += \\

lib-bank00 += \\
    \$(LIBC) \\

# app-mode1에서 비정상 다중 bank 구성(네이티브 경로 미지원)
src-bank01 += \\
    $ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/examples/sharksym/1HELLO/MAIN.C \\

lib-bank01 += \\
    \$(LIBC) \\
CFG

"$BLTOOLC" mkrule "$TMP/UNSUP.CFG" "$TMP/UNSUP.MK" >/tmp/bltoolc_phase6_unsupported_mkrule.log 2>&1

set +e
"$BLTOOLC" mkrule-build --manifest "$TMP/UNSUP.MK.json" >/tmp/bltoolc_phase6_unsupported_build.log 2>&1
rc=$?
set -e

if [[ $rc -eq 0 ]]; then
  echo "[test-bltoolc-phase6-unsupported] expected failure, got rc=0" >&2
  exit 1
fi

if ! rg -q "unsupported manifest pattern" /tmp/bltoolc_phase6_unsupported_build.log; then
  echo "[test-bltoolc-phase6-unsupported] missing expected error message" >&2
  sed -n '1,120p' /tmp/bltoolc_phase6_unsupported_build.log >&2 || true
  exit 1
fi

echo "[test-bltoolc-phase6-unsupported] PASS (rc=$rc)"
