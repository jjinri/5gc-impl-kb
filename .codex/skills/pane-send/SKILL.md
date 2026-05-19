---
name: pane-send
description: Send literal text to another tmux pane in the current session/window. Use when the user asks to pass an opinion, question, command, or review request to "pane 1", "pane 2", another pane, or a second-opinion pane. This skill only sends input and presses Enter; it does not wait for or capture the target pane's response.
---

# pane-send — send text to a tmux pane

## Scope

- Send-only: deliver text to an existing tmux pane and press Enter.
- Do not poll, wait, or compare the target response.
- Do not inspect or mutate the target pane beyond sending the requested text.

## Usage

Prefer the bundled script:

```bash
bash .codex/skills/pane-send/scripts/pane-send.sh <pane-index> <text>
```

For multi-line or long text:

```bash
cat <<'MSG' | bash .codex/skills/pane-send/scripts/pane-send.sh <pane-index> --stdin
...
MSG
```

List panes first when target identity is uncertain:

```bash
bash .codex/skills/pane-send/scripts/pane-send.sh --list
```

## Rules

1. Target pane index must exist in the current tmux window.
2. Self-send is blocked when the current pane can be identified from the process tree.
3. Empty text is rejected.
4. Text is sent literally with `tmux send-keys -l`, followed by `Enter`.
5. Report only: target pane, character count, and first line summary.
