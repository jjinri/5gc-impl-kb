# NSSF 자율 코드 생성 첫 tracer-bullet 사이클 — 2026-05-21

Status: in_progress (plan draft)
선행: PR #33~#37 사이클 (ADR-0004 security baseline + architecture + dev + engineering-design re-ratify + history cleanup). `nf-eng-status nssf` `gates.eng_frozen` PASS (8/0) = 자율 코드 생성 GO 신호 (ADR-0002).
Current objective: NSSF 첫 자율 코드 생성 *tracer-bullet* — compile + 1 endpoint (NSSelectionGet) 까지의 vertical slice 산출. 4 source (ADR-0004 baseline + engineering-design + architecture + dev) 합집합을 codegen agent 입력으로 따른다. **본 사이클은 plan 작성까지** — 실 codegen 실행은 plan 승인 후 별 사이클.

## 컨텍스트 — 입력 상태 (검증 완료)

| source | gate | 상태 |
|---|---|---|
| `docs/adr/ADR-0004-project-security-baseline.md` | (정책 source) | merged (#33) |
| `design/nssf/architecture/` (9 doc + ADR-0001) | `/nf-arch-status nssf` arch_consistent | PASS (#34 merged) |
| `dev/nssf/` (impl-plan + tasks + test-matrix + traceability) | `/nf-impl-status nssf` impl_consistent | PASS (#35 merged) |
| `engineering/nssf/engineering-design.md` (13 slot + 8 register) | `/nf-eng-status nssf` eng_frozen | PASS 8/0 (#36 merged) |
| `handoff/nssf/contract.yaml` | `/nf-contract-check nssf` handoff_ready | PASS (local 재생성) |

### 핵심 적용 결정 (engineering-design 발췌)

| 영역 | 결정 |
|---|---|
| 언어/런타임 | C11 / native POSIX |
| 빌드 | CMake, single project, src/<module>/ + src/transport/ + src/common/ |
| HTTP/2 | nghttp2 server+client |
| TLS | OpenSSL app_library backend (mode=enabled, configurable) |
| OAuth2 inbound | libjwt + JWKS (configurable) |
| OAuth2 outbound | NRF token endpoint + cache + attach (configurable) |
| Persistence | PostgreSQL via libpq, 3 table schema |
| Codegen | openapi-generator(C) + cJSON pinned/vendored |
| Config | inih (INI) + env override + secret ref-only |
| Test | Unity (vendored), CMake CTest |
| Lint/static | clang-tidy + clang-analyzer + -Werror |

## Scope

### 포함 범위 (tracer-bullet)

1. **build harness** — CMake skeleton, system/container externalized 의존 (nghttp2, OpenSSL, libpq, libjwt, prometheus-client-c) + vendored (cJSON, inih, Unity).
2. **openapi-generator 통합** — `specs/29.531/TS29531_Nnssf_NSSelection.yaml` 만 generated/ 출력 (NSSAIAvailability 는 후속). generated artifact repo commit + CI drift gate.
3. **transport skeleton (nssf-transport-server task 부분)** — nghttp2 server bind, h2c listen (dev profile), TLS context init code path 존재 (config 로 disable). 1 endpoint route: `/nnssf-nsselection/v2/network-slice-information`.
4. **inbound security stub** — TLS / mTLS / OAuth2 bearer validation code path 의 *stub function* (실 검증 logic 은 후속). config 로 enable/disable.
5. **NSSelectionGet route stub** — request parse → ProblemDetails (501 NOT_IMPLEMENTED) 응답. business logic 미구현 (SelectionEngine 후속).
6. **PostgreSQL schema migration** — `nssf_availability` table 1 종만 (subscriptions / retry queue 후속). versioned SQL migration script + 기동 시 schema_version 검사.
7. **contract smoke test** — Unity test 1~2 case: (a) build 가 통과 (b) NSSelectionGet stub 호출이 501 ProblemDetails JSON 반환.
8. **CI gate skeleton** — build + test + lint + analyze + openapi-generator drift check.

### 제외 범위 (별 사이클)

- 실 SelectionEngine 알고리즘 (`allowed = requested ∩ subscribed ∩ policy ∩ availability`).
- AvailabilityEngine / SubscriptionStore / NotificationDispatcher 구현.
- NSSAIAvailability 7 operation.
- outbound NRF discovery / AMF callback.
- 2 / 3 / 5 ~ 7 의 *실 구현* (stub 만). production-capable code path 의 *형태* 는 보유, *내부 logic* 은 stub.
- contract test 의 full 8 operation matrix.
- soak / load test.
- 배포 (container image / k8s manifest).

## Phases

| Phase | 목표 | 주요 task / acceptance |
|---|---|---|
| P1 | repo bootstrap — CMake + 의존 manifest | CMakeLists.txt root, src/{transport,common,selection_engine}/CMakeLists.txt, vendored deps (cJSON / inih / Unity) sources commit, system dep find_package (nghttp2/OpenSSL/libpq/libjwt/prometheus-client-c). build 가 빈 main() 으로 통과. |
| P2 | openapi-generator 통합 | scripts/codegen.sh — openapi-generator-cli (C) 호출, `specs/29.531/TS29531_Nnssf_NSSelection.yaml` → generated/ 출력. CI drift check (재생성 → diff 0). generated 산출 repo commit. |
| P3 | transport skeleton | nghttp2 server bind + accept loop (epoll). h2c listen `:8080` (dev default). TLS context init (cert/key/CA load via OpenSSL) 함수 stub — config `tls.enabled=false` default. 1 route `/nnssf-nsselection/v2/network-slice-information` GET handler stub. |
| P4 | inbound security stub | mTLS peer verify hook (config `mtls.enabled=false` default), OAuth2 bearer validation hook (config `oauth2_inbound.enabled=false` default). production code path 형태 보유, 실 검증 logic 은 후속. |
| P5 | NSSelectionGet route stub | generated type (`AuthorizedNetworkSliceInfo` 등) 사용, request parse → 501 NOT_IMPLEMENTED ProblemDetails (application/problem+json) 응답. correlation-id 헤더 추출 + log. |
| P6 | PostgreSQL schema migration | scripts/migrate.sh + sql/0001_init_availability.sql (`nssf_availability` table only). libpq connection 기동 시 `nssf_schema_version` table 검사·불일치 시 기동 거부. |
| P7 | contract smoke test + CI gate | tests/test_nsselection_stub.c (Unity) — (a) NSSelectionGet 호출이 501 ProblemDetails JSON 반환 (b) cJSON parse 가능. CI = build + ctest + clang-tidy + clang-analyzer + codegen drift check. 모두 PASS. |

phase 의존성 — P1 → P2 → P3 → P4·P5 → P6·P7 (P6/P7 병렬). P5 가 P2 generated type 의존, P3 transport 의존.

## Acceptance gates (tracer-bullet 완료 조건)

1. `cmake -B build && cmake --build build` 가 warning 없이 통과 (`-Werror`).
2. `ctest --output-on-failure` PASS.
3. clang-tidy + clang-analyzer 가 새 경고 0.
4. codegen drift CI check PASS (`scripts/codegen.sh && git diff --exit-code generated/`).
5. NSSelectionGet stub 이 valid ProblemDetails JSON 반환 (Unity test).
6. PostgreSQL migration 가 빈 DB 에 적용·기동 통과.
7. config `tls.enabled` / `mtls.enabled` / `oauth2_inbound.enabled` / `oauth2_outbound.enabled` 4 capability 의 code path 가 *형태 존재* (stub 호출 site 로 검증).

## Risks

- **openapi-generator C support 성숙도** — C generator 가 일부 OpenAPI 패턴 (예 oneOf, allOf) 에서 약함. NSSelection.yaml 내 schema 가 한계 case 면 codegen 부분 실패 가능. mitigation — P2 acceptance 에서 generated 산출 직접 검토, fallback 으로 단순 schema 만 generated + 복잡 case 는 hand-written wrapper.
- **system dep externalized 의존 폭** — nghttp2 + OpenSSL + libpq + libjwt + prometheus-client-c = 5 system pkg. CI / dev container Dockerfile 의 의존 install 가 필요. mitigation — Dockerfile 명시 (이번 사이클 scope 외, 후속).
- **libjwt + OpenSSL ABI 호환** — libjwt 가 OpenSSL major version 변경에 민감. mitigation — engineering-design `tls_dependency` 의 version_policy=externalized 와 일관, container 에서 양쪽 같은 OpenSSL major 로 고정.
- **PostgreSQL 의 dev 환경 요구** — CI / dev container 가 PostgreSQL 인스턴스 필요. mitigation — testcontainers 또는 ephemeral postgres binary (이번 사이클 scope 외, P6 acceptance 는 migration script 자체만 검증).
- **contract test 의 시작 작은 표면** — 1 endpoint stub 1~2 case 만 — 실 ration 매트릭스가 미적용. mitigation — 의도된 tracer-bullet 범위. 실 매트릭스는 후속 사이클.
- **codegen agent 의 source 합집합 해석** — 4 source (ADR-0004 / eng-design / arch / dev) 에서 충돌이 있으면 ADR-0004 precedence (ADR-0004 §Source precedence). codegen agent prompt 에 precedence 명시 필요. mitigation — codegen 사이클 prompt 에 명시 (별 plan).

## Layout (예상)

```
.
├── CMakeLists.txt
├── cmake/                       # toolchain helpers
├── src/
│   ├── transport/               # nghttp2 server + TLS + mTLS + OAuth2 stub
│   ├── common/                  # ProblemDetails mapper, observability, config (inih)
│   ├── selection_engine/        # P5: handler stub
│   ├── availability_engine/     # (placeholder, 후속)
│   ├── subscription_store/      # (placeholder)
│   └── notification_dispatcher/ # (placeholder)
├── generated/                   # P2: openapi-generator(C) 산출, repo commit
├── vendored/                    # P1: cJSON / inih / Unity sources commit
├── sql/                         # P6: migration scripts
├── scripts/                     # codegen.sh / migrate.sh
├── tests/
│   └── test_nsselection_stub.c  # P7: Unity smoke test
├── ci/                          # CI workflow definition
└── configs/
    └── nssf.dev.ini             # dev profile (tls.enabled=false 등)
```

## 종결 조건

1. 본 plan 자체가 PR review + merge → status: closed (codegen 실 실행은 별 사이클).
2. plan 의 P1~P7 phase 가 합리적 vertical slice 인지 Pane 2 second-opinion.
3. acceptance gates 7 항목 가 tracer-bullet 의도와 일치 — *형태 존재 + smoke pass* 까지, 실 구현 미요구.
4. risk 항목이 cover 됐는지 (Pane 2 검토).

## Pane 2 second-opinion (pending)

본 plan draft 후 Pane 2 검토 요청 — 특히:
- 4 source precedence 명시 위치 (codegen prompt vs plan 본문).
- openapi-generator C 의 schema 한계 mitigation 적절성.
- PostgreSQL 의 dev/CI 인스턴스 처리 (이번 사이클 scope vs 후속).
- contract smoke test 가 너무 작은가 (1 endpoint 1~2 case).

## 후속 사이클 (본 plan 종결 후)

- **codegen 실 실행 사이클** — 본 plan 입력으로 codegen agent 가 P1~P7 산출.
- **NSSAIAvailability 4 op + Subscribe 3 op 구현 사이클** — tracer-bullet 후 횡 확장.
- **outbound NotificationDispatcher + NRF discovery 사이클** — outbound 시퀀스.
- **production CI/CD + container image + k8s manifest 사이클** — 배포.
- **profile v3 강화 사이클** (ADR-0004 Deferred follow-up) — engineering-core-slots typed shape level enforcement.

## References

- `docs/adr/ADR-0002-engineering-design-freeze.md` — `eng_frozen` 게이트 = codegen GO 신호.
- `docs/adr/ADR-0004-project-security-baseline.md` — TLS / mTLS / OAuth2 production-capable code path 의무.
- `engineering/nssf/engineering-design.md` — 13 slot + 8 register frozen 결정 (적용 lib/구조).
- `design/nssf/architecture/` — 9 doc + ADR-0001 (security flow 반영).
- `dev/nssf/{implementation-plan,tasks,test-matrix,traceability}.md` — task / test 분해.
- `handoff/nssf/contract.yaml` + `design/nssf/contract/data-model/` — 8 op contract, generated type 입력.
- `ONBOARDING.md` — lifecycle 흐름, ★eng_frozen PASS — 자율 코드 생성 GO★ 게이트.
- `docs/artifact-management.md` — repo 파일 7 class 관리 기준.
