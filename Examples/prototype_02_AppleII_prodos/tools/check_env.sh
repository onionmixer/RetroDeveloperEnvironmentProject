#!/usr/bin/env bash
set -euo pipefail

need_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Error: required command not found: $1" >&2
    exit 1
  fi
}

need_cmd cl65
need_cmd tail
need_cmd wc

echo "cl65: $(command -v cl65)"
cl65 --version | head -n 1

if command -v rdedisktool >/dev/null 2>&1; then
  echo "rdedisktool: $(command -v rdedisktool)"
else
  echo "Warning: rdedisktool not found (disk insert step will be skipped)." >&2
fi
