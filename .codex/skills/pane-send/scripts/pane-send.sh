#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat >&2 <<'USAGE'
usage:
  pane-send.sh --list
  pane-send.sh <pane-index> <text...>
  pane-send.sh <pane-index> --stdin
USAGE
}

list_panes() {
  tmux list-panes -F '#{pane_index} #{pane_id} active=#{pane_active} cmd=#{pane_current_command} title=#{pane_title} cwd=#{pane_current_path}'
}

self_pane() {
  local pid="$$"
  local match=""
  while [ "${pid:-0}" -gt 1 ] 2>/dev/null; do
    match=$(tmux list-panes -F '#{pane_index} #{pane_pid}' 2>/dev/null | awk -v p="$pid" '$2==p {print $1; exit}')
    if [ -n "$match" ]; then
      printf '%s' "$match"
      return 0
    fi
    pid=$(ps -o ppid= -p "$pid" 2>/dev/null | tr -d ' ')
    [ -z "$pid" ] && break
  done
  return 1
}

if [ "${1:-}" = "--list" ]; then
  list_panes
  exit 0
fi

if [ $# -lt 2 ]; then
  usage
  exit 2
fi

target="$1"
shift

if ! [[ "$target" =~ ^[0-9]+$ ]]; then
  echo "pane-send: target pane index must be an integer: $target" >&2
  exit 2
fi

if ! tmux list-panes -F '#{pane_index}' | grep -qx "$target"; then
  echo "pane-send: target pane $target not found in current tmux window" >&2
  list_panes >&2 || true
  exit 1
fi

if self=$(self_pane 2>/dev/null); then
  if [ "$self" = "$target" ]; then
    echo "pane-send: self-send blocked (pane $target)" >&2
    exit 1
  fi
fi

if [ "${1:-}" = "--stdin" ]; then
  text=$(cat)
else
  text="$*"
fi

if [ -z "$text" ]; then
  echo "pane-send: empty text rejected" >&2
  exit 2
fi

tmux send-keys -t "$target" -l "$text"
tmux send-keys -t "$target" Enter

first_line=$(printf '%s' "$text" | sed -n '1p' | cut -c 1-120)
chars=$(printf '%s' "$text" | wc -m | tr -d ' ')
printf 'sent to pane %s (%s chars): %s\n' "$target" "$chars" "$first_line"
