---
nf: nssf
stage: architecture-design
status: draft
source_contract: handoff/nssf/contract.yaml
generated_date: 2026-05-14
---

# NSSF Observability

## Purpose

NSSF 의 log · metric · trace · audit point 와 correlation 전파 정책을 정의한다. 본 architecture 가 명시한 signal 이 dev 단계 instrumentation 의 기준선이다.

## Inputs (contract)

- 8 API topics — operation 별 호출 signal.
- `interface` topic — `3gpp-Sbi-Correlation-Info` / `3gpp-Sbi-Producer-Id` 등 SBI header (fresh-full 주의: 현 generated `interface` output 은 materializer `auto_interface` first-YAML-only 구현이라 첫 primary YAML 기반 partial — `overview.md` `## Inputs` 참조).

## Boundaries

본 문서가 다루는 것 — signal *종류*, 수집 위치, correlation key, severity 기준.

본 문서가 다루지 않는 것 — exporter / sink 구체 (Prometheus / OTLP / OpenTelemetry SDK 버전 등 — dev 결정).

## Decisions

### log

| level | 사례 |
|---|---|
| `error` | 5xx 응답, panic 회복, outbound notification 최종 실패 |
| `warn` | 4xx 응답 중 cause unexpected, outbound retry, OAuth2 token 갱신 실패 |
| `info` | inbound request start/finish (operation, status, duration), subscription create/modify/delete |
| `debug` | request body summary (PII 제거 후), 매핑 단계 trace |

### metric

| metric | label | 의미 |
|---|---|---|
| `nssf_request_total` | operation, status_code, cause | inbound 호출 카운트 |
| `nssf_request_duration_seconds` | operation | latency histogram |
| `nssf_subscription_count` | state (active / pending_delete) | SubscriptionStore 의 현재 보유 |
| `nssf_notification_dispatch_total` | result (success / retry / dead_letter) | outbound POST 결과 |
| `nssf_notification_queue_depth` | (gauge) | 미발송 + retry 대기 |
| `nssf_availability_change_total` | tai_bucket, op (put/patch/delete) | 변경 이벤트 발생 |

### trace

- inbound entry 마다 *span* 생성. operation = span name.
- 모듈 dispatch → span attribute `module`.
- outbound notification → child span. `peer.service=amf`, `peer.subscription_id`.
- propagation header — `traceparent` (W3C) + `3gpp-Sbi-Correlation-Info` 동시 부착·소비.

### correlation

본 architecture 가 강제하는 correlation key.

| key | 값 source | 전파 위치 |
|---|---|---|
| `correlation_id` | inbound `3gpp-Sbi-Correlation-Info` 또는 신규 생성 | 모든 log / metric exemplar / trace baggage / outbound `3gpp-Sbi-Correlation-Info` header |
| `subscription_id` | SubscriptionStore 생성 시 | subscription lifecycle log, outbound notification |
| `tai_bucket` | availability 변경 대상 | availability change event log + metric |

correlation-id 는 *반드시* outbound notification 까지 전파. 이는 본 architecture 의 핵심 결정 — AMF 가 변경 사건을 자기 inbound trace 와 매칭 가능.

### audit

- subscription create / unsubscribe → audit log (info level + dedicated stream).
- availability change → audit log (info level).
- OAuth2 client credentials 토큰 발급 / 갱신 → audit (info, secret 제외).

## Open Questions

- PII 처리 — UE identifier (SUPI / GUTI) 가 inbound payload 에 노출되면 log 마스킹 정책 필요. NSSF 가 직접 UE id 받는 operation 은 적으나 *전파* 시점 확인 필요.
- metric 명명 컨벤션 — Prometheus naming vs OpenTelemetry semantic. 본 architecture 는 *추상 이름* 만, dev 가 export 시 확정.
- correlation-id 부재 시 본 NSSF 가 *생성* 하는 정책 — 무조건 생성 vs 부재 허용.

## References

- [[request-flow]] — 모든 단계에 correlation 부착.
- [[runtime-model]] — outbound async + correlation 전파.
- [[error-propagation]] — error severity 와 log level 매핑.
- [[configuration-strategy]] — `log.level`, `metrics.exporter`, `trace.sample_rate`.
