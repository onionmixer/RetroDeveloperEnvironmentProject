#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
EXAMPLES=("2HELLO" "2ASSERT" "2LMEM" "2BGM" "2HANGUL" "2HANIME" "2TETRIS")

echo "[test-gate3-matrix] run examples: ${EXAMPLES[*]}"
for ex in "${EXAMPLES[@]}"; do
  echo "[test-gate3-matrix] === $ex ==="
  EXAMPLE="$ex" "$ROOT/tools/msx/test_phase6_gate3.sh"
done

echo "[test-gate3-matrix] === 2ASSERTN (NDEBUG) ==="
EXAMPLE="2ASSERT" EXAMPLE_VARIANT="NDEBUG" "$ROOT/tools/msx/test_phase6_gate3.sh"

if [[ "${ENABLE_OPENMSX_SMOKE:-0}" == "1" ]]; then
  echo "[test-gate3-matrix] openmsx smoke on representative example (2HELLO)"
  ENABLE_OPENMSX_SMOKE=1 EXAMPLE="2HELLO" "$ROOT/tools/msx/test_phase6_gate3.sh"
fi

echo "[test-gate3-matrix] PASS"
