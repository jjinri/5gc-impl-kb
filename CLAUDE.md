# 5gc-impl-kb — 정책

본 KB 의 *정책 only* 문서. 프로젝트 소개·setup·browsing 은 [`README.md`](./README.md) 가 진실 출처.

> **Inheritance.** 전역 `~/.claude/CLAUDE.md` 의 모든 행동 규칙 (가정 금지·단순성·외과적 변경·테스트 우선·시맨틱 커밋·에러는 추측 말고 읽기 등) 을 그대로 따른다. 이 파일은 그 위에 본 프로젝트 고유 규칙만 추가한다.

> **Language policy.**
> - **design 본문(prose) 은 한국어**로 작성한다.
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
| 1. 매니페스트 보강·재생성 | `/nf-init <nf> --primary <spec> [--reset]` | specs/ 의존성 자동 검출. `ready_for_build` 까지 반복 호출. `--reset` 은 기존 산출을 `design/<nf>/_archive/<ts>/` 로 mv 후 manifest 재생성 (백업·재시작 통합) |
| 2. 페이지 빌드 | `/nf-build <nf> [--<category>]` | 7 카테고리 페이지 생성·갱신 + `handoff/<nf>/_handoff.yaml` 자동 갱신 |
| 3. 완성도 검사 | `/nf-status <nf>` | acceptance gate 평가, FAIL 마다 `to_pass` 액션 보고 |

각 SKILL 의 절차 *세부* 는 해당 `SKILL.md` 가 진실의 출처. CLAUDE.md 는 *정책* 만 정의하고 절차는 SKILL 로 위임한다. onboarding (Quick start) 은 README 에.

> 새 SKILL 작성·개선 — Anthropic Skill Creator 가이드 (<https://claude.ai/customize/skills>) 또는 설치된 plugin `/skill-creator:skill-creator` 참고. 본 프로젝트의 3 SKILL (nf-init/build/status) 이 *원칙 + 이유 + 예시* 패턴의 모범 — 새 SKILL 도 같은 골격을 따른다.

---

## 원격 동기화 — PR 분기

원칙. **PR 은 검토·기록·통합 리스크를 줄일 때만 만든다.** 단순히 작업 단위마다 PR 을 만들면 review/CI 비용·merge 대기·컨텍스트 스위칭만 커진다.

### PR 필요 (OR — 하나라도 해당 시)

| 범주 | 트리거 |
|---|---|
| **main 보호** | production/runtime 코드, public API·schema·contract, migration, 보안·권한·인증, 배포·CI/CD |
| **리뷰 가치** | 설계 판단, 다중 파일/모듈 영향, 회귀 위험 버그 fix, 테스트 전략 검토, 의도·구현 분리 확인 |
| **협업·기록** | 인수인계 가능성, 의사결정 이력, 큰 기능 milestone, 외부 리뷰어 승인, issue/spec/plan 연결 |
| **자동화 게이트** | CI 전체 실행, branch protection required checks, merge queue, release note 검증 |

본 repo 의 대표 트리거 — `design/scripts/*.py` 행위 변경, handoff schema (build-handoff/validate-extraction 룰/seed schema), `.claude/skills/*/SKILL.md`, `specs/` 추가·신규 NF 폴더, 한 push 에 commits ≥ 3.

### PR 불필요 — direct push 허용

| 범주 | 예 |
|---|---|
| **로컬·개인 산출물** | 임시 스크립트, 실험 로그, 개인 agent skill 조정, `.omx`·local scratch, 공유 수준 아닌 WIP |
| **매우 작은 비기능 변경** | typo, prose 문구·서식·정책 텍스트 정리, 도구 산출 재emit (도구 미변경). semantic 변경 없음 |

판단 모호 → PR. 본 분기는 *단독 작업자 + origin 진실 출처* 가정 위에서만 유효 — 협업자 합류 시 모든 변경 PR 격상.

### 절차

| 케이스 | 명령 |
|---|---|
| PR 필요 | `git switch -c push/<topic>-<yyyymmdd>` → push → `gh pr create --base main` → 머지 → 동기화 → push branch 삭제 |
| Direct push | `git switch main && git commit && git push origin main` 한 사이클로 마무리 (미push 누적 금지) |

PR 본문은 *요약 + 이유 + 검증* — 머지 commit 메시지 또는 retro 그대로 reuse.

### 머지 후 동기화 — 머지 방식별

| 머지 방식 | 처치 |
|---|---|
| Merge commit (본 repo) | `git pull --ff-only origin main` |
| Squash / Rebase | `git fetch origin && git reset --hard origin/main` |

FF 실패 → squash/rebase 신호 → reset 전환. 본 repo 는 **merge commit** 통일 — 혼용 금지. GitHub `Automatically delete head branches` ON 권장 (현재 OFF, 머지 후 수동 `git push origin --delete <branch>`).

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

## Knowledge depth, not breadth

본 KB 의 가치는 *NF 한 개당 깊이* 에서 나온다. 카테고리별 placeholder 로 페이지 수를 늘리는 것보다 한 NF 의 7 카테고리를 모두 handoff_ready 로 끌어올리는 것이 우선. 그렇게 끌어올린 NF 의 `design/overviews/` 합성 (예 cross-NF 호출 그래프) 이 가장 가치 있는 산출.

---

## When in doubt, follow rule #1.
