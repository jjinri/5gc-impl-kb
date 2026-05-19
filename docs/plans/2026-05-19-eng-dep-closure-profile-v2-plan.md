# Engineering Dependency Closure — profile v2 + ADR-0003 — 2026-05-19

Status: in progress (2026-05-19)
Current objective: `engineering-core-slots.yaml` v1→v2 — software-using slot 에 library/package/version closure 를 강제해 `eng_frozen` 이 *코드 생성 가능 수준의 닫힌 결정* 을 요구하게 한다. 신 `ADR-0003`(profile policy 강화), validator/skill/문서 v2 동기화. **본 사이클 종료 후 별도 사이클로** `/nf-eng-design nssf` 첫 실행(본 사이클 명시적 제외 — 메커니즘만).

선행: Engineering Design Freeze skill-build (PR #28 `d71f76e`, ADR-0002 Accepted). 본 사이클은 그 retro 가 식별한 후속 — slot typed shape 가 *어느 단계 결정* 은 강제하나 *library/version 닫힘* 은 약했던 gap. Pane 2 second-opinion 다회 라운드로 build-ready 승인.

## 확정 spec (Pane 2 build-ready 승인 — 차단 0)

`design/schemas/engineering-core-slots.yaml`:

- `version: 2`. **13 slot** (v1 12 + `sbi_client_stack` 신설, `sbi_http_framework`→`sbi_server_stack` rename).
- `common_row_fields` 불변 (`source` = 결정 근거/참조 의미 유지).
- **`dependency_shape`** (nested, software-using slot 이 참조):
  - required `[dependency_source, version_policy]`. `dependency_source ∈ {stdlib, third_party, generated, externalized, none}`. `version_policy ∈ {pinned, range, repo_lockfile, runtime_managed, externalized, none}`. `package` 는 `dependency_source ∉ {stdlib, externalized, none}` 일 때 필수.
  - 차단1 해소 — `source` 가 아니라 `dependency_source` (common `source` 와 의미 충돌 제거).
- **표기** (Pane 2 정제 채택): `required_shape` 필드 값은 magic string 이 아니라 object — `{type: scalar}` 또는 `{shape_ref: dependency_shape}`. validator 가 `shape_ref == dependency_shape` 분기. 향후 `arch_ref`/`non_empty_list`/`enum` 확장 용이.
- slot 적용 (차단2 해소 — dependency_shape *실제* nested 적용):
  - `language` `{name:scalar, version:scalar}` · `runtime` 동일 (toolchain decision — dependency_shape 미적용, Pane 2 확인).
  - `deployment_topology` `{form:scalar}` · `module_source_layout` `{layout:scalar}` (software dep slot 아님).
  - `sbi_server_stack` `{server_runtime:scalar, http2_mode:scalar, framework:{shape_ref:dependency_shape}}`.
  - `sbi_client_stack` conditional `discriminant: required` — `required` 진실 source = **F architecture**. `true` → `{required, source_arch_ref, targets(non-empty), http2_mode, timeout_retry_policy, client:{shape_ref:dependency_shape}}`. `false` → `{required, source_arch_ref, rationale}`. validator 결정론 — `source_arch_ref` 가 `design/<nf>/architecture/` 하위 경로·파일 존재·heading/anchor 존재. 구조화 F 산출(sbi-clients.md/external-dependencies.md/_arch_status outbound_capabilities)은 **ADR-0003 follow-up — 본 v2 scope 외** (F skill/validator 미접촉).
  - `schema_codegen` `{generated_artifact_policy:scalar, tool:{shape_ref:dependency_shape}}`.
  - `tls_security` conditional `mode`: `enabled`→`[mode,min_version,cipher_policy,mutual_auth,tls_provider]`, **tls_provider=app_library 면 `tls_dependency:{shape_ref:dependency_shape}` 도 필수** (정제 채택 — v2 thesis 일관); `external`→`[mode,externalized_to]`; `disabled`→`[mode]`.
  - `oauth2_token_validation` conditional `enabled`: `true`→`[enabled, token_validation_strategy, lib:{shape_ref:dependency_shape}]` (`token_validation_strategy ∈ jwks|introspection|externalized`); `false`→`[enabled, rationale]` (common `source` 가 이미 필수 — variant 에 재기재 안 함, 정제 채택: variant-required vs common-required 중복 회피).
  - `persistence` conditional `backend`: `rdbms`→기존 + `driver_or_orm:{shape_ref:dependency_shape}`; `kv`→`[backend,key_schema,durability,client:{shape_ref:dependency_shape}]`; `file`/`in_memory`/`none`→기존(소프트웨어 dep 아님 → dependency_source none/stdlib 가능).
  - `telemetry` `{logging:{shape_ref:dependency_shape}, metrics:{shape_ref:dependency_shape}, tracing:{shape_ref:dependency_shape}}` (nested — 다수 library).
  - `configuration_management` `{secret_strategy:scalar, override_order:scalar, config:{shape_ref:dependency_shape}}`.
  - `test_build_tooling` `{lockfile_policy:scalar, test_runner:{shape_ref:dependency_shape}, build_tool:{shape_ref:dependency_shape}, lint_tool:{shape_ref:dependency_shape}, typecheck_tool:{shape_ref:dependency_shape}}` (nested).

## 빌드 절차

| P | 산출 | 검증 |
|---|---|---|
| P1 plan + ADR-0003 초안 | 본 파일 + `docs/adr/ADR-0003-engineering-dependency-closure.md` + Pane2 | 사용자 승인 |
| P2 profile v2 | `design/schemas/engineering-core-slots.yaml` version:2, 위 spec | yaml lint, 13 slot, dependency_shape, shape_ref |
| P3 validator v2 | `nf-eng-status.py` — `{shape_ref:dependency_shape}`/`{type:scalar}` 해석, dependency_source/version_policy/package 조건부, sbi_client_stack source_arch_ref 결정론 검사, variant∪common required dedup | bare-bool·nested dependency·rdbms·sbi_client true/false 더미로 FAIL→PASS 결정론 재검증 |
| P4 migration | nf-eng-design SKILL(12→13·rename·dependency 포맷 mandate, ENG_CANON 불변) · nf-eng-status SKILL wording · ONBOARDING 12→13 · ADR-0002 extension note · ADR-0003 Accepted | 문서 정합, single-source |
| P5 PR | 커밋 분리(profile=pipeline-policy 별 커밋), gitignored/.venv 제외, push 브랜치+gh pr create | — |
| P6 Pane2 | `$git-pr-review` → 반영 → 사용자 머지 → sync → 정리 → retro+plan-close | — |

## 명시적 제외

- **구조화 F outbound 산출** (sbi-clients.md / external-dependencies.md / `_arch_status` outbound_capabilities) — ADR-0003 follow-up. 본 v2 는 `sbi_client_stack.source_arch_ref` traceability gate 까지만 (F skill/validator scope 확대 회피).
- **NSSF 실 engineering-design.md 작성** — 본 사이클은 메커니즘(profile v2) 만. `/nf-eng-design nssf` 첫 실행은 별 사이클.
- **자율 코드 생성** — `eng_frozen` PASS 이후. 프로젝트 최종 목표.
- ADR-0002 재정의 — amend 아님. ADR-0003 가 profile policy 를 *강화*, ADR-0002 엔 extension note 만.

## 위험 / 함정

- **세 번째 SKILL↔validator drift** — `ENG_CANON` 불변(canonical 섹션 v1=v2 동일). dependency 포맷 mandate 는 nf-eng-design SKILL ↔ nf-eng-status.py 가 single-source.
- **profile = pipeline policy** — 모든 NF eng_frozen 의미 변경. 별 커밋 + ADR-0003 기록.
- validator blocking 결정론 유지 — shape_ref 해석도 결정론. LLM judge 금지.
- 산출 *내용* 직접 검증 — bare YAML bool·nested dependency·conditional 전부 더미 fixture 로 FAIL→PASS 확인 (v1 의 slugify·bool discriminant 버그가 content 검증으로 적발된 교훈 — 원시 YAML 타입 변형까지 fixture 포함).
- worktree `git add .` 금지 (.venv·gitignored 혼입). 특정 파일만.
