# Deterministic readiness regeneration goal

이 문서는 `/nf-readiness nssf` 를 *완전 자동·결정론적 KB 재생성 workflow* 로 발전시키기 위한 후속 논의 메모다. 현재 동작 설명이 아니라 목표 구조와 사전에 합의해야 할 결정 목록을 정리한다.

## 1. 목표

최종 목표는 다음 상태다.

```text
clean checkout
+ specs/ 원본
+ tracked registry/config/decision files
+ lifecycle tools
→ /nf-readiness nssf
→ dev/nssf/ readiness pack 을 현재 NSSF readiness pack 수준으로 결정론적 재생성
→ drift check
→ readiness_pack_ready PASS/FAIL
```

즉, `dev/<nf>/` readiness pack 은 사람이 한 번 작성해 보존하는 문서가 아니라, 상위 source/config/decision 을 입력으로 재생성 가능한 NF별 implementation KB 가 된다.

## 2. 현재 한계

현재 `dev/nssf/` readiness pack 은 tracked KB artifact 이며, agent 가 contract/architecture/engineering 산출을 읽고 실질 내용을 작성한 상태다.

따라서 `dev/` 를 삭제한 뒤 `/nf-readiness nssf` 를 실행하면 다음은 가능하다.

- template skeleton 생성
- validator 에 의한 TODO/placeholder 차단
- 사람이/agent가 다시 실질 내용을 채우면 PASS

하지만 다음은 아직 보장되지 않는다.

- 현재 수준의 9-file readiness pack 자동 복원
- work item graph / gap classification / human review summary 의 완전 결정론 생성
- tracked readiness pack 과 generator output 의 drift 비교

## 3. 원칙

1. **사람 승인 대상은 config/decision file 에 고정한다.**
   - agent 산문 안에만 남긴 결정은 삭제 시 사라진다.
   - 사람의 선택·승인은 tracked YAML/ADR/engineering decision 으로 승격한다.
2. **`/nf-readiness` 는 작성 workflow 가 아니라 compiler 에 가까워져야 한다.**
   - input: specs + registry + config + decisions + tools
   - output: readiness pack + status
3. **generated/cache 와 tracked KB 를 분리한다.**
   - local cache 는 재생성한다.
   - tracked KB 는 재생성 후 drift 를 비교한다.
4. **No spec semantic rediscovery 를 유지한다.**
   - implementation agent 가 spec 을 다시 읽어 의미를 보강하면 실패다.
   - 필요한 의미는 readiness generator 가 상위 source/config 에서 만들어야 한다.
5. **사람 review 는 남긴다.**
   - Human Review Pack 은 자동 생성될 수 있지만, 그 목적은 사람이 확인하기 쉬운 audit view 제공이다.

## 4. 사전 합의가 필요한 결정 영역

### 4.1 NF routing / spec selection

합의할 것.

- NF key (`nssf`) 와 primary spec (`29.531`).
- 사용할 OpenAPI YAML 목록.
- supporting spec 포함 조건.
- 33.501/33.310/33.210 같은 security/profile spec 을 ADR-0004 로 흡수하는 정책.

고정 위치 후보.

- `design/nf-registry.yaml`
- `design/<nf>/readiness-config.yaml`

### 4.2 Security baseline

합의할 것.

- internal TLS code path 의무.
- mTLS code path 의무.
- inbound OAuth2 bearer validation 의무.
- outbound OAuth2 client credentials/token attach 의무.
- dev disable 가능 범위.
- TLS/X.509/JWT primitive 직접 구현 금지.
- operator/library responsibility boundary.

고정 위치 후보.

- `docs/adr/ADR-0004-project-security-baseline.md`
- `engineering/<nf>/engineering-design.md`
- optional machine-readable config.

### 4.3 Engineering decisions

합의할 것.

- implementation language/runtime.
- build system.
- HTTP/2 server/client library.
- TLS provider / OpenSSL 사용 방식.
- OAuth2/JWT/JWKS library.
- persistence backend and DB driver.
- schema codegen tool and JSON library.
- test framework.
- observability stack.
- config/env 처리 방식.

고정 위치 후보.

- `engineering/<nf>/engineering-design.md`
- optional `engineering/<nf>/engineering-config.yaml`

### 4.4 Data model generation policy

합의할 것.

- OpenAPI generator 사용 범위.
- generated model naming convention.
- C type hint derivation.
- wrapper 필요 조건.
- handwritten schema 허용 기준.
- `PatchDocument` 처리 정책.
- `ProblemDetails` shared wrapper 정책.

고정 위치 후보.

- `design/<nf>/contract/data-model/*.json` metadata
- `design/<nf>/readiness-config.yaml`
- generic readiness profile.

### 4.5 API / operation implementation policy

합의할 것.

- operation → handler naming.
- operation → module mapping.
- operation → OAuth2 scope mapping.
- operation → persistence mapping.
- error response / ProblemDetails mapping.
- redirect 307/308 policy.
- OPTIONS 처리 방식.

고정 위치 후보.

- `design/<nf>/architecture/**`
- `engineering/<nf>/engineering-design.md`
- `design/<nf>/readiness-config.yaml`

### 4.6 Persistence / schema policy

합의할 것.

- PostgreSQL/libpq 단일 backend 여부.
- table 목록, PK, index, constraints.
- JSONB 사용 범위.
- migration 방식.
- repository interface and in-memory mock 범위.

고정 위치 후보.

- `engineering/<nf>/engineering-design.md`
- `design/<nf>/architecture/state-persistence.md`
- optional structured persistence config.

### 4.7 Work item / phase policy

합의할 것.

- Phase 1~5 정의.
- tracer-bullet operation.
- phase/wave mapping.
- work item ID naming.
- work item dependency graph.
- owner lane.
- expected file naming/layout.

고정 위치 후보.

- `design/<nf>/readiness-config.yaml`
- generic work-item profile.

### 4.8 Test / verification policy

합의할 것.

- unit/integration/contract/security/e2e/observability gate 정의.
- coverage threshold.
- PostgreSQL fixture 방식.
- TLS/mTLS/JWKS fixture 방식.
- contract test matrix 범위.
- mocked peer 범위.

고정 위치 후보.

- `design/<nf>/architecture/test-strategy.md`
- `engineering/<nf>/engineering-design.md`
- `design/<nf>/readiness-config.yaml`

### 4.9 Gap / assumption policy

합의할 것.

- gap category enum.
- blocker 정의.
- operator-provided 허용 항목.
- library-assumed 허용 항목.
- deferred 허용 조건.
- readiness-followup 표현 방식.

고정 위치 후보.

- `design/<nf>/readiness-config.yaml`
- project policy doc.

### 4.10 Human review pack generation policy

합의할 것.

- executive review 생성 기준.
- design adequacy checklist 항목.
- spec-to-design coverage row 생성 기준.
- `spec reread needed?` enum and blocker rule.
- recommendation GO/NO-GO 문구 생성 기준.

고정 위치 후보.

- generic human-review-pack profile.
- `design/<nf>/readiness-config.yaml` overrides.

## 5. Proposed config shape

초기에는 여러 config 로 쪼개기보다 NF별 하나의 config 로 시작한다.

```yaml
schema_version: nf-readiness-config-v1
nf: nssf

specs:
  primary: "29.531"
  openapi:
    - specs/29.531/TS29531_Nnssf_NSSelection.yaml
    - specs/29.531/TS29531_Nnssf_NSSAIAvailability.yaml
  absorbed_security_specs:
    - "33.501"
    - "33.310"
    - "33.210"

security:
  baseline_adr: docs/adr/ADR-0004-project-security-baseline.md

phase_policy:
  tracer_bullet_operation: NSSelectionGet
  phases:
    phase1:
      - WI-codegen-bootstrap
      - WI-schema-bootstrap
      - WI-tls-bootstrap
      - WI-problem-details-wrapper
      - WI-nftype-wrapper
      - WI-availability-repo
      - WI-selection-engine
      - WI-nsselection-handler

data_model_policy:
  patch_document: json_patch_rfc6902
  problem_details: shared_wrapper
  wrapper_rules:
    - anyOf
    - oneOf
    - allOf
    - discriminator
    - problem_details_response

gaps:
  categories:
    - blocker
    - deferred
    - operator-provided
    - library-assumed
    - test-gap
    - assumption
  operator_provided:
    - tls_cert
    - postgres_dsn
    - jwks_endpoint
```

## 6. Generator target

추가할 generator 후보.

```text
design/scripts/nf-readiness-pack-generate.py <nf>
```

입력.

- `design/nf-registry.yaml`
- `design/<nf>/readiness-config.yaml`
- `handoff/<nf>/contract.yaml`
- `design/<nf>/contract/data-model/*.json`
- `design/<nf>/architecture/**`
- `engineering/<nf>/engineering-design.md`
- project ADR/policy docs

출력.

- `dev/<nf>/implementation-plan.md`
- `dev/<nf>/tasks.yaml`
- `dev/<nf>/test-matrix.md`
- `dev/<nf>/traceability.md`
- readiness pack 9 files

필수 동작.

- deterministic output.
- no timestamp noise, or normalized generated date.
- existing tracked pack 과 diff 비교.
- `--write` / `--check` 모드 분리.
- `nf-impl-status.py` 와 `nf-readiness-status.py` 를 후속 실행.

## 7. `/nf-readiness` target behavior

목표 동작.

```text
/nf-readiness nssf
  1. registry resolve
  2. local cache 재생성: manifest, seed, contract, handoff, status
  3. tracked KB 재생성/check: architecture, engineering, dev readiness pack
  4. drift 있으면 FAIL 또는 write mode 에서 갱신
  5. readiness_pack_ready PASS/FAIL 보고
```

정책 선택이 필요하다.

- default 를 `--check` 로 둘지 `--write` 로 둘지.
- tracked KB drift 가 있으면 자동 overwrite 할지 PR 필요로 멈출지.
- 사람 ratify 가 필요한 engineering 결정이 없으면 config 기반 자동 ratify 로 볼지, 여전히 별도 확인을 요구할지.

## 8. Open decisions for next discussion

1. `design/<nf>/readiness-config.yaml` 를 새 tracked source 로 둘 것인가.
2. `engineering-design.md` 를 계속 사람이 읽는 markdown+yaml 로 둘지, 별도 `engineering-config.yaml` 를 추가할지.
3. `dev/<nf>/` readiness pack 을 tracked generated output 으로 취급하고 drift check 를 도입할지.
4. `/nf-readiness` default mode 를 check-only 로 할지 write/update 로 할지.
5. 현재 NSSF pack 내용을 어떤 generator rule/source 로 역승격할지.
6. Human Review Pack 을 완전 자동 render 로 볼지, 일부 사람 작성 review 로 남길지.
7. status cache (`_*_status.yaml`) 는 계속 비추적 유지할지.

## 9. Non-goals

- 이 문서는 `/nf-implement` 실제 구현 범위를 정의하지 않는다.
- 이 문서는 current `/nf-readiness` 가 이미 완전 결정론 generator 라고 주장하지 않는다.
- 이 문서는 `docs/plans/**` 또는 `docs/retros/**` 를 수정하지 않는다.
