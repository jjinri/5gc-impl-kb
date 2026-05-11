---
name: nf-reset
description: 본 5gc-impl-kb 의 특정 NF 페이지를 *백업 후 처음부터 다시 빌드* 하기 위해 현 산출을 archive 폴더로 옮기고 design/<nf>/ 를 비워주는 워크플로우. 사용자가 "/nf-reset nssf", "NSSF 백업하고 다시 빌드", "NRF 페이지 리셋", "기존 wiki 백업하고 새로 시작", "reset nf page", "NSSF 페이지 다시 만들어" 등을 말하거나 NF 이름을 지정하면 무조건 이 skill 을 사용한다. 동작 — design/<nf>/ 의 페이지·_status.yaml 을 design/<nf>/_archive/<timestamp>/ 로 mv 하고 root index.md 의 NF 섹션을 placeholder (`_(아직 페이지 없음)_`) 로 되돌린다. 기본 모드는 _manifest.yaml 보존 (재빌드 시 의존성 재발견 비용 절감), `--full` 옵션이면 매니페스트도 함께 archive. archive 후 사용자는 즉시 `/nf-build <nf>` 또는 `/nf-init <nf> --primary <spec>` 으로 fresh 시작 가능. 본 skill 은 *파괴적 행동* 이므로 실행 전 archive 위치를 사용자에게 알리고 확인을 묻는다 — 사일런트 wipe 금지.
argument-hint: "<nf> [--full]"
allowed-tools: Bash(mkdir -p *) Bash(mv *) Bash(ls *) Bash(git mv *)
---

# nf-reset — NF 페이지 백업 + 재빌드 준비

## 입력
- `<nf>` — NF 이름 (소문자, design/ 하위 폴더명).
- `--full` — `_manifest.yaml` 도 archive 폴더로 함께 mv. 매니페스트도 처음부터 다시 만들고 싶을 때.
- 인자 없으면 어느 NF 인지 묻고 정지.

## 본 skill 의 책임 범위

본 skill 은 *파괴적* — 산출 파일을 archive 로 *옮긴다*. 옮기기 전 사용자에게 확인을 받고, archive 위치를 명시한다. 새 페이지 빌드는 *하지 않는다* — `/nf-build` 의 책임.

| 시나리오 | 사용 skill |
|---|---|
| 페이지를 처음부터 다시 빌드 (manifest 는 살림) | `/nf-reset <nf>` → `/nf-build <nf>` |
| spec 시리즈 자체를 다른 것으로 바꿔 새로 시작 | `/nf-reset <nf> --full` → `/nf-init <nf> --primary <new-spec>` → `/nf-build <nf>` |
| 단순히 부분 카테고리만 재빌드 (리셋 불필요) | `/nf-build <nf> --<category>` (직접) |

## 동작 원칙 (이유 포함)

- **CLAUDE.md THE FOUR RULES 가 우선.**
- **archive 는 timestamp 폴더로 분리** (`design/<nf>/_archive/<YYYYMMDD-HHMMSS>/`). 같은 NF 를 여러 번 reset 해도 이전 archive 가 덮어쓰이지 않게 — 사용자가 *언제든 어떤 시점으로든* 되돌아갈 수 있어야 한다.
- **실행 전 사용자 확인 필수.** 본 skill 은 *파괴적* (archive 폴더로 mv 는 사실상 wipe). 사일런트 실행은 사용자가 의도하지 않은 시점에 작업물을 잃을 위험. archive 위치 + 옮길 파일 표를 미리 보여주고 명시 동의 받는다.
- **`--full` 없이는 `_manifest.yaml` 보존.** 이유 — 매니페스트 자동 검출은 비용이 있는 작업 (docx 추출 + ref 분석). 페이지만 다시 짓고 싶을 때 매니페스트까지 archive 하면 `/nf-init` 부터 다시 해야 해 작업 흐름이 늘어진다. 두 의도 (페이지만 vs 처음부터) 를 명시적 옵션으로 분리.
- **`index.md` 의 NF 섹션도 placeholder 로 되돌린다.** 페이지를 archive 로 옮기면 `index.md` 의 `[[<nf>/<stem>]]` wikilink 가 깨진다. 다음 reset → build 사이의 과도기에 *깨진 link 를 남겨두지 않기 위해* placeholder (`_(아직 페이지 없음)_`) 로 즉시 되돌린다. `/nf-build` 가 새 페이지를 만들 때 같은 자리를 다시 갱신한다.
- **archive 후 페이지 빌드는 별도 호출.** 본 skill 은 백업·정리만 한다 — `/nf-build` 가 빌드의 진실 출처. 한 skill 이 mv + build 둘 다 하면 *어느 시점에 무엇이 망가졌는지* 추적이 어려워진다.

## Workflow

### 1. 입력 검증
- `<nf>` 가 비어있거나 `design/<nf>/` 가 부재하면 정지.
- 옮길 파일 목록 수집 — `design/<nf>/3gpp-*.md`, `design/<nf>/_status.yaml`, `handoff/<nf>/_handoff.yaml` (존재 시). `--full` 면 `_manifest.yaml` 추가.
- `index.md` 의 해당 NF 섹션이 wikilink 항목을 가지고 있는지 확인 — 있으면 §3 에서 placeholder 로 되돌릴 대상.
- 옮길 파일이 0개이고 index 항목도 없으면 "이미 비어있음" 보고 후 정지 (archive 폴더 만들지 않음).

### 2. 사용자 확인
- archive 경로 — `design/<nf>/_archive/<YYYYMMDD-HHMMSS>/`.
- 옮길 파일 표 (path · 크기) + index.md 의 갱신 예고 (어떤 라인이 placeholder 로 바뀌는지) 를 사용자에게 보여주고 "[Y/n] 진행할까요?" 묻는다.
- N 이면 정지.

### 3. archive 실행
- timestamp = `date '+%Y%m%d-%H%M%S'`.
- `mkdir -p design/<nf>/_archive/<timestamp>/`.
- 옮길 파일들을 `git mv` (가능하면) 로 이동.
- `index.md` 의 NF 섹션 정리 — 해당 섹션의 한 줄 항목 (`- [[<nf>/...]] — ...`) 들을 모두 제거하고 그 자리에 `_(아직 페이지 없음)_` 한 줄로 교체. 섹션 헤더 (`## <NF> — ...`) 와 그 아래 인용문 (`>`) 은 유지.
- 결과 표시 — archive 위치, mv 된 파일 수, index.md 갱신 여부.

### 4. 결과 보고
- archive 위치.
- 남은 design/<nf>/ 파일 (보존된 것). `--full` 이 아니면 `_manifest.yaml` 만 남음.
- `handoff/<nf>/_handoff.yaml` 은 `--full` 여부와 무관하게 항상 archive (`design/` 페이지와 함께 갱신되므로 오래된 yaml 잔존은 잘못된 계약이 됨).
- index.md 의 NF 섹션이 placeholder 로 되돌려졌음을 명시.
- 다음 액션 추천.
  - `_manifest.yaml` 살림 + manifest 가 ready → `/nf-build <nf>` 로 즉시 재빌드.
  - `_manifest.yaml` 살림 + manifest 가 not ready → 누락 spec 처리 후 `/nf-init <nf> --primary <spec>` 으로 보강.
  - `--full` 사용 → `/nf-init <nf> --primary <spec>` 부터.

**커밋은 사용자 검토 후 별도** — 본 skill 은 자동 커밋 안 한다. archive 는 git 입장에서 *경로 변경* 으로 보이며, 사용자가 의미 단위로 묶어 커밋.

## 예시

**호출 — manifest 살리고 페이지만 백업.**

```
사용자: /nf-reset nssf
도구:
  archive 위치 — design/nssf/_archive/20260509-220000/
  옮길 파일 —
    design/nssf/3gpp-ts-29531.md (38 KB)
    design/nssf/_status.yaml (7 KB)
    handoff/nssf/_handoff.yaml (존재 시)
  보존 — design/nssf/_manifest.yaml
  index.md NSSF 섹션 — [[nssf/3gpp-ts-29531]] 항목 제거 후 placeholder 로 되돌림.
  진행할까요? [Y/n]:
사용자: Y
도구:   git mv ... + index.md 갱신
보고:   "archive 완료. index NSSF 섹션 placeholder 복원.
        manifest 보존 (ready=true 가정) → `/nf-build nssf` 로 fresh 빌드."
```

**호출 — manifest 까지 처음부터.**

```
사용자: /nf-reset nssf --full
도구:
  archive 위치 — design/nssf/_archive/20260509-220500/
  옮길 파일 —
    design/nssf/3gpp-ts-29531.md, _status.yaml, _manifest.yaml
  보존 — (없음)
  index.md NSSF 섹션 — placeholder 복원.
  진행할까요? [Y/n]: Y
도구:   git mv ... + index.md 갱신
보고:   "archive 완료. 다음 — `/nf-init nssf --primary <spec>` 부터 시작."
```

## 자주 틀리는 지점

- timestamp 가 같은 초에 두 번 reset 호출되면 archive 충돌. 본 skill 은 timestamp 충돌 시 사용자에게 1초 기다리거나 다른 timestamp 로 재시도.
- archive 폴더 안의 파일은 *git history 에 이미 있는* 사본 — 굳이 archive 가 git 에 새로 들어갈 필요는 없지만 archive 를 git 에 두는 것 자체는 안전. `.gitignore` 갱신은 사용자 결정.
- `--full` 없이 reset 후 `/nf-init` 을 부르면 manifest 가 보존되어 의존성 재발견을 건너뛰지만, 사용자가 그 사이 specs/ 를 변경했으면 manifest 가 stale. 그 경우 `/nf-init <nf> --primary <spec>` 한 번 더 호출해 manifest 를 갱신 권장.
- index.md 정리를 누락하면 다음 빌드 직전까지 wikilink 가 깨진 채로 남는다. `/nf-status` 의 `wikilinks_resolve` check 가 detect 하지만 reset 직후라 status 호출 자체가 의미 없음 — 따라서 reset *시점에* 정리하는 게 책임의 진실 위치.

## 참고 — 본 skill 안에 다시 적지 말 것

- 매니페스트 생성: `/nf-init` SKILL.md.
- 페이지 빌드: `/nf-build` SKILL.md.
- 디렉터리·파일명 규칙: CLAUDE.md.
