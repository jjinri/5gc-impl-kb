# LLM Wiki — 3GPP 5G Core Systems (5GC)

3GPP 5G Core 시스템 스펙·관련 paper 의 개인 지식 베이스. [Karpathy 의 LLM Wiki 패턴](https://gist.github.com/karpathy/1dd0294ef9567971c1e4348a90d69285) 을 기반으로, [joonan30 의 fork](https://gist.github.com/joonan30/cbce305684d079dbe9a3fbaefe4e3959) 를 본 프로젝트 도메인(3GPP)에 맞게 적응.

```
Original spec doc → sources/*.md (LLM 요약, 한국어) → wiki/{nf}/*.md (정리된 페이지, 한국어)
```

> **Inheritance.** 전역 `~/.claude/CLAUDE.md` 의 모든 행동 규칙(가정 금지·단순성·외과적 변경·테스트 우선·시맨틱 커밋·에러는 추측 말고 읽기 등)을 그대로 따른다. 이 파일은 그 위에 본 프로젝트 고유 규칙만 추가한다.

> **Language policy.**
> - **wiki 본문(prose)·`sources/*.md`·`index.md` 설명문은 한국어**로 작성한다.
> - YAML frontmatter, 섹션 헤더(`## Summary` 등), 3GPP 용어·약어(NSSF, AMF, SUCI, S-NSSAI 등)는 영어 원문 유지.
> - 스펙 정의 문구는 영어 원문 인용 + 한국어 해설 병기.
> - 대화·커밋 메시지는 한국어 그대로.
>
> **Override — 전역 §5 (Korean output)**: wiki 본문에도 적용. 한국어 문장은 `:` 이 아니라 `.`/`?`/`!` 로 끝낸다. 콜론은 코드/key-value/라벨 안에서만.
>
> **Note — 전역 §6 (Korean header comments)**: 마크다운/YAML 파일에는 해당 없음. 단 `scripts/*.py` 등 코드 파일에는 적용.

---

## THE FOUR RULES (do not violate)

매 응답에 적용. overview 페이지에서도 wiki 에 실제 존재하는 spec/paper 만 인용한다.

1. **No web search.** `WebSearch`/`WebFetch` 로 빈틈을 메우지 않는다. 모든 답은 우리가 가진 문서에 근거한다.
2. **Answer from the wiki first.** `sources/` 와 `wiki/` 만이 진실의 출처다.
3. **If the wiki is insufficient, re-read the source document.** `papers/{spec}/{file}` 를 `scripts/extract.py` 로 다시 추출하고 wiki 를 갱신한다.
4. **If the wiki has no document on the topic, say so.** *"해당 주제의 문서가 wiki 에 없습니다 — PDF/DOC 를 주세요."* 임의 추정 금지.

---

## Repository Structure

```
llm-wiki/
├── CLAUDE.md
├── index.md                # 페이지 카탈로그
├── checklist.md            # 사용자용 워크플로우 체크리스트
├── context-notes.md        # 결정 기록
├── papers/                 # 원본 문서 (.pdf / .doc / .docx, cp only — symlink 금지)
│   └── {spec-number-with-dot}/
│       └── {original-3gpp-filename}.{ext}
├── sources/                # LLM 1차 요약 (한국어)
│   └── {stem}.md
├── wiki/                   # 정리된 wiki 페이지 (한국어)
│   ├── {nf}/               # NF 단위 폴더 (nssf, amf, smf, ...)
│   ├── architecture/
│   ├── interfaces/
│   ├── security/
│   ├── slicing/
│   ├── concepts/
│   ├── overviews/          # 시너지 페이지 (compounding)
│   └── other/
├── scripts/
│   └── extract.py          # .pdf/.doc/.docx 통합 추출 헬퍼
└── .venv/                  # Python venv (gitignored)
```

---

## File Naming Convention

### Original (`papers/`) — 3GPP 원본 보존

```
papers/{spec-number-with-dot}/{original-3gpp-filename}.{ext}
```

- spec-number 폴더는 점 포함 정식 표기 (`29.531`, `23.501`, `29.503`)
- 파일명은 3GPP 다운로드 원본 그대로 (`29531-i40.docx`). release/version 인코딩이 이미 들어있어 재명명 불필요
- 같은 spec 의 다른 release/version 은 **같은 폴더**에 공존 (`29531-h60.docx` + `29531-i40.docx`)
- 같은 버전의 다른 포맷도 공존 가능 (`29531-i40.docx` + `29531-i40.pdf`)

### Normalized stem (`sources/`, `wiki/`)

```
{stem} = 3gpp-{ts|tr}-{number-no-dot}-v{version}        # source 1개당 1버전
{stem} = 3gpp-{ts|tr}-{number-no-dot}                   # wiki 시리즈 페이지 (canonical)
```

예시:
- `sources/3gpp-ts-29531-v18.4.0.md` (특정 버전 요약)
- `wiki/nssf/3gpp-ts-29531.md` (시리즈 정리 페이지, 본문에 버전 이력 노트)

비-3GPP paper 는 Karpathy 원안 그대로:
```
{first-author-lastname}-{year}-{first-5-title-words}.{ext}
```

---

## 매핑 — Karpathy 1:1:1 에서의 의도적 분기

| 계층 | 단위 | 명명 |
|---|---|---|
| `papers/` | spec **버전마다** 1파일 (멀티 포맷·멀티 릴리즈 공존) | 3GPP 원본 파일명 |
| `sources/` | 실제로 요약한 **버전마다** 1개 | `3gpp-ts-{n}-v{ver}.md` |
| `wiki/{nf}/` | spec **시리즈마다** 1개 (canonical/현재 버전 중심) | `3gpp-ts-{n}.md`. 본문 `## Version History` 섹션에 변경점 |

이유: 3GPP 스펙은 같은 시리즈의 여러 release/version 이 공존한다. wiki 페이지를 시리즈 단위로 두면 사용자가 "TS 29.503 가 뭐냐" 라는 자연스러운 질문에 한 페이지로 답할 수 있다.

---

## Categories

```
wiki/
├── nssf/         # ★ 시작점 — Network Slice Selection Function (TS 29.531 등)
├── {nf}/         # NF 추가 시 폴더 생성. 소문자 그대로:
│                 #   amf, smf, upf, nrf, ausf, udm, udr,
│                 #   pcf, nef, nwdaf, chf, bsf, scp, sepp
├── architecture/ # 5GC 전체 아키텍처·절차 (TS 23.501, TS 23.502)
├── interfaces/   # SBI, N1/N2/N4/N6, Namf/Nsmf/Nnssf, HTTP/2, OpenAPI
├── security/     # AKA, key hierarchy, SUCI/SUPI (AUSF/UDM 와 교차)
├── slicing/      # S-NSSAI, NSI, NSACF — NSSF 외 cross-NF 관점
├── concepts/     # SBA, OAuth 2.0, JSON Patch 등 일반화된 개념
├── overviews/    # 여러 NF/spec 을 가로지르는 synthesis
└── other/        # 잔여
```

**분류 규칙**: NF 고유 spec (예: TS 29.531 = Nnssf 서비스) → 해당 NF 폴더. 아키텍처 spec (TS 23.501) → `architecture/`. 슬라이싱처럼 여러 NF 에 걸치는 주제 중 *NF 외부 관점* → `slicing/`. NF 폴더는 **첫 문서가 들어갈 때** 생성한다 (전역 §2 단순성).

---

## Adding a New Document

### Step 0 — Spec 시리즈 폴더 확보

```bash
SPEC=29.503    # 점 포함 정식 표기
mkdir -p ~/AI/llm-wiki/papers/$SPEC
```

### Step 1 — 원본 파일 cp (절대 symlink 금지)

```bash
cp ~/Downloads/29503-i40.docx ~/AI/llm-wiki/papers/29.503/
```

### Step 2 — 텍스트 추출

```bash
cd ~/AI/llm-wiki
python3 scripts/extract.py papers/29.503/29503-i40.docx
# stdout 으로 처음 ~12,000 chars 출력. 긴 문서는 페이지/문단 단위로 잘라서 호출.
```

지원 포맷: `.pdf` (pypdf), `.docx` (python-docx), `.doc` (soffice 또는 antiword 필요).

### Step 3 — `sources/{stem}.md` 작성 (한국어 본문)

```yaml
---
title: "Network Slice Selection Services"      # 영문 원제 보존
authors: 3GPP CT4
spec: TS 29.531
release: 18
version: 18.4.0
year: 2024
category: nssf
source_path: /home/jjinri/AI/llm-wiki/papers/29.531/29531-i40.docx
source_filename: 29531-i40.docx
source_format: docx
source_collection: 3gpp
---

## One-line Summary
[한국어 한 줄 요약]

## 1. Document Information
## 2. Key Contributions
## 3. Methodology and Architecture
## 4. Key Procedures and Messages
## 5. Limitations and Open Issues
## 6. Related Specs
## 7. Glossary
```

### Step 4 — `wiki/{nf}/{stem}.md` 작성 (시리즈 페이지, 한국어)

```yaml
---
title: "Network Slice Selection Services"
authors: 3GPP CT4
spec: TS 29.531
release: 18
version: 18.4.0    # 현재 wiki 가 정리한 기준 버전
year: 2024
source: 3gpp-ts-29531-v18.4.0.md
category: nssf
source_path: /home/jjinri/AI/llm-wiki/papers/29.531/29531-i40.docx
source_filename: 29531-i40.docx
source_format: docx
source_collection: 3gpp
related_versions:
  - { release: 17, version: 17.6.0, source_filename: 29531-h60.docx }
tags: [nssf, sbi, slice-selection]
---

## Summary
[한국어 prose. 약어·메시지명·필드명은 영어 그대로]

## Key Contributions
## Methodology and Architecture
## Key Procedures
## Version History
- v18.4.0 (Rel-18, 2024) — [주요 변경점]
- v17.6.0 (Rel-17) — [...]

## Related Pages
- [[architecture/3gpp-ts-23501]] — 5GC 전체 아키텍처 맥락
- [[slicing/snssai-and-nsi]] — S-NSSAI 개념
```

### Step 5 — `index.md` 갱신

해당 카테고리 아래 한 줄 항목 추가 (한국어 설명).

---

## Source Document Management Rules

- **항상 cp, 절대 symlink 금지.** 외부 경로(`~/Downloads/` 등)에서 `papers/` 안으로 복사한다.
- `source_path` 는 항상 `papers/` 내부의 nested 경로 (`papers/{spec}/{file}`).
- `source_filename` 은 `basename(source_path)` 와 정확히 일치.
- `source_format` 은 확장자와 일치 (`pdf` | `doc` | `docx`).
- 같은 spec 시리즈의 다른 버전·포맷은 **같은 spec-number 폴더**에 공존.
- 폴더명은 점 포함 (`29.531`), 파일명은 3GPP 원본 보존.

---

## Knowledge Compounding

가장 가치 있는 페이지는 개별 spec 요약이 아니라 `wiki/overviews/` 의 합성 페이지다. 좋은 답이 나오면 사용자에게:

> "Save this as an overview page in `wiki/overviews/`"

대화 1회당 5–15개의 wiki 페이지 신규/갱신을 목표로 한다.

---

## Browsing with Obsidian

[Obsidian](https://obsidian.md/) (무료) 에 `~/AI/llm-wiki` 를 Vault 로 열면 `[[wikilinks]]`, graph view, 전문 검색 사용 가능. Obsidian 은 읽기만 하므로 agent 의 편집과 충돌하지 않는다.

`.gitignore` 가 `.obsidian/workspace*`·`cache` 등 per-user 상태 파일은 제외하므로, 플러그인 설정만 공유된다.

---

## Setup

### 자동 셋업 (권장)

```bash
git clone <repo> && cd llm-wiki
sudo apt install python3-venv python3-pip   # 사용자 권한 필요 — 한 번만
bash scripts/setup.sh                         # 나머지 자동 + 선택 항목 대화형 가이드
```

`scripts/setup.sh` 가 venv 생성, `pypdf`/`python-docx` 설치, `extract.py` 동작 검증(.docx smoke test) 까지 자동 처리한다. 그 후 `.doc` 지원·Obsidian·`checklist.md` 동기화 같은 선택 항목을 질문으로 가이드한다. 비대화형 재실행은 `bash scripts/setup.sh --yes` 또는 `--no-prompts`.

### 수동 폴백

setup.sh 가 동작하지 않는 환경이면 직접 실행한다.

```bash
sudo apt install python3-venv python3-pip
python3 -m venv .venv
.venv/bin/pip install pypdf python-docx
```

`.doc` (legacy MS Word) 지원이 필요하면 다음 중 하나를 추가로 설치한다.

```bash
sudo apt install libreoffice-core   # 권장 (포맷 정확도 높음)
sudo apt install antiword            # 가벼운 대안
```

`scripts/extract.py` 가 자동으로 `soffice` 또는 `antiword` 를 찾아 사용한다.

---

## When in doubt, follow rule #1.
