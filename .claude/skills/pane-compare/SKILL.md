---
name: pane-compare
description: 본 tmux 세션의 다른 pane (예 codex/gpt-5.5, 다른 claude) 의 최근 출력을 capture 해 본 pane 의 직전 의견·분석과 *비교 검토* 한 뒤 *분기·합의·통합 결론* 표를 만든다. 사용자가 "/pane-compare 2", "pane 2 결과 가져와 비교", "second-opinion 종합 리뷰", "다른 pane 의 의견과 본 결론 비교", "pane 2 검토 의견 확인" 등을 말하거나 자매 skill `/pane-send` 로 보낸 검토 요청의 응답이 *완료됨을 사용자가 확인* 한 시점에 사용한다. 본 skill 은 *수신 + 비교* 에 한정 — target pane 에 *송신* 하지 않으며, 응답 *완료 polling* 도 하지 않는다 (사용자가 완료 시점을 안다는 가정).
argument-hint: "<pane-index> [--lines <n>]"
allowed-tools: Bash(tmux capture-pane *) Bash(tmux list-panes *) Bash(ps *)
---

# pane-compare — 다른 pane 응답 capture + 본 pane 의견과 비교

## 입력

- `<pane-index>` — 응답을 가져올 *target pane index* (정수, 보통 1~9). 본 pane 과 다른 값.
- `--lines <n>` (옵션, default `500`) — capture-pane history 깊이. 응답이 길면 늘림.

## 책임 범위

- *수신 + 비교*. target pane 에 송신 안 함 — 그것은 `/pane-send`.
- *응답 완료 polling 안 함*. 사용자가 완료 시점을 *이미 안다* 는 가정.
- 본 skill 은 자동 커밋·파일 변경 안 함. *대화 안 비교 표 + 통합 결론* 만 출력.

## 동작 원칙 (이유 포함)

- **polling 생략은 의도된 결정.** target pane 의 *완료* 검출은 CLI 종류 (claude / codex / shell) 마다 휴리스틱이 달라 오탐 비용이 큼. 사용자가 *눈으로* 응답 완료를 확인 후 본 skill 호출이 안전·단순.
- **capture 영역은 `-S -<lines>` 로 history 포함.** target pane 의 visible buffer 만 보면 긴 응답이 잘려 평가가 부분적. default 500 줄로 평소 응답 cover. 더 길면 사용자가 `--lines` 키워 호출.
- **본 pane 의 *직전 의견* 은 *대화 컨텍스트* 에서 추출.** 본 skill 은 본 pane 자신의 *현재 대화 흐름* 안에서 *가장 최근* 의 분석/의견/권고를 *Pane 1 의견* 으로 사용. 별도 인자 없음.
- **비교 표는 항목별.** *항목 / Pane 1 의견 / Pane 2 응답 / 합의·분기* 4 컬럼. 분기 항목은 *근거* 와 *Pane 1 의 후속 판단* 명시.
- **자동 결정 안 함.** 본 skill 은 *비교 + 통합 결론 권고* 까지. *결정* 은 사용자.

## Workflow

### 1. 인자 검증
- `<pane-index>` 정수 + `tmux list-panes` 에 존재. 아니면 정지.
- `--lines` 가 있으면 정수, 양수.
- *self pane 식별* — `/pane-send` SKILL §2 의 `self_pane()` (process tree 매칭) 와 동일 방식. self 와 `<pane-index>` 가 같으면 capture 가 무의미 — 정지. `tmux display-message` 는 *사용 금지* (TMUX env 미상속 시 오반환).

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
[ "$self" = "<pane-index>" ] && { echo "self-capture 차단 (self=$self)"; exit 1; }
```

*zsh 호환 주의* — `local match=""` 별도 줄 선언, zsh 의 `local match=$()` 빈 값 stdout 부작용 회피.

### 2. capture
```bash
tmux capture-pane -t <pane-index> -p -S -<lines>
```
- ANSI escape 제거 (`-p` 가 plain text 출력. 일부 escape 잔존 시 sed 로 정리 가능 — 본 skill 은 plain 그대로 사용).
- 결과를 임시 파일 또는 본 skill 의 working memory 에 저장.

### 3. 응답 영역 식별
- capture 텍스트에서 *target pane 의 가장 최근 응답* 만 추출. 식별 휴리스틱.
  - **codex/gpt-5.5** — `›` 또는 `Ready · Context` 라인 직전까지가 최신 응답. `Worked for ...` 가 응답 종료 marker.
  - **claude CLI** — `╭─ /` box prompt 가 입력 줄. 그 전까지가 응답.
  - **shell** — 마지막 `$` / `# ` prompt 전까지.
- 식별 실패 시 *capture 전체* 를 응답 후보로 두고 사용자에게 알림.

### 4. Pane 1 의견 추출
- 본 pane 의 *현재 대화 흐름* 에서 *가장 최근* 의 분석/의견/권고를 식별.
  - retro 작성, 영구화 후보 정리, design 결정, review 결과 등 *명시적 결론* 이 있는 위치 우선.
  - 의견이 *명확하지 않으면* 사용자에게 "본 pane 의 비교 대상 의견을 명시해줘" 요청.

### 5. 비교 표 작성
| 항목 | Pane 1 의견 | Pane 2 응답 | 합의 / 분기 |

- 항목 — Pane 1 의 *주요 결론 항목* 또는 Pane 2 의 *명시적 응답 항목*. 둘 다 다루지 않는 항목은 skip.
- *합의* — 양쪽 같은 결론. 행 끝에 ✓.
- *분기* — 양쪽 다른 결론. *근거 + Pane 1 의 후속 판단* 짧게.
- *한쪽만 다룸* — `(Pane X 미언급)` 표시.

### 6. 통합 결론
- 합의 항목 — Pane 1 의견 *유지 또는 강화*.
- 분기 항목 — *Pane 2 가 더 설득력 있는 근거 있으면* 본 pane 의견 *수정 권고*, 아니면 *Pane 1 유지 + Pane 2 응답 비고*.
- 통합 결과의 *다음 액션 권고* (예 PR 진행 / 보류 / 추가 조사).

### 7. 출력 형식
- 본 pane 의 응답 메시지 안에 *마크다운 표 + 통합 결론 + 다음 액션* 직접 작성.
- 별도 파일 저장 안 함 (필요시 사용자가 retro 또는 plan 에 인용).

## 예시

**호출 — Pane 2 (GPT-5.5) 의 영구화 검토 응답 종합.**
```
사용자: /pane-compare 2
동작:   tmux capture-pane -t 2 -p -S -500
        → 응답 영역 식별 (codex `Worked for ...` 직후 또는 `›` 직전)
        → 본 pane 의 직전 *영구화 후보 6 종 정리* 추출
        → 비교 표 + 통합 결론

보고:
  | 항목 | Pane 1 | Pane 2 | 합의 |
  |---|---|---|---|
  | dev 섹션 영구화 1 순위 | ✓ 진행 | ✓ 진행 (stop condition 좁힘) | 합의 |
  | arch + module 분리 vs 묶음 | 분리 (2 순위 / 5 순위) | 묶어서, PR 분리 | 분기 — Pane 2 의 cross-ref 근거 강함 |
  | validator skill | 보류 | 보류 (NSSF 1 사이클 과적합) | 합의 |
  | Phase 패턴 영구화 | 별도 후순위 | 별도 PR | 합의 |

  통합 결론. dev 섹션 영구화 먼저, arch+module 묶어서 sub-PR 2 개. validator + Phase 패턴 보류.
  다음 액션 — dev template + SKILL.md 갱신 PR.
```

## 자주 틀리는 지점

- capture 영역이 짧아 응답이 잘림. `--lines` 키워 재호출.
- target pane 의 응답이 *아직 진행 중* 인데 호출 — 부분 응답으로 비교 시 잘못된 합의 도출. 사용자가 완료 확인 후 호출 책임.
- 본 pane 의견이 *명시적이지 않음* — 본 skill 이 임의 추출하면 의도와 다를 수 있음. 직전 응답이 *애매* 하면 사용자에게 의견 명시 요청.
- ANSI escape 잔존 — `tmux capture-pane -p` 가 보통 정리하지만 일부 형식 escape 가 남으면 가독성 떨어짐. *분석에는 영향 없으나* 표 안 raw escape 출력 회피 (응답 본문 인용 시 정리).
- target pane 이 *지원하지 않는 인스턴스* (예 OMX coordinator) — 응답 영역 식별 실패. capture 전체 출력 + 사용자에게 보고.

## 참고 — 본 skill 안에 다시 적지 말 것

- pane 라벨 규칙: memory `feedback-pane-naming`.
- 송신: 자매 skill `/pane-send`.
- tmux 명령 상세: `man tmux`.
