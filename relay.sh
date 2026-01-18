#!/usr/bin/env bash
set -euo pipefail

if [[ -z "${RELAY_CMD:-}" ]]; then
  if [[ -x ./relay_ft245r_macos ]]; then
    RELAY_CMD="./relay_ft245r_macos"
  else
    RELAY_CMD="./relay_ft245r.exe"
  fi
fi

usage() {
  cat <<'USAGE'
Usage:
  relay.sh <1-8|all> <on|off>
  relay.sh <1-8|all> blink [count] [interval_ms]
  relay.sh list
  relay.sh interactive

Environment:
  RELAY_CMD  Path to relay_ft245r binary (default: ./relay_ft245r_macos if present, else ./relay_ft245r.exe)

Examples:
  relay.sh 2 on
  relay.sh all off
  relay.sh 3 blink
  relay.sh 4 blink 5
  relay.sh 4 blink 5 250
  RELAY_CMD=./relay_ft245r.exe ./relay.sh 1 on
USAGE
}

fail() {
  echo "error: $*" >&2
  exit 1
}

if [[ $# -lt 1 ]]; then
  usage
  exit 1
fi

if [[ ! -x "$RELAY_CMD" ]]; then
  fail "RELAY_CMD is not executable: $RELAY_CMD"
fi

cmd="$1"
case "$cmd" in
  list)
    printf 'list\nquit\n' | "$RELAY_CMD"
    ;;
  interactive)
    exec "$RELAY_CMD"
    ;;
  all|[1-8])
    state="${2:-}"
    if [[ "$state" == "blink" ]]; then
      count="${3:-1}"
      interval_ms="${4:-500}"
      if ! [[ "$count" =~ ^[0-9]+$ ]] || [[ "$count" -lt 1 ]]; then
        fail "blink count must be a positive integer"
      fi
      if ! [[ "$interval_ms" =~ ^[0-9]+$ ]] || [[ "$interval_ms" -lt 1 ]]; then
        fail "blink interval_ms must be a positive integer"
      fi
      interval_s="$(awk "BEGIN { printf \"%.3f\", $interval_ms / 1000 }")"
      for ((i = 0; i < count; i++)); do
        "$RELAY_CMD" r "$cmd" s on
        sleep "$interval_s"
        "$RELAY_CMD" r "$cmd" s off
        sleep "$interval_s"
      done
    else
      if [[ "$state" != "on" && "$state" != "off" ]]; then
        fail "state must be 'on' or 'off'"
      fi
      "$RELAY_CMD" r "$cmd" s "$state"
    fi
    ;;
  -h|--help|help)
    usage
    ;;
  *)
    usage
    exit 1
    ;;
esac
