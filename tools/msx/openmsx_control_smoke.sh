#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OPENMSX="$ROOT/Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx"
SHARE="$ROOT/Emulator/openMSX/share"
USER_HOME="${USER_HOME:-/tmp/openmsx_home_sandbox}"
TIMEOUT_SEC="${TIMEOUT_SEC:-4}"

mkdir -p "$USER_HOME"

if [[ ! -x "$OPENMSX" ]]; then
  echo "ERROR: openMSX not found: $OPENMSX" >&2
  exit 1
fi

set +e
timeout "${TIMEOUT_SEC}s" env \
  OPENMSX_SYSTEM_DATA="$SHARE" \
  OPENMSX_DISABLE_SDL_JOYSTICK=1 \
  OPENMSX_HOME="$USER_HOME" \
  OPENMSX_USER_DATA="$USER_HOME" \
  SDL_AUDIODRIVER=dummy \
  "$OPENMSX" -control stdio >/tmp/openmsx_control_smoke.log 2>&1
rc=$?
set -e

echo "openmsx_control_smoke rc=$rc (124 means timeout while running)"
sed -n '1,80p' /tmp/openmsx_control_smoke.log
