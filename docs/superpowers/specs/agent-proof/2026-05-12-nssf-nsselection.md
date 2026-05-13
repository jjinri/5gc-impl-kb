# NSSF NSSelectionGet 구현 plan

본 plan — `handoff/nssf/_handoff.yaml` 의 task `nssf-api-nsselection-get` (phase `02-api-implementation`) 한정. 외부 자료·웹 검색 없음. 모든 근거는 yaml 과 yaml `default_read_order` 가 가리키는 design 페이지에서만.

## Read

`agent_contract.default_read_order` + `tasks.nssf-api-nsselection-get.read` 순으로 읽은 파일들.

1. `handoff/nssf/_handoff.yaml` — entry point, categories/topics/tasks/spec_index/sources.
2. `handoff/nssf/_handoff.yaml#categories` — 7 카테고리 status 표 (interface/error-handling/api/data-model = handoff_ready, module-decomposition topic 단 SelectionEngine 만 handoff_ready, 나머지 draft·persistence deferred).
3. `design/nssf/api/NSSelectionGet.md` — target topic. API Matrix (GET `/network-slice-information`, idempotent, OAuth, `nnssf-nsselection`), Request query 파라미터 표, Response code 표, Implementation Notes (handler stateless·JSON-in-query url-encode).
4. `design/nssf/data-model/SliceInfoForRegistration.md` — request body schema 의 핵심 query (registration 경로). subscribedNssai / requestedNssai / allowedNssaiCurrentAccess / mappingOfNssai 의미.
5. `design/nssf/data-model/AuthorizedNetworkSliceInfo.md` — 200 응답 body. authorizedNssai / targetAmfSet / candidateAmfList / rejectedNssaiInPlmn / rejectedNssaiInTa / nsiInformationList 의미.
6. `design/nssf/module-decomposition/SelectionEngine.md` — 내부 분해 (scope: NSSelectionGet MVP only). Handler → Validator → SelectionEngine → {SubscriptionLookup, PolicyResolver, NsiResolver} → ResponseBuilder · ProblemDetailsBuilder.
7. `design/nssf/interface.md` — OAuth 2.0 (client credentials, NRF issuer, scope `nnssf-nsselection`), HTTP/2 over TLS, base URL `{apiRoot}/nnssf-nsselection/v1`, `3gpp-Sbi-*` 헤더.
8. `design/nssf/error-handling.md` (anchors `#nsselection-400`, `#nsselection-403`, `#nsselection-404`) — ProblemDetails 매트릭스 (400 INVALID_QUERY_PARAM, 403 UNAUTHORIZED_NSSAI, 404 NSSAI_NOT_AVAILABLE, 500 SYSTEM_FAILURE), Content-Type `application/problem+json`, `invalidParams` 다중 항목 한 응답에 담기.

`must_not` 준수 — `design/nssf/_archive/` 자료, draft·blocked 토픽, `(참조 규격 미등록)` leaf 는 인용하지 않음. `slice-info-request-for-pdu-session` 경로 / NSSAIAvailability / persistence 는 명시적으로 MVP 범위 밖이므로 본 plan 에서 다루지 않음.

## Implementation

본 절은 `module-decomposition/SelectionEngine` 의 책임 분배와 `api/NSSelectionGet` · `interface` · `error-handling` 토픽 내용만 근거로 한다. 라이브러리·언어·프레임워크 선택은 `may_decide` 영역.

### 1. HTTP 진입 — `NSSelectionGet` Handler

- Route — `GET {apiRoot}/nnssf-nsselection/v1/network-slice-information` (interface — base_url, apiVersion=v1; api-matrix — method=GET, path=`/network-slice-information`).
- Auth — OAuth 2.0 client credentials 토큰 검증, scope `nnssf-nsselection` 일치 확인 (interface auth-block + api-matrix scope/auth). 토큰 실패 → ProblemDetails (HTTP 401/403 — error-matrix 의 403 UNAUTHORIZED_NSSAI 는 *NSSAI 정책* 거부 전용이므로 토큰 자체 거부는 표 외 일반 401/403; 본 task 의 acceptance 가 명시한 400/403/404 만 self-judge 범위).
- Transport — HTTP/2 over TLS (interface). `3gpp-Sbi-*` 헤더 전달 — TLS termination 이 service mesh 든 NF 내장이든 헤더 보존 (interface Implementation Notes).
- Content negotiation — 200 응답은 `application/json`, 4xx/5xx 응답은 `application/problem+json` (api-matrix produces, error-handling Implementation Notes RFC 7807).
- Idempotency — GET 의 HTTP 의미상 idempotent, side effect 없음 (api-matrix idempotent=true; SelectionEngine Implementation Notes — handler stateless).

### 2. 요청 파싱 — Request Validator

- 필수 query — `nf-type`, `nf-id`, `tai`, `slice-info-request-for-registration` (api request-schema 의 required=true 행 + Implementation Notes — registration 경로만 본 MVP).
- 선택 query — `home-plmn-id` (roaming), `supported-features` (feature negotiation).
- 상호 배타 — `slice-info-request-for-registration` 과 `slice-info-request-for-pdu-session` mutually exclusive; 본 MVP 는 후자 경로 없음 (api request-schema 비고).
- JSON-in-query 역직렬화 — `slice-info-request-for-registration` 은 yaml `parameters.content.application/json.schema` 기반. url-decode 후 JSON 파싱 → `SliceInfoForRegistration` 객체 (api Implementation Notes).
- 위반 — 누락·형식 오류 시 400 ProblemDetails (`cause=INVALID_QUERY_PARAM`). 여러 필드 위반은 `invalidParams` array 에 한꺼번에 (error-handling Implementation Notes).

### 3. 핵심 로직 — `SelectionEngine`

순수 함수 — `(SliceInfoForRegistration, UE context) → AuthorizedNetworkSliceInfo` (module-decomposition 책임 분배).

- **SubscriptionLookup** — UDM 의 subscribed NSSAI 확보. 본 MVP — UDM client skeleton 만, 실제 cross-NF 호출은 후속 사이클 (module-decomposition Implementation Notes + categories.cross-nf=draft). 입력으로 들어온 `subscribedNssai` 는 *복제본*, NSSF 가 진실 출처 (UDM) 와 재대조 (data-model/SliceInfoForRegistration Implementation Notes).
- **PolicyResolver** — `requested ∩ subscribed ∩ PLMN policy`. 비어있는 `requestedNssai` 면 default subscribed NSSAI 적용 (data-model/SliceInfoForRegistration Implementation Notes). roaming 인 경우 `mappingOfNssai` 를 통해 home ↔ visited S-NSSAI 매핑 후 본 PLMN S-NSSAI 만 사용.
- **NsiResolver** — 허용된 슬라이스마다 NSI instance 조회. 본 MVP — stub (module-decomposition Implementation Notes — 실제 NRF 호출은 후속 사이클).

### 4. 응답 직렬화

- 정상 — Response Builder 가 `AuthorizedNetworkSliceInfo` 생성 (data-model/AuthorizedNetworkSliceInfo).
  - `authorizedNssai` — UE 가 본 PLMN 에서 사용 가능한 S-NSSAI 집합.
  - `targetAmfSet` / `candidateAmfList` — AMF 추천 (candidateAmfList 는 비어있을 수 있음).
  - `rejectedNssaiInPlmn` / `rejectedNssaiInTa` — 거부된 S-NSSAI + 사유.
  - `nsiInformationList` — slice 별 NSI ID (NRF 등록).
- 거부 정책 → 403 — `UNAUTHORIZED_NSSAI` (PolicyResolver 결과 모든 요청 NSSAI 가 PLMN policy 외).
- slice instance 부재 → 404 — `NSSAI_NOT_AVAILABLE` (NsiResolver 가 매칭 NSI 없음).
- 내부 오류 → 500 — `SYSTEM_FAILURE` (error-handling matrix; 본 task acceptance 의 enumeration 에는 없으나 error matrix 에는 존재 — 본 MVP 는 default catch-all 로 매핑).
- ProblemDetails 직렬화 — `application/problem+json`, `cause` enum 은 TS 29.500 §5.2.7.2 generic + TS 29.531 §6.1.7.3 NSSF-specific 둘 다 허용 (error-handling Implementation Notes).

### 5. 분해 → 모듈 매핑 (요약)

| 모듈 (논리) | 책임 출처 |
| --- | --- |
| `NSSelectionGetHandler` | module-decomposition Handler 행 |
| `RequestValidator` | module-decomposition Validator 행 + api Implementation Notes |
| `SelectionEngine` | module-decomposition SelectionEngine 행 |
| `SubscriptionLookup` (UDM 캐시 skeleton) | module-decomposition + data-model/SliceInfoForRegistration |
| `PolicyResolver` (NSSAI ∩ PLMN policy) | module-decomposition + data-model/SliceInfoForRegistration |
| `NsiResolver` (NRF lookup stub) | module-decomposition + data-model/AuthorizedNetworkSliceInfo |
| `AuthorizedSliceInfoBuilder` (200) | module-decomposition + data-model/AuthorizedNetworkSliceInfo |
| `ProblemDetailsBuilder` (4xx/5xx) | module-decomposition + error-handling |

`may_decide` 범위 — 패키지 이름·router 등록 방식·로깅 라이브러리·테스트 runner 매핑. `must_ask_or_block` 영역 — timeout/retry 의 *정책 값* (api-matrix 에 "정책 — agent 가 환경에 맞춰 결정 가능" 으로 명시), cross-NF 상대 op (UDM/NRF) 가 본 MVP 에서 stub.

## Produces

`tasks.nssf-api-nsselection-get.produces` 의 placeholder 패턴 그대로. 구체 경로는 dev `may_decide` — 본 plan 에서는 다음과 같이 결정.

- `<impl>/api/nsselection.*` → `dev/nssf/api/nsselection_get_handler.{ext}` (Handler + Validator), `dev/nssf/api/nsselection_get_engine.{ext}` (SelectionEngine + 하위 resolver stub), `dev/nssf/api/nsselection_get_problem.{ext}` (ProblemDetailsBuilder). 확장자·언어는 dev agent 환경에서 정함.
- `<test>/test_nsselection.*` → `dev/nssf/tests/test_nsselection_get.{ext}` (handler integration test + SelectionEngine unit test). module-decomposition Implementation Notes 의 "SelectionEngine 은 순수 함수 → unit, Handler/Validator 는 integration" 분할 그대로.

placeholder 형태인 `<impl>` / `<test>` 는 `may_decide` 의 "내부 패키지/모듈 이름" 에 해당.

## Acceptance

`tasks.nssf-api-nsselection-get.acceptance` 의 5 개 항목을 self-judge 기준으로 그대로 채택.

1. `handles 200/400/403/404 responses per error-handling matrix` — error-handling matrix 의 4 코드를 모두 ProblemDetailsBuilder + AuthorizedSliceInfoBuilder 가 처리. (500 SYSTEM_FAILURE 는 matrix 에 있으나 본 acceptance enumeration 외 — default catch-all 로만 처리.)
2. `preserves idempotency policy (GET, no side effects)` — api-matrix idempotent=true + SelectionEngine 의 stateless 분해. Handler 는 어떤 mutation 도 일으키지 않음 (SubscriptionLookup 의 UDM 캐시 read-only).
3. `request body schema matches data-model/SliceInfoForRegistration JSON` — RequestValidator 가 `slice-info-request-for-registration` query (JSON-in-url-encoded) 를 `design/nssf/data-model/SliceInfoForRegistration.json` 스키마로 검증.
4. `response body schema matches data-model/AuthorizedNetworkSliceInfo JSON` — AuthorizedSliceInfoBuilder 의 200 출력이 `design/nssf/data-model/AuthorizedNetworkSliceInfo.json` 스키마 준수.
5. `negative test exists for each error cause (INVALID_QUERY_PARAM, UNAUTHORIZED_NSSAI, NSSAI_NOT_AVAILABLE)` — `test_nsselection_get` 안에 3 cause 별 케이스 (필수 query 누락 → 400, 모든 NSSAI 가 PLMN policy 외 → 403, 매칭 NSI 부재 → 404).

`blocked_by` 검사 — 본 task 의 `blocked_by` 3 항목 (api/NSSelectionGet, data-model/SliceInfoForRegistration, data-model/AuthorizedNetworkSliceInfo) 모두 yaml 의 topics 표에서 `handoff_ready` 이상 (`handoff_ready` / `canonical` / `canonical`). 따라서 시작 허용.

## PASS receipt

date: 2026-05-13
schema: handoff-v2
nf: nssf
api: NSSelectionGet
criteria_passed: [1, 2, 3, 4]
scorer: plan 실행자 (general-purpose subagent + 본 세션 검토)

채점 근거 —
1. **read order follows agent_contract.default_read_order** — `## Read` 1번이 `_handoff.yaml` (entry), 3번이 target topic (`api/NSSelectionGet`), 4-5번이 depends_on (data-model 2), 6번이 module-decomposition, 7번이 interface, 8번이 error_refs (error-handling). agent_contract 순서 (entry → categories → target topic → depends_on → related → error_refs) 와 일치.
2. **draft/blocked 토픽 implement 대상 제외** — `## Implementation` 어디에도 service-scenarios / behavior-state / failure-policy / configuration / persistence / test-matrix / work-plan / cross-nf (모두 draft) 산출 없음. SelectionEngine (handoff_ready, scope=MVP) 만 포함.
3. **yaml 외 토픽 invent 없음** — `## Produces` 가 yaml `tasks.nssf-api-nsselection-get.produces` 의 `<impl>/api/nsselection.*` + `<test>/test_nsselection.*` 패턴 그대로. yaml 에 없는 토픽 ID 등장 없음.
4. **task schema isomorphic** — 4 절 헤더 (Read / Implementation / Produces / Acceptance) 가 yaml task schema 4 키 (read / 비-yaml 분석 / produces / acceptance) 와 1:1.
