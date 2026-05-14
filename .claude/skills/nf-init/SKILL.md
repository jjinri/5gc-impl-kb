---
name: nf-init
description: 본 5gc-impl-kb 의 새 NF (Network Function) 작업을 *시작* 하거나 기존 NF 산출을 *백업 후 처음부터 다시 시작* 하는 워크플로우. 사용자가 "NSSF 시작", "/nf-init nrf", "AMF 매니페스트 만들어", "amf 구현 시작", "create nf manifest", "29.510 으로 NRF 작업 시작", "NSSF 리셋", "NRF 페이지 백업하고 다시 빌드", "/nf-init nssf --primary 29.531 --reset" 등을 말하거나 NF 이름 + 주 spec 번호를 지정하면 무조건 이 skill 을 사용한다. 동작 — (1) `--reset` 인 경우 기존 산출 (`design/<nf>/` 안의 페이지·`_status.yaml`·`_handoff_seed.yaml`·`_manifest.yaml` + `handoff/<nf>/contract.yaml`) 을 `design/<nf>/_archive/<timestamp>/` 로 mv (2) `design/scripts/nf-manifest.py <nf> --primary <spec> --write` 호출 (3) 산출 매니페스트 (`design/<nf>/_manifest.yaml`) 의 status 를 사용자에게 보고 (4) `ready_for_build = true` 가 되려면 어느 spec 을 specs/ 에 추가해야 하는지 priority 순으로 알린다. 본 skill 은 *반복 가능* 하다 — 사용자가 새 spec 을 specs/ 에 cp 한 뒤 다시 호출하면 매니페스트가 보강된다. ready 가 될 때까지 반복하고, 그 다음 `/nf-build` 로 페이지 생성. 신규 NF 의 매니페스트 *생성/재생성* 만 한다 — 페이지 빌드는 `/nf-build`, 완성도 검사는 `/nf-status` 의 책임.
argument-hint: "<nf> --primary <spec> [--reset]"
allowed-tools: Bash(.venv/bin/python3 design/scripts/nf-manifest.py *) Bash(ls *) Bash(cat *) Bash(mkdir -p *) Bash(git mv *) Bash(mv *) Bash(date *) Bash(find design/* *)
---

# nf-init — NF 매니페스트 생성·보강

> Compatibility alias. Canonical user-facing wrapper is `/nf-spec-discover`. Keep this workflow behavior identical until alias removal is explicitly planned.

## 입력
- `<nf>` — `nssf`, `nrf`, `amf` 등 NF 이름 (소문자, design/ 하위 폴더명).
- `--primary <spec>` — NF 의 주 spec 번호 (점 포함, 예 `29.531`). 필수.
- `--reset` — *파괴적 옵션*. `design/<nf>/` 의 모든 산출 (페이지·`_status.yaml`·`_handoff_seed.yaml`·`_manifest.yaml`) 과 `handoff/<nf>/contract.yaml` 을 `design/<nf>/_archive/<YYYYMMDD-HHMMSS>/` 로 mv 한 뒤 manifest 를 *재생성*. `--primary <spec>` 과 함께 써야 하며, 명시 flag 이므로 추가 [Y/n] 프롬프트 없이 즉시 archive 실행. 옮길 파일 표 + archive 위치는 항상 *결과 보고* 로 출력.
- 인자 없으면 어느 NF 인지·주 spec 이 무엇인지 사용자에게 묻고 정지.

> 다중 primary spec (예 NWDAF = 23.288 + 29.520) 은 sprint 후반에 도구 인자 확장 예정. 현재는 *대표 1개* 로 시작하고 나머지는 매니페스트에서 docx_clause_2 references 로 자동 검출.

## 본 skill 의 책임 범위

본 skill 은 매니페스트 *생성·보강·재생성* 에 한정. *페이지 빌드는 `/nf-build`, 완성도 검사는 `/nf-status`* 의 책임.

| 시나리오 | 사용 skill |
| --- | --- |
| 새 NF 작업 처음 시작 | `/nf-init <nf> --primary <spec>` |
| specs/ 에 새 ref spec cp 후 매니페스트 갱신 | `/nf-init <nf> --primary <spec>` (재실행) |
| 매니페스트가 ready 됐고 페이지 생성 차례 | `/nf-build <nf>` |
| 페이지가 handoff_ready 기준 통과하는지 점검 | `/nf-status <nf>` |
| 기존 NF 페이지를 백업하고 fresh 빌드 | `/nf-init <nf> --primary <spec> --reset` → `/nf-build <nf>` |
| spec 시리즈 자체를 바꿔 새로 시작 | `/nf-init <nf> --primary <new-spec> --reset` → `/nf-build <nf>` |

## 동작 원칙 (이유 포함)

- **CLAUDE.md THE FOUR RULES 가 우선.** 본 skill 의 매니페스트 자동 검출도 그 규칙 안에서 동작 — 외부 web 에서 의존성 가져오지 않고, 추출 텍스트(docx 또는 yaml) 에 적힌 ref 만 카운트.
- **사용자 `manual_overrides` 는 `nf-manifest.py` 가 보존한다.** 이유 — 매니페스트 자동 검출은 *제안* 이고 일부 spec 은 NF 컨텍스트 외 (예 NG-RAN 38.413). 사용자가 exclude 로 결정하면 그 결정을 다음 호출에서 잃으면 안 됨. 단 `--reset` 은 manifest 자체를 archive 하므로 manual_overrides 도 함께 사라진다 — *fresh 시작* 의 명시적 의도.
- **ready_for_build 가 false 면 *무엇을* 추가해야 하는지 알린다.** 그저 "ready=false" 만 보고하면 사용자가 다음 액션을 추측해야 한다. priority 순서로 missing spec 을 명시하면 즉시 다음 행동으로 이어진다.
- **`--reset` 은 archive (mv) — 삭제 아님.** `design/<nf>/_archive/<timestamp>/` 로 옮긴다. timestamp 충돌은 1초 대기 또는 다른 timestamp 로 재시도. archive 폴더 자체는 `.gitignore` 로 untrack — repo 트리 노이즈 방지. git history 보존이 필요하면 archive 전에 `git tag archive/<nf>-<YYYYMMDD>` 사용 권장.
- **archive 후 페이지 빌드는 별도 호출.** 본 skill 은 매니페스트 재생성까지만 — `/nf-build` 가 빌드의 진실 출처. 한 skill 이 archive + manifest + build 셋 다 하면 *어느 단계에서 무엇이 망가졌는지* 추적이 어려워진다.
- **`--reset` 의 destructive mv 는 SKILL 의 bash 가 수행, `nf-manifest.py` 는 manifest 만 만든다.** 단일 책임 — 도구는 자동 검출 로직만 갖고, 파괴적 행위는 SKILL 단계에서 명시 처리.

## Workflow

### 1. 인자 검증
- `<nf>` 가 비어있거나 `--primary` 가 없으면 사용자에게 묻고 정지.
- `<spec>` 의 형식 — `^\d{2}\.\d{3}$` (점 포함). 다른 형식이면 정지.
- `--reset` 이 있으면 `design/<nf>/` 가 존재해야 한다. 부재 시 정지 — "기존 산출이 없습니다. `--reset` 없이 호출하세요".

### 2. specs/ 폴더 실재 확인
- `specs/<spec>/` 디렉터리 존재 + .docx/.pdf/.doc 1개 이상 보유 확인.
- 없으면 정지 — "먼저 `specs/<spec>/` 에 원본 cp 후 재호출".

### 3. (선택) `--reset` archive
`--reset` 인 경우에만 실행. 미지정 시 본 단계 건너뛰고 §4 로.

- `ts=$(date '+%Y%m%d-%H%M%S')`.
- `archive_dir=design/<nf>/_archive/<ts>/`.
- `mkdir -p <archive_dir>`.
- **mv 대상 (블랙리스트 패턴)** — `design/<nf>/` 안의 모든 파일·디렉터리 중 `_archive` 만 보존, 나머지 모두 mv. 즉 `_manifest.yaml`·`_handoff_seed.yaml`·`_status.yaml`·페이지 (`*.md`)·토픽 디렉터리 (`contract/`, `module-decomposition/` 등) 모두 archive 로 이동. 명시 화이트리스트 (예 `3gpp-*.md` 만) 는 handoff-v2 topic layout 에서 drift 위험 — 블랙리스트가 안전.
  ```bash
  find design/<nf>/ -mindepth 1 -maxdepth 1 ! -name _archive -exec git mv -k {} <archive_dir>/ \;
  ```
  (`git mv -k` 실패 시 `mv` 로 fallback. archive 폴더는 `.gitignore` 대상이라 git 추적 밖.)
- **handoff yaml** — `handoff/<nf>/contract.yaml` 존재 시 같은 archive 폴더 안 `contract.yaml` 로 mv. 설계와 분리된 별도 트리이지만 페이지와 한 사이클로 재생성되므로 함께 archive.
- 옮길 파일이 0 개이면 archive 폴더 생성하지 말고 "이미 비어있음" 보고 후 §4 로 진행.

### 4. 도구 실행
```bash
.venv/bin/python3 design/scripts/nf-manifest.py <nf> --primary <spec> --write
```
- `--write` 로 `design/<nf>/_manifest.yaml` 저장.
- 도구 stderr 의 status 라인을 사용자에게 그대로 전달.
- `--reset` 이었으면 이전 `_manifest.yaml` 이 archive 로 갔으므로 manual_overrides 빈 상태로 재생성된다. 의도된 동작.

### 5. 결과 보고
- (`--reset` 시) archive 경로 + mv 된 파일 수.
- 매니페스트 위치 — `design/<nf>/_manifest.yaml`.
- `manifest_completeness` 비율 (예 `5/11`).
- `ready_for_build` (true/false).
- false 면 `missing_priority` 리스트 + 각각의 의미 (sba_common / data_types / security 등).
- *다음 추천 액션* — (a) priority 순으로 specs/ 에 cp 후 재호출 (b) 또는 manual_overrides.exclude 에 등록 후 재호출 (c) ready 면 `/nf-build <nf>`.

### 6. 마무리
- 사용자가 ready 시키면 `/nf-build <nf>` 가 다음 단계. 본 skill 은 거기로 *자동 위임하지 않는다* — ready_for_build 결과를 사용자가 확인 후 별도 호출.
- 매니페스트가 이미 ready 였으면 — "이미 빌드 가능. `/nf-build <nf>` 로 진행" 안내.
- 커밋은 사용자 검토 후 별도 — 본 skill 은 자동 커밋 안 한다.

## 예시

**호출 — 첫 NSSF 시작.**

```
사용자: /nf-init nssf --primary 29.531
도구:   design/scripts/nf-manifest.py nssf --primary 29.531 --write
산출:   design/nssf/_manifest.yaml (12 의존 spec 검출, 5/11 in-scope present, ready=false)
보고:   "manifest_completeness 5/11. ready_for_build=false.
        추가 필요 priority — [29.500 SBA HTTP/2, 29.501 service 정의, 33.501 보안,
                            29.503 chain leaf RecurTime, 23.003 NF instance ID].
        specs/<spec>/ 에 cp 또는 manual_overrides.exclude 등록 후 재호출."
```

**호출 — specs/29.500 cp 후 재실행.**

```
사용자: /nf-init nssf --primary 29.531
도구:   재실행 (manual_overrides 보존)
보고:   "manifest_completeness 6/11 (29.500 추가). ready=false. 남은 4건 ..."
```

→ ready 가 될 때까지 specs/ cp + 재호출. ready=true 면 `/nf-build nssf`.

**호출 — 기존 NSSF 산출을 archive 후 fresh 시작.**

```
사용자: /nf-init nssf --primary 29.531 --reset
도구:   ts=20260513-153000; mkdir -p design/nssf/_archive/20260513-153000/
        find design/nssf/ -mindepth 1 -maxdepth 1 ! -name _archive -exec git mv -k {} ... \;
        mv handoff/nssf/contract.yaml design/nssf/_archive/20260513-153000/contract.yaml
        design/scripts/nf-manifest.py nssf --primary 29.531 --write
보고:   "archive — design/nssf/_archive/20260513-153000/ (mv entries: contract/, module-decomposition/, _manifest.yaml, _handoff_seed.yaml, _status.yaml + handoff/contract.yaml).
        manifest_completeness 11/11. ready_for_build=true.
        다음 — /nf-build nssf 로 fresh 빌드."
```

## 자주 틀리는 지점

- 사용자의 `manual_overrides` 가 도구 재실행 후 사라졌는가. (도구가 보존하므로 사라지면 안 됨 — 사라졌다면 도구 회귀 의심. 단 `--reset` 은 의도된 fresh 시작이라 overrides 도 함께 archive 로 간다.)
- `<nf>` 와 `<spec>` 의 관계가 맞는가 (29.531 ↔ nssf, 29.510 ↔ nrf, 29.503 ↔ udm 등). 다른 조합은 사용자에게 확인.
- `--reset` 호출 시 timestamp 가 같은 초에 두 번 발생하면 archive 충돌. 1초 대기 또는 다른 timestamp 로 재시도.
- archive 폴더는 `.gitignore` 대상이라 *그대로 두면 git 에서 보이지 않는다*. 사용자가 보존 추적이 필요하면 archive 직전에 `git tag archive/<nf>-<YYYYMMDD>` 권장.

## 참고 — 본 skill 안에 다시 적지 말 것

- 매니페스트 schema·자동 검출 알고리즘: `design/scripts/nf-manifest.py` docstring.
- 카테고리 분류 표 (CATEGORY_TABLE): 같은 도구 안.
- 디렉터리·파일명 규칙: `CLAUDE.md`.
