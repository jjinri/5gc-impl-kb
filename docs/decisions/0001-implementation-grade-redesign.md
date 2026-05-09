# ADR 0001 — Implementation-Grade Knowledge Base 재정의

- **Status**: Proposed (사용자 승인 대기)
- **Date**: 2026-05-09
- **Authors**: jjinri, Claude (Opus 4.7)
- **Supersedes**: 본 프로젝트의 초기 정의 (Karpathy LLM Wiki 패턴 기반 spec 요약)

## Context

2026-05-09 시작 시점 정의는 *3GPP 5GC 스펙·관련 paper 의 개인 지식 베이스* 였다 (Karpathy 의 LLM Wiki 패턴 → 3GPP 도메인 적응). 같은 날 첫 NSSF (TS 29.531 v19.6.0) 페이지를 등록하며 다음 자산을 구축.

- 도구 — `scripts/extract.py`, `scripts/setup.sh`, `scripts/render-mermaid.py`, `scripts/resolve-yaml-refs.py`, `scripts/nf-manifest.py`
- skill — `.claude/skills/add-spec/`, `.claude/skills/update-spec/`, plugin `skill-creator`
- 첫 페이지 — `kb/nssf/3gpp-ts-29531.md` (657 줄, Service Flows mermaid 3개 + Data Model chain 트리 9개)
- 첫 매니페스트 — `kb/nssf/_manifest.yaml` (manual_overrides 동작 검증 완료)

같은 날 사용자가 wiki 의 *목적* 을 재정의했다.

> 3GPP 규격 *이해* 를 위해서가 아니라, *core NF 의 software 구현* 단계에서 필요한 모든 지식을 카테고리별로 정리한 knowledge base. TS 29.531 NSSF / TS 29.510 NRF 같은 명확한 core NF 가 개발 target 이고, IOT 연동을 위한 다른 NF 지식은 보조. C 언어 자료형으로 떨어뜨리려면 spec 간 reference chain 이 끝까지 풀려야 — `void *` 가 남으면 실제 빌드가 안 된다. 긴 docx 의 섹션 분해, 그림·표 해석 능력도 필요.

본 ADR 은 이 재정의로부터 도출된 결정·sprint 계획·열린 이슈를 기록한다.

## Decisions

### D1 — 새 프로젝트 목적

> 3GPP 5GC Core NF 의 *소프트웨어 구현* 에 필요한 모든 지식을 카테고리별로 정리한 knowledge base.

이해용 wiki (Karpathy 패턴) 와의 차이는 결정적. 이해는 prose 요약·"참조 규격 미등록" leaf 로 끝나도 OK. 구현은 leaf 가 *컴파일 안 되는 C struct* 가 됨. 따라서 chain 완전성·표 보존·그림 해석·매트릭스화가 모두 *acceptance criteria* 로 들어감.

### D2 — NF 페이지 schema (7 카테고리)

한 NF 페이지의 ## H2 섹션을 다음 7개로 표준화.

| 카테고리 | 담는 정보 | 주 자료원 |
|---|---|---|
| Interface | API URI, HTTP/2, OAuth, header convention | primary yaml `info`/`servers` + docx §6.x.1 |
| API | operation 매트릭스 (method · path · req · resp · error) | primary yaml `paths` |
| Data Model | `$ref` chain 끝까지 풀린 자료형 트리 | yaml + cross-spec yaml chain |
| Service Scenarios | sequence diagram (mermaid) + 상태도 + 트리거 절차 | primary docx + 23.502 cross-ref |
| Cross-NF Dependencies | consumer / producer 호출 그래프 | 모든 NF spec grep |
| Configuration | feature, supportedFeatures bit, default, timeout | primary yaml + docx 표 |
| Error Handling | code 표 + ProblemDetails cause + 권장 복구 | primary yaml `responses` + docx ProblemDetails 표 |

### D3 — 페이지 분리 정책 (γ 하이브리드)

한 NF = 한 페이지 (7 카테고리 모두). 단 Data Model 트리가 단일 800 줄 초과 시 `<page>-data-model.md` sub-page 로 분리. 다른 카테고리는 분리하지 않음.

### D4 — Acceptance criteria (현 시점 — sprint 후반에 강화)

*"이 페이지로 코드를 짤 수 있는가"* 의 binary 정의.

- **(a)** Data Model chain leaf 0 건. 즉 모든 `$ref` 가 primitive 또는 `manual_overrides.exclude` 로 의도된 종료.
- **(b)** docx 표가 markdown table 로 보존 (현 `extract.py` 의 cell join 평탄화 X).
- **(c)** 그림 PNG 추출 + alt text 보유 — sprint 2 이후 도입.
- **(d)** cross-NF caller 표가 자동 검증 통과 — sprint 2 이후.
- **(e)** `yaml-to-c.py` 산출이 `gcc -fsyntax-only` 통과 — sprint 2 이후.

(a)+(b) 가 sprint 1 의 acceptance, 그 외는 점진 도입.

### D5 — 진실 출처 분담

- **`CLAUDE.md`** = *정책*. purpose / FOUR RULES / 디렉터리 규약 / 언어 정책 / 본 ADR 인덱스 / onboarding.
- **`.claude/skills/*/SKILL.md`** = *절차*. workflow / 도구 호출 인자 / 자기검증 체크리스트.
- 두 곳에 동일 내용 중복 금지. SKILL 본문이 곧 정책이 되거나 CLAUDE.md 가 절차를 베끼는 일 없음.

### D6 — Skill 구조 (NF-centric, 구 spec-centric 폐기)

| 신규 skill | 책임 |
|---|---|
| `/nf-init <nf> [--primary <spec>...]` | 매니페스트 보강 (반복 가능). 사용자 입력 = NF 이름 + 주 spec(s). 나머지 의존 spec 은 yaml `$ref` chain + docx Clause 2 References 자동 검출 |
| `/nf-build <nf> [<category>]` | 매니페스트 ready_for_build = true 일 때 7 카테고리 페이지 생성. category 인자로 부분 빌드 (예 `data-model` 만) |
| `/nf-status <nf>` | `_status.yaml` 산출. acceptance check 개별 평가 + gate 통과 여부 |

구 `add-spec` / `update-spec` 은 `.claude/skills/_legacy/` 로 이동 (즉시 삭제 안 함, sprint 1 끝까지 grace).

### D7 — 평가 framework (무가중치 + criterion 의무 + NF profile)

`_status.yaml` 의 invariant.

1. **무가중합·무등급**. 가중치는 사람마다 관점이 다르므로 합산 점수 / A·B·C 등급 안 만든다.
2. **모든 check 는 `criterion` (PASS 정의) + `to_pass` (FAIL 시 액션 리스트) 의무**. FAIL 이 영구 blocker 로 남는 일 없음.
3. **각 check 는 `applies_to: [profile, ...]` 명시**. 적용 안 되는 profile 에선 `NOT_APPLICABLE` 상태.
4. **Profile 4종** — `stage_3_only` (NSSF/NRF/UDM 등 yaml 동봉), `stage_2_only` (드뭄), `mixed` (NWDAF/AMF/SMF), `meta_only` (TS 23.501 자체 페이지).
5. **Acceptance gate 는 check id 의 AND**. `NOT_APPLICABLE` 은 자동 만족. blocker 만 보고.
6. **`manual_overrides.pass_anyway`** — 진짜 우회는 사용자 명시 결정 + 사유 기록.

Gate 단계 — `draft → ready_for_review → implementation_ready → production`. `implementation_ready` 가 본 wiki 의 핵심 acceptance 지점.

### D8 — Sprint 구조 (forward plan)

| Sprint | 작업 | 산출 |
|---|---|---|
| 0a | 정의·결정 합의 | 본 ADR (이 문서) |
| 0b | CLAUDE.md 재작성 | 새 정의·새 파이프라인·THE FOUR RULES 갱신 |
| 0c | 3 신규 SKILL + `scripts/nf-status.py` + 구 skill decommission | `.claude/skills/{nf-init, nf-build, nf-status}/` |
| 1 | NSSF 페이지 새 7-카테고리 schema 로 재구성 | `kb/nssf/3gpp-ts-29531.md` 갱신 + `_status.yaml` |
| 2 | sprint 1 에서 발견된 도구 갭 1개 신규 | `extract-tables.py` 또는 `extract-figures.py` 또는 `yaml-to-c.py` 중 하나 |
| 3 | 두 번째 NF (NWDAF mixed profile 또는 NRF stage_3_only) 로 generic 검증 | `kb/{nf}/` |
| 4+ | 추가 NF / IOT / cross-cutting | 누적 |

## Consequences

### 살리는 것

- 도구 5종 모두 그대로 유효 — `extract.py`, `resolve-yaml-refs.py`, `render-mermaid.py`, `nf-manifest.py`, `setup.sh`.
- 현 NSSF 페이지의 *Data Model 트리* 와 *Service Flows mermaid* — 새 schema 의 일부로 재배치.
- File naming convention (`specs/{spec}/{file}`, stem `3gpp-{ts|tr}-{n}-v{ver}` / `3gpp-{ts|tr}-{n}`).
- Categories 디렉터리 트리 (`{nf}/`, `architecture/`, `interfaces/`, `security/`, `slicing/`, `concepts/`, `overviews/`, `other/`).
- Language policy (한국어 prose, 영문 frontmatter, 콜론 종결 금지).
- Source document management rules (cp only, symlink 금지, source_path 절대 경로).

### 갈아엎는 것

- CLAUDE.md 의 첫 머리 — Karpathy 인용·"단순 요약→정리" 다이어그램.
- "Adding a New Document" Step 0~5 의 spec 단위 절차 → NF 단위로.
- "Knowledge Compounding" 의 양적 KPI ("대화 1회당 5–15 wiki 페이지") → *깊이* 우선.
- THE FOUR RULES #4 표현 — "wiki 에 doc 없으면 say so" 에서 "chain 끝까지 안 풀리면 implementation 불가능" 으로 강화.
- `add-spec` / `update-spec` SKILL — `_legacy/` 로 이동.
- 현 NSSF 페이지의 섹션 구조 (Summary/KC/Methodology/...) → 7 카테고리.

## Open Issues

### O1 — 디렉터리·프로젝트 이름

새 정의에서 다음 이름이 *완벽히 정확하지 않음*.

| 현 이름 | 새 정의에서 어색한 이유 | 더 정확한 후보 |
|---|---|---|
| `llm-wiki` (프로젝트) | "LLM wiki" 는 paper 합성 위주 Karpathy 패턴. 새 정의는 *구현 KB* | `5gc-impl-kb`, `nf-impl-kb`, `5gc-kb` |
| `specs/` | "paper" 는 academic 어감. 우리는 3GPP spec | `specs/`, `originals/`, `3gpp/` |
| `digests/` | "sources" 는 source code 어감. 여기선 *spec 의 1차 한국어 요약* | `digests/`, `summaries/`, `notes/` |
| `kb/` | 읽기용 백과사전 어감. 새 정의는 *구현용 페이지 모음* | `kb/`, `nfs/`, `pages/` |

본 ADR 종료 직후 별도 결정. 본 문서 자체의 위치 (`decisions/`) 도 결정 영향권.

### O2 — `_legacy/` 보관 기간

구 `add-spec` / `update-spec` 의 즉시 삭제 vs sprint N 끝까지 보관. 미결.

### O3 — Tier 3 / Tier 4 도구 도입 시점

- Tier 3 — `yaml-to-c.py` (자료형 → C struct 컴파일 검증). NSSF Sprint 1 후반 또는 Sprint 2.
- Tier 4 — LLM-as-judge 자동화. 미결 — 사용자 수동 review 우선.

### O4 — Cross-NF caller 그래프 자동 검증

모든 spec 의 `*.docx` 와 `*.yaml` 에서 본 NF 호출 grep — 도구 신규 (`scripts/extract-cross-nf.py`). Sprint 2 후반 후보.

### O5 — 큰 docx 의 섹션별 사전 분해 (`scripts/docx-split.py`)

TS 23.501 (1,000+ 페이지) 같은 spec 처리 시 필수가 될 가능성 높음. 첫 stage 2 NF (mixed profile) 처리 시점에 도입.

### O6 — Figure 추출 (`scripts/extract-figures.py`)

`python-docx.inline_shapes` 로 PNG 추출. 정확한 sequence diagram 을 mermaid 로 재현하는 출처. Acceptance criterion (c) 활성화 시 필수.

### O7 — Multi-primary NF 처리 (`/nf-init` 인자 형태)

NWDAF (23.288 + 29.520), AMF (29.518 + 29.508 + ...) 같이 *복수 primary spec* NF. `/nf-init <nf> --primary <spec1> [<spec2> ...]` 로 가변 인자 지원 필요. `nf-manifest.py` 도 단일 primary 가정을 푸어야 함.

## Status of agreement

다음 7 결정에 대해 *기본값* 합의 또는 조정.

1. **카테고리 7개 (D2)** — 기본값 OK?
2. **γ 하이브리드 (D3)** — 800 줄 임계 OK?
3. **Acceptance (D4)** — (a)+(b) sprint 1, (c)~(e) 후반?
4. **진실 출처 (D5)** — α (SKILL = 절차, CLAUDE.md = 정책)?
5. **Skill 구조 (D6)** — α (NF-centric 신설 + 구 skill `_legacy`)?
6. **평가 framework (D7)** — α' (무가중 + criterion 의무 + applies_to)?
7. **Generalization (D7 NF profile)** — 4 profile 도입?

7개 모두 *기본값 OK* 면 본 ADR Status 를 *Accepted* 로 갱신, sprint 0b/0c 로 진행.

## Artifacts (현재까지)

본 ADR 생성 시점의 주요 자산.

- 회의록 — 본 파일 (`decisions/0001-implementation-grade-redesign.md`)
- 도구 — `scripts/extract.py`, `setup.sh`, `render-mermaid.py`, `resolve-yaml-refs.py`, `nf-manifest.py`
- skill — `add-spec/`, `update-spec/` (둘 다 sprint 0c 에서 `_legacy/` 이동 예정)
- 산출 — `kb/nssf/3gpp-ts-29531.md`, `kb/nssf/_manifest.yaml`, `kb/nssf/_diagrams/*.svg`
- 직전 커밋 — `502fdf6 feat(tools): nf-manifest manual_overrides 적용`
