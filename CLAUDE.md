# 5gc-impl-kb — 정책

본 KB 의 *정책 only* 문서.

> **Language policy.**
> - **design 본문(prose) 은 한국어**로 작성한다.
> - YAML frontmatter, 섹션 헤더(`## Interface` 등), 3GPP 용어·약어(NSSF, AMF, SUCI, S-NSSAI 등) 는 영어 원문 유지.
> - 스펙 정의 문구는 영어 원문 인용 + 한국어 해설 병기.
> - 대화·커밋 메시지는 한국어 그대로.
>
> **Override — 전역 §5 (Korean output).** design 본문에도 적용. 한국어 문장은 `:` 이 아니라 `.`/`?`/`!` 로 끝낸다. 콜론은 코드/key-value/라벨 안에서만.
>
> **Note — 전역 §6 (Korean header comments).** 마크다운/YAML 비대상. `design/scripts/*.py` 등 코드 파일에는 적용.

> **CLAUDE.md 유지 정책.** 본 파일은 *정책* 만 — 절차는 `SKILL.md`, 결정의 *역사적 맥락* 이 필요하면 `git log` 가 진실 출처다. 새 항목 추가 전에 *기존 위치에 흡수 가능한지·다른 곳에 더 어울리는지* 검토. 중복·stale 발견 시 즉시 제거. 본 파일이 증식하면 검색·요약·리뷰 비용이 모두 늘어 정책 문서로서의 가치가 떨어진다. 본 파일을 수정할 때는 *순 변화량* 을 의식한다 — 추가만큼 정리.


---

## Lifecycle vocabulary policy

NF 개발 단계 이름은 [`docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md`](./docs/adr/ADR-0001-nf-lifecycle-and-vocabulary.md) 를 따른다.

- `/nf-init` 은 현재 spec discovery + seed auto-gen 호환 skill 이며 canonical wrapper 는 `/nf-spec-discover` 다. 핵심 목적은 사람이 legacy handoff yaml 을 수동 작성하지 않도록 `_contract_seed.yaml` 을 자동 생성하는 것이다. reset 은 별도 skill 이 아니라 `--reset` 옵션이고 contract 산출물만 archive 한다.
- `/nf-build` 는 code build 가 아니라 contract extraction/generation 단계다. canonical wrapper 는 `/nf-contract-build` 다.
- `/nf-status` 는 contract validation 호환 skill 이며 canonical wrapper 는 `/nf-contract-check` 다. 상세 아키텍처나 구현 검증 status 와 혼동하지 않는다.
- `/nf-arch-design` 은 handoff-ready contract 를 상세 아키텍처 문서로 변환하는 canonical skill 이다. implementation planning 을 자동 호출하지 않는다.
- `/nf-impl-plan` 은 architecture 문서를 구현 작업·테스트·traceability 계획으로 변환하는 canonical skill 이다. 소스 코드나 build system 을 만들지 않는다.
- user-facing lifecycle skill 이 다음 user-facing lifecycle skill 을 자동 호출하지 않는다. 같은 단계의 필수 script/check 만 내부 실행하고, 다음 단계는 추천으로 보고한다.
- `handoff_ready` 는 상세 아키텍처 설계를 시작할 수 있는 contract 입력이 충분하다는 뜻이다. 상세 아키텍처와 구현 계획은 별도 단계다.

## Source-of-truth policy

- Spec 원문은 `specs/` 가 보존한다. `specs/<spec>/` 안에는 3GPP 원본 (`.docx`/`.yaml`/`.pdf`) 만 두며, 그 외 캐시·split·추출 산출 (`_extracted/` 등) 은 git 에서 추적하지 않는다.
- Spec-derived contract 는 `design/<nf>/contract/` 와 `handoff/<nf>/contract.yaml` 에 *로컬 재생성* 한다. git 추적 대상이 아니다 — 도구 (`design/scripts/build-handoff.py` + `/nf-build` SKILL) 가 진실 출처이며, fresh checkout 후엔 `/nf-init` → `/nf-build` 로 재생성한다. Legacy `handoff/<nf>/_handoff.yaml` 는 폐기됐고 새 workflow 의 입력이 아니다.
- `/nf-init` 산출 `design/<nf>/_contract_seed.yaml` 과 `/nf-status` 산출 `_contract_status.yaml`, `_manifest.yaml` 도 같은 정책 — 로컬 재생성, 도구가 진실 출처.
- Architecture design 은 목표 구조상 `design/<nf>/architecture/` 에 둔다.
- Implementation planning 은 `dev/<nf>/` 에 둔다. 이 영역은 계획 산출물 위치이며 소스 코드 작성 시작 신호가 아니다.
- `.omx/` 는 runtime/state 영역이며 cleanup·구조 변경 대상이 아니다.

---

## Git 작업 원칙

Git 작업 원칙은 [`AGENTS.md`](./AGENTS.md) 를 따른다.

- PR/direct push 판단, merge 후 동기화, branch 정리, 검증 요건은 AGENTS.md 기준.
- 여기에는 중복 서술을 두지 않는다.

---

## File Naming Convention

### Original (`specs/`) — 3GPP 원본 보존

```
specs/{spec-number-with-dot}/{original-3gpp-filename}.{ext}
```

- spec-number 폴더는 점 포함 정식 표기 (`29.531`, `23.501`, `29.503`).
- 파일명은 3GPP 다운로드 원본 그대로 (`29531-i40.docx`). release/version 인코딩 (`-{letter}{N}{M}`) 이 이미 들어있어 재명명 불필요.
- 같이 들어있는 OpenAPI yaml (예 `TS29531_Nnssf_NSSelection.yaml`) 도 같은 폴더.

---

## When in doubt, follow rule #1.
