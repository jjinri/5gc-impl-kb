---
id: error-handling
status: handoff_ready
generated_sections:
  - error-matrix
user_sections:
  - recovery-prose
  - implementation-notes
---

# Error Handling

NSSF NSSelectionGet 의 ProblemDetails 응답 — TS 29.531 §6.1.7, yaml `responses`.

## Error Matrix

<!-- AUTO:error-matrix:start -->
| HTTP | ProblemDetails.cause | 의미 | 권장 동작 |
| --- | --- | --- | --- |
| 200 | — | success — AuthorizedNetworkSliceInfo 반환 | continue |
| 400 | INVALID_QUERY_PARAM | 필수 query 누락·형식 오류 | client 가 요청 정정 |
| 403 | UNAUTHORIZED_NSSAI | 요청 S-NSSAI 가 home/serving PLMN 에서 허용되지 않음 | UE 측 NSSAI 재협상 |
| 404 | NSSAI_NOT_AVAILABLE | 해당 S-NSSAI 슬라이스 미배포 | fall-back default NSSAI |
| 500 | SYSTEM_FAILURE | NSSF 내부 오류 | 재시도 + alert |
<!-- AUTO:error-matrix:end -->

<a id="nsselection-400"></a>
## NSSelection 400 — INVALID_QUERY_PARAM

<a id="nsselection-403"></a>
## NSSelection 403 — UNAUTHORIZED_NSSAI

<a id="nsselection-404"></a>
## NSSelection 404 — NSSAI_NOT_AVAILABLE

## Recovery 정책

<!-- USER:recovery-prose:start -->
- 400 — client 잘못된 요청. 재시도 가치 없음. 응답 시 cause + invalidParams 채움.
- 403 — UE 의 requestedNssai 가 PLMN policy 외. UDR 의 subscribed NSSAI 와 비교 후 default 적용 여부 결정.
- 404 — slice instance 부재. AMF/SMF 가 fall-back default NSSAI 처리.
- 500 — NSSF 내부 오류. 재시도 가능하지만 cause 가 `SYSTEM_FAILURE` 면 backoff (exponential, 최대 3회 권장).
<!-- USER:recovery-prose:end -->

## Implementation Notes

<!-- USER:implementation-notes:start -->
- ProblemDetails 직렬화 — Content-Type `application/problem+json`. RFC 7807.
- `invalidParams` 필드는 array of {param, reason}. 가능한 모든 invalid param 을 한 응답에 담는다 (client round-trip 줄임).
- cause string enum 은 TS 29.500 §5.2.7.2 의 generic causes + TS 29.531 §6.1.7.3 의 NSSF-specific causes 둘 다 허용.
<!-- USER:implementation-notes:end -->
