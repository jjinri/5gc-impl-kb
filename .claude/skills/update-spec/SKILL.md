---
name: update-spec
description: 본 LLM Wiki(3GPP 5GC) 의 papers/ 디렉터리를 sweep 해서 sources/ 에 매칭이 *없는* 파일(=미등록 신규 spec) 을 모두 찾아 일괄로 wiki 에 등록하는 워크플로우. 사용자가 새 머신에서 git clone 후 papers/ 에 여러 .pdf/.docx/.doc 를 cp 했거나, 평소에 누적된 미등록 파일을 한꺼번에 정리하고 싶을 때 사용한다. 트리거 표현 — "새로 받은 spec 들 다 등록해", "papers 검사", "미등록 wiki 에 올려", "papers/ 비교해서 wiki 갱신", "update wiki", "sweep specs". 단일 파일 등록은 sibling skill `/add-spec` 의 책임이며, 본 skill 은 발견된 각 파일에 대해 add-spec 의 절차(Workflow §2~§7) 를 그대로 위임한다. 본 skill 자체는 (1) sweep + diff (2) 사용자 확인 (3) 파일별 등록 위임 (4) 마지막 통합 보고 의 4단계만 수행한다. 커밋은 절대 자동으로 하지 않는다 — 사용자가 검토 후 별도로.
argument-hint: "(empty) | --yes | <spec-or-nf-filter>"
allowed-tools: Bash(.venv/bin/python3 scripts/extract.py *) Bash(mkdir -p *) Bash(ls *) Bash(find *) Bash(grep *) Bash(awk *) Bash(comm *) Bash(sort *)
---

# update-spec — papers/ sweep 후 미등록 파일 일괄 wiki 등록

## 입력
- `$ARGUMENTS` 는 두 가지만 의미가 있다.
  - 비어 있음 → 모든 미등록 파일을 후보로 본다.
  - `--yes` → 사용자 확인 단계를 건너뛰고 후보 전부 자동 처리.
  - 그 외 토큰 (예: `29.531`, `nssf`) → spec 번호 또는 NF 폴더명 필터로 해석. 후보를 그 범위로 좁힌다.

## 본 skill 의 책임 범위

본 skill 은 *발견·조율* 만 한다. 파일 한 건의 실제 등록 절차는 `.claude/skills/add-spec/SKILL.md` 의 Workflow §2~§7 (메타데이터 추론 / 텍스트 추출 / sources 작성 / wiki 작성 / index.md 갱신 / 결과 보고) 을 *그대로 위임*한다. 절차를 본 파일에 다시 적지 않는다 — 한쪽이 발전하면 다른 쪽도 함께 따라가야 하는 중복을 방지하기 위해서다.

## 절대 규칙
1. CLAUDE.md THE FOUR RULES 를 따른다 (web search 금지, sources/wiki 만 진실, 부족하면 사용자에게 묻기).
2. 절차의 진실 출처는 `add-spec/SKILL.md` 다. 두 skill 간 정책이 어긋나면 add-spec 이 우선.
3. 한 번에 처리할 파일이 많으면 *순서대로* 처리한다 (병렬 금지). 같은 spec 시리즈의 여러 버전이 섞여 있으면 *낮은 버전부터 → 높은 버전* 으로 처리해야 wiki 시리즈 페이지의 `## Version History` 가 자연스럽게 누적된다.
4. 각 파일을 처리할 때 **모호한 메타데이터(spec type / release / version / NF)** 가 있으면 add-spec 의 정책대로 그 파일 처리만 정지하고 사용자에게 묻는다. 다른 파일 처리는 계속 시도하지 않고 일단 멈춘다 — 일괄 모드에서도 추측 금지.
5. 커밋은 자동 수행 금지. 마지막 통합 보고에 제안 commit 메시지만 출력한다.

---

## Workflow

### 1. Sweep — 미등록 파일 식별

원본 집합과 등록 집합의 차이를 구한다.

```bash
# 원본 집합 — papers/ 안의 모든 .pdf/.docx/.doc
find papers -type f \( -iname '*.pdf' -o -iname '*.docx' -o -iname '*.doc' \) \
  | sort > /tmp/_papers_all.txt

# 등록 집합 — sources/*.md 의 frontmatter source_filename
grep -hE '^source_filename:' sources/*.md 2>/dev/null \
  | awk '{print $2}' | sort -u > /tmp/_papers_registered.txt

# 미등록 = basename 차집합 → 다시 full path 로 매핑
while read -r p; do
  bn="$(basename "$p")"
  if ! grep -qxF "$bn" /tmp/_papers_registered.txt; then
    echo "$p"
  fi
done < /tmp/_papers_all.txt > /tmp/_papers_unregistered.txt
```

선택 인자가 있으면 후보 목록을 추가로 좁힌다.
- spec 번호 (`29.531` / `papers/29.531`) → `grep` 으로 path 필터.
- NF 폴더명 (`nssf` 등) → 이미 등록된 같은 NF 의 sources frontmatter `category` 값으로 추론. 명확하지 않으면 사용자에게 확인.

### 2. 후보 확인

미등록 파일이 0개면 "모두 등록됨" 보고 후 정지. 1개 이상이면 다음을 표로 보여준다.

| # | 경로 | 추정 spec | 추정 version | 추정 NF |
|---|------|-----------|---------------|---------|
| 1 | papers/29.531/29531-j60.docx | TS 29.531 | v19.6.0 | nssf |

추정 컬럼은 add-spec/SKILL.md "메타데이터 추론" 의 letter+NM 매핑 + spec→NF 힌트 표를 *경량* 으로 적용한 결과다. 실제 검증은 각 파일 처리 시 add-spec 절차가 다시 한다.

`--yes` 가 없으면 사용자에게 `[Y/n/번호 골라서]` 형태로 처리 범위를 묻는다.
- `Y` → 모두 처리.
- `n` → 정지.
- `1,3,5` 같은 번호 → 해당 항목만.

### 3. 파일별 등록 위임

선택된 각 파일에 대해 순서대로 (낮은 버전 → 높은 버전) 다음을 실행한다.

> add-spec/SKILL.md 의 Workflow §2 (메타데이터 추론) → §3 (텍스트 추출) → §4 (sources 작성) → §5 (wiki 작성/갱신, Version History 누적) → §6 (index.md 갱신) → §7 (파일별 결과 보고).

각 파일이 끝날 때마다 한 줄 요약 (`✓ <stem> registered (sources + wiki + index)` 또는 `⚠ <stem> stopped — 사유`) 을 누적한다. 한 파일이 모호 메타데이터로 정지하면 본 skill 도 정지하고 그 시점까지의 누적을 보고한다.

### 4. 통합 보고

- 처리된 파일 수 / 건너뛴 파일 수 / 실패·정지한 파일 수.
- 새로 만들어진/수정된 파일 목록 (`sources/...`, `wiki/{nf}/...`, `index.md`).
- 새로 생성된 NF 폴더 목록 (있을 때).
- 알려진 한계 (truncate 된 섹션, 추출 미완료 등) — 파일 단위로.
- 제안 commit 메시지 한 줄 (예: `feat(wiki): NSSF·SMF 첫 spec 페이지 batch 등록 (TS 29.531 v19.6.0, TS 29.502 v18.4.0)`).

**커밋은 자동 수행 금지.**

---

## 자주 틀리는 지점 — 사전 점검

- 같은 spec 시리즈의 여러 버전을 처리할 때 *낮은 버전 → 높은 버전* 순서를 지켰는가. 그렇지 않으면 `## Version History` 가 거꾸로 쌓일 수 있다.
- "미등록" 판정 기준이 frontmatter `source_filename` 한 가지 뿐이다. 사용자가 sources 파일을 수동 편집해 frontmatter 를 빠뜨렸다면 오탐(중복 등록 시도) 이 발생할 수 있다 — 그 경우 add-spec §4 의 "이미 있으면 덮어쓸지 확인" 가드가 잡는다.
- `--yes` 모드에서도 모호 메타데이터는 정지가 정답이다. silently 추측해 잘못된 페이지를 만들면 안 된다.
- 일괄 처리 도중 새 NF 폴더가 만들어지면 `index.md` 의 `## 다른 NF (예정)` 안내에서 그 NF 를 빼고 별도 섹션을 만든다 (add-spec 정책과 동일).

## 참고 — 본 skill 안에 다시 적지 말 것

- 단일 파일 등록 절차의 모든 세부 사항은 `add-spec/SKILL.md` 의 Workflow §2~§7 가 진실 출처. 본 skill 은 거기에 위임만 한다.
- 디렉터리·파일명 규칙: `CLAUDE.md` 의 "Repository Structure"·"File Naming Convention"·"Categories".
- 추출 도구의 옵션·폴백: `scripts/extract.py --help`.
