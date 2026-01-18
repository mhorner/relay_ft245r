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
  relay.sh list
  relay.sh interactive

Environment:
  RELAY_CMD  Path to relay_ft245r binary (default: ./relay_ft245r_macos if present, else ./relay_ft245r.exe)

Examples:
  relay.sh 2 on
  relay.sh all off
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
    if [[ "$state" != "on" && "$state" != "off" ]]; then
      fail "state must be 'on' or 'off'"
    fi
    "$RELAY_CMD" r "$cmd" s "$state"
    ;;
  -h|--help|help)
    usage
    ;;
  *)
    usage
    exit 1
    ;;
esac
