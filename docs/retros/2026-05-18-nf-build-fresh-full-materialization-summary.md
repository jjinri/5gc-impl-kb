# 2026-05-18 — nf-build fresh full materialization summary

Status: complete
Current objective: fresh 상태에서 `_contract_seed.yaml` topic 전량을 사람 수동 저작 없이 materialize 해 `handoff_ready` 까지 도달 가능하게 만든 사이클을 한 문서로 남긴다.

## 한 줄 요약

`docs/plans/2026-05-18-nf-build-fresh-full-materialization-plan.md` 를 단일 PR (#24) 로 실행했다. nf-build SKILL.md 에 fresh-full 명세 + `materialize-contract.py` 신설로, contract/ 부재 상태에서 25 topic + data-model JSON 을 무인 materialize 하고 spec-derived 4 카테고리를 자동 handoff_ready 격상한다. 진행 중 resolver 버그 2건을 root-fix 했다.

## 사이클 진행

### 결정 수렴 (Pane 2 다회 second-opinion)

- "fresh 스크립트 주도 전량 신규 저작이 명세돼 있나" 검증 → **없음** 확정 (build-handoff 는 seed→yaml 만, topic .md 부재 시 validate #3/#9 FAIL → handoff_ready 도달 불가, fresh seed 전 카테고리 draft → step2 활성집합 공집합).
- plan 2 라운드 비평 — (1) status=draft↔handoff_ready 의미 (build-handoff `agent_contract.must_not`), (2) #11/#12 순환, (3) marker schema 구체성, (4) 단일 PR 스코프. 4문장+부속 전부 반영.
- 사용자 결정 — spec-derived(api·data-model·interface·error-handling) 자동 격상, 非spec skeleton draft 유지, "자동격상=spec-derived 구조검증 한정" 명시.

### 구현 (PR #24, 3+1 커밋)

- P1 nf-build SKILL.md §"Fresh full materialization" — 13 카테고리 marker/frontmatter schema 표 + 6단계 preflight 자동격상 알고리즘 (#11 순환 회피) + step2 예외.
- P2 `materialize-contract.py` 신설 — seed.topics 존재 entry 만 (invent 금지), 카테고리별 AUTO (data-model=resolve-yaml-refs, api/interface/error=OpenAPI yaml), USER 보존/TODO, preflight→promotion→seed/JSON status 동일 갱신→category status, 멱등.
- scalar/enum root JSON fix — resolve-yaml-refs `_root_node` 신설 + emit_json non-breaking top-level `root` (scalar/enum/anyOf 표현 복원).
- 리뷰 반영 — promoted topic `.md` frontmatter status 동기화 (3-way split 제거).

### 진행 중 발견·root-fix 한 버그 2건

1. **materialize-contract.py `yaml_for_schema`** — NSSF primary yaml 2개만 탐색 → 공통 자료형 (Snssai·PlmnId·NfInstanceId·SupportedFeatures@29.571, NFType@29.510) 미발견 → resolve-yaml-refs SystemExit → 빈 stub. fix: 외부 spec yaml 탐색 추가. (P3 stop-condition 이 적발)
2. **resolve-yaml-refs `_schema_node` 항상 type=object** — scalar/enum/anyOf root 의 type·enum·pattern 을 JSON machine_file 에서 손실 (NFType.json 이 66-value enum 계약 상실). fix: `_root_node` 분기 헬퍼 + top-level `root`. (DataModel JSON 내용 검증이 적발)
3. **promotion 후 .md frontmatter status draft 잔존** — seed/JSON 만 격상, .md 미갱신 → 3-way split. fix: promotion 루프가 frontmatter status in-place patch. (Pane 2 PR #24 리뷰가 적발)

세 버그 모두 *gate 우회 없이 root-fix*. unresolved_refs 를 비워 격상하는 우회는 금지 (Pane 2 규율 + memory `feedback-doc-code-accuracy`).

### 검증 (P3 stop-condition)

- fresh 무인 파이프라인 — materialize 25/25 handoff_ready·draft 0, build-handoff, validate basic 13/0.
- (D) `/nf-contract-check` handoff_ready PASS. tasks.*.read→draft NONE.
- USER 보존 OK, 멱등 OK(rerun diff 0), synthetic marker mismatch→#8 / JSON 누락→#9 정탐.
- scalar root assertion (NFType.enum=67 / NfInstanceId.format=uuid / SupportedFeatures.pattern / composite fields 회귀 0) PASS.
- 3-way status 정합 — seed==md frontmatter==JSON, mismatch NONE.
- (F) `/nf-arch-status` arch_consistent PASS (canonical baseline 무변경, evidence only).

## 잘된 점

- **stop-condition·내용검증이 버그 3건을 단계적으로 적발**. P3 stop-condition → yaml_for_schema 버그, DataModel JSON 내용검증 → scalar root 손실, Pane 2 리뷰 → 3-way status split. "지표·gate 통과" 에 안주하지 않고 *산출 내용* 을 직접 본 게 결정적.
- **gate 우회 거부 규율 일관**. 6 leaf unresolved 를 비워 격상하는 손쉬운 우회 대신 외부 yaml 탐색 root-fix. memory `feedback-doc-code-accuracy` 가 사이클 내내 적용 기준이 됨 (이번 사이클로 5회차 패턴 확인).
- **2 라운드 plan 비평이 #11 순환을 사전 차단**. "validate 통과 후 격상" 의 JSON↔handoff status 순환을 구현 전에 preflight 알고리즘으로 닫음 — 구현 후 발견했으면 비용 컸을 결함.
- **gitignored 산출 PR 위생 유지**. 25 topic + 15 JSON + manifest/seed/handoff 전부 비추적, PR diff = SKILL.md+scripts+plan 4파일. tooling 변경과 생성물 분리.

## 도구·운영 관찰 (영구화 후보)

- **fresh worktree 운영 마찰** — `.venv`(symlink 우회), `_manifest.yaml`/`_contract_seed.yaml` 부재가 매 사이클 반복 (retro 2026-05-14 도 관찰, 본 사이클 재발). `.venv/` gitignore 가 디렉터리 패턴이라 symlink 미매치 → 매번 수동 add 회피 필요. retro 후보 (6) worktree-aware hook 재발 2회 누적 — hook 화 trigger 도달.
- **Pane 2 send-keys 실패** — pane 2 비입력 상태에서 send-keys 가 `not in a mode` ×21 로 무성 실패, "sent" echo 가 전달 보장 아님. `-X cancel`/`q` 로 mode 해제 후 재송신으로 복구. pane-send SKILL 에 *송신 후 입력 박스 확인* 또는 mode-해제 전처리 권고 후보.
- **다른 agent 의 직접 main 커밋** — Pane 2 가 `.codex/skills/git-pr-review/SKILL.md` 를 로컬 main 직접 커밋(b755821, 미push) → 본 사이클 머지 후 로컬 main FF 차단. rebase 로 보존 (fe42d70, ahead 1). 교차-agent 직접 main 커밋의 동기화 비용 관찰.

## 다음 사이클 후보

- **(E) `/nf-arch-design nssf` 재생성** — 새 materialized contract 기준 architecture 재생성 → canonical(PR #16/#20) diff 검토 → `/nf-arch-status` → 별도 PR. 의도적 후속, 본 사이클서 분리.
- **fresh-full 다른 NF 일반화** — 본 capability 가 NSSF 외 NF 에서 동작하는지 (사용자 결정상 NSSF 구현+배포 완료 전까지 다른 NF 논외 — 보류).
- **worktree-aware manifest/seed/.venv hook** — 2회 재발, hook 화 trigger.
- **Phase 2 deep traceability id-relation gate** (validator 후속), **canonical 상수↔SKILL.md drift 방지** — 이전 retro 이월.

## 최종 상태

- PR #24 머지 (`4967274`). origin/main 과 로컬 main 동기 (Pane 2 의 `fe42d70` 만 로컬 ahead 1 — Pane 2 소관).
- fresh-full materialization capability + scalar/enum root JSON fidelity + 3-way status 정합 확정.
- `docs/plans/2026-05-18-nf-build-fresh-full-materialization-plan.md` Status closed 마킹 (본 retro 와 함께).
- (E) architecture 재생성은 다음 사이클 trigger 대기.
