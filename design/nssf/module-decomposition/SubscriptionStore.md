---
nf: nssf
stage: architecture-design
status: draft
source_contract: handoff/nssf/contract.yaml
generated_date: 2026-05-14
---

# SubscriptionStore

## Responsibility

NSSF inbound `Nnssf_NSSAIAvailability` 의 *subscription lifecycle* persistence — Subscribe (POST) 시 subscription resource 생성·id 발급, SubModifyPatch 시 filter / lifetime / callbackUri 갱신, Unsubscribe 시 제거. 변경 이벤트가 들어오면 *영향 받는 subscription 목록* 을 NotificationDispatcher 에 공급한다. 본 모듈은 *subscription 자체* 만 — availability map 의 CRUD 책임 없음, outbound 호출 책임 없음.

## Inputs

- inbound — `api/NSSAIAvailabilityPost` (Subscribe), `api/NSSAIAvailabilitySubModifyPatch`, `api/NSSAIAvailabilityUnsubscribe`.
- contract data-model — `NssfEventSubscriptionCreateData` (subscribe input), `NssfEventSubscriptionCreatedData` (subscribe output + 추후 lookup).
- AvailabilityEngine 의 *변경 이벤트* — `(tai, change_type, before, after)`.
- persistence — subscription repository interface.

## Outputs

- 정상 — `NssfEventSubscriptionCreatedData` (Post 201) / 200 (SubModifyPatch) / 204 (Unsubscribe).
- error → ProblemDetailsMapper.
- *영향 받는 subscription 목록* (filter 매칭 적용) → NotificationDispatcher.

## State

- subscription resource set — id, callbackUri, filter (TAI · S-NSSAI · event type), lifetime / expiry, etag (옵션).
- 영속성 — backend 후보 (`state-persistence.md`).
- *crash 후 복원 필수* — subscription 손실 시 AMF 가 재구독 필요.

## Decisions

| 결정 | 내용 |
|---|---|
| 책임 경계 | subscription resource 와 그 lifecycle 만. callback URI 호출은 NotificationDispatcher. |
| id 발급 | spec 강제 형식 (UUID 또는 NSSF 정책) 따름. 본 architecture 는 형식 강제 안 함. |
| lifetime 관리 | spec 의 `validityTime` 만료 시 lazy delete 또는 background sweep — backend 선택에 의존. |
| filter 매칭 알고리즘 | 변경 이벤트의 `tai` + `change_type` 을 subscription 의 filter 와 매칭. naive scan 또는 indexed (backend 결정). |
| etag 지원 | 옵션. SubModifyPatch 에서 412 정책 (`error-propagation.md` 와 연동). |

## Open Questions

- 만료 처리 정책 — eager (background sweep) vs lazy (lookup 시 expire).
- callback URI 의 *재인증* — token 부착 정책 (`configuration-strategy.md` 의 OAuth2 옵션 참고).
- subscription cardinality 한도 — 동일 AMF 가 무제한 등록 vs limit 적용.
- AMF deregistration 시 본 NSSF 의 subscription 자동 정리 정책.

## References

- [[../architecture/module-boundaries]] — 4 모듈 책임.
- [[../architecture/request-flow]] — Subscribe / SubModifyPatch / Unsubscribe 시퀀스.
- [[../architecture/state-persistence]] — subscription repository backend 후보.
- `handoff/nssf/contract.yaml` `api/NSSAIAvailability{Post,SubModifyPatch,Unsubscribe}` + data-model `NssfEventSubscriptionCreate{,d}Data`.
- [[AvailabilityEngine]] — 변경 이벤트 source.
- [[NotificationDispatcher]] — subscription lookup consumer.
