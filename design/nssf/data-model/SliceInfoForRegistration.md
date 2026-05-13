---
id: data-model/SliceInfoForRegistration
status: canonical
generated_sections:
  - chain-tree
  - field-table
user_sections:
  - implementation-notes
---

# data-model/SliceInfoForRegistration

본 토픽 — registration 시 UE 의 requested NSSAI + UE PLMN context. spec_refs — TS 29.531 §6.1.6.2.4. 기계 산출 (agent/codegen primary) — `SliceInfoForRegistration.json`.

## Chain Tree

<!-- AUTO:chain-tree:start -->
```text
# placeholder — /nf-build nssf --topic data-model/SliceInfoForRegistration 으로 도구가 채움
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
- `subscribedNssai` 는 array — UE 의 subscription DB (UDM) 에서 가져온다. SliceInfoForRegistration 본 schema 안에는 *복제본*이라 NSSF 가 다시 검증 (UDM 단일 진실).
- `requestedNssai` 가 비었으면 default subscribed NSSAI 적용 — 정책은 home PLMN 의 NSSF.
- `allowedNssaiCurrentAccess` — 같은 UE 가 이전 access (예 3GPP) 에서 받은 allowed NSSAI. mobility 흐름에서 NSSF 가 동일 set 을 유지하려고 활용.
- `mappingOfNssai` — visited PLMN ↔ home PLMN 의 S-NSSAI 매핑 (roaming). NSSF 는 매핑 결과로 본 PLMN 의 S-NSSAI 만 응답에 사용.
<!-- USER:implementation-notes:end -->
