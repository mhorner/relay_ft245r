#!/usr/bin/env bash
set -euo pipefail

SRC="relay_ft245r_macos.c"
OUT="relay_ft245r_macos"

FTD2XX_INCLUDE="${FTD2XX_INCLUDE:-}"
FTD2XX_LIB="${FTD2XX_LIB:-}"

if [[ -z "$FTD2XX_INCLUDE" ]]; then
  if [[ -f /opt/homebrew/include/ftd2xx.h ]]; then
    FTD2XX_INCLUDE="/opt/homebrew/include"
  elif [[ -f /usr/local/include/ftd2xx.h ]]; then
    FTD2XX_INCLUDE="/usr/local/include"
  fi
fi

if [[ -z "$FTD2XX_LIB" ]]; then
  if [[ -f /opt/homebrew/lib/libftd2xx.dylib ]]; then
    FTD2XX_LIB="/opt/homebrew/lib"
  elif [[ -f /usr/local/lib/libftd2xx.dylib ]]; then
    FTD2XX_LIB="/usr/local/lib"
  fi
fi

if [[ -z "$FTD2XX_INCLUDE" || -z "$FTD2XX_LIB" ]]; then
  cat <<'EOF' >&2
error: ftd2xx headers/libs not found.
Install the FTDI D2XX driver for macOS and set:
  FTD2XX_INCLUDE=/path/to/include
  FTD2XX_LIB=/path/to/lib
EOF
  exit 1
fi

clang -O2 -Wall -Wextra \
  -I"$FTD2XX_INCLUDE" \
  -L"$FTD2XX_LIB" \
  -Wl,-rpath,"$FTD2XX_LIB" \
  -Wl,-rpath,@loader_path \
  -o "$OUT" "$SRC" -lftd2xx

LIB_PATH="$FTD2XX_LIB/libftd2xx.dylib"
if [[ -f "$LIB_PATH" ]]; then
  install_name_tool -change "libftd2xx.dylib" "$LIB_PATH" "$OUT"
fi

echo "built: $OUT"
