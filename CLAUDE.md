# 5gc-impl-kb — 정책

본 KB 의 *정책 only* 문서.

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

## Lifecycle vocabulary policy

NF 개발 단계 이름은 [`docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md`](./docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md) 를 따른다.

- `/nf-init` 은 현재 spec discovery + seed auto-gen 호환 skill 이며 canonical wrapper 는 `/nf-spec-discover` 다. 핵심 목적은 사람이 legacy handoff yaml 을 수동 작성하지 않도록 `_contract_seed.yaml` 을 자동 생성하는 것이다. reset 은 별도 skill 이 아니라 `--reset` 옵션이고 contract 산출물만 archive 한다.
- `/nf-build` 는 code build 가 아니라 contract extraction/generation 단계다. canonical wrapper 는 `/nf-contract-build` 다.
- `/nf-status` 는 contract validation 호환 skill 이며 canonical wrapper 는 `/nf-contract-check` 다. 상세 아키텍처나 구현 검증 status 와 혼동하지 않는다.
- `/nf-arch-design` 은 handoff-ready contract 를 상세 아키텍처 문서로 변환하는 canonical skill 이다. implementation planning 을 자동 호출하지 않는다.
- `/nf-impl-plan` 은 architecture 문서를 구현 작업·테스트·traceability 계획으로 변환하는 canonical skill 이다. 소스 코드나 build system 을 만들지 않는다.
- user-facing lifecycle skill 이 다음 user-facing lifecycle skill 을 자동 호출하지 않는다. 같은 단계의 필수 script/check 만 내부 실행하고, 다음 단계는 추천으로 보고한다.
- `handoff_ready` 는 상세 아키텍처 설계를 시작할 수 있는 contract 입력이 충분하다는 뜻이다. 상세 아키텍처와 구현 계획은 별도 단계다.

## Source-of-truth policy

- Spec 원문은 `specs/` 가 보존한다.
- Spec-derived contract 는 현재 `design/<nf>/contract/` 와 `handoff/<nf>/contract.yaml` 이 보존한다. Legacy `handoff/<nf>/_handoff.yaml` 는 폐기됐고 새 workflow 의 입력이 아니다.
- Architecture design 은 목표 구조상 `design/<nf>/architecture/` 에 둔다.
- Implementation planning 은 `dev/<nf>/` 에 둔다. 이 영역은 계획 산출물 위치이며 소스 코드 작성 시작 신호가 아니다.
- `.omx/` 는 runtime/state 영역이며 cleanup·구조 변경 대상이 아니다.

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

## File Naming Convention

### Original (`specs/`) — 3GPP 원본 보존

```
specs/{spec-number-with-dot}/{original-3gpp-filename}.{ext}
```

- spec-number 폴더는 점 포함 정식 표기 (`29.531`, `23.501`, `29.503`).
- 파일명은 3GPP 다운로드 원본 그대로 (`29531-i40.docx`). release/version 인코딩 (`-{letter}{N}{M}`) 이 이미 들어있어 재명명 불필요.
- 같이 들어있는 OpenAPI yaml (예 `TS29531_Nnssf_NSSelection.yaml`) 도 같은 폴더.

---

## When in doubt, follow rule #1.
