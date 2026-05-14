# 2026-05-14 — NSSF full scope 정정 + seed-gen schema_refs fix summary

Status: complete
Current objective: NSSF 범위를 NSSelectionGet MVP 한정에서 *전체 NSSF 기능 (NSSelection + NSSAIAvailability)* 으로 재정의하고, 그 과정에서 발견한 `nf-seed-gen.py` 의 잘못된 schema_refs 인식 버그를 fix 한 사이클을 한 문서로 남긴다.

## 한 줄 요약

`/nf-init nssf --primary 29.531` 결과를 사용자가 검토하다 *MVP scope 가정* 오해가 드러났고, 본 NSSF 는 두 SBI service (Nnssf_NSSelection 1 op + Nnssf_NSSAIAvailability 7 op) 전체를 다루는 *full NSSF* 구현 대상이라는 점이 재확인됐다. 동시에 seed 의 `data-model/<HTTP code>` 18 개 노이즈가 도구 버그임을 식별해 `_schema_refs` 가 `components/schemas/` 만 인식하도록 PR #15 로 fix 했다.

## Phase 0 — 사이클 시작 컨텍스트

- 직전 사이클 (PR #14 + 후속 direct push) 로 `_extracted/`, `design/<nf>/contract/`, `handoff/<nf>/contract.yaml`, `_contract_seed.yaml` 모두 gitignored 정책 완성.
- 사용자가 `/nf-init nssf --primary 29.531` 호출.

## Phase 1 — manifest 첫 생성 + missing 진단

- specs/29.531 안 docx + 2 yaml 만 있는 fresh 상태에서 도구가 9/11 in-scope, ready=false 진단.
- `missing_priority` — `33.501` (security), `38.413` (cross-nf NG-RAN).

## Phase 2 — `missing_priority` 도출 근거 설명

사용자 질의에 따라 알고리즘 추적.

- 의존 spec = `yaml_refs ∪ docx_refs - {primary}`.
- `CATEGORY_TABLE` + head 번호 fallback 으로 카테고리 분류.
- `zzz_skip` + `manual_overrides.exclude` 제외 후 `in_scope`.
- `present:false` 만 missing.
- priority — sba_common(0) < security(1) < data_types(2) < ids(3) < procedures(4) < architecture(5) < discovery(6) < cross-nf(7) < other(8).

NSSF 의 33.501·38.413 도출 출처 — 둘 다 `docx_clause_2`.

## Phase 3 — 33.501·38.413 의 NSSF 실효성 분석

NSSF 본문 grep 으로 *실제* 인용 패턴 확인.

- 33.501. NSSF API 가 자체 OAuth2 scope 정의 없음. OAuth2 client credentials 입력은 29.510 + 29.500 이 진실. NSSF 가 받는 정보는 *OAuth2 사용 사실 자체* 뿐.
- 38.413. 본 spec 내 단 한 줄 인용 — AMF reallocation via RAN 의 NGAP REROUTE NAS REQUEST §8.6.5. 본 절차는 *조건부 지원* 기능.

→ 둘 다 `manual_overrides.exclude` 후보로 식별.

## Phase 4 — `manual_overrides.exclude` 등록 + ready=true 도달

- `design/nssf/_manifest.yaml` 의 `manual_overrides.exclude` 에 33.501 + 38.413 추가.
- nf-manifest.py 재실행 → completeness 9/9, ready_for_build=true.
- nf-seed-gen.py 자동 실행 → seed emit. `categories=13 topics=43 tasks=8`.

## Phase 5 — *MVP scope* 가정 정정

사용자 정정. NSSF 는 NSSelectionGet 단일 op 한정이 아니라 **두 service 의 모든 operation** 을 구현해야 한다.

seed 의 8 tasks 가 두 service 모두 자동 도출했다는 사실은 다행이지만, 직전 사이클 retro 와 manifest reason 의 *MVP* 문구는 정정 대상이다.

## Phase 6 — `nf-seed-gen.py` schema_refs 노이즈 식별

seed 의 data-model 토픽 43 개 중 18 개가 HTTP 상태 코드 (`307`, `308`, `400`, ..., `default`).

원인. `_schema_refs` 가 OpenAPI 안 *모든* `$ref` 를 schema 로 간주. `responses: { 400: { $ref: '#/components/responses/400' } }` 패턴이 `data-model/400` 토픽으로 들어갔다.

## Phase 7 — 사용자 결정 4 건

| # | 항목 | 결정 |
|---|---|---|
| 1 | 38.413 처리 | exclude 유지, reason 에서 MVP 문구 제거 + "운영 결정 보류" 명시 |
| 2 | 33.501 처리 | exclude 유지, reason 에서 MVP 문구 제거 + "운영/배포 정책으로 외부화, 구현 깊이 0" 명시 |
| 3 | seed-gen 노이즈 | **도구 fix 먼저** — PR 로 처리 |
| 4 | architecture/dev 산출 | full scope 으로 명시적 재작업 — 별도 후속 사이클 |

추가 — 본 사이클 새 retro 작성 + 다음 사이클은 arch/dev full scope 확장으로 진행.

## Phase 8 — manifest reason 정정 (local-only edit)

`design/nssf/_manifest.yaml` 은 gitignored 라 로컬 편집만.

- 33.501 reason — "NSSF 가 자체 OAuth2 scope 정의 없음. OAuth2 client credentials 구현 입력은 29.510 + 29.500 이 진실. SBA TLS·token 정책은 운영/배포 정책으로 외부화 — 구현 깊이 0."
- 38.413 reason — "AMF reallocation via RAN (NGAP REROUTE NAS REQUEST §8.6.5) 지원 결정 보류. 운영 정책으로 미구현 확정되면 keep, 지원하기로 하면 specs/38.413 cp + exclude 제거."

도구가 manifest 의 deps `excluded` 필드와 manual_overrides `reason` 필드 양쪽에 같은 문구를 쓰므로 양쪽 모두 정정. bg session edit 차단으로 bash + python 으로 in-place 편집.

## Phase 9 — seed-gen fix PR #15

`design/scripts/nf-seed-gen.py` 의 `_schema_refs` 가 `$ref` 안에 `/components/schemas/` 가 포함됐을 때만 schema 토픽 등록하도록 수정.

### 변경

```python
_SCHEMA_REF_RE = "/components/schemas/"

def _schema_refs(obj):
    if isinstance(obj, dict):
        ref = obj.get("$ref")
        if isinstance(ref, str) and _SCHEMA_REF_RE in ref:
            refs.append(_ref_name(ref))
        ...
```

### 테스트

`tests/scripts/test_seed_gen.py` 신규.

- `test_response_ref_does_not_become_schema_topic` — `responses: { 400: { $ref: '#/components/responses/400' } }` 패턴이 `data-model/400` 토픽으로 들어가지 않음 회귀.
- `test_request_body_schema_ref_kept` — `requestBody.content.<media>.schema.$ref` 가 정상 등록됨 정방향.

### 검증

- `pytest tests/scripts` — `42 passed` (이전 40 + 신규 2).

### PR

- branch `push/nf-seed-gen-schema-ref-filter-20260514`.
- commit `c237f82 nf-seed-gen 가 components/schemas 의 $ref 만 schema 토픽으로 인식한다`.
- merge commit `d986697`.

## Phase 10 — 동기화 + 정리

- ExitWorktree remove (worktree + 로컬 push branch 삭제).
- `git pull --ff-only origin main` — `c237f82` 까지 ff.
- `git push origin --delete push/nf-seed-gen-schema-ref-filter-20260514` — 원격 정리.
- 현재 main HEAD — `d986697`.

## 변경된 핵심 파일

PR #15 머지 기준.

- `design/scripts/nf-seed-gen.py` (`_schema_refs` 의 `/components/schemas/` 필터)
- `tests/scripts/test_seed_gen.py` (신규 2 테스트)

로컬만 (gitignored).

- `design/nssf/_manifest.yaml` (33.501 + 38.413 manual_overrides.exclude 등록 + reason 정정)
- `design/nssf/_contract_seed.yaml` (auto-gen 산출)

## 관찰 + 다음 사이클 권고

- **scope 가정은 매 사이클 명시적으로 재확인하라.** 직전 retro 의 "NSSF MVP (NSSelectionGet)" 문구가 본 사이클까지 그대로 흘러와 `manual_overrides.exclude.reason` 에 들어갔다. retro 의 *결정* 은 후속 사이클의 *입력 가정* 으로 자동 승격되므로 명시적 검토가 필요하다.
- **seed 자동 생성은 contract 빌드 직전에 한 번 검증하라.** 본 사이클의 18 개 노이즈는 도구 버그였으나, seed 의 `topics` 분포는 사람이 한 번 훑어볼 가치가 있다 — `api/<op>` 갯수가 yaml `paths` 의 operation 갯수와 일치하는지, `data-model/<schema>` 가 실제 schema 이름인지.
- **gitignored 파일도 reason 텍스트는 다중 위치에 복제될 수 있다.** nf-manifest.py 가 `excluded` 와 `reason` 양쪽 같은 텍스트를 쓰므로 정정 시 `replace` 전체 대체가 필요. `replace(o, n, 1)` 으로 첫 매치만 바꾸면 다른 한쪽이 그대로 남는다.
- **다음 사이클** — `design/nssf/architecture/`, `design/nssf/module-decomposition/`, `dev/nssf/` 의 기존 산출은 MVP 가정으로 작성됐다. full scope (8 operation + 통지/구독 model + AvailabilityEngine·NotificationDispatcher 모듈) 으로 재작업 필요. 별도 PR 사이클로 분리.

## 최종 상태

- NSSF scope = full (NSSelection + NSSAIAvailability 두 service 의 8 operation).
- 33.501·38.413 = `manual_overrides.exclude`, reason 의 MVP 문구 제거 완료, 운영 결정 보류 명시.
- seed-gen schema_refs 노이즈 = 도구 fix 머지 완료, 회귀 테스트 보유.
- arch/dev full scope 재작업 = 다음 사이클로 미룸.
- 원격 main = `d986697` clean, 로컬 sync 완료.
