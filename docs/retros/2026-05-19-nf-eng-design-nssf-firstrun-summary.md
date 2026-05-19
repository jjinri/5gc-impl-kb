# `/nf-eng-design nssf` 첫 실행 — 사이클 회고 (2026-05-19)

plan: `docs/plans/2026-05-19-nf-eng-design-nssf-firstrun-plan.md` (closed)
PR: #31 (`4ac102f` merge — `9f30ec4` plan / `4279a8b` engineering-design freeze)
선행: PR #25~#30 (ADR-0002/0003, profile v2, nf-eng-design/status skill). 본 사이클이 그 메커니즘의 첫 실 NF 적용.

## 한 일

NSSF 에 `/nf-eng-design` 첫 실행. 13 core slot + 7 연기 레지스터 = 20 inventory engineering decision 을 AI 초안 + slot 성격별 decision-support 로 작성, jjinri ratify (2026-05-19). `/nf-eng-status` `gates.eng_frozen` **PASS (8/0)** 달성 — 프로젝트 최종 목표(자율 코드 생성)의 GO 신호.

핵심 결정 — language=C(C11)/native(선택가능 decision-support 후 사용자 선택), persistence=rdbms(PostgreSQL/libpq, 3 table schema), schema_codegen tool=openapi-generator + json_runtime=cJSON, sbi_client_stack required=true(F-derived NotificationDispatcher→AMF/NRF), tls_security=external·oauth2=false(33.501), AMF-reallocation(38.413)·SBA-security-depth(33.501) explicitly_out_of_scope.

contract 는 fresh worktree 라 재생성(handoff §2): manifest manual_overrides 33.501·38.413 복원 → seed → materialize(25/25) → handoff_ready PASS, validate basic 13/0.

## 잘된 것

- **content 직접 검증이 실버그 3건 적발** (handoff §60 원칙). (a) manifest Read-tool stale view 가 populated 보였으나 yaml-parse 가 빈 overrides 진실 적발 → python 디스크 직접 주입으로 우회. (b) Pane 2 가 실 validator 재실행으로 `SELECT FOR UPDATE` 의 `SELECT` 가 UNDECIDED regex(`\bselect\b`) 에 걸리는 것 적발 — 자체검증은 ratify-only 로 오판했음. (c) PR 리뷰 Lore-trailer finding 을 AGENTS.md grep 으로 직접 검증해 근거 부재 확인·기각.
- **Pane 2 second-opinion ×4 가 실질 기여** — plan(A→R-arch·embedded-TLS rejected·persistence 단일 backend), rdbms 구성(합의+schema 위험 보정), full-draft(SELECT blocker), PR#31(stale 산문 + JSON runtime semantic gap). 특히 json_runtime gap 은 profile v2 가 강제 못 하는 의미적 dependency-closure 누락 — 구조 PASS 만으로 못 잡는 것을 교차검토가 잡음.
- slot 성격별 decision-support(S/R/R-arch) 이분이 실 NF 에서 작동 — 사용자 비개발자 전제 하 language=C·persistence=rdbms 를 후보·criteria·rejected·consequence 로 ratify.

## 함정·교훈

- **worktree 절대경로 Write/Edit = 메인 repo silent 작성** (engineering-design.md·plan 둘 다 적중). worktree cwd 임에도 `/home/jjinri/AI/5gc-impl-kb/...` 절대경로가 메인 repo 로 감 — Pane 2 P0 가 일관 경고했으나 2회 반복. 둘 다 worktree 로 mv·메인 repo stray 정리. → 교훈: worktree 작업 시 *상대경로* 또는 *worktree 절대경로* 사용, 산출 직후 `git status` 로 거주 확인. (memory `feedback-worktree-abs-path` 신설.)
- **자체 validator 결과 맹신 금지** — `eng_rows_well_formed` FAIL 을 ratify-only 로 단정했으나 Pane 2 가 SELECT regex hit 추가 적발. 자체검증은 *원인 분해까지* 직접 확인해야 (FAIL 항목의 모든 사유 열거).
- **semantic gap ≠ 구조 PASS** — eng_frozen PASS 여도 profile 이 강제 못 하는 dependency(여기선 runtime JSON lib)는 누락 가능. ADR-0003 closure 의 의미적 완전성은 교차검토/후속 profile v3 후보.

## 후속 (별 사이클, 본 사이클 scope 외)

- **자율 코드 생성** — eng_frozen PASS = GO. NSSF 실 코드 생성이 다음 단계(파이프라인 밖, 프로젝트 최종 목표).
- **profile v3 후보** — json_runtime(+기타 runtime dep) 을 schema_codegen 또는 신 slot 으로 *강제 shape* 화. 본 사이클은 artifact extra-field 로 의미 폐쇄, validator 강제는 미적용 (Pane 2 동의).
- structured F outbound 산출(sbi-clients.md 등 ADR-0003 follow-up) — 미착수.
