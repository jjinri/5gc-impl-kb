# NSSF plan amendments

본 디렉토리는 `dev/nssf/pr-slicing-plan.yaml` 갱신 PR (plan amendment) 의
1-pager 보관소다. ADR-0005 D5 의 의무 산출물.

## 언제 amendment 가 작성되는가

`nf-orchestrator` subagent 가 *out-of-scope* 작업 발견 시 — 예를 들면:

- pr-slicing-plan.yaml 의 기존 slice 가 다루지 않는 신규 작업이 필요한 경우
  (missing dependency wrapper, helper module, build-system patch 등).
- 기존 slice 의 `depends_on` / `scope_files` / `required_checks` 가 실제와
  drift 한 경우 (예 PR merge 후 plan 의 `status: merged` 미반영).
- slice 의 LOC 가 `pr_size_target` 을 hard_max 까지 초과해 분할이 필요한 경우.

orchestrator 는 plan amendment PR 를 *먼저* 만들고 (코드 PR 보다 우선),
reviewer subagent 의 planner-mode 검사 + ADR-0005 D2 4-condition gate 통과 시
self-merge. ADR / `eng_frozen` / ADR-0004 영향 변경은 3-trigger escape — 즉시
stop + 사람 보고.

## 파일 명명

`<YYYY-MM-DD>-<short-slug>.md` 형식. 예시:

- `2026-06-01-add-patchdocument-wrapper.md`
- `2026-06-03-split-availability-handlers-by-method.md`

## 1-pager 구조

`_template.md` 가 표준. 3 섹션 필수.

1. **이유** — 무엇이 plan 과 어긋났고 왜 amendment 가 필요한가.
2. **대안** — 고려한 다른 접근 (slice 분할 X, scope 확장, 별도 ADR 등) +
   각 거부 이유.
3. **영향** — pr-slicing-plan.yaml 의 어떤 entry 가 어떻게 변하고, 다른
   slice / WI / verification matrix 에 미치는 cascade.

amendment 1-pager 부재 또는 3 섹션 미충족 시 nf-reviewer subagent 가
planner-mode 에서 reject 한다.

## 본 디렉토리의 file lifecycle

- `_template.md` — git tracked, 본 README 와 함께 영구 보존.
- `<date>-<slug>.md` — git tracked, plan amendment PR merge 와 함께 main 진입.
  amendment 는 *history* 로 남는다 — 사후 audit 의 진실 출처.

## 관련 source

- ADR-0005 D5 — plan amendment self-merge + 3-trigger escape.
- `.claude/skills/nf-implement/SKILL.md` — plan amendment 가 orchestrator
  workflow 의 어디서 트리거되는가.
- `.claude/agents/nf-reviewer.md` — planner-mode 검사 (ADR-0001 vocab + slice
  policy alignment + 3-trigger check + amendment 1-pager 존재).
- `dev/nssf/pr-slicing-plan.yaml` — amendment 의 대상.
