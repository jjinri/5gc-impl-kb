# 5gc-impl-kb context

본 repo 는 3GPP spec 을 NF 구현용 **implementation knowledge base** 로 변환한다. 이 문서는 현재 workflow 의 용어집이다.

## Canonical terms

**Implementation Knowledge Base (KB)**
AI agent 가 NF 코드를 작성하고 사람이 설계를 검토하기 위해 읽는 tracked 산출물 집합. NF별 KB 는 주로 `design/<nf>/architecture/**`, `design/<nf>/module-decomposition/**`, `engineering/<nf>/engineering-design.md`, `dev/<nf>/` readiness pack 으로 구성된다.

**Source Input**
사람이 외부에서 확보해 `specs/<spec>/` 에 넣는 원본 docx/yaml/pdf. lifecycle 이 새로 시작되는 clean 상태의 필수 입력이다.

**Local Reproducible Artifact**
동일 source input + script/skill 로 재생성 가능한 cache 산출. 예: `design/<nf>/_manifest.yaml`, `design/<nf>/_contract_seed.yaml`, `design/<nf>/contract/**`, `handoff/<nf>/contract.yaml`, `_*_status.yaml`. git source of truth 가 아니다.

**Reviewed Lifecycle Artifact**
skill 이 생성할 수 있지만 PR review/ratify 후 다음 단계의 source of truth 로 쓰이는 tracked 산출. 예: architecture 문서, `engineering-design.md`, `dev/<nf>/` readiness pack.

**Readiness Pack**
`/nf-impl-plan` 이 만드는 `dev/<nf>/` implementation KB. Agent Execution Pack 과 Human Review Pack 을 포함하며 `impl_ready_for_codegen` 의 입력이다.

**Agent Execution Pack**
구현 agent 가 코드 생성을 위해 읽는 readiness pack 부분. 대표 파일: `api-implementation-matrix.md`, `data-model-implementation-map.md`, `codegen-work-items.yaml`, `team-execution-plan.md`, `verification-plan.md`.

**Human Review Pack**
사람이 설계 충분성, trace, gap 분류를 검토하기 위해 읽는 readiness pack 부분. 대표 파일: `implementation-readiness-review.md`, `design-adequacy-checklist.md`, `spec-to-design-coverage.md`, `open-gaps-and-assumptions.md`.

**Engineering Design**
spec 에서 자동 도출할 수 없는 library/DB/runtime/tool/operator-policy 결정. `engineering/<nf>/engineering-design.md` 가 source of truth 다.

**eng_frozen**
Engineering Design 의 technology decision freeze gate. 최종 implementation GO 가 아니라 `readiness_pack_ready` 의 구성요소다.

**readiness_pack_ready**
`/nf-implement <nf>` 시작 가능성을 뜻하는 aggregate gate. 다음 gate 가 모두 PASS 해야 한다: `handoff_ready`, `contract_implementable`, `arch_consistent`, `impl_ready_for_codegen`, `eng_frozen`.

**No Spec Semantic Rediscovery**
구현 agent 가 원본 spec/OpenAPI 를 의미 보강이나 data model 재해석 목적으로 다시 읽으면 readiness 실패로 본다. 원본은 generator input, drift/source trace 확인에만 허용된다.

**Project Security Baseline**
모든 NF 구현이 따라야 하는 TLS/mTLS/OAuth2 production-capable code path 의무. Source = `docs/adr/ADR-0004-project-security-baseline.md`. 33.501/33.310/33.210 세부는 operator-provided config + maintained library compliance 로 다루며 NF별 lifecycle dependency 로 끌어들이지 않는다.

## Relationships

- `specs/` 는 source input 이다.
- `/nf-readiness <nf>` 는 source input 을 NF별 KB/readiness pack 으로 변환한다.
- `/nf-implement <nf>` 는 readiness pack 을 읽어 실제 source/test/CI 를 만든다.
- `eng_frozen` 은 necessary but not sufficient 이다. 최종 GO 는 `readiness_pack_ready` 다.
- Human Review Pack 은 자동 gate 의 대상이 아니라 사람이 audit 하는 표면이다. 단, 파일 존재·frontmatter·traceability 같은 구조는 `nf-impl-status.py` 가 검사한다.
- Local reproducible artifact 는 중요하지만 git source of truth 가 아니다. 필요하면 skill/script 로 재생성한다.

## Avoid ambiguous wording

| 피할 표현 | 대신 사용할 표현 |
|---|---|
| design output | spec-derived contract / architecture / engineering design 중 하나로 구체화 |
| implementation plan 이 곧 개발 설계 | implementation planning 과 engineering design 을 분리 |
| eng_frozen 이 codegen GO | readiness_pack_ready 가 implementation GO |
| generated file = gitignore | local reproducible artifact 또는 reviewed lifecycle artifact 로 구분 |
| spec 다시 읽어서 구현 | readiness pack 보강 후 구현 |
