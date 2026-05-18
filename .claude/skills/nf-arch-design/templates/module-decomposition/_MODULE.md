---
nf: {{nf}}
stage: architecture-design
status: draft
source_contract: {{contract_path}}
generated_date: {{generated_date}}
---

# {{Module}}

> 본 파일은 module-decomposition template 이다. `<Module>` 한 개당 한 파일을 만들고
> 파일명을 module 명으로 둔다 (예 `SelectionEngine.md`). `{{Module}}` 을 module 명으로 치환한다.

## Responsibility

TODO: 본 모듈의 *단일* 책임을 1 ~ 3 줄로 적는다. *무엇을 하지 않는지* (다른 모듈의 책임) 도 명시한다.

## Inputs

- TODO: inbound — 본 모듈로 들어오는 request / 호출.
- TODO: contract data-model — 본 모듈이 의존하는 schema id.
- TODO: collaborator — interface 로 추상화된 의존 (외부 NF, 다른 모듈 read 참조 등).

## Outputs

- TODO: 정상 — 본 모듈이 만드는 데이터·응답.
- TODO: error — 본 모듈이 반환하는 cause.
- TODO: log/metric/trace event (`../architecture/observability.md` 와 연결).

## State

- TODO: 본 모듈이 보유·소비하는 상태. stateless 면 명시.
- TODO: 캐시·세션 상태의 소유 경계 (collaborator 책임이면 그렇게 적는다).

## Decisions

| 결정 | 내용 |
|---|---|
| TODO | TODO |

## Open Questions

- TODO: 다음 사이클 또는 dev 단계로 미루는 미해결 항목.
- TODO: 운영 결정 보류 항목 (있으면).

## References

- [[../architecture/module-boundaries]] — 모듈 책임 경계.
- TODO: [[../architecture/request-flow]] 등 관련 architecture 문서 wikilink.
- `{{contract_path}}` — 의존 API / data-model topic (진실 출처).
- TODO: 협력하는 다른 module wikilink (예 [[OtherModule]]).
