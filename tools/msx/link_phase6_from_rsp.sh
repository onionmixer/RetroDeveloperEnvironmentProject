#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/bin"
SHARKSYM="$ROOT/Toolchain/MSX/HITECH_TOOLCHAIN/lib/CPMEMU_HI-TECH_C"

RSP="${1:-}"
OUT_COM="${2:-}"
OUT_MAP="${3:-}"
LIB="${4:-$SHARKSYM/LIBCMSX.LIB}"
PSECT="${5:--ptext=100H,data,bss}"

if [[ -z "$RSP" || -z "$OUT_COM" || -z "$OUT_MAP" ]]; then
  echo "usage: $0 <keep.rsp> <out.com> <out.map> [lib] [psect]" >&2
  exit 1
fi

if [[ ! -f "$RSP" ]]; then
  echo "[phase6-link] missing rsp: $RSP" >&2
  exit 1
fi
if [[ ! -f "$LIB" ]]; then
  echo "[phase6-link] missing lib: $LIB" >&2
  exit 1
fi

objs=()
while IFS= read -r line; do
  f="${line#"${line%%[![:space:]]*}"}"
  f="${f%"${f##*[![:space:]]}"}"
  [[ -z "$f" ]] && continue
  if [[ ! -f "$f" ]]; then
    echo "[phase6-link] missing object from rsp: $f" >&2
    exit 1
  fi
  objs+=("$f")
done < "$RSP"

if [[ ${#objs[@]} -eq 0 ]]; then
  echo "[phase6-link] empty rsp: $RSP" >&2
  exit 1
fi

mkdir -p "$(dirname "$OUT_COM")" "$(dirname "$OUT_MAP")"
# linq3 emits a stable warning for -z binary links without a start record.
# Keep all other diagnostics visible and do not mask non-zero exit codes.
tmp_stderr="$(mktemp)"
set +e
"$BIN/linq3" -z "$PSECT" -m"$OUT_MAP" -o"$OUT_COM" "${objs[@]}" "$LIB" 2>"$tmp_stderr"
rc=$?
set -e
if [[ $rc -ne 0 ]]; then
  cat "$tmp_stderr" >&2
  rm -f "$tmp_stderr"
  exit $rc
fi
if [[ -s "$tmp_stderr" ]]; then
  if grep -Fqx "no start record: entry point defaults to zero (warning)" "$tmp_stderr"; then
    echo "[phase6-link] note: linq3 warning ignored (no start record for -z binary output)"
  else
    cat "$tmp_stderr" >&2
  fi
fi
rm -f "$tmp_stderr"

echo "[phase6-link] objs=${#objs[@]} lib=$(basename "$LIB")"
echo "[phase6-link] out_com=$OUT_COM"
echo "[phase6-link] out_map=$OUT_MAP"
