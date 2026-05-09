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
bash scripts/setup.sh                         # 나머지 자동 + 선택 항목 대화형 가이드
```

`scripts/setup.sh` 가 venv 생성, 의존성 (`pypdf`·`python-docx`·`pyyaml`) 설치, `extract.py` smoke test, mermaid-cli 선택 설치를 처리한다. `.doc` 지원·Obsidian·`docs/checklist.md` 동기화 같은 선택 항목은 질문으로 가이드. 비대화형은 `--yes` / `--no-prompts`. 옵션·수동 폴백 절차는 `bash scripts/setup.sh --help` 또는 스크립트 docstring 이 진실 출처.

## 작업 사이클

NF 한 개를 KB 에 등록·완성하는 흐름은 4 SKILL 의 사이클로 표현.

| 단계 | SKILL | 책임 |
|---|---|---|
| 매니페스트 보강 | `/nf-init <nf> --primary <spec>` | specs/ 의존성 자동 검출. ready 까지 반복 호출 |
| 페이지 빌드 | `/nf-build <nf> [--<category>]` | 7 카테고리 페이지 생성·갱신 + `index.md` 항목 갱신 |
| 완성도 검사 | `/nf-status <nf>` | acceptance gate 평가, FAIL 마다 `to_pass` 액션 보고 |
| 백업·재시작 | `/nf-reset <nf> [--full]` | 현 산출을 archive 폴더로 mv 후 fresh 빌드 |

각 SKILL 절차의 진실 출처는 `.claude/skills/<name>/SKILL.md`.

## Browsing with Obsidian

[Obsidian](https://obsidian.md/) (무료) 에 본 폴더를 Vault 로 열면 `[[wikilinks]]`, graph view, 전문 검색 사용 가능. Obsidian 은 읽기만 하므로 agent 의 편집과 충돌하지 않는다.

`.gitignore` 가 `.obsidian/workspace*`·`cache` 등 per-user 상태 파일은 제외하므로, 플러그인 설정만 공유된다.

## 더 보기

- [`CLAUDE.md`](./CLAUDE.md) — 디렉터리 규칙·언어 정책·THE FOUR RULES·acceptance gate 등 *정책* 의 진실 출처.
- [`docs/checklist.md`](./docs/checklist.md) — 부트스트랩·신규 페이지 추가·운영 점검 체크리스트.
- [`index.md`](./index.md) — KB 페이지 카탈로그 (NF 별 인덱스).
