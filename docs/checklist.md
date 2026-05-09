# Checklist — LLM Wiki 운영 체크리스트

전역 `~/.claude/CLAUDE.md` §7 (Plan + Checklist + Context Notes) 의무에 따라 유지하는 사용자용 체크리스트. 다음 세션이 처음 들어와도 이 파일만 보면 진행 상황을 파악할 수 있어야 한다.

---

## A. 부트스트랩 (1회성)

- [x] `~/AI/llm-wiki` 디렉터리 구조 생성 (`specs/29.531`, `digests`, `kb/nssf`, `scripts`)
- [x] `.gitignore` 작성 (.venv, .obsidian/workspace*, __pycache__ 등 제외)
- [x] `CLAUDE.md` 작성 (Karpathy 패턴 → 3GPP 5GC 적응)
- [x] `index.md` placeholder 작성
- [x] `scripts/extract.py` 작성 (.pdf/.doc/.docx 통합 추출)
- [x] `docs/checklist.md` (이 파일)
- [x] `git init` + 첫 커밋
- [ ] `sudo apt install python3-venv python3-pip` (사용자 직접 실행 필요)
- [x] `python3 -m venv .venv && .venv/bin/pip install pypdf python-docx`
- [ ] (선택) `.doc` 지원: `sudo apt install libreoffice-core` 또는 `sudo apt install antiword`
- [ ] Obsidian 설치 후 본 폴더를 Vault 로 열기 (시각적 탐색용)
- [x] (신규 머신) `bash scripts/setup.sh` 실행 — venv·pip·extract.py 검증 자동화

## B. NSSF 첫 페이지 등록 (다음 단계)

- [ ] NSSF 관련 3GPP 스펙 PDF/DOC/DOCX 확보 (예: TS 29.531)
- [ ] `specs/29.531/` 에 cp (절대 symlink 금지)
- [ ] `python3 scripts/extract.py specs/29.531/{file}` 로 텍스트 추출 확인
- [ ] `digests/3gpp-ts-29531-v{version}.md` 작성 (한국어 요약, 영문 frontmatter)
- [ ] `kb/nssf/3gpp-ts-29531.md` 작성 (시리즈 페이지, `## Version History` 포함)
- [ ] `index.md` 의 NSSF 섹션에 한 줄 항목 추가
- [ ] semantic 커밋: `feat(nssf): TS 29.531 wiki 페이지 추가`

## C. 새 NF 추가 시 (반복)

- [ ] `kb/{nf}/` 폴더 생성 (소문자 NF 명)
- [ ] `index.md` 에 해당 NF 섹션 추가
- [ ] 위 B 절차를 그 NF 의 첫 spec 에 대해 반복

## D. Cross-cutting 페이지 추가 시 (architecture/interfaces/security/slicing/concepts/overviews/other)

- [ ] 해당 폴더 생성 (`mkdir kb/{category}`)
- [ ] `index.md` 의 해당 카테고리 placeholder 를 페이지 항목으로 갱신
- [ ] 페이지 작성 후 wiki 다른 페이지에서 `[[category/page]]` 로 링크

## E. 운영 규칙 점검 (주기적)

- [ ] CLAUDE.md THE FOUR RULES 위반 없는지 — wiki 외부 추측·web search 인용 금지
- [ ] 한 카테고리 폴더가 ~500 파일을 넘으면 분할 검토
- [ ] `kb/overviews/` 가 정기적으로 늘고 있는가 — knowledge compounding 지표

---

## 미해결 / 보류

- [ ] WSL2 에서 LibreOffice headless 변환 속도 측정 — 느리면 antiword 폴백을 기본으로 쓸지 재검토
- [ ] 3GPP 다운로드 ZIP 자동 풀어서 specs/{spec}/ 로 옮기는 import 스크립트 — NSSF 첫 등록 후 필요성 판단
