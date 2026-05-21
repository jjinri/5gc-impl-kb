# Engineering Dependency Closure profile v2 — 사이클 회고 (2026-05-19)

> **Historical record — 2026-05-21 note.** 본 사이클의 profile v2 (slot typed shape) 자체는 유효. 단 본 retro 의 후속 작업 항목 (33.501/38.413/AMF ratify) 중 33.501 결정은 2026-05-21 사이클 (PR #33~#36) 에서 `docs/adr/ADR-0004-project-security-baseline.md` 으로 흡수되어 NSSF engineering-design 재-ratify 됨. profile v3 강화 (typed shape level enforcement) 는 ADR-0004 의 Deferred follow-up.

plan: `docs/plans/2026-05-19-eng-dep-closure-profile-v2-plan.md` (closed)
PR: #29 (`3cf1aed` — `162e487` profile v2 / `3c3d62d` migration / `e79de97` decision-support)
ADR: `docs/adr/ADR-0003-engineering-dependency-closure.md` (Accepted), ADR-0002 extension note
선행: Engineering Design Freeze skill-build (PR #28). 그 retro 가 식별한 gap 의 의도된 후속.

## 한 일

`engineering-core-slots.yaml` v1→v2 — software-using slot 에 library/package/version closure 강제.

- `dependency_shape` (nested): `dependency_source`(stdlib|third_party|generated|externalized|none) + `version_policy` + `package`(조건부). software-using slot 이 `{shape_ref: dependency_shape}` 로 참조.
- `required_shape` 필드 표기 = object `{type: scalar|non_empty_list}` | `{shape_ref: dependency_shape}` (magic string 폐기 — 확장성).
- `sbi_http_framework`→`sbi_server_stack` rename + `sbi_client_stack` conditional 신설 (outbound NF 호출, `required` 진실 source = F architecture, `source_arch_ref` 결정론 검사). 12→13 slot.
- `nf-eng-status.py` v2 대응, `nf-eng-design` SKILL 13 slot + dependency mandate, ONBOARDING 12→13, ADR-0002 extension note, ADR-0003 Accepted.
- **decision-support mandate** (Pane 2 가이드, e79de97) — 비개발자 전제: 미결정 slot 에 considered≥2·criteria·recommendation·rejected·consequence 제시 후 사람 ratify. language constraint vs choice 분기.

## 결정·근거

- **gap = library/version 닫힘 미강제** (v1 의 slot typed shape 는 "어느 단계 결정"만 강제). codegen agent 가 eng_frozen PASS 후 library 발명 가능 → eng_frozen GO 신호 목적 약화.
- **ADR-0003 (amend 아닌 extension)** — ADR-0002 는 단계/게이트 구조 결정으로 유효, profile policy 강화는 별 ADR + ADR-0002 extension note.
- **sbi_client_stack.required source=F architecture** — outbound 필요성은 F 도출(NotificationDispatcher 등), library 결정은 Engineering Design. v2 는 `source_arch_ref` traceability gate(경로/파일/anchor 결정론)까지, 구조화 F 산출은 follow-up.
- **Pane 2 차단 2건** (source 충돌→`dependency_source`, dependency_shape 미적용→nested 실적용) + 정제(shape_ref object 표기·tls app_library conditional·oauth2 dedup) + decision-support mandate 전량 반영.

## 함정·교훈

- **content 직접 검증이 closure 강제력 확인** — positive 더미(13+7 PASS) + Pane 2 의 *negative* fixture(package 누락·invalid source_arch_ref·tls_dependency 누락·empty targets → eng_slot_typed_shape FAIL *as expected*). v1 의 slugify·bool-discriminant 버그 교훈대로 bare-YAML-type 변형 fixture 포함 → 본 사이클 버그 0.
- **gap 이 validator 가 아니라 UX 였음** (Pane 2 식별) — schema 만 강화하면 비개발자가 후보·트레이드오프 모른 채 ratify. decision-support mandate 로 SKILL 에 제도화 (PR scope 적합 — 이미 nf-eng-design SKILL 수정 중).
- **단일 source 유지** — ENG_CANON(불변) + dependency 포맷 mandate 가 nf-eng-design SKILL ↔ nf-eng-status.py single-source (세 번째 drift 쌍, ADR-0002 Consequences).
- profile = pipeline policy → 별 커밋 + ADR 기록 (모든 NF eng_frozen 의미 변경).

## 미결·후속

- **`/nf-eng-design nssf` 첫 실행** (별 사이클) — NSSF 13 slot+7 register 결정 AI 초안+decision-support → 사람 ratify. **language(C/Go/Python) constraint vs choice 결정** 이 그 사이클의 핵심 — 고정 제약이면 CONTEXT/ADR 에 먼저 기록·consume, 선택이면 considered/criteria/recommendation/rejected/consequence 안내. 33.501/38.413/AMF 도 ratify. → eng_frozen PASS.
- **구조화 F outbound 산출** (sbi-clients.md / external-dependencies.md / `_arch_status` outbound_capabilities) — ADR-0003 follow-up. `sbi_client_stack.required` 의 F-도출을 ref traceability 이상으로 검증하려면 F skill/validator 확장 필요. 별 사이클.
- **자율 코드 생성** — eng_frozen PASS 이후. 프로젝트 최종 목표(3GPP spec → 동작 NSSF 코드).
- local main sync — 본 사이클 메인 repo Pane2 WIP 없어 clean FF (`88c52b1..3cf1aed`).
