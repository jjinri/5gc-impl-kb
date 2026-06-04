# NSSF DOCX 기반 설계 보강 분석

작성일: 2026-06-04
대상: `specs/29.531/29531-j60.docx`
작업 브랜치: `analysis/nssf-docx-design-gaps`
범위: 분석 산출물 작성만. `pr-slicing-plan.yaml` 직접 수정 없음.

## 1. 왜 이 작업이 필요한가

기존 하네스는 OpenAPI YAML에서 API path, schema, response shape를 잘 뽑는다. 그러나 3GPP DOCX 본문이 담고 있는 다음 정보는 약하게 반영되어 있었다.

| DOCX 본문 정보 | OpenAPI YAML만으로 부족한 이유 | 설계 보강 효과 |
|---|---|---|
| 어떤 절차에서 API가 호출되는가 | path/schema는 같아도 registration, PDU, UE config update 의미가 다름 | e2e/contract test matrix 개선 |
| if/when/shall 조건 | schema required만으로 정책 조건을 알기 어려움 | SelectionEngine/SubscriptionStore 조건 테스트 강화 |
| cross-NF/roaming 문맥 | OpenAPI는 AMF/SMF/NWDAF/V-NSSF 흐름을 충분히 설명하지 않음 | operator config와 deferred scope 명확화 |
| notification 예외 | callbackUri가 있어도 notify하면 안 되는 경우가 있음 | fan-out correctness 개선 |
| feature negotiation | supportedFeatures 하나로는 ONSSAI/EANAN/NSRP 등 행위 차이를 알기 어려움 | feature flag matrix와 fail-closed 정책 강화 |

## 2. 이번에 실제로 한 일

### 2.1 DOCX section cache 생성

명령:

```bash
python3 design/scripts/spec-split.py 29.531 --force
```

생성된 주요 파일:

| 파일 | 의미 |
|---|---|
| `specs/29.531/_extracted/05-services-offered-by-the-nssf.md` | 서비스 설명, 절차별 operation prose |
| `specs/29.531/_extracted/06-1-nnssf-nsselection-service-api.md` | NSSelection API URI/HTTP/security/redirect prose |
| `specs/29.531/_extracted/06-2-nnssf-nssaiavailability-service-api.md` | NSSAIAvailability API URI/subscription/notification/security/redirect prose |

### 2.2 분석 산출물 생성

| 산출물 | 목적 |
|---|---|
| `design/nssf/_docx_semantic_analysis.yaml` | DOCX 본문에서 추출한 서비스 의미, 조건, operator input 후보 |
| `dev/nssf/docx-derived-design-gaps.yaml` | 현재 설계/plan 대비 부족한 점과 권장 반영 위치 |
| `docs/kb/nssf-docx-design-gap-analysis.md` | 사람이 읽는 요약 리포트 |

## 3. 핵심 발견

### 3.1 fan-out에서 같은 AMF로 notification을 보내면 안 되는 경우 — #142로 닫힘

DOCX Clause 5.3 Notify prose는 availability update가 AMF에 의해 발생한 경우, NSSF가 그 같은 AMF에 notification을 보내지 말아야 한다고 설명한다.

상태:

- `PR-phase3-fanout-integration` #142, merge commit `e4beb87`에서 구현 완료.
- subscription row에 `amf_id`/`amf_set_id`를 filter JSON과 별도 truth로 저장한다.
- fan-out은 `subscriber amf_id == event nf_id` exact match일 때만 self-notification enqueue를 suppress한다.
- suppression은 enqueue-skip만 수행한다. DELETE change의 matched subscription tombstone cleanup은 유지된다.
- `amfSetId`만 있거나 legacy/null identity인 subscription은 same individual AMF를 확정할 수 없으므로 suppress하지 않는다.

이 항목은 더 이상 open implementation gap이 아니라, 이후 fan-out 변경이 회귀시키면 안 되는 DOCX-derived evidence/guardrail이다.

### 3.2 Subscription PATCH 불변 조건

DOCX는 subscription modification에서 event IE를 변경하면 안 된다고 설명한다. 또한 `taiList` empty array는 ONSSAI feature support 조건이 있다.

권장:

- `SubModifyPatch` 테스트에 event mutation reject 추가.
- ONSSAI feature flag가 없으면 empty `taiList` reject.

### 3.3 subscription expiry는 단순 default TTL 이상

DOCX는 NSSF가 operator policy에 따라 expiry를 결정할 수 있고, expiry를 응답하지 않으면 consumer는 무기한 subscription으로 간주한다고 설명한다. 또한 많은 subscription이 같은 시점에 만료되어 재생성 폭주가 생기지 않도록 같은 expiry를 피해야 한다.

권장:

- operator guide에 `default_expiry`, `allow_indefinite`, `expiry_jitter_seconds` 추가.
- Phase4/Phase5에서 expiry spread 테스트 또는 문서화.

### 3.4 NSSelection GET은 하나의 API지만 여러 절차 의미를 가진다

DOCX는 NSSelection GET이 registration, PDU session, UE configuration update, PDN establishment, NWDAF analytics, roaming/indirect sharing에서 쓰인다고 설명한다.

권장:

- Phase4 contract/e2e matrix를 절차별로 확장.
- 최소 행: registration, PDU, UE config update, PDN, NWDAF NSI lookup, roaming mapping.

### 3.5 307/308 redirect는 target NF header가 필요

DOCX API prose는 NSSF set 내부 redirect에서 307/308을 쓰면 `3gpp-Sbi-Target-Nf-Id` header로 새 NSSF identity를 표시해야 한다고 설명한다.

권장:

- server-side redirect를 지원하지 않으면 operator guide/open-gaps에 명시.
- 지원하면 Phase4 contract/security test에 header 검증 추가.

### 3.6 29.531 Nnssf API 자체는 OAuth2 scope를 정의하지 않음

DOCX는 NSSelection/NSSAIAvailability API 모두 OAuth2를 사용할 수 있지만, API 자체에 정의된 OAuth2 scope는 없다고 설명한다.

권장:

- 현재 outbound OAuth2 scope는 NRF/operator policy로 유지 가능.
- 단, 이를 “29.531 Nnssf-defined scope”처럼 문서화하지 말아야 한다.

## 4. 현재 하네스에서 개선되는 점

| 개선 전 | 개선 후 |
|---|---|
| OpenAPI schema 중심 | schema + procedure prose 결합 |
| fan-out 대상 match만 고려 | fan-out originator suppression 조건 추가 |
| subscription expiry = default TTL | indefinite/jitter/operator policy 포함 |
| NSSelectionGet generic 테스트 | procedure별 matrix 가능 |
| redirect는 주로 outbound M4 관점 | NSSF API server-side redirect header까지 분리 |
| OAuth2 scope가 일반론으로 흐를 위험 | 29.531은 scope 미정의임을 명확화 |

## 5. 반영 권장 순서

현재 window 2에서 `nf-orchestrator`가 phase3 작업 중이므로, 이 분석은 바로 plan을 고치지 않는다.

권장 순서:

1. `DOCX-GAP-001`은 `PR-phase3-fanout-integration` #142, merge commit `e4beb87`로 닫혔으므로 evidence/guardrail로 보존한다.
2. phase3 안정화 후 별도 plan-amendment PR 작성.
3. Phase4 test slices에 `DOCX-GAP-002`, `DOCX-GAP-004`, `DOCX-GAP-005`, `DOCX-GAP-007`, `DOCX-GAP-008` 반영.
4. Operator config guide slice에 `DOCX-GAP-003`, `DOCX-GAP-006`, `DOCX-GAP-009` 반영.

### 5.1 Phase 3 → Phase 4 gate로 남길 일

Phase 3 runtime wiring이 안정화되면, Phase 4 contract/e2e/security test 확장에 들어가기 전에 `DOCX Harness Phase 2`를 별도 gate로 수행한다.

목표:

- 지금처럼 LLM이 직접 DOCX를 읽어 gap을 쓰는 일회성 분석을 반복 가능한 하네스 단계로 바꾼다.
- `spec-split.py`로 만든 `_extracted/*.md`에서 SHALL/SHOULD/IF/WHEN/NOTE 후보를 script로 추출한다.
- LLM은 후보를 고정 schema에 맞춰 분류하고, 모든 항목에 `source_ref`, `target_slice`, `status`, `acceptance`를 남긴다.
- validator가 gap의 open/closed/deferred/not_applicable 상태와 plan/test/operator-guide 연결 여부를 검사한다.
- Phase 1~3 산출물과 개발 소스에 영향이 있으면 focused backfill PR로 닫은 뒤 Phase 4 test matrix를 확장한다.

이 gate의 이유:

> Phase 4는 테스트 확장 단계이므로, OpenAPI-derived contract만 기준으로 삼으면 절차/조건/운영 예외가 다시 누락될 수 있다. Phase 3 완료 후 Phase 4 전이 DOCX prose 분석을 정식 하네스에 넣기 가장 안전한 시점이다.

## 6. 결론

DOCX 분석은 “전체 규격 자동 이해”가 아니라 “OpenAPI YAML이 놓치는 설계 조건을 조기 발견하는 장치”로 가장 가치가 크다.

이번 분석에서 가장 즉시성 있던 보강점은 `DOCX-GAP-001`이었다.

> `PR-phase3-fanout-integration`은 availability update를 발생시킨 AMF에게 notification을 되돌려 보내지 않는 originator suppression 조건을 검증해야 한다.

이 항목은 #142 merge commit `e4beb87`로 닫혔다. 앞으로는 open gap이 아니라 regression 방지 evidence로 보존한다.
