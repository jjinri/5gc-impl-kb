# 2026-05-12 — NSSF design→dev MVP retrospective

상위 spec — `docs/superpowers/specs/2026-05-12-nf-build-restructure-design.md`.
MVP spec — `docs/superpowers/specs/2026-05-12-nf-build-restructure-agent-consumable-mvp.md`.
Agent Proof — `docs/superpowers/specs/agent-proof/2026-05-12-nssf-nsselection.md`.

## 시작 가정

- dev consumer = LLM agent (사람 아님).
- 진실 출처는 `handoff/<nf>/_handoff.yaml` 단일 entry point.
- MVP scope — NSSelection 1 API, 6 토픽 + 2 JSON, 5 활성 카테고리.

## 무엇이 잘 작동했나

- **handoff-v2 schema 단일 entry point.** Agent Proof 의 fresh subagent 가 다른 파일 추측 없이 `_handoff.yaml → agent_contract.default_read_order` 만 따라 7~8 파일을 정확히 읽었다. `default_read_order` 의 순서가 깔끔히 작동 (entry → categories → target topic → depends_on → related → error_refs).
- **AUTO/USER marker 의 frontmatter 동기 검사 (validator basic #7·#8).** 토픽 작성 중 marker ID 와 `generated_sections`/`user_sections` 가 자연히 맞춰진다. mismatch 사례 본 사이클에서 0회 — 사람이 marker 만 쓰면 frontmatter 가 자명.
- **data-model JSON 의 `unresolved_refs` + `_inlined_from` 패턴.** SliceInfoForRegistration / AuthorizedNetworkSliceInfo 모두 transitive deps 가 모두 inline 됐고 (`unresolved=0`) status `canonical` 정합. 본 패턴이 status enum 결정의 객관 근거가 됨.
- **TDD-first tool 개발.** Task 1~3 의 pytest harness + lib (path_resolution, marker_parser) 가 Task 4~8 의 validator/build-handoff/JSON emit 개발을 빠르게 만들었다. failing test → impl 흐름이 코드 backward compat hack 없이 깔끔.
- **`/nf-status` 의 v2-aware demoted set.** v2 NF 가 단일 페이지 가정에서 false-FAIL 하던 check 들이 자동 NOT_APPLICABLE 로 강등 — 사용자가 봐야 할 진실 (`validate_extraction_basic`) 한 줄이 명확히 노출.

## 무엇이 마찰을 일으켰나

- **plan T9 의 demoted set 불완전.** `frontmatter_valid` + `handoff_yaml_valid` 가 demoted 에 빠져있어 T21 단계에서 gate `handoff_ready` 가 FAIL. fix commit (fcc5612) 으로 demoted 확장. 후속 — 매 check 정의 시점에 *"v2 가 부적용인가?"* 를 코드 옆에 라벨로 박는 식으로 검증 의무화.
- **plan T13 의 `_manifest.yaml 보존` 가정 깨짐.** 실제로는 manifest 파일이 gitignored 라 작업 시작 시점에 부재 — T21 단계에서 nf-manifest.py 재호출 + manual_overrides.exclude 작성 필요. plan 이 *현존 가정* 을 점검 step 으로 명시했어야.
- **handoff_yaml_valid 가 v1 schema 하드코딩.** schema_version=`handoff-v1` 외 모두 FAIL — v2 NF 에서는 demoted set 으로 우회했지만 본질적으로는 *schema-aware* check 가 필요 (v1·v2 둘 다 처리).
- **subagent dispatch 시 working directory.** Agent Proof 는 worktree 경로 (`.claude/worktrees/nf-build-restructure`) 에서만 v2 파일을 갖는다. subagent 프롬프트에 *worktree 경로 명시* 가 필수 — 누락 시 main 브랜치의 v1 archive 를 읽을 위험.

## spec 어절 중 약했던 부분

- **§5 의 NSSF category status 매트릭스.** "module-decomposition: status=draft, but SelectionEngine topic=handoff_ready" 같은 의도적 mixed 상태가 plan T20 의 seed 작성에서 사람의 *해석* 을 요구. `status_precedence: topic_over_category` 가 spec §1 에 박혀있지만, *왜 SelectionEngine 만 격상 가능한가* (scope 명시 → MVP 한정 의도) 는 plan 본문에서 다시 해설해야 했다. spec 에 1 예시 추가가 합리적.
- **§6 의 PASS 채점 주체.** "fresh agent 가 작성, plan 실행자가 채점" 인데 *plan 실행자의 self-judge bias* 위험. 본 사이클은 명확한 4 기준이라 통과했지만, 후속에서는 score 가 흔들리는 케이스가 나올 수 있다. 다음 사이클에서 채점 rubric 의 binary boundary 강화 검토.
- **§4.5 path resolution 의 anchor 시맨틱.** `error-handling#nsselection-400` 같은 anchor 가 markdown 본문 안 `<a id="...">` 를 가리키는 규약은 spec §4.5 에 있지만, validator basic #4 가 실제 anchor 가 본문에 존재하는지까지는 검사 *안 한다* (file 존재만 검사). frontend renderer 에서만 깨짐 — strict 룰에서 cover 예정.

## 다음 사이클 권고

- **NSSelectionPost (POST /network-slice-information/subscriptions)** — 더 복잡한 API 로 contract 한계 검증 (spec §8 risk #1). subscription 모델은 stateful 경로라 본 사이클의 stateless 가정이 시험된다.
- **Service Scenarios 카테고리 활성화** — sequence diagram (mermaid) + actor 어절 정합. handoff-v2 에 시퀀스 토픽이 등장하면 agent 의 default_read_order 가 정확히 어디까지 따라오는지 관찰.
- **Cross-NF 카테고리** — NSSF ↔ NRF (NF discovery), NSSF ↔ UDM (subscription) 의 cross-NF op 정의. 본 MVP 의 stub 부분 (NRF, UDM lookup) 을 실제 contract 로 채움.
- **다른 NF (AMF, SMF) 로 확장** — `agent_contract` 가 NF 별로 어떻게 분기되는지 관찰. 같다면 spec §8 risk #4 의 옵션 B (NF별 contract 가 아닌 global contract) 재검토.
- **schema-aware handoff_yaml_valid.** v1·v2 둘 다 검사하도록 `nf-status.py` 의 check 를 갱신 — demoted 우회 줄임. 본 사이클은 fix commit 으로 처리했지만 근본 처치는 후속.

## 메트릭

- 작업 단계 수 — 22 commits (worktree base `4c19b4d` 이후, T1~T23 + T21 fix).
- Agent Proof 결과 — PASS, 기준 4/4. receipt — `docs/superpowers/specs/agent-proof/2026-05-12-nssf-nsselection.md`.
- validate-extraction basic FAIL → fix cycle — 1 라운드 (T21 에서 manifest 부재 + nf-status.py demoted set 누락 두 가지를 한 라운드에 해결).
- 전체 fresh subagent dispatch — 1 회 (Agent Proof).
- Reused 도구 — `resolve-yaml-refs.py --emit-json`, `build-handoff.py`, `validate-extraction.py`, `nf-status.py` 모두 T20 의 한 사이클에서 chain 작동.
