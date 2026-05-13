---
id: data-model/AuthorizedNetworkSliceInfo
status: canonical
generated_sections:
  - chain-tree
  - field-table
user_sections:
  - implementation-notes
---

# data-model/AuthorizedNetworkSliceInfo

NSSelectionGet 200 응답 본문. spec_refs — TS 29.531 §6.1.6.2.5. 기계 산출 — `AuthorizedNetworkSliceInfo.json`.

## Chain Tree

<!-- AUTO:chain-tree:start -->
```text
# placeholder — /nf-build nssf --topic data-model/AuthorizedNetworkSliceInfo 으로 도구가 채움
```
<!-- AUTO:chain-tree:end -->

## Field Table

<!-- AUTO:field-table:start -->
| field | required | type | note |
| --- | --- | --- | --- |
| (도구 산출 placeholder — 본 표는 build 시 갱신) | — | — | — |
<!-- AUTO:field-table:end -->

## Implementation Notes

<!-- USER:implementation-notes:start -->
- `authorizedNssai` 가 핵심 — UE 가 본 PLMN 에서 사용 *가능한* S-NSSAI 집합 + 각 슬라이스의 NSI (Network Slice Instance) 정보.
- `targetAmfSet` — NSSF 가 적합한 AMF set 을 추천. AMF 가 직접 선택할 수도, NRF 에 위임할 수도 있다 (정책 결정).
- `candidateAmfList` — 구체 AMF instance 리스트. 비어있을 수 있음 (targetAmfSet 만 줄 때).
- `rejectedNssaiInPlmn` / `rejectedNssaiInTa` — requestedNssai 중 거부된 항목 + 사유. UE 에게 fall-back 결정 정보 제공.
- `nsiInformationList` — 각 슬라이스의 NSI 식별 (NRF 등록된 instance ID). 본 정보로 AMF/SMF 가 slice-specific NF 를 찾는다.
<!-- USER:implementation-notes:end -->
