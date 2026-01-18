#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage:
  teams_status.sh
  teams_status.sh watch [interval_ms]

Environment:
  RELAY_SCRIPT  Path to relay control script (default: ./relay.sh)
  RELAY_NUM     Relay number to toggle in watch mode (default: 2)
USAGE
}

get_status() {
  osascript <<'APPLESCRIPT'
set appNames to {"Microsoft Teams", "Microsoft Teams (work or school)", "Teams"}
tell application "System Events"
  repeat with appName in appNames
    if exists (process appName) then
      tell process appName
        try
          set windowTitles to name of windows
        on error
          set windowTitles to {}
        end try
        repeat with t in windowTitles
          set tStr to t as text
          if tStr contains "Meeting" or tStr contains "Call" or tStr contains "In call" or tStr contains "Meet now" then
            return "active"
          end if
        end repeat
        try
          if exists (buttons whose name is "Leave") then return "active"
          if exists (buttons whose name is "Hang up") then return "active"
        end try
      end tell
      return "inactive"
    end if
  end repeat
end tell
return "inactive"
APPLESCRIPT
}

mode="${1:-}"
case "$mode" in
  "" )
    status="$(get_status)" || { echo "unknown"; exit 2; }
    printf '%s\n' "$status"
    if [[ "$status" == "active" ]]; then exit 0; fi
    if [[ "$status" == "inactive" ]]; then exit 1; fi
    exit 2
    ;;
  watch)
    interval_ms="${2:-2000}"
    if ! [[ "$interval_ms" =~ ^[0-9]+$ ]] || [[ "$interval_ms" -lt 1 ]]; then
      echo "error: interval_ms must be a positive integer" >&2
      exit 2
    fi
    relay_script="${RELAY_SCRIPT:-./relay.sh}"
    relay_num="${RELAY_NUM:-2}"
    if [[ ! -x "$relay_script" ]]; then
      echo "error: RELAY_SCRIPT is not executable: $relay_script" >&2
      exit 2
    fi
    interval_s="$(awk "BEGIN { printf \"%.3f\", $interval_ms / 1000 }")"
    last_state=""
    while true; do
      status="$(get_status)" || status="unknown"
      if [[ "$status" == "active" ]]; then
        if [[ "$last_state" != "active" ]]; then
          "$relay_script" "$relay_num" on
          last_state="active"
        fi
      elif [[ "$status" == "inactive" ]]; then
        if [[ "$last_state" != "inactive" ]]; then
          "$relay_script" "$relay_num" off
          last_state="inactive"
        fi
      else
        last_state="unknown"
      fi
      sleep "$interval_s"
    done
    ;;
  -h|--help|help)
    usage
    ;;
  *)
    usage
    exit 1
    ;;
esac
