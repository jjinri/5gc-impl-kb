# Engineering Design Freeze skill-build — 사이클 회고 (2026-05-19)

plan: `docs/plans/2026-05-19-eng-design-freeze-skill-build-plan.md` (closed)
PR: #28 (`d71f76e` — `ce20baf` profile / `207e09b` mechanism / `de7f242` bool fix)
선행: grill-with-docs 2026-05-19 (PR #27 `032ab80`, ADR-0002 Proposed + CONTEXT.md). 본 사이클이 그 의도된 후속 구현.

## 한 일

ADR-0002 Engineering Design Freeze 단계의 *메커니즘* 구현.

- `design/schemas/engineering-core-slots.yaml` — 12 core slot profile, per-slot typed shape, conditional/discriminated (persistence/tls_security/oauth2 는 discriminant variants), common_row_fields (모든 frozen row `ratified_by`/`date` 필수), 내부 `version:1`. pipeline policy → 별 커밋.
- `design/scripts/nf-eng-status.py` — `eng_frozen` 게이트 validator. 결정론 blocking (inventory = profile ∪ per-NF 연기 레지스터, slot typed shape incl conditional, deferral register missing/parsefail=FAIL). `advisory.impl_plan_alignment` 별도 namespace·비차단. nf-arch-status.py 패턴.
- `.claude/skills/nf-eng-design`(생성, AI 초안→사람 ratify, DB schema 확정 위치, `ENG_CANON` single-source) · `.claude/skills/nf-eng-status`(read-only 측정).
- `.gitignore` `engineering/*/_engineering_status.yaml`(협소) · ONBOARDING 단계 I/J + 표 · lifecycle ADR-0001 단계 1행 · ADR-0002 Status Proposed→Accepted, Open choices→Decided.

## 결정·근거

- **R1~R5 ratify**(plan 승인) — tracking policy, core slot 12, canonical 5 섹션, out-of-scope/frozen-row ratify schema, advisory 산식. ADR-0002 Open choices 를 이로써 Decided 로 잠금 → Accepted.
- **Pane 2 second-opinion 다회 라운드** — R2 11→12(+configuration_management), R3 Open Questions PASS 규칙, R4 conditional typed schema(+DB schema staging: F=필요성·G=연기·/nf-eng-design=AI초안·사람 ratify·codegen=소비), R5 좁힘, "skill triplet"→"2 skill+1 script", 모든 frozen row ratify(decided 도) 필수.
- **단계 명명** — ONBOARDING 기존 단계 H(impl-status)와 충돌 회피 → Engineering Design Freeze = 단계 I(생성)+J(게이트).
- **scope 한정** — 메커니즘만. NSSF 실 engineering-design.md·33.501/38.413/AMF ratify 는 별 사이클(`/nf-eng-design nssf` 첫 실행) — 메커니즘 PR ≠ NSSF 첫실행 PR 로 책임경계 선명.

## 함정·교훈

- **content 검증이 두 실버그 적발** (지표·존재 아닌 산출 직접 검증):
  - `slugify` ASCII-only → 한글 연기 레지스터 choice 가 빈 slug 붕괴 → unicode-aware 수정.
  - `oauth2_token_validation.enabled` bare YAML boolean → `str(True)!="true"` variant 미스 → false-negative 가 `eng_frozen`(자율코드생성 GO) 차단. discriminant lookup `str(val).strip().lower()` 정규화.
- **test gap** — 1차 FAIL→PASS sanity 가 따옴표 string(`"false"`)만 써 bare-bool 케이스 누락. Pane 2 가 bare bool fixture 로 적발. 교훈 — typed/discriminant 검증은 *원시 YAML 타입 변형*(bare bool/quoted/case)까지 fixture 에 포함.
- **single-source 강제** — `ENG_CANON`(validator) ↔ nf-eng-design SKILL mandate 가 세 번째 SKILL↔validator drift 쌍(ADR-0002 Consequences). SKILL 에 명시.
- **프레이밍 정정** — ONBOARDING "구현 코드 작성 본 문서 범위 밖" 이 *프로젝트 범위 밖* 으로 오독 가능 → "코드 생성은 프로젝트 최종 목표, 본 문서는 eng_frozen GO 게이트까지의 하네스" 로 명확화. 자율 코드 생성은 eng_frozen PASS 이후 *다음 단계*.

## 미결·후속

- **`/nf-eng-design nssf` 첫 실행** (별 사이클) — NSSF 실 12 slot + 7 register 결정 AI 초안 → 사람 ratify(33.501/38.413/AMF 포함). `eng_frozen` PASS 목표.
- **자율 코드 생성** — `eng_frozen` PASS 이후. 프로젝트 최종 목표(3GPP spec → 동작 NSSF 코드). 파이프라인 밖 단계.
- local main sync — 본 사이클은 메인 repo Pane2 WIP 없어 clean FF (`032ab80..d71f76e`). 정상.
- **profile typed shape library/version 강제 보강** (Pane 2, 사이클 종료 시 식별) — 현 slot typed shape 는 *어느 단계 결정하나* 는 강제하나 *library/package/version 이 닫혔는지* 는 약함 (예 `sbi_http_framework={name,http2}` 에 package/version/server_runtime 없음, `oauth2={enabled,token_source,scope_check}` 에 실제 library 없음, `telemetry` library vs mode 모호). eng_frozen "재량 0" 취지상 코드생성 agent 가 여전히 library/version 발명 가능. → **별 profile-bump 사이클** (`engineering-core-slots.yaml` `version:1→2`, software-using slot 에 `library`/`package`/`version_policy` 강제: sbi_http_framework=package+version_policy+server_runtime, schema_codegen=tool+version_policy, oauth2=library+jwks/introspection_strategy, persistence=driver_or_orm+migration_tool+version_policy, telemetry=logging/metrics/tracing library, configuration_management=config_library+secret_strategy, test_build_tooling=runner/build/lint/type). pipeline-policy 변경이라 PR+ADR 기록. **`/nf-eng-design nssf` 첫 실행 *전*에 수행** 권고 — 안 그러면 NSSF eng-design 이 under-specified slot 으로 freeze 되어 eng_frozen 목적 약화.
