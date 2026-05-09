---
name: nf-reset
description: 본 5gc-impl-kb 의 특정 NF 페이지를 *백업 후 처음부터 다시 빌드* 하기 위해 현 산출을 archive 폴더로 옮기고 kb/<nf>/ 를 비워주는 워크플로우. 사용자가 "/nf-reset nssf", "NSSF 백업하고 다시 빌드", "NRF 페이지 리셋", "기존 wiki 백업하고 새로 시작", "reset nf page", "NSSF 페이지 다시 만들어" 등을 말하거나 NF 이름을 지정하면 무조건 이 skill 을 사용한다. 동작 — kb/<nf>/ 의 페이지·_status.yaml·_diagrams/ 를 kb/<nf>/_archive/<timestamp>/ 로 mv 하고, 기본 모드는 _manifest.yaml 보존 (재빌드 시 의존성 재발견 비용 절감), `--full` 옵션이면 매니페스트도 함께 archive. archive 후 사용자는 즉시 `/nf-build <nf>` 또는 `/nf-init <nf> --primary <spec>` 으로 fresh 시작 가능. 본 skill 은 *파괴적 행동* 이므로 실행 전 archive 위치를 사용자에게 알리고 확인을 묻는다 — 사일런트 wipe 금지.
argument-hint: "<nf> [--full]"
allowed-tools: Bash(mkdir -p *) Bash(mv *) Bash(ls *)
---

# nf-reset — NF 페이지 백업 + 재빌드 준비

## 입력
- `<nf>` — NF 이름 (소문자, kb/ 하위 폴더명).
- `--full` — `_manifest.yaml` 도 archive 폴더로 함께 mv. 매니페스트도 처음부터 다시 만들고 싶을 때.
- 인자 없으면 어느 NF 인지 묻고 정지.

## 본 skill 의 책임 범위

본 skill 은 *파괴적* — 산출 파일을 archive 로 *옮긴다*. 옮기기 전 사용자에게 확인을 받고, archive 위치를 명시한다. 새 페이지 빌드는 *하지 않는다* — `/nf-build` 의 책임.

| 시나리오 | 사용 skill |
|---|---|
| 페이지를 처음부터 다시 빌드 (manifest 는 살림) | `/nf-reset <nf>` → `/nf-build <nf>` |
| spec 시리즈 자체를 다른 것으로 바꿔 새로 시작 | `/nf-reset <nf> --full` → `/nf-init <nf> --primary <new-spec>` → `/nf-build <nf>` |
| 단순히 부분 카테고리만 재빌드 (리셋 불필요) | `/nf-build <nf> --<category>` (직접) |

## 절대 규칙
1. CLAUDE.md THE FOUR RULES 를 따른다.
2. archive 는 `kb/<nf>/_archive/<YYYYMMDD-HHMMSS>/` 로 timestamp 분리. 같은 NF 를 여러 번 reset 해도 이전 archive 가 덮어쓰이지 않는다.
3. 실행 전 archive 위치 + 옮길 파일 목록을 사용자에게 알리고 확인을 받는다 (사용자 명시 동의 시에만 실행).
4. `--full` 없이는 `_manifest.yaml` 을 보존. 사용자 의도 (manifest 살림 vs 함께 archive) 를 명확히 분리.
5. 페이지 본 빌드는 *별도 호출* — 본 skill 은 archive 만 한다.

## Workflow

### 1. 입력 검증
- `<nf>` 가 비어있거나 `kb/<nf>/` 가 부재하면 정지.
- 옮길 파일 목록 수집 — `kb/<nf>/3gpp-*.md`, `kb/<nf>/_status.yaml`, `kb/<nf>/_diagrams/`. `--full` 면 `_manifest.yaml` 추가.
- 옮길 파일이 0개면 "이미 비어있음" 보고 후 정지 (archive 폴더 만들지 않음).

### 2. 사용자 확인
- archive 경로 — `kb/<nf>/_archive/<YYYYMMDD-HHMMSS>/`.
- 옮길 파일 표 (path · 크기) 를 사용자에게 보여주고 "[Y/n] 진행할까요?" 묻는다.
- N 이면 정지.

### 3. archive 실행
- timestamp = `date '+%Y%m%d-%H%M%S'`.
- `mkdir -p kb/<nf>/_archive/<timestamp>/`.
- 옮길 파일들을 `git mv` (가능하면) 로 이동. _diagrams/ 폴더는 통째로 이동.
- 결과 표시 — archive 위치, mv 된 파일 수.

### 4. 결과 보고
- archive 위치.
- 남은 kb/<nf>/ 파일 (보존된 것). `--full` 이 아니면 `_manifest.yaml` 만 남음.
- 다음 액션 추천.
  - `_manifest.yaml` 살림 → `/nf-build <nf>` 로 즉시 재빌드.
  - `--full` 사용 → `/nf-init <nf> --primary <spec>` 부터.

**커밋 자동 수행 금지.** archive 는 git 입장에서 *경로 변경* 으로 보이며, 사용자가 검토 후 별도 커밋.

## 자주 틀리는 지점

- timestamp 가 같은 초에 두 번 reset 호출되면 archive 충돌. 본 skill 은 timestamp 충돌 시 사용자에게 1초 기다리거나 다른 timestamp 로 재시도.
- archive 폴더 안의 파일은 *git history 에 이미 있는* 사본 — 굳이 archive 가 git 에 새로 들어갈 필요는 없지만 archive 를 git 에 두는 것 자체는 안전. `.gitignore` 갱신은 사용자 결정.
- `--full` 없이 reset 후 `/nf-init` 을 부르면 manifest 가 보존되어 의존성 재발견을 건너뛰지만, 사용자가 그 사이 specs/ 를 변경했으면 manifest 가 stale. 그 경우 `/nf-init <nf> --primary <spec>` 한 번 더 호출해 manifest 를 갱신 권장.

## 참고 — 본 skill 안에 다시 적지 말 것

- 매니페스트 생성: `/nf-init` SKILL.md.
- 페이지 빌드: `/nf-build` SKILL.md.
- 디렉터리·파일명 규칙: CLAUDE.md.
