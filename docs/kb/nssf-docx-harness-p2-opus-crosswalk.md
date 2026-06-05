# NSSF DOCX Harness P2 + Opus Crosswalk

Status: **analysis-only**.
Scope: DOCX-derived gaps × `docs/opus_0526/*.md` × merged Phase1~3 implementation.
Non-goals: no `src/**` changes, no tests changes, no `dev/nssf/pr-slicing-plan.yaml` edits.

## Executive conclusion

Phase4 is technically un-gated after the reliability gate, but Phase4 should **consume this matrix first**. The Opus corpus adds procedure/test specificity that the OpenAPI-first harness did not fully encode.

Recommended sequence:

1. Merge this analysis PR as `DOCX-HARNESS-PHASE-2` evidence.
2. Add a small plan-amendment PR that maps the rows below into existing Phase4 test slices.
3. Run Phase4 contract/security/e2e tests.
4. Defer operator policy gaps to Phase5 guide.

No new runtime implementation slice is required **before** Phase4 if unsupported/deferred rows are explicit in the Phase4 matrix.

## Inputs used

| Input | Purpose |
|---|---|
| `dev/nssf/docx-derived-design-gaps.yaml` | DOCX P1 gap list and target slices |
| `design/nssf/_docx_semantic_analysis.yaml` | source_refs and DOCX semantic basis |
| `docs/opus_0526/29.531-features.md` | FEAT-level SHALL/SHALL NOT registry |
| `docs/opus_0526/NSSF-procedure-analysis.md` | PROC-level procedure matrix |
| `docs/opus_0526/NSSF-test-spec.md` | TC-level test rows |
| `docs/opus_0526/NSSF-api-spec.md` | API/header/security notes |
| `docs/opus_0526/NSSF-db-design.md` | ENT/state model notes |
| `src/nssf/**`, `tests/nssf/**` | Phase1~3 implementation evidence |

## 1. DOCX gap status revalidation

| Gap | P2 status | Evidence-based conclusion |
|---|---|---|
| DOCX-GAP-001 | **closed** | #142 implemented same-AMF suppress at fan-out enqueue. Keep regression row only. |
| DOCX-GAP-002 | **partial/open** | Top-level `/event` is rejected by current patch resource shape, but `/filter/event` can be patchable when the create body was stored as filter. Empty `taiList`/ONSSAI is not first-class. |
| DOCX-GAP-003 | **open, Phase5** | Current default is fixed 24h. Opus requires spread/jitter/cleanup semantics. Mark in Phase4; implement/document in Phase5. |
| DOCX-GAP-004 | **open** | Current engine covers registration/PDU/UE-config subset. Opus lists PDN, NWDAF/other-purpose, roaming/inter-PLMN variants too. |
| DOCX-GAP-005 | **open** | Current selection formula lacks NSSRG/NSAG/UDM provide-all response shaping. Generated models exist, but logic/tests do not. |
| DOCX-GAP-006 | **open, Phase5** | NSSF discovery/home-NSSF/FQDN policy is not separated from OAuth2/NRF config in operator docs. |
| DOCX-GAP-007 | **open** | Current code intentionally does not emit server-side redirect; Phase4 must prove unsupported/deferred or enforce `Location` + `3gpp-Sbi-Target-Nf-Id`. |
| DOCX-GAP-008 | **open** | Generic notification payload exists, but NSRP/NSIUN/validity/feature-gated payload matrix is not covered. |
| DOCX-GAP-009 | **open, wording advisory** | Code may enforce deployment scopes, but docs/tests must not claim 29.531 defines Nnssf API scopes. |

## 2. Opus crosswalk matrix

| Category | Items |
|---|---|
| DOCX-only | GAP-001 exact same-origin AMF suppression. Opus has notification fan-out, but DOCX found this precise suppress rule. |
| Both, Opus stronger | GAP-002, GAP-003, GAP-004, GAP-005, GAP-007, GAP-008. Opus provides FEAT/PROC/TC IDs and mock/test rows. |
| Both, wording conflict | GAP-009. Opus says no resource-level scopes but also proposes endpoint scope names. Current project must phrase these as deployment/NRF policy scopes. |
| Opus-only future candidates | NF registration/deregistration/heartbeat lifecycle, gzip content-encoding, full mock corpus, detailed FEAT/TC registry import. |

Important tension:

- Opus `NSSF-test-spec.md` includes callback 307 retry rows for notification callbacks.
- Current project ratified fail-closed/no redirect-follow for outbound OAuth2/notification paths.
- Treat Opus redirect-follow rows as **candidate only**; ADR/security ratification wins unless re-ratified.

## 3. Phase1~3 impact

| Impact | Affected surface | Risk |
|---|---|---|
| Event mutation under PATCH | `SubscriptionStore::patch`, subscription PATCH handler | `/filter/event` may mutate event semantics if whole create document was stored as filter. |
| Incomplete NSSelection procedure matrix | `SelectionEngine`, NSSelection handler/tests | Phase4 could overclaim NSSelection completeness. |
| Conditional NSSRG/NSAG/UDM behavior absent | `SelectionEngine` seams/output | Optional features need explicit unsupported/deferred rows. |
| Server redirect unimplemented | `SelectionEngine`, router/server | Must not claim 307/308 support without target NF header. |
| Feature-gated notifications incomplete | dispatcher/fan-out | Generic change notification is not NSRP/NSIUN/validity feature-complete. |
| Scope wording drift | OAuth2 scope tests/docs | Project policy scopes must not be represented as 29.531-defined scopes. |

## 4. Phase4 test rows / backfill candidates

### `PR-phase4-contract-tests`

| Gap | Candidate row |
|---|---|
| GAP-002 | `SubModifyPatch` rejects `/event` and `/filter/event` mutation with 400 and no persisted change. |
| GAP-002 | Empty `taiList` is rejected unless ONSSAI support is explicitly enabled; otherwise unsupported response is documented. |
| GAP-004 | Registration, PDU session, UE config update rows for currently implemented variants. |
| GAP-004 | PDN/RSIPCE, OtherPurpose/NWDAF/SIOP, Inter-PLMN rows as supported-or-deferred markers. |
| GAP-007 | Server 307/308 either unsupported or must include `Location` + `3gpp-Sbi-Target-Nf-Id`. |

### `PR-phase4-security-tests`

| Gap | Candidate row |
|---|---|
| GAP-002 | Rejected mutation leaves persisted event/filter unchanged. |
| GAP-007 | No server redirect is emitted without an operator target NF identity. |
| GAP-009 | Scope tests label scope strings as ADR/deployment policy, not 29.531-defined resource scopes. |

### `PR-phase4-e2e-tests`

| Gap | Candidate row |
|---|---|
| GAP-001 | Same-origin AMF fan-out suppression regression. |
| GAP-004 | End-to-end rows for implemented NSSelection variants. |
| GAP-005 | NSSRG/NSAG/UDM conditional rows as supported or explicit unsupported. |
| GAP-008 | Notification feature payload matrix: generic status-change supported; NSRP/NSIUN/validity either supported or 501/unsupported. |

### Phase5 operator guide

| Gap | Candidate content |
|---|---|
| GAP-003 | `default_expiry`, `allow_indefinite`, `expiry_jitter_seconds`, cleanup/sweeper policy. |
| GAP-006 | same-PLMN local NSSF URL, NRF-discovered NSSF URL, home-NSSF URL/FQDN policy. |
| GAP-009 | Nnssf APIs define no spec scopes; configured scopes are deployment/NRF/operator policy. |

## 5. Plan-amendment recommendation

- Required: **yes**, but not in this PR.
- Change `pr-slicing-plan.yaml` here: **no**.
- New runtime slice before Phase4: **not required**.
- Recommended next PR: small plan amendment that registers this analysis as the Phase3→Phase4 gate evidence and expands Phase4 acceptance rows.
- Defer to Phase5: expiry policy, NSSF discovery/home-NSSF/FQDN, OAuth2 scope wording in operator guide.

## Evidence highlights from Opus

- Procedure variants are explicit in `NSSF-procedure-analysis.md` lines 38-44 and detailed in lines 315-670.
- Subscription modify immutability and ONSSAI-related rows appear in `29.531-features.md` lines 87-92 and `NSSF-test-spec.md` TC-0025/0073/0074.
- Expiry spread is in `29.531-features.md` line 83 and `NSSF-test-spec.md` TC-0023/0072/0142.
- Redirect header requirement is in `NSSF-api-spec.md` lines 517 and 548-549.
- Scope caveat is in `NSSF-hints.md` line 101 and `NSSF-api-spec.md` security section.
