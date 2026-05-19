# `/nf-eng-design nssf` 첫 실행 사이클 — 2026-05-19

Status: closed (2026-05-19)
종결: P1~P5 완료. PR #31 머지 (`4ac102f` — `9f30ec4` plan / `4279a8b` engineering-design freeze). NSSF 20 inventory(13 slot + 7 register) jjinri ratify, `gates.eng_frozen` PASS 8/0 = 자율 코드 생성 GO 달성. Pane 2 second-opinion ×4 (plan/rdbms/full-draft/PR#31) 반영 — SELECT regex blocker·stale 산문·json_runtime semantic gap fix, Lore-trailer finding 기각. 회고 `docs/retros/2026-05-19-nf-eng-design-nssf-firstrun-summary.md`. 후속(별 사이클) — 자율 코드 생성(프로젝트 최종 목표), profile v3 runtime-dep 강제 후보.
Pane 2 second-opinion (codex gpt-5.5, 2026-05-19): 조건부 승인 — 보정 6건 (A→R-arch / embedded-TLS=rejected / oauth2 inbound·outbound 분리 / persistence 단일 backend / register slug 정합 / 산출 worktree 거주) 반영 완료.
Current objective: NSSF 에 대해 `/nf-eng-design nssf` 첫 실행 — 13 core slot + per-NF 연기 레지스터 7행의 engineering decision 을 AI 초안 + slot 성격별 decision-support 로 작성, 사람 ratify 후 `engineering/nssf/engineering-design.md` freeze, `/nf-eng-status nssf` `gates.eng_frozen` PASS 달성. eng_frozen PASS = 자율 코드 생성 GO 신호 (프로젝트 최종 목표의 진입 게이트).

직전 사이클: PR #30 머지 (`4ff0220`) — nf-eng-design SKILL decision-support 를 slot 성격별 이분. 본 사이클은 그 메커니즘의 첫 *실 NF 적용* (메커니즘 재설계 금지, consume 만).

## 컨텍스트 — 입력 상태 (검증 완료)

worktree `nf-eng-design-nssf-firstrun` (origin/main `4ff0220` fresh). `.venv` symlink.

- **contract 재생성 완료** (handoff §2): manifest `ready_for_build:true` `9/9` (manual_overrides.exclude=33.501·38.413 정확 문구 복원·yaml-parse 검증) → seed (13 cat / 25 topic / 8 task) → materialize-contract (25/25 promoted, draft 0) → build-handoff (`handoff/nssf/contract.yaml`) → validate-extraction basic **13/0** → `/nf-contract-check` **handoff_ready PASS** (canonical FAIL=implementation_guidance_quality, eng-design 입력엔 무관).
- **F architecture** (git 추적, 재생성 불요) — `decisions/ADR-0001-architecture-baseline.md` `## Open choices` = 연기 레지스터 7행 (F-hard 입력).
- **profile v2** `design/schemas/engineering-core-slots.yaml` `version:2`, 13 slot, typed shape + dependency_shape closure (read-only).
- **language 분기 = 선택 가능** (사용자 확정 2026-05-19). → language/runtime slot 은 full decision-support (considered≥2·criteria·recommendation·rejected·consequence). C 유력 후보여도 비교 절차 거침. CONTEXT.md/ADR project-constraint 선기록 *불요* (constraint 가 아님 — 본 단계서 선택·ratify).
- **sbi_client_stack.required=true 확정** — `design/nssf/architecture/module-boundaries.md` NotificationDispatcher 행 (availability 변경 → subscription callback URI 로 outbound POST + correlation-id + retry). source_arch_ref 결정론 검사 대상.

## 연기 레지스터 7행 (arch ADR `## Open choices`)

| # | choice | arch status | 본 사이클 처리 방향 |
|---|---|---|---|
| 1 | 구현 언어 / 런타임 | TBD | **선택 가능** → language·runtime slot full decision-support 와 연동 ratify (register 행은 slot 결정 참조·동일 ratify) |
| 2 | persistence backend (subscription, availability) | TBD | persistence slot 과 연동 — contract data model 로 DB/저장 schema 초안 → recommendation ratify |
| 3 | HTTP/2 / TLS / OAuth2 client library | TBD | sbi_client_stack·tls_security·oauth2_token_validation slot 과 연동 — library closure (dependency_shape) ratify |
| 4 | 배포 토폴로지 (service mesh / NF 내장 TLS) | TBD | deployment_topology slot 과 연동 — recommendation ratify |
| 5 | AMF reallocation via RAN (NGAP REROUTE NAS REQUEST) | 보류 | **explicitly_out_of_scope** — 38.413 manifest exclude 정합. rationale=운영정책 미구현 default, consequence=지원 전환 시 38.413 cp+arch hooking, source=module-boundaries/manifest. 사람 ratify |
| 6 | SBA security profile 깊이 (TLS version, cipher) | 보류 | **tls_security mode=external** (33.501 manifest exclude — 운영/배포 정책 외부화). externalized_to=배포(mesh/ingress). 사람 ratify |
| 7 | Subscription store backend default 권고 | 보류 | persistence slot 단일 backend 결정 참조 (추상 아님 — 닫힌 단일값). register 행은 그 결정 참조 ratify. #2 와 중복 아님 (같은 slot 결정을 별 register row 가 참조) |

> **register slug 정합 (Pane 2 보정)** — yaml inventory_id 는 nf-eng-status 가 산출하는 *실 slug* 정확 사용. 특히 #5 = `amf_reallocation_via_ran_지원_ngap_reroute_nas_request` (`지원` 누락 시 row miss → eng_frozen FAIL). P2 서 `/nf-eng-status nssf` 의 `eng_inventory_covered` 미포함 목록으로 7행 정확 slug 확정 후 yaml 작성.
>
> **산출 거주 (Pane 2 P0 해소)** — 모든 산출(plan·engineering-design.md·manifest·seed·contract)은 worktree `.claude/worktrees/nf-eng-design-nssf-firstrun/` 거주. Pane 2 가 repo root 서 검토해 worktree 경로 미발견 보고했으나 동일 내용 검토함 — path artifact, plan 결함 아님.

## 13 core slot — 초안 방향 + decision-support 성격

성격 (SKILL 은 2분류 — 선택가능 vs 고정. R-arch 는 R 의 하위, 독립 아님): **S**=선택 가능(full decision-support: 후보≥2·criteria·rec·rejected·consequence) / **R**=requirement trace(고정·비교 생략, source·consequence·downstream) / **R-arch**=F-architecture-derived requirement trace (sbi_client_stack.required — required 자체는 R, client lib/http2/retry 는 S). [Pane 2 보정: A→R-arch, 독립 성격 아님]

| slot | 성격 | 초안 방향 (ratify 전, 사용자 제시용) |
|---|---|---|
| language | S | 후보 C / Rust / Go / C++. criteria=5GC NF 성능·메모리 제어·생태계·codegen·팀역량. rec 제시 + rejected 사유 + consequence(언어 따라 memory ownership·error handling 모델 강제) |
| runtime | S | language 종속 — native(C/Rust/C++) vs managed(Go GC). language ratify 와 일관 |
| deployment_topology | S | form 후보 = sidecar-mesh / standalone-pod. **embedded-TLS = rejected alternative** (tls_security=external 와 정합 — TLS 외부화 시 NF 내장 TLS 는 실후보 아님, R/S 충돌 회피). rec=mesh 외부화(33.501 externalize 정합) [Pane 2 보정] |
| module_source_layout | S | arch module-boundaries 4 모듈 반영 layout. 단일 repo·모듈 디렉터리 rec |
| sbi_server_stack | S | server_runtime·http2_mode(h2c vs h2)·framework(dependency_shape — language 종속 HTTP/2 server lib) |
| sbi_client_stack | R-arch | required=**true** = R (F-derived, source_arch_ref=module-boundaries.md NotificationDispatcher anchor). targets=AMF callback(NSSAIAvailability 변경 통지)+NRF discovery. http2_mode·timeout_retry·client lib(dependency_shape) = S |
| schema_codegen | S | OpenAPI(TS29531 yaml) → 코드 생성 정책. generated_artifact_policy(commit vs build-time)·tool(dependency_shape, language 종속) |
| tls_security | R | mode=**external** — 33.501 manifest exclude·arch "config 외부화 권고". externalized_to=배포(mesh/ingress). register #6 정합 |
| oauth2_token_validation | R | enabled=**false** — NSSF 자체 OAuth2 scope 정의 없음(manifest 33.501 exclude rationale). rationale=SBA token 정책 배포 외부화. **consequence/source 에 inbound token validation(mesh/gateway 위임) ↔ outbound OAuth2 client credentials(NSSF→NRF/AMF) 분리 명시** [Pane 2 보정 — 혼동 금지] |
| persistence | S | discriminant=backend. arch state-persistence 후보표(in-memory/file/KV/RDBMS) + contract data model(SubscriptionStore·availability map) → DB/저장 schema 초안. **frozen row = 단일 backend 로 닫음** (profile discriminant 단일값. 추상 repo·multi-impl 은 코드 관심사≠frozen 결정). P2 ratify 서 file/kv/rdbms/in_memory 중 1 확정 + 그 variant required_shape 충족. register #2·#7 정합 [Pane 2 보정] |
| telemetry | S | logging/metrics/tracing 각 dependency_shape — arch observability.md 근거, language 종속 lib |
| configuration_management | S | secret_strategy·override_order·config lib(dependency_shape) — arch configuration-strategy.md 근거 |
| test_build_tooling | S | lockfile_policy·test_runner·build_tool·lint_tool·typecheck_tool (각 dependency_shape) — language toolchain 종속 |

## downstream trace (language ratify 후 필수 전파)

language 가 S 로 ratify 되면 그 값이 다수 slot 의 dependency_shape 를 강제한다 — 본 사이클서 일관 반영:

- **C 선택 시**: runtime=native, memory ownership 정책·error handling(errno/return-code)·build/linking(make/cmake)·dependency(시스템 lib·vendoring)·sanitizer(ASan/UBSan) 정책을 language row consequence + 관련 slot(sbi_server/client·schema_codegen·test_build_tooling)에 trace. SKILL `decision support 의무` (2) downstream required decisions 와 동형이나, *선택 가능* 이므로 비교 후 그 귀결로 기록 (requirement trace 아님 — rationale 에 선택 근거 + consequence 에 강제 후속).
- **Rust/Go/C++ 선택 시**: 각 언어 toolchain·메모리 모델·codegen 생태계로 동일 trace.

## 진행 절차 (P-steps)

| P | 산출 | 게이트 |
|---|---|---|
| P1 plan | 본 파일 | Pane 2 second-opinion → 사용자 승인 |
| P2 `/nf-eng-design nssf` | `engineering/nssf/engineering-design.md` (canonical 5 섹션 `## Purpose·Decisions·Out of scope·Open Questions·References` + `## Decisions` 첫 ```yaml 블록 = inventory_id→row). 13 slot typed shape 충족 + 7 register 행. AI 초안 + slot 성격별 decision-support 제시 | 사용자 ratify (`ratified_by`/`date` 모든 frozen row). `## Open Questions` 비어야 함 |
| P3 `/nf-eng-status nssf` | `engineering/nssf/_engineering_status.yaml` (gitignored) | `gates.eng_frozen` **PASS** (blocking 결정론). advisory.impl_plan_alignment 비차단. FAIL 이면 to_pass 보강·재실행 |
| P4 PR | 커밋 분리 (engineering-design.md = 사람소유 산출 / plan·manifest·seed·contract 별 처리. gitignored·.venv 제외) → push 브랜치 → `gh pr create` | — |
| P5 Pane2 | `$git-pr-review <PR#>` (실 본문 `gh pr view` fetch 후 비교) → 반영 → 사용자 머지 지시 → sync (Pane2 codex WIP 검사·자동 rebase 금지) → worktree 정리 → retro + 본 plan close | — |

## 명시적 제외 (scope 밖)

- **메커니즘 재설계 금지** — profile v2 / nf-eng-status.py / SKILL 은 PR #25~#30 완료. consume 만, 수정 금지.
- **구조화 F outbound 산출** (sbi-clients.md 등 ADR-0003 follow-up) = 별 사이클. 본 사이클은 sbi_client_stack row 만.
- **자율 코드 생성** = eng_frozen PASS 후 별 단계 (파이프라인 밖, 프로젝트 최종 목표). 본 plan 범위 아님.
- **contract 재추출 반복** — 이미 handoff_ready PASS. 재실행 불요.
- **Pane 2 (codex `.codex/`) 영역 무접촉** — sync 시 미커밋 작업 있으면 자동 rebase/stash 금지.

## 함정 (직전 세션 학습)

- content 직접 검증 필수 — 지표·gate·요약 맹신 금지 (본 사이클 manifest 가 Read-tool stale view 로 populated 보였으나 yaml-parse 가 빈 overrides 진실 적발 — handoff §60 정확 적중). engineering-design.md 산출도 yaml 블록을 직접 파싱 검증.
- decision-support 단순 단일값 통보 금지 — slot 성격별(S/R/A) 산물 형태 준수. 사람 ratify 의 의미를 비우지 말 것.
- explicitly_out_of_scope (38.413 / 33.501) 도 실질 rationale/consequence/source + 사람 ratify 필수 (빈칸·`보류` 금지).
