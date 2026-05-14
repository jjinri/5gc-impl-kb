---
nf: nssf
stage: architecture-design
status: draft
source_contract: handoff/nssf/contract.yaml
generated_date: 2026-05-14
---

# SelectionEngine

## Responsibility

NSSF inbound `NSSelectionGet` 의 핵심 로직 — UE 등록 시점 input (`SliceInfoForRegistration` + 컨텍스트) 을 받아 allowed NSSAI 와 target AMF set 을 결정해 `AuthorizedNetworkSliceInfo` 를 반환한다. 본 모듈은 NSSelectionGet 처리 전담 — NSSAIAvailability 의 CRUD / subscription / notification 책임은 가지지 않는다.

## Inputs

- inbound — `api/NSSelectionGet` 의 query + body (`SliceInfoForRegistration` 또는 `SliceInfoForPDUSession` / `SliceInfoForUEConfigurationUpdate`).
- contract data-model — `SliceInfoForRegistration`, `Snssai`, `Tai`, `PlmnId`, `NfInstanceId`, `SupportedFeatures`.
- collaborator (interface) — subscribed NSSAI source (UDM 추상화), PLMN policy source (config 또는 외부), NRF discovery (target AMF set 결정용).
- AvailabilityEngine 의 현재 availability map (TAI × S-NSSAI) — read-only 참조 (slice 가용성 확인).

## Outputs

- 정상 — `AuthorizedNetworkSliceInfo` (200).
- error — 의도된 cause (`INVALID_QUERY_PARAM`, `UNAUTHORIZED_NSSAI`, `NSSAI_NOT_AVAILABLE`) 를 호출자 (handler / ProblemDetailsMapper) 에 반환.
- log/metric/trace event — `request-flow.md` 의 trace span + `observability.md` 의 metric.

## State

- 없음 (stateless). 모든 입력은 인자 또는 collaborator 호출로 수신.
- 캐시 (subscribed NSSAI, PLMN policy) 는 collaborator 내부 책임이며 SelectionEngine 의 상태가 아니다.

## Decisions

| 결정 | 내용 |
|---|---|
| 순수성 | 외부 호출 (UDM, NRF) 은 *interface* 로 추상화. SelectionEngine 본체는 *입력 → 출력* 매핑 함수에 가깝게 유지. |
| 알고리즘 | `allowed = requested ∩ subscribed ∩ plmn_policy ∩ availability[tai]`. 부분 거부 시 `rejectedNssai` 채움. |
| target AMF set | NRF discovery 호출. 실패 시 NSSF 가 cache 된 last-known AMF set 사용 또는 5xx fallback. |
| 두 종류 input | SliceInfoForRegistration / PDUSession / UEConfigurationUpdate 셋 다 처리. Registration 이 핵심 path, 나머지 두 종류는 spec 정의 따라 같은 모듈에서 처리. |
| 부분 거부 정책 | rejected 항목은 응답에 포함, 200 응답 유지. 전체 거부 시 4xx. |

## Open Questions

- subscribed NSSAI 캐시의 stale 정책 — TTL vs invalidate-on-change.
- AMF reallocation via RAN 지원 시 target AMF set 결정 로직 변경 (38.413 운영 결정과 연동).

## References

- [[../architecture/module-boundaries]] — 4 모듈 책임.
- [[../architecture/request-flow]] — NSSelectionGet 시퀀스.
- `handoff/nssf/contract.yaml` `api/NSSelectionGet` topic — 진실 출처.
- `handoff/nssf/contract.yaml` data-model — `SliceInfoForRegistration`, `AuthorizedNetworkSliceInfo`, `Snssai`, `Tai`.
- [[AvailabilityEngine]] — availability map read source.
