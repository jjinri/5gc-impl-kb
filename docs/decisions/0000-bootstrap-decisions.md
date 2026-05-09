# ADR 0000 — Bootstrap decisions

- **Status**: Accepted (2026-05-09 부트스트랩 시점). 일부 항목은 ADR 0001 이 supersede 함 (각 항목에 표시).
- **Date**: 2026-05-09 (부트스트랩 시점)
- **Authors**: jjinri, Claude
- **Note**: 본 문서는 부트스트랩 단계의 free-form `context-notes.md` 를 ADR 형식으로 재구성한 것이다. 이후 결정은 ADR 0001+ 로 이어진다.

## Context

본 프로젝트의 출발점. Karpathy 의 LLM Wiki 패턴을 3GPP 5GC 도메인에 적응해 NSSF (TS 29.531) 첫 페이지를 등록하기까지 내린 결정들의 모음. 이 시점의 정의는 *"3GPP 5GC 스펙·관련 paper 의 개인 지식 베이스"* 였다. 이후 ADR 0001 에서 *implementation-grade knowledge base* 로 재정의됨에 따라 본 ADR 의 일부 항목은 더 이상 진실의 출처가 아니다 — 각 항목에 표시.

## Decisions

### D1 — 분야 / 분야명
- 결정: `3GPP 5G Core Systems (5GC)`. RAN·물리계층은 범위 외.
- 이유: 사용자가 명시. NSSF 시작 → NF 단위로 확장 계획.
- 현재 상태: ADR 0001 로 *심화* — "이해" 가 아니라 "구현용 KB" 로 목적이 강화됨.

### D2 — 베이스 템플릿 (~~Karpathy 패턴~~)
- 결정: Karpathy LLM Wiki 패턴 + joonan30 fork 를 출발점.
- 이유: "원본 PDF → sources(LLM 요약) → wiki(정리)" 3-tier 구조와 THE FOUR RULES (no web search, wiki first, re-read source, say-so) 가 hallucination 방지에 잘 맞음.
- ~~상태: Active~~ → **ADR 0001 로 supersede.** 새 정의에서는 *prose 요약 → 정리* 흐름이 아니라 *spec + yaml + figure → 7 카테고리 페이지* 흐름. THE FOUR RULES 의 정신은 살리되 #4 표현은 강화 ("chain 끝까지 풀려야 implementation 가능").

### D3 — 전역 CLAUDE.md 와의 관계 (deduplication)
- 결정: 프로젝트 CLAUDE.md 는 도메인 고유 부분만, 전역 규칙은 "Inheritance" 블록으로 계승.
- 삭제: 템플릿 원안의 맨 아래 "Design Principles" 섹션 (Four Rules + 3-tier 헤더와 내부 중복).
- 현재 상태: Active. ADR 0001 의 D5 (진실 출처 분담) 와 합쳐서 더 강화 — CLAUDE.md = 정책, SKILL = 절차.

### D4 — 언어 정책
- 결정: wiki 본문 prose·sources/*.md·index.md 설명문은 한국어. YAML frontmatter, 섹션 헤더, 3GPP 약어, 스펙 정의 인용은 영어 유지.
- Override 명시: 전역 §5 (한국어 마침표, 콜론 종결 금지) 는 wiki 본문에도 적용. 전역 §6 (한국어 헤더 주석) 은 마크다운/YAML 비대상, `scripts/*.py` 에는 적용.
- 현재 상태: Active. 변경 없음.

### D5 — Repository 조직 — papers/ 의 spec-number 폴더
- 결정: `papers/{spec-number-with-dot}/{original-3gpp-filename}` 구조. 폴더명 점 포함 (`29.531`), 파일명은 3GPP 원본.
- 이유: 3GPP 다운로드는 spec 시리즈 단위로 묶임 + 같은 시리즈에 여러 release/version 공존. nested 가 자연스러움.
- 현재 상태: Active. 단 ADR 0001 의 O1 에서 *디렉터리 이름 자체* (`papers/`, `sources/`, `wiki/`) 가 새 정의에 부정확하다는 이슈가 열림 — sprint 0c 에서 `specs/`, `digests/`, `kb/` 로 rename 예정.

### D6 — Karpathy 1:1:1 매핑 분기
- 결정:
  - papers — spec 버전마다 1파일 (멀티 포맷·멀티 릴리즈 공존)
  - sources — 실제 요약한 버전마다 1개 (`sources/3gpp-ts-{n}-v{ver}.md`)
  - wiki — spec 시리즈마다 1개 (`wiki/{nf}/3gpp-ts-{n}.md`), 본문에 `## Version History`
- 이유: "TS 29.503 가 뭐냐" 자연스러운 질문에 한 페이지로 답할 수 있어야 함.
- 현재 상태: Active. 7 카테고리 schema (ADR 0001 D2) 도입 후에도 *시리즈 단위 한 페이지* 원칙은 유지.

### D7 — 멀티 포맷 지원 (.pdf/.doc/.docx)
- 결정: `scripts/extract.py` 단일 헬퍼에서 확장자 분기. `.pdf`→pypdf, `.docx`→python-docx, `.doc`→soffice 후 python-docx (폴백 antiword).
- 이유: 3GPP 는 전통적으로 .doc/.docx 배포. PDF only 면 워크플로우 깨짐.
- YAML 필드 변경: 원안 `pdf_path`/`pdf_filename` → `source_path`/`source_filename`/`source_format` 으로 일반화.
- 현재 상태: Active.

### D8 — NF 폴더 — 사전 생성 안 함
- 결정: `wiki/nssf/` 만 만들고, 다른 NF 폴더는 첫 문서 추가 시점에 생성.
- 이유: 전역 §2 (단순성, 사변적 셋업 금지).
- 현재 상태: Active. NF-centric skill 도입 후에도 동일.

### D9 — 의존성 관리 (venv)
- 결정: 시스템 pip 대신 프로젝트 `.venv/`. `.gitignore` 에 `.venv/` 포함.
- 이유: WSL2 Ubuntu 의 PEP 668 정책으로 시스템 Python 직접 설치 불가.
- 현재 상태: Active.

### D10 — `.doc` 지원 도구 (지연 결정)
- 결정: extract.py 가 .doc 입력 시 친절한 안내. 사용자가 첫 .doc 마주칠 때 설치.
- 이유: 어느 도구를 쓸지 (LibreOffice 무거움 vs antiword 가벼움) 는 실제 사용 패턴 보고 결정. 미리 강제 안 함.
- 현재 상태: Active. 아직 .doc 마주친 케이스 없음.

### D11 — Git
- 결정: `git init -b main`. semantic 커밋 (`chore:`, `feat(nssf):` 등).
- 이유: 전역 §9 시맨틱 커밋. wiki 페이지 단위로 의미 있는 단위 커밋 가능.
- 현재 상태: Active.

### D12 — Obsidian
- 결정: Vault 로 그대로 사용. `.obsidian/workspace*`·`cache` 만 gitignore.
- 이유: 플러그인 설정은 다른 머신과 공유 가능, per-user workspace state 분리.
- 현재 상태: Active.

## Consequences

본 ADR 의 결정으로부터 직접 도출된 산출.

- 디렉터리 — `papers/{spec-number-with-dot}/`, `sources/`, `wiki/{nf}/`, `scripts/`, `.venv/` (gitignored).
- 도구 — `scripts/extract.py` (.pdf/.docx/.doc 통합 추출).
- 정책 — frontmatter 에 `source_path`/`source_filename`/`source_format` (포맷 일반화).
- 첫 페이지 — `wiki/nssf/3gpp-ts-29531.md` (ADR 0000 부트스트랩 직후 등록 + ADR 0001 시점에 7 카테고리 재구성 예정).

## Supersession trail

- ADR 0001 (2026-05-09) 이 D2 (Karpathy 패턴) 의 *목적 정의* 부분을 supersede. 본 ADR 의 D2 는 *역사적 출발점* 으로만 유효.
- ADR 0001 의 O1 (디렉터리·프로젝트명) 이 D5 의 *이름* 부분을 supersede 예정 (sprint 0c 에서 적용).

## References

- 출처 — 부트스트랩 시점의 `context-notes.md` (본 ADR 작성 시 root 에서 삭제됨).
- 후속 — `docs/decisions/0001-implementation-grade-redesign.md`.
