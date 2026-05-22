---
name: pane-send
description: 본 tmux 세션의 다른 pane (예 codex/gpt-5.5, 다른 claude, shell) 에 텍스트를 송신해 *검토 요청·질문·명령* 을 전달한다. 사용자가 "/pane-send 2 ...", "pane 2 에 보내", "pane 3 에 요청", "다른 pane 으로 검토 요청", "second-opinion 요청 보내" 등을 말하거나 본 pane 외의 인스턴스에 *입력* 을 전달해야 할 때 사용한다. 본 skill 은 *송신* 에 한정 — target pane 의 Ready 상태 검증·응답 대기·응답 수신은 *하지 않는다*. 사용자가 target pane 의 상태를 *이미 안다는 가정* 으로 동작. 응답 수신·비교는 자매 skill `/pane-compare` 의 책임.
argument-hint: "<pane-index> <text>"
allowed-tools: Bash(tmux send-keys *) Bash(tmux list-panes *) Bash(ps *)
---

# pane-send — tmux pane 으로 텍스트 송신

## 입력

- `<pane-index>` — target pane 의 *tmux pane index* (정수, 보통 1~9). `tmux list-panes` 로 확인 가능. 본 pane 의 index 와 다른 값.
- `<text>` — 전송할 텍스트. 줄바꿈 포함 가능. 끝에 자동 Enter.

## 책임 범위

- *송신만*. target pane 의 busy / Ready 상태 검사 안 함. 사용자 책임.
- 송신 후 *기다리지 않음*. 응답 capture 는 `/pane-compare` 또는 사용자 직접.
- 본 skill 은 자동 커밋·파일 변경 안 함.

## 동작 원칙 (이유 포함)

- **상태 검사 생략은 의도된 결정.** 사용자가 본 skill 을 호출할 때 *이미 target pane 의 입력 가능 상태를 봤다* 는 가정. 본 skill 이 자체 검사하면 오탐·헛 abort 가 늘어 비용이 큰다. 단 target pane index 가 존재하지 않으면 즉시 정지 (data corruption 방지).
- **send-keys 후 Enter *항상* 자동 — atomic 단일 호출.** 모든 사용 사례가 *한 줄·여러 줄 prompt + 확정 입력* 이라 Enter 부착이 default. 본 skill 은 *Enter 송신 없는* 호출을 지원하지 않는다 — 텍스트 paste + Enter 가 *하나의 atomic 동작* 이며, 둘을 분리해 호출하지 말 것 (paste 만 보내고 멈추면 target pane 의 buffer 가 오염된 채 사용자가 수동 확정해야 한다). 향후 `--no-enter` 가 필요하면 별도 옵션으로 추가, default 는 *Enter 송신*.
- **텍스트 escaping 은 안전한 single-pass.** tmux send-keys 가 받는 token 은 *literal*. 본 skill 은 사용자 텍스트를 *그대로* 전달, shell expansion 안 함.

## Workflow

### 1. 인자 검증
- `<pane-index>` 가 정수 1~9 범위인지 확인. 아니면 정지.
- `<text>` 가 비어있으면 정지 — empty send 금지.

### 2. self pane 검출 + self-send 차단

`tmux display-message -p '#{pane_index}'` 는 *현 shell 이 TMUX env 를 상속하지 않으면* (예 Claude Code 가 bg job 으로 invoke 한 Bash) 잘못된 pane 을 반환한다. 따라서 *process tree 매칭* 으로 self pane 을 식별한다.

```bash
self_pane() {
  local pid=$$
  local match=""
  while [ "$pid" -gt 1 ]; do
    match=$(tmux list-panes -F '#{pane_index} #{pane_pid}' 2>/dev/null \
            | awk -v p="$pid" '$2==p {print $1; exit}')
    if [ -n "$match" ]; then
      printf "%s" "$match"
      return 0
    fi
    pid=$(ps -o ppid= -p "$pid" 2>/dev/null | tr -d ' ')
    [ -z "$pid" ] && break
  done
  return 1
}
self=$(self_pane)
[ "$self" = "<pane-index>" ] && { echo "self-send 차단 (self=$self)"; exit 1; }
```

- `$$` (현 shell PID) 에서 `ppid` 따라 위로 올라가며 각 PID 를 `tmux list-panes` 의 `pane_pid` 와 매칭.
- 매칭되는 pane index 가 self.
- self 식별 실패 시 (`return 1`) — bg context 등에서 정상 가능, *self-send 차단 skip* (사용자 책임).
- *zsh 호환 주의* — `local match` 를 *별도 줄에서 빈 문자열로 선언* 후 다음 줄에서 assign. zsh 에서 `local match=$(...)` 가 빈 값일 때 `match=''` 를 stdout 출력하는 부작용 회피.

### 3. target pane 존재 확인
```bash
tmux list-panes -F '#{pane_index}' | grep -q "^<pane-index>$" || { echo "target pane <pane-index> 부재"; exit 1; }
```
- 존재 안 하면 정지. 사용자에게 `tmux list-panes` 결과 보고.

### 4. 송신 (텍스트 + Enter atomic)
```bash
tmux send-keys -t <pane-index> -l "<text>" && sleep 0.05 && tmux send-keys -t <pane-index> Enter
```
- *항상* 위 한 줄 (chained) 형태로 호출. 두 send-keys 를 분리된 Bash 호출로 쪼개지 말 것 — 분리하면 paste 후 Enter 누락 가능성이 생긴다.
- `-l` (literal) flag — `\$`, `\n` 등의 tmux 해석 차단.
- `sleep 0.05` — paste 와 Enter 사이 race condition 회피 (codex/claude CLI 처럼 paste buffer 처리에 ms 단위 지연이 있는 인스턴스 보호).
- 텍스트 + Enter 별도 send-keys — multi-line 텍스트의 줄바꿈도 보존 (텍스트 안 `\n` 은 send-keys 가 newline 으로 처리, 받는 pane 이 paste 처리).
- 줄바꿈을 명시 multi-line 으로 보내려면 텍스트를 `\n` 으로 직접 join.
- Enter 송신 생략은 *허용되지 않는다* — 본 skill 의 단일 책임이 "텍스트 입력 + 확정" 이라 Enter 가 없으면 송신 자체가 미완성.

### 5. 보고
- "sent to pane <pane-index>: <첫 줄 + 길이>" 형식.
- 사용자에게 *응답 확인은 사용자 책임* 임을 짧게 안내.
- 본 skill 은 응답 polling 안 함.

## 예시

**호출 — Pane 2 (GPT-5.5) 에 영구화 검토 요청.**
```
사용자: /pane-send 2 NSSF arch/dev rework 의 영구화 후보 6 종 — dev 섹션 / arch 섹션 / module 섹션 / Phase 패턴 / validator skill / Pane 패턴. 본 사이클 1 NF 결과로 영구화 trigger 가 충족됐는지 평가해줘.
동작: tmux send-keys -t 2 -l "NSSF arch/dev rework ... 평가해줘" && sleep 0.05 && tmux send-keys -t 2 Enter
보고: "sent to pane 2 (160 chars). 응답 완료 시 /pane-compare 2 호출."
```

**호출 — Pane 3 의 shell 에 명령.**
```
사용자: /pane-send 3 ls -la .claude/skills/
동작: tmux send-keys -t 3 -l "ls -la .claude/skills/" && sleep 0.05 && tmux send-keys -t 3 Enter
보고: "sent to pane 3 (28 chars)."
```

## 자주 틀리는 지점

- *Enter 송신 누락* — `tmux send-keys -t <pane> -l "<text>"` 만 호출하고 멈추면 target pane 의 prompt buffer 에 텍스트만 paste 된 채 *submit 안 됨*. §4 의 chained 형태 (`... && sleep 0.05 && tmux send-keys -t <pane> Enter`) 를 *반드시* 한 묶음으로 호출.
- 본 pane index 와 target index 혼동. 본 pane index 는 §2 의 `self_pane` 함수 (process tree 매칭) 로 확인. `tmux display-message -p '#{pane_index}'` 는 TMUX env 미상속 시 잘못된 pane 반환 — *사용 금지*.
- target pane 이 *수정·실행 중인 prompt buffer* 보유 시 본 skill 의 텍스트가 그 buffer 에 *추가* 돼 corruption. 사용자가 사전 확인 필수.
- multi-line 텍스트의 줄바꿈 — `-l` flag 가 *literal* 모드라 줄바꿈은 newline 으로 그대로 전달. 받는 측이 *paste 처리* 하면 한 prompt 로 합침, *line 마다 enter* 하면 의도와 달리 분리될 수 있음.
- target pane 이 *지원하지 않는 인스턴스* (예 OMX coordinator) 면 send 가 의미 없음. 사용자 책임.

## 참고 — 본 skill 안에 다시 적지 말 것

- pane 라벨 규칙 (Pane 1 = 본 pane, Pane 2 = 다른 pane 의 독립 출력): memory `feedback-pane-naming`.
- 응답 수신·비교: 자매 skill `/pane-compare`.
- tmux 명령 상세: `man tmux`.
