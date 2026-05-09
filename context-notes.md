# Context Notes — LLM Wiki 결정 기록

전역 `~/.claude/CLAUDE.md` §7 의무. 작업 중 내린 결정과 그 근거를 누적 기록. 다음 세션이 "왜 이렇게 됐냐" 묻지 않도록.

날짜는 결정 시점 기준 (KST).

---

## 2026-05-09 — 부트스트랩

### 분야 / 분야명
- **결정**: `3GPP 5G Core Systems (5GC)`. RAN·물리계층은 범위 외.
- **이유**: 사용자가 명시. NSSF 시작 → NF 단위로 확장 계획.

### 베이스 템플릿
- **결정**: [Karpathy LLM Wiki 패턴](https://gist.github.com/karpathy/1dd0294ef9567971c1e4348a90d69285) 의 [joonan30 fork](https://gist.github.com/joonan30/cbce305684d079dbe9a3fbaefe4e3959) 를 출발점으로, 3GPP 도메인에 맞게 변형.
- **이유**: "원본 PDF → sources(LLM 요약) → wiki(정리)" 3-tier 구조와 THE FOUR RULES (no web search, wiki first, re-read source, say-so) 가 hallucination 방지에 잘 맞음.

### 전역 CLAUDE.md 와의 관계 — deduplication
- **결정**: 프로젝트 CLAUDE.md 는 전역과 직접 중복되는 항목이 거의 없으므로, 도메인 고유 부분만 적고 상단에 "Inheritance" 블록으로 전역 규칙 계승을 명시.
- **삭제**: 템플릿 원안의 맨 아래 "Design Principles" 섹션 — Four Rules + 3-tier 헤더와 내부 중복.

### 언어 정책
- **결정**: wiki 본문 prose·sources/*.md·index.md 설명문은 **한국어**. YAML frontmatter, 섹션 헤더, 3GPP 약어, 스펙 정의 인용은 영어 유지.
- **이유**: 사용자가 wiki 를 직접 읽을 것이므로 가독성 우선. 단 약어와 메시지·필드명은 원문 추적성 위해 영어 보존.
- **Override 명시**: 전역 §5 (한국어 마침표) 는 wiki 본문에도 적용. 전역 §6 (한국어 헤더 주석) 은 마크다운/YAML 비대상, `scripts/*.py` 에는 적용 (extract.py 1행 주석 적용 완료).

### Repository 조직 — papers/ 의 spec-number 폴더
- **결정**: `papers/{spec-number-with-dot}/{original-3gpp-filename}` 구조. 폴더명은 점 포함 (`29.531`). 파일명은 3GPP 원본 (`29531-i40.docx`) 그대로.
- **이유**: 3GPP 다운로드는 spec 시리즈 단위로 묶여 오고, 같은 시리즈에 여러 release/version 이 공존함. nested 가 자연스러움. 점 포함은 사람 가독성 우선.
- **트레이드오프**: Karpathy 의 평면 papers/ 가정에서 벗어남. 다음 항목에서 매핑 분기 처리.

### Karpathy 1:1:1 매핑 분기
- **결정**:
  - papers — spec 버전마다 1파일 (멀티 포맷·멀티 릴리즈 공존)
  - sources — 실제 요약한 버전마다 1개 (`sources/3gpp-ts-{n}-v{ver}.md`)
  - wiki — spec 시리즈마다 1개 (`wiki/{nf}/3gpp-ts-{n}.md`), 본문에 `## Version History`
- **이유**: 자연스러운 사용자 질문 ("TS 29.503 가 뭐냐") 에 한 페이지로 답할 수 있어야 함. 버전별로 페이지가 N개면 cross-reference 가 폭발.

### 멀티 포맷 지원 — .pdf/.doc/.docx
- **결정**: `scripts/extract.py` 단일 헬퍼에서 확장자 분기. `.pdf`→pypdf, `.docx`→python-docx, `.doc`→soffice 변환 후 python-docx (폴백: antiword).
- **이유**: 3GPP 는 전통적으로 .doc 로 배포됨. PDF 만 지원하면 워크플로우 깨짐.
- **YAML 필드 변경**: 원안의 `pdf_path`/`pdf_filename` → `source_path`/`source_filename`/`source_format` 으로 일반화. 원안 보존성보다 정확성 우선.

### NF 폴더 — 사전 생성 안 함
- **결정**: `wiki/nssf/` 만 만들고, 다른 NF 폴더는 첫 문서 추가 시점에 생성.
- **이유**: 전역 §2 (단순성, 사변적 셋업 금지). 빈 폴더 14개 미리 만들면 깨끗함만 해침.

### 의존성 관리 — venv
- **결정**: 시스템 pip 대신 프로젝트 `.venv/` 사용. `.gitignore` 에 `.venv/` 포함.
- **이유**: WSL2 Ubuntu 의 PEP 668 정책으로 시스템 Python 에 직접 설치 불가. venv 가 표준.
- **현황**: 시스템에 `python3-venv`/`python3-pip` 패키지 자체가 미설치. 사용자가 `sudo apt install python3-venv python3-pip` 수동 실행 필요.

### `.doc` 지원 도구
- **현황**: soffice (LibreOffice), antiword 모두 미설치.
- **결정**: extract.py 가 .doc 입력 시 친절한 안내 메시지 출력. 사용자가 첫 .doc 마주칠 때 설치하도록.
- **이유**: 어느 도구를 쓸지(LibreOffice 무거움 vs antiword 가벼움)는 실제 사용 패턴 보고 결정. 미리 강제 설치하지 않음.

### Git
- **결정**: `git init -b main`. semantic 커밋 (`chore:`, `feat(nssf):` 등).
- **이유**: 전역 §9 시맨틱 커밋. wiki 페이지 단위로 의미 있는 단위 커밋 가능.

### Obsidian
- **결정**: Vault 로 그대로 사용. `.obsidian/workspace*`·`cache` 만 gitignore.
- **이유**: 플러그인 설정은 다른 머신과 공유할 수 있으나 per-user workspace state 는 분리.

---

## 결정 변경 / 정정 시

이 파일에 새 날짜로 항목 추가하고, 이전 결정 항목에 `~~취소선~~` + "→ {새 결정 위치 링크}" 표시. 삭제 금지 — 이력 자체가 컨텍스트.
