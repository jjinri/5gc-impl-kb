# 5gc-impl-kb

`5gc-impl-kb` 는 3GPP spec 을 입력으로 NF(Network Function) 구현에 필요한 지식을 축적·검증하는 **implementation knowledge base**다. 목표는 사람이 spec 원본만 준비하면 AI agent 가 구현에 필요한 계약·설계·작업·검증 정보를 KB 에서 읽고 `/nf-implement <nf>` 로 코드를 생성할 수 있게 하는 것이다.

사람의 일반 workflow 는 3가지다.

```text
1. specs/ 에 3GPP 원본 docx/yaml 투입
2. /nf-readiness <nf>   → NF별 implementation KB/readiness pack 생성·검증
3. /nf-implement <nf>   → readiness pack 기반 autonomous 구현 시작
```

세부 lifecycle skill 은 `/nf-readiness` 내부 subroutine 이다. 사람이 직접 호출하는 경우는 third-party library 교체, DBMS 교체, 보안 정책 변경, spec 추가·제외, public contract 변경처럼 **새 계약**이 필요한 때로 한정한다.

## Quick start

```bash
git clone https://github.com/jjinri/5gc-impl-kb.git
cd 5gc-impl-kb
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
```

도구 실행은 venv 활성화 없이 `.venv/bin/python3 design/scripts/<script>.py ...` 형태로 가능하다. `.doc` 변환이 필요하면 host 에 `libreoffice` 또는 `antiword` 가 추가로 필요할 수 있다.

## KB 구조

```text
specs/                    # 사람이 넣는 원본 spec/API 파일
  29.531/*.docx|*.yaml

design/nf-registry.yaml   # NF 라우팅 catalog, git 추적

design/<nf>/              # spec-derived contract/architecture 영역
  architecture/           # review 된 architecture KB, git 추적
  module-decomposition/   # module 책임 KB, git 추적
  _manifest.yaml          # local rendered cache, 비추적
  _contract_seed.yaml     # local rendered cache, 비추적
  contract/               # local rendered contract topics, 비추적

handoff/<nf>/contract.yaml # local machine-readable contract cache, 비추적

engineering/<nf>/engineering-design.md  # library/DB/runtime/tool 결정 freeze, git 추적

dev/<nf>/                 # NF별 implementation KB/readiness pack, git 추적
  implementation-plan.md
  tasks.yaml
  test-matrix.md
  traceability.md
  api-implementation-matrix.md
  data-model-implementation-map.md
  codegen-work-items.yaml
  team-execution-plan.md
  verification-plan.md
  implementation-readiness-review.md
  design-adequacy-checklist.md
  spec-to-design-coverage.md
  open-gaps-and-assumptions.md
```

자세한 파일 분류와 fresh clone 재생성 순서는 [`docs/artifact-management.md`](./docs/artifact-management.md) 를 따른다. KB 독해 순서는 [`docs/kb/README.md`](./docs/kb/README.md) 를 따른다.

## Public workflow skills

| 사람 호출 | 목적 | 성공 gate | 주요 산출 |
|---|---|---|---|
| `/nf-readiness <nf>` | spec 에서 implementation-ready KB 생성·검증 | `readiness_pack_ready` | `design/<nf>/`, `engineering/<nf>/`, `dev/<nf>/` readiness pack |
| `/nf-implement <nf>` | readiness pack 을 입력으로 장기 autonomous 구현 | `tracer_bullet_passed` → `full_nf_done` | `src/`, generated code, SQL, tests, CI/runtime 산출 |

`readiness_pack_ready` 는 다음 gate 의 AND 다.

```text
handoff_ready
∧ contract_implementable
∧ arch_consistent
∧ impl_ready_for_codegen
∧ eng_frozen
```

`eng_frozen` 은 technology decision freeze 일 뿐이며, 단독 implementation GO 신호가 아니다.

## Internal lifecycle skills

| 내부 단계 | skill | 역할 | 산출 class |
|---|---|---|---|
| NF routing | `/nf-readiness` Phase 0 + `nf-registry-bootstrap.py` | `specs/` 에서 NF primary spec 후보 resolve | `design/nf-registry.yaml` tracked, manifest cache |
| Spec discovery | `/nf-spec-discover` | manifest/seed 생성 | local reproducible cache |
| Contract build | `/nf-contract-build` | contract topic + handoff 생성 | local reproducible cache |
| Contract check | `/nf-contract-check` | `handoff_ready`, `contract_implementable` 측정 | local status cache |
| Architecture | `/nf-arch-design` / `/nf-arch-status` | architecture KB 생성·검증 | tracked architecture + local status |
| Implementation planning | `/nf-impl-plan` / `/nf-impl-status` | readiness pack 생성·검증 | tracked `dev/<nf>/` KB + local status |
| Engineering freeze | `/nf-eng-design` / `/nf-eng-status` | library/DB/runtime/tool 결정 freeze | tracked engineering design + local status |
| Aggregate | `nf-readiness-status.py` | 최종 readiness gate 계산 | local status cache |

## 원칙

- **No spec semantic rediscovery**: implementation agent 는 원본 OpenAPI/spec 을 drift/source trace/generator input 으로만 사용할 수 있다. 구현 의미 판단은 KB 산출물이 제공해야 한다.
- **Tracked KB vs local cache 분리**: 다음 단계가 source of truth 로 읽는 reviewed 산출은 git 추적, deterministic cache/status 는 비추적.
- **Security baseline**: TLS/mTLS/OAuth2 production-capable code path 의무는 [`ADR-0004`](./docs/adr/ADR-0004-project-security-baseline.md) 가 project-wide source 다. 33.501/33.310/33.210 은 NF별 lifecycle extraction dependency 로 추가하지 않는다.

## References

- [`ONBOARDING.md`](./ONBOARDING.md) — 사람/agent 사용 가이드.
- [`CONTEXT.md`](./CONTEXT.md) — canonical 용어집.
- [`docs/kb/README.md`](./docs/kb/README.md) — KB 독해 순서와 agent/human review 관점.
- [`docs/artifact-management.md`](./docs/artifact-management.md) — 파일 class, git 정책, 재생성 순서.
- [`docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md`](./docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md) — lifecycle vocabulary.
- [`docs/adr/ADR-0002-engineering-design-freeze.md`](./docs/adr/ADR-0002-engineering-design-freeze.md) — `eng_frozen` 의미.
- [`docs/adr/ADR-0004-project-security-baseline.md`](./docs/adr/ADR-0004-project-security-baseline.md) — security baseline.
