---
title: NSSF Domain Hints (spec-reading output)
tags: [sources, hints, spec-reading, nf:nssf, spec:29.531]
last_updated: 2026-05-26
source: doc/analysis/NSSF_hints.md
nf: NSSF
spec: 29.531
ids_summary: Hints H1~H8 (no stable IDs — derivation rules)
---

> **위키 편입 정보**
> - 원본: `doc/analysis/NSSF_hints.md`
> - last_updated: 2026-05-26
> - 안정 ID 요약: Hints H1~H8 (no stable IDs — derivation rules)

# NSSF 도메인 힌트

> 본 문서는 `spec-reading` 스킬이 정의한 8개 섹션 표준 카탈로그를 따릅니다.
> 후속 분석 스킬(`feature-extraction`, `api-analysis`, `procedure-analysis`, `db-design`, `impl-spec-generation`, `mock-data-generation`, `test-spec-generation`, `plan-generation`, `tech-stack-selection`)은 본 문서를 **최우선(MUST)으로 참고**합니다.
> 작성 시각: 2026-05-26 · 작성 모드: 신규 (기존 hints.md 없음) · target_language: C

## H1. NF 정체성

- **약어**: NSSF
- **Full name**: Network Slice Selection Function
- **별칭**: H-NSSF (Home PLMN NSSF), V-NSSF (Visited PLMN NSSF), hNSSF / vNSSF (소문자 표기, 23.502 §5.30 등)
- **주요 책임** (TS 29.531 §4.1 + TS 23.501 §6.2.14):
  - UE를 위해 가용한 **Network Slice instance 집합 선택**
  - **Allowed NSSAI** 및 (필요 시) Subscribed S-NSSAI로의 매핑 결정
  - **Configured NSSAI** 및 (필요 시) Subscribed S-NSSAI로의 매핑 결정
  - 등록 시 사용할 **AMF Set** 또는 candidate AMF 목록 결정 (필요 시 NRF 질의)
  - NWDAF 분석 기반의 **Network Slice/instance restriction**
  - S-NSSAI **Replacement** 판단·통지 (Alternative S-NSSAI 제공, NWDAF/OAM 트리거)
- **수행 인터페이스**: Nnssf SBI (HTTP/2, JSON, OAuth2)
- **3GPP 규격 핵심**: TS 29.531 (stage 3, 본 분석 메인), TS 23.501 §6.2.14 (stage 2 기능), TS 23.502 §5.2.16 (서비스 매트릭스), §4.2.2 (Registration 절차)

## H2. 서비스 그룹

| 서비스명 | apiName | 1줄 설명 |
|---|---|---|
| Nnssf_NSSelection | `nnssf-nsselection` | AMF/SMF/NWDAF/타 PLMN NSSF가 호출하는 슬라이스 선택 서비스 — Allowed/Configured NSSAI·NSAG·VPLMN↔HPLMN S-NSSAI 매핑·NSI ID 제공 (단일 `Get` operation) |
| Nnssf_NSSAIAvailability | `nnssf-nssaiavailability` | AMF가 호출하는 per-TA NSSAI 가용성 등록·구독·통지 서비스 — 6개 operation(Update/Subscribe/Unsubscribe/Notify/Delete/Options), PATCH(JSON Patch) 사용, Slice Replacement·NSSAI Validity 통지 |

> apiVersion 은 둘 다 `v2` (TS 29.531 §6.1.1 / §6.2.1; URI 패턴 `{apiRoot}/<apiName>/v2`).
> OpenAPI YAML 동봉: `doc/specs/29531-j60/TS29531_Nnssf_NSSelection.yaml`, `TS29531_Nnssf_NSSAIAvailability.yaml`.

## H3. 핵심 복합 절차 목록

| 절차명 | TS 23.502 § (또는 메인 규격 §) | 본 NF의 일반 역할 |
|---|---|---|
| Registration procedure (초기 등록, AMF→NSSF 슬라이스 선택 질의) | 23.502 §4.2.2.2.2 (Initial AMF→NSSF: Nnssf_NSSelection_Get, 라인 6221) | Producer — Allowed NSSAI / target AMF Set / Configured NSSAI 반환 |
| Registration with AMF re-allocation | 23.502 §4.2.2.2.3 | Producer — re-allocation 대상 AMF Set 산출 |
| EPS→5GS Handover (N26) / Mobility Registration | 23.502 §4.11.1.2.2, §4.11.1.3.3/3A/4, §4.23.12 | Producer — 핸드오버용 슬라이스 정보 |
| Xn / N2 Handover with PLMN change | 23.502 §4.9.1, §4.23.7, §4.23.11 | Producer — PLMN 변경 시 NSSelection_Get |
| UE Configuration Update | 23.502 §4.2.4.2 | Producer — Configured NSSAI 갱신 |
| SMF selection (non-roaming / LBO / home-routed) | 23.502 §4.3.2.2.3.2, §4.3.2.2.3.3 | Producer — VPLMN↔HPLMN 슬라이스 매핑 제공 |
| PDN Connection Establishment in EPC (SMF+PGW-C) | 23.502 §4.11.0a.5 | Producer — SMF+PGW-C 호출 시 슬라이스 매핑 |
| Roaming NSSelection (V-NSSF↔H-NSSF) | 23.501 §5.30 / 23.502 §5.2.16.2 (라인 13088~13119) | 양방향 — V-NSSF는 H-NSSF의 Consumer이자 AMF의 Producer |
| NSSAI Availability Update (per-TA, AMF↔NSSF) | 23.502 §5.2.16.3.2 (NSSAIAvailability_Update) | Producer — AMF의 per-TA S-NSSAI 지원 등록 및 AuthorizedNssaiAvailability 응답 |
| NSSAI Availability Subscribe / Notify | 23.502 §5.2.16.3.3/.4/.5 | Producer — AMF가 Subscriber, NSSF가 변경 시 Notify (콜백 패턴) |
| Network Slice Replacement / NSI Replacement notification | 23.502 §5.2.16.3.x, TS 29.531 §6.2.5 | Producer — Slice Replacement 트리거 시 콜백 통지 |
| NWDAF Network Slice load analytics | 23.288 §6.3.4 | Producer — NWDAF가 NSI ID 조회 |

## H4. 핵심 데이터 엔티티

| 엔티티 논리명 | 1줄 설명 | 영역 추정 (Config/Runtime) |
|---|---|---|
| NSSAI Configuration per TAI | 운영자가 사전 설정하는 PLMN/TAI별 지원 S-NSSAI 리스트 (Configured NSSAI 결정 기반) | Config |
| Restricted S-NSSAI per TAI per PLMN | TAI/PLMN 단위 제한된 슬라이스 집합 (Allowed NSSAI 계산 입력) | Config |
| AMF Set ↔ S-NSSAI 매핑 | 슬라이스별 서비스 가능한 AMF Set / candidate AMF 목록 | Config |
| NSI ID ↔ S-NSSAI 매핑 | Network Slice Instance 식별자와 S-NSSAI의 매핑 (NWDAF/AMF에 반환) | Config |
| NRF per Slice | 슬라이스 내 NF 발견용 NRF, AMF 후보 선택용 NRF의 주소 매핑 | Config |
| VPLMN↔HPLMN S-NSSAI Mapping | 로밍 시 슬라이스 ID 변환 테이블 (SMF+PGW-C/vNSSF가 사용) | Config |
| NSAG Configuration | TAI별 NSAG 구성 (Configured NSSAI 와 매핑) | Config |
| AMF 등록 상태 (per nfId) | AMF가 Update operation으로 등록한 per-TA 지원 S-NSSAI/NSAG 집합 (NssaiAvailabilityInfo) | Runtime |
| Authorized NSSAI Availability per AMF | NSSF가 AMF의 등록을 인증한 결과 (AuthorizedNssaiAvailabilityData) | Runtime |
| NSSAI Availability Subscription | AMF가 등록한 변경 통지 구독 (subscriptionId, 콜백 URI, 필터 조건) | Runtime |
| NSSAI Validity Time | NSSAI 유효 시간 정보 (변경 시 통지 대상) | Runtime |
| Slice Replacement Plan / State | OAM 또는 NWDAF 트리거 기반의 슬라이스 대체 정책 및 진행 상태 | Config + Runtime |
| HTTP/2 클라이언트 풀 (NRF/콜백 타깃) | OAuth2 토큰 캐시 및 NRF Access Token Request 클라이언트 상태 | Runtime |

> 위 엔티티는 **논리명만** 기재. 정확한 스키마(필드명·타입·제약)는 `db-design` 스킬이 §6.1.6/§6.2.6 데이터 모델 및 OpenAPI YAML을 기반으로 확정.

## H5. 언어별 매핑 예시 (target_language = C)

> **초안 — 사용자 보강 권장**. C에는 표준 JSON 객체 매핑이 없으므로 jansson/cJSON 등 선택에 따라 구조체 디자인이 달라집니다. 첫 분석 라운드의 sketch만 제공 — `api-analysis` 3단계에서 정식 매핑 표 작성, `tech-stack-selection`에서 JSON 라이브러리·CRUD 라이브러리 확정.

| 3GPP 데이터 타입 | C 표현 (스케치) | 메모 |
|---|---|---|
| `Snssai` (TS 29.571 §5.4.4.2: sst integer 0~255 M / sd string 6 hex O) | ```c struct snssai { uint8_t sst; char sd[7]; /* "RRGGBB\0", optional → sd[0]='\0' for absence */ bool has_sd; }; ``` | sst는 0~255 (uint8_t로 정확). sd는 RFC pattern `^[A-Fa-f0-9]{6}$`. JSON 직렬화 시 "sd" 키 생략은 has_sd로 구분. map key 변환 규칙은 29.571 §5.4.4.2 NOTE 참조 (`"sst"` 또는 `"sst-sd"`) |
| `Tai` (TS 29.571 §5.4.4.4: plmnId M, tac M, nid O) | ```c struct tai { struct plmn_id plmn; char tac[7]; /* "HHHHHH\0" 24-bit hex */ char nid[12]; /* SNPN 시 11자리 hex, 미사용 시 '\0' */ bool has_nid; }; struct plmn_id { char mcc[4]; char mnc[4]; }; ``` | tac는 6자 또는 4자 hex 문자열 (`Tac` 정의 참조). nid는 SNPN에서만 사용. JSON 매핑 시 nid 미존재는 키 생략. |
| `ProblemDetails` (TS 29.571 §5.2.4 / RFC 9457) | ```c struct problem_details { char *type; char *title; int status; /* HTTP code */ char *detail; char *instance; char *cause; /* 3GPP application error */ struct invalid_param *invalid_params; size_t invalid_params_len; }; ``` | 모든 필드 optional (RFC 9457). `cause` 는 3GPP application error code (e.g. `NF_DISCOVERY_FAILED`). 응답 시 HTTP status 와 status 필드 일치 필수. |

> 추가 매핑 후보 (필요 시 `api-analysis`가 확장): `AuthorizedNetworkSliceInfo`, `AllowedNssai`, `NssaiAvailabilityInfo`, `AuthorizedNssaiAvailabilityData`, `RestrictedSnssai`, `NssaiAvailabilityNotification`, `PatchItem`.

## H6. 보안·인증 정책

- **OAuth2 grant 방식**: Client Credentials (RFC 6749 §4.4)
- **Authorization Server**: NRF (TS 29.510 §5.4.2.2 의 Access Token Request 서비스)
- **OAuth2 scope 명명**:
  - `Nnssf_NSSelection API`는 **scope를 정의하지 않음** (TS 29.531 §6.1.9 마지막 단락: "The Nnssf_NSSelection API does not define any scopes for OAuth2 authorization.")
  - `Nnssf_NSSAIAvailability API`도 **scope를 정의하지 않음** (TS 29.531 §6.2.9 마지막 단락)
  - 즉 NSSF는 API-level scope만 적용 (`nnssf-nsselection`, `nnssf-nssaiavailability`)되며, 추가 resource-level scope는 미정의.
- **mTLS 적용 범위**: TS 33.501 + TS 29.500 §6.7에 따라 SBI 구간(NSSF↔Consumer, NSSF↔NRF, NSSF↔Notify 콜백) 전체에 TLS 적용. PLMN 간(N32) 경유 시 SEPP 통한 mutual authentication.
- **NRF 의존성**: OAuth2 토큰 발급 NRF는 *Consumer가 NSSF 발견에 사용한 NRF*와 동일해야 함 (TS 29.531 §6.1.9 NOTE).
- **특이 정책**:
  - 동일 NSSF 또는 동일 NSSF set 내 HTTP redirection (307/308) 허용 — `ES3XX` feature negotiation으로 협상.
  - **N32 inter-PLMN 호출**: V-NSSF↔H-NSSF 통신 시 SEPP 경유 (TS 33.501 §13 / TS 29.500 §5.10).

## H7. 인접 NF 카탈로그

| 인접 NF | 관계 (호출/피호출/Routing) | 주요 메시지 (있으면) |
|---|---|---|
| NRF | NSSF가 호출 (mandatory) | (a) NSSF 자체 등록·heartbeat (`Nnrf_NFManagement_NFRegister/Update/Deregister`), (b) OAuth2 Access Token 요청 (`Nnrf_AccessToken_Get`), (c) AMF 후보 조회를 위한 Discovery (`Nnrf_NFDiscovery_NFDiscover`) |
| AMF | 피호출 (NSSelection_Get) + 양방향 (NSSAIAvailability Update/Subscribe + Notify callback) | NSSelection_Get 요청, NSSAIAvailability Update (PUT/PATCH), Subscribe 요청, NSSF→AMF의 Notify callback |
| SMF (및 SMF+PGW-C) | NSSF가 피호출 (NSSelection_Get) | SMF selection·PDN Connection Establishment 시 VPLMN↔HPLMN S-NSSAI 매핑 조회 |
| NWDAF | NSSF가 피호출 (NSSelection_Get) + 데이터 소비 | NSI ID 조회. Slice load analytics를 NWDAF로부터 수신하여 Replacement 판단 |
| V-NSSF / H-NSSF (타 PLMN NSSF) | 양방향 (Roaming NSSelection) | V-NSSF가 H-NSSF의 `Nnssf_NSSelection_Get` Consumer가 됨 |
| SEPP | Routing (inter-PLMN 우회) | V-NSSF↔H-NSSF, V-NSSF↔H-NRF 등 N32 구간 통과 |
| OAM 시스템 | 호출 받음 (관리·통지 트리거) | Slice Replacement·Restriction의 local trigger 입력 |

## H8. 도메인 특이사항

- **`supportedFeatures` 비트맵 협상** (TS 29.500 §6.6 + TS 29.571 §5.2.2):
  - Nnssf_NSSelection: `ES3XX`(F#1, M, 307/308 redirection), `TargetNssai`(F#2, O, TS 23.501 §5.3.4.3.3) 및 이후 j60에서 추가된 다수 feature.
  - Nnssf_NSSAIAvailability: `NSAG`, `RACS`, `NSSRG`, `NetworkSliceReplacement`, `NsiReplacement`, `NSSAIValidityTime` 등 (정확한 비트번호는 §6.2.8 표 6.2.8-1 참조).
  - 협상 결과는 양방향(Request의 supportedFeatures 와 Response의 supportedFeatures 비트 AND).
- **PATCH (JSON Patch, RFC 6902)**:
  - Nnssf_NSSAIAvailability_Update는 **PUT(전체 교체)와 PATCH(JSON Patch operation list) 모두 지원** (TS 29.531 §5.3.2.2 / §6.2.3).
  - PATCH body는 `PatchDocument`로 `op`/`path`/`value` 형태의 PatchItem 배열. `supportedSnssai` 및 NSAG 정보에 대한 부분 갱신 가능.
- **Callback / Notification 패턴**:
  - Nnssf_NSSAIAvailability_Subscribe는 AMF가 콜백 URI(`callbackReference`)를 등록 → NSSF는 변경 시 해당 URI로 POST notification (Notify operation, TS 29.531 §6.2.5).
  - 통지 종류: NSSAI 가용성 변경, Slice Replacement, NSI Replacement, NSSAI Validity Time 변경 (4종).
- **per-TAI 의미론**:
  - 모든 NSSAIAvailability 데이터는 **TAI 단위로 분할**되며, "restricted per PLMN in that TA" 의 의미는 *해당 TAI에서 특정 PLMN UE만 접근 제한*이라는 것 (TS 29.531 §5.3.1).
  - NSSAIAvailability는 **AMF 인스턴스 (`{nfId}`) 별로 등록** 되고, NSSF는 모든 AMF의 합집합으로 PLMN-wide availability를 산출.
- **Indirect Network Sharing**:
  - 일반적인 PLMN 슬라이스 선택 외에, "hosting operator's network" 에서의 슬라이스 선택을 별도로 지원 (TS 29.531 §5.1, §5.2.1).
- **TS 29.531 j60(=Release 18) 신규 항목**: `NSAG` (Network Slice AS Group, §3.2 Abbreviations 단독 추가), Slice/NSI Replacement 통지, NSSAI Validity Time 변경 통지.
- **HTTP redirection (307/308)**:
  - 동일 NSSF 인스턴스 내 또는 동일 NSSF set 내 다른 NSSF 인스턴스로 redirect 가능. `ES3XX` feature 미협상 시 R15 동작만 지원.
- **OpenAPI 첨부 yaml과 본문 표 동기성**: §6.1.6 / §6.2.6 표는 OpenAPI YAML(`TS29531_Nnssf_NSSelection.yaml`, `TS29531_Nnssf_NSSAIAvailability.yaml`)과 1:1 대응. `api-analysis`는 YAML을 우선 파싱하되 표와의 drift를 검증할 것.
- **로그/관측 우선순위 (구현 권장)**: NSSelection_Get 의 입력(Requested NSSAI/PLMN/TAI)과 출력(Allowed NSSAI/AMF Set) 매핑은 슬라이스 진단에 필수 — structured log 권장. NSSAIAvailability 구독 라이프사이클(생성/갱신/만료/통지 실패)도 별도 관측 채널 권장.
