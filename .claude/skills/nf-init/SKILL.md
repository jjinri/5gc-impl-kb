---
name: nf-init
description: 본 5gc-impl-kb 의 새 NF (Network Function) 작업을 *시작* 하거나 기존 NF 의 contract 추출 산출물만 *백업 후 다시 추출 준비* 하는 워크플로우. 사용자가 "NSSF 시작", "/nf-init nrf", "AMF 매니페스트 만들어", "amf 구현 시작", "create nf manifest", "29.510 으로 NRF 작업 시작", "NSSF 리셋", "NRF contract 백업하고 다시 빌드", "/nf-init nssf --primary 29.531 --reset" 등을 말하거나 NF 이름 + 주 spec 번호를 지정하면 무조건 이 skill 을 사용한다. 핵심 의도는 사람이 legacy `_handoff.yaml` 을 수동 작성하던 부담을 없애고 `_contract_seed.yaml` 을 자동 생성해 contract extraction 을 시작 가능하게 만드는 것이다. 동작 — (1) `--reset` 인 경우 contract 산출물 (`design/<nf>/contract/`·`design/<nf>/_contract_status.yaml`·`handoff/<nf>/contract.yaml`) 만 `design/<nf>/_archive/<timestamp>/` 로 mv 하고 `_manifest.yaml`·`_contract_seed.yaml`·architecture/dev planning 산출은 보존 (2) `design/scripts/nf-manifest.py <nf> --primary <spec> --write` 호출해 `design/<nf>/_manifest.yaml` 을 refresh (3) ready 이면 `design/scripts/nf-seed-gen.py <nf>` 로 `_contract_seed.yaml` 을 자동 생성/갱신 (4) 산출 매니페스트·seed status 를 사용자에게 보고 (5) ready 가 아니면 어느 spec 을 specs/ 에 추가해야 하는지 priority 순으로 알린다. 본 skill 은 *반복 가능* 하다 — 사용자가 새 spec 을 specs/ 에 cp 한 뒤 다시 호출하면 매니페스트가 보강된다. ready 가 될 때까지 반복하고, 그 다음 `/nf-build` 로 contract 산출물을 생성한다. 신규 NF 의 매니페스트 + seed *생성/재생성* 만 한다 — 페이지 빌드는 `/nf-build`, 완성도 검사는 `/nf-status` 의 책임.
argument-hint: "<nf> --primary <spec> [--reset]"
allowed-tools: Bash(.venv/bin/python3 design/scripts/nf-manifest.py *) Bash(.venv/bin/python3 design/scripts/nf-seed-gen.py *) Bash(ls *) Bash(cat *) Bash(mkdir -p *) Bash(git mv *) Bash(mv *) Bash(date *) Bash(find design/* *)
---

# nf-init — NF 매니페스트 + seed 생성·보강

> Compatibility alias. Canonical user-facing wrapper is `/nf-spec-discover`. Keep this workflow behavior identical until alias removal is explicitly planned.

## 입력
- `<nf>` — `nssf`, `nrf`, `amf` 등 NF 이름 (소문자, design/ 하위 폴더명).
- `--primary <spec>` — NF 의 주 spec 번호 (점 포함, 예 `29.531`). 필수.
- `--reset` — *파괴적 옵션*. contract 추출 산출물 (`design/<nf>/contract/`, `design/<nf>/_contract_status.yaml`, `handoff/<nf>/contract.yaml`) 만 `design/<nf>/_archive/<YYYYMMDD-HHMMSS>/` 로 mv 한 뒤 manifest 를 refresh 하고 ready 이면 seed auto-gen 을 호출한다. `design/<nf>/_manifest.yaml`, `design/<nf>/_contract_seed.yaml`, `design/<nf>/architecture/`, `design/<nf>/module-decomposition/`, `dev/<nf>/` 는 보존한다. 주의 — `_contract_seed.yaml` 도 보존 대상이라 `--reset` 만으로는 *기존 seed 가 재생성되지 않는다* (보수적 default). 도구 fix 후 stale seed 를 깨끗이 다시 만들려면 step 5 의 `--force` 를 함께 써야 한다. `--primary <spec>` 과 함께 써야 하며, 명시 flag 이므로 추가 [Y/n] 프롬프트 없이 즉시 archive 실행. 옮길 파일 표 + 보존 파일 표 + archive 위치는 항상 *결과 보고* 로 출력.
- 인자 없으면 어느 NF 인지·주 spec 이 무엇인지 사용자에게 묻고 정지.

> 다중 primary spec (예 NWDAF = 23.288 + 29.520) 은 sprint 후반에 도구 인자 확장 예정. 현재는 *대표 1개* 로 시작하고 나머지는 매니페스트에서 docx_clause_2 references 로 자동 검출.

## 본 skill 의 책임 범위

본 skill 은 매니페스트 + `_contract_seed.yaml` *생성·보강·재생성* 에 한정. *페이지 빌드는 `/nf-build`, 완성도 검사는 `/nf-status`* 의 책임.

| 시나리오 | 사용 skill |
| --- | --- |
| 새 NF 작업 처음 시작 | `/nf-init <nf> --primary <spec>` |
| specs/ 에 새 ref spec cp 후 매니페스트 갱신 | `/nf-init <nf> --primary <spec>` (재실행) |
| 매니페스트가 ready 됐고 페이지 생성 차례 | `/nf-build <nf>` |
| 페이지가 handoff_ready 기준 통과하는지 점검 | `/nf-status <nf>` |
| 기존 NF contract 산출만 백업하고 재추출 준비 | `/nf-init <nf> --primary <spec> --reset` → `/nf-build <nf>` |
| spec 시리즈 자체를 바꿔 새로 시작 | `/nf-init <nf> --primary <new-spec> --reset` → `/nf-build <nf>` |

## 동작 원칙 (이유 포함)

- **CLAUDE.md THE FOUR RULES 가 우선.** 본 skill 의 매니페스트 자동 검출도 그 규칙 안에서 동작 — 외부 web 에서 의존성 가져오지 않고, 추출 텍스트(docx 또는 yaml) 에 적힌 ref 만 카운트.
- **사용자 `manual_overrides` 는 `nf-manifest.py` 가 보존한다.** 이유 — 매니페스트 자동 검출은 *제안* 이고 일부 spec 은 NF 컨텍스트 외 (예 NG-RAN 38.413). 사용자가 exclude 로 결정하면 그 결정을 다음 호출에서 잃으면 안 됨. `--reset` 도 manifest 를 archive 하지 않으므로 manual_overrides 를 보존한 채 contract 추출 산출물만 다시 만든다.
- **ready_for_build 가 false 면 *무엇을* 추가해야 하는지 알린다.** 그저 "ready=false" 만 보고하면 사용자가 다음 액션을 추측해야 한다. priority 순서로 missing spec 을 명시하면 즉시 다음 행동으로 이어진다.
- **ready=true 가 되는 즉시 seed 자동 emit.** 이 skill 의 핵심 목적은 사람이 legacy handoff 계약을 손으로 쓰지 않게 하는 것이다. yaml/docx 로부터 가능한 categories, api/data-model topics, tasks 를 `_contract_seed.yaml` 로 자동 생성해 `/nf-build` 가 `handoff/<nf>/contract.yaml` 을 만들 수 있게 한다. module-decomposition 및 USER 섹션 prose 는 자동 범위 밖 — 사람이 후속 사이클에 보강.
- **`--reset` 은 contract archive (mv) — 삭제 아님.** contract 추출 산출물만 `design/<nf>/_archive/<timestamp>/` 로 옮긴다. timestamp 충돌은 1초 대기 또는 다른 timestamp 로 재시도. archive 폴더 자체는 `.gitignore` 로 untrack — repo 트리 노이즈 방지. git history 보존이 필요하면 archive 전에 `git tag archive/<nf>-<YYYYMMDD>` 사용 권장.
- **archive 후 페이지 빌드는 별도 호출.** 본 skill 은 manifest + seed 생성까지만 — `/nf-build` 가 빌드의 진실 출처. 한 skill 이 archive + manifest + seed + build 모두 하면 *어느 단계에서 무엇이 망가졌는지* 추적이 어려워진다.
- **`--reset` 의 destructive mv 는 SKILL 의 bash 가 수행, `nf-manifest.py`·`nf-seed-gen.py` 는 자기 산출만 만든다.** 단일 책임 — 도구는 자동 검출/seed 생성 로직만 갖고, 파괴적 행위는 SKILL 단계에서 명시 처리.

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
- **mv 대상 (contract-only allowlist)** — contract extraction 산출물만 archive 한다. `_manifest.yaml`, `_contract_seed.yaml`, `architecture/`, `module-decomposition/`, `dev/<nf>/` 는 보존한다.
  ```bash
  if [ -e design/<nf>/contract ]; then
    git mv -k design/<nf>/contract <archive_dir>/contract 2>/dev/null || mv design/<nf>/contract <archive_dir>/contract
  fi
  if [ -e design/<nf>/_contract_status.yaml ]; then
    git mv -k design/<nf>/_contract_status.yaml <archive_dir>/_contract_status.yaml 2>/dev/null || mv design/<nf>/_contract_status.yaml <archive_dir>/_contract_status.yaml
  fi
  if [ -e handoff/<nf>/contract.yaml ]; then
    mkdir -p <archive_dir>/handoff
    git mv -k handoff/<nf>/contract.yaml <archive_dir>/handoff/contract.yaml 2>/dev/null || mv handoff/<nf>/contract.yaml <archive_dir>/handoff/contract.yaml
  fi
  ```
  (`git mv -k` 실패 시 `mv` 로 fallback. archive 폴더는 `.gitignore` 대상이라 git 추적 밖.)
- **handoff yaml** — `handoff/<nf>/contract.yaml` 만 대상이다. legacy `handoff/<nf>/_handoff.yaml` 는 폐기됐고 reset 입력도 archive 대상도 아니다.
- 옮길 파일이 0 개이면 archive 폴더 생성하지 말고 "이미 비어있음" 보고 후 §4 로 진행.

### 4. 도구 실행
```bash
.venv/bin/python3 design/scripts/nf-manifest.py <nf> --primary <spec> --write
```
- `--write` 로 `design/<nf>/_manifest.yaml` 저장.
- 도구 stderr 의 status 라인을 사용자에게 그대로 전달.
- `--reset` 이어도 기존 `_manifest.yaml` 은 보존되므로 manual_overrides 는 계속 유지된다.

### 5. (조건부) seed 자동 생성
manifest 의 `status.ready_for_build == true` 인 경우에만 실행.

```bash
.venv/bin/python3 design/scripts/nf-seed-gen.py <nf>
```
- 산출 — `design/<nf>/_contract_seed.yaml`.
- yaml `paths` 의 모든 operationId 를 `api/<OpId>` 토픽으로, schema chain 을 `data-model/<Schema>` 토픽으로 자동 등록.
- module-decomposition 카테고리 = `status: draft, topics 없음` — 사람이 후속 사이클에 보강.
- 이 자동 seed 가 legacy `_handoff.yaml` 수동 작성 단계를 대체한다.
- **기본은 보수적 — 기존 seed 가 있으면 보존하고 재생성하지 않는다** (`nf-seed-gen.py` 가 scoped 결정·수동 편집 손실을 막으려 `--force` 없이는 보존). 기존 seed 없을 때만 새로 생성.

`--force` 운영 가이드.

```bash
.venv/bin/python3 design/scripts/nf-seed-gen.py <nf> --force
```
- `--force` 는 기존 seed 를 *덮어쓰고* 자동 산출 범위로 재생성한다.
- 사용 시점 — (a) `nf-seed-gen.py`/`nf-manifest.py` 도구가 fix 된 뒤 stale seed 를 깨끗이 다시 만들 때 (예 schema_refs 노이즈 fix 후), (b) manifest `manual_overrides` 변경을 seed 에 반영해야 할 때, (c) 사용자 작업트리에 손상·stale `_contract_seed.yaml` 이 의심될 때.
- 주의 — `--force` 는 seed 의 *사용자 수동 편집* 도 잃는다. scoped 결정이 seed 에 직접 들어있다면 `--force` 전에 그 결정을 manifest `manual_overrides` 로 옮겨라 (그래야 재생성 후에도 반영됨).
- `--reset` (contract 산출물 archive) 과 무관 — `--force` 는 seed 파일 1 개의 재생성 여부만 제어한다.

### 6. 결과 보고
- (`--reset` 시) archive 경로 + mv 된 파일 수.
- (`--reset` 시) 보존된 주요 산출 — `_manifest.yaml`, `_contract_seed.yaml`, `architecture/`, `module-decomposition/`, `dev/<nf>/`.
- 매니페스트 위치 — `design/<nf>/_manifest.yaml`.
- `manifest_completeness` 비율 (예 `5/11`).
- `ready_for_build` (true/false).
- false 면 `missing_priority` 리스트 + 각각의 의미 (sba_common / data_types / security 등).
- ready=true 면 seed 위치 + api/data-model topic 수·task 수. *다음 — `/nf-build <nf>`*.
- ready=false 면 *다음 추천 액션* — (a) priority 순으로 specs/ 에 cp 후 재호출 (b) 또는 manual_overrides.exclude 에 등록 후 재호출.

### 7. 마무리
- 사용자가 ready 시키면 `/nf-build <nf>` 가 다음 단계. 본 skill 은 거기로 *자동 위임하지 않는다* — ready_for_build 결과를 사용자가 확인 후 별도 호출.
- 매니페스트가 이미 ready 였으면 — seed 도 자동 emit. "이미 빌드 가능. `/nf-build <nf>` 로 진행" 안내.
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

**호출 — 기존 NSSF contract 산출만 archive 후 seed 자동 갱신.**

```
사용자: /nf-init nssf --primary 29.531 --reset
도구:   ts=20260513-153000; mkdir -p design/nssf/_archive/20260513-153000/
        mv design/nssf/contract design/nssf/_archive/20260513-153000/contract
        mv design/nssf/_contract_status.yaml design/nssf/_archive/20260513-153000/_contract_status.yaml
        mv handoff/nssf/contract.yaml design/nssf/_archive/20260513-153000/handoff/contract.yaml
        design/scripts/nf-manifest.py nssf --primary 29.531 --write
        (ready=true 면) design/scripts/nf-seed-gen.py nssf
보고:   "archive — design/nssf/_archive/20260513-153000/ (mv entries: contract/, _contract_status.yaml, handoff/contract.yaml; preserved: _manifest.yaml, _contract_seed.yaml, architecture/, module-decomposition/, dev/nssf/).
        manifest_completeness 11/11. ready_for_build=true.
        seed emit — api topics=8, data-model topics=25, tasks=8.
        다음 — /nf-build nssf 로 contract 재추출."
```

## 자주 틀리는 지점

- 사용자의 `manual_overrides` 가 도구 재실행 후 사라졌는가. (도구가 보존하므로 사라지면 안 됨 — 사라졌다면 도구 회귀 의심. `--reset` 도 manifest 를 보존하므로 overrides 가 사라지면 안 됨.)
- `_contract_seed.yaml` 자동 생성이 빠졌는가. (이 skill 의 핵심 목적은 사람이 legacy handoff yaml 을 수동 작성하지 않도록 seed 를 자동 생성하는 것이다.)
- `<nf>` 와 `<spec>` 의 관계가 맞는가 (29.531 ↔ nssf, 29.510 ↔ nrf, 29.503 ↔ udm 등). 다른 조합은 사용자에게 확인.
- `--reset` 호출 시 timestamp 가 같은 초에 두 번 발생하면 archive 충돌. 1초 대기 또는 다른 timestamp 로 재시도.
- archive 폴더는 `.gitignore` 대상이라 *그대로 두면 git 에서 보이지 않는다*. 사용자가 보존 추적이 필요하면 archive 직전에 `git tag archive/<nf>-<YYYYMMDD>` 권장.

## 참고 — 본 skill 안에 다시 적지 말 것

- 매니페스트 schema·자동 검출 알고리즘: `design/scripts/nf-manifest.py` docstring.
- 카테고리 분류 표 (CATEGORY_TABLE): 같은 도구 안.
- seed 카테고리 표·자동화 알고리즘 (paths → api 토픽, schema chain → data-model 토픽, task 템플릿): `design/scripts/nf-seed-gen.py` docstring.
- 디렉터리·파일명 규칙: `CLAUDE.md`.
