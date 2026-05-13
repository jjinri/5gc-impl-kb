# 5gc-impl-kb

3GPP spec 으로부터 LLM agent 자동 파이프라인으로 *NF design deliverable* 을 생산하는 **5gc-design 시스템**. `design/<nf>/3gpp-*.md` (7 카테고리 페이지) 와 `handoff/<nf>/_handoff.yaml` (5gc-dev contract) 가 주요 산출.

```
specs/{spec}/{file}.{pdf,docx,doc,yaml}
        │
        ▼  nf-manifest.py
   _manifest.yaml  (의존성 + ready_for_build)
        │
        ▼  nf-build SKILL
   design/<nf>/3gpp-{ts|tr}-{n}.md   (7 카테고리 페이지)
   ├── Interface         (URI, transport, auth)
   ├── API               (operation 매트릭스)
   ├── Data Model        (chain-resolved 트리)
   ├── Service Scenarios (mermaid sequenceDiagram)
   ├── Cross-NF Deps     (호출 그래프)
   ├── Configuration     (feature, default, timeout)
   └── Error Handling    (code · cause · 복구)
        │
        ▼  build-handoff.py  (nf-build 내 자동 호출)
   handoff/<nf>/_handoff.yaml   (5gc-dev contract — self-contained)
        │
        ▼  nf-status SKILL
   _status.yaml  (acceptance gate: draft → review_ready → handoff_ready → canonical)
```

## Quick start

```bash
git clone https://github.com/jjinri/5gc-impl-kb.git && cd 5gc-impl-kb
sudo apt install python3-venv python3-pip   # 사용자 권한 필요 — 한 번만
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
```

의존성은 `requirements.txt` 가 진실 출처 (`pypdf`·`python-docx`·`pyyaml`). 본 repo 의 모든 도구는 `.venv/bin/python3 design/scripts/<name>.py` 형태로 호출하므로 venv 활성화는 불필요. `.doc` (legacy MS Word) 처리가 필요하면 별도로 `sudo apt install libreoffice-core` 또는 `antiword`.

## 작업 사이클 — 3 SKILL

NF 한 개를 design 에 등록·완성·재시작하는 흐름은 다음 3 SKILL 의 사이클.

- `/nf-init <nf> --primary <spec> [--reset]` — 매니페스트 보강·재생성 (반복). `--reset` 은 기존 산출을 `design/<nf>/_archive/<ts>/` 로 백업 후 manifest 재생성 (백업·재시작 통합).
- `/nf-build <nf> [--<category>]` — 7 카테고리 페이지 생성·갱신 + `handoff/<nf>/_handoff.yaml` 자동 갱신.
- `/nf-status <nf>` — acceptance gate 평가, FAIL 마다 다음 액션 보고.

책임 매트릭스 + repo-local agent 정책은 [`CLAUDE.md`](./CLAUDE.md), 절차 세부는 `.claude/skills/<name>/SKILL.md`.

## 더 보기

- [`CLAUDE.md`](./CLAUDE.md) — repo-local agent 정책 (언어·THE FOUR RULES·PR 분기·design↔dev 책임 경계).
- 디렉터리·파일 명명·NF profile·acceptance gate 등 세부는 각 진실 출처 (`design/scripts/*` docstring, `docs/superpowers/specs/2026-05-12-*`).
