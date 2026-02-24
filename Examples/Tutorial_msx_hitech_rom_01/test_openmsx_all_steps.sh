#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

step() {
  echo "========================================"
  echo "$1"
  echo "========================================"
}

step "Step 1/2: default-verified"
"$SCRIPT_DIR/test_openmsx_step.sh" default-verified

step "Step 2/2: fallback-verified"
"$SCRIPT_DIR/test_openmsx_step.sh" fallback-verified
