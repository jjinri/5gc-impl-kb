# 5gc-impl-kb — 3GPP 5G Core 구현 KB

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

> **Inheritance.** 전역 `~/.claude/CLAUDE.md` 의 모든 행동 규칙 (가정 금지·단순성·외과적 변경·테스트 우선·시맨틱 커밋·에러는 추측 말고 읽기 등) 을 그대로 따른다. 이 파일은 그 위에 본 프로젝트 고유 규칙만 추가한다.

> **Language policy.**
> - **kb 본문(prose)·`digests/*.md`·`index.md` 설명문은 한국어**로 작성한다.
> - YAML frontmatter, 섹션 헤더(`## Interface` 등), 3GPP 용어·약어(NSSF, AMF, SUCI, S-NSSAI 등) 는 영어 원문 유지.
> - 스펙 정의 문구는 영어 원문 인용 + 한국어 해설 병기.
> - 대화·커밋 메시지는 한국어 그대로.
>
> **Override — 전역 §5 (Korean output).** kb 본문에도 적용. 한국어 문장은 `:` 이 아니라 `.`/`?`/`!` 로 끝낸다. 콜론은 코드/key-value/라벨 안에서만.
>
> **Note — 전역 §6 (Korean header comments).** 마크다운/YAML 비대상. `scripts/*.py` 등 코드 파일에는 적용.

---

## THE FOUR RULES (do not violate)

매 응답에 적용. 본 KB 의 implementation 정의에 맞춰 강화됨.

1. **No web search.** `WebSearch`/`WebFetch` 로 빈틈을 메우지 않는다. 모든 답은 `specs/` 와 `kb/` 에 근거한다.
2. **Answer from kb first.** `kb/`·`digests/` 의 7 카테고리 페이지가 진실의 출처. spec 본문은 그 출처의 정당성을 뒷받침할 때 재추출한다.
3. **If kb is insufficient, re-read the source.** `specs/{spec}/{file}` 를 `scripts/extract.py`·`scripts/resolve-yaml-refs.py` 로 다시 추출하고, 그 결과로 kb 를 *재빌드* 한다 (`/nf-build <nf>` 호출).
4. **If chain ends incomplete, say so explicitly.** `(참조 규격 미등록)` leaf, 누락 mermaid, 비어있는 카테고리는 *침묵하지 않는다* — `/nf-status` 가 FAIL 로 잡고 `to_pass` 로 다음 액션을 알린다. 임의 추정으로 leaf 를 메우지 않는다.

---

## 작업 흐름 — NF-centric (3 SKILL 한 사이클)

본 KB 의 모든 신규 작업은 다음 3 SKILL 의 사이클로 표현된다.

| 단계 | SKILL | 책임 |
|---|---|---|
| 1. 매니페스트 보강 | `/nf-init <nf> --primary <spec>` | papers/ 의존성 자동 검출. `ready_for_build` 까지 반복 호출 |
| 2. 페이지 빌드 | `/nf-build <nf> [--<category>]` | 7 카테고리 implementation-grade 페이지 생성·갱신 |
| 3. 완성도 검사 | `/nf-status <nf>` | acceptance gate 평가, FAIL 마다 `to_pass` 액션 보고 |

각 SKILL 의 절차 *세부* 는 해당 `SKILL.md` 가 진실의 출처. CLAUDE.md 는 *정책* 만 정의하고 절차는 SKILL 로 위임한다 (ADR 0001 D5).

> 신규 머신 onboarding — `bash scripts/setup.sh` 한 번 후 `/nf-init <nf> --primary <spec>` 으로 시작. `docs/checklist.md` 의 부트스트랩 체크리스트 참고.

---

## Repository Structure

```
5gc-impl-kb/
├── CLAUDE.md                    # 본 파일 — 정책
├── index.md                     # 페이지 카탈로그
├── docs/
│   ├── checklist.md             # 사용자용 워크플로우 체크리스트
│   └── decisions/               # ADR — 결정 기록
│       ├── 0000-bootstrap-decisions.md
│       └── 0001-implementation-grade-redesign.md
├── specs/                       # 3GPP 원본 (.pdf / .doc / .docx / .yaml, cp only — symlink 금지)
│   └── {spec-number-with-dot}/
├── digests/                     # LLM 1차 한국어 발췌 (버전 단위)
│   └── {stem}.md
├── kb/                          # implementation-grade 페이지 (시리즈 단위)
│   ├── {nf}/                    # NF 단위 폴더 (nssf, nrf, amf, smf, ...)
│   │   ├── 3gpp-{ts|tr}-{n}.md
│   │   ├── _manifest.yaml       # /nf-init 산출 — 의존성 + ready_for_build
│   │   ├── _status.yaml         # /nf-status 산출 — acceptance gate
│   │   └── _diagrams/           # mermaid → SVG 산출
│   ├── architecture/            # 5GC 전체 아키텍처 (TS 23.501 등)
│   ├── interfaces/, security/, slicing/, concepts/, overviews/, other/
├── scripts/
│   ├── extract.py               # .pdf/.doc/.docx → text
│   ├── nf-manifest.py           # NF 의존성 자동 검출 → _manifest.yaml
│   ├── resolve-yaml-refs.py     # OpenAPI yaml $ref chain 추적
│   ├── render-mermaid.py        # mermaid → SVG
│   ├── nf-status.py             # _status.yaml 산출
│   └── setup.sh                 # 자동 셋업
├── .claude/
│   └── skills/
│       ├── nf-init/SKILL.md     # /nf-init
│       ├── nf-build/SKILL.md    # /nf-build
│       ├── nf-status/SKILL.md   # /nf-status
│       └── _legacy/             # 구 add-spec, update-spec (sprint 1 끝까지 보존)
└── .venv/, .npm-tools/          # gitignored
```

---

## File Naming Convention

### Original (`specs/`) — 3GPP 원본 보존

```
specs/{spec-number-with-dot}/{original-3gpp-filename}.{ext}
```

- spec-number 폴더는 점 포함 정식 표기 (`29.531`, `23.501`, `29.503`).
- 파일명은 3GPP 다운로드 원본 그대로 (`29531-i40.docx`). release/version 인코딩 (`-{letter}{N}{M}`) 이 이미 들어있어 재명명 불필요.
- 같은 spec 의 다른 release/version 은 **같은 폴더** 에 공존. 같은 버전의 다른 포맷 (`.docx` + `.pdf`) 도 공존 가능.
- 같이 들어있는 OpenAPI yaml (예 `TS29531_Nnssf_NSSelection.yaml`) 도 같은 폴더.

### Normalized stem (`digests/`, `kb/`)

```
{stem} = 3gpp-{ts|tr}-{number-no-dot}-v{version}        # digests — 버전마다 1개
{stem} = 3gpp-{ts|tr}-{number-no-dot}                   # kb 시리즈 페이지
```

예시 — `digests/3gpp-ts-29531-v19.6.0.md`, `kb/nssf/3gpp-ts-29531.md`.

---

## 매핑 — 3-tier (Karpathy 1:1:1 에서의 의도적 분기)

| 계층 | 단위 | 명명 |
|---|---|---|
| `specs/` | spec **버전·포맷마다** 1파일 (멀티 포맷·멀티 릴리즈 공존) | 3GPP 원본 파일명 |
| `digests/` | 실제로 추출·요약한 **버전마다** 1개 | `3gpp-ts-{n}-v{ver}.md` |
| `kb/{nf}/` | spec **시리즈마다** 1개 (canonical) | `3gpp-ts-{n}.md`. 본문 `## Version History` 에 변경점 |

이유. 3GPP 시리즈는 같은 NF 의 여러 release 가 공존한다. kb 페이지를 시리즈 단위로 두면 사용자가 "TS 29.503 가 뭐냐" 라는 자연스러운 질문에 한 페이지로 답할 수 있고, Cross-NF 표·Data Model chain 의 cross-spec 추적이 자연스럽다.

---

## Categories

```
kb/
├── nssf/         # ★ 시작점 — Network Slice Selection Function (TS 29.531 등)
├── {nf}/         # NF 추가 시 폴더 생성. 소문자 그대로:
│                 #   amf, smf, upf, nrf, ausf, udm, udr,
│                 #   pcf, nef, nwdaf, chf, bsf, scp, sepp
├── architecture/ # 5GC 전체 아키텍처·절차 (TS 23.501, TS 23.502)
├── interfaces/   # SBI, N1/N2/N4/N6, Namf/Nsmf/Nnssf, HTTP/2, OpenAPI
├── security/     # AKA, key hierarchy, SUCI/SUPI (AUSF/UDM 와 교차)
├── slicing/      # S-NSSAI, NSI, NSACF — NSSF 외 cross-NF 관점
├── concepts/     # SBA, OAuth 2.0, JSON Patch 등 일반화된 개념
├── overviews/    # 여러 NF/spec 을 가로지르는 synthesis (cross-NF 호출 그래프 등)
└── other/        # 잔여
```

**분류 규칙**. NF 고유 spec (예 TS 29.531 = Nnssf) → 해당 NF 폴더. 아키텍처 spec (TS 23.501) → `architecture/`. 슬라이싱처럼 여러 NF 에 걸치는 *NF 외부 관점* → `slicing/`. NF 폴더는 **첫 문서가 들어갈 때** 생성한다 (전역 §2 단순성).

---

## NF Profile (D7)

매니페스트 `profile` 필드. 검사·빌드 동작이 profile 별로 다르다.

| profile | 정의 | 적용 NF 예 |
|---|---|---|
| `stage_3_only` | yaml 동봉 stage 3 spec primary | NSSF (29.531), NRF (29.510), UDM (29.503), AUSF (29.509) |
| `stage_2_only` | prose stage 2 spec primary, yaml 부재 | (드뭄) |
| `mixed` | stage 2 + stage 3 둘 다 primary | NWDAF (23.288 + 29.520), AMF (23.502 + 29.518) |
| `meta_only` | spec 자체가 cross-cutting (구현 NF 가 아님) | TS 23.501 본 spec 페이지 |

NF profile 별 적용 check 는 `scripts/nf-status.py` 의 `applies_to` 가 진실 출처.

---

## Acceptance Gates (D7 무가중 framework)

각 페이지의 완성도는 4 단계 gate 로 평가된다. Gate 는 *check id 의 AND* — 가중치 없음.

| Gate | 의미 | 통과 조건 |
|---|---|---|
| `draft` | 페이지 골격 형성 | frontmatter_valid |
| `ready_for_review` | 사람이 검토 가능한 상태 | + sections_complete + manifest_ready |
| `implementation_ready` | *구현 가능* — 본 KB 의 핵심 acceptance | + Tier 2 모든 항목 PASS |
| `production` | 자동·수동 검증 모두 통과 | + Tier 3 (yaml-to-c) + Tier 4 (review) |

`_status.yaml` 의 모든 FAIL check 는 `to_pass` 에 *다음 액션* 이 적혀있다 — silent FAIL 없음.

---

## Source Document Management Rules

- **항상 cp, 절대 symlink 금지.** 외부 경로(`~/Downloads/` 등)에서 `specs/` 안으로 복사한다.
- `source_path` 는 *repo-relative* 경로 (`specs/{spec}/{file}`). 프로젝트 디렉터리명에 의존하지 않는다.
- `source_filename` 은 `basename(source_path)` 와 정확히 일치.
- `source_format` 은 확장자와 일치 (`pdf` | `doc` | `docx`).
- 같은 spec 시리즈의 다른 버전·포맷은 **같은 spec-number 폴더** 에 공존.

---

## Decision records (ADR)

본 프로젝트의 결정 기록은 `docs/decisions/` 의 ADR 들이 진실의 출처.

- [`0000-bootstrap-decisions.md`](docs/decisions/0000-bootstrap-decisions.md) — 부트스트랩 시점의 12개 결정 (일부 supersede 됨).
- [`0001-implementation-grade-redesign.md`](docs/decisions/0001-implementation-grade-redesign.md) — 본 KB 를 implementation-grade 정의로 재정의 + 7 결정 (D1~D8) + open issues.

새 결정은 `0002-*.md` 부터 ADR 형식으로 추가. 기존 ADR 은 *immutable*, 변경 시 새 ADR 로 supersede.

---

## Knowledge depth, not breadth

본 KB 의 가치는 *NF 한 개당 깊이* 에서 나온다. 카테고리별 placeholder 로 페이지 수를 늘리는 것보다 한 NF 의 7 카테고리를 모두 implementation-ready 로 끌어올리는 것이 우선. 그렇게 끌어올린 NF 의 `kb/overviews/` 합성 (예 cross-NF 호출 그래프) 이 가장 가치 있는 산출.

---

## Browsing with Obsidian

[Obsidian](https://obsidian.md/) (무료) 에 본 폴더를 Vault 로 열면 `[[wikilinks]]`, graph view, 전문 검색 사용 가능. Obsidian 은 읽기만 하므로 agent 의 편집과 충돌하지 않는다.

`.gitignore` 가 `.obsidian/workspace*`·`cache` 등 per-user 상태 파일은 제외하므로, 플러그인 설정만 공유된다.

---

## Setup

### 자동 셋업 (권장)

```bash
git clone <repo> && cd 5gc-impl-kb
sudo apt install python3-venv python3-pip   # 사용자 권한 필요 — 한 번만
bash scripts/setup.sh                         # 나머지 자동 + 선택 항목 대화형 가이드
```

`scripts/setup.sh` 가 venv 생성, `pypdf`/`python-docx`/`pyyaml` 설치, `extract.py` 동작 검증(.docx smoke test), mermaid-cli 선택 설치까지 처리한다. 그 후 `.doc` 지원·Obsidian·`docs/checklist.md` 동기화 같은 선택 항목을 질문으로 가이드한다. 비대화형 재실행은 `bash scripts/setup.sh --yes` 또는 `--no-prompts`.

### 수동 폴백

setup.sh 가 동작하지 않는 환경이면 직접 실행한다.

```bash
sudo apt install python3-venv python3-pip
python3 -m venv .venv
.venv/bin/pip install pypdf python-docx pyyaml
```

`.doc` (legacy MS Word) 지원이 필요하면 `sudo apt install libreoffice-core` 또는 `antiword` 추가.

---

## When in doubt, follow rule #1.
