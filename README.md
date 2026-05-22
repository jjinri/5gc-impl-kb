# 5gc-impl-kb

3GPP spec 으로부터 AI agent 가 NF (Network Function) 구현 코드를 *자율 생성* 하는 **5gc design-to-dev knowledge base**. 사람의 public workflow 는 *3 행위* 로 축소된다: (1) `specs/` 에 3GPP 원본 투입, (2) `/nf-readiness <nf>` 요청 (implementation readiness pack 생성), (3) `/nf-implement <nf>` 요청 (장기 autonomous 구현). 세부 lifecycle skill (`/nf-spec-discover`, `/nf-contract-build`, `/nf-arch-design`, `/nf-impl-plan`, `/nf-eng-design` 등) 은 `/nf-readiness` 내부 subroutine 으로 재배치되며 사람의 주 작업 surface 가 아니다.

> **2026-05-21 workflow upgrade progress.** 최종 public workflow (`/nf-readiness` + `/nf-implement`) 는 `docs/plans/2026-05-21-nf-readiness-implementation-workflow-upgrade-plan.md` 의 PR A~G 사이클로 단계적으로 도입된다. **PR E3 (2026-05-22)** 까지 `/nf-readiness <nf>` + `/nf-implement <nf>` wrapper *skeleton* 이 모두 신설됐다. NSSF 의 readiness pack 실데이터 갱신은 PR F, `/nf-implement` 의 NSSF Phase 1 tracer-bullet 실행은 PR G 의 책임.

```text
Target public workflow:
  specs/{spec}/{file}.{docx,yaml}        ← 사람 입력
        │
        ▼  /nf-readiness <nf>            ← 사람 호출 1
   dev/<nf>/ readiness pack              ← Agent Execution Pack + Human Review Pack
        │
        ▼  readiness_pack_ready PASS     ← 최종 GO gate (eng_frozen 단독 GO 폐기)
        │
        ▼  /nf-implement <nf>            ← 사람 호출 2
   src/, generated/, sql/, tests/        ← autonomous code generation 산출
        │
        ▼  full_nf_done                  ← merge/release
```

정확한 lifecycle 용어와 canonical skill 이름은 [`docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md`](./docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md) 를 따른다. 8 gate 의미 정정 (`specs_ready`, `contract_implementable`, `arch_consistent`, `impl_ready_for_codegen`, `eng_frozen`, `readiness_pack_ready`, `tracer_bullet_passed`, `full_nf_done`) 는 위 upgrade plan §3 참조.

## Quick start

```bash
git clone https://github.com/jjinri/5gc-impl-kb.git && cd 5gc-impl-kb
sudo apt install python3-venv python3-pip   # 사용자 권한 필요 — 한 번만
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
```

의존성은 `requirements.txt` 가 진실 출처 (`pypdf`·`python-docx`·`pyyaml`). 본 repo 의 모든 도구는 `.venv/bin/python3 design/scripts/<name>.py` 형태로 호출하므로 venv 활성화는 불필요. `.doc` 처리가 필요하면 별도로 `sudo apt install libreoffice-core` 또는 `antiword`.

## 사람이 호출하는 lifecycle skill

### Target public surface (upgrade 완료 후)

| 단계 | skill | 기능 | 주요 산출물 |
|---|---|---|---|
| Readiness | `/nf-readiness <nf>` (PR E2, 2026-05-22) | spec → contract → arch → impl → eng-design 11-step lifecycle chain 자동 실행, registry resolve + first-blocker short-circuit, aggregate gate `readiness_pack_ready` 평가 | `dev/<nf>/` readiness pack (Agent Execution Pack 5 + Human Review Pack 4) + `dev/<nf>/_readiness_status.yaml` |
| Implementation | `/nf-implement <nf>` (PR E3, 2026-05-22 — skeleton) | readiness pack 입력 autonomous code generation. Phase 0 readiness gate → Phase 1 tracer-bullet → Phase 2 task graph → Phase 3 feature wave → Phase 4 contract/security/e2e → Phase 5 hardening. run state `dev/<nf>/_implementation_run_state.yaml` 로 stop/resume. *no spec semantic rediscovery* 강제. 실제 NSSF Phase 1 산출은 PR G | `src/`, `generated/`, `sql/`, `tests/`, `vendored/`, CI 산출 |

### Internal subroutine skills (public wrappers 내부에서 호출; 새 계약 / override 상황에서만 직접 호출)

| 단계 | 호환 skill | canonical skill | 기능 | 주요 산출물 |
|---|---|---|---|---|
| Spec discovery | `/nf-init <nf> --primary <spec>` | `/nf-spec-discover <nf> --primary <spec>` | primary/ref spec 식별, manifest 생성·보강, ready 시 seed auto-gen | `design/<nf>/_manifest.yaml`, `design/<nf>/_contract_seed.yaml` |
| Reset + rediscovery | `/nf-init <nf> --primary <spec> --reset` | `/nf-spec-discover <nf> --primary <spec> --reset` | contract 산출만 archive 후 manifest refresh + seed auto-gen | `design/<nf>/_archive/<ts>/`, manifest/seed 보존·갱신 |
| Contract extraction | `/nf-build <nf>` | `/nf-contract-build <nf>` | spec-derived contract markdown/json 과 handoff contract 생성 (PR B 에서 data-model field table / generated-vs-wrapper 판단 추가) | `design/<nf>/contract/...`, `handoff/<nf>/contract.yaml` |
| Contract validation | `/nf-status <nf>` | `/nf-contract-check <nf>` | contract 가 implementation 입력으로 충분한지 검사 (PR B 에서 `contract_implementable` gate 추가) | `design/<nf>/_contract_status.yaml` |
| Architecture design | 없음 | `/nf-arch-design <nf>` | contract 를 module/flow/state/error/test seam 으로 변환 | `design/<nf>/architecture/*` |
| Architecture validation | 없음 | `/nf-arch-status <nf>` | architecture 자기 일관성 검사 (`arch_consistent`) | `design/<nf>/_arch_status.yaml` |
| Implementation planning | 없음 | `/nf-impl-plan <nf>` | 아키텍처를 implementation readiness pack 으로 변환 (PR C 에서 Agent Execution Pack + Human Review Pack 9 파일 산출 추가) | `dev/<nf>/implementation-plan.md`, `tasks.yaml`, `test-matrix.md`, `traceability.md`, readiness pack 9 파일 |
| Implementation validation | 없음 | `/nf-impl-status <nf>` | impl plan 자기 일관성 + codegen 진입 가능 검사 (PR C 에서 `impl_ready_for_codegen` gate 추가) | `dev/<nf>/_impl_status.yaml` |
| Engineering design | 없음 | `/nf-eng-design <nf>` | tech decision freeze (library / DB / runtime / tool / policy) | `engineering/<nf>/engineering-design.md` |
| Engineering validation | 없음 | `/nf-eng-status <nf>` | engineering decision freeze 검사 (`eng_frozen`) | `engineering/<nf>/_engineering_status.yaml` |

세부 wrapper skill 직접 호출은 *예외 경우* — third-party library 교체, DBMS 교체, security policy 변경, spec 추가/제외, public contract 변경 등 "새 계약" 이 필요할 때만. 일반 NF 작업은 `/nf-readiness` + `/nf-implement` 2 행위로 처리.

`nf-reset` 은 별도 skill 이 아니라 `/nf-init --reset` 으로 통합된 destructive option 이다.

### NF registry (PR E1, 2026-05-22)

`design/nf-registry.yaml` 는 *durable NF catalog / routing source* 다 (plan §3.1). `design/scripts/nf-registry-bootstrap.py` 가 `specs/` 폴더의 OpenAPI 신호 (filename `TS<n>_N<nf>_...`, `info.title`, `paths` prefix) 로 NF 별 primary_spec 후보를 발견하고 `generated.nfs` 영역만 갱신한다. `manual_overrides.nfs` 는 사람 전용 — bootstrap 은 절대 덮어쓰지 않는다.

primary_spec_confidence 정책. **high** = 2+ 독립 signal type + 충돌 없음. **medium** = 1 signal type. **low** = 다중 NF 후보 / known-table fallback / 신호 없음. `/nf-readiness` wrapper (PR E2) 가 confidence ≠ high 일 때 manual_overrides 확인 후 진행 여부를 결정한다.

bootstrap 은 `--write` 없이 dry-run 이 기본. registry 재생성은 `.venv/bin/python3 design/scripts/nf-registry-bootstrap.py --write`. NSSF / NRF / UDM 의 기존 산출 (`design/<nf>/_manifest.yaml`, `_contract_seed.yaml`, `contract/`, `architecture/`, `dev/`, `engineering/`) 은 본 bootstrap 으로 변경되지 않는다 (PR F 가 책임).

## 더 보기

- [`CLAUDE.md`](./CLAUDE.md) — repo-local agent 정책.
- [`docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md`](./docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md) — lifecycle 단계와 skill vocabulary 결정.
- [`docs/adr/ADR-0002-engineering-design-freeze.md`](./docs/adr/ADR-0002-engineering-design-freeze.md) — `eng_frozen` 게이트 (PR D 에서 readiness 구성요소로 의미 좁힘 예정).
- [`docs/adr/ADR-0004-project-security-baseline.md`](./docs/adr/ADR-0004-project-security-baseline.md) — TLS / mTLS / OAuth2 production-capable code path 의무 source.
- [`docs/artifact-management.md`](./docs/artifact-management.md) — 원본·재생성 산출물·reviewed lifecycle 산출물·작업 계획 문서의 파일 관리 기준.
- [`docs/plans/2026-05-21-nf-readiness-implementation-workflow-upgrade-plan.md`](./docs/plans/2026-05-21-nf-readiness-implementation-workflow-upgrade-plan.md) — `/nf-readiness` + `/nf-implement` workflow upgrade 마스터 plan (PR A~G 사이클).
- 세부 schema/gate/tool 동작은 `design/scripts/*` docstring 과 `.claude/skills/*/SKILL.md` 가 진실 출처다.
