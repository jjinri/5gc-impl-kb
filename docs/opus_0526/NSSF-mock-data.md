---
title: NSSF Mock Data (TDD payloads)
tags: [implementation-specs, mock-data, tdd, fixtures, nf:nssf]
last_updated: 2026-05-26
source: doc/analysis/impl-specs/NSSF_mock_data.md
nf: NSSF
spec: 29.531
ids_summary: MOCK=114 (77 baseline + 19 boundary + 11 supportedFeatures + 7 conf bundle)
---

> **위키 편입 정보**
> - 원본: `doc/analysis/impl-specs/NSSF_mock_data.md`
> - last_updated: 2026-05-26
> - 안정 ID 요약: MOCK=114 (77 baseline + 19 boundary + 11 supportedFeatures + 7 conf bundle)

# NSSF Mock 데이터

## 0. 메타

| 항목 | 값 |
|---|---|
| 대상 NF | NSSF |
| 메인 규격 | TS 29.531 j60 (Rel-18) |
| 입력 산출물 | [impl_spec §9 (51 VS)](../implementation-specs/NSSF-impl-spec.md) / [api-analysis §2/5/6/7](../implementation-specs/NSSF-api-spec.md) / [features.md M-Not 5건](../entities-features/29.531-features.md) / [procedure-analysis §5-B](../entities-features/NSSF-procedure-analysis.md) / [db-design §8-C 시드](../implementation-specs/NSSF-db-design.md) |
| 0단계 입구 점검 | ✅ 통과 (5 산출물 존재, impl_spec §11 17/17 ✅, VS-0001~VS-0051 존재, api §2/5/6/7 작성됨, M-Not 5건 식별) |
| 도메인 힌트 | [NSSF_hints.md](../sources/NSSF-hints.md) 적용 (H5 C 매핑 예시 값) |

## 1. VS ↔ MOCK 매핑

| MOCK-ID | VS-ID | 페이로드 종류 | 매핑 endpoint # | 매핑 기능 ID | 상태 |
|---|---|---|---|---|---|
| MOCK-0001 | VS-0001 | REQ (E1 Registration 정상) | E1 | SVC-0002 | active |
| MOCK-0002 | VS-0001 | RESP-2xx (200 AuthorizedNetworkSliceInfo) | E1 | SVC-0002, DAT-0001~0004 | active |
| MOCK-0003 | VS-0002 | REQ (E1 미지원 SNSSAI) | E1 | – | active |
| MOCK-0004 | VS-0002 | RESP-4xx (403 SNSSAI_NOT_SUPPORTED) | E1 | ERR-0001 | active |
| MOCK-0005 | VS-0003 | REQ (E1 만료 토큰) | E1 | – | active |
| MOCK-0006 | VS-0003 | RESP-4xx (401 TOKEN_EXPIRED) | E1 | – | active |
| MOCK-0007 | VS-0004 | REQ (E1 nf-id 누락) | E1 | – | active |
| MOCK-0008 | VS-0004 | RESP-4xx (400 MANDATORY_QUERY_PARAM_MISSING) | E1 | – | active |
| MOCK-0009 | VS-0005 | REQ (E1 PDU HR roaming) | E1 | SVC-0005 | active |
| MOCK-0010 | VS-0005 | RESP-2xx (200 + mappingOfNssai) | E1 | DAT-0005 | active |
| MOCK-0011 | VS-0006 | REQ (E1 PDN Connection RSIPCE) | E1 | SVC-0010 | active |
| MOCK-0012 | VS-0006 | RESP-2xx (200) | E1 | DAT-0009 | active |
| MOCK-0013 | VS-0007 | REQ (E1 RSIPCE 미협상) | E1 | DAT-0009 (M-Not 부정) | active |
| MOCK-0014 | VS-0007 | RESP-4xx (400 FEATURE_NOT_SUPPORTED) | E1 | – | active |
| MOCK-0015 | VS-0008 | REQ (E1 NWDAF Other Purpose SIOP) | E1 | SVC-0011 | active |
| MOCK-0016 | VS-0008 | RESP-2xx (200 snssaiInfoRspData) | E1 | DAT-0011/0012 | active |
| MOCK-0017 | VS-0009 | RESP-2xx (200 partial, candidateAmfList 생략) | E1 | MGMT-0001 | active |
| MOCK-0018 | VS-0010 | REQ (E2 PUT 정상) | E2 | SVC-0012, DAT-0013 | active |
| MOCK-0019 | VS-0010 | RESP-2xx (200 AuthorizedNssaiAvailabilityInfo) | E2 | – | active |
| MOCK-0020 | VS-0011 | REQ (E2 PUT 미지원 SNSSAI) | E2 | ERR-0010 | active |
| MOCK-0021 | VS-0011 | RESP-4xx (403 SNSSAI_NOT_SUPPORTED) | E2 | – | active |
| MOCK-0022 | VS-0012 | RESP-2xx (204 No Content) | E2 | DAT-0013 | active |
| MOCK-0023 | VS-0013 | RESP-4xx (415 Unsupported Media Type) | E2 | – | active |
| MOCK-0024 | VS-0014 | REQ (E3 PATCH 정상 add) | E3 | SVC-0013, DAT-0015 | active |
| MOCK-0025 | VS-0014 | RESP-2xx (200 갱신된 AuthorizedNssaiAvailabilityInfo) | E3 | – | active |
| MOCK-0026 | VS-0015 | REQ (E3 PATCH 미존재 nfId) | E3 | – | active |
| MOCK-0027 | VS-0015 | RESP-4xx (404 RESOURCE_NOT_FOUND) | E3 | DAT-0030, ERR-0010 | active |
| MOCK-0028 | VS-0016 | REQ (E3 PATCH 무결성 위반) | E3 | SVC-0042 | active |
| MOCK-0029 | VS-0016 | RESP-4xx (400 INVALID_IE invalidParams=[path]) | E3 | – | active |
| MOCK-0030 | VS-0017 | RESP-4xx (415) | E3 | – | active |
| MOCK-0031 | VS-0018 | REQ (E4 DELETE 정상) | E4 | SVC-0030 | active |
| MOCK-0032 | VS-0018 | RESP-2xx (204) | E4 | – | active |
| MOCK-0033 | VS-0019 | RESP-4xx (404 RESOURCE_NOT_FOUND) | E4 | ERR-0015 | active |
| MOCK-0034 | VS-0020 | REQ (E5 Subscribe 4 이벤트) | E5 | SVC-0014, DAT-0017/0018 | active |
| MOCK-0035 | VS-0020 | RESP-2xx (201 Created + Location) | E5 | – | active |
| MOCK-0036 | VS-0021 | REQ (E5 Subscribe 미지원 event) | E5 | – | active |
| MOCK-0037 | VS-0021 | RESP-4xx (501 UNSUPPORTED_EVENT_TYPE) | E5 | DAT-0033 | active |
| MOCK-0038 | VS-0022 | REQ (E5 callbackUri 누락) | E5 | – | active |
| MOCK-0039 | VS-0022 | RESP-4xx (400 MANDATORY_IE_MISSING) | E5 | ERR-0011, DAT-0032 | active |
| MOCK-0040 | VS-0023 | REQ (E5 동일 expiry 다중 구독) | E5 | SVC-0016 (M-Not 부정) | active |
| MOCK-0041 | VS-0023 | RESP-2xx (201 expiry 분산) | E5 | – | active |
| MOCK-0042 | VS-0024 | REQ (E6 PATCH SUMOD 정상) | E6 | SVC-0018/0019, DAT-0024 | active |
| MOCK-0043 | VS-0024 | RESP-2xx (200) | E6 | – | active |
| MOCK-0044 | VS-0025 | REQ (E6 event IE 변경 시도) | E6 | SVC-0020 (M-Not 부정), DAT-0025 (M-Not 부정), ERR-0012 | active |
| MOCK-0045 | VS-0025 | RESP-4xx (400 MODIFICATION_NOT_ALLOWED) | E6 | – | active |
| MOCK-0046 | VS-0026 | RESP-4xx (403 NOT_AUTHORIZED — SUMOD 미협상) | E6 | – | active |
| MOCK-0047 | VS-0027 | RESP-4xx (404 SUBSCRIPTION_NOT_FOUND) | E6 | – | active |
| MOCK-0048 | VS-0028 | REQ (E7 Unsubscribe) | E7 | SVC-0021 | active |
| MOCK-0049 | VS-0028 | RESP-2xx (204) | E7 | – | active |
| MOCK-0050 | VS-0029 | RESP-4xx (404 SUBSCRIPTION_NOT_FOUND) | E7 | ERR-0013 | active |
| MOCK-0051 | VS-0030 | REQ (E8 OPTIONS) | E8 | SVC-0032 | active |
| MOCK-0052 | VS-0030 | RESP-2xx (200 + Accept-Encoding) | E8 | – | active |
| MOCK-0053 | VS-0031 | CALLBACK (C1 Notify status change) | C1 | SVC-0023, DAT-0026, PRC-0001 | active |
| MOCK-0054 | VS-0031 | CALLBACK-RESP (204) | C1 | – | active |
| MOCK-0055 | VS-0032 | CALLBACK-RESP-4xx (400 RESOURCE_CONTEXT_NOT_FOUND) | C1 | DAT-0034, ERR-0014 | active |
| MOCK-0056 | VS-0033 | CALLBACK-RESP-4xx (404 RESOURCE_URI_STRUCTURE_NOT_FOUND) | C1 | DAT-0035 | active |
| MOCK-0057 | VS-0034 | CALLBACK-RESP-5xx (503 + Retry-After) | C1 | – | active |
| MOCK-0058 | VS-0035 | CALLBACK-RESP-3xx (307 Location) | C1 | DAT-0036 | active |
| MOCK-0059 | VS-0036 | CALLBACK (C1 Slice Replacement) | C1 | SVC-0027, PRC-0003 | active |
| MOCK-0060 | VS-0037 | CALLBACK (C1 Validity Time) | C1 | PRC-0002 | active |
| MOCK-0061 | VS-0038 | CALLBACK (C1 EANAN 빈 배열) | C1 | SEC-0002, SVC-0029 (M-Not 부정) | active |
| MOCK-0062 | VS-0039 | (negative test fixture) SEC-0001 위반 시도 (코드 거부) | C1 | SEC-0001 (M-Not 부정) | active |
| MOCK-0063 | VS-0040 | REQ (PROC-0001 NF Register) | – | – | active |
| MOCK-0064 | VS-0040 | RESP-2xx (201 NfProfile + heartBeatTimer) | – | – | active |
| MOCK-0065 | VS-0041 | RESP-5xx (503 NRF 일시 장애) | – | – | active |
| MOCK-0066 | VS-0042 | REQ (PROC-0003 Heartbeat PATCH) | – | – | active |
| MOCK-0067 | VS-0042 | RESP-2xx (204) | – | – | active |
| MOCK-0068 | VS-0043 | RESP-4xx (404 NRF lost reg) | – | – | active |
| MOCK-0069 | VS-0044 | REQ (PROC-0005 Token) | – | – | active |
| MOCK-0070 | VS-0044 | RESP-2xx (200 access_token) | – | – | active |
| MOCK-0071 | VS-0045 | RESP-4xx (401 token invalid) | – | – | active |
| MOCK-0072 | VS-0046 | (no message — internal timer) | – | – | active |
| MOCK-0073 | VS-0047 | (dedup 부정 시험 fixture) | C1 | SVC-0029 (M-Not) | active |
| MOCK-0074 | VS-0048 | (system-level e2e) | E1 + NRF | – | active |
| MOCK-0075 | VS-0049 | (system-level e2e Update+Notify) | E2 + C1 | – | active |
| MOCK-0076 | VS-0050 | (system-level e2e HR Roaming) | E1 + SEPP | – | active |
| MOCK-0077 | VS-0051 | (system-level restart 시험 fixture) | – | – | active |

## 2. 정상 페이로드

### 2.1 MOCK-0001 (VS-0001 E1 Initial Registration REQ)

- **출처**: VS-0001, api §2.1 E1, SVC-0002
- **요청 라인**: `GET /nnssf-nsselection/v2/network-slice-information?nf-type=AMF&nf-id=8e6f8a3c-1d4e-4f12-9a3b-7e5c2f4a1b0d&slice-info-request-for-registration=%7B%22subscribedNssai%22%3A%5B%7B%22subscribedSnssai%22%3A%7B%22sst%22%3A1%7D%7D%5D%2C%22requestedNssai%22%3A%5B%7B%22sst%22%3A1%7D%5D%7D&tai=%7B%22plmnId%22%3A%7B%22mcc%22%3A%22450%22%2C%22mnc%22%3A%2208%22%7D%2C%22tac%22%3A%22000001%22%7D&supported-features=F HTTP/2`
- **요청 헤더**: HEADER-001

slice-info-request-for-registration JSON (URL-디코딩한 형태):

```json
{
  "subscribedNssai": [
    {"subscribedSnssai": {"sst": 1}, "defaultIndication": true}
  ],
  "requestedNssai": [
    {"sst": 1}
  ],
  "defaultConfiguredSnssaiInd": false,
  "nsagSupported": false
}
```

### 2.2 MOCK-0002 (VS-0001 E1 200 RESP)

- **출처**: VS-0001, api §2.1 E1 2-C, DAT-0001~0004
- **상태**: `200 OK`
- **헤더**: `Content-Type: application/json`, `3gpp-Sbi-Sender-Timestamp: Mon, 26 May 2026 11:35:00 GMT`

```json
{
  "allowedNssaiList": [
    {
      "allowedSnssaiList": [
        {
          "allowedSnssai": {"sst": 1},
          "nsiInformationList": [
            {
              "nrfId": "https://nrf.internal:8443/nnrf-nfm/v1",
              "nsiId": "nsi-001-450-08-1"
            }
          ]
        }
      ],
      "accessType": "3GPP_ACCESS"
    }
  ],
  "configuredNssai": [
    {"configuredSnssai": {"sst": 1}}
  ],
  "targetAmfSet": "450-08-01-001",
  "supportedFeatures": "F"
}
```

### 2.3 MOCK-0009/0010 (VS-0005 E1 PDU Session HR roaming)

**MOCK-0009 (REQ)** — `slice-info-request-for-pdu-session`:

```json
{
  "sNssai": {"sst": 1},
  "roamingIndication": "HOME_ROUTED_ROAMING",
  "homeSnssai": {"sst": 1, "sd": "000001"}
}
```

**MOCK-0010 (RESP 200)**:

```json
{
  "allowedNssaiList": [{"allowedSnssaiList": [{"allowedSnssai": {"sst": 1}, "mappedHomeSnssai": {"sst": 1, "sd": "000001"}}], "accessType": "3GPP_ACCESS"}],
  "mappingOfNssai": [{"servingSnssai": {"sst": 1}, "homeSnssai": {"sst": 1, "sd": "000001"}}],
  "nsiInformation": {"nrfId": "https://h-nrf.example.com:8443/nnrf-nfm/v1"}
}
```

### 2.4 MOCK-0011/0012 (VS-0006 E1 PDN Connection RSIPCE)

**MOCK-0011 (REQ)** — query 의 `slice-info-request-for-pdn-connection`:

```json
[
  {"sst": 1},
  {"sst": 2, "sd": "000001"}
]
```

**MOCK-0012 (RESP 200)**:

```json
{
  "mappingOfNssai": [
    {"servingSnssai": {"sst": 1}, "homeSnssai": {"sst": 1, "sd": "000001"}},
    {"servingSnssai": {"sst": 2, "sd": "000001"}, "homeSnssai": {"sst": 2, "sd": "000001"}}
  ],
  "supportedFeatures": "4"
}
```

### 2.5 MOCK-0015/0016 (VS-0008 E1 NWDAF SIOP)

**MOCK-0015 (REQ)** — `slice-info-request-for-other-purpose`:

```json
[{"sst": 1}, {"sst": 2, "sd": "000001"}]
```

**MOCK-0016 (RESP 200)**:

```json
{
  "snssaiInfoRspData": {
    "sst=1": {"nsiIds": ["nsi-001-450-08-1"]},
    "sst=2&sd=000001": {"nsiIds": ["nsi-002-450-08-2"]}
  },
  "supportedFeatures": "8"
}
```

### 2.6 MOCK-0017 (VS-0009 E1 partial RESP — NRF discover 실패)

```json
{
  "allowedNssaiList": [{"allowedSnssaiList": [{"allowedSnssai": {"sst": 1}}], "accessType": "3GPP_ACCESS"}],
  "configuredNssai": [{"configuredSnssai": {"sst": 1}}],
  "targetAmfSet": "450-08-01-001",
  "supportedFeatures": "F"
}
```

> `candidateAmfList` 생략 — NRF 5xx 응답 시 partial response 정책.

### 2.7 MOCK-0018 (VS-0010 E2 PUT REQ)

- **요청 라인**: `PUT /nnssf-nssaiavailability/v1/nssai-availability/8e6f8a3c-1d4e-4f12-9a3b-7e5c2f4a1b0d HTTP/2`
- **헤더**: HEADER-002

```json
{
  "supportedNssaiAvailabilityData": [
    {
      "tai": {"plmnId": {"mcc": "450", "mnc": "08"}, "tac": "000001"},
      "supportedSnssaiList": [
        {"sst": 1},
        {"sst": 2, "sd": "000001"}
      ]
    }
  ],
  "supportedFeatures": "78",
  "amfSetId": "450-08-01-001"
}
```

### 2.8 MOCK-0019 (VS-0010 E2 200 RESP)

```json
{
  "authorizedNssaiAvailabilityData": [
    {
      "tai": {"plmnId": {"mcc": "450", "mnc": "08"}, "tac": "000001"},
      "supportedSnssaiList": [{"sst": 1}, {"sst": 2, "sd": "000001"}],
      "restrictedSnssaiList": []
    }
  ],
  "supportedFeatures": "78"
}
```

### 2.9 MOCK-0022 (VS-0012 E2 204 No Content)

```
HTTP/2 204 No Content
3gpp-Sbi-Sender-Timestamp: Mon, 26 May 2026 11:35:00 GMT
```

(empty body)

### 2.10 MOCK-0024 (VS-0014 E3 PATCH REQ)

- **헤더**: `Content-Type: application/json-patch+json`

```json
[
  {"op": "add", "path": "/supportedNssaiAvailabilityData/0/supportedSnssaiList/-", "value": {"sst": 3, "sd": "00000A"}}
]
```

### 2.11 MOCK-0025 (VS-0014 E3 200 RESP)

```json
{
  "authorizedNssaiAvailabilityData": [
    {
      "tai": {"plmnId": {"mcc": "450", "mnc": "08"}, "tac": "000001"},
      "supportedSnssaiList": [{"sst": 1}, {"sst": 2, "sd": "000001"}, {"sst": 3, "sd": "00000A"}]
    }
  ],
  "supportedFeatures": "78"
}
```

### 2.12 MOCK-0031/0032 (VS-0018 E4 DELETE)

**REQ**: `DELETE /nnssf-nssaiavailability/v1/nssai-availability/8e6f8a3c-1d4e-4f12-9a3b-7e5c2f4a1b0d HTTP/2`

**RESP**: `204 No Content` (empty body)

### 2.13 MOCK-0034 (VS-0020 E5 Subscribe REQ)

```json
{
  "nfNssaiAvailabilityUri": "https://amf.consumer.example.com:8443/namf-callback/v1/nssai-availability/cb-uuid-9999",
  "taiList": [{"plmnId": {"mcc": "450", "mnc": "08"}, "tac": "000001"}],
  "event": "SNSSAI_STATUS_CHANGE_REPORT",
  "additionalEvents": ["SNSSAI_REPLACEMENT_REPORT", "NSI_UNAVAILABILITY_REPORT", "SNSSAI_VALIDITY_TIME_REPORT"],
  "expiry": "2026-05-27T11:35:00Z",
  "amfSetId": "450-08-01-001",
  "supportedFeatures": "78",
  "validityTimeSubList": [{"sst": 1}]
}
```

### 2.14 MOCK-0035 (VS-0020 E5 201 RESP)

- **상태**: `201 Created`
- **헤더**: `Location: /nnssf-nssaiavailability/v1/nssai-availability/subscriptions/sub-7e5c2f4a-1b0d-4e91-a3b7-1234567890ab`, `Content-Type: application/json`

```json
{
  "subscriptionId": "sub-7e5c2f4a-1b0d-4e91-a3b7-1234567890ab",
  "expiry": "2026-05-27T11:35:00Z",
  "acceptedEvents": ["SNSSAI_STATUS_CHANGE_REPORT", "SNSSAI_REPLACEMENT_REPORT", "NSI_UNAVAILABILITY_REPORT", "SNSSAI_VALIDITY_TIME_REPORT"],
  "supportedFeatures": "78"
}
```

### 2.15 MOCK-0042 (VS-0024 E6 PATCH SUMOD)

- **헤더**: `Content-Type: application/json-patch+json`

```json
[
  {"op": "replace", "path": "/taiList/0/tac", "value": "000002"},
  {"op": "replace", "path": "/expiry", "value": "2026-05-28T11:35:00Z"}
]
```

### 2.16 MOCK-0043 (VS-0024 E6 200 RESP)

```json
{
  "subscriptionId": "sub-7e5c2f4a-1b0d-4e91-a3b7-1234567890ab",
  "expiry": "2026-05-28T11:35:00Z",
  "supportedFeatures": "78",
  "acceptedEvents": ["SNSSAI_STATUS_CHANGE_REPORT", "SNSSAI_REPLACEMENT_REPORT", "NSI_UNAVAILABILITY_REPORT", "SNSSAI_VALIDITY_TIME_REPORT"]
}
```

### 2.17 MOCK-0051/0052 (VS-0030 E8 OPTIONS)

**REQ**: `OPTIONS /nnssf-nssaiavailability/v1/nssai-availability HTTP/2`

**RESP 200**: `Accept-Encoding: gzip` (empty body)

### 2.18 MOCK-0053/0054 (VS-0031 C1 Notify Status Change)

**MOCK-0053 (CALLBACK REQ)** — NSSF → AMF callback URI:

- **요청 라인**: `POST https://amf.consumer.example.com:8443/namf-callback/v1/nssai-availability/cb-uuid-9999 HTTP/2`
- **헤더**: HEADER-003

```json
{
  "subscriptionId": "sub-7e5c2f4a-1b0d-4e91-a3b7-1234567890ab",
  "authorizedNssaiAvailabilityData": [
    {
      "tai": {"plmnId": {"mcc": "450", "mnc": "08"}, "tac": "000001"},
      "supportedSnssaiList": [{"sst": 1}],
      "restrictedSnssaiList": [
        {"homePlmnId": {"mcc": "999", "mnc": "99"}, "sNssaiList": [{"sst": 2, "sd": "000001"}]}
      ]
    }
  ]
}
```

**MOCK-0054**: `204 No Content` (Consumer 응답)

### 2.19 MOCK-0059 (VS-0036 C1 Notify Slice Replacement, NSRP)

```json
{
  "subscriptionId": "sub-7e5c2f4a-1b0d-4e91-a3b7-1234567890ab",
  "altNssai": [
    {
      "snssai": {"sst": 2, "sd": "000001"},
      "alternativeSnssai": {"sst": 2, "sd": "00000A"},
      "status": "REPLACEMENT_STARTED"
    }
  ]
}
```

### 2.20 MOCK-0060 (VS-0037 C1 Notify Validity Time)

```json
{
  "subscriptionId": "sub-7e5c2f4a-1b0d-4e91-a3b7-1234567890ab",
  "nssaiValidityTimeInfoList": {
    "sst=1": [{"timeOfDay": "08:00:00", "daysOfWeek": [1, 2, 3, 4, 5]}, {"timeOfDay": "18:00:00", "daysOfWeek": [1, 2, 3, 4, 5]}]
  }
}
```

### 2.21 MOCK-0061 (VS-0038 C1 Notify EANAN — empty array)

```json
{
  "subscriptionId": "sub-7e5c2f4a-1b0d-4e91-a3b7-1234567890ab",
  "authorizedNssaiAvailabilityData": []
}
```

### 2.22 MOCK-0063/0064 (VS-0040 PROC-0001 NF Register)

**MOCK-0063 (REQ)** — `PUT /nnrf-nfm/v1/nf-instances/8e6f8a3c-1d4e-4f12-9a3b-7e5c2f4a1b0d HTTP/2`:

```json
{
  "nfInstanceId": "8e6f8a3c-1d4e-4f12-9a3b-7e5c2f4a1b0d",
  "nfType": "NSSF",
  "nfStatus": "REGISTERED",
  "plmnList": [{"mcc": "450", "mnc": "08"}],
  "fqdn": "nssf.internal",
  "ipv4Addresses": ["10.0.1.10"],
  "ipv6Addresses": [],
  "nfServices": [
    {
      "serviceInstanceId": "nssf-nsselection-01",
      "serviceName": "nnssf-nsselection",
      "versions": [{"apiVersionInUri": "v2", "apiFullVersion": "2.4.0"}],
      "scheme": "https",
      "nfServiceStatus": "REGISTERED",
      "ipEndPoints": [{"ipv4Address": "10.0.1.10", "port": 8443}]
    },
    {
      "serviceInstanceId": "nssf-nssaiavail-01",
      "serviceName": "nnssf-nssaiavailability",
      "versions": [{"apiVersionInUri": "v1", "apiFullVersion": "1.4.0"}],
      "scheme": "https",
      "nfServiceStatus": "REGISTERED",
      "ipEndPoints": [{"ipv4Address": "10.0.1.10", "port": 8443}]
    }
  ],
  "nssfInfo": {
    "supportedNssaiAvailabilityData": [{"tai": {"plmnId": {"mcc": "450", "mnc": "08"}, "tac": "000001"}, "supportedSnssaiList": [{"sst": 1}, {"sst": 2, "sd": "000001"}]}]
  }
}
```

**MOCK-0064 (RESP 201)** — body = MOCK-0063 + `heartBeatTimer: 60`:

```json
{
  "nfInstanceId": "8e6f8a3c-1d4e-4f12-9a3b-7e5c2f4a1b0d",
  "nfType": "NSSF",
  "nfStatus": "REGISTERED",
  "heartBeatTimer": 60,
  "plmnList": [{"mcc": "450", "mnc": "08"}]
}
```

### 2.23 MOCK-0066/0067 (VS-0042 PROC-0003 Heartbeat)

**MOCK-0066 (REQ)** — `PATCH /nnrf-nfm/v1/nf-instances/8e6f8a3c-... HTTP/2`, `Content-Type: application/json-patch+json`:

```json
[
  {"op": "replace", "path": "/nfStatus", "value": "REGISTERED"}
]
```

**MOCK-0067**: `204 No Content`

### 2.24 MOCK-0069/0070 (VS-0044 PROC-0005 Token)

**MOCK-0069 (REQ)** — `POST /oauth2/token HTTP/2`, `Content-Type: application/x-www-form-urlencoded`:

```
grant_type=client_credentials&nfInstanceId=8e6f8a3c-1d4e-4f12-9a3b-7e5c2f4a1b0d&nfType=NSSF&targetNfType=AMF&scope=namf-callback
```

**MOCK-0070 (RESP 200)**:

```json
{
  "access_token": "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiI4ZTZmOGEzYy0xZDRlLTRmMTItOWEzYi03ZTVjMmY0YTFiMGQiLCJpc3MiOiJucmYiLCJhdWQiOiJBTUYiLCJzY29wZSI6Im5hbWYtY2FsbGJhY2siLCJleHAiOjE3NzgyMzM5MDB9.signature_bytes",
  "token_type": "Bearer",
  "expires_in": 3600,
  "scope": "namf-callback"
}
```

## 3. 부정 페이로드

### 3.1 부정 페이로드 (M-Not 매핑 + 에러 cause)

| MOCK-ID | 부정 분류 | HTTP·cause | 출처 (features.md / api §6) |
|---|---|---|---|
| MOCK-0004 | 에러 cause | 403 SNSSAI_NOT_SUPPORTED | api §6.1 E1, ERR-0001 |
| MOCK-0006 | 에러 cause | 401 TOKEN_EXPIRED | api §6.1 E1 |
| MOCK-0008 | 에러 cause | 400 MANDATORY_QUERY_PARAM_MISSING | api §6.1 E1 |
| MOCK-0014 | 에러 cause | 400 FEATURE_NOT_SUPPORTED (RSIPCE 미협상) | api §6.1 |
| MOCK-0021 | 에러 cause | 403 SNSSAI_NOT_SUPPORTED | api §6.2 E2, ERR-0010 |
| MOCK-0023 | 에러 cause | 415 Unsupported Media Type | api §6.2 |
| MOCK-0027 | 에러 cause | 404 RESOURCE_NOT_FOUND | api §6.2 E3, DAT-0030, ERR-0010 |
| MOCK-0029 | 에러 cause | 400 INVALID_IE invalidParams=[path] | api §6.2 (PATCH 무결성) |
| MOCK-0030 | 에러 cause | 415 (E3 Content-Type 불일치) | api §6.2 |
| MOCK-0033 | 에러 cause | 404 RESOURCE_NOT_FOUND | api §6.3 E4, ERR-0015 |
| MOCK-0037 | 에러 cause | 501 UNSUPPORTED_EVENT_TYPE | api §6.4 E5, DAT-0033 |
| MOCK-0039 | 에러 cause | 400 MANDATORY_IE_MISSING invalidParams=[nfNssaiAvailabilityUri] | api §6.4 E5, ERR-0011, DAT-0032 |
| MOCK-0040 | M-Not 부정 (SVC-0016) | (요청 fixture, 응답은 NSSF 가 expiry 분산 처리 검증) | SVC-0016 |
| MOCK-0044 | M-Not 부정 (SVC-0020, DAT-0025) | (요청 fixture — event IE 수정 시도) | SVC-0020, DAT-0025 |
| MOCK-0045 | 에러 cause | 400 MODIFICATION_NOT_ALLOWED | api §6.5 E6, ERR-0012 |
| MOCK-0046 | 에러 cause | 403 NOT_AUTHORIZED (SUMOD 미협상) | api §6.5 E6 |
| MOCK-0047 | 에러 cause | 404 SUBSCRIPTION_NOT_FOUND | api §6.5 E6 |
| MOCK-0050 | 에러 cause | 404 SUBSCRIPTION_NOT_FOUND | api §6.6 E7, ERR-0013 |
| MOCK-0055 | 에러 cause | 400 RESOURCE_CONTEXT_NOT_FOUND | api §6.8 C1, DAT-0034, ERR-0014 |
| MOCK-0056 | 에러 cause | 404 RESOURCE_URI_STRUCTURE_NOT_FOUND | api §6.8 C1, DAT-0035 |
| MOCK-0057 | 에러 cause | 503 + Retry-After | api §6.8 C1 |
| MOCK-0062 | M-Not 부정 (SEC-0001) | (negative test — NSSF code 가 송신 거부) | SEC-0001 |
| MOCK-0065 | 에러 cause | 503 NRF (재시도) | – |
| MOCK-0068 | 에러 cause | 404 NRF lost reg | – |
| MOCK-0071 | 에러 cause | 401 token invalid | – |
| MOCK-0073 | M-Not 부정 (SVC-0029) | (negative — 중복 Notify dedup 검증 fixture) | SVC-0029 |

### 3.2 ProblemDetails 예시

#### MOCK-0004 (E1 403 SNSSAI_NOT_SUPPORTED)

```json
{
  "type": "/3gpp/29531/error",
  "title": "S-NSSAI not supported",
  "status": 403,
  "detail": "The requested slice selection information is for S-NSSAI(s) not supported by the NSSF.",
  "cause": "SNSSAI_NOT_SUPPORTED",
  "instance": "/nnssf-nsselection/v2/network-slice-information"
}
```

#### MOCK-0006 (E1 401 TOKEN_EXPIRED)

```json
{
  "type": "/3gpp/29571/error",
  "title": "Token expired",
  "status": 401,
  "detail": "Access token has expired.",
  "cause": "TOKEN_EXPIRED"
}
```

#### MOCK-0008 (E1 400 MANDATORY_QUERY_PARAM_MISSING)

```json
{
  "type": "/3gpp/29571/error",
  "title": "Mandatory query parameter missing",
  "status": 400,
  "detail": "Required query parameter is missing in the request.",
  "cause": "MANDATORY_QUERY_PARAM_MISSING",
  "invalidParams": [
    {"param": "nf-id", "reason": "Required query parameter nf-id is missing"}
  ]
}
```

#### MOCK-0014 (E1 400 FEATURE_NOT_SUPPORTED)

```json
{
  "type": "/3gpp/29571/error",
  "title": "Feature not supported",
  "status": 400,
  "detail": "slice-info-request-for-pdn-connection requires RSIPCE feature negotiation.",
  "cause": "FEATURE_NOT_SUPPORTED",
  "invalidParams": [{"param": "supportedFeatures", "reason": "RSIPCE bit (#3) not set"}]
}
```

#### MOCK-0021 (E2 403 SNSSAI_NOT_SUPPORTED)

```json
{
  "type": "/3gpp/29531/error",
  "title": "S-NSSAI not supported",
  "status": 403,
  "detail": "The S-NSSAI provided in the request is not supported in the PLMN.",
  "cause": "SNSSAI_NOT_SUPPORTED"
}
```

#### MOCK-0023 (E2 415 Unsupported Media Type)

```json
{
  "type": "/3gpp/29571/error",
  "title": "Unsupported Media Type",
  "status": 415,
  "detail": "Content-Type must be application/json.",
  "cause": "UNSUPPORTED_MEDIA_TYPE"
}
```

#### MOCK-0027 (E3 404 RESOURCE_NOT_FOUND)

```json
{
  "type": "/3gpp/29531/error",
  "title": "Resource not found",
  "status": 404,
  "detail": "NSSAI Availability resource for the given nfId does not exist (PATCH requires prior PUT).",
  "cause": "RESOURCE_NOT_FOUND"
}
```

#### MOCK-0029 (E3 400 INVALID_IE invalidParams=[path])

```json
{
  "type": "/3gpp/29571/error",
  "title": "Invalid Information Element",
  "status": 400,
  "detail": "JSON Patch path is invalid or targets a non-existent node.",
  "cause": "INVALID_IE",
  "invalidParams": [
    {"param": "path", "reason": "/invalid/path does not exist in target document"}
  ]
}
```

#### MOCK-0033 (E4 404 RESOURCE_NOT_FOUND)

```json
{"type": "/3gpp/29531/error", "title": "Resource not found", "status": 404, "detail": "AMF NSSAI Availability context not found for given nfId.", "cause": "RESOURCE_NOT_FOUND"}
```

#### MOCK-0037 (E5 501 UNSUPPORTED_EVENT_TYPE)

```json
{
  "type": "/3gpp/29531/error",
  "title": "Unsupported event type",
  "status": 501,
  "detail": "None of the requested events are supported by the NSSF.",
  "cause": "UNSUPPORTED_EVENT_TYPE",
  "invalidParams": [{"param": "event", "reason": "event 'UNKNOWN_X' not supported"}]
}
```

#### MOCK-0039 (E5 400 MANDATORY_IE_MISSING)

```json
{
  "type": "/3gpp/29571/error",
  "title": "Mandatory IE missing",
  "status": 400,
  "detail": "Required field nfNssaiAvailabilityUri is missing in the request body.",
  "cause": "MANDATORY_IE_MISSING",
  "invalidParams": [{"param": "nfNssaiAvailabilityUri", "reason": "missing"}]
}
```

#### MOCK-0045 (E6 400 MODIFICATION_NOT_ALLOWED)

```json
{
  "type": "/3gpp/29531/error",
  "title": "Modification not allowed",
  "status": 400,
  "detail": "The event IE cannot be modified via PATCH (SVC-0020 / DAT-0025 SHALL NOT).",
  "cause": "MODIFICATION_NOT_ALLOWED",
  "invalidParams": [{"param": "/event", "reason": "event IE is immutable"}]
}
```

#### MOCK-0046 (E6 403 NOT_AUTHORIZED — SUMOD 미협상)

```json
{
  "type": "/3gpp/29531/error",
  "title": "Not authorized",
  "status": 403,
  "detail": "Subscription modification requires SUMOD feature negotiation.",
  "cause": "NOT_AUTHORIZED"
}
```

#### MOCK-0047 / MOCK-0050 (E6/E7 404 SUBSCRIPTION_NOT_FOUND)

```json
{
  "type": "/3gpp/29531/error",
  "title": "Subscription not found",
  "status": 404,
  "detail": "Subscription with the given subscriptionId does not exist.",
  "cause": "SUBSCRIPTION_NOT_FOUND"
}
```

#### MOCK-0055 (C1 callback 응답: 400 RESOURCE_CONTEXT_NOT_FOUND)

Consumer (AMF) 가 NSSF callback 에 응답:

```json
{
  "type": "/3gpp/29531/error",
  "title": "Resource context not found",
  "status": 400,
  "detail": "Subscription context not found at NF Service Consumer.",
  "cause": "RESOURCE_CONTEXT_NOT_FOUND"
}
```

#### MOCK-0056 (C1 callback 응답: 404 RESOURCE_URI_STRUCTURE_NOT_FOUND)

```json
{
  "type": "/3gpp/29531/error",
  "title": "Resource URI structure not found",
  "status": 404,
  "detail": "Callback URI is not known to the NF Service Consumer.",
  "cause": "RESOURCE_URI_STRUCTURE_NOT_FOUND"
}
```

#### MOCK-0057 (C1 callback 응답: 503 + Retry-After)

- **헤더**: `Retry-After: 5`

```json
{
  "type": "/3gpp/29571/error",
  "title": "Service Unavailable",
  "status": 503,
  "detail": "Consumer NF temporarily unavailable.",
  "cause": "SERVICE_UNAVAILABLE"
}
```

#### MOCK-0058 (C1 callback 응답: 307 Location)

- **상태**: `307 Temporary Redirect`
- **헤더**: `Location: https://amf-2.consumer.example.com:8443/namf-callback/v1/nssai-availability/cb-uuid-9999`

(empty body)

## 4. 경계값 페이로드

### 4.1 Snssai 제약 (TS 29.571 §5.4.4.2)

| MOCK-ID | 필드 | 제약 | 값 | 통과/실패 |
|---|---|---|---|---|
| MOCK-B-0001 | `sst` | min=0 | `{"sst": 0}` | 통과 |
| MOCK-B-0002 | `sst` | max=255 | `{"sst": 255}` | 통과 |
| MOCK-B-0003 | `sst` | max+1 (256) | `{"sst": 256}` | 실패 (400 INVALID_IE) |
| MOCK-B-0004 | `sst` | type=int | `{"sst": "1"}` (string) | 실패 |
| MOCK-B-0005 | `sd` | pattern `^[A-Fa-f0-9]{6}$` 통과 | `{"sst": 1, "sd": "ABCDEF"}` | 통과 |
| MOCK-B-0006 | `sd` | pattern 실패 (길이) | `{"sst": 1, "sd": "ABCDE"}` (5자) | 실패 (400 invalidParams=[sd]) |
| MOCK-B-0007 | `sd` | pattern 실패 (문자) | `{"sst": 1, "sd": "GHIJKL"}` | 실패 |

### 4.2 Tai 제약 (TS 29.571 §5.4.4.4)

| MOCK-ID | 필드 | 제약 | 값 | 통과/실패 |
|---|---|---|---|---|
| MOCK-B-0010 | `tac` | 6 hex | `"000001"` | 통과 |
| MOCK-B-0011 | `tac` | 4 hex (legacy) | `"0001"` | 통과 |
| MOCK-B-0012 | `tac` | length 5 (실패) | `"00001"` | 실패 |
| MOCK-B-0013 | `mcc`/`mnc` | digits only | `{"mcc": "12A", "mnc": "08"}` | 실패 |

### 4.3 NfInstanceId 제약 (RFC 4122)

| MOCK-ID | 값 | 통과/실패 |
|---|---|---|
| MOCK-B-0020 | `8e6f8a3c-1d4e-4f12-9a3b-7e5c2f4a1b0d` | 통과 |
| MOCK-B-0021 | `8e6f8a3c-1d4e-4f12-9a3b` (불완전) | 실패 |
| MOCK-B-0022 | `not-a-uuid` | 실패 |

### 4.4 supportedFeatures 제약

| MOCK-ID | 값 | 통과/실패 |
|---|---|---|
| MOCK-B-0030 | `"F"` (NSSelection 4비트 전체) | 통과 |
| MOCK-B-0031 | `"0"` (NSSelection 협상 결과 — ES3XX(M) 미포함 시 server 측 거부) | 실패 (FEATURE_NOT_SUPPORTED — ES3XX M) |
| MOCK-B-0032 | `"GHI"` (hex 외 문자) | 실패 |
| MOCK-B-0033 | `""` (빈 문자열) | 실패 |
| MOCK-B-0034 | `"FFFFFFFFFFFFFFFFFFFF"` (overflow) | 구현 정책 (보통 무시 또는 truncate) |

### 4.5 amfSetId pattern

| MOCK-ID | 값 | 통과/실패 |
|---|---|---|
| MOCK-B-0040 | `"450-08-01-001"` | 통과 |
| MOCK-B-0041 | `"450-08-G1-001"` (G hex 외) | 실패 |
| MOCK-B-0042 | `"450-8-01-001"` (mnc 2자만, OK per pattern) | 통과 |

### 4.6 NssaiAvailabilityInfo `supportedNssaiAvailabilityData` 카디

| MOCK-ID | 값 | 통과/실패 |
|---|---|---|
| MOCK-B-0050 | minItems=1 (1건) | 통과 |
| MOCK-B-0051 | 빈 배열 `[]` | 실패 (minItems 위반, 400 MANDATORY_IE_MISSING) |
| MOCK-B-0052 | M 필드 `tai` 누락 | 실패 (400 invalidParams=[tai]) |

### 4.7 callback URI 제약

| MOCK-ID | 값 | 통과/실패 |
|---|---|---|
| MOCK-B-0060 | `"https://amf.example.com:8443/callback"` | 통과 |
| MOCK-B-0061 | `"http://insecure.example.com"` (TLS X) | 실패 (운영 정책: HTTPS 만) |
| MOCK-B-0062 | `"not-a-uri"` | 실패 |

## 5. 특수 패턴

### 5-A. Subscribe/Notify

본 §2.13~2.21 + §3.x 에 포함됨. 추가 페이로드:

#### Subscription expiry 분산 (SVC-0016 M-Not 검증)

요청 N개 (각 다른 consumer) 모두 expiry=`2026-05-27T11:35:00Z` 명시 → NSSF 가 expiry 를 다른 timestamp 로 분산 부여 (jitter ±N초). 동일 timestamp 가 다수 부여되지 않음.

### 5-B. PATCH

- **JSON Patch 적용 endpoint**: E3 (NSSAIAvailability_Update) / E6 (Subscribe_Modify)
- **Content-Type**: `application/json-patch+json` (RFC 6902)
- **Merge Patch**: 본 NF 미적용 (해당 없음 — api §7-B 확인)

추가 부정 케이스 fixture:

```json
[
  {"op": "test", "path": "/supportedFeatures", "value": "78"},
  {"op": "replace", "path": "/supportedFeatures", "value": "FF"}
]
```

→ test 가 통과해야 replace 진행. test 실패 시 412 Precondition Failed.

### 5-C. supportedFeatures 비트맵

#### NSSelection (api §7-C)

| MOCK-ID | hex | 활성 비트 | 의미 |
|---|---|---|---|
| MOCK-SF-0001 | `"F"` | 1,2,3,4 모두 | ES3XX + TargetNssai + RSIPCE + SIOP |
| MOCK-SF-0002 | `"1"` | 1 | ES3XX (M only) |
| MOCK-SF-0003 | `"5"` | 1, 3 | ES3XX + RSIPCE |
| MOCK-SF-0004 | `"9"` | 1, 4 | ES3XX + SIOP |
| MOCK-SF-0005 | `"3"` | 1, 2 | ES3XX + TargetNssai |

#### NSSAIAvailability (api §7-C, 7비트)

| MOCK-ID | hex | 활성 비트 | 의미 |
|---|---|---|---|
| MOCK-SF-0010 | `"7F"` | 1~7 모두 | ONSSAI+SUMOD+EANAN+ES3XX+SATAS+NSIUN+NSRP |
| MOCK-SF-0011 | `"8"` | 4 | ES3XX (M only) |
| MOCK-SF-0012 | `"78"` | 4, 5, 6, 7 | ES3XX + SATAS + NSIUN + NSRP (Rel-18 보강 기본 권장) |
| MOCK-SF-0013 | `"A"` | 2, 4 | SUMOD + ES3XX |
| MOCK-SF-0014 | `"C"` | 3, 4 | EANAN + ES3XX |

## 6. 헤더 인용 표

### HEADER-001 (E1 GET 요청 — Initial Registration)

| 키 | 값 |
|---|---|
| `Authorization` | `Bearer eyJhbGciOiJSUzI1NiIs...` (NRF 발급 token) |
| `Accept` | `application/json, application/problem+json` |
| `User-Agent` | `AMF/1.0` |
| `3gpp-Sbi-Sender-Timestamp` | `Mon, 26 May 2026 11:35:00 GMT` |
| `3gpp-Sbi-Max-Rsp-Time` | `5000` (ms) |
| `3gpp-Sbi-Discovery-target-nf-type` | `NSSF` |

### HEADER-002 (E2 PUT 요청)

| 키 | 값 |
|---|---|
| `Authorization` | `Bearer eyJ...` |
| `Content-Type` | `application/json` |
| `Accept` | `application/json, application/problem+json` |
| `Content-Encoding` | `gzip` (선택) |
| `Accept-Encoding` | `gzip` |
| `3gpp-Sbi-Sender-Timestamp` | `Mon, 26 May 2026 11:35:00 GMT` |

### HEADER-003 (C1 NSSF→Consumer Notify 송신)

| 키 | 값 |
|---|---|
| `Authorization` | `Bearer eyJ...` (Consumer 측 NRF 발급 token, scope=namf-callback) |
| `Content-Type` | `application/json` |
| `3gpp-Sbi-Sender-Timestamp` | `Mon, 26 May 2026 11:35:00 GMT` |
| `3gpp-Sbi-Producer-Id` | NSSF NfInstanceId |
| `3gpp-Sbi-Callback` | `Nnssf_NSSAIAvailability_Notify` |

### HEADER-004 (E3/E6 PATCH 요청)

| 키 | 값 |
|---|---|
| `Authorization` | `Bearer eyJ...` |
| `Content-Type` | `application/json-patch+json` |
| `Accept` | `application/json, application/problem+json` |

### Callback 페이로드 인용 표

| MOCK-ID | callbackUri 형식 | Notify 메시지 모델 | 인증 |
|---|---|---|---|
| MOCK-0053 | `{nfNssaiAvailabilityUri}` (구독 시 등록) — 예 `https://amf.consumer.example.com:8443/namf-callback/v1/nssai-availability/cb-uuid-9999` | NssfEventNotification (authorizedNssaiAvailabilityData) | OAuth2 (Consumer 측 NRF) + mTLS |
| MOCK-0059 | 동 | NssfEventNotification (altNssai, NSRP feature) | 동 |
| MOCK-0060 | 동 | NssfEventNotification (nssaiValidityTimeInfoList) | 동 |
| MOCK-0061 | 동 | NssfEventNotification (authorizedNssaiAvailabilityData=[], EANAN feature) | 동 |

## 7-bis. 운영 conf bundle (R11 — impl_spec §8-E 인용)

### MOCK-CFG-0001 (cfg 페이로드 — 운영 변형)

- **출처**: [impl_spec §3-C-5-1](../implementation-specs/NSSF-impl-spec.md) + [db-design §8-C 시드](../implementation-specs/NSSF-db-design.md)
- **산출물 위치**: `dev/conf/samples/nssfd.json.example` (확장자 `.json` — cJSON 파서 결정)
- **본문**: impl_spec §3-C-5-1 운영 변형 JSON 전체 (재기술 없이 인용 — 본 산출물에서 변경 시 impl_spec 갱신 필요)

### MOCK-CFG-0002 (cfg 페이로드 — dev 변형)

- **출처**: [impl_spec §3-C-5-2](../implementation-specs/NSSF-impl-spec.md)
- **산출물 위치**: `dev/conf/nssfd.dev.json`
- **본문**: impl_spec §3-C-5-2 dev 변형 JSON 전체 (재기술 없이 인용)

### MOCK-CFG-0003 (운영↔dev 차이 표)

| 영역 | 운영 (MOCK-CFG-0001) | dev (MOCK-CFG-0002) |
|---|---|---|
| `db.conninfo` | `sslmode=require`, 운영 DSN | `127.0.0.1`, `sslmode=disable`, `nssf_dev` |
| `http_srv.bind_port` | 8443 | 18443 |
| `http_srv.tls.mtls_required` | true | false |
| `http_srv.tls.cert_path` 등 | `/etc/nssfd/tls/*` | `dev/conf/samples/tls/*` |
| `auth.oauth2.enabled` | true | false |
| `auth.tls.verify_peer` | true | false |
| `nrf.base_url` | https://nrf.internal:8443 | http://127.0.0.1:18000 |
| `seed.*` | 운영 PLMN 450-08 + 다중 슬라이스 | 최소 (PLMN 001-01, 슬라이스 1개) |
| `life.shutdown_grace_sec` | 30 | 5 |

### MOCK-LOG-0001 (log conf — 운영)

- **출처**: [impl_spec §8-C-1 운영 변형](../implementation-specs/NSSF-impl-spec.md)
- **산출물 위치**: `/etc/nssfd/log.conf`
- **본문**: impl_spec §8-C-1 운영 변형 zlog conf 전체 (재기술 없이 인용)

### MOCK-LOG-0002 (log conf — dev)

- **출처**: [impl_spec §8-C-1 dev 변형](../implementation-specs/NSSF-impl-spec.md)
- **산출물 위치**: `dev/conf/samples/log.dev.conf`
- **본문**: impl_spec §8-C-1 dev 변형 (stdout/stderr only)

### MOCK-TLS-0001 (dummy TLS bundle)

- **출처**: [impl_spec §3-C-6](../implementation-specs/NSSF-impl-spec.md)
- **산출물 위치**: `dev/conf/samples/tls/{ca.crt, ca.key, server.crt, server.key, client.crt, client.key, nrf_pubkey.pem, nrf_priv.pem}`
- **gen 스크립트 위치**: `dev/scripts/gen-dummy-tls.sh`
- **스크립트 본문**: impl_spec §3-C-6-2 bash 블록 (idempotent 보강 — 기존 cert 보존, `--force` 시 덮어쓰기)

```bash
#!/usr/bin/env bash
# dev/scripts/gen-dummy-tls.sh
# 더미 TLS bundle 생성 (idempotent)
set -euo pipefail

TLS_DIR="${TLS_DIR:-dev/conf/samples/tls}"
FORCE="${1:-}"

mkdir -p "${TLS_DIR}"
cd "${TLS_DIR}"

if [[ -f ca.crt && "$FORCE" != "--force" ]]; then
    echo "TLS bundle exists at ${TLS_DIR}. Pass --force to regenerate."
    exit 0
fi

openssl req -x509 -newkey rsa:2048 -nodes -keyout ca.key -out ca.crt -days 365 \
    -subj "/CN=NSSF-DEV-CA"

openssl req -newkey rsa:2048 -nodes -keyout server.key -out server.csr \
    -subj "/CN=nssf.dev.local"
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
    -out server.crt -days 365 -extfile <(printf "subjectAltName=DNS:nssf.dev.local,IP:127.0.0.1")

openssl req -newkey rsa:2048 -nodes -keyout client.key -out client.csr \
    -subj "/CN=nssf-client"
openssl x509 -req -in client.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
    -out client.crt -days 365

openssl ecparam -name prime256v1 -genkey -noout -out nrf_priv.pem
openssl ec -in nrf_priv.pem -pubout -out nrf_pubkey.pem

chmod 400 *.key nrf_priv.pem
rm -f *.csr *.srl

echo "Generated dummy TLS bundle in ${TLS_DIR}"
```

### MOCK-TLS-0002 (.gitignore 룰)

`.gitignore` 항목 (impl_spec §3-C-6-1 인용):

```
# 운영 cert/key 추적 금지
/etc/nssfd/tls/
dev/conf/tls/

# sample bundle 만 추적 허용
!dev/conf/samples/tls/
```

## 7. 자체 완료 체크리스트

| 점검 항목 | 합격 기준 | 결과 |
|---|---|---|
| 0단계 입구 점검 통과 | 5 산출물 + impl_spec 체크리스트 + VS 표 모두 | ✅ |
| MOCK-ID 형식 | 모든 ID `MOCK-[0-9]{4}` 또는 `MOCK-(B|SF|CFG|LOG|TLS)-[0-9]{4}` | ✅ (77 + 경계값 + 특수 + R11) |
| VS-ID 매핑 완전성 | impl_spec VS-0001~VS-0051 모두 ≥1 MOCK | ✅ (1:N 매핑) |
| 부정 페이로드 의무 | features.md M-Not 5건 (SVC-0016/0020/0025/0029, SEC-0001) 모두 ≥1 MOCK | ✅ (MOCK-0040/0044/0044/0073/0062) |
| 에러 매트릭스 커버 | api §6.1~6.8 모든 (HTTP·cause) 조합에 ProblemDetails MOCK | ✅ (E1: SNSSAI_NOT_SUPPORTED, NOT_AUTHORIZED, TOKEN_*, MANDATORY_*, FEATURE_NOT_SUPPORTED / E2/E3: + RESOURCE_NOT_FOUND, INVALID_IE, 415 / E4: RESOURCE_NOT_FOUND / E5: UNSUPPORTED_EVENT_TYPE, MANDATORY_IE_MISSING / E6: MODIFICATION_NOT_ALLOWED, SUBSCRIPTION_NOT_FOUND, SUMOD / E7: SUBSCRIPTION_NOT_FOUND / C1: RESOURCE_CONTEXT_NOT_FOUND, RESOURCE_URI_STRUCTURE_NOT_FOUND, 503+Retry-After, 307) |
| 경계값 자동 도출 | 모든 제약 (Snssai sst/sd, Tai tac, NfInstanceId UUID, supportedFeatures hex, amfSetId pattern, callback URI) 경계 페이로드 | ✅ (MOCK-B-0001~0062) |
| 특수 패턴 커버 | 7-A (Subscribe/Notify) + 7-B (PATCH) + 7-C (supportedFeatures) 모두 | ✅ (§5-A/B/C) |
| R11 운영 conf bundle | MOCK-CFG + MOCK-LOG + MOCK-TLS 모두 ≥1건 + impl_spec §3-C-5/3-C-6/§8-C-1 인용 | ✅ (CFG 3 + LOG 2 + TLS 2) |
| 페이로드 무결성 | 모든 ` ```json ` 블록 유효 JSON | ✅ |
| 출처 인용 무결성 | 모든 MOCK 행에 출처 (api §, features.md ID, VS-ID 중 ≥1) | ✅ |
| 라이브러리·NF 비하드코딩 | NF·필드 명칭 모두 업스트림 산출물에서 인용 | ✅ |
| 미치환 템플릿 변수 | `{{...}}` 잔존 0건 | ✅ |
