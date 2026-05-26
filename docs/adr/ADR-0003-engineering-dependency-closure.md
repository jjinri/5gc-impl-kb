# ADR-0003 — Engineering Dependency Closure in eng_frozen Profile v2

Date: 2026-05-19
Status: Accepted (2026-05-19); cross-referenced 2026-05-26 — operationalized in `engineering-core-slots.yaml` v2

> ADR-0002 가 Engineering Design Freeze 단계와 `eng_frozen` 게이트를 정의하고 Accepted 됐다. ADR-0003 은 그 *accepted profile policy* 를 version 1 → 2 로 확장한다 — software-using slot 에 대해 software dependency/library/version closure 를 `eng_frozen` 에 포함시킨다. ADR-0002 를 재정의하지 않으며, ADR-0002 에는 extension note 만 추가한다.
>
> **Accepted** — profile-v2 사이클 (plan `docs/plans/2026-05-19-eng-dep-closure-profile-v2-plan.md`) 에서 v2 schema 가 Pane 2 second-opinion 다회 라운드로 build-ready 승인되고 사용자 승인됨. 구현 = `engineering-core-slots.yaml` v2(13 slot, `dependency_shape`, `{shape_ref}`/`{type}` 표기) + `nf-eng-status.py` v2 + skill/ONBOARDING/ADR migration. FAIL→PASS 결정론 검증 통과 (nested dependency·sbi_client bare-bool·conditional tls·oauth2 dedup). 구조화 F outbound 산출은 follow-up (Consequences 참조).

## Context

ADR-0002 + skill-build 사이클(PR #28)로 `eng_frozen` 메커니즘이 가동됐다. 그 사이클 retro 가 식별한 gap — profile v1 의 slot typed shape 는 *어느 단계에서 결정하나* 는 강제하나 *어떤 library/package/version 인지 닫혔는가* 는 약하다. 예: `sbi_http_framework={name,http2}` 에 package/version 없음, `oauth2_token_validation={enabled,token_source,scope_check}` 에 실제 library 없음, `telemetry` 가 library 인지 mode 인지 모호. 그 결과 코드 생성 agent 가 `eng_frozen` PASS 이후에도 library/version 을 임의 발명할 수 있어, `eng_frozen` 이 *자율 코드 생성 GO 신호* 라는 목적이 약화된다.

또한 outbound NF 호출 필요성(예 NSSF 의 NotificationDispatcher → AMF callback)은 F architecture 가 도출하는 *required capability* 이지 Engineering Design 에서 처음 발견할 것이 아니다. v1 엔 inbound `sbi_http_framework` 만 있어 outbound client 결정을 구조적으로 강제하지 못했다.

## Decision

Normative source: [`design/schemas/engineering-core-slots.yaml`](../../design/schemas/engineering-core-slots.yaml) v2 — `dependency_shape`, `sbi_server_stack`/`sbi_client_stack`, `common_row_fields`, slot 별 `required_shape`. 본 ADR §Decision 의 5점 모두 v2 schema 에 implementation. ADR 본문은 rationale + considered options 만 유지.

`design/schemas/engineering-core-slots.yaml` 을 `version: 2` 로 강화한다 (pipeline policy 변경 — 모든 NF 의 `eng_frozen` 의미를 바꾼다).

1. **dependency closure.** `dependency_shape` (nested) 를 도입 — `dependency_source ∈ {stdlib,third_party,generated,externalized,none}` + `version_policy ∈ {pinned,range,repo_lockfile,runtime_managed,externalized,none}` + `package` (dependency_source ∉ {stdlib,externalized,none} 일 때 필수). software-using slot(sbi_server/client·schema_codegen·oauth2·persistence rdbms/kv·telemetry·configuration·test_build)이 이를 *실제 nested 적용* 한다. `language`/`runtime` 은 toolchain decision 이라 미적용.

2. **source 의미 분리.** dependency 의 출처는 `dependency_source` 다. `common_row_fields.source`(결정 근거/참조)와 같은 row 안에서 충돌하지 않게 분리한다.

3. **shape 표기.** `required_shape` 필드 값은 magic string 이 아니라 object — `{type: scalar}` 또는 `{shape_ref: dependency_shape}`. validator 가 `shape_ref` 로 분기한다. 향후 `arch_ref`/`non_empty_list`/`enum` shape 확장에 열려 있다.

4. **inbound/outbound 분리.** v1 `sbi_http_framework` → `sbi_server_stack` (server-only 명확화). 신설 `sbi_client_stack` (conditional `discriminant: required`). `required` 의 진실 source 는 **F architecture** — `required=true` 면 `source_arch_ref`·`targets`(non-empty)·client dependency 등을 닫고, `false` 면 `source_arch_ref`·`rationale` 로 정당화한다. validator 는 `source_arch_ref` 가 `design/<nf>/architecture/` 하위 경로·파일 존재·heading/anchor 존재인지 *결정론적으로* 검사한다.

5. **NF-invariant.** `dependency_source: stdlib|externalized|none` variant 가 모든 NF 에 적용 가능성을 보장한다(특정 NF 만 RDBMS·OAuth2 등 — none/externalized 로 닫음).

## Considered options

- **profile v1 에 flat 필드만 추가.** per-slot 반복 많고 telemetry 같은 다중 library slot 의 closure 가 약하다. → nested `dependency_shape` 재사용.
- **`source` 를 dependency·근거 양쪽에 그대로.** 같은 row 두 의미 충돌, validator·작성자 혼동. → `dependency_source` 분리.
- **구조화 F outbound 산출(sbi-clients.md 등)을 이번에 강제.** F skill/validator 까지 번져 scope 과대. → 이번엔 `sbi_client_stack.source_arch_ref` traceability gate 까지, 구조화 F 산출은 follow-up.
- **ADR-0002 amend.** ADR-0002 는 단계/게이트 구조 결정으로 이미 Accepted. profile v1→v2 는 후속 *정책 강화* 라 별 ADR 이 맞다. → ADR-0003 + ADR-0002 extension note.
- **magic string `"dependency"` 표기.** 확장 불가·모호. → `{shape_ref: dependency_shape}` object.

## Consequences

- **profile = pipeline policy.** v2 변경은 모든 NF 의 `eng_frozen` 의미를 바꾼다. 별 커밋 + 본 ADR 기록. 향후 v3 변경도 ADR 동반.
- **v1→v2 migration.** 기존 `engineering-design.md` 산출 NF 0 → 데이터 migration clean. doc/skill migration 필요 — profile `version:2`, nf-eng-design SKILL 12→13 slot·`sbi_http_framework`→`sbi_server_stack` rename·dependency 포맷 mandate, nf-eng-status SKILL wording, ONBOARDING 12→13, ADR-0002 extension note.
- **세 번째 SKILL↔validator drift 쌍 유지.** dependency 포맷·`ENG_CANON`(불변)은 nf-eng-design SKILL ↔ nf-eng-status.py single-source.
- **follow-up.** F architecture 의 구조화 outbound capability 산출(sbi-clients.md / external-dependencies.md / `_arch_status` outbound_capabilities)은 별 사이클 — `sbi_client_stack.required` 의 F-도출을 ref 문자열 traceability 이상으로 검증하려면 F skill/validator 확장 필요.

## References

- `docs/adr/ADR-0002-engineering-design-freeze.md` — Engineering Design Freeze 단계·`eng_frozen` 게이트 (본 ADR 가 profile policy 를 확장; ADR-0002 엔 extension note)
- `design/schemas/engineering-core-slots.yaml` — profile v2 (구현)
- `docs/retros/2026-05-19-eng-design-freeze-skill-build-summary.md` — v2 gap 식별 출처
- `CONTEXT.md` — **개발 설계**·`eng_frozen`·**게이트** 용어
- Pane 2 second-opinion 다회 라운드 (2026-05-19) — build-ready 수렴
