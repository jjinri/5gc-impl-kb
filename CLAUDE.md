# 5gc-impl-kb — 정책

본 KB 의 *정책 only* 문서. 프로젝트 소개·setup·browsing 은 [`README.md`](./README.md) 가 진실 출처.

> **Inheritance.** 전역 `~/.claude/CLAUDE.md` 의 모든 행동 규칙 (가정 금지·단순성·외과적 변경·테스트 우선·시맨틱 커밋·에러는 추측 말고 읽기 등) 을 그대로 따른다. 이 파일은 그 위에 본 프로젝트 고유 규칙만 추가한다.

> **Language policy.**
> - **design 본문(prose)·`index.md` 설명문은 한국어**로 작성한다.
> - YAML frontmatter, 섹션 헤더(`## Interface` 등), 3GPP 용어·약어(NSSF, AMF, SUCI, S-NSSAI 등) 는 영어 원문 유지.
> - 스펙 정의 문구는 영어 원문 인용 + 한국어 해설 병기.
> - 대화·커밋 메시지는 한국어 그대로.
>
> **Override — 전역 §5 (Korean output).** design 본문에도 적용. 한국어 문장은 `:` 이 아니라 `.`/`?`/`!` 로 끝낸다. 콜론은 코드/key-value/라벨 안에서만.
>
> **Note — 전역 §6 (Korean header comments).** 마크다운/YAML 비대상. `design/scripts/*.py` 등 코드 파일에는 적용.

> **CLAUDE.md 유지 정책.** 본 파일은 *정책* 만 — 절차는 `SKILL.md`, 결정의 *역사적 맥락* 이 필요하면 `git log` 가 진실 출처다. 새 항목 추가 전에 *기존 위치에 흡수 가능한지·다른 곳에 더 어울리는지* 검토. 중복·stale 발견 시 즉시 제거. 본 파일이 증식하면 검색·요약·리뷰 비용이 모두 늘어 정책 문서로서의 가치가 떨어진다. 본 파일을 수정할 때는 *순 변화량* 을 의식한다 — 추가만큼 정리.

---

## THE FOUR RULES (do not violate)

매 응답에 적용.

1. **No web search.** `WebSearch`/`WebFetch` 로 빈틈을 메우지 않는다. 모든 답은 `specs/` 와 `design/` 에 근거한다.
2. **Answer from design first.** `design/` 의 7 카테고리 페이지가 진실의 출처. spec 본문은 그 출처의 정당성을 뒷받침할 때 재추출한다.
3. **If design is insufficient, re-read the source.** `specs/{spec}/{file}` 를 `design/scripts/extract.py`·`design/scripts/resolve-yaml-refs.py` 로 다시 추출하고, 그 결과로 design 페이지를 *재빌드* 한다 (`/nf-build <nf>` 호출).
4. **If chain ends incomplete, say so explicitly.** `(참조 규격 미등록)` leaf, 누락 mermaid, 비어있는 카테고리는 *침묵하지 않는다* — `/nf-status` 가 FAIL 로 잡고 `to_pass` 로 다음 액션을 알린다. 임의 추정으로 leaf 를 메우지 않는다.

---

## 작업 흐름 — NF-centric (3 SKILL 한 사이클)

본 KB 의 모든 신규 작업은 다음 3 SKILL 의 사이클로 표현된다.

| 단계 | SKILL | 책임 |
|---|---|---|
| 1. 매니페스트 보강 | `/nf-init <nf> --primary <spec>` | specs/ 의존성 자동 검출. `ready_for_build` 까지 반복 호출 |
| 2. 페이지 빌드 | `/nf-build <nf> [--<category>]` | 7 카테고리 페이지 생성·갱신 + `index.md` 항목 갱신 + `handoff/<nf>/_handoff.yaml` 자동 갱신 |
| 3. 완성도 검사 | `/nf-status <nf>` | acceptance gate 평가, FAIL 마다 `to_pass` 액션 보고 |
| (선택) 백업·재시작 | `/nf-reset <nf> [--full]` | 현 산출을 `design/<nf>/_archive/<ts>/` 로 mv 후 `/nf-build` 로 fresh 빌드 |

각 SKILL 의 절차 *세부* 는 해당 `SKILL.md` 가 진실의 출처. CLAUDE.md 는 *정책* 만 정의하고 절차는 SKILL 로 위임한다. onboarding (Quick start, Obsidian) 은 README 에.

> 새 SKILL 작성·개선 — Anthropic Skill Creator 가이드 (<https://claude.ai/customize/skills>) 또는 설치된 plugin `/skill-creator:skill-creator` 참고. 본 프로젝트의 4 SKILL (nf-init/build/status/reset) 이 *원칙 + 이유 + 예시* 패턴의 모범 — 새 SKILL 도 같은 골격을 따른다.

---

## 원격 동기화 — 로컬 main 머지 후 PR 의무

로컬 `main` 에 작업 commits 가 머지된 직후 (worktree·feature 브랜치 FF/no-FF 무관) **원격 `main` 으로 직접 push 하지 않는다**. 다음 절차로 원격 PR 사이클을 거친다.

1. 머지된 commits 를 별도 push 브랜치로 분기. 예 `git switch -c push/<topic>-<yyyymmdd> <merge-commit-sha>` 후 `git push -u origin push/<topic>-<yyyymmdd>`. (worktree branch 가 이미 push 된 경우 그대로 사용 가능.)
2. `gh pr create --base main --head push/<topic>-<yyyymmdd>` 로 PR 작성. 제목·본문은 머지된 commits 의 *요약 + 이유 + 검증* — 머지 commit 메시지 또는 retro 문서를 그대로 reuse.
3. 리뷰·CI 후 원격에서 PR squash 또는 merge → 원격 `main` 갱신.
4. 로컬 `main` 은 `git pull --ff-only origin main` 로 정합. push 브랜치는 머지 확인 후 `git branch -d` + `git push origin --delete <branch>`.

이유 — 단독 작업자라도 원격 PR 이 *변경 일람 + 사유 + 재현 단계* 의 1급 보존소가 된다. 원격 `main` 직접 push 는 *왜* 의 흔적이 commit log 외 어디에도 남지 않아 후속 작업·롤백·감사가 어렵다.

본 정책은 *예외 없음*. 핫픽스 등 긴급 상황은 사용자가 *명시적으로* "원격 main 직접 push 해" 라고 지시했을 때만 우회.

### 머지 후 동기화 처치 — 머지 방식별 명령

| GitHub 머지 방식 | 원격 main 변화 | 로컬 main 상태 | 동기화 명령 |
|---|---|---|---|
| **Merge commit** | PR commits + 1 merge commit, SHA 보존 | strict ancestor | `git pull --ff-only origin main` |
| **Squash** | 1 신규 commit, SHA 다름 | *divergence* (로컬에 squash 안된 PR commits 잔존) | `git fetch origin && git reset --hard origin/main` |
| **Rebase** | PR commits 가 새 SHA 로 cherry-pick | *divergence* | `git fetch origin && git reset --hard origin/main` |

FF 시도가 실패하면 머지 방식이 squash/rebase 라는 신호 — `reset --hard origin/main` 로 전환. *단독 작업자 + origin 이 진실 출처* 라는 본 repo 의 가정 위에서만 안전.

본 repo 의 현 머지 방식은 **merge commit** — FF 동기화로 충분 (PR #1 검증 완료).

### 이슈 방지 운영 규칙 (divergence 자체를 봉쇄)

1. **로컬 `main` 에 직접 commit 하지 않는다.** 모든 변경은 worktree branch 또는 `push/<topic>-<date>` branch 로. 로컬 main 의 HEAD 는 *오직* PR 머지 결과 (또는 그 머지의 ancestor) 여야 한다 — 이 invariant 가 깨지지 않으면 divergence 가 발생할 경로 자체가 없다.
2. **PR 머지 직후 즉시 동기화.** 머지 commit 위에 새 작업이 쌓이기 전 `git pull --ff-only` (또는 reset) + 로컬·원격 push branch 삭제로 마무리. 미루면 다음 사이클의 작업 분기점이 stale 해진다.
3. **GitHub repo setting `Automatically delete head branches` ON 권장.** OFF 시 매 PR 머지 후 `git push origin --delete <branch>` 수동 필요 — 본 repo 는 현재 OFF.
4. **머지 방식 단일 통일.** 본 repo 는 merge commit 사용. 향후 squash 로 바꿀 경우 위 동기화 처치도 reset 방식으로 일괄 전환 — 혼용 금지 (혼용은 사용자 인지 부담 증가 + 잘못된 명령 선택 위험).

---

## 5gc-design ↔ 5gc-dev 책임 경계

| 결정 | 어느 시스템 |
|---|---|
| OpenAPI path·schema, error matrix | design |
| service scenario sequence, cross-NF spec | design |
| configuration key·default, data model | design |
| OS·언어·DBMS·배포 (bare/docker/VM) | dev |
| HTTP 라이브러리·threading model | dev |
| 테스트 코드·빌드 시스템 (CMake 등) | dev |

한 줄 원칙 — *spec 이 글자로 박혀있으면 design, 사용자가 고르면 dev.*

---

## Repository Structure

```
5gc-impl-kb/
├── CLAUDE.md                    # 본 파일 — 정책
├── README.md                    # 프로젝트 소개·setup·browsing
├── requirements.txt             # python 의존성
├── index.md                     # 페이지 카탈로그
├── specs/                       # 3GPP 원본 (.pdf / .doc / .docx / .yaml, cp only — symlink 금지)
│   └── {spec-number-with-dot}/
│       └── _extracted/          # spec-split.py 산출 — § 단위 .md (캐시, commit)
├── design/                      # 5gc-design 영역 — NF design deliverable
│   ├── {nf}/                    # NF 단위 폴더 (nssf, nrf, amf, smf, ...)
│   │   ├── 3gpp-{ts|tr}-{n}.md
│   │   ├── _manifest.yaml       # /nf-init 산출 — 의존성 + ready_for_build (gitignored)
│   │   ├── _handoff_seed.yaml   # /nf-build 가 함께 편집, build-handoff.py 입력 (v2 NF 만)
│   │   └── _status.yaml         # /nf-status 산출 — acceptance gate (gitignored)
│   ├── architecture/, interfaces/, security/, slicing/, concepts/, overviews/, other/
│   └── scripts/                 # 자동화 도구
│       ├── extract.py           # .pdf/.doc/.docx → text
│       ├── spec-split.py        # docx → specs/<spec>/_extracted/ 안 § 단위 .md
│       ├── nf-manifest.py       # NF 의존성 자동 검출 → _manifest.yaml
│       ├── resolve-yaml-refs.py # OpenAPI yaml $ref chain 추적
│       ├── build-handoff.py     # 7 카테고리 markdown → handoff-v1 yaml
│       └── nf-status.py         # _status.yaml 산출
├── handoff/                     # design ↔ dev contract (git tracked, 1급 산출)
│   └── {nf}/_handoff.yaml       # 7 카테고리 1:1 미러 self-contained yaml
├── dev/                         # 5gc-dev 영역 (placeholder)
├── docs/                        # plan, handover, setup
├── .claude/
│   └── skills/
│       ├── nf-init/SKILL.md     # /nf-init
│       ├── nf-build/SKILL.md    # /nf-build
│       ├── nf-status/SKILL.md   # /nf-status
│       └── nf-reset/SKILL.md    # /nf-reset (백업 + 재빌드 준비)
└── .venv/                       # gitignored
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

### Normalized stem (`design/`)

```
{stem} = 3gpp-{ts|tr}-{number-no-dot}                   # design 시리즈 페이지
```

예시 — `design/nssf/3gpp-ts-29531.md`.

---

## 매핑 — 2-tier (Karpathy 1:1 에서의 의도적 분기)

| 계층 | 단위 | 명명 |
|---|---|---|
| `specs/` | spec **버전·포맷마다** 1파일 (멀티 포맷·멀티 릴리즈 공존) | 3GPP 원본 파일명 |
| `design/{nf}/` | spec **시리즈마다** 1개 (canonical) | `3gpp-ts-{n}.md`. 본문 `## Version History` 에 변경점 |

이유. 3GPP 시리즈는 같은 NF 의 여러 release 가 공존한다. design 페이지를 시리즈 단위로 두면 사용자가 "TS 29.503 가 뭐냐" 라는 자연스러운 질문에 한 페이지로 답할 수 있고, Cross-NF 표·Data Model chain 의 cross-spec 추적이 자연스럽다.

---

## Categories

```
design/
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

## NF Profile

매니페스트 `profile` 필드. 검사·빌드 동작이 profile 별로 다르다.

| profile | 정의 | 적용 NF 예 |
|---|---|---|
| `stage_3_only` | yaml 동봉 stage 3 spec primary | NSSF (29.531), NRF (29.510), UDM (29.503), AUSF (29.509) |
| `stage_2_only` | prose stage 2 spec primary, yaml 부재 | (드뭄) |
| `mixed` | stage 2 + stage 3 둘 다 primary | NWDAF (23.288 + 29.520), AMF (23.502 + 29.518) |
| `meta_only` | spec 자체가 cross-cutting (구현 NF 가 아님) | TS 23.501 본 spec 페이지 |

NF profile 별 적용 check 는 `design/scripts/nf-status.py` 의 `applies_to` 가 진실 출처.

---

## Acceptance Gates (무가중 framework)

각 페이지의 완성도는 4 단계 gate 로 평가된다. Gate 는 *check id 의 AND* — 가중치 없음.

| Gate | 의미 | 통과 조건 |
|---|---|---|
| `draft` | 페이지 골격 형성 | frontmatter_valid |
| `review_ready` | 사람이 검토 가능한 상태 | + sections_complete + manifest_ready |
| `handoff_ready` | dev (agent or human) 가 `_handoff.yaml` 만으로 NF 빌드 시작 가능 | + Tier 2 모두 PASS. v2 NF 는 `validate_extraction_basic` 가 진실 출처 |
| `canonical` | 해당 spec 버전의 design 정본 | + schema_implementable + implementation_guidance_quality ≥ 4 |

**Topic-level status enum (handoff-v2 NF 부터).** 카테고리·토픽 status 는 다음 5종 — `canonical` / `handoff_ready` / `draft` / `blocked` / `not_applicable`. spec `2026-05-12-nf-build-restructure-agent-consumable-mvp.md` §1 의 행동 매핑이 진실 출처.

**status_precedence: topic_over_category.** category status 와 topic status 가 다를 때 토픽 status 가 우선. category=draft + topic=handoff_ready (scope 명시) 는 MVP 한정 의도이며 정상. 역방향 (category=handoff_ready + topic=draft) 만 validator basic #5 가 FAIL.

`_status.yaml` 의 모든 FAIL check 는 `to_pass` 에 *다음 액션* 이 적혀있다 — silent FAIL 없음.

---

## Source Document Management Rules

- **항상 cp, 절대 symlink 금지.** 외부 경로(`~/Downloads/` 등)에서 `specs/` 안으로 복사한다.
- `source_path` 는 *repo-relative* 경로 (`specs/{spec}/{file}`). 프로젝트 디렉터리명에 의존하지 않는다.
- `source_filename` 은 `basename(source_path)` 와 정확히 일치.
- `source_format` 은 확장자와 일치 (`pdf` | `doc` | `docx`).
- 같은 spec 시리즈의 다른 버전·포맷은 **같은 spec-number 폴더** 에 공존.

---

## Knowledge depth, not breadth

본 KB 의 가치는 *NF 한 개당 깊이* 에서 나온다. 카테고리별 placeholder 로 페이지 수를 늘리는 것보다 한 NF 의 7 카테고리를 모두 handoff_ready 로 끌어올리는 것이 우선. 그렇게 끌어올린 NF 의 `design/overviews/` 합성 (예 cross-NF 호출 그래프) 이 가장 가치 있는 산출.

---

## When in doubt, follow rule #1.
