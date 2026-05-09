# 5gc-impl-kb

3GPP 5G Core 의 *core NF (TS 29.531 NSSF, TS 29.510 NRF 등) 를 software 로 구현하는 데 필요한 모든 지식* 을 카테고리별로 정리한 knowledge base. 단순 spec 요약 wiki 가 아니라 *구현용* — `$ref` chain 이 끝까지 풀려 C struct 로 떨어질 수 있어야 하고, 표·그림이 보존되어야 하며, cross-NF 호출 그래프가 명확해야 한다.

```
specs/{spec}/{file}.{pdf,docx,doc,yaml}
        │
        ▼
   매니페스트 (kb/<nf>/_manifest.yaml)
   yaml $ref chain + docx clause 2 References 자동 검출
        │
        ▼
   7 카테고리 페이지 (kb/<nf>/3gpp-{ts|tr}-{n}.md)
   ├── Interface         (URI, transport, auth)
   ├── API               (operation 매트릭스)
   ├── Data Model        (chain-resolved 트리)
   ├── Service Scenarios (mermaid sequenceDiagram)
   ├── Cross-NF Deps     (호출 그래프)
   ├── Configuration     (feature, default, timeout)
   └── Error Handling    (code · cause · 복구)
        │
        ▼
   완성도 평가 (kb/<nf>/_status.yaml) — Tier 1~4 acceptance gate
```

## Quick start

```bash
git clone <repo> && cd 5gc-impl-kb
sudo apt install python3-venv python3-pip   # 사용자 권한 필요 — 한 번만
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
```

의존성은 `requirements.txt` 가 진실 출처 (`pypdf`·`python-docx`·`pyyaml`). 본 repo 의 모든 도구는 `.venv/bin/python3 scripts/<name>.py` 형태로 호출하므로 venv 활성화는 불필요. `.doc` (legacy MS Word) 처리가 필요하면 별도로 `sudo apt install libreoffice-core` 또는 `antiword`.

## 작업 사이클 — 4 SKILL

NF 한 개를 KB 에 등록·완성·재시작하는 흐름은 다음 4 SKILL 의 사이클.

- `/nf-init <nf> --primary <spec>` — 매니페스트 보강 (반복).
- `/nf-build <nf> [--<category>]` — 7 카테고리 페이지 생성·갱신.
- `/nf-status <nf>` — acceptance gate 평가, FAIL 마다 다음 액션 보고.
- `/nf-reset <nf> [--full]` — 백업 후 재시작.

책임 매트릭스 + 정책의 진실 출처는 [`CLAUDE.md`](./CLAUDE.md) "작업 흐름" 섹션, 절차 세부는 `.claude/skills/<name>/SKILL.md`.

## Browsing with Obsidian

[Obsidian](https://obsidian.md/) (무료) 에 본 폴더를 Vault 로 열면 `[[wikilinks]]`, graph view, 전문 검색 사용 가능. Obsidian 은 읽기만 하므로 agent 의 편집과 충돌하지 않는다.

`.gitignore` 가 `.obsidian/workspace*`·`cache` 등 per-user 상태 파일은 제외하므로, 플러그인 설정만 공유된다.

## 더 보기

- [`CLAUDE.md`](./CLAUDE.md) — 디렉터리 규칙·언어 정책·THE FOUR RULES·acceptance gate 등 *정책* 의 진실 출처.
- [`index.md`](./index.md) — KB 페이지 카탈로그 (NF 별 인덱스).
