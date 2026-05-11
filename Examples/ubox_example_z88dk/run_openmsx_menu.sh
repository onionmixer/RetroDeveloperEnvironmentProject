#!/usr/bin/env bash
# ubox_example_z88dk — interactive menu launcher.
# Lists 12 ported examples, lets the user pick one + ROM/DOS mode,
# auto-builds if needed, then hands off to the per-example launcher.
#
# Usage:
#   ./run_openmsx_menu.sh                    # interactive menu
#   ./run_openmsx_menu.sh 01 rom             # direct: example 01, ROM
#   ./run_openmsx_menu.sh snake dos          # direct by short name + DOS
#   AUTO_BUILD=0 ./run_openmsx_menu.sh ...   # don't auto-rebuild

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXAMPLES_DIR="$SCRIPT_DIR/examples"
AUTO_BUILD="${AUTO_BUILD:-1}"

# row: number short_alias dirname dos_name notes
EXAMPLES=(
    "01 hello       01_hello         HELLO     ubox_only"
    "02 conhello    02_consolehello  CONHELLO  no_ubox"
    "03 timer       03_timer         TIMER     user_isr"
    "04 clib        04_clibrary      CLIB      printf_builtin"
    "05 music       05_music         MUSIC     mplayer_real_AKM"
    "06 sound       06_sound         SOUND     mplayer_real_AKM"
    "07 snake       07_snakebyte     SNAKE     malloc"
    "08 sokoban     08_socoban       SOKOBAN   game_3file"
    "09 tetris      09_tetris        TETRIS    util_msxdos_guard"
    "10 breakout    10_breakout      BREAKOUT  spman"
    "11 log         11_log           LOG       malloc"
    "12 debugger    12_debugger      DEBUGGER  ubox_only"
)

print_menu() {
    echo "========================================"
    echo "  ubox-msx-lib-z88dk — example launcher"
    echo "========================================"
    echo "  No  Name           Dir              Notes"
    echo "  --  -------------- ---------------- ----------------------"
    for row in "${EXAMPLES[@]}"; do
        # collapse multiple spaces into pipe, then split
        IFS=' ' read -r num short dir dos notes <<<"$row"
        printf "  %-3s %-14s %-16s %s\n" "$num" "$short" "$dir" "$notes"
    done
    echo
}

resolve_example() {
    local key="$1"
    for row in "${EXAMPLES[@]}"; do
        IFS=' ' read -r num short dir dos notes <<<"$row"
        if [[ "$key" == "$num" || "$key" == "$short" || "$key" == "$dir" ]]; then
            echo "$dir|$dos"
            return 0
        fi
    done
    return 1
}

prompt_choice() {
    local prompt="$1" valid="$2" reply
    while true; do
        read -r -p "$prompt" reply
        if [[ ",$valid," == *",$reply,"* ]]; then
            echo "$reply"
            return 0
        fi
        echo "  invalid: '$reply' (valid: $valid)" >&2
    done
}

# --- direct CLI form: ./run_openmsx_menu.sh <example> <mode>
EX_KEY="${1:-}"
MODE="${2:-}"

if [[ -z "$EX_KEY" ]]; then
    print_menu
    read -r -p "Pick example by number / short / dir name: " EX_KEY
fi

resolved="$(resolve_example "$EX_KEY")" || {
    echo "error: unknown example '$EX_KEY'" >&2
    print_menu
    exit 1
}
EX_DIR="${resolved%|*}"
EX_DOS="${resolved#*|}"

if [[ -z "$MODE" ]]; then
    echo
    echo "Selected: $EX_DIR (DOS name: $EX_DOS)"
    MODE="$(prompt_choice "Mode? [rom/dos]: " "rom,dos,r,d")"
    case "$MODE" in r) MODE=rom ;; d) MODE=dos ;; esac
fi
case "$MODE" in
    rom|ROM|r) MODE=rom ;;
    dos|DOS|d) MODE=dos ;;
    *) echo "error: unknown mode '$MODE' (rom|dos)" >&2; exit 1 ;;
esac

EX_PATH="$EXAMPLES_DIR/$EX_DIR"
[[ -d "$EX_PATH" ]] || { echo "error: missing example dir: $EX_PATH" >&2; exit 1; }

# --- auto-build if needed
if [[ "$AUTO_BUILD" == "1" ]]; then
    case "$MODE" in
        rom)
            if [[ ! -f "$EX_PATH/build/${EX_DIR}.rom" ]]; then
                echo "[auto-build] $EX_DIR ROM"
                (cd "$EX_PATH" && ./compile.sh build) || { echo "build failed" >&2; exit 1; }
            fi
            ;;
        dos)
            COM=""
            [[ -f "$EX_PATH/build_dos/${EX_DIR}.COM" ]] && COM="$EX_PATH/build_dos/${EX_DIR}.COM"
            [[ -z "$COM" && -f "$EX_PATH/build_dos/$EX_DIR" ]] && COM="$EX_PATH/build_dos/$EX_DIR"
            if [[ -z "$COM" ]]; then
                echo "[auto-build] $EX_DIR DOS"
                (cd "$EX_PATH" && ./compile_dos.sh build) || { echo "build failed" >&2; exit 1; }
            fi
            ;;
    esac
fi

# --- launch
case "$MODE" in
    rom) exec "$EX_PATH/run_openmsx.sh" ;;
    dos) exec "$EX_PATH/run_openmsx_dos.sh" ;;
esac
