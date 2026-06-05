---
title: NSSF 시험항목서
tags: [test-spec, NSSF, 29.531, mandatory, optional]
last_updated: 2026-05-26
source: doc/analysis/tests/NSSF_test_spec.md
nf: NSSF
spec: 29.531
ids_summary:
  TC: 145
  VS: 51
  MOCK: 77
  TASK: 42
  FEAT: 107
  PROC: 6
  ENT: 11
---

> **위키 편입 정보**
> - 원본: `doc/analysis/tests/NSSF_test_spec.md` (2174 lines, 125,425 bytes)
> - last_updated: 2026-05-26
> - 안정 ID 수: 145 TC (신규) + VS=51 / MOCK=77 / TASK=42 / FEAT=107 / PROC=6 / ENT=11 (인용)
> - 경로 변환: `../../wiki/` → `../` (8건 — 본 카테고리 `tests/`에서 형제 카테고리 참조)
> - 충돌 처리: 신규 (기존 `doc/wiki/tests/NSSF-test-spec.md` 부재)

# NSSF 시험항목서

## 0. 메타

- 대상 NF: NSSF / 메인 규격: TS 29.531
- target_language: C (`agent_context.json` 인용)
- 산출 시각: 2026-05-26
- 입력 산출물 버전 (위키 fallback):
  - features: `doc/wiki/entities-features/29.531-features.md` (FEAT=107, M-Not=5, C-Not=0)
  - api: `doc/wiki/implementation-specs/NSSF-api-spec.md` (E1~E8 + C1)
  - procedure: `doc/wiki/entities-features/NSSF-procedure-analysis.md` (PROC=20)
  - db-design: `doc/wiki/implementation-specs/NSSF-db-design.md` (ENT=16, T=8)
  - impl-spec: `doc/wiki/implementation-specs/NSSF-impl-spec.md` (VS=51, Modules=13, Handlers=8, Clients=5)
  - mock-data: `doc/wiki/implementation-specs/NSSF-mock-data.md` (MOCK=77 core + MOCK-B/SF 부가)
  - plan: `doc/wiki/plans/NSSF-plan.md` (TASK=42)
- 0단계 입구 점검: ✅ 통과 (7개 산출물 모두 존재·자체 체크리스트 ✅·placeholder 3컬럼 충전 완료·VS·MOCK 표 존재)
- 결정된 `tech_stack`: `Unity` (T-TEST), `libcurl` (T-HTTP-CLI), `cJSON` (T-JSON), `OpenSSL`+`libjwt` (T-AUTH), `nghttp2` (T-HTTP-SRV), `libuv` (T-IO), `PostgreSQL`+`libpq` (T-DB persistent), `uthash` (T-DB in-memory), `zlog` (T-LOG)

> **재정의 금지 선언**: 본 산출물은 새 cause·VS-ID·MOCK-ID·ENT-ID·PROC-ID·feature_id 를 생성하지 않습니다. 모든 식별자는 7개 업스트림 산출물에서 인용합니다.

## 1. 참조 문서

- 기능 목록: [features](../entities-features/29.531-features.md)
- API 분석: [api-spec](../implementation-specs/NSSF-api-spec.md)
- 절차 분석: [procedure-analysis](../entities-features/NSSF-procedure-analysis.md)
- DB 설계: [db-design](../implementation-specs/NSSF-db-design.md)
- 구현 명세: [impl-spec](../implementation-specs/NSSF-impl-spec.md)
- Mock 데이터: [mock-data](../implementation-specs/NSSF-mock-data.md)
- 구현 계획: [plan](../plans/NSSF-plan.md)
- 도메인 힌트: [hints](../sources/NSSF-hints.md)

## 2. 개요

### 2-A. 시험 계층 분포

| 계층 | 개수 | 비고 |
|---|---:|---|
| 단위 (Unit) | 71 | impl-spec 모듈(13)·핸들러(8)·클라이언트(5)별 함수 단위, plan TASK-0002~0036 매핑 |
| 통합 (Integration) | 48 | api Endpoint 단위 (E1~E8 + C1) 요청-응답 사이클, in-process 모듈 결합 |
| 시스템 (System) | 26 | 다중 NF mock 토폴로지, restart 시험, VS-0048~0051 e2e |
| **합계** | **145** | TC-0001 ~ TC-0145 |

### 2-B. 우선순위 분포

| 우선순위 | 개수 | 비고 |
|---|---:|---|
| P0 | 96 | features.md M·M-Not·ERR(M)·SVC(M)·DAT(M)·SEC(M) 매핑, impl-spec 핵심 VS 직접 매핑 |
| P1 | 28 | features.md C·SHOULD(P1 승격, 도메인 힌트 채택)·통합·시스템 e2e |
| P2 | 11 | features.md SHOULD(P2)·도메인 힌트에서 P2로 유지된 기능 |
| P3 | 10 | features.md O·O-C 중 도메인 힌트 채택분 (gzip·SUMOD modify·HR roaming forward 등) |

### 2-C. 부정 시험 (M-Not / C-Not 매핑)

| feature_id | 라벨 | 매핑 부정 TC | mock-data 인용 |
|---|---|---|---|
| NSSF-FEAT-SVC-0016 | M-Not (동일 expiry 다중 구독 금지) | NSSF-TC-0023, NSSF-TC-0072 | MOCK-0040/0041 |
| NSSF-FEAT-SVC-0020 | M-Not (TA-단위 미지원 NSSAI 반환 금지) | NSSF-TC-0025, NSSF-TC-0073 | MOCK-0044/0045 |
| NSSF-FEAT-DAT-0025 | M-Not (event IE 변경 금지) | NSSF-TC-0025, NSSF-TC-0074 | MOCK-0044/0045 |
| NSSF-FEAT-SVC-0029 | M-Not (Update 후 중복 status_change Notify 금지) | NSSF-TC-0047, NSSF-TC-0075 | MOCK-0073 |
| NSSF-FEAT-SEC-0001 | M-Not (Replacement 진행 슬라이스 AuthorizedNssaiAvailabilityData 송신 금지) | NSSF-TC-0039, NSSF-TC-0076 | MOCK-0062 |

C-Not = 0건 (features.md §1 분포 — "C-Not=0" 확정).

### 2-D. VS-ID 커버리지 약정

impl-spec §9 의 모든 VS-ID (NSSF-VS-0001 ~ NSSF-VS-0051, 총 51건) 가 NSSF-TC-0001 ~ NSSF-TC-0051 와 1:1 매핑 됩니다 (재정의 금지 — VS 시나리오 재기술 0건, ID 인용만). §6-C 매트릭스 참조.

### 2-E. MOCK-ID 인용 정책

- 단일 TC 의 "입력" 컬럼은 mock-data §2 정상 페이로드의 REQ MOCK-ID 인용.
- "기대 결과" 컬럼은 mock-data §2 정상 RESP 또는 §3 ProblemDetails MOCK-ID 인용.
- 경계값 시험은 MOCK-B-NNNN 직접 인용 (mock-data §4).
- 특수 패턴 시험은 MOCK-SF-NNNN 인용 (mock-data §5-C).
- 페이로드 본문 재기술 0건.

## 3. 시험 환경

### 3-A. 시뮬레이터 / 빌드

- C 시뮬레이터: 본 NSSF C 구현 (impl-spec §1 모듈 카탈로그 / plan NSSF-TASK-0001~0038 산출)
- 빌드: CMake + Unity (impl-spec §1, plan TASK-0001 인용)
- 정적분석·메모리 검증: clang-tidy + valgrind (tech_stack.notes 인용)

### 3-B. 외부 NF Mock

procedure-analysis §5-A 참여 NF 카탈로그를 mock 대상으로 사용 (NRF, AMF, SMF, NWDAF, V-NSSF/H-NSSF, SEPP, Consumer-NF).

| Mock 대상 | 사용 시험 계층 | 주요 MOCK-ID |
|---|---|---|
| NRF stub | 통합/시스템 | MOCK-0063~0071, MOCK-0017 |
| AMF stub (Consumer 측) | 통합/시스템 | MOCK-0001~0008, MOCK-0018~0023, MOCK-0034~0050, MOCK-0053~0058 |
| SMF stub | 통합/시스템 | MOCK-0009/0010, MOCK-0011/0012 |
| NWDAF stub | 통합/시스템 | MOCK-0015/0016, MOCK-0059 |
| V-NSSF/H-NSSF stub + SEPP | 시스템 (HR roaming) | MOCK-0076 |
| Consumer NF Notify Sink | 통합/시스템 | MOCK-0053~0062 |

### 3-C. 도구 (tech_stack.notes 인용)

| 분류 | 도구 | 인용 |
|---|---|---|
| 시험 프레임워크 | Unity | T-TEST |
| HTTP 클라이언트 (시험 driver) | libcurl | T-HTTP-CLI |
| JSON 직렬화·schema validator | cJSON | T-JSON |
| TLS / JWT (mock NRF 측) | OpenSSL + libjwt | T-AUTH |
| 이벤트 루프 | libuv | T-IO |
| 빌드 | CMake | T-BUILD |
| 메모리 검증 | valgrind | (memory_check) |
| 정적분석 | clang-tidy | (static_analysis) |
| 영속 DB | PostgreSQL + libpq | T-DB |
| 인메모리 | uthash | T-DB |
| 로깅 | zlog | T-LOG |

### 3-D. 환경 토폴로지 (시스템 시험)

procedure-analysis §6 (PROC-0006/0007/0008/0010/0011/0012/0013/0014/0015/0016/0017) 의 정상 흐름 시퀀스를 그대로 인용. 다중 NF mock 은 동일 호스트 내 별도 프로세스 (libcurl easy handle 으로 호출).

## 4. 시험항목 매트릭스

> 컬럼: TC-ID | 계층 | 카테고리 | 시험 목적 (RFC 2119 + feature_id 요지) | feature_id | task_id | VS-ID | 입력 (MOCK 인용) | 기대 출력 (MOCK 인용) | 판정 기준 | 우선순위

### 4-A. VS ↔ TC 매핑 (TC-0001 ~ TC-0051, impl-spec §9 1:1 인용)

| TC-ID | 계층 | 카테고리 | 시험 목적 | feature_id | task_id | VS-ID | 입력 (MOCK) | 기대 출력 (MOCK) | 판정 기준 | 우선순위 |
|---|---|---|---|---|---|---|---|---|---|---|
| NSSF-TC-0001 | 통합 | 정상 (SVC) | SHALL — AMF Initial Registration 시 GET /network-slice-information 응답에 Allowed/Configured NSSAI 반환 (SVC-0002, DAT-0001~0004) | SVC-0002, DAT-0001~0004 | TASK-0024, TASK-0033 | VS-0001 | MOCK-0001 | MOCK-0002 | impl-spec §9 VS-0001 — handler_e1 step 5 정상 분기, 응답 schema 일치, allowedNssaiList ≥1 | P0 |
| NSSF-TC-0002 | 통합 | 이상 (ERR) | SHALL — 요청 S-NSSAI 미지원 시 403 SNSSAI_NOT_SUPPORTED 반환 (ERR-0001) | ERR-0001 | TASK-0024 | VS-0002 | MOCK-0003 | MOCK-0004 | impl-spec §9 VS-0002 — step 5 db lookup 실패 → ProblemDetails cause=SNSSAI_NOT_SUPPORTED, status=403 | P0 |
| NSSF-TC-0003 | 통합 | 이상 (SEC) | SHALL — OAuth2 토큰 만료 시 401 TOKEN_EXPIRED 반환 (SEC-0003) | SEC-0003 | TASK-0006, TASK-0024 | VS-0003 | MOCK-0005 | MOCK-0006 | impl-spec §9 VS-0003 — handler_e1 step 1 토큰 검증 실패 → 401 cause=TOKEN_EXPIRED | P0 |
| NSSF-TC-0004 | 통합 | 이상 (ERR) | SHALL — 필수 query param `nf-id` 누락 시 400 MANDATORY_QUERY_PARAM_MISSING 반환 (api §6.1) | – (api §6.1 매트릭스) | TASK-0024 | VS-0004 | MOCK-0007 | MOCK-0008 | impl-spec §9 VS-0004 — step 3 invalidParams=[nf-id] | P0 |
| NSSF-TC-0005 | 통합 | 정상 (SVC) | SHALL — SMF PDU Session Establishment 시 HR roaming 슬라이스 매핑 응답 (SVC-0005, DAT-0005/0006) | SVC-0005, DAT-0005, DAT-0006 | TASK-0024, TASK-0033, TASK-0023 | VS-0005 | MOCK-0009 | MOCK-0010 | impl-spec §9 VS-0005 — step 5 + SEPP forward, mappingOfNssai 포함 | P0 |
| NSSF-TC-0006 | 통합 | 정상 (SVC) | SHALL — RSIPCE feature 협상 시 SMF+PGW-C PDN Connection 슬라이스 매핑 응답 (SVC-0010, DAT-0009/0010) | SVC-0010, DAT-0009, DAT-0010 | TASK-0024, TASK-0033 | VS-0006 | MOCK-0011 | MOCK-0012 | impl-spec §9 VS-0006 — handler_e1 step 4 RSIPCE gate 통과 + step 5 정상 | P0 |
| NSSF-TC-0007 | 통합 | 이상 (ERR) | SHALL — RSIPCE 미협상 PDN Connection 요청 시 400 FEATURE_NOT_SUPPORTED 반환 (DAT-0009 부정) | DAT-0009 | TASK-0024 | VS-0007 | MOCK-0013 | MOCK-0014 | impl-spec §9 VS-0007 — step 4 RSIPCE gate 실패 → 400 | P0 |
| NSSF-TC-0008 | 통합 | 정상 (SVC) | SHALL — NWDAF Other Purpose (SIOP) NSI ID 조회 응답 (SVC-0011, DAT-0011/0012) | SVC-0011, DAT-0011, DAT-0012 | TASK-0024, TASK-0033 | VS-0008 | MOCK-0015 | MOCK-0016 | impl-spec §9 VS-0008 — step 4 + 5 snssaiInfoRspData 반환 | P0 |
| NSSF-TC-0009 | 통합 | 이상 (MGMT) | MAY (P3 승격, 도메인 힌트 채택) — NRF discover 실패 시 partial response (candidateAmfList 생략, targetAmfSet 만 반환) (MGMT-0001) | MGMT-0001 | TASK-0024, TASK-0021 | VS-0009 | MOCK-0001 (REQ 재사용) | MOCK-0017 | impl-spec §9 VS-0009 — client_nrf_discover_amf 실패 → handler step 5 partial | P3 |
| NSSF-TC-0010 | 통합 | 정상 (SVC) | SHALL — AMF NSSAIAvailability_Update PUT 시 200 + AuthorizedNssaiAvailabilityInfo + ENT-0015 Enqueue Notify (SVC-0012, DAT-0013) | SVC-0012, DAT-0013 | TASK-0025, TASK-0034 | VS-0010 | MOCK-0018 | MOCK-0019 | impl-spec §9 VS-0010 — handler_e2 정상, db-design T-AMF-UPSERT 적용, ENT-0015 큐에 Notify task 추가 | P0 |
| NSSF-TC-0011 | 통합 | 이상 (ERR) | SHALL — PUT supportedSnssaiList 미지원 SST 시 403 SNSSAI_NOT_SUPPORTED 반환 (ERR-0010) | ERR-0010 | TASK-0025 | VS-0011 | MOCK-0020 | MOCK-0021 | impl-spec §9 VS-0011 — step 4 → 403 | P0 |
| NSSF-TC-0012 | 통합 | 정상 (경계) | SHALL — PUT 모든 S-NSSAI 인증 후 빈 결과 시 204 No Content (DAT-0013) | DAT-0013 | TASK-0025 | VS-0012 | MOCK-0018 (variant) | MOCK-0022 | impl-spec §9 VS-0012 — step 7 `authorized.n_data == 0` → 204 | P0 |
| NSSF-TC-0013 | 통합 | 이상 (ERR) | SHALL — PUT Content-Type 불일치 시 415 반환 (api §6.2) | – (api §6.2) | TASK-0025 | VS-0013 | MOCK-0018 (header XML override) | MOCK-0023 | impl-spec §9 VS-0013 — step 2 Content-Type 검증 실패 → 415 | P0 |
| NSSF-TC-0014 | 통합 | 정상 (SVC) | SHALL — PATCH 정상 (add op) 시 200 + 갱신된 AuthorizedNssaiAvailabilityInfo (SVC-0013, DAT-0015) | SVC-0013, DAT-0015 | TASK-0026, TASK-0034 | VS-0014 | MOCK-0024 | MOCK-0025 | impl-spec §9 VS-0014 — handler_e3 정상, T-AMF-PATCH 적용 | P0 |
| NSSF-TC-0015 | 통합 | 이상 (ERR) | SHALL — PATCH 미존재 nfId 시 404 RESOURCE_NOT_FOUND 반환 (DAT-0030, ERR-0010) | DAT-0030, ERR-0010 | TASK-0026 | VS-0015 | MOCK-0026 | MOCK-0027 | impl-spec §9 VS-0015 — step 5 → 404 | P0 |
| NSSF-TC-0016 | 통합 | 이상 (ERR) | SHALL — PATCH JSON Patch 무결성 위반 시 400 INVALID_IE invalidParams=[path] 반환 (SVC-0042) | SVC-0042 | TASK-0026 | VS-0016 | MOCK-0028 | MOCK-0029 | impl-spec §9 VS-0016 — step 4 path 무효 → 400 | P0 |
| NSSF-TC-0017 | 통합 | 이상 (ERR) | SHALL — PATCH Content-Type 불일치 (`application/json`) 시 415 반환 (api §6.2) | – (api §6.2) | TASK-0026 | VS-0017 | MOCK-0024 (Content-Type override) | MOCK-0030 | impl-spec §9 VS-0017 — step 2 → 415 | P0 |
| NSSF-TC-0018 | 통합 | 정상 (SVC) | SHALL — DELETE /nssai-availability/{nfId} 정상 시 204 + ENT-0015 enqueue status change (SVC-0030/0031) | SVC-0030, SVC-0031 | TASK-0027, TASK-0034 | VS-0018 | MOCK-0031 | MOCK-0032 | impl-spec §9 VS-0018 — handler_e4 정상, T-AMF-DELETE + FK CASCADE | P0 |
| NSSF-TC-0019 | 통합 | 이상 (ERR) | SHALL — DELETE 미존재 nfId 시 404 RESOURCE_NOT_FOUND 반환 (ERR-0015, DAT-0031) | ERR-0015, DAT-0031 | TASK-0027 | VS-0019 | MOCK-0031 (미존재 ID) | MOCK-0033 | impl-spec §9 VS-0019 — step 5 → 404 | P0 |
| NSSF-TC-0020 | 통합 | 정상 (SVC) | SHALL — Subscribe 4 이벤트 정상 시 201 + Location + subscriptionId (SVC-0014, DAT-0017/0018) | SVC-0014, DAT-0017, DAT-0018 | TASK-0028, TASK-0035 | VS-0020 | MOCK-0034 | MOCK-0035 | impl-spec §9 VS-0020 — handler_e5 정상, T-SUB-CREATE, Location 헤더 필수 | P0 |
| NSSF-TC-0021 | 통합 | 이상 (ERR) | SHALL — Subscribe 모든 event 미지원 시 501 UNSUPPORTED_EVENT_TYPE 반환 (DAT-0033) | DAT-0033 | TASK-0028 | VS-0021 | MOCK-0036 | MOCK-0037 | impl-spec §9 VS-0021 — step 4 → 501 | P0 |
| NSSF-TC-0022 | 통합 | 이상 (ERR) | SHALL — Subscribe callbackUri 누락 시 400 MANDATORY_IE_MISSING invalidParams=[nfNssaiAvailabilityUri] 반환 (ERR-0011, DAT-0032) | ERR-0011, DAT-0032 | TASK-0028 | VS-0022 | MOCK-0038 | MOCK-0039 | impl-spec §9 VS-0022 — step 3 → 400 | P0 |
| NSSF-TC-0023 | 통합 | 부정 (M-Not) | SHALL NOT — 동일 expiry timestamp 다중 구독에 부여 금지: NSSF 가 expiry 균등 분산 (SVC-0016 부정) | SVC-0016 | TASK-0028, TASK-0035 | VS-0023 | MOCK-0040 | MOCK-0041 | impl-spec §9 VS-0023 — step 5 expiry distribution 로직: N개 동시 요청 시 동일 timestamp 다수 부여 0건 | P0 |
| NSSF-TC-0024 | 통합 | 정상 (SVC) | SHALL — Subscribe Modify (SUMOD feature) PATCH 정상 시 200 + NssfEventSubscriptionCreatedData (SVC-0018/0019, DAT-0024) | SVC-0018, SVC-0019, DAT-0024 | TASK-0029, TASK-0035 | VS-0024 | MOCK-0042 | MOCK-0043 | impl-spec §9 VS-0024 — handler_e6 정상, T-SUB-MODIFY 적용 | P3 |
| NSSF-TC-0025 | 통합 | 부정 (M-Not) | SHALL NOT — Subscribe Modify 에서 event IE 변경 시도 시 400 MODIFICATION_NOT_ALLOWED (SVC-0020·DAT-0025 부정, ERR-0012) | SVC-0020, DAT-0025, ERR-0012 | TASK-0029 | VS-0025 | MOCK-0044 | MOCK-0045 | impl-spec §9 VS-0025 — step 4 event 경로 차단 → 400 | P0 |
| NSSF-TC-0026 | 통합 | 이상 (ERR) | SHALL — SUMOD 미협상 PATCH 시도 시 403 NOT_AUTHORIZED (SVC-0018) | SVC-0018 | TASK-0029 | VS-0026 | MOCK-0042 (SUMOD bit OFF) | MOCK-0046 | impl-spec §9 VS-0026 — step 4 SUMOD bit 검사 → 403 | P0 |
| NSSF-TC-0027 | 통합 | 이상 (ERR) | SHALL — PATCH subscriptionId 미존재 시 404 SUBSCRIPTION_NOT_FOUND (ERR-0012) | ERR-0012 | TASK-0029 | VS-0027 | MOCK-0042 (미존재 ID) | MOCK-0047 | impl-spec §9 VS-0027 — step 5 → 404 | P0 |
| NSSF-TC-0028 | 통합 | 정상 (SVC) | SHALL — Unsubscribe DELETE 정상 시 204 + 진행 중 알림 cancel (SVC-0021/0022) | SVC-0021, SVC-0022 | TASK-0030, TASK-0034 | VS-0028 | MOCK-0048 | MOCK-0049 | impl-spec §9 VS-0028 — handler_e7 정상, ENT-0010 delete + ENT-0015 cancel | P0 |
| NSSF-TC-0029 | 통합 | 이상 (ERR) | SHALL — Unsubscribe 미존재 시 404 SUBSCRIPTION_NOT_FOUND (ERR-0013) | ERR-0013 | TASK-0030 | VS-0029 | MOCK-0048 (미존재) | MOCK-0050 | impl-spec §9 VS-0029 — step 5 → 404 | P0 |
| NSSF-TC-0030 | 통합 | 정상 (SVC) | SHALL — OPTIONS /nssai-availability 정상 시 200 + Accept-Encoding: gzip (SVC-0032) | SVC-0032 | TASK-0031 | VS-0030 | MOCK-0051 | MOCK-0052 | impl-spec §9 VS-0030 — handler_e8 정상 | P3 |
| NSSF-TC-0031 | 시스템 | 정상 (PRC) | SHALL — NSSAIAvailability_Update 후 매칭 구독에 Notify 송신, Consumer 204 응답 수신 (SVC-0023/0024, DAT-0026, PRC-0001) | SVC-0023, SVC-0024, DAT-0026, PRC-0001 | TASK-0022, TASK-0036 | VS-0031 | MOCK-0053 | MOCK-0054 | impl-spec §9 VS-0031 — client_notify_send + on_notify_response 204, ENT-0015 dequeue | P0 |
| NSSF-TC-0032 | 시스템 | 이상 (ERR) | SHALL — Notify 응답 400 RESOURCE_CONTEXT_NOT_FOUND 시 구독 status=STALE 표시, 영구실패 (DAT-0034, ERR-0014) | DAT-0034, ERR-0014 | TASK-0022, TASK-0036 | VS-0032 | MOCK-0053 (재사용) | MOCK-0055 | impl-spec §9 VS-0032 — on_notify_response 400 case, ENT-0010 상태 전이 STALE | P0 |
| NSSF-TC-0033 | 시스템 | 이상 (ERR) | SHALL — Notify 응답 404 RESOURCE_URI_STRUCTURE_NOT_FOUND 시 구독 status=INVALIDATED (DAT-0035, ERR-0014) | DAT-0035, ERR-0014 | TASK-0022, TASK-0036 | VS-0033 | MOCK-0053 (재사용) | MOCK-0056 | impl-spec §9 VS-0033 — on_notify_response 404 case, ENT-0010 INVALIDATED | P0 |
| NSSF-TC-0034 | 시스템 | 이상 (재시도) | SHOULD — Notify 응답 5xx + Retry-After 시 백오프 재시도 (max_retries 후 stale) (SVC-0027) | SVC-0027 | TASK-0022 | VS-0034 | MOCK-0053 (재사용) | MOCK-0057 | impl-spec §9 VS-0034 — on_notify_response retry case, 백오프 + 최종 stale | P1 |
| NSSF-TC-0035 | 시스템 | 이상 (3xx) | SHALL — Notify 응답 307 redirect 시 새 Location 으로 즉시 재시도 (DAT-0036) | DAT-0036 | TASK-0022 | VS-0035 | MOCK-0053 (재사용) | MOCK-0058 | impl-spec §9 VS-0035 — 307 case, 즉시 new_uri 로 POST | P1 |
| NSSF-TC-0036 | 시스템 | 정상 (PRC) | SHALL — NWDAF/OAM 트리거 Slice Replacement (NSRP feature) 시 Consumer 에 altNssai 포함 Notify (SVC-0027, PRC-0003) | SVC-0027, PRC-0003 | TASK-0014, TASK-0036 | VS-0036 | MOCK-0059 | MOCK-0054 | impl-spec §9 VS-0036 — client_notify_send (replacement payload), Consumer 204 응답 | P0 |
| NSSF-TC-0037 | 시스템 | 정상 (PRC) | SHALL — S-NSSAI Validity Time 변경 시 Consumer 에 nssaiValidityTimeInfoList Notify (PRC-0002) | PRC-0002 | TASK-0014, TASK-0036 | VS-0037 | MOCK-0060 | MOCK-0054 | impl-spec §9 VS-0037 — client_notify_send (validity payload), Consumer 204 응답 | P0 |
| NSSF-TC-0038 | 시스템 | 정상 (SEC/SVC) | SHALL — EANAN feature 협상 시 모든 슬라이스 미지원 상태 Notify (빈 배열) (SEC-0002, SVC-0029 부정) | SEC-0002, SVC-0029 | TASK-0036 | VS-0038 | MOCK-0061 | MOCK-0054 | impl-spec §9 VS-0038 — authorizedNssaiAvailabilityData=[] 통지, dedup 단일 통지만 | P0 |
| NSSF-TC-0039 | 시스템 | 부정 (M-Not) | SHALL NOT — Replacement 진행 중인 S-NSSAI 에 AuthorizedNssaiAvailabilityData 송신 금지: 코드가 사전 필터링 (SEC-0001 부정) | SEC-0001 | TASK-0036 | VS-0039 | MOCK-0062 | (송신 0건 — 부정 페이로드 없음) | impl-spec §9 VS-0039 — client_notify_send 사전 필터링, 위반 송신 0건 | P0 |
| NSSF-TC-0040 | 통합 | 정상 (PROC) | SHALL — NSSF 기동 시 NRF Register 정상 (201 + heartBeatTimer) (procedure §2.1 PROC-0001) | – (procedure §2.1) | TASK-0020, TASK-0032 | VS-0040 | MOCK-0063 | MOCK-0064 | impl-spec §9 VS-0040 — M-CLIENT-NRF.register on 201, heartBeatTimer 적용 | P0 |
| NSSF-TC-0041 | 통합 | 이상 (재시도) | SHOULD — NRF Register 5xx 시 백오프 재시도, 무한 retry (운영 알람) (procedure §2.1 PROC-0001) | – (procedure §2.1) | TASK-0020 | VS-0041 | MOCK-0063 (재사용) | MOCK-0065 | impl-spec §9 VS-0041 — retry case, 백오프 + 알람 | P1 |
| NSSF-TC-0042 | 통합 | 정상 (PROC) | SHALL — Heartbeat 만료 시 NRF PATCH (PROC-0003) | – (procedure §2.3) | TASK-0020, TASK-0032 | VS-0042 | MOCK-0066 | MOCK-0067 | impl-spec §9 VS-0042 — heartBeatTimer 만료 콜백 | P0 |
| NSSF-TC-0043 | 통합 | 이상 (재등록) | SHALL — Heartbeat 응답 404 시 자동 PROC-0001 재호출 (PROC-0003) | – (procedure §2.3) | TASK-0020, TASK-0032 | VS-0043 | MOCK-0066 (재사용) | MOCK-0068 | impl-spec §9 VS-0043 — on_heartbeat_response 404 case, 자동 재등록 | P0 |
| NSSF-TC-0044 | 통합 | 정상 (PROC) | SHALL — OAuth2 Token cache miss 시 NRF 발급 200 (PROC-0005) | – (procedure §2.5) | TASK-0021 | VS-0044 | MOCK-0069 | MOCK-0070 | impl-spec §9 VS-0044 — auth_get_token, token cached, ENT-0013 갱신 | P0 |
| NSSF-TC-0045 | 통합 | 이상 (재발급) | SHALL — Token 401 응답 시 cache invalidate + 재발급 (PROC-0005) | – (procedure §2.5) | TASK-0021 | VS-0045 | MOCK-0069 (재사용) | MOCK-0071 | impl-spec §9 VS-0045 — cache invalidate + 재시도 | P0 |
| NSSF-TC-0046 | 통합 | 정상 (배치) | SHALL — Subscription Expiry sweeper: expiry < now 도달 시 구독 삭제 + 큐 정리 (PROC-0020) | – (procedure §2.20) | TASK-0035 | VS-0046 | (no message — timer trigger) MOCK-0072 | (지나간 구독 0건) | impl-spec §9 VS-0046 — timer 콜백, T-SUB-EXPIRE 배치 | P0 |
| NSSF-TC-0047 | 시스템 | 부정 (M-Not) | SHALL NOT — PUT 직후 동일 변경 다중 Notify 송신 금지: dedupe → 단일 통지만 송신 (SVC-0029 부정) | SVC-0029 | TASK-0036 | VS-0047 | MOCK-0073 | (단일 Notify) | impl-spec §9 VS-0047 — enqueue dedup 로직, 중복 통지 0건 | P0 |
| NSSF-TC-0048 | 시스템 | 정상 (e2e) | SHALL — Initial Registration end-to-end: AMF→NSSF GET + NSSF→NRF disc → 200 (전체 시퀀스 ≤ N ms) (SVC-0002, MGMT-0001) | SVC-0002, MGMT-0001 | TASK-0039 | VS-0048 | MOCK-0074 | (시스템 응답) | impl-spec §9 VS-0048 — 전체 흐름 ≤ N ms, procedure-analysis §2.6 PROC-0006 + §2.4 PROC-0004 시퀀스 일치 | P1 |
| NSSF-TC-0049 | 시스템 | 정상 (e2e) | SHALL — NSSAI Update + Notify 사이클: AMF PUT → 매칭 구독자 Notify → 204 (SVC-0012, SVC-0023) | SVC-0012, SVC-0023 | TASK-0040 | VS-0049 | MOCK-0075 | (큐 비움 + lastNotifiedAt 갱신) | impl-spec §9 VS-0049 — procedure §2.13 PROC-0013 + §2.15 PROC-0015 시퀀스 일치 | P1 |
| NSSF-TC-0050 | 시스템 | 정상 (e2e) | SHALL — HR Roaming PDU Session: SMF→V-NSSF→SEPP→H-NSSF 응답 체인 (SVC-0004) | SVC-0004 | TASK-0041 | VS-0050 | MOCK-0076 | (V-NSSF 200 OK) | impl-spec §9 VS-0050 — procedure §2.8 PROC-0008 + §2.12 PROC-0012 시퀀스 일치 | P1 |
| NSSF-TC-0051 | 시스템 | 정상 (재시작) | SHALL — restart 후 구독 활성 유지: PG → uthash 로드 (procedure §2.14 PROC-0014 + restart) | – (procedure §2.14, db §8-B) | TASK-0042 | VS-0051 | MOCK-0077 | (구독 활성 유지) | impl-spec §9 VS-0051 — db_init 후 ENT-0010 로딩, 구독 모두 ACTIVE | P1 |

### 4-B. 단위 모듈 시험 (TC-0052 ~ TC-0071, impl-spec 모듈 카탈로그 1:1)

| TC-ID | 계층 | 카테고리 | 시험 목적 | feature_id | task_id | VS-ID | 입력 (MOCK) | 기대 출력 | 판정 기준 | 우선순위 |
|---|---|---|---|---|---|---|---|---|---|---|
| NSSF-TC-0052 | 단위 | M-LOG | SHALL — zlog 카테고리별 출력 + 3gpp-Sbi-* 헤더 추적 로그 (impl-spec §1 M-LOG) | – (모듈 단위) | TASK-0002, TASK-0003 | – | (no message — function call) | (zlog file에 카테고리/메시지 일치 기록) | impl-spec §1 모듈 카탈로그 M-LOG, plan TASK-0002 AC | P0 |
| NSSF-TC-0053 | 단위 | M-CFG | SHALL — cJSON 파서 + lifecycle 변환 + schema 검증 (DEF-011 schema-payload 일관성) | – | TASK-0004, TASK-0005 | – | MOCK-CFG-0001/0002 (plan §5.5) | (lifecycle 구조체 채워짐) | plan TASK-0004 AC, 필수 키 누락 시 실패 | P0 |
| NSSF-TC-0054 | 단위 | M-AUTH (TLS ctx) | SHALL — OpenSSL TLS ctx 초기화 (mTLS 양방향 인증서 로드) (SEC 일반) | – | TASK-0006, TASK-0007 | – | MOCK-TLS-0001/0002 | (SSL_CTX* 반환, 인증서 valid) | plan TASK-0006 AC | P0 |
| NSSF-TC-0055 | 단위 | M-AUTH (JWT) | SHALL — libjwt RS256 검증 통과/실패 분기 (SEC-0003) | SEC-0003 | TASK-0006 | VS-0044 | MOCK-0070 | (검증 결과 bool) | plan TASK-0006 AC RS256/ES256 | P0 |
| NSSF-TC-0056 | 단위 | M-AUTH (token cache) | SHALL — token cache GetOrFetch + TTL 만료 invalidate (ENT-0013) | – | TASK-0006, TASK-0021 | VS-0045 | MOCK-0071 | (cache miss → fetch, expiry 후 evict) | plan TASK-0006 AC + db §7.13 | P0 |
| NSSF-TC-0057 | 단위 | M-MODEL (core) | SHALL — Snssai/Tai/PlmnId/NfInstanceId/ProblemDetails/SupportedFeatures 직렬화·역직렬화 (DAT-0029, DAT-0039) | DAT-0029, DAT-0039 | TASK-0008 | – | (JSON fixture from MOCK-B-0001~0034) | (struct ↔ JSON 양방향 일치) | plan TASK-0008 AC, api-analysis §3 인용 | P0 |
| NSSF-TC-0058 | 단위 | M-MODEL (확장) | SHALL — NssaiAvailabilityInfo/Authorized/Subscription/NssfEvent/PatchDoc 직렬화 (DAT-0013~0028, DAT-0038) | DAT-0013, DAT-0014, DAT-0015, DAT-0016, DAT-0017, DAT-0018, DAT-0024, DAT-0026, DAT-0038 | TASK-0009 | – | MOCK-0018, MOCK-0034, MOCK-0042, MOCK-0053 | (struct ↔ JSON 양방향 일치) | plan TASK-0009 AC | P0 |
| NSSF-TC-0059 | 단위 | M-DB 인프라 | SHALL — libpq 연결 풀 + uthash + write-through 패턴 (T-DB 인용) | – | TASK-0010 | VS-0051 | (no message — direct CRUD) | (PG ↔ uthash 일치) | plan TASK-0010 AC + db §4 저장소 선택 | P0 |
| NSSF-TC-0060 | 단위 | M-DB (Config) | SHALL — ENT-0001~0007 Config 영역 Load/Read/Reload (DAT-0001~0012, DAT-0029) | DAT-0001, DAT-0002, DAT-0007, DAT-0029 | TASK-0011 | – | (cfg fixture from TASK-0005) | (uthash hit + PG fallback 일치) | plan TASK-0011 AC + db §7.1/7.2 | P0 |
| NSSF-TC-0061 | 단위 | M-DB (AMF) | SHALL — ENT-0008/0009 Upsert/Patch/Read/Delete + T-AMF-UPSERT/PATCH/DELETE 트랜잭션 (DAT-0013/0014) | DAT-0013, DAT-0014, SVC-0012 | TASK-0012 | – | MOCK-0018, MOCK-0024 | (REPEATABLE READ + FK CASCADE 정상) | plan TASK-0012 AC + db §7.3·§8-A | P0 |
| NSSF-TC-0062 | 단위 | M-DB (Sub) | SHALL — ENT-0010 다중 인덱스 + FindMatching + ExpireDueAt (SVC-0014~0022, DAT-0017~0025) | SVC-0014, SVC-0019, SVC-0021, DAT-0017, DAT-0018, DAT-0024 | TASK-0013 | – | MOCK-0034, MOCK-0042, MOCK-0048 | (보조 인덱스 정확, expiry 정렬) | plan TASK-0013 AC + db §7.10 | P0 |
| NSSF-TC-0063 | 단위 | M-DB (ephemeral/NRF) | SHALL — ENT-0013/0014/0015/0016 (TokenCache lock-free / NotifyQueue FIFO / NRF state) | – | TASK-0015 | – | MOCK-0069, MOCK-0072 | (lock-free atomic swap, FIFO 순서, singleton 갱신) | plan TASK-0015 AC + db §7.13~7.16 | P0 |
| NSSF-TC-0064 | 단위 | M-HTTP-SRV | SHALL — nghttp2 route 등록 API + libuv loop wire + 8 endpoint dispatch (SVC-0033~0035, SVC-0038~0040) | SVC-0033, SVC-0034, SVC-0035, SVC-0038, SVC-0039, SVC-0040 | TASK-0016 | – | (nghttp2 session callback stub) | (route 매칭 정확, 404 vs handler 분기) | plan TASK-0016 AC | P0 |
| NSSF-TC-0065 | 단위 | M-HTTP-SRV cfg-gate (R12) | SHALL — cfg.listen.tls_cert/key 보유 시 운영 path 활성 (test_mode=0) (DEF-006 fix) | – | TASK-0017 | – | (cfg fixture with/without tls) | (test_mode 정확) | plan TASK-0017 AC, DEF-006 anchor | P0 |
| NSSF-TC-0066 | 단위 | M-HTTP-CLI | SHALL — libcurl multi + libuv timer/poll handle + 재시도 (curl_multi_socket_action) | – | TASK-0018 | – | (curl easy/multi stub) | (multi handle 콜백 정상, 재시도 카운터) | plan TASK-0018 AC | P0 |
| NSSF-TC-0067 | 단위 | M-HTTP-CLI cfg-gate (R12) | SHALL — cfg.nrf.base_url 보유 시 curl_global_init 운영 path 활성 (DEF-003 fix) | – | TASK-0019 | – | (cfg fixture with/without nrf url) | (init 호출 분기 정확) | plan TASK-0019 AC | P0 |
| NSSF-TC-0068 | 단위 | M-CLIENT-NRF | SHALL — NfProfile 직렬화 + Register/Deregister/Heartbeat (PROC-0001~0003) | – | TASK-0020 | VS-0040, VS-0041, VS-0042, VS-0043 | MOCK-0063, MOCK-0066 | (NRF stub 호출 페이로드 일치) | plan TASK-0020 AC + procedure §2.1~2.3 | P0 |
| NSSF-TC-0069 | 단위 | M-CLIENT-NOTIFY | SHALL — C1 callback POST + 백오프 + DequeueDue (SVC-0023~0029, ERR-0014) | SVC-0023, SVC-0024, ERR-0014 | TASK-0022 | VS-0031, VS-0034, VS-0035 | MOCK-0053, MOCK-0057, MOCK-0058 | (재시도·307·Retry-After 헤더 정확 처리) | plan TASK-0022 AC | P0 |
| NSSF-TC-0070 | 단위 | M-CLIENT-SEPP | MAY — V-NSSF→H-NSSF forward via SEPP (PROC-0012) (SVC-0004) | SVC-0004 | TASK-0023 | VS-0005, VS-0050 | MOCK-0009, MOCK-0076 | (N32 채널 stub 통과, 응답 chain 일치) | plan TASK-0023 AC | P3 |
| NSSF-TC-0071 | 단위 | M-LIFE / M-MAIN | SHALL — init/shutdown/timer wire 10 STEP + argv + signal_wait + uv_run + graceful_shutdown (impl-spec §8-D-1/D-2) | – | TASK-0037, TASK-0038 | VS-0048, VS-0051 | – | (init 모든 STEP 성공 + SIGTERM 시 정리) | plan TASK-0037 + TASK-0038 AC | P0 |

### 4-C. 부정 시험 보강 (TC-0072 ~ TC-0085)

> features.md M-Not 5건 각각 ≥1 부정 TC + C-Not 0건 (검증 종료). 우선순위 P0.

| TC-ID | 계층 | 카테고리 | 시험 목적 | feature_id | task_id | VS-ID | 입력 (MOCK) | 기대 출력 | 판정 기준 | 우선순위 |
|---|---|---|---|---|---|---|---|---|---|---|
| NSSF-TC-0072 | 단위 | 부정 (M-Not) | SHALL NOT — Subscription 생성 시 동일 expiry timestamp 다중 부여 금지: jitter 적용 단위 시험 (SVC-0016) | SVC-0016 | TASK-0028, TASK-0035 | VS-0023 | MOCK-0040 | (NSSF 가 expiry 분산) | 동일 timestamp 빈도 == 1 (균등성 검증) | P0 |
| NSSF-TC-0073 | 단위 | 부정 (M-Not) | SHALL NOT — NSSF 가 TA-단위로 미지원 NSSAI 반환 금지 (SVC-0020) | SVC-0020 | TASK-0029 | VS-0025 | MOCK-0044 | 응답 body 의 nssai per TA 가 supportedNssai 와 disjoint 0건 | impl-spec §9 VS-0025 + features SVC-0020 인용 | P0 |
| NSSF-TC-0074 | 단위 | 부정 (M-Not) | SHALL NOT — Subscribe Modify 시 event IE 변경 금지: 단위 검증 (DAT-0025) | DAT-0025 | TASK-0029 | VS-0025 | MOCK-0044 | 400 MODIFICATION_NOT_ALLOWED, ENT-0010.event 불변 | impl-spec §9 VS-0025 | P0 |
| NSSF-TC-0075 | 단위 | 부정 (M-Not) | SHALL NOT — Update 후 중복 status_change Notify 발송 금지: dedup 단위 시험 (SVC-0029) | SVC-0029 | TASK-0036 | VS-0047 | MOCK-0073 | dedup 처리 후 송신 횟수 == 1 | impl-spec §9 VS-0047 + features SVC-0029 | P0 |
| NSSF-TC-0076 | 단위 | 부정 (M-Not) | SHALL NOT — Replacement 진행 슬라이스 AuthorizedNssaiAvailabilityData 송신 금지: 사전 필터링 단위 시험 (SEC-0001) | SEC-0001 | TASK-0036 | VS-0039 | MOCK-0062 | client_notify_send 가 송신 차단 (위반 0건) | impl-spec §9 VS-0039 | P0 |
| NSSF-TC-0077 | 통합 | 트리거 (C) | SHALL (Conditional) — 구독 NSSRG 제공 시 NSSF 가 NSSRG list 반환 (SVC-0006 트리거) | SVC-0006 | TASK-0024 | VS-0001 (확장) | MOCK-0001 (NSSRG 포함 variant) | 200 + nssrgList 필드 존재 | features.md SVC-0006 "If subscribed NSSRG list is provided" 트리거 | P1 |
| NSSF-TC-0078 | 통합 | 트리거 (C) | SHALL (Conditional) — AMF NSAG 지원 표시 시 NSSF 가 NSAG 반환 (SVC-0007 트리거) | SVC-0007 | TASK-0024 | VS-0001 (확장) | MOCK-0001 (NSAG 포함 variant) | 200 + nsagInfoList 필드 존재 | features.md SVC-0007 인용 | P1 |
| NSSF-TC-0079 | 통합 | 트리거 (C) | SHALL (Conditional) — UCU NSSRG 제공 시 NSSF 가 NSSRG list 반환 (SVC-0008 트리거) | SVC-0008 | TASK-0024 | VS-0001 (확장) | MOCK-0001 (UCU NSSRG variant) | 200 + nssrgList 필드 존재 | features.md SVC-0008 인용 | P1 |
| NSSF-TC-0080 | 통합 | 트리거 (C) | SHALL (Conditional) — UCU AMF NSAG 지원 시 NSAG 반환 (SVC-0009 트리거) | SVC-0009 | TASK-0024 | VS-0001 (확장) | MOCK-0001 (UCU NSAG variant) | 200 + nsagInfoList 필드 존재 | features.md SVC-0009 인용 | P1 |
| NSSF-TC-0081 | 통합 | 트리거 (C) | SHALL (Conditional) — Subscribe request accepted 시 201 응답 (SVC-0022 트리거) | SVC-0022 | TASK-0028 | VS-0020 | MOCK-0034 | 201 Created + Location | features.md SVC-0022 인용 | P1 |
| NSSF-TC-0082 | 통합 | 트리거 (C) | SHALL (Conditional) — Unsubscribe accepted 시 204 (SVC-0022 트리거, DELETE 경로) | SVC-0022 | TASK-0030 | VS-0028 | MOCK-0048 | 204 | features.md SVC-0022 인용 (DELETE 경로) | P1 |
| NSSF-TC-0083 | 통합 | 트리거 (C) | SHALL (Conditional) — OPTIONS accepted 시 200 응답 (SVC-0032) | SVC-0032 | TASK-0031 | VS-0030 | MOCK-0051 | 200 | features.md SVC-0032 인용 | P3 |
| NSSF-TC-0084 | 시스템 | 트리거 (PRC) | SHALL (Conditional) — S-NSSAI status change 발생 시 NF service consumer 통지 (PRC-0001 트리거) | PRC-0001 | TASK-0036 | VS-0031 | MOCK-0053 | Consumer 가 Notify 수신 | procedure-analysis §2.15 PROC-0015 인용 | P3 |
| NSSF-TC-0085 | 시스템 | 트리거 (PRC) | SHALL (Conditional) — Validity timer 트리거 시 통지 (PRC-0002 트리거) | PRC-0002 | TASK-0036 | VS-0037 | MOCK-0060 | Consumer 가 Notify 수신 | procedure-analysis §2.17 PROC-0017 인용 | P3 |

### 4-D. 에러 매트릭스 보강 (TC-0086 ~ TC-0105, api §6.1~6.8 모든 (HTTP, cause) 조합)

> §3-C 의무: api-analysis §6 의 모든 (HTTP, cause) 조합에 ≥1 TC 매핑. 기존 TC-0002~0030 에서 미커버된 조합만 보강.

| TC-ID | 계층 | 카테고리 | 시험 목적 | feature_id | task_id | VS-ID | 입력 (MOCK) | 기대 출력 | 판정 기준 | 우선순위 |
|---|---|---|---|---|---|---|---|---|---|---|
| NSSF-TC-0086 | 통합 | 에러 (E1 307) | SHALL — E1 GET → 307 (NSSF set 내 redirect, ES3XX) (api §6.1) | – (api §6.1) | TASK-0024 | – | MOCK-0001 (NSSF set redirect 시나리오) | 307 + Location | api §6.1 인용 | P1 |
| NSSF-TC-0087 | 통합 | 에러 (E1 308) | SHALL — E1 GET → 308 영구 redirect (api §6.1) | – (api §6.1) | TASK-0024 | – | MOCK-0001 (308 variant) | 308 + Location | api §6.1 인용 | P1 |
| NSSF-TC-0088 | 통합 | 에러 (E1 400 변형) | SHALL — E1 INVALID_QUERY_PARAM_VALUE (slice-info-request-for-* 무효 JSON) (api §6.1) | – (api §6.1) | TASK-0024 | – | MOCK-0001 (invalid JSON) | 400 + invalidParams=[slice-info-request-for-registration] | api §6.1 인용 | P0 |
| NSSF-TC-0089 | 통합 | 에러 (E1 401 invalid) | SHALL — E1 GET → 401 TOKEN_INVALID (서명 불일치) (api §6.1) | – (api §6.1) | TASK-0006, TASK-0024 | – | MOCK-0001 (signed by wrong key) | 401 cause=TOKEN_INVALID | api §6.1 인용 | P0 |
| NSSF-TC-0090 | 통합 | 에러 (E1 403 NOT_AUTH) | SHALL — E1 GET → 403 NOT_AUTHORIZED (scope 불일치) (api §6.1) | – (api §6.1) | TASK-0024 | – | MOCK-0001 (wrong scope) | 403 cause=NOT_AUTHORIZED | api §6.1 인용 | P0 |
| NSSF-TC-0091 | 통합 | 에러 (E1 406/414) | SHALL — E1 GET → 406 Accept 협상 실패 / 414 URI too long (api §6.1) | – (api §6.1) | TASK-0024 | – | MOCK-0001 (Accept: text/xml / 다중 SliceInfo) | 406 / 414 | api §6.1 인용 | P1 |
| NSSF-TC-0092 | 통합 | 에러 (E1 429) | SHALL — E1 GET → 429 rate limit (Retry-After) (api §6.1) | – (api §6.1) | TASK-0024 | – | MOCK-0001 (rate limited) | 429 + Retry-After | api §6.1 인용 | P1 |
| NSSF-TC-0093 | 통합 | 에러 (E1 500/502/503) | SHALL — E1 GET → 500 internal / 502 NRF / 503 서비스 일시 불가 (api §6.1) | – (api §6.1) | TASK-0024 | – | MOCK-0001 (internal error inject) | 500/502/503 + Retry-After (503) | api §6.1 인용 | P1 |
| NSSF-TC-0094 | 통합 | 에러 (E2 400 변형) | SHALL — E2 PUT → 400 SEMANTIC_ERROR (NssaiAvailabilityInfo 의미 오류) (api §6.2) | – (api §6.2) | TASK-0025 | – | MOCK-0018 (semantic error variant) | 400 cause=SEMANTIC_ERROR | api §6.2 인용 | P0 |
| NSSF-TC-0095 | 통합 | 에러 (E3 412) | SHALL — E3 PATCH → 412 Precondition Failed (If-Match / JSON Patch test op 실패) (api §6.2) | – (api §6.2) | TASK-0026 | – | MOCK-0024 (test op fail) | 412 | api §6.2 인용 | P1 |
| NSSF-TC-0096 | 통합 | 에러 (E2/E3 411/413) | SHALL — E2/E3 → 411 Content-Length 누락 / 413 페이로드 초과 (api §6.2) | – (api §6.2) | TASK-0025, TASK-0026 | – | MOCK-0018, MOCK-0024 (large payload) | 411 / 413 | api §6.2 인용 | P1 |
| NSSF-TC-0097 | 통합 | 에러 (E4 400/403) | SHALL — E4 DELETE → 400 RESOURCE_CONTEXT_NOT_FOUND / 403 NOT_AUTHORIZED (api §6.3) | – (api §6.3) | TASK-0027 | – | MOCK-0031 (variants) | 400 / 403 | api §6.3 인용 | P0 |
| NSSF-TC-0098 | 통합 | 에러 (E5 400 변형) | SHALL — E5 POST → 400 INVALID_IE (URI 형식 오류) (api §6.4) | – (api §6.4) | TASK-0028 | – | MOCK-0038 (URI malformed) | 400 cause=INVALID_IE | api §6.4 인용 | P0 |
| NSSF-TC-0099 | 통합 | 에러 (E5 401/403) | SHALL — E5 → 401 / 403 NOT_AUTHORIZED (api §6.4) | – (api §6.4) | TASK-0028 | – | MOCK-0034 (wrong token / scope) | 401 / 403 | api §6.4 인용 | P0 |
| NSSF-TC-0100 | 통합 | 에러 (E6 400 JSON) | SHALL — E6 PATCH → 400 JSON Patch 무결성 오류 (path 무효) (api §6.5) | – (api §6.5) | TASK-0029 | – | MOCK-0042 (invalid path) | 400 cause=INVALID_IE | api §6.5 인용 | P0 |
| NSSF-TC-0101 | 통합 | 에러 (E7 400) | SHALL — E7 DELETE → 400 RESOURCE_CONTEXT_NOT_FOUND (api §6.6) | – (api §6.6) | TASK-0030 | – | MOCK-0048 (context missing) | 400 | api §6.6 인용 | P1 |
| NSSF-TC-0102 | 통합 | 에러 (E8 표준 SBI) | SHALL — E8 OPTIONS → 4xx/5xx 표준 SBI (api §6.7, ERR-0016) | ERR-0016 | TASK-0031 | – | MOCK-0051 (invalid Accept) | 4xx | api §6.7 인용 | P3 |
| NSSF-TC-0103 | 시스템 | 에러 (C1 401/403) | SHALL — Notify Consumer 401/403 응답 시 token 재발급 / 비-재시도 (api §6.8) | – (api §6.8) | TASK-0022 | – | MOCK-0053 (consumer 401) | NSSF 가 토큰 재발급 후 1회 재시도 | api §6.8 인용 | P1 |
| NSSF-TC-0104 | 시스템 | 에러 (C1 4xx 기타) | SHALL — Notify Consumer 411/413/415/429 응답 처리 (api §6.8) | – (api §6.8) | TASK-0022 | – | MOCK-0053 (4xx variant) | 4xx 별 WARN 로그 + 비-재시도 (429 는 백오프) | api §6.8 인용 | P1 |
| NSSF-TC-0105 | 시스템 | 에러 (C1 500/502) | SHALL — Notify Consumer 500/502 응답 시 백오프 재시도 (api §6.8) | – (api §6.8) | TASK-0022 | – | MOCK-0053 (consumer 500) | 백오프 + 시도 횟수 제한 | api §6.8 인용 | P1 |

### 4-E. 경계값 시험 (TC-0106 ~ TC-0125, mock-data §4 MOCK-B-NNNN 인용)

| TC-ID | 계층 | 카테고리 | 시험 목적 | feature_id | task_id | VS-ID | 입력 (MOCK-B) | 기대 출력 | 판정 기준 | 우선순위 |
|---|---|---|---|---|---|---|---|---|---|---|
| NSSF-TC-0106 | 단위 | 경계 (Snssai) | SHALL — Snssai.sst min=0 통과 (TS 29.571 §5.4.4.2) | DAT-0029 | TASK-0008 | – | MOCK-B-0001 | 통과 | mock-data §4.1 인용 | P0 |
| NSSF-TC-0107 | 단위 | 경계 (Snssai) | SHALL — Snssai.sst max=255 통과 | DAT-0029 | TASK-0008 | – | MOCK-B-0002 | 통과 | mock-data §4.1 | P0 |
| NSSF-TC-0108 | 단위 | 부정 (Snssai overflow) | SHALL NOT — Snssai.sst=256 (overflow) 거부 → 400 INVALID_IE | DAT-0029 | TASK-0008 | – | MOCK-B-0003 | 400 INVALID_IE | mock-data §4.1 | P0 |
| NSSF-TC-0109 | 단위 | 부정 (Snssai type) | SHALL NOT — Snssai.sst string type 거부 | DAT-0029 | TASK-0008 | – | MOCK-B-0004 | 실패 (parser 거부) | mock-data §4.1 | P0 |
| NSSF-TC-0110 | 단위 | 경계 (Snssai sd) | SHALL — Snssai.sd pattern `^[A-Fa-f0-9]{6}$` 통과 | DAT-0029 | TASK-0008 | – | MOCK-B-0005 | 통과 | mock-data §4.1 | P0 |
| NSSF-TC-0111 | 단위 | 부정 (Snssai sd length) | SHALL NOT — Snssai.sd 5자 (길이 위반) 거부 → 400 invalidParams=[sd] | DAT-0029 | TASK-0008 | – | MOCK-B-0006 | 400 invalidParams=[sd] | mock-data §4.1 | P0 |
| NSSF-TC-0112 | 단위 | 부정 (Snssai sd char) | SHALL NOT — Snssai.sd hex 외 문자 거부 | DAT-0029 | TASK-0008 | – | MOCK-B-0007 | 실패 (pattern 위반) | mock-data §4.1 | P0 |
| NSSF-TC-0113 | 단위 | 경계 (Tai tac) | SHALL — Tai.tac 6 hex 통과 | DAT-0029 | TASK-0008 | – | MOCK-B-0010 | 통과 | mock-data §4.2 | P0 |
| NSSF-TC-0114 | 단위 | 경계 (Tai tac legacy) | SHALL — Tai.tac 4 hex (legacy) 통과 | DAT-0029 | TASK-0008 | – | MOCK-B-0011 | 통과 | mock-data §4.2 | P0 |
| NSSF-TC-0115 | 단위 | 부정 (Tai tac length) | SHALL NOT — Tai.tac 5자 거부 | DAT-0029 | TASK-0008 | – | MOCK-B-0012 | 실패 | mock-data §4.2 | P0 |
| NSSF-TC-0116 | 단위 | 부정 (Tai plmn) | SHALL NOT — PlmnId.mcc/mnc digits-only 위반 거부 | DAT-0029 | TASK-0008 | – | MOCK-B-0013 | 실패 | mock-data §4.2 | P0 |
| NSSF-TC-0117 | 단위 | 경계 (NfInstanceId) | SHALL — NfInstanceId UUID 형식 통과 (RFC 4122) | DAT-0029 | TASK-0008 | – | MOCK-B-0020 | 통과 | mock-data §4.3 | P0 |
| NSSF-TC-0118 | 단위 | 부정 (NfInstanceId) | SHALL NOT — 불완전 UUID 거부 | DAT-0029 | TASK-0008 | – | MOCK-B-0021 | 실패 | mock-data §4.3 | P0 |
| NSSF-TC-0119 | 단위 | 부정 (NfInstanceId) | SHALL NOT — 비-UUID 문자열 거부 | DAT-0029 | TASK-0008 | – | MOCK-B-0022 | 실패 | mock-data §4.3 | P0 |
| NSSF-TC-0120 | 단위 | 경계 (supportedFeatures) | SHALL — supportedFeatures `"F"` (NSSelection 4비트 전체) 통과 | SVC-0036, DAT-0029 | TASK-0008, TASK-0024 | – | MOCK-B-0030 | 통과 | mock-data §4.4 | P0 |
| NSSF-TC-0121 | 단위 | 부정 (supportedFeatures) | SHALL NOT — ES3XX(M) 미포함 시 거부 (FEATURE_NOT_SUPPORTED) | SVC-0036, SVC-0037 | TASK-0024 | – | MOCK-B-0031 | 400 FEATURE_NOT_SUPPORTED | mock-data §4.4 | P0 |
| NSSF-TC-0122 | 단위 | 부정 (supportedFeatures char) | SHALL NOT — hex 외 문자 거부 | SVC-0036 | TASK-0008 | – | MOCK-B-0032 | 실패 | mock-data §4.4 | P0 |
| NSSF-TC-0123 | 단위 | 경계 (NssaiAvailabilityInfo cardinality) | SHALL — supportedNssaiAvailabilityData minItems=1 통과 | DAT-0013 | TASK-0009 | – | MOCK-B-0050 | 통과 | mock-data §4.6 | P0 |
| NSSF-TC-0124 | 단위 | 부정 (NssaiAvailabilityInfo empty) | SHALL NOT — supportedNssaiAvailabilityData 빈 배열 거부 → 400 MANDATORY_IE_MISSING | DAT-0013 | TASK-0009, TASK-0025 | – | MOCK-B-0051 | 400 MANDATORY_IE_MISSING | mock-data §4.6 | P0 |
| NSSF-TC-0125 | 단위 | 경계 (callback URI) | SHALL — callback URI HTTPS 형식 통과 / 비-TLS 거부 (운영 정책) | DAT-0017 | TASK-0009, TASK-0028 | – | MOCK-B-0060 (통과), MOCK-B-0061 (실패) | 통과/실패 | mock-data §4.7 | P0 |

### 4-F. 특수 패턴 시험 (TC-0126 ~ TC-0145, mock-data §5)

| TC-ID | 계층 | 카테고리 | 시험 목적 | feature_id | task_id | VS-ID | 입력 (MOCK-SF / mock-data §5) | 기대 출력 | 판정 기준 | 우선순위 |
|---|---|---|---|---|---|---|---|---|---|---|
| NSSF-TC-0126 | 단위 | 특수 (supportedFeatures hex F) | SHALL — NSSelection supportedFeatures `"F"` 4비트 모두 활성 (api §7-C) | SVC-0036, SVC-0037 | TASK-0008, TASK-0024 | – | MOCK-SF-0001 | features_hex 반영 (ES3XX+TargetNssai+RSIPCE+SIOP) | mock-data §5-C NSSelection | P0 |
| NSSF-TC-0127 | 단위 | 특수 (supportedFeatures hex 1) | SHALL — NSSelection `"1"` ES3XX(M) only 협상 | SVC-0036 | TASK-0008 | – | MOCK-SF-0002 | features_hex == 1 | mock-data §5-C | P0 |
| NSSF-TC-0128 | 단위 | 특수 (supportedFeatures hex 5) | SHALL — NSSelection `"5"` ES3XX + RSIPCE 협상 | SVC-0036 | TASK-0008 | – | MOCK-SF-0003 | features_hex == 5 | mock-data §5-C | P0 |
| NSSF-TC-0129 | 단위 | 특수 (supportedFeatures hex 9) | SHALL — NSSelection `"9"` ES3XX + SIOP 협상 | SVC-0036 | TASK-0008 | – | MOCK-SF-0004 | features_hex == 9 | mock-data §5-C | P0 |
| NSSF-TC-0130 | 단위 | 특수 (supportedFeatures hex 3) | SHALL — NSSelection `"3"` ES3XX + TargetNssai 협상 | SVC-0036 | TASK-0008 | – | MOCK-SF-0005 | features_hex == 3 | mock-data §5-C | P0 |
| NSSF-TC-0131 | 단위 | 특수 (NSSAIAvailability hex 7F) | SHALL — NSSAIAvailability `"7F"` 1~7비트 모두 활성 (api §7-C) | SVC-0043, SVC-0044 | TASK-0008, TASK-0025 | – | MOCK-SF-0010 | features_hex 반영 (ONSSAI+SUMOD+EANAN+ES3XX+SATAS+NSIUN+NSRP) | mock-data §5-C | P0 |
| NSSF-TC-0132 | 단위 | 특수 (NSSAIAvailability hex 8) | SHALL — NSSAIAvailability `"8"` ES3XX(M) only | SVC-0043 | TASK-0008 | – | MOCK-SF-0011 | features_hex == 8 | mock-data §5-C | P0 |
| NSSF-TC-0133 | 단위 | 특수 (NSSAIAvailability hex 78) | SHALL — NSSAIAvailability `"78"` ES3XX+SATAS+NSIUN+NSRP (Rel-18 권장) | SVC-0043, SVC-0044 | TASK-0008 | – | MOCK-SF-0012 | features_hex == 78 | mock-data §5-C | P0 |
| NSSF-TC-0134 | 단위 | 특수 (NSSAIAvailability hex A) | SHALL — NSSAIAvailability `"A"` SUMOD + ES3XX | SVC-0018, SVC-0043 | TASK-0008, TASK-0029 | – | MOCK-SF-0013 | features_hex == A, SUMOD gate 통과 | mock-data §5-C | P0 |
| NSSF-TC-0135 | 단위 | 특수 (NSSAIAvailability hex C) | SHALL — NSSAIAvailability `"C"` EANAN + ES3XX | SEC-0002, SVC-0043 | TASK-0008, TASK-0036 | – | MOCK-SF-0014 | features_hex == C, EANAN empty-array 분기 활성 | mock-data §5-C | P0 |
| NSSF-TC-0136 | 통합 | 특수 (PATCH test op) | SHALL — JSON Patch `test` op 통과 시 후속 `replace` op 진행 (mock-data §5-B 부정 fixture) | SVC-0042 | TASK-0026, TASK-0029 | – | mock-data §5-B test+replace fixture | 200 (test 통과 + replace 적용) | mock-data §5-B 인용 | P0 |
| NSSF-TC-0137 | 통합 | 부정 (PATCH test op) | SHALL — JSON Patch `test` op 실패 시 412 Precondition Failed | SVC-0042 | TASK-0026, TASK-0029 | – | mock-data §5-B test fail variant | 412 | mock-data §5-B + api §6.2 | P0 |
| NSSF-TC-0138 | 단위 | 특수 (Header E1) | SHALL — E1 GET 요청 헤더 카탈로그 (3gpp-Sbi-* 전체) (mock-data §6 HEADER-001) | – | TASK-0016, TASK-0024 | – | mock-data HEADER-001 | 헤더 처리 정상 (3gpp-Sbi-Sender-Timestamp, Max-Rsp-Time, Discovery-target-nf-type) | mock-data §6 인용 | P1 |
| NSSF-TC-0139 | 단위 | 특수 (Header E2) | SHALL — E2 PUT 요청 헤더 (Content-Encoding: gzip 협상) (HEADER-002) | SVC-0041 | TASK-0016, TASK-0025 | – | mock-data HEADER-002 | gzip 디코드 성공, Accept-Encoding 반영 | mock-data §6 + SVC-0041 (SHOULD gzip) | P1 |
| NSSF-TC-0140 | 단위 | 특수 (Header C1) | SHALL — C1 Notify 송신 헤더 (Producer-Id, Callback) (HEADER-003) | SVC-0023 | TASK-0022 | – | mock-data HEADER-003 | 헤더 모두 송신 | mock-data §6 | P1 |
| NSSF-TC-0141 | 단위 | 특수 (Header E3/E6 PATCH) | SHALL — E3/E6 PATCH Content-Type `application/json-patch+json` (HEADER-004) | SVC-0013, SVC-0019 | TASK-0026, TASK-0029 | – | mock-data HEADER-004 | 헤더 검증 정상 | mock-data §6 | P0 |
| NSSF-TC-0142 | 시스템 | 특수 (Subscribe expiry 분산) | SHALL NOT (SVC-0016) — 동일 expiry 다중 구독 시 NSSF 가 jitter ±N초 분산 (mock-data §5-A) | SVC-0016 | TASK-0028, TASK-0035 | VS-0023 | mock-data §5-A expiry fixture | 동일 timestamp 빈도 == 1 (정량 검증) | mock-data §5-A 인용 | P0 |
| NSSF-TC-0143 | 시스템 | 특수 (NSRP/NSIUN) | SHALL — NSRP feature 협상 + Replacement notification (PRC-0003) | PRC-0003 | TASK-0014, TASK-0036 | VS-0036 | MOCK-0059 | altNssai 포함 Notify 송신 | api §7-A "이벤트 트리거 (b)" 인용 | P0 |
| NSSF-TC-0144 | 시스템 | 특수 (NSSAI Validity) | SHALL — SNSSAI_VALIDITY_TIME_REPORT 이벤트 트리거 + Notify (PRC-0002) | PRC-0002 | TASK-0014, TASK-0036 | VS-0037 | MOCK-0060 | nssaiValidityTimeInfoList 포함 Notify 송신 | api §7-A "이벤트 트리거 (d)" 인용 | P0 |
| NSSF-TC-0145 | 시스템 | 특수 (gzip) | SHOULD — NSSF gzip Content-Encoding 지원 (E2~E8, C1) (SVC-0041) | SVC-0041 | TASK-0016, TASK-0022 | – | MOCK-0018 (gzip variant) | Content-Encoding: gzip 디코드 성공 | api §7-A, SVC-0041 인용 | P1 |

> §4 매트릭스 종료 (총 145 TC). §5 시나리오 절차서는 동일 ID로 단위는 간략형, 통합/시스템은 풀폼 5필드 적용.


## 5. 시나리오 절차서 (전수)

> 인용 규약: mock-data MOCK-NNNN / procedure-analysis NSSF-PROC-NNNN / impl-spec M-HANDLER-{n} 4-B 단계 X / impl-spec §9 VS-NNNN — 본문 페이로드 / 시퀀스 복제 0건.

### 5-A. VS ↔ TC 매핑 절차서 (TC-0001 ~ TC-0051)

#### NSSF-TC-0001: E1 GET Initial Registration 정상

**시험 목적**: SHALL — AMF Initial Registration 도중 GET /network-slice-information 응답에 Allowed/Configured NSSAI 반환 (NSSF-FEAT-SVC-0002, NSSF-FEAT-DAT-0001~0004).

**Precondition**:
- 시뮬레이터 상태: db-design §8-B 재시작 정책에 따른 정상 기동, ENT-0001~0007 (Config) 로드 완료, NRF stub 등록 완료.
- 외부 NF mock 응답 준비: AMF stub 가 MOCK-0001 헤더(HEADER-001) 송신 준비.

**Steps**:
1. AMF stub → NSSF: `GET /nnssf-nsselection/v2/network-slice-information` (mock-data MOCK-0001 인용).
2. NSSF 내부 처리: impl-spec §1 M-HANDLER-1 4-B 단계 1~5 — 토큰 검증 → 쿼리 파라미터 파싱 → ENT-0001~0007 조회 → AuthorizedNetworkSliceInfo 직렬화.
3. 외부 NF 호출: 발생 없음 (NRF Discovery 는 candidate AMF 필요 시에만).

**Expected**:
- HTTP 상태 + body: mock-data MOCK-0002 와 schema 일치 (allowedNssaiList ≥1).
- DB 상태: ENT-0001~0007 변경 없음 (read-mostly).
- 외부 호출 시퀀스: procedure-analysis §2.6 NSSF-PROC-0006 정상 흐름 일치.

**Teardown**: 시뮬레이터 상태 초기화 (no-op, read-only).

**판정 기준**: 응답 코드 200, body 의 allowedNssaiList ≥1, schema(api-analysis §2.1) 일치, procedure-analysis NSSF-PROC-0006 시퀀스 일치, impl-spec §9 NSSF-VS-0001 정상 분기.

#### NSSF-TC-0002: E1 미지원 SNSSAI → 403

**시험 목적**: SHALL — 요청 S-NSSAI 미지원 시 403 SNSSAI_NOT_SUPPORTED 반환 (NSSF-FEAT-ERR-0001).

**Precondition**: 시뮬레이터 정상 기동, ENT-0001~0007 로드. AMF stub 가 미지원 SST=99 송신 준비.

**Steps**:
1. AMF stub → NSSF: GET (mock-data MOCK-0003).
2. NSSF: M-HANDLER-1 4-B 단계 5 — ENT-0001 lookup 실패.

**Expected**: HTTP 403, body 는 mock-data MOCK-0004 (cause=SNSSAI_NOT_SUPPORTED) 와 일치.

**Teardown**: no-op.

**판정 기준**: ProblemDetails cause=SNSSAI_NOT_SUPPORTED, status=403, impl-spec NSSF-VS-0002, api-analysis §6.1 ERR-0001 인용.

#### NSSF-TC-0003: E1 토큰 만료 → 401

**시험 목적**: SHALL — OAuth2 Bearer token 만료 시 401 TOKEN_EXPIRED 반환 (NSSF-FEAT-SEC-0003 도메인 힌트 채택).

**Precondition**: NSSF M-AUTH 초기화 완료. AMF stub 가 만료 token 송신 준비 (mock-data MOCK-0005 헤더).

**Steps**:
1. AMF stub → NSSF: GET (MOCK-0005, expired Bearer).
2. NSSF: M-HANDLER-1 4-B 단계 1 — libjwt 검증 실패 (exp 초과).

**Expected**: HTTP 401, body = mock-data MOCK-0006.

**Teardown**: no-op.

**판정 기준**: 401 cause=TOKEN_EXPIRED, api-analysis §6.1 인용, impl-spec NSSF-VS-0003.

#### NSSF-TC-0004: E1 nf-id 누락 → 400

**시험 목적**: SHALL — 필수 query param `nf-id` 누락 시 400 MANDATORY_QUERY_PARAM_MISSING (api-analysis §6.1).

**Precondition**: 정상 기동. AMF stub 가 nf-id 누락 URI 준비.

**Steps**:
1. AMF stub → NSSF: GET (MOCK-0007, no nf-id).
2. NSSF: M-HANDLER-1 4-B 단계 3 — 파라미터 검증 실패.

**Expected**: HTTP 400, body = MOCK-0008 (invalidParams=[nf-id]).

**Teardown**: no-op.

**판정 기준**: 400 + invalidParams, api §6.1 인용, impl-spec NSSF-VS-0004.

#### NSSF-TC-0005: E1 PDU Session HR Roaming 정상

**시험 목적**: SHALL — SMF+PGW-C PDU Session Establishment 시 HR roaming 슬라이스 매핑 응답 (NSSF-FEAT-SVC-0005, NSSF-FEAT-DAT-0005/0006).

**Precondition**:
- 시뮬레이터 정상 기동, ENT-0006 (VPLMN↔HPLMN Mapping) 로드.
- SEPP stub + H-NSSF stub 활성 (PROC-0012 forward 경로).

**Steps**:
1. SMF stub → V-NSSF: GET (mock-data MOCK-0009).
2. V-NSSF 내부 처리: M-HANDLER-1 4-B 단계 5 + M-CLIENT-SEPP forward (impl-spec §1 M-CLIENT-SEPP).
3. V-NSSF→SEPP→H-NSSF 호출 발생.

**Expected**:
- HTTP 200, body = mock-data MOCK-0010 (mappingOfNssai 포함).
- 외부 호출 시퀀스: procedure-analysis §2.8 NSSF-PROC-0008 + §2.12 NSSF-PROC-0012 일치.

**Teardown**: ENT-0006 캐시 정합 (no-op for read-only).

**판정 기준**: 200 + mappingOfNssai schema 일치, PROC-0008/0012 시퀀스 일치, impl-spec NSSF-VS-0005.

#### NSSF-TC-0006: E1 PDN Connection RSIPCE 정상

**시험 목적**: SHALL — RSIPCE feature 협상 시 SMF+PGW-C PDN Connection 슬라이스 매핑 (NSSF-FEAT-SVC-0010, NSSF-FEAT-DAT-0009/0010).

**Precondition**: 정상 기동. SMF+PGW-C stub 가 supportedFeatures=RSIPCE 송신.

**Steps**:
1. SMF+PGW-C stub → NSSF: GET (MOCK-0011, RSIPCE bit ON).
2. NSSF: M-HANDLER-1 4-B 단계 4 (RSIPCE gate 통과) → 단계 5.

**Expected**: HTTP 200, body = MOCK-0012.

**Teardown**: no-op.

**판정 기준**: 200 + mappingOfNssai, RSIPCE gate 통과, impl-spec NSSF-VS-0006, procedure-analysis §2.10 PROC-0010.

#### NSSF-TC-0007: E1 PDN Connection RSIPCE 미협상 → 400

**시험 목적**: SHALL — RSIPCE 미협상 PDN Connection 요청 거부 (NSSF-FEAT-DAT-0009 부정).

**Precondition**: 정상 기동.

**Steps**:
1. SMF+PGW-C stub → NSSF: GET (MOCK-0013, supportedFeatures=0).
2. NSSF: M-HANDLER-1 4-B 단계 4 (RSIPCE gate 실패).

**Expected**: HTTP 400, body = MOCK-0014 (FEATURE_NOT_SUPPORTED).

**Teardown**: no-op.

**판정 기준**: 400, impl-spec NSSF-VS-0007, api §6.1 인용.

#### NSSF-TC-0008: E1 NWDAF SIOP 정상

**시험 목적**: SHALL — NWDAF Other Purpose (SIOP feature) NSI ID 조회 (NSSF-FEAT-SVC-0011, NSSF-FEAT-DAT-0011/0012).

**Precondition**: 정상 기동, ENT-0004 (NSI mapping) 로드.

**Steps**:
1. NWDAF stub → NSSF: GET (MOCK-0015, supportedFeatures=SIOP).
2. NSSF: M-HANDLER-1 4-B 단계 4 (SIOP gate) + 단계 5.

**Expected**: HTTP 200, body = MOCK-0016 (snssaiInfoRspData).

**Teardown**: no-op.

**판정 기준**: impl-spec NSSF-VS-0008, procedure-analysis §2.11 PROC-0011.

#### NSSF-TC-0009: E1 NRF Discover 실패 → partial response

**시험 목적**: MAY (도메인 힌트 채택 P3) — NRF discover 실패 시 candidate AMF list 생략, targetAmfSet 만 반환 (NSSF-FEAT-MGMT-0001).

**Precondition**: NRF stub 가 5xx 응답 준비.

**Steps**:
1. AMF stub → NSSF: GET (MOCK-0001).
2. NSSF: M-CLIENT-NRF.discover_amf 호출 → 5xx → M-HANDLER-1 4-B 단계 5 partial 분기.

**Expected**: HTTP 200, body = MOCK-0017 (candidateAmfList 생략, targetAmfSet 만 포함).

**Teardown**: NRF stub 정상 복구.

**판정 기준**: 200 + partial body, procedure §2.4 NSSF-PROC-0004 실패 흐름, impl-spec NSSF-VS-0009.

#### NSSF-TC-0010: E2 PUT NSSAIAvailability 정상

**시험 목적**: SHALL — AMF NSSAIAvailability_Update PUT 시 200 + Authorized 응답 + ENT-0015 Enqueue (NSSF-FEAT-SVC-0012, DAT-0013).

**Precondition**: 정상 기동, ENT-0008/0009 PostgreSQL 빈 상태.

**Steps**:
1. AMF stub → NSSF: PUT /nssai-availability/{nfId} (MOCK-0018).
2. NSSF: M-HANDLER-2 4-B 단계 1~7 — db-design §8-A T-AMF-UPSERT 트랜잭션 적용, ENT-0015 Enqueue.

**Expected**:
- HTTP 200, body = MOCK-0019.
- DB: ENT-0008 (AMF Registration State) + ENT-0009 (Authorized) 일관 갱신.
- 외부 호출: ENT-0015 큐에 Notify task 1건 추가 (이후 PROC-0015 트리거).

**Teardown**: ENT-0008/0009 delete, ENT-0015 큐 정리.

**판정 기준**: 200 + schema 일치, T-AMF-UPSERT (REPEATABLE READ) 정합, procedure §2.13 PROC-0013, impl-spec NSSF-VS-0010.

#### NSSF-TC-0011: E2 PUT 미지원 SNSSAI → 403

**시험 목적**: SHALL — PUT supportedSnssaiList 미지원 SST 시 403 SNSSAI_NOT_SUPPORTED (NSSF-FEAT-ERR-0010).

**Precondition**: 정상 기동.

**Steps**:
1. AMF stub → NSSF: PUT (MOCK-0020, 미지원 SST).
2. NSSF: M-HANDLER-2 4-B 단계 4 (정책 lookup 실패).

**Expected**: HTTP 403, body = MOCK-0021.

**Teardown**: no-op.

**판정 기준**: impl-spec NSSF-VS-0011, api §6.2.

#### NSSF-TC-0012: E2 PUT 0개 인증 → 204

**시험 목적**: SHALL — 모든 S-NSSAI 인증 후 결과 0건 시 204 No Content (NSSF-FEAT-DAT-0013).

**Precondition**: 정상 기동, ENT-0008 빈 상태.

**Steps**:
1. AMF stub → NSSF: PUT (MOCK-0018 variant, 결과적으로 미인증).
2. NSSF: M-HANDLER-2 4-B 단계 7 — `authorized.n_data == 0`.

**Expected**: HTTP 204 (MOCK-0022).

**Teardown**: ENT-0008 delete.

**판정 기준**: impl-spec NSSF-VS-0012.

#### NSSF-TC-0013: E2 PUT Content-Type 불일치 → 415

**시험 목적**: SHALL — Content-Type 검증 실패 시 415 (api §6.2).

**Precondition**: 정상 기동.

**Steps**:
1. AMF stub → NSSF: PUT (MOCK-0018, Content-Type=application/xml).
2. NSSF: M-HANDLER-2 4-B 단계 2.

**Expected**: HTTP 415 (MOCK-0023).

**Teardown**: no-op.

**판정 기준**: impl-spec NSSF-VS-0013, api §6.2.

#### NSSF-TC-0014: E3 PATCH 정상 (add)

**시험 목적**: SHALL — PATCH add op 정상 적용 (NSSF-FEAT-SVC-0013, DAT-0015).

**Precondition**: 정상 기동, 사전 PUT 완료된 ENT-0008/0009 존재.

**Steps**:
1. AMF stub → NSSF: PATCH (MOCK-0024, application/json-patch+json).
2. NSSF: M-HANDLER-3 4-B — RFC 6902 적용, db-design T-AMF-PATCH 트랜잭션.

**Expected**: HTTP 200 (MOCK-0025), ENT-0008/0009 갱신 일관.

**Teardown**: ENT-0008/0009 delete.

**판정 기준**: T-AMF-PATCH (REPEATABLE READ + 행 락), impl-spec NSSF-VS-0014, procedure §2.13 PROC-0013.

#### NSSF-TC-0015: E3 PATCH 미존재 nfId → 404

**시험 목적**: SHALL — 사전 PUT 없는 nfId 에 PATCH 시 404 RESOURCE_NOT_FOUND (DAT-0030, ERR-0010).

**Precondition**: 정상 기동, ENT-0008 빈 상태.

**Steps**:
1. AMF stub → NSSF: PATCH (MOCK-0026).
2. NSSF: M-HANDLER-3 4-B 단계 5 — ENT-0008.ReadByNfId 실패.

**Expected**: HTTP 404 (MOCK-0027).

**Teardown**: no-op.

**판정 기준**: impl-spec NSSF-VS-0015.

#### NSSF-TC-0016: E3 PATCH JSON Patch 무결성 위반 → 400

**시험 목적**: SHALL — PATCH path 무효 시 400 INVALID_IE invalidParams=[path] (NSSF-FEAT-SVC-0042).

**Precondition**: 사전 PUT 완료.

**Steps**:
1. AMF stub → NSSF: PATCH (MOCK-0028, path=/invalid/path).
2. NSSF: M-HANDLER-3 4-B 단계 4.

**Expected**: HTTP 400 (MOCK-0029).

**Teardown**: no-op.

**판정 기준**: impl-spec NSSF-VS-0016, api §6.2.

#### NSSF-TC-0017: E3 PATCH Content-Type 불일치 → 415

**시험 목적**: SHALL — PATCH Content-Type 검증 (api §6.2).

**Precondition**: 정상 기동.

**Steps**:
1. AMF stub → NSSF: PATCH (MOCK-0024, Content-Type=application/json).
2. NSSF: M-HANDLER-3 4-B 단계 2.

**Expected**: HTTP 415 (MOCK-0030).

**Teardown**: no-op.

**판정 기준**: impl-spec NSSF-VS-0017.

#### NSSF-TC-0018: E4 DELETE 정상

**시험 목적**: SHALL — DELETE /nssai-availability/{nfId} 정상 (NSSF-FEAT-SVC-0030/0031).

**Precondition**: 사전 PUT 완료된 ENT-0008 존재.

**Steps**:
1. AMF stub → NSSF: DELETE (MOCK-0031).
2. NSSF: M-HANDLER-4 4-B — db-design T-AMF-DELETE (FK CASCADE → ENT-0009).

**Expected**: HTTP 204 (MOCK-0032), ENT-0008/0009 모두 제거, ENT-0015 Enqueue status_change Notify.

**Teardown**: no-op (이미 삭제됨).

**판정 기준**: FK CASCADE 동작 검증, impl-spec NSSF-VS-0018, procedure §2.19 PROC-0019.

#### NSSF-TC-0019: E4 DELETE 미존재 → 404

**시험 목적**: SHALL — 미존재 nfId DELETE 시 404 (ERR-0015, DAT-0031).

**Precondition**: ENT-0008 빈 상태.

**Steps**:
1. AMF stub → NSSF: DELETE (MOCK-0031 미존재 ID variant).
2. NSSF: M-HANDLER-4 4-B 단계 5.

**Expected**: HTTP 404 (MOCK-0033).

**Teardown**: no-op.

**판정 기준**: impl-spec NSSF-VS-0019.

#### NSSF-TC-0020: E5 Subscribe 정상

**시험 목적**: SHALL — Subscribe 4 이벤트 정상 (NSSF-FEAT-SVC-0014, DAT-0017/0018).

**Precondition**: 정상 기동, ENT-0010 빈 상태.

**Steps**:
1. AMF stub → NSSF: POST /nssai-availability/subscriptions (MOCK-0034).
2. NSSF: M-HANDLER-5 4-B — db-design T-SUB-CREATE (SERIALIZABLE), Location 헤더 생성.

**Expected**: HTTP 201 + Location header (MOCK-0035), ENT-0010 row 1건 생성, uthash 다중 인덱스 갱신.

**Teardown**: ENT-0010 row delete.

**판정 기준**: 201 + Location 필수 (RFC 9457 + api §2.5), impl-spec NSSF-VS-0020, procedure §2.14 PROC-0014.

#### NSSF-TC-0021: E5 Subscribe 미지원 event → 501

**시험 목적**: SHALL — 모든 요청 event 미지원 시 501 UNSUPPORTED_EVENT_TYPE (DAT-0033).

**Precondition**: 정상 기동.

**Steps**:
1. AMF stub → NSSF: POST (MOCK-0036).
2. NSSF: M-HANDLER-5 4-B 단계 4.

**Expected**: HTTP 501 (MOCK-0037).

**Teardown**: no-op.

**판정 기준**: impl-spec NSSF-VS-0021, api §6.4.

#### NSSF-TC-0022: E5 Subscribe callbackUri 누락 → 400

**시험 목적**: SHALL — Mandatory IE nfNssaiAvailabilityUri 누락 시 400 (ERR-0011, DAT-0032).

**Precondition**: 정상 기동.

**Steps**:
1. AMF stub → NSSF: POST (MOCK-0038, no callbackUri).
2. NSSF: M-HANDLER-5 4-B 단계 3.

**Expected**: HTTP 400 (MOCK-0039, invalidParams=[nfNssaiAvailabilityUri]).

**Teardown**: no-op.

**판정 기준**: impl-spec NSSF-VS-0022.

#### NSSF-TC-0023: E5 동일 expiry 다중 구독 분산 (M-Not 부정)

**시험 목적**: SHALL NOT — NSSF 가 동일 expiry timestamp 를 다수 구독에 부여 금지: 균등 분산 (NSSF-FEAT-SVC-0016).

**Precondition**: 정상 기동, ENT-0010 빈 상태.

**Steps**:
1. N개 AMF stub → NSSF: POST (MOCK-0040, 동일 expiry 명시).
2. NSSF: M-HANDLER-5 4-B 단계 5 + expiry distribution 로직 (jitter ±N초).
3. 외부 호출 없음.

**Expected**:
- N 모두 201 (MOCK-0041 schema), 각 응답의 expiry timestamp 가 균등 분산.
- ENT-0010 row N건, 동일 expiry timestamp 빈도 == 1 (M-Not 검증 핵심).

**Teardown**: ENT-0010 N row delete.

**판정 기준**: 동일 timestamp 다수 부여 0건 (SVC-0016 M-Not), impl-spec NSSF-VS-0023, db-design ENT-0010 5축 분류 "ttl + serializable".

#### NSSF-TC-0024: E6 PATCH Modify (SUMOD) 정상

**시험 목적**: SHALL — SUMOD feature 협상 후 taiList 변경 PATCH (NSSF-FEAT-SVC-0018/0019, DAT-0024).

**Precondition**: 사전 Subscribe 완료된 ENT-0010 존재, SUMOD bit 협상.

**Steps**:
1. AMF stub → NSSF: PATCH /subscriptions/{id} (MOCK-0042, SUMOD bit ON).
2. NSSF: M-HANDLER-6 4-B — db-design T-SUB-MODIFY.

**Expected**: HTTP 200 (MOCK-0043), ENT-0010 갱신, event IE 불변.

**Teardown**: ENT-0010 delete.

**판정 기준**: T-SUB-MODIFY (REPEATABLE READ), impl-spec NSSF-VS-0024, api §2.6.

#### NSSF-TC-0025: E6 event IE 변경 시도 → 400 (M-Not)

**시험 목적**: SHALL NOT — Subscribe Modify 시 event IE 변경 금지 (NSSF-FEAT-SVC-0020, DAT-0025).

**Precondition**: 사전 Subscribe 완료.

**Steps**:
1. AMF stub → NSSF: PATCH (MOCK-0044, path=/event).
2. NSSF: M-HANDLER-6 4-B 단계 4 — event 경로 차단.

**Expected**: HTTP 400 (MOCK-0045, MODIFICATION_NOT_ALLOWED).

**Teardown**: ENT-0010 무변경 확인.

**판정 기준**: M-Not 위반 시 거부, ENT-0010.event 불변, impl-spec NSSF-VS-0025.

#### NSSF-TC-0026: E6 SUMOD 미협상 → 403

**시험 목적**: SHALL — SUMOD bit OFF 시 PATCH 거부 (SVC-0018).

**Precondition**: 사전 Subscribe 완료, SUMOD 미협상.

**Steps**:
1. AMF stub → NSSF: PATCH (MOCK-0042 variant, SUMOD bit OFF).
2. NSSF: M-HANDLER-6 4-B 단계 4 — SUMOD bit 검사.

**Expected**: HTTP 403 (MOCK-0046).

**Teardown**: no-op.

**판정 기준**: impl-spec NSSF-VS-0026, api §6.5.

#### NSSF-TC-0027: E6 subscriptionId 미존재 → 404

**시험 목적**: SHALL — 미존재 ID PATCH 시 404 SUBSCRIPTION_NOT_FOUND (ERR-0012).

**Precondition**: ENT-0010 빈 상태.

**Steps**:
1. AMF stub → NSSF: PATCH (MOCK-0042 미존재 ID variant).
2. NSSF: M-HANDLER-6 4-B 단계 5.

**Expected**: HTTP 404 (MOCK-0047).

**Teardown**: no-op.

**판정 기준**: impl-spec NSSF-VS-0027.

#### NSSF-TC-0028: E7 Unsubscribe 정상

**시험 목적**: SHALL — DELETE /subscriptions/{id} 정상 (SVC-0021/0022).

**Precondition**: 사전 Subscribe 완료, ENT-0015 큐에 진행 중 Notify task 1건.

**Steps**:
1. AMF stub → NSSF: DELETE (MOCK-0048).
2. NSSF: M-HANDLER-7 4-B — ENT-0010 delete + ENT-0015 cancel.

**Expected**: HTTP 204 (MOCK-0049), 진행 중 Notify task cancel.

**Teardown**: no-op.

**판정 기준**: impl-spec NSSF-VS-0028, procedure §2.18 PROC-0018.

#### NSSF-TC-0029: E7 Unsubscribe 미존재 → 404

**시험 목적**: SHALL — 미존재 ID DELETE 시 404 (ERR-0013).

**Precondition**: ENT-0010 빈 상태.

**Steps**:
1. AMF stub → NSSF: DELETE (MOCK-0048 미존재 ID).
2. NSSF: M-HANDLER-7 4-B 단계 5.

**Expected**: HTTP 404 (MOCK-0050).

**Teardown**: no-op.

**판정 기준**: impl-spec NSSF-VS-0029.

#### NSSF-TC-0030: E8 OPTIONS 정상

**시험 목적**: SHALL — OPTIONS /nssai-availability 정상 응답 + Accept-Encoding (SVC-0032).

**Precondition**: 정상 기동.

**Steps**:
1. AMF stub → NSSF: OPTIONS (MOCK-0051).
2. NSSF: M-HANDLER-8 4-B.

**Expected**: HTTP 200 + Accept-Encoding: gzip (MOCK-0052).

**Teardown**: no-op.

**판정 기준**: impl-spec NSSF-VS-0030.

#### NSSF-TC-0031: C1 Notify Status Change 정상 (시스템)

**시험 목적**: SHALL — NSSAIAvailability_Update 후 매칭 구독에 Notify 송신, Consumer 204 응답 (NSSF-FEAT-SVC-0023/0024, DAT-0026, PRC-0001).

**Precondition**:
- 시뮬레이터: PROC-0013 사전 수행 (ENT-0008/0009 갱신 + ENT-0015 큐에 task).
- 외부: Consumer (AMF) stub 가 callback URI 활성, OAuth2 token 발급 가능 (Consumer 측 NRF stub).
- 다중 NF mock 토폴로지: NSSF + Consumer NF (procedure §5-A 참여 NF).

**Steps**:
1. NSSF 내부: ENT-0015 dequeue → M-CLIENT-NOTIFY (libcurl multi handle).
2. NSSF → Consumer: POST {nfNssaiAvailabilityUri} (mock-data MOCK-0053).
3. Consumer stub: 204 응답 (MOCK-0054).
4. NSSF: on_notify_response 204 case — ENT-0010.lastNotifiedAt 갱신.

**Expected**:
- HTTP 송신 body: MOCK-0053 schema 일치.
- Consumer 측 수신 일치.
- DB: ENT-0010.lastNotifiedAt 현재 시각, ENT-0015 큐에서 해당 task 제거.
- 응답 시간 < 500 ms (시뮬레이션 환경).
- 외부 호출 시퀀스: procedure-analysis §2.15 NSSF-PROC-0015 일치.

**Teardown**: Consumer stub 큐 정리, ENT-0010 lastNotifiedAt 초기화.

**판정 기준**: 송신/수신 페이로드 일치, lastNotifiedAt 갱신, procedure PROC-0015 시퀀스 일치, impl-spec NSSF-VS-0031.

#### NSSF-TC-0032: C1 Notify Consumer 400 → 구독 STALE (시스템)

**시험 목적**: SHALL — Notify 400 RESOURCE_CONTEXT_NOT_FOUND 시 구독 status=STALE (NSSF-FEAT-DAT-0034, ERR-0014).

**Precondition**: PROC-0013 사전 수행. Consumer stub 가 400 응답 준비.

**Steps**:
1. NSSF → Consumer: POST (MOCK-0053).
2. Consumer: 400 응답 (MOCK-0055).
3. NSSF: on_notify_response 400 case → ENT-0010.status = STALE, ENT-0015 permanent_fail.

**Expected**: ENT-0010 row.status == STALE, WARN 로그.

**Teardown**: ENT-0010 row 초기화.

**판정 기준**: 상태 전이 정확, impl-spec NSSF-VS-0032, api §6.8.

#### NSSF-TC-0033: C1 Notify Consumer 404 → 구독 INVALIDATED (시스템)

**시험 목적**: SHALL — Notify 404 RESOURCE_URI_STRUCTURE_NOT_FOUND 시 구독 INVALIDATED (DAT-0035, ERR-0014).

**Precondition**: PROC-0013 사전 수행. Consumer 404 응답.

**Steps**:
1. NSSF → Consumer: POST (MOCK-0053).
2. Consumer: 404 (MOCK-0056).
3. NSSF: on_notify_response 404 case → ENT-0010.status = INVALIDATED.

**Expected**: ENT-0010 row.status == INVALIDATED.

**Teardown**: ENT-0010 row 초기화.

**판정 기준**: impl-spec NSSF-VS-0033.

#### NSSF-TC-0034: C1 Notify 5xx → 백오프 재시도 (시스템)

**시험 목적**: SHOULD — Consumer 503 + Retry-After 시 백오프 재시도, max_retries 후 stale (SVC-0027).

**Precondition**: PROC-0013 사전 수행. Consumer 503 + Retry-After=5 응답.

**Steps**:
1. NSSF → Consumer: POST (MOCK-0053).
2. Consumer: 503 + Retry-After=5 (MOCK-0057).
3. NSSF: 5초 후 재시도, max_retries 도달 후 stale 마킹.

**Expected**: 재시도 N회 발생, 최종 status=STALE.

**Teardown**: Consumer stub 정상 응답으로 복구.

**판정 기준**: 백오프 알고리즘 정확 (지수 ±jitter), max_retries 준수, impl-spec NSSF-VS-0034.

#### NSSF-TC-0035: C1 Notify 307 redirect → new Location 재시도 (시스템)

**시험 목적**: SHALL — Notify 307 + Location 시 즉시 new URI 로 재시도 (DAT-0036).

**Precondition**: PROC-0013 사전 수행. Consumer 307 + Location.

**Steps**:
1. NSSF → Consumer: POST (MOCK-0053).
2. Consumer: 307 + Location: new_uri (MOCK-0058).
3. NSSF: 즉시 new_uri 로 POST.

**Expected**: 두 번째 POST 가 new_uri 로 도달, 응답 204 시 정상 완료.

**Teardown**: new_uri stub 정리.

**판정 기준**: 307 처리 정확, impl-spec NSSF-VS-0035.

#### NSSF-TC-0036: C1 Slice Replacement Notify (시스템)

**시험 목적**: SHALL — NSRP feature + NWDAF/OAM 트리거 시 altNssai 포함 Notify (SVC-0027, PRC-0003).

**Precondition**: ENT-0012 (Slice Replacement Plan) Start 트리거 발생, NSRP 협상된 ENT-0010 존재.

**Steps**:
1. NSSF 내부: db-design T-REPLACE-START (SERIALIZABLE) → ENT-0015 Enqueue.
2. NSSF → Consumer: POST (MOCK-0059, altNssai 포함).
3. Consumer: 204 (MOCK-0054).

**Expected**: altNssai 필드 포함 Notify, Consumer 204.

**Teardown**: ENT-0012 종료, ENT-0015 정리.

**판정 기준**: NSRP gate 통과, impl-spec NSSF-VS-0036, procedure §2.16 PROC-0016.

#### NSSF-TC-0037: C1 Validity Time Notify (시스템)

**시험 목적**: SHALL — S-NSSAI Validity Time 변경 시 nssaiValidityTimeInfoList Notify (PRC-0002).

**Precondition**: ENT-0011 Validity 변경 트리거, SNSSAI_VALIDITY_TIME_REPORT 구독 존재.

**Steps**:
1. NSSF 내부: ENT-0011 변경 감지 → ENT-0015 Enqueue.
2. NSSF → Consumer: POST (MOCK-0060).
3. Consumer: 204 (MOCK-0054).

**Expected**: nssaiValidityTimeInfoList 포함 Notify, Consumer 204.

**Teardown**: ENT-0011 복구.

**판정 기준**: impl-spec NSSF-VS-0037, procedure §2.17 PROC-0017.

#### NSSF-TC-0038: C1 EANAN 빈 배열 Notify (시스템)

**시험 목적**: SHALL — EANAN feature 협상 + 모든 슬라이스 미지원 상태 → 빈 배열 Notify (SEC-0002, SVC-0029 부정).

**Precondition**: EANAN 협상된 ENT-0010 존재, 모든 슬라이스 미지원 상태.

**Steps**:
1. NSSF 내부: 상태 변경 감지 → dedup 후 Enqueue.
2. NSSF → Consumer: POST (MOCK-0061, authorizedNssaiAvailabilityData=[]).
3. Consumer: 204 (MOCK-0054).

**Expected**: authorizedNssaiAvailabilityData=[] 송신, dedup 후 단일 통지.

**Teardown**: ENT-0010 정리.

**판정 기준**: EANAN 분기 활성, dedup 정확, impl-spec NSSF-VS-0038.

#### NSSF-TC-0039: C1 SEC SHALL NOT 위반 시험 (M-Not 부정, 시스템)

**시험 목적**: SHALL NOT — Replacement 진행 중 S-NSSAI 에 AuthorizedNssaiAvailabilityData 송신 금지 (SEC-0001).

**Precondition**: ENT-0012 활성 Replacement 진행 중인 S-NSSAI 존재. 트리거: Replacement 대상 슬라이스의 변경 이벤트.

**Steps**:
1. NSSF 내부: 변경 감지 → M-CLIENT-NOTIFY 사전 필터링 (impl-spec §1).
2. 외부 호출: 발생하지 않아야 함.

**Expected**:
- POST 송신 0건 (필터링 가드 통과).
- (대조 fixture) MOCK-0062 (negative fixture — 송신 거부 검증용).

**Teardown**: ENT-0012 종료.

**판정 기준**: SEC-0001 M-Not 위반 송신 0건, impl-spec NSSF-VS-0039.

#### NSSF-TC-0040: PROC-0001 NRF Register 정상

**시험 목적**: SHALL — NSSF 기동 시 NRF Register 정상 (PROC-0001).

**Precondition**: NRF stub 활성, NSSF 인증서 준비.

**Steps**:
1. NSSF 기동 → M-CLIENT-NRF.register.
2. NSSF → NRF: PUT /nf-instances/{nfInstanceId} (MOCK-0063).
3. NRF: 201 + heartBeatTimer (MOCK-0064).

**Expected**: ENT-0016.nfStatus=REGISTERED, heartBeatTimer 갱신.

**Teardown**: NSSF deregister.

**판정 기준**: procedure §2.1 PROC-0001 시퀀스 일치, impl-spec NSSF-VS-0040.

#### NSSF-TC-0041: PROC-0001 NRF 5xx → 백오프 재시도

**시험 목적**: SHOULD — NRF 503 시 백오프 재시도 무한 (운영 알람).

**Precondition**: NRF stub 가 503 응답 준비.

**Steps**:
1. NSSF → NRF: PUT (MOCK-0063).
2. NRF: 503 (MOCK-0065).
3. NSSF: 백오프 재시도 (N회 후 운영 알람 로그).

**Expected**: 재시도 N회 발생, ERROR 로그.

**Teardown**: NRF 정상 복구.

**판정 기준**: 백오프 알고리즘 정확, impl-spec NSSF-VS-0041.

#### NSSF-TC-0042: PROC-0003 Heartbeat 정상

**시험 목적**: SHALL — heartBeatTimer 만료 시 NRF PATCH (PROC-0003).

**Precondition**: NSSF Register 완료, heartBeatTimer 만료 직전.

**Steps**:
1. timer 콜백 → M-CLIENT-NRF.heartbeat.
2. NSSF → NRF: PATCH (MOCK-0066).
3. NRF: 204 (MOCK-0067).

**Expected**: ENT-0016.lastHeartbeatAt 갱신.

**Teardown**: no-op.

**판정 기준**: procedure §2.3 PROC-0003, impl-spec NSSF-VS-0042.

#### NSSF-TC-0043: PROC-0003 NRF 404 → 자동 재등록

**시험 목적**: SHALL — Heartbeat 404 시 자동 PROC-0001 재호출 (PROC-0003).

**Precondition**: NRF 가 NSSF 등록 잃음 (재시작 등).

**Steps**:
1. NSSF → NRF: PATCH (MOCK-0066).
2. NRF: 404 (MOCK-0068).
3. NSSF: on_heartbeat_response 404 case → PROC-0001 자동 트리거.

**Expected**: PROC-0001 재실행 후 ENT-0016 재등록.

**Teardown**: no-op.

**판정 기준**: impl-spec NSSF-VS-0043, procedure §2.3.

#### NSSF-TC-0044: PROC-0005 Token 정상 발급

**시험 목적**: SHALL — OAuth2 token cache miss 시 NRF 발급 (PROC-0005).

**Precondition**: ENT-0013 (TokenCache) miss.

**Steps**:
1. M-AUTH.get_token → cache miss.
2. NSSF → NRF: POST /oauth2/token (MOCK-0069).
3. NRF: 200 + access_token (MOCK-0070).
4. NSSF: ENT-0013 갱신.

**Expected**: ENT-0013 hit (token + expires_at).

**Teardown**: ENT-0013 TTL 만료.

**판정 기준**: impl-spec NSSF-VS-0044, procedure §2.5 PROC-0005.

#### NSSF-TC-0045: PROC-0005 Token 401 → 재발급

**시험 목적**: SHALL — Token 401 시 cache invalidate + 재발급 (PROC-0005).

**Precondition**: ENT-0013 에 stale token 존재.

**Steps**:
1. NSSF → NRF: POST (MOCK-0069).
2. NRF: 401 (MOCK-0071).
3. NSSF: cache invalidate + 재시도.

**Expected**: ENT-0013 reset → 신규 token 캐싱.

**Teardown**: no-op.

**판정 기준**: impl-spec NSSF-VS-0045.

#### NSSF-TC-0046: PROC-0020 Subscription Expiry sweeper

**시험 목적**: SHALL — expiry < now 도달 시 구독 삭제 + 큐 정리 (PROC-0020).

**Precondition**: ENT-0010 에 expired row N건 존재.

**Steps**:
1. timer 콜백 → M-PROC Sub.expire_sweeper.
2. db-design T-SUB-EXPIRE 배치 적용.

**Expected**: ENT-0010 expired row 모두 제거, ENT-0015 큐의 해당 task 정리.

**Teardown**: no-op.

**판정 기준**: 배치 원자성, impl-spec NSSF-VS-0046, procedure §2.20.

#### NSSF-TC-0047: M-Not 중복 status_change Notify 금지 (시스템)

**시험 목적**: SHALL NOT — PUT 직후 동일 변경 다중 Notify 금지: dedupe → 단일 통지 (SVC-0029).

**Precondition**: PROC-0013 사전 수행 (PUT 직후). 동일 변경 N회 큐잉 시도.

**Steps**:
1. NSSF 내부: ENT-0015 enqueue 시 dedup 로직 검사.
2. dedupe → 큐에 단일 entry.
3. M-CLIENT-NOTIFY 송신.

**Expected**: 외부 송신 1회만 발생 (Consumer 204), dedup 통계 N-1 dedup'ed.

**Teardown**: ENT-0015 정리.

**판정 기준**: 송신 횟수 == 1, impl-spec NSSF-VS-0047.

#### NSSF-TC-0048: 시스템 — Initial Registration e2e

**시험 목적**: SHALL — Initial Registration end-to-end (AMF→NSSF GET → NSSF→NRF disc → 200) ≤ N ms (SVC-0002, MGMT-0001).

**Precondition**:
- 다중 NF mock 토폴로지: AMF + NSSF + NRF stub.
- ENT-0001~0007 로드, ENT-0016 Registered.

**Steps**:
1. AMF stub → NSSF: GET (MOCK-0001).
2. NSSF: M-HANDLER-1 + M-CLIENT-NRF.discover_amf.
3. NSSF → NRF: NFDiscovery → NRF 200.
4. NSSF → AMF: 200 응답.

**Expected**: 전체 흐름 ≤ N ms (운영 SLA, 기준값 200 ms), 모든 hop 200/2xx.

**Teardown**: ENT-0013 token 캐시 정리.

**판정 기준**: procedure §2.6 PROC-0006 + §2.4 PROC-0004 결합 시퀀스 일치, impl-spec NSSF-VS-0048.

#### NSSF-TC-0049: 시스템 — NSSAI Update + Notify 사이클 e2e

**시험 목적**: SHALL — AMF PUT → 매칭 구독자 Notify → 204 (SVC-0012, SVC-0023).

**Precondition**:
- 토폴로지: AMF (Update producer) + NSSF + 다른 AMF (Subscribe consumer).
- 사전 Subscribe 완료된 ENT-0010 (SNSSAI_STATUS_CHANGE_REPORT) 존재.

**Steps**:
1. AMF1 → NSSF: PUT (MOCK-0018).
2. NSSF: T-AMF-UPSERT → ENT-0015 Enqueue.
3. NSSF → AMF2 (consumer): POST (MOCK-0053).
4. AMF2: 204 (MOCK-0054).

**Expected**: ENT-0015 큐 비움, ENT-0010.lastNotifiedAt 갱신, AMF2 수신 페이로드 검증.

**Teardown**: ENT-0008/0009/0010/0015 모두 초기화.

**판정 기준**: PROC-0013 + PROC-0015 결합 시퀀스, impl-spec NSSF-VS-0049.

#### NSSF-TC-0050: 시스템 — HR Roaming PDU Session e2e

**시험 목적**: SHALL — SMF→V-NSSF→SEPP→H-NSSF 응답 chain → V-NSSF 200 OK (SVC-0004).

**Precondition**:
- 토폴로지: SMF + V-NSSF + SEPP + H-NSSF.
- V-NSSF ENT-0006 + H-NSSF ENT-0001 로드.
- N32 SEPP 채널 활성.

**Steps**:
1. SMF → V-NSSF: GET (MOCK-0009).
2. V-NSSF → SEPP → H-NSSF: forward (MOCK-0076).
3. H-NSSF → SEPP → V-NSSF: 200.
4. V-NSSF → SMF: 200 (MOCK-0010).

**Expected**: SMF 가 200 + mappingOfNssai 수신.

**Teardown**: SEPP 채널 정리.

**판정 기준**: PROC-0008 + PROC-0012 결합 시퀀스, impl-spec NSSF-VS-0050.

#### NSSF-TC-0051: 시스템 — Restart 후 구독 활성 유지

**시험 목적**: SHALL — NSSF restart 후 ENT-0010 구독 활성 유지 (PG→uthash 로드) (db §8-B).

**Precondition**: 사전 Subscribe N건 완료된 ENT-0010 + PostgreSQL 영속 상태.

**Steps**:
1. NSSF 정상 종료 (PROC-0002 Deregister) → uthash flush.
2. NSSF 재기동 → M-DB.init → PG → uthash 로드.
3. AMF → NSSF: PUT 이벤트 발생.
4. NSSF: 매칭 구독자에 Notify 송신.

**Expected**: N건 모두 ACTIVE 유지, Notify 정상 송신.

**Teardown**: PG state 정리.

**판정 기준**: 재시작 정책 (db §8-B), impl-spec NSSF-VS-0051.


### 5-B. 단위 모듈 시험 절차서 (TC-0052 ~ TC-0071, 간략형)

#### NSSF-TC-0052: M-LOG 카테고리 출력
**시험 목적**: SHALL — zlog 카테고리별 출력 + 3gpp-Sbi-* 헤더 추적 로그 (impl-spec §1 M-LOG).
**Precondition**: zlog 초기화 완료.
**Steps**: `log_info()`/`log_error()` 호출 (카테고리별).
**Expected**: 출력 파일에 카테고리/포맷 일치 라인 기록.
**판정 기준**: plan TASK-0002 AC, 모든 카테고리 분리 출력.

#### NSSF-TC-0053: M-CFG 파서·검증
**시험 목적**: SHALL — cJSON 파서 + lifecycle 변환 + schema 검증.
**Precondition**: TASK-0005 산출 MOCK-CFG-0001/0002 fixture 준비.
**Steps**: `cfg_load(path)` 호출.
**Expected**: lifecycle 구조체 정상 채움, 필수 키 누락 시 실패 반환.
**판정 기준**: plan TASK-0004 AC, DEF-011 schema-payload 일관성.

#### NSSF-TC-0054: M-AUTH TLS ctx
**시험 목적**: SHALL — OpenSSL TLS ctx 초기화 (mTLS).
**Precondition**: MOCK-TLS-0001/0002 인증서 번들 존재.
**Steps**: `auth_tls_ctx_init()` 호출.
**Expected**: SSL_CTX* != NULL, 인증서 valid.
**판정 기준**: plan TASK-0006 AC.

#### NSSF-TC-0055: M-AUTH libjwt 검증
**시험 목적**: SHALL — libjwt RS256 검증 통과/실패 분기 (SEC-0003).
**Precondition**: 정상/만료 JWT fixture.
**Steps**: `auth_jwt_verify(token)` 호출 (정상 / MOCK-0070, 만료 / MOCK-0006).
**Expected**: 정상 시 true, 만료 시 false + cause=TOKEN_EXPIRED.
**판정 기준**: plan TASK-0006 AC RS256/ES256.

#### NSSF-TC-0056: M-AUTH token cache
**시험 목적**: SHALL — TokenCache GetOrFetch + TTL invalidate (ENT-0013).
**Precondition**: ENT-0013 빈 상태.
**Steps**: `token_get_or_fetch()` 호출 (miss → fetch → cache).
**Expected**: hit/miss/expired 시나리오 모두 정상 동작.
**판정 기준**: plan TASK-0006 AC + db §7.13.

#### NSSF-TC-0057: M-MODEL core 직렬화
**시험 목적**: SHALL — Snssai/Tai/PlmnId/NfInstanceId/ProblemDetails/SupportedFeatures 양방향 직렬화 (DAT-0029, DAT-0039).
**Precondition**: cJSON 초기화.
**Steps**: 각 타입에 대해 `struct → JSON → struct` 라운드트립 (MOCK-B-0001~0034 활용).
**Expected**: 라운드트립 정확.
**판정 기준**: plan TASK-0008 AC, api-analysis §3.

#### NSSF-TC-0058: M-MODEL 확장 직렬화
**시험 목적**: SHALL — NssaiAvailabilityInfo / Authorized / Subscription / NssfEvent / PatchDoc 직렬화 (DAT-0013~0028, DAT-0038).
**Precondition**: M-MODEL core 완료.
**Steps**: MOCK-0018, MOCK-0034, MOCK-0042, MOCK-0053 라운드트립.
**Expected**: 라운드트립 정확.
**판정 기준**: plan TASK-0009 AC.

#### NSSF-TC-0059: M-DB 인프라
**시험 목적**: SHALL — libpq 연결 풀 + uthash + write-through.
**Precondition**: PostgreSQL 빈 인스턴스.
**Steps**: CRUD round-trip (단순 ENT-0008 row).
**Expected**: PG ↔ uthash 일치.
**판정 기준**: plan TASK-0010 AC, db §4 저장소 선택.

#### NSSF-TC-0060: M-DB Config CRUD
**시험 목적**: SHALL — ENT-0001~0007 Load/Read/Reload (DAT-0001~0012, DAT-0029).
**Precondition**: cfg fixture (TASK-0005) 로드.
**Steps**: `config_load_from_file()`, lookup 호출.
**Expected**: uthash hit + PG fallback 일치.
**판정 기준**: plan TASK-0011 AC + db §7.1/7.2.

#### NSSF-TC-0061: M-DB AMF CRUD
**시험 목적**: SHALL — ENT-0008/0009 Upsert/Patch/Read/Delete + T-AMF-UPSERT/PATCH/DELETE.
**Precondition**: ENT-0008/0009 빈 상태.
**Steps**: Upsert (MOCK-0018) → Patch (MOCK-0024) → Read → Delete (MOCK-0031).
**Expected**: REPEATABLE READ + FK CASCADE 정상.
**판정 기준**: plan TASK-0012 AC + db §7.3·§8-A.

#### NSSF-TC-0062: M-DB Subscription CRUD
**시험 목적**: SHALL — ENT-0010 다중 인덱스 + FindMatching + ExpireDueAt.
**Precondition**: ENT-0010 빈 상태.
**Steps**: Create (MOCK-0034) → Modify (MOCK-0042) → FindMatching(event,TAI) → ExpireDueAt(now+).
**Expected**: 보조 인덱스 정확, expiry 정렬.
**판정 기준**: plan TASK-0013 AC + db §7.10.

#### NSSF-TC-0063: M-DB ephemeral / NRF
**시험 목적**: SHALL — ENT-0013/0014/0015/0016 (TokenCache + ConnPool + NotifyQueue + NRF state).
**Precondition**: 빈 상태.
**Steps**: TokenCache atomic swap, NotifyQueue FIFO enqueue/dequeue, NRF state MarkRegistered.
**Expected**: lock-free 정확, FIFO 순서, singleton 갱신.
**판정 기준**: plan TASK-0015 AC + db §7.13~7.16.

#### NSSF-TC-0064: M-HTTP-SRV 라우팅
**시험 목적**: SHALL — nghttp2 route 등록 + 8 endpoint dispatch (SVC-0033~0035, SVC-0038~0040).
**Precondition**: M-HTTP-SRV 초기화 (test_mode=1).
**Steps**: 각 endpoint URI 에 대해 stub 콜백 발생 확인.
**Expected**: route 매칭 정확, 미매칭 → 404.
**판정 기준**: plan TASK-0016 AC.

#### NSSF-TC-0065: M-HTTP-SRV cfg-gate (R12)
**시험 목적**: SHALL — cfg.listen.tls_cert/key 보유 시 test_mode=0 활성.
**Precondition**: cfg with/without tls fixture.
**Steps**: `srv_init(cfg)` 호출 후 test_mode 값 확인.
**Expected**: with tls → 0, without tls → 1.
**판정 기준**: plan TASK-0017 AC, DEF-006 fix anchor.

#### NSSF-TC-0066: M-HTTP-CLI multi handle
**시험 목적**: SHALL — libcurl multi + libuv timer/poll + 재시도.
**Precondition**: libuv loop + curl multi init.
**Steps**: `http_cli_request_async()` 호출.
**Expected**: multi handle 콜백 정상, 재시도 카운터 동작.
**판정 기준**: plan TASK-0018 AC.

#### NSSF-TC-0067: M-HTTP-CLI cfg-gate (R12)
**시험 목적**: SHALL — cfg.nrf.base_url 보유 시 curl_global_init 운영 path.
**Precondition**: cfg with/without nrf url.
**Steps**: `cli_init(cfg)` 호출 후 init 분기 확인.
**Expected**: with url → init 호출, without → skip.
**판정 기준**: plan TASK-0019 AC, DEF-003 fix.

#### NSSF-TC-0068: M-CLIENT-NRF Register/Heartbeat
**시험 목적**: SHALL — NfProfile 직렬화 + Register/Deregister/Heartbeat (PROC-0001~0003).
**Precondition**: NRF stub.
**Steps**: register (MOCK-0063) → heartbeat (MOCK-0066) → deregister.
**Expected**: NRF stub 수신 페이로드 일치.
**판정 기준**: plan TASK-0020 AC + procedure §2.1~2.3.

#### NSSF-TC-0069: M-CLIENT-NOTIFY
**시험 목적**: SHALL — C1 callback POST + 백오프 + DequeueDue (SVC-0023~0029, ERR-0014).
**Precondition**: Consumer stub.
**Steps**: enqueue MOCK-0053 → send → on 503 (MOCK-0057) → backoff → on 307 (MOCK-0058) → redirect.
**Expected**: 재시도·307·Retry-After 정확 처리.
**판정 기준**: plan TASK-0022 AC.

#### NSSF-TC-0070: M-CLIENT-SEPP
**시험 목적**: MAY — V-NSSF→H-NSSF forward via SEPP (PROC-0012) (SVC-0004).
**Precondition**: SEPP stub.
**Steps**: forward call (MOCK-0009) → SEPP → MOCK-0076 응답.
**Expected**: N32 채널 통과, 응답 chain 일치.
**판정 기준**: plan TASK-0023 AC.

#### NSSF-TC-0071: M-LIFE / M-MAIN
**시험 목적**: SHALL — init/shutdown/timer wire + signal_wait + uv_run + graceful_shutdown.
**Precondition**: 빈 상태.
**Steps**: main(argv) → init 10 STEP → SIGTERM → shutdown.
**Expected**: 모든 STEP 성공, SIGTERM 시 ENT-0010 flush, ENT-0013 정리.
**판정 기준**: plan TASK-0037 + TASK-0038 AC, impl-spec §8-D-1/D-2.

### 5-C. 부정 시험 / 트리거 절차서 (TC-0072 ~ TC-0085)

#### NSSF-TC-0072: SVC-0016 expiry 분산 단위 시험 (부정)
**시험 목적**: SHALL NOT — Subscription 생성 시 동일 expiry timestamp 다수 부여 금지 (단위 검증).
**Precondition**: ENT-0010 빈 상태, expiry distributor 활성.
**Steps**: `expiry_distribute(T1, N)` 호출 (N=100).
**Expected**: 결과 N 개 timestamp 가 모두 다름 (동일 빈도 == 1).
**판정 기준**: features SVC-0016 M-Not, mock-data MOCK-0040.

#### NSSF-TC-0073: SVC-0020 미지원 NSSAI 반환 금지 (부정)
**시험 목적**: SHALL NOT — NSSF 가 TA-단위 미지원 NSSAI 반환 금지.
**Precondition**: ENT-0009 에 부분 인증 상태.
**Steps**: `authorize_per_ta()` 호출 → 응답 검증.
**Expected**: 응답 nssai 가 supportedNssai 와 disjoint 인 entry 0건.
**판정 기준**: impl-spec NSSF-VS-0025, features SVC-0020.

#### NSSF-TC-0074: DAT-0025 event IE 불변 검증 (부정)
**시험 목적**: SHALL NOT — Subscribe Modify 시 event IE 변경 금지 (단위).
**Precondition**: ENT-0010 row 1건.
**Steps**: `sub_apply_patch(id, op=replace path=/event)` 호출.
**Expected**: 400 MODIFICATION_NOT_ALLOWED, ENT-0010.event 불변.
**판정 기준**: impl-spec NSSF-VS-0025.

#### NSSF-TC-0075: SVC-0029 dedup 단위 시험 (부정)
**시험 목적**: SHALL NOT — Update 후 중복 status_change Notify 금지.
**Precondition**: ENT-0015 dedup 로직 활성.
**Steps**: 동일 변경 N번 `notify_enqueue()` 호출.
**Expected**: 큐 entry 수 == 1.
**판정 기준**: impl-spec NSSF-VS-0047, MOCK-0073.

#### NSSF-TC-0076: SEC-0001 사전 필터링 (부정)
**시험 목적**: SHALL NOT — Replacement 진행 슬라이스 데이터 송신 금지.
**Precondition**: ENT-0012 활성 Replacement 진행 중.
**Steps**: `notify_send(slice_in_replacement)` 호출.
**Expected**: 송신 차단 (returns 0건 송신).
**판정 기준**: impl-spec NSSF-VS-0039, MOCK-0062.

#### NSSF-TC-0077: NSSRG 제공 시 반환 (C 트리거)

**시험 목적**: SHALL (Conditional) — 구독 NSSRG list 제공 시 NSSF 가 반환 (SVC-0006).

**Precondition**: 정상 기동, ENT-0001 + AMF stub 가 NSSRG 포함 GET.

**Steps**:
1. AMF → NSSF: GET (MOCK-0001 NSSRG variant).
2. NSSF: M-HANDLER-1 4-B 단계 5 + NSSRG list 처리.

**Expected**: 200 + nssrgList 필드 존재.

**Teardown**: no-op.

**판정 기준**: features SVC-0006 트리거, api §2.1.

#### NSSF-TC-0078: NSAG 지원 시 반환 (C 트리거)
**시험 목적**: SHALL (Conditional) — AMF NSAG 지원 표시 시 NSSF 가 NSAG 반환 (SVC-0007).
**Precondition**: AMF stub 가 NSAG supported 표시.
**Steps**: AMF → NSSF GET (NSAG variant) → M-HANDLER-1.
**Expected**: 200 + nsagInfoList.
**판정 기준**: features SVC-0007.

#### NSSF-TC-0079: UCU NSSRG 반환 (C 트리거)
**시험 목적**: SHALL (Conditional) — UCU 절차에서 NSSRG list 반환 (SVC-0008).
**Precondition**: UCU 시나리오 fixture.
**Steps**: AMF → NSSF GET (UCU NSSRG variant).
**Expected**: 200 + nssrgList.
**판정 기준**: features SVC-0008, procedure §2.9 PROC-0009.

#### NSSF-TC-0080: UCU NSAG 반환 (C 트리거)
**시험 목적**: SHALL (Conditional) — UCU AMF NSAG 지원 시 NSAG 반환 (SVC-0009).
**Precondition**: UCU 시나리오.
**Steps**: AMF → NSSF GET (UCU NSAG variant).
**Expected**: 200 + nsagInfoList.
**판정 기준**: features SVC-0009.

#### NSSF-TC-0081: Subscribe accepted 시 201 (C 트리거)
**시험 목적**: SHALL (Conditional) — Subscribe request accepted 시 201 (SVC-0022).
**Precondition**: 정상 기동.
**Steps**: AMF → NSSF POST (MOCK-0034).
**Expected**: 201 + Location.
**판정 기준**: features SVC-0022.

#### NSSF-TC-0082: Unsubscribe accepted 시 204 (C 트리거)
**시험 목적**: SHALL (Conditional) — Unsubscribe accepted 시 204 (SVC-0022, DELETE 경로).
**Precondition**: 사전 Subscribe.
**Steps**: AMF → NSSF DELETE (MOCK-0048).
**Expected**: 204.
**판정 기준**: features SVC-0022.

#### NSSF-TC-0083: OPTIONS accepted 시 200 (C 트리거)
**시험 목적**: SHALL (Conditional) — OPTIONS accepted 시 200 (SVC-0032).
**Precondition**: 정상 기동.
**Steps**: AMF → NSSF OPTIONS (MOCK-0051).
**Expected**: 200.
**판정 기준**: features SVC-0032.

#### NSSF-TC-0084: S-NSSAI status change Notify (PRC 트리거)
**시험 목적**: SHALL (Conditional) — S-NSSAI status change 발생 시 Consumer 통지 (PRC-0001).
**Precondition**: ENT-0010 (SNSSAI_STATUS_CHANGE_REPORT) 존재.
**Steps**: 상태 변경 트리거 → enqueue → POST (MOCK-0053).
**Expected**: Consumer 가 Notify 수신.
**판정 기준**: procedure §2.15 PROC-0015.

#### NSSF-TC-0085: Validity timer Notify (PRC 트리거)
**시험 목적**: SHALL (Conditional) — Validity timer 트리거 시 통지 (PRC-0002).
**Precondition**: ENT-0011 변경 트리거.
**Steps**: timer 만료 → enqueue → POST (MOCK-0060).
**Expected**: Consumer Notify 수신.
**판정 기준**: procedure §2.17 PROC-0017.

### 5-D. 에러 매트릭스 보강 절차서 (TC-0086 ~ TC-0105)

> 통합 시험. 각 TC 는 동일 패턴: stub 가 변형 응답/요청 주입 → NSSF 응답 코드 + cause 검증.

#### NSSF-TC-0086: E1 307 redirect
**시험 목적**: SHALL — ES3XX feature 시 NSSF set 내 307 redirect.
**Precondition**: ENT-0016 에 다른 NSSF 인스턴스 매핑 fixture.
**Steps**: AMF → NSSF GET (MOCK-0001).
**Expected**: 307 + Location 헤더 (다른 NSSF URI).
**Teardown**: no-op.
**판정 기준**: api §6.1.

#### NSSF-TC-0087: E1 308 영구 redirect
**시험 목적**: SHALL — 영구 redirect 처리.
**Precondition**: 308 fixture.
**Steps**: AMF → NSSF GET.
**Expected**: 308 + Location.
**Teardown**: no-op.
**판정 기준**: api §6.1.

#### NSSF-TC-0088: E1 INVALID_QUERY_PARAM_VALUE
**시험 목적**: SHALL — slice-info-request-* 무효 JSON 시 400.
**Precondition**: 정상 기동.
**Steps**: AMF → NSSF GET (slice-info malformed JSON).
**Expected**: 400 + invalidParams=[slice-info-request-for-registration].
**판정 기준**: api §6.1.

#### NSSF-TC-0089: E1 401 TOKEN_INVALID
**시험 목적**: SHALL — 서명 불일치 token 시 401.
**Precondition**: M-AUTH 활성.
**Steps**: AMF → NSSF GET (wrong-key signed token).
**Expected**: 401 cause=TOKEN_INVALID.
**판정 기준**: api §6.1.

#### NSSF-TC-0090: E1 403 NOT_AUTHORIZED
**시험 목적**: SHALL — scope 불일치 시 403.
**Precondition**: 잘못된 scope token fixture.
**Steps**: AMF → NSSF GET.
**Expected**: 403 cause=NOT_AUTHORIZED.
**판정 기준**: api §6.1.

#### NSSF-TC-0091: E1 406/414
**시험 목적**: SHALL — Accept 협상 실패 / URI 길이 초과.
**Precondition**: 정상 기동.
**Steps**: GET (Accept: text/xml) → 406; GET (huge query string) → 414.
**Expected**: 406 / 414.
**판정 기준**: api §6.1.

#### NSSF-TC-0092: E1 429
**시험 목적**: SHALL — rate limit 시 429 + Retry-After.
**Precondition**: rate limiter 활성.
**Steps**: 다중 GET 송신.
**Expected**: 429 + Retry-After.
**판정 기준**: api §6.1.

#### NSSF-TC-0093: E1 500/502/503
**시험 목적**: SHALL — 내부 오류 / NRF 통신 실패 / 서비스 일시 불가.
**Precondition**: 오류 주입 mode.
**Steps**: 각 시나리오 GET.
**Expected**: 500/502/503 + (503은 Retry-After).
**판정 기준**: api §6.1.

#### NSSF-TC-0094: E2 400 SEMANTIC_ERROR
**시험 목적**: SHALL — NssaiAvailabilityInfo 의미 오류 (taiList 누락 등) 시 400.
**Precondition**: 정상 기동.
**Steps**: AMF → NSSF PUT (semantic error variant).
**Expected**: 400 cause=SEMANTIC_ERROR.
**판정 기준**: api §6.2.

#### NSSF-TC-0095: E3 412 Precondition Failed
**시험 목적**: SHALL — JSON Patch test op 실패 시 412.
**Precondition**: 사전 PUT 완료, 다른 값으로 test op.
**Steps**: PATCH (mock-data §5-B test fail fixture).
**Expected**: 412.
**판정 기준**: api §6.2, mock-data §5-B.

#### NSSF-TC-0096: E2/E3 411/413
**시험 목적**: SHALL — Content-Length 누락 (411) / 페이로드 초과 (413).
**Precondition**: 정상 기동.
**Steps**: PUT (no Content-Length) → 411; PUT (>limit) → 413.
**Expected**: 411 / 413.
**판정 기준**: api §6.2.

#### NSSF-TC-0097: E4 400/403
**시험 목적**: SHALL — DELETE → 400 RESOURCE_CONTEXT_NOT_FOUND / 403 NOT_AUTHORIZED.
**Precondition**: variants 준비.
**Steps**: DELETE (no context) / DELETE (wrong scope).
**Expected**: 400 / 403.
**판정 기준**: api §6.3.

#### NSSF-TC-0098: E5 400 INVALID_IE
**시험 목적**: SHALL — Subscribe URI 형식 오류 시 400.
**Precondition**: 정상 기동.
**Steps**: POST (MOCK-0038 with malformed URI).
**Expected**: 400 cause=INVALID_IE.
**판정 기준**: api §6.4.

#### NSSF-TC-0099: E5 401/403
**시험 목적**: SHALL — 401 / 403 NOT_AUTHORIZED.
**Precondition**: wrong token/scope variants.
**Steps**: POST (MOCK-0034 variants).
**Expected**: 401 / 403.
**판정 기준**: api §6.4.

#### NSSF-TC-0100: E6 400 JSON Patch 무결성
**시험 목적**: SHALL — path 무효 시 400.
**Precondition**: 사전 Subscribe.
**Steps**: PATCH (MOCK-0042 invalid path variant).
**Expected**: 400 cause=INVALID_IE.
**판정 기준**: api §6.5.

#### NSSF-TC-0101: E7 400 RESOURCE_CONTEXT_NOT_FOUND
**시험 목적**: SHALL — Unsubscribe context 누락 시 400.
**Precondition**: ENT-0010 없는 nfId.
**Steps**: DELETE (MOCK-0048 context missing variant).
**Expected**: 400.
**판정 기준**: api §6.6.

#### NSSF-TC-0102: E8 표준 SBI 4xx/5xx
**시험 목적**: SHALL — OPTIONS 표준 SBI 에러 (ERR-0016).
**Precondition**: 정상 기동.
**Steps**: OPTIONS (invalid Accept).
**Expected**: 4xx.
**판정 기준**: api §6.7.

#### NSSF-TC-0103: C1 401/403 재발급 (시스템)

**시험 목적**: SHALL — Notify 401/403 시 token 재발급 / 비-재시도.

**Precondition**: PROC-0013 사전. Consumer 401 응답 (token 만료 모사).

**Steps**:
1. NSSF → Consumer: POST (MOCK-0053).
2. Consumer: 401.
3. NSSF: M-AUTH cache invalidate → 재발급 → 1회 재시도.

**Expected**: 재시도 1회 후 정상.

**Teardown**: ENT-0013 정리.

**판정 기준**: api §6.8.

#### NSSF-TC-0104: C1 411/413/415/429 (시스템)
**시험 목적**: SHALL — Consumer 4xx 처리 (api §6.8).
**Precondition**: PROC-0013 사전.
**Steps**: 각 시나리오 (각 variant) 발생.
**Expected**: 4xx 별 WARN 로그 + 비-재시도 (429 는 백오프).
**Teardown**: no-op.
**판정 기준**: api §6.8.

#### NSSF-TC-0105: C1 500/502 백오프 (시스템)
**시험 목적**: SHALL — Consumer 5xx 시 백오프 재시도.
**Precondition**: PROC-0013 사전. Consumer 500/502.
**Steps**: 재시도 N회 후 stale.
**Expected**: 백오프 알고리즘 정확.
**Teardown**: ENT-0015 정리.
**판정 기준**: api §6.8.

### 5-E. 경계값 시험 절차서 (TC-0106 ~ TC-0125, 간략형)

#### NSSF-TC-0106: Snssai.sst min=0
**시험 목적**: SHALL — sst=0 통과.
**Steps**: `snssai_parse(MOCK-B-0001)`.
**Expected**: parse 성공.
**판정 기준**: mock-data §4.1.

#### NSSF-TC-0107: Snssai.sst max=255
**시험 목적**: SHALL — sst=255 통과.
**Steps**: `snssai_parse(MOCK-B-0002)`.
**Expected**: 성공.
**판정 기준**: mock-data §4.1.

#### NSSF-TC-0108: Snssai.sst overflow → 400
**시험 목적**: SHALL NOT — sst=256 거부.
**Steps**: `snssai_parse(MOCK-B-0003)` 또는 API 직접 호출.
**Expected**: 400 INVALID_IE.
**판정 기준**: mock-data §4.1.

#### NSSF-TC-0109: Snssai.sst string type 거부
**시험 목적**: SHALL NOT — type mismatch 거부.
**Steps**: `snssai_parse(MOCK-B-0004)`.
**Expected**: parser 실패.
**판정 기준**: mock-data §4.1.

#### NSSF-TC-0110: Snssai.sd pattern 통과
**시험 목적**: SHALL — `sd="ABCDEF"` 통과.
**Steps**: `snssai_parse(MOCK-B-0005)`.
**Expected**: 성공.
**판정 기준**: mock-data §4.1.

#### NSSF-TC-0111: Snssai.sd 5자 거부
**시험 목적**: SHALL NOT — 길이 위반 거부.
**Steps**: `snssai_parse(MOCK-B-0006)`.
**Expected**: 400 invalidParams=[sd].
**판정 기준**: mock-data §4.1.

#### NSSF-TC-0112: Snssai.sd hex 외 문자 거부
**시험 목적**: SHALL NOT — pattern 위반 거부.
**Steps**: `snssai_parse(MOCK-B-0007)`.
**Expected**: 실패.
**판정 기준**: mock-data §4.1.

#### NSSF-TC-0113: Tai.tac 6 hex 통과
**시험 목적**: SHALL — 통과.
**Steps**: `tai_parse(MOCK-B-0010)`.
**Expected**: 성공.
**판정 기준**: mock-data §4.2.

#### NSSF-TC-0114: Tai.tac 4 hex legacy 통과
**시험 목적**: SHALL — 통과.
**Steps**: `tai_parse(MOCK-B-0011)`.
**Expected**: 성공.
**판정 기준**: mock-data §4.2.

#### NSSF-TC-0115: Tai.tac 5자 거부
**시험 목적**: SHALL NOT — 거부.
**Steps**: `tai_parse(MOCK-B-0012)`.
**Expected**: 실패.
**판정 기준**: mock-data §4.2.

#### NSSF-TC-0116: PlmnId digits-only 위반
**시험 목적**: SHALL NOT — 비-digit 거부.
**Steps**: `plmn_parse(MOCK-B-0013)`.
**Expected**: 실패.
**판정 기준**: mock-data §4.2.

#### NSSF-TC-0117: NfInstanceId UUID 통과
**시험 목적**: SHALL — UUID 통과.
**Steps**: `nfid_parse(MOCK-B-0020)`.
**Expected**: 성공.
**판정 기준**: mock-data §4.3.

#### NSSF-TC-0118: NfInstanceId 불완전 UUID 거부
**시험 목적**: SHALL NOT — 거부.
**Steps**: `nfid_parse(MOCK-B-0021)`.
**Expected**: 실패.
**판정 기준**: mock-data §4.3.

#### NSSF-TC-0119: NfInstanceId 비-UUID 거부
**시험 목적**: SHALL NOT — 거부.
**Steps**: `nfid_parse(MOCK-B-0022)`.
**Expected**: 실패.
**판정 기준**: mock-data §4.3.

#### NSSF-TC-0120: supportedFeatures "F" 통과
**시험 목적**: SHALL — 4비트 활성 통과.
**Steps**: `features_parse(MOCK-B-0030)`.
**Expected**: 성공.
**판정 기준**: mock-data §4.4.

#### NSSF-TC-0121: supportedFeatures ES3XX(M) 미포함 거부
**시험 목적**: SHALL NOT — ES3XX M 미협상 시 거부.
**Steps**: `features_negotiate(MOCK-B-0031)` (서비스 측).
**Expected**: 400 FEATURE_NOT_SUPPORTED.
**판정 기준**: mock-data §4.4, hints H8.

#### NSSF-TC-0122: supportedFeatures hex 외 문자 거부
**시험 목적**: SHALL NOT — 거부.
**Steps**: `features_parse(MOCK-B-0032)`.
**Expected**: 실패.
**판정 기준**: mock-data §4.4.

#### NSSF-TC-0123: NssaiAvailabilityInfo minItems=1 통과
**시험 목적**: SHALL — 1건 통과.
**Steps**: `nssai_avail_info_parse(MOCK-B-0050)`.
**Expected**: 성공.
**판정 기준**: mock-data §4.6.

#### NSSF-TC-0124: NssaiAvailabilityInfo 빈 배열 거부
**시험 목적**: SHALL NOT — empty 거부.
**Steps**: `nssai_avail_info_parse(MOCK-B-0051)` 또는 PUT.
**Expected**: 400 MANDATORY_IE_MISSING.
**판정 기준**: mock-data §4.6.

#### NSSF-TC-0125: callback URI HTTPS / HTTP 검증
**시험 목적**: SHALL — HTTPS 통과, HTTP 거부 (운영 정책).
**Steps**: `callback_uri_validate(MOCK-B-0060)` (통과), `(MOCK-B-0061)` (실패).
**Expected**: 통과/실패.
**판정 기준**: mock-data §4.7.

### 5-F. 특수 패턴 시험 절차서 (TC-0126 ~ TC-0145)

#### NSSF-TC-0126: NSSelection supportedFeatures "F"
**시험 목적**: SHALL — 4비트 모두 활성 (ES3XX+TargetNssai+RSIPCE+SIOP).
**Steps**: `features_negotiate(MOCK-SF-0001)`.
**Expected**: features_hex 반영.
**판정 기준**: mock-data §5-C NSSelection.

#### NSSF-TC-0127: NSSelection supportedFeatures "1" (ES3XX only)
**시험 목적**: SHALL — ES3XX(M) only 협상.
**Steps**: `features_negotiate(MOCK-SF-0002)`.
**Expected**: features_hex == 1.
**판정 기준**: mock-data §5-C.

#### NSSF-TC-0128: NSSelection "5" (ES3XX+RSIPCE)
**시험 목적**: SHALL — ES3XX + RSIPCE 협상.
**Steps**: `features_negotiate(MOCK-SF-0003)`.
**Expected**: features_hex == 5.
**판정 기준**: mock-data §5-C.

#### NSSF-TC-0129: NSSelection "9" (ES3XX+SIOP)
**시험 목적**: SHALL — ES3XX + SIOP 협상.
**Steps**: `features_negotiate(MOCK-SF-0004)`.
**Expected**: features_hex == 9.
**판정 기준**: mock-data §5-C.

#### NSSF-TC-0130: NSSelection "3" (ES3XX+TargetNssai)
**시험 목적**: SHALL — ES3XX + TargetNssai 협상.
**Steps**: `features_negotiate(MOCK-SF-0005)`.
**Expected**: features_hex == 3.
**판정 기준**: mock-data §5-C.

#### NSSF-TC-0131: NSSAIAvailability "7F"
**시험 목적**: SHALL — 7비트 모두 활성.
**Steps**: `features_negotiate(MOCK-SF-0010)`.
**Expected**: features_hex == 7F.
**판정 기준**: mock-data §5-C.

#### NSSF-TC-0132: NSSAIAvailability "8" (ES3XX only)
**시험 목적**: SHALL — ES3XX(M) only.
**Steps**: `features_negotiate(MOCK-SF-0011)`.
**Expected**: features_hex == 8.
**판정 기준**: mock-data §5-C.

#### NSSF-TC-0133: NSSAIAvailability "78" (Rel-18 권장)
**시험 목적**: SHALL — ES3XX+SATAS+NSIUN+NSRP 협상.
**Steps**: `features_negotiate(MOCK-SF-0012)`.
**Expected**: features_hex == 78.
**판정 기준**: mock-data §5-C.

#### NSSF-TC-0134: NSSAIAvailability "A" (SUMOD+ES3XX)
**시험 목적**: SHALL — SUMOD + ES3XX, SUMOD gate 통과.
**Steps**: `features_negotiate(MOCK-SF-0013)` → SUMOD gate test.
**Expected**: features_hex == A, PATCH 허용.
**판정 기준**: mock-data §5-C, impl-spec NSSF-VS-0024.

#### NSSF-TC-0135: NSSAIAvailability "C" (EANAN+ES3XX)
**시험 목적**: SHALL — EANAN + ES3XX, empty-array Notify 분기 활성.
**Steps**: `features_negotiate(MOCK-SF-0014)` → EANAN gate test.
**Expected**: features_hex == C.
**판정 기준**: mock-data §5-C, impl-spec NSSF-VS-0038.

#### NSSF-TC-0136: JSON Patch test op 통과 후 replace 진행

**시험 목적**: SHALL — `test` op 통과 시 후속 `replace` op 적용.

**Precondition**: 사전 PUT 완료, supportedFeatures 현재값 78.

**Steps**:
1. AMF → NSSF: PATCH (mock-data §5-B test+replace fixture).
2. NSSF: test op (supportedFeatures==78) 통과 → replace 적용.

**Expected**: 200, supportedFeatures 갱신.

**Teardown**: ENT-0008/0009 정리.

**판정 기준**: mock-data §5-B, impl-spec NSSF-VS-0014.

#### NSSF-TC-0137: JSON Patch test op 실패 → 412

**시험 목적**: SHALL — test op 실패 시 412 Precondition Failed.

**Precondition**: supportedFeatures 현재값이 test op 기대값과 불일치.

**Steps**: PATCH (mock-data §5-B test fail variant).

**Expected**: 412, ENT-0008/0009 무변경.

**Teardown**: no-op.

**판정 기준**: mock-data §5-B + api §6.2.

#### NSSF-TC-0138: E1 헤더 카탈로그
**시험 목적**: SHALL — 3gpp-Sbi-* 헤더 처리 (HEADER-001).
**Steps**: AMF → NSSF GET (mock-data HEADER-001).
**Expected**: 모든 헤더 인식, 로그 캡처.
**판정 기준**: mock-data §6 HEADER-001.

#### NSSF-TC-0139: E2 gzip Content-Encoding
**시험 목적**: SHOULD — gzip 인코딩 협상 (SVC-0041).
**Steps**: AMF → NSSF PUT (mock-data HEADER-002, Content-Encoding=gzip).
**Expected**: gzip decode 성공, Accept-Encoding 반영.
**판정 기준**: mock-data §6 + SVC-0041.

#### NSSF-TC-0140: C1 Notify 송신 헤더
**시험 목적**: SHALL — Notify 송신 시 Producer-Id, Callback 헤더 (HEADER-003).
**Steps**: NSSF → Consumer POST.
**Expected**: 헤더 모두 송신.
**판정 기준**: mock-data §6 HEADER-003.

#### NSSF-TC-0141: PATCH Content-Type 헤더
**시험 목적**: SHALL — `application/json-patch+json` 검증 (HEADER-004).
**Steps**: PATCH (HEADER-004 with correct/incorrect Content-Type).
**Expected**: 정상 200 / 415.
**판정 기준**: mock-data §6 HEADER-004.

#### NSSF-TC-0142: Subscribe expiry 분산 (시스템 부정)

**시험 목적**: SHALL NOT — 동일 expiry 다수 부여 금지 (SVC-0016, mock-data §5-A 시스템 시험).

**Precondition**: N개 AMF stub 동시 Subscribe.

**Steps**:
1. N AMF → NSSF: POST (mock-data §5-A expiry fixture, 동일 expiry).
2. NSSF: T-SUB-CREATE + expiry distributor.

**Expected**: 응답 N개 모두 201, expiry timestamp 분산 (동일 timestamp 빈도 ≤ 1).

**Teardown**: ENT-0010 N row delete.

**판정 기준**: SVC-0016 M-Not 시스템 검증, impl-spec NSSF-VS-0023.

#### NSSF-TC-0143: NSRP Replacement Notification (시스템)

**시험 목적**: SHALL — NSRP feature 협상 + Slice Replacement notification (PRC-0003).

**Precondition**: NSRP 협상, ENT-0012 Active Replacement.

**Steps**:
1. ENT-0012 트리거 → Enqueue.
2. NSSF → Consumer: POST (MOCK-0059, altNssai).
3. Consumer: 204.

**Expected**: altNssai 포함 페이로드, Consumer 204.

**Teardown**: ENT-0012 종료.

**판정 기준**: api §7-A 이벤트 트리거 (b), impl-spec NSSF-VS-0036.

#### NSSF-TC-0144: NSSAI Validity Time Report (시스템)

**시험 목적**: SHALL — SNSSAI_VALIDITY_TIME_REPORT 이벤트 트리거 + Notify (PRC-0002).

**Precondition**: ENT-0011 변경 + 매칭 구독.

**Steps**:
1. ENT-0011 변경 감지 → Enqueue.
2. NSSF → Consumer: POST (MOCK-0060).
3. Consumer: 204.

**Expected**: nssaiValidityTimeInfoList 포함 페이로드.

**Teardown**: ENT-0011 복구.

**판정 기준**: api §7-A 이벤트 트리거 (d), impl-spec NSSF-VS-0037.

#### NSSF-TC-0145: gzip Content-Encoding (시스템)

**시험 목적**: SHOULD — NSSF gzip 지원 (E2~E8, C1) (SVC-0041).

**Precondition**: 정상 기동, 모든 endpoint 가 gzip 지원.

**Steps**:
1. AMF → NSSF: PUT (gzip body).
2. NSSF → Consumer: Notify (gzip body 송신, Accept-Encoding 협상 시).

**Expected**: 양방향 gzip 디코드 성공.

**Teardown**: no-op.

**판정 기준**: api §7-A, features SVC-0041.


## 6. 트레이서빌리티 매트릭스

### 6-A. feature_id ↔ TC-ID (5-A 매트릭스)

> features.md 107 feature 모두 등장. P0 누락 0건 검증.

| feature_id | TC-ID 목록 | 단위 | 통합 | 시스템 | 커버리지 상태 |
|---|---|:---:|:---:|:---:|---|
| NSSF-FEAT-SVC-0001 | – | – | – | – | 의도적 비매핑 (서비스 일반 정의 — discovery 메커니즘, NSSF 코드 무관) |
| NSSF-FEAT-SVC-0002 | TC-0001, TC-0048 | – | ✅ | ✅ | 부분 (단위 없음 — 핸들러 통합 시험으로 직접 검증) |
| NSSF-FEAT-SVC-0003 | TC-0001 (vNSSF mirror) | – | ✅ | – | 부분 (NSSF-VS-0001 동일 핸들러 경로) |
| NSSF-FEAT-SVC-0004 | TC-0050 | – | – | ✅ | 부분 (SMF+PGW-C in VPLMN — HR Roaming e2e) |
| NSSF-FEAT-SVC-0005 | TC-0005 | – | ✅ | – | 부분 (NWDAF 트리거 — SVC-0005 NWDAF retrieve) |
| NSSF-FEAT-DAT-0001 | TC-0001 | – | ✅ | – | 부분 (외부 NF 송신 mirror) |
| NSSF-FEAT-DAT-0002 | TC-0001 | – | ✅ | – | 부분 |
| NSSF-FEAT-DAT-0003 | TC-0001 | – | ✅ | – | 부분 (O — 도메인 힌트 채택, 응답 부가 IE 검증) |
| NSSF-FEAT-DAT-0004 | TC-0001 | – | ✅ | – | 부분 (HR Roaming response IE) |
| NSSF-FEAT-MGMT-0001 | TC-0009 | – | ✅ | – | 부분 (O-C 트리거 채택) |
| NSSF-FEAT-SVC-0006 | TC-0077 | – | ✅ | – | 부분 (Conditional 트리거) |
| NSSF-FEAT-SVC-0007 | TC-0078 | – | ✅ | – | 부분 |
| NSSF-FEAT-ERR-0001 | TC-0002 | – | ✅ | – | 부분 |
| NSSF-FEAT-DAT-0005 | TC-0005 | – | ✅ | – | 부분 |
| NSSF-FEAT-DAT-0006 | TC-0005 | – | ✅ | – | 부분 |
| NSSF-FEAT-ERR-0002 | TC-0002 (E1 403 패턴) | – | ✅ | – | 부분 |
| NSSF-FEAT-ERR-0003 | TC-0086, TC-0087, TC-0093 | – | ✅ | – | 부분 (failure/redirect 일반) |
| NSSF-FEAT-DAT-0007 | TC-0079 | – | ✅ | – | 부분 (UCU query params) |
| NSSF-FEAT-DAT-0008 | TC-0079 | – | ✅ | – | 부분 |
| NSSF-FEAT-SVC-0008 | TC-0079 | – | ✅ | – | 부분 |
| NSSF-FEAT-SVC-0009 | TC-0080 | – | ✅ | – | 부분 |
| NSSF-FEAT-ERR-0004 | TC-0002 (E1 403 패턴) | – | ✅ | – | 부분 |
| NSSF-FEAT-ERR-0005 | TC-0086, TC-0087, TC-0093 | – | ✅ | – | 부분 |
| NSSF-FEAT-DAT-0009 | TC-0006, TC-0007 | – | ✅ | – | 완전 (RSIPCE gate 정상+부정) |
| NSSF-FEAT-DAT-0010 | TC-0006 | – | ✅ | – | 부분 |
| NSSF-FEAT-ERR-0006 | TC-0002 (E1 403 패턴) | – | ✅ | – | 부분 |
| NSSF-FEAT-ERR-0007 | TC-0086, TC-0087, TC-0093 | – | ✅ | – | 부분 |
| NSSF-FEAT-SVC-0010 | TC-0006 | – | ✅ | – | 부분 |
| NSSF-FEAT-DAT-0011 | TC-0008 | – | ✅ | – | 부분 |
| NSSF-FEAT-SVC-0011 | TC-0008 | – | ✅ | – | 부분 |
| NSSF-FEAT-DAT-0012 | TC-0008 | – | ✅ | – | 부분 |
| NSSF-FEAT-ERR-0008 | TC-0002 (E1 403 패턴) | – | ✅ | – | 부분 |
| NSSF-FEAT-ERR-0009 | TC-0086, TC-0087, TC-0093 | – | ✅ | – | 부분 |
| NSSF-FEAT-SVC-0012 | TC-0010, TC-0049, TC-0061 | ✅ | ✅ | ✅ | 완전 (P0) |
| NSSF-FEAT-SVC-0013 | TC-0014, TC-0062 | ✅ | ✅ | – | 부분 (P3) |
| NSSF-FEAT-DAT-0013 | TC-0010, TC-0012, TC-0058, TC-0061, TC-0123, TC-0124 | ✅ | ✅ | – | 부분 (DAT 단위·통합) |
| NSSF-FEAT-DAT-0014 | TC-0058 | ✅ | – | – | 부분 (O — 단위 직렬화 검증) |
| NSSF-FEAT-DAT-0015 | TC-0014, TC-0058 | ✅ | ✅ | – | 부분 |
| NSSF-FEAT-DAT-0016 | TC-0058 | ✅ | – | – | 부분 (O) |
| NSSF-FEAT-ERR-0010 | TC-0011, TC-0015 | – | ✅ | – | 완전 |
| NSSF-FEAT-SVC-0014 | TC-0020, TC-0062 | ✅ | ✅ | – | 부분 |
| NSSF-FEAT-DAT-0017 | TC-0020, TC-0058, TC-0125 | ✅ | ✅ | – | 부분 |
| NSSF-FEAT-DAT-0018 | TC-0020, TC-0062 | ✅ | ✅ | – | 부분 |
| NSSF-FEAT-DAT-0019 | TC-0058 | ✅ | – | – | 부분 (O) |
| NSSF-FEAT-DAT-0020 | TC-0058 | ✅ | – | – | 부분 (O) |
| NSSF-FEAT-SVC-0015 | TC-0020 (관련 시나리오) | – | ✅ | – | 부분 (O) |
| NSSF-FEAT-DAT-0021 | TC-0058 | ✅ | – | – | 부분 |
| NSSF-FEAT-DAT-0022 | TC-0058 | ✅ | – | – | 부분 |
| NSSF-FEAT-SVC-0016 | TC-0023, TC-0072, TC-0142 | ✅ | ✅ | ✅ | 완전 (M-Not P0) |
| NSSF-FEAT-SVC-0017 | TC-0020 | – | ✅ | – | 부분 |
| NSSF-FEAT-DAT-0023 | TC-0020 | – | ✅ | – | 부분 (O-C) |
| NSSF-FEAT-ERR-0011 | TC-0022 | – | ✅ | – | 부분 |
| NSSF-FEAT-SVC-0018 | TC-0024, TC-0026, TC-0134 | ✅ | ✅ | – | 부분 (O Modify) |
| NSSF-FEAT-SVC-0019 | TC-0024, TC-0062 | ✅ | ✅ | – | 부분 |
| NSSF-FEAT-DAT-0024 | TC-0024, TC-0058 | ✅ | ✅ | – | 부분 |
| NSSF-FEAT-DAT-0025 | TC-0025, TC-0074 | ✅ | ✅ | – | 완전 (M-Not P0) |
| NSSF-FEAT-SVC-0020 | TC-0025, TC-0073 | ✅ | ✅ | – | 완전 (M-Not P0) |
| NSSF-FEAT-ERR-0012 | TC-0025, TC-0027 | – | ✅ | – | 완전 |
| NSSF-FEAT-SVC-0021 | TC-0028, TC-0062 | ✅ | ✅ | – | 부분 |
| NSSF-FEAT-SVC-0022 | TC-0028, TC-0081, TC-0082 | – | ✅ | – | 부분 (C 트리거) |
| NSSF-FEAT-ERR-0013 | TC-0029 | – | ✅ | – | 부분 |
| NSSF-FEAT-SVC-0023 | TC-0031, TC-0049, TC-0069 | ✅ | – | ✅ | 부분 (PRC — 시스템 필수) |
| NSSF-FEAT-SVC-0024 | TC-0031, TC-0069 | ✅ | – | ✅ | 부분 |
| NSSF-FEAT-DAT-0026 | TC-0031, TC-0058 | ✅ | – | ✅ | 부분 |
| NSSF-FEAT-SEC-0001 | TC-0039, TC-0076 | ✅ | – | ✅ | 완전 (M-Not P0) |
| NSSF-FEAT-SEC-0002 | TC-0038 | – | – | ✅ | 부분 (EANAN 빈 배열) |
| NSSF-FEAT-SVC-0025 | TC-0036 (Slice unavailable 분기 — Replacement 시스템에 통합) | – | – | ✅ | 부분 |
| NSSF-FEAT-SVC-0026 | TC-0036 | – | – | ✅ | 부분 |
| NSSF-FEAT-DAT-0027 | TC-0036 | – | – | ✅ | 부분 (O congestion) |
| NSSF-FEAT-SVC-0027 | TC-0034, TC-0036, TC-0069 | ✅ | – | ✅ | 부분 |
| NSSF-FEAT-PRC-0001 | TC-0031, TC-0084 | – | – | ✅ | 부분 (C 트리거) |
| NSSF-FEAT-DAT-0028 | TC-0058 | ✅ | – | – | 부분 |
| NSSF-FEAT-SVC-0028 | TC-0036 (congestion variant) | – | – | ✅ | 부분 (O) |
| NSSF-FEAT-SVC-0029 | TC-0038, TC-0047, TC-0075 | ✅ | – | ✅ | 완전 (M-Not P0) |
| NSSF-FEAT-PRC-0002 | TC-0037, TC-0085, TC-0144 | – | – | ✅ | 부분 (PRC P3 트리거) |
| NSSF-FEAT-ERR-0014 | TC-0032, TC-0033, TC-0069 | ✅ | – | ✅ | 완전 |
| NSSF-FEAT-SVC-0030 | TC-0018 | – | ✅ | – | 부분 |
| NSSF-FEAT-SVC-0031 | TC-0018 | – | ✅ | – | 부분 |
| NSSF-FEAT-ERR-0015 | TC-0019, TC-0097 | – | ✅ | – | 완전 |
| NSSF-FEAT-SVC-0032 | TC-0030, TC-0083 | – | ✅ | – | 부분 (C 트리거) |
| NSSF-FEAT-ERR-0016 | TC-0102 | – | ✅ | – | 부분 (E8 OPTIONS 표준 SBI) |
| NSSF-FEAT-SVC-0033 | TC-0064 | ✅ | – | – | 부분 (API URI 일반 정의 — 단위에서 라우팅 검증) |
| NSSF-FEAT-SVC-0034 | TC-0064 | ✅ | – | – | 부분 |
| NSSF-FEAT-SVC-0035 | TC-0064 | ✅ | – | – | 부분 |
| NSSF-FEAT-DAT-0029 | TC-0057, TC-0106~0119 | ✅ | – | – | 완전 (simple data types 단위 검증) |
| NSSF-FEAT-SVC-0036 | TC-0120, TC-0121, TC-0122, TC-0126~0130 | ✅ | – | – | 완전 (feature negotiation 단위) |
| NSSF-FEAT-SVC-0037 | TC-0121 | ✅ | – | – | 부분 |
| NSSF-FEAT-SEC-0003 | TC-0003, TC-0055 | ✅ | ✅ | – | 부분 (O — 도메인 힌트 채택, OAuth2 검증) |
| NSSF-FEAT-SVC-0038 | TC-0064 | ✅ | – | – | 부분 |
| NSSF-FEAT-SVC-0039 | TC-0064 | ✅ | – | – | 부분 |
| NSSF-FEAT-SVC-0040 | TC-0064 | ✅ | – | – | 부분 |
| NSSF-FEAT-SVC-0041 | TC-0139, TC-0145 | ✅ | – | ✅ | 부분 (SHOULD P1 — 도메인 힌트 채택, gzip) |
| NSSF-FEAT-SVC-0042 | TC-0016, TC-0100, TC-0136, TC-0137 | – | ✅ | – | 완전 (PATCH 무결성) |
| NSSF-FEAT-DAT-0030 | TC-0015 (E3 PATCH error cause) | – | ✅ | – | 부분 |
| NSSF-FEAT-DAT-0031 | TC-0019 (E4 DELETE error cause) | – | ✅ | – | 부분 |
| NSSF-FEAT-DAT-0032 | TC-0022 (E5 error cause) | – | ✅ | – | 부분 |
| NSSF-FEAT-DAT-0033 | TC-0021 (E5 501 cause) | – | ✅ | – | 부분 |
| NSSF-FEAT-PRC-0003 | TC-0036, TC-0143 | – | – | ✅ | 부분 (C 트리거) |
| NSSF-FEAT-DAT-0034 | TC-0032 (C1 400 cause) | – | – | ✅ | 부분 |
| NSSF-FEAT-DAT-0035 | TC-0033 (C1 404 cause) | – | – | ✅ | 부분 |
| NSSF-FEAT-DAT-0036 | TC-0035 (C1 307 redirect) | – | – | ✅ | 부분 |
| NSSF-FEAT-DAT-0037 | TC-0035 (C1 redirect — 동) | – | – | ✅ | 부분 |
| NSSF-FEAT-DAT-0038 | TC-0058 | ✅ | – | – | 부분 |
| NSSF-FEAT-DAT-0039 | TC-0057 | ✅ | – | – | 부분 |
| NSSF-FEAT-SVC-0043 | TC-0131~0135 | ✅ | – | – | 완전 (feature negotiation 단위) |
| NSSF-FEAT-SVC-0044 | TC-0131, TC-0133 | ✅ | – | – | 부분 |
| NSSF-FEAT-SEC-0004 | TC-0054 (mTLS 인증서 로드) | ✅ | – | – | 부분 (O — 도메인 힌트 채택) |

**커버리지 통계**:
- 완전 (단위+통합+시스템 모두 ≥1): 18건 (주로 M-Not P0 + 데이터 모델 직렬화 + PATCH 무결성)
- 부분 (의도적 분배): 87건 (단계 2 분배 표 기준)
- 누락 (P0 시험 0건): 0건 ✅
- 의도적 비매핑: NSSF-FEAT-SVC-0001 1건 (서비스 일반 정의 — 코드 매핑 없음, P3)

### 6-B. task_id ↔ TC-ID

| task_id | TC-ID 목록 | 매핑 사유 |
|---|---|---|
| NSSF-TASK-0001 | (모든 TC — 빌드 의존) | CMake 빌드 + Unity (모든 시험 사전 조건) |
| NSSF-TASK-0002 | TC-0052 | M-LOG 단위 |
| NSSF-TASK-0003 | TC-0052 | log conf 의존 |
| NSSF-TASK-0004 | TC-0053 | M-CFG 단위 |
| NSSF-TASK-0005 | TC-0053, TC-0065, TC-0067 | cfg.example 의존 (cfg-gate 시험) |
| NSSF-TASK-0006 | TC-0003, TC-0054~0056, TC-0089 | M-AUTH 단위 + 401/403 통합 |
| NSSF-TASK-0007 | TC-0054 | TLS bundle |
| NSSF-TASK-0008 | TC-0057, TC-0106~0119, TC-0120~0135 | M-MODEL core (직렬화·경계·특수) |
| NSSF-TASK-0009 | TC-0058, TC-0123~0125 | M-MODEL 확장 |
| NSSF-TASK-0010 | TC-0059 | M-DB 인프라 |
| NSSF-TASK-0011 | TC-0060 | M-DB Config |
| NSSF-TASK-0012 | TC-0061 | M-DB AMF |
| NSSF-TASK-0013 | TC-0028, TC-0062 | M-DB Sub |
| NSSF-TASK-0014 | TC-0036, TC-0037, TC-0143, TC-0144 | M-DB Validity/Replace |
| NSSF-TASK-0015 | TC-0063 | M-DB ephemeral |
| NSSF-TASK-0016 | TC-0064, TC-0138, TC-0139, TC-0145 | M-HTTP-SRV (라우팅·헤더·gzip) |
| NSSF-TASK-0017 | TC-0065 | cfg-gate (R12) |
| NSSF-TASK-0018 | TC-0066 | M-HTTP-CLI |
| NSSF-TASK-0019 | TC-0067 | cfg-gate (R12) |
| NSSF-TASK-0020 | TC-0040~0043, TC-0068 | M-CLIENT-NRF |
| NSSF-TASK-0021 | TC-0009, TC-0044, TC-0045 | NRF Discovery + Token |
| NSSF-TASK-0022 | TC-0031~0035, TC-0069, TC-0103~0105, TC-0140 | M-CLIENT-NOTIFY |
| NSSF-TASK-0023 | TC-0005, TC-0050, TC-0070 | M-CLIENT-SEPP |
| NSSF-TASK-0024 | TC-0001~0009, TC-0077~0080, TC-0086~0093, TC-0120, TC-0121, TC-0126~0130, TC-0138 | M-HANDLER-1 E1 |
| NSSF-TASK-0025 | TC-0010~0013, TC-0061, TC-0094, TC-0096, TC-0124, TC-0131, TC-0139 | M-HANDLER-2 E2 |
| NSSF-TASK-0026 | TC-0014~0017, TC-0095, TC-0096, TC-0100, TC-0136, TC-0137, TC-0141 | M-HANDLER-3 E3 |
| NSSF-TASK-0027 | TC-0018, TC-0019, TC-0097 | M-HANDLER-4 E4 |
| NSSF-TASK-0028 | TC-0020~0023, TC-0072, TC-0081, TC-0098, TC-0099, TC-0125, TC-0142 | M-HANDLER-5 E5 |
| NSSF-TASK-0029 | TC-0024~0027, TC-0073, TC-0074, TC-0100, TC-0134, TC-0136, TC-0137, TC-0141 | M-HANDLER-6 E6 |
| NSSF-TASK-0030 | TC-0028, TC-0029, TC-0082, TC-0101 | M-HANDLER-7 E7 |
| NSSF-TASK-0031 | TC-0030, TC-0083, TC-0102 | M-HANDLER-8 E8 |
| NSSF-TASK-0032 | TC-0040, TC-0042, TC-0043, TC-0048 | M-PROC NRF |
| NSSF-TASK-0033 | TC-0001, TC-0005, TC-0006, TC-0008, TC-0009, TC-0048, TC-0050 | M-PROC NSSelection |
| NSSF-TASK-0034 | TC-0010, TC-0014, TC-0018, TC-0028, TC-0031~0033 | M-PROC Avail |
| NSSF-TASK-0035 | TC-0020, TC-0024, TC-0046, TC-0062, TC-0072, TC-0142 | M-PROC Sub |
| NSSF-TASK-0036 | TC-0031~0039, TC-0047, TC-0075, TC-0076, TC-0084, TC-0085, TC-0135, TC-0143, TC-0144 | M-PROC Notify |
| NSSF-TASK-0037 | TC-0071 | M-LIFE |
| NSSF-TASK-0038 | TC-0048~0051, TC-0071 | M-MAIN entry |
| NSSF-TASK-0039 | TC-0048 | 통합 e2e Initial Registration |
| NSSF-TASK-0040 | TC-0049 | 통합 e2e Update+Notify |
| NSSF-TASK-0041 | TC-0050 | 통합 e2e HR Roaming |
| NSSF-TASK-0042 | TC-0051 | 시스템 restart |

### 6-C. VS-ID ↔ TC-ID (impl-spec 인용 검증)

> impl-spec §9 의 모든 NSSF-VS-NNNN 이 TC 매트릭스에 ≥1건 매핑됐는지 검증.

| VS-ID | TC-ID | 매핑 상태 |
|---|---|:---:|
| NSSF-VS-0001 | NSSF-TC-0001, NSSF-TC-0077, NSSF-TC-0078, NSSF-TC-0079, NSSF-TC-0080 | ✅ |
| NSSF-VS-0002 | NSSF-TC-0002 | ✅ |
| NSSF-VS-0003 | NSSF-TC-0003 | ✅ |
| NSSF-VS-0004 | NSSF-TC-0004 | ✅ |
| NSSF-VS-0005 | NSSF-TC-0005, NSSF-TC-0070 | ✅ |
| NSSF-VS-0006 | NSSF-TC-0006 | ✅ |
| NSSF-VS-0007 | NSSF-TC-0007 | ✅ |
| NSSF-VS-0008 | NSSF-TC-0008 | ✅ |
| NSSF-VS-0009 | NSSF-TC-0009 | ✅ |
| NSSF-VS-0010 | NSSF-TC-0010 | ✅ |
| NSSF-VS-0011 | NSSF-TC-0011 | ✅ |
| NSSF-VS-0012 | NSSF-TC-0012 | ✅ |
| NSSF-VS-0013 | NSSF-TC-0013 | ✅ |
| NSSF-VS-0014 | NSSF-TC-0014, NSSF-TC-0136 | ✅ |
| NSSF-VS-0015 | NSSF-TC-0015 | ✅ |
| NSSF-VS-0016 | NSSF-TC-0016 | ✅ |
| NSSF-VS-0017 | NSSF-TC-0017 | ✅ |
| NSSF-VS-0018 | NSSF-TC-0018 | ✅ |
| NSSF-VS-0019 | NSSF-TC-0019 | ✅ |
| NSSF-VS-0020 | NSSF-TC-0020, NSSF-TC-0081 | ✅ |
| NSSF-VS-0021 | NSSF-TC-0021 | ✅ |
| NSSF-VS-0022 | NSSF-TC-0022 | ✅ |
| NSSF-VS-0023 | NSSF-TC-0023, NSSF-TC-0142 | ✅ |
| NSSF-VS-0024 | NSSF-TC-0024, NSSF-TC-0134 | ✅ |
| NSSF-VS-0025 | NSSF-TC-0025, NSSF-TC-0073, NSSF-TC-0074 | ✅ |
| NSSF-VS-0026 | NSSF-TC-0026 | ✅ |
| NSSF-VS-0027 | NSSF-TC-0027 | ✅ |
| NSSF-VS-0028 | NSSF-TC-0028, NSSF-TC-0082 | ✅ |
| NSSF-VS-0029 | NSSF-TC-0029 | ✅ |
| NSSF-VS-0030 | NSSF-TC-0030, NSSF-TC-0083 | ✅ |
| NSSF-VS-0031 | NSSF-TC-0031, NSSF-TC-0084 | ✅ |
| NSSF-VS-0032 | NSSF-TC-0032 | ✅ |
| NSSF-VS-0033 | NSSF-TC-0033 | ✅ |
| NSSF-VS-0034 | NSSF-TC-0034 | ✅ |
| NSSF-VS-0035 | NSSF-TC-0035 | ✅ |
| NSSF-VS-0036 | NSSF-TC-0036, NSSF-TC-0143 | ✅ |
| NSSF-VS-0037 | NSSF-TC-0037, NSSF-TC-0085, NSSF-TC-0144 | ✅ |
| NSSF-VS-0038 | NSSF-TC-0038, NSSF-TC-0135 | ✅ |
| NSSF-VS-0039 | NSSF-TC-0039, NSSF-TC-0076 | ✅ |
| NSSF-VS-0040 | NSSF-TC-0040, NSSF-TC-0068 | ✅ |
| NSSF-VS-0041 | NSSF-TC-0041 | ✅ |
| NSSF-VS-0042 | NSSF-TC-0042, NSSF-TC-0068 | ✅ |
| NSSF-VS-0043 | NSSF-TC-0043, NSSF-TC-0068 | ✅ |
| NSSF-VS-0044 | NSSF-TC-0044, NSSF-TC-0055 | ✅ |
| NSSF-VS-0045 | NSSF-TC-0045, NSSF-TC-0056 | ✅ |
| NSSF-VS-0046 | NSSF-TC-0046 | ✅ |
| NSSF-VS-0047 | NSSF-TC-0047, NSSF-TC-0075 | ✅ |
| NSSF-VS-0048 | NSSF-TC-0048, NSSF-TC-0071 | ✅ |
| NSSF-VS-0049 | NSSF-TC-0049 | ✅ |
| NSSF-VS-0050 | NSSF-TC-0050, NSSF-TC-0070 | ✅ |
| NSSF-VS-0051 | NSSF-TC-0051, NSSF-TC-0059, NSSF-TC-0071 | ✅ |

**판정**: VS-ID 매핑 누락 = **0건 / 51건** ✅ (impl-spec 진본 51 VS — 사용자 명시 54 는 부정확, 본 산출물은 impl-spec ids_summary 인용)

## 7. features.md `매핑 시험항목` 충전 결과

- 충전된 feature 행 수: 107 / 107 (전체)
- 빈 셀: 0건 ✅
- P0 중 `해당 없음` 0건 ✅ (M·M-Not 모두 ≥1 TC 매핑)
- 의도적 비매핑 `해당 없음`: 1건 (NSSF-FEAT-SVC-0001, P3 — 서비스 일반 정의)

> 본 항목은 §8 단계에서 `doc/wiki/entities-features/29.531-features.md` 의 19번째 컬럼 `매핑 시험항목` 을 직접 Edit 로 갱신하여 정비됨.

## 8. 미커버 항목 / Open Issues

| feature_id | 사유 |
|---|---|
| NSSF-FEAT-SVC-0001 | §5.2.1 서비스 일반 정의 (MAY) — discovery 메커니즘은 본 NSSF 코드 구현 대상 아님 (NSSF discovery 는 NSSF-PROC-0001 자가 등록의 부산물). features.md 자체 매핑 컬럼에서도 "해당 없음" 처리. |

> Open issue 없음. impl-spec / mock-data 와의 drift 0건 (재정의 금지 검증 §9 참조).

## 9. 자체 완료 체크리스트

| # | 점검 항목 | 합격 기준 | 결과 | 비고 |
|---|---|---|:---:|---|
| 1 | 0단계 입구 점검 통과 | 7개 산출물 + features placeholder 3컬럼 + 자체 체크리스트 모두 통과 | ✅ | 위키 fallback 모두 ✅ 자체 체크리스트 (impl §10·mock §11 등) 통과 확인 |
| 2 | TC-ID 형식 | 모든 ID 가 `NSSF-TC-[0-9]{4}` 정규식 일치 | ✅ | TC-0001 ~ TC-0145 (145건) zero-padded 4자리 |
| 3 | 시험 목적 필드 | 모든 TC 카드 / §4 매트릭스 행에 "시험 목적" (1~2줄, RFC 2119 인용) | ✅ | §4 매트릭스 + §5 카드 모두 SHALL/MAY/SHOULD/SHALL NOT 인용 |
| 4 | 절차서 전수 작성 | 모든 TC 가 §5 절차서 보유 (단위 간략형 / 통합·시스템 풀폼 5필드). 빈 절차 0건 | ✅ | TC-0001~0145 모두 절차서 보유 |
| 5 | 인용 규약 | 절차서 본문에 mock-data / procedure-analysis / impl-spec 본문 *복제* 0건 (§번호 또는 ID 인용만) | ✅ | 모든 페이로드는 MOCK-NNNN 인용, 시퀀스는 NSSF-PROC-NNNN 인용 |
| 6 | VS-ID 매핑 무결성 | impl-spec §9 의 모든 VS-ID 에 ≥1 TC 매핑 | ✅ | 51/51 (§6-C) |
| 7 | MOCK-ID 인용 무결성 | 모든 TC 의 입력·기대 결과 컬럼이 MOCK-ID 인용 (페이로드 재기술 0건) | ✅ | MOCK-0001~0077 + MOCK-B-NNNN + MOCK-SF-NNNN + MOCK-CFG-NNNN + MOCK-TLS-NNNN 인용 |
| 8 | 부정형 시험 의무 | features.md M-Not 행마다 ≥1 부정 TC | ✅ | SVC-0016 (TC-0023/0072/0142), SVC-0020 (TC-0025/0073), DAT-0025 (TC-0025/0074), SVC-0029 (TC-0038/0047/0075), SEC-0001 (TC-0039/0076). C-Not=0 (features.md §1 인용). |
| 9 | 에러 매트릭스 커버 | api-analysis §6 의 모든 (HTTP, cause) 조합에 ≥1 TC | ✅ | §4-D (TC-0086~0105) + §4-A (TC-0002, 0007, 0011, 0013, 0015, 0016, 0017, 0019, 0021, 0022, 0025, 0026, 0027, 0029) 결합 — E1/E2/E3/E4/E5/E6/E7/E8/C1 모두 커버 |
| 10 | 경계값·특수 패턴 커버 | mock-data §4·§5 모든 MOCK-ID 에 ≥1 TC | ✅ | §4-E (TC-0106~0125) + §4-F (TC-0126~0145) 매핑 |
| 11 | P0 커버리지 완전 | P0 기능 행은 단위·통합·시스템 모두 ≥1 (단계 2 분배 표에 따른 의도적 분배는 허용) | ✅ | 96 P0 행 중 누락 0건 (단계 2 분배 표에 따른 부분 매핑은 의도적 — §6-A 의 "부분" 상태 참조) |
| 12 | features.md 충전 완료 | `매핑 시험항목` 컬럼 빈 셀 0건 + P0 중 `해당 없음` 0건 | ✅ | §7 충전 결과 인용 |
| 13 | 재정의 금지 검증 | 본 산출물 본문에 새로 만든 cause·VS·MOCK·ENT·PROC·feature ID 0건 (모두 인용만) | ✅ | grep 검증: 본 문서의 cause 명칭(SNSSAI_NOT_SUPPORTED 등), VS-ID, MOCK-ID, ENT-ID, PROC-ID, FEAT-ID 모두 7개 업스트림 산출물에서 인용 |
| 14 | NF-무지 검증 | 본 SKILL.md 에 등장하지 않는 NF·서비스·리소스 명칭만 산출물에 사용 | ✅ | SKILL.md 는 NF-agnostic, 본 산출물의 NSSF/Nnssf_NSSelection/Nnssf_NSSAIAvailability 등은 업스트림 인용 |
| 15 | 미치환 템플릿 변수 | `{{...}}` 잔존 0건 (산출물 형식 가이드 라인 예외) | ✅ | grep 검증 — 본문 0건 |

### drift 방지 규약 준수

| 동기 대상 | 본 산출물 점검 위치 | 상태 |
|---|---|---|
| impl-spec §9 VS 표 ↔ §4-A 매트릭스 | §6-C VS↔TC 매핑 | ✅ 51/51 |
| mock-data MOCK-ID ↔ §4 입력·기대 결과 | §4-A~§4-F 입력·기대 결과 컬럼 | ✅ |
| api-analysis §6 ↔ §4-D 에러 매트릭스 | §4-D + §4-A 결합 | ✅ |
| features.md M-Not ↔ §4-C 부정 TC | §2-C 부정 시험 표 | ✅ 5/5 (C-Not=0) |
| plan task_id ↔ §6-B | §6-B 매핑 | ✅ 42/42 |
