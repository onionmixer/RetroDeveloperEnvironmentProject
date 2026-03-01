#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
PROGRAM_NAME="${PROGRAM_NAME:-HELLO}"
PROGRAM_FILE="$SCRIPT_DIR/HELLO"
DISK_IMAGE="${DISK_IMAGE:-$BUILD_DIR/prototype_02_appleii_prodos.po}"
RDEDISKTOOL="${RDEDISKTOOL:-}"
PROGRAM_DISK_TEMPLATE="${PROGRAM_DISK_TEMPLATE:-}"
BOOT_DISK="${BOOT_DISK:-}"
cd "$SCRIPT_DIR"

first_existing_file() {
  local p
  for p in "$@"; do
    if [[ -f "$p" ]]; then
      printf '%s\n' "$p"
      return 0
    fi
  done
  return 1
}

first_existing_exec() {
  local p
  for p in "$@"; do
    if [[ -x "$p" ]]; then
      printf '%s\n' "$p"
      return 0
    fi
  done
  return 1
}

clean() {
  rm -f HELLO HELLO.tmp
  rm -rf "$BUILD_DIR"
}

gen_data() {
  python3 ./tools/json_to_room_data.py \
    --source /mnt/USERS/onion/DATA_ORIGN/Workspace/04_Game/prototype/01/data \
    --out-dir ./src \
    --binary-dir "$BUILD_DIR"
}

gen_tileset() {
  python3 ./tools/gen_tileset.py --out-dir "$BUILD_DIR"
}

build() {
  ./tools/check_env.sh
  mkdir -p "$BUILD_DIR"
  gen_data
  gen_tileset

  echo "[1/3] Building HELLO"
  cl65 -t apple2 -C src/apple2-hgr-ext.cfg -O -o HELLO \
    /usr/share/cc65/lib/apple2-iobuf-0800.o \
    src/main.c src/render.c src/logic.c src/monster.c \
    src/input.c src/help.c src/room_data.c

  echo "[2/3] Stripping AppleSingle header"
  tail -c +59 HELLO > HELLO.tmp
  mv HELLO.tmp HELLO

  echo "Built: $SCRIPT_DIR/HELLO ($(wc -c < HELLO) bytes)"
}

resolve_paths() {
  if [[ -z "$RDEDISKTOOL" ]]; then
    RDEDISKTOOL="$(first_existing_exec \
      "$PROJECT_ROOT/RetroDeveloperEnvironmentDisktool/build_local/rdedisktool" \
      "$PROJECT_ROOT/RetroDeveloperEnvironmentDisktool/build/rdedisktool" \
      "$(command -v rdedisktool 2>/dev/null || true)" \
    )" || RDEDISKTOOL="$PROJECT_ROOT/RetroDeveloperEnvironmentDisktool/build_local/rdedisktool"
  fi

  if [[ -z "$BOOT_DISK" ]]; then
    BOOT_DISK="$(first_existing_file \
      "$PROJECT_ROOT/diskwork/bootdisk/AppleII/ProDOS_2_4_3.po" \
      "$PROJECT_ROOT/diskwork/bootdisk/AppleII/prodos242.dsk" \
    )" || BOOT_DISK="$PROJECT_ROOT/diskwork/bootdisk/AppleII/ProDOS_2_4_3.po"
  fi

  if [[ -z "$PROGRAM_DISK_TEMPLATE" ]]; then
    PROGRAM_DISK_TEMPLATE="$(first_existing_file \
      "$PROJECT_ROOT/Examples/Tutorial_apple_prodos_01/Tutorial_apple_prodos_01.po" \
      "$BOOT_DISK" \
    )" || PROGRAM_DISK_TEMPLATE="$PROJECT_ROOT/Examples/Tutorial_apple_prodos_01/Tutorial_apple_prodos_01.po"
  fi
}

disk() {
  resolve_paths

  if [[ ! -x "$RDEDISKTOOL" ]]; then
    echo "Error: rdedisktool not executable at $RDEDISKTOOL" >&2
    exit 1
  fi
  if [[ ! -f "$PROGRAM_DISK_TEMPLATE" ]]; then
    echo "Error: program disk template not found at $PROGRAM_DISK_TEMPLATE" >&2
    exit 1
  fi
  if [[ ! -f "$PROGRAM_FILE" ]]; then
    echo "Error: program file not found at $PROGRAM_FILE (run build first)" >&2
    exit 1
  fi

  mkdir -p "$BUILD_DIR"
  cp "$PROGRAM_DISK_TEMPLATE" "$DISK_IMAGE"

  echo "[3/3] Preparing ProDOS disk image: $DISK_IMAGE"
  "$RDEDISKTOOL" --bootdisk-mode off delete "$DISK_IMAGE" "$PROGRAM_NAME" >/dev/null 2>&1 || true
  "$RDEDISKTOOL" --bootdisk-mode strict add --type B --addr 0x0803 \
    "$DISK_IMAGE" "$PROGRAM_FILE" "$PROGRAM_NAME"

  # Add ROOM binary files
  for f in "$BUILD_DIR"/ROOM??; do
    [ -f "$f" ] || continue
    fname="$(basename "$f")"
    "$RDEDISKTOOL" --bootdisk-mode off delete "$DISK_IMAGE" "$fname" >/dev/null 2>&1 || true
    "$RDEDISKTOOL" --bootdisk-mode off add --type B --addr 0x1CD0 \
      "$DISK_IMAGE" "$f" "$fname"
    echo "  Added: $fname"
  done

  # Add TILES binary files
  for f in "$BUILD_DIR"/TILES?; do
    [ -f "$f" ] || continue
    fname="$(basename "$f")"
    "$RDEDISKTOOL" --bootdisk-mode off delete "$DISK_IMAGE" "$fname" >/dev/null 2>&1 || true
    "$RDEDISKTOOL" --bootdisk-mode off add --type B --addr 0x1C88 \
      "$DISK_IMAGE" "$f" "$fname"
    echo "  Added: $fname"
  done

  echo "Disk image ready: $DISK_IMAGE"
}

usage() {
  cat <<USAGE
Usage: $0 [build|disk|run|clean|all]
  build : generate room data + build HELLO (HGR mode)
  disk  : create ProDOS disk image + add HELLO
  run   : build + disk + run AppleWin
  clean : remove build outputs
  all   : clean + build + disk (default)
USAGE
}

case "${1:-all}" in
  build) build ;;
  disk) disk ;;
  run) build; disk; AUTO_BUILD=0 "$SCRIPT_DIR/run_applewin_prodos.sh" ;;
  clean) clean ;;
  all) clean; build; disk ;;
  *) usage; exit 1 ;;
esac
