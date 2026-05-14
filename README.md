# 5gc-impl-kb

3GPP spec 으로부터 LLM agent 가 NF 개발 입력물을 단계적으로 생산하는 **5gc design-to-dev knowledge base**. 현재 repo 의 핵심 산출은 spec-derived NF contract 이며, 이후 상세 아키텍처 설계와 구현 계획 단계로 이어진다.

```text
specs/{spec}/{file}.{pdf,docx,doc,yaml}
        │
        ▼  spec discovery
   design/<nf>/_manifest.yaml
        │
        ▼  contract extraction
   design/<nf>/contract/...        # handoff-v2 topic contract artifacts
   handoff/<nf>/_handoff.yaml      # 현재 machine-readable dev contract
        │
        ▼  contract validation
   design/<nf>/_status.yaml
        │
        ▼  architecture design
   design/<nf>/architecture/*
        │
        ▼  implementation planning
   dev/<nf>/implementation-plan.md
```

정확한 lifecycle 용어와 canonical skill 이름은 [`docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md`](./docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md) 를 따른다.

## Quick start

```bash
git clone https://github.com/jjinri/5gc-impl-kb.git && cd 5gc-impl-kb
sudo apt install python3-venv python3-pip   # 사용자 권한 필요 — 한 번만
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
```

의존성은 `requirements.txt` 가 진실 출처 (`pypdf`·`python-docx`·`pyyaml`). 본 repo 의 모든 도구는 `.venv/bin/python3 design/scripts/<name>.py` 형태로 호출하므로 venv 활성화는 불필요. `.doc` 처리가 필요하면 별도로 `sudo apt install libreoffice-core` 또는 `antiword`.

## 사람이 호출하는 lifecycle skill

현재 호환 명령과 canonical 이름을 분리한다. `/nf-spec-discover`, `/nf-contract-build`, `/nf-contract-check` 는 현재 wrapper 로 제공되며, 기존 `/nf-init`, `/nf-build`, `/nf-status` 도 호환 alias 로 유지한다. README 는 사용자 trigger 표면만 보여주고, 각 skill 이 내부에서 실행할 script/check 절차는 `.claude/skills/<name>/SKILL.md` 가 맡는다.

| 단계 | 호환 skill | canonical skill | 기능 | 주요 산출물 |
|---|---|---|---|---|
| Spec discovery | `/nf-init <nf> --primary <spec>` | `/nf-spec-discover <nf> --primary <spec>` | primary/ref spec 식별, manifest 생성·보강 | `design/<nf>/_manifest.yaml` |
| Reset + rediscovery | `/nf-init <nf> --primary <spec> --reset` | `/nf-spec-discover <nf> --primary <spec> --reset` | 기존 산출 archive 후 manifest 재생성 | `design/<nf>/_archive/<ts>/`, 새 manifest |
| Contract extraction | `/nf-build <nf>` | `/nf-contract-build <nf>` | spec-derived contract markdown/json 과 handoff contract 생성 | `design/<nf>/contract/...`, `handoff/<nf>/_handoff.yaml` |
| Contract validation | `/nf-status <nf>` | `/nf-contract-check <nf>` | contract 가 architecture 설계 입력으로 충분한지 검사 | `design/<nf>/_status.yaml` |
| Architecture design | 없음 | `/nf-arch-design <nf>` | contract 를 상세 아키텍처로 변환 | `design/<nf>/architecture/*` |
| Implementation planning | 없음 | `/nf-impl-plan <nf>` | 아키텍처를 구현 작업·테스트 계획으로 분해 | `dev/<nf>/implementation-plan.md`, `tasks.yaml` |

`nf-reset` 은 별도 skill 이 아니라 `/nf-init --reset` 으로 통합된 destructive option 이다.

## 더 보기

- [`CLAUDE.md`](./CLAUDE.md) — repo-local agent 정책.
- [`docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md`](./docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md) — lifecycle 단계와 skill vocabulary 결정.
- [`docs/plans/2026-05-13-lifecycle-structure-skill-rename-plan.md`](./docs/plans/2026-05-13-lifecycle-structure-skill-rename-plan.md) — 구조 변경 실행 계획.
- 세부 schema/gate/tool 동작은 `design/scripts/*` docstring 과 `.claude/skills/*/SKILL.md` 가 진실 출처다.
