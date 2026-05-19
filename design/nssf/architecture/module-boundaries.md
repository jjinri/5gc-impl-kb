---
nf: nssf
stage: architecture-design
status: draft
source_contract: handoff/nssf/contract.yaml
generated_date: 2026-05-14
---

# NSSF Module Boundaries

## Purpose

NSSF 의 두 service · 8 operation 책임을 4 모듈로 분해하고 seam 을 정의한다. dev 단계의 코드 구조가 이 모듈 경계를 *그대로* 따르도록 강제하지는 않으나, 책임 누락·중복 진단의 기준선이 된다.

## Inputs (contract)

- 8 API topics — operation → 모듈 매핑의 입력.
- `interface` topic — 모든 모듈이 공유하는 HTTP/2·OAuth2·SBI header 전제 (fresh-full 주의: 현 generated `interface` output 은 materializer `auto_interface` first-YAML-only 구현이라 첫 primary YAML 기반 partial — `overview.md` `## Inputs` 참조).
- `error-handling` topic — 모든 모듈의 ProblemDetails 매핑 책임 공통 (fresh-full 주의: 현 generated error-handling output 은 status code 목록만 — `error-propagation.md` `## Inputs` 참조).

## Boundaries

4 모듈 + 공통 책임.

| 모듈 | 단일 책임 | 다루는 operation | 다루지 않는 것 |
|---|---|---|---|
| `SelectionEngine` | UE 등록 시점 slice 선택 — subscribed NSSAI → allowed NSSAI 매핑, NRF 조회로 target AMF set 결정 | NSSelectionGet | NSSAIAvailability 의 CRUD·subscription·notification |
| `AvailabilityEngine` | TA 별 S-NSSAI availability 동기 CRUD — AMF 가 제공한 availability 의 store/lookup/patch/delete | NSSAIAvailability Put · Patch · Delete · Options | subscription lifecycle, outbound notification |
| `SubscriptionStore` | AMF 가 availability 변경 통지를 받기 위한 subscription 의 lifecycle persistence — id 발급, 갱신, 만료, 삭제 | NSSAIAvailability Post (Subscribe) · SubModifyPatch · Unsubscribe | outbound HTTP 호출, AvailabilityEngine 의 CRUD 로직 |
| `NotificationDispatcher` | availability 변경 이벤트를 SubscriptionStore 에서 받은 subscription 의 callback URI 로 outbound POST 발송 + correlation-id 추적 + retry | (outbound POST — NSSAIAvailability 변경 통지) | inbound 처리, subscription 의 CRUD |

공통 책임 — 모듈 외부 / 모든 모듈 공유.

- `RequestValidator` 책임 — 8 operation 의 query·body schema validation. 별도 모듈 격상 안 함. 각 entry handler 안 함수 또는 공통 utility.
- `ProblemDetailsMapper` 책임 — error → ProblemDetails 응답 매핑. 같은 위치 (공통 utility).
- HTTP/2·OAuth2·SBI header 처리 — 모듈 외부의 transport 레이어 책임. dev 단계가 라이브러리로 처리.

## Decisions

| 결정 | 내용 |
|---|---|
| 분해 단위 | service 가 아니라 *책임 종류* 로 분해. AvailabilityEngine 과 SubscriptionStore 가 같은 service 의 다른 책임을 가진다. |
| 모듈 격상 기준 | 독립 상태·다른 lifecycle·교체 가능성. RequestValidator / ProblemDetailsMapper 는 셋 다 미흡 — 모듈 미격상. |
| outbound vs inbound 분리 | NotificationDispatcher 만 outbound. 나머지 3 모듈은 inbound 책임. transport 레이어가 inbound 와 outbound 양쪽을 다룸. |
| 모듈 간 호출 방향 | inbound entry → SelectionEngine / AvailabilityEngine / SubscriptionStore. AvailabilityEngine 이 변경 이벤트 발행 → SubscriptionStore lookup → NotificationDispatcher. |

## Open Questions

- AvailabilityEngine ↔ SubscriptionStore 의 *변경 이벤트* 가 in-process call 인지 message bus 인지 — 본 architecture 단계는 *추상 이벤트* 로만 명시, dev 가 선택.
- NotificationDispatcher 의 retry queue 가 SubscriptionStore 와 같은 backend 공유하는지 분리하는지 — `state-persistence.md` `## Open Questions` 와 연동.

## References

- [[overview]] — full scope 정의.
- [[request-flow]] — 모듈 간 호출 시퀀스.
- [[runtime-model]] — long-lived state 와 request-response 분리.
- [[state-persistence]] — SubscriptionStore backend.
- `handoff/nssf/contract.yaml` — 8 API topics 의 method / path / depends_on.
