---
nf: nssf
stage: engineering-design-freeze
adr: docs/adr/ADR-0002-engineering-design-freeze.md
profile: design/schemas/engineering-core-slots.yaml
inputs:
  - design/nssf/architecture/
  - design/nssf/architecture/decisions/ADR-0001-architecture-baseline.md
  - handoff/nssf/contract.yaml
---

# NSSF — Engineering Design Freeze

## Purpose

자율 코드 생성(프로젝트 최종 목표) 진입 전, NSSF 의 사람 소유 엔지니어링 결정을 freeze 한다. F architecture(language-neutral)·G·contract 를 근거로 13 core slot + per-NF 연기 레지스터 7행을 *AI 초안* 으로 작성하고, 각 행을 사람이 ratify(`ratified_by`/`date`) 해야 `eng_frozen` PASS = GO 신호다. ratify 전 행은 frozen 아님.

본 사이클 2 gating 결정 — language=C(선택가능 decision-support 후 사용자 선택), persistence=rdbms/PostgreSQL(단일 backend 로 닫음, Pane 2 교차검토 합의) — 이 나머지 dependency slot 을 강제한다.

## Decisions

아래 `inventory_id: {row}` 매핑이 기계 계약(validator 파싱 대상). 20행 모두 **jjinri / 2026-05-19 ratify 완료 — frozen 상태** (`nf-eng-status` `gates.eng_frozen` PASS). 결정 변경 시 해당 행 재-ratify(`ratified_by`/`date` 갱신) 필요.

```yaml
language:
  decision: "구현 언어 = C (C11)"
  status: decided
  name: c
  version: c11
  rationale: "선택가능 decision (사용자 비개발자 전제 decision-support). considered=[C, Rust, Go, C++]. criteria=성능·메모리제어·HTTP/2 SBI 생태계·OpenAPI codegen·자율코드생성 적합성. 사용자 선택=C (최대 제어·전통 5GC). rejected — Rust(GC 없는 안전성 강하나 학습곡선·C codegen 생태계 약, 프로젝트 C 선호), Go(개발속도 좋으나 GC latency jitter·메모리 제어 약), C++(복잡도·safety footgun)."
  consequence: "downstream 강제 — 수동 memory ownership 정책, errno/return-code error handling, CMake build/linking, system/container externalized dependency(vendoring 최소), ASan/UBSan sanitizer 정책 필수. runtime=native. 모든 dependency_shape slot 이 C 라이브러리로 닫힘."
  source: "decision-support 제시 후 사용자 ratify. F architecture 는 language-neutral."
  ratified_by: "jjinri"
  date: "2026-05-19"

runtime:
  decision: "runtime = native POSIX 프로세스 (managed runtime/VM 없음)"
  status: decided
  name: native-posix
  version: posix-2017
  rationale: "language=C 종속 — native 외 후보 부재(managed runtime 은 C 와 무관). language slot consequence 의 직접 귀결."
  consequence: "단일 native 바이너리. GC 없음 — latency 예측가능. 메모리/리소스 수명 수동 관리."
  source: "language=C ratify 의 귀결 (본 문서 language slot)."
  ratified_by: "jjinri"
  date: "2026-05-19"

deployment_topology:
  decision: "배포 = standalone container pod + service-mesh sidecar (mesh 가 TLS 종단·inbound authN)"
  status: decided
  form: "standalone container pod behind service-mesh sidecar; single native binary; TLS·inbound-auth externalized to mesh"
  rationale: "선택가능. considered=[mesh-sidecar standalone, embedded-TLS NF]. embedded-TLS=rejected (tls_security=external 와 충돌 — TLS 외부화 시 NF 내장 TLS 실후보 아님). mesh 외부화가 33.501 SBA security 외부화 권고와 정합."
  consequence: "NF 는 h2c(cleartext HTTP/2) listen, mesh sidecar 가 TLS·peer authN. NF 바이너리 TLS stack 불요 — 의존 축소."
  source: "design/nssf/architecture/module-boundaries.md; design/nssf/_manifest.yaml manual_overrides.exclude 33.501."
  ratified_by: "jjinri"
  date: "2026-05-19"

module_source_layout:
  decision: "단일 CMake project, arch 4 모듈별 src/ 디렉터리"
  status: decided
  layout: "single CMake project; src/<module>/ per arch 4 모듈 (selection_engine, availability_engine, subscription_store, notification_dispatcher) + src/transport/ + src/common/; tests/ mirrors src/; generated/ = OpenAPI codegen 출력"
  rationale: "선택가능. arch module-boundaries.md 4 모듈 경계 반영. 단일 repo·모듈 디렉터리·CMake target per module 권고. rejected — flat single-dir(모듈 경계 흐려짐)."
  consequence: "모듈별 컴파일 단위·테스트 격리. arch 경계가 디렉터리로 enforce."
  source: "design/nssf/architecture/module-boundaries.md"
  ratified_by: "jjinri"
  date: "2026-05-19"

sbi_server_stack:
  decision: "SBI server = nghttp2 기반 h2c, epoll 단일 프로세스"
  status: decided
  server_runtime: "single-process event loop (epoll), C native"
  http2_mode: "h2c (cleartext HTTP/2 prior-knowledge; TLS 는 mesh sidecar 종단)"
  framework:
    dependency_source: third_party
    version_policy: externalized
    package: nghttp2
  rationale: "선택가능. C HTTP/2 server 후보=[nghttp2, libh2o, custom]. nghttp2=성숙·3GPP SBI 적합·표준 추종. rejected — libh2o(통합 복잡), custom(재발명 위험). h2c 는 deployment_topology mesh-TLS 외부화 귀결."
  consequence: "nghttp2 system/container package(externalized) 의존. TLS 미내장 — mesh 필수. HTTP/2 prior-knowledge cleartext."
  source: "design/nssf/architecture/runtime-model.md; deployment_topology 결정."
  ratified_by: "jjinri"
  date: "2026-05-19"

sbi_client_stack:
  decision: "outbound SBI client 필요 (NotificationDispatcher→AMF callback, NRF discovery). nghttp2 client, mesh egress."
  status: decided
  required: true
  source_arch_ref: "design/nssf/architecture/module-boundaries.md#boundaries"
  targets:
    - "AMF — NSSAIAvailability 변경 통지 subscription callback URI (outbound POST)"
    - "NRF — NF discovery"
  http2_mode: "h2 to peer via mesh egress (TLS externalized to sidecar); NF emits h2c, sidecar upgrades"
  timeout_retry_policy: "connect 2s / request 5s; notification retry exponential backoff base 1s cap 60s max 10 attempts, persisted in nssf_notification_retry_queue (status+next_attempt_at, row-lock dequeue via FOR UPDATE SKIP LOCKED)"
  client:
    dependency_source: third_party
    version_policy: externalized
    package: nghttp2
  rationale: "required=true 는 R/F-derived requirement trace — arch NotificationDispatcher 가 availability 변경을 subscription callback URI 로 outbound POST. client lib/http2/retry 는 S — nghttp2 선택(sbi_server 와 동일 stack 재사용)."
  consequence: "retry queue 가 persistence(rdbms) 와 동일 backend → subscription 변경+enqueue 1 트랜잭션(arch Open Q 해소). mesh egress 가 outbound TLS·peer authN."
  source: "design/nssf/architecture/module-boundaries.md#boundaries (NotificationDispatcher row)."
  ratified_by: "jjinri"
  date: "2026-05-19"

schema_codegen:
  decision: "OpenAPI codegen = openapi-generator(C), 생성물 repo commit + CI drift gate"
  status: decided
  generated_artifact_policy: "TS29531 OpenAPI → C struct/serializer 생성물 repo commit; CI 가 재생성 drift 검사"
  tool:
    dependency_source: third_party
    version_policy: pinned
    package: openapi-generator
  json_runtime:
    dependency_source: third_party
    version_policy: pinned
    package: cJSON
  rationale: "선택가능. (a) codegen tool 후보=[openapi-generator, custom datamodel]. openapi-generator=다언어 성숙·C 지원. commit 정책(vs build-time-only)=재현성·리뷰가능. rejected — custom(유지부담), build-time-only(추출 비결정 위험). (b) json_runtime — NSSF 는 SBI application/json·application/problem+json·JSON Patch(PatchDocument: NSSAIAvailabilityPatch/SubModifyPatch)·PostgreSQL JSONB payload/filter·openapi-generator 생성 C model parse/serialize 때문에 runtime JSON 라이브러리 필수(용도=직렬화/역직렬화). 후보=[cJSON, jansson, yyjson]. criteria=vendoring 가능성·의존 폭·API 단순성·기존 정책 일관. 추천 cJSON(초경량 vendored source commit — inih/Unity vendoring 패턴 일관, 외부 system dep 0, API 단순). rejected — jansson(system externalized dep 추가, 닫힘 폭 증가), yyjson(속도 우위나 단순성·보편성·vendoring 일관서 cJSON)."
  consequence: "openapi-generator pinned 버전 의존(빌드환경). 생성물 PR 리뷰 대상. spec 변경 시 재생성+drift fail. json_runtime=cJSON pinned/vendored(소스 commit) — 코드 생성 agent 는 임의 JSON 라이브러리 도입 금지(cJSON 으로 고정), 생성 C model·SBI body·JSON Patch·JSONB 직렬화 전부 cJSON 사용."
  source: "handoff/nssf/contract.yaml (TS29531 OpenAPI); design/nssf/architecture/."
  ratified_by: "jjinri"
  date: "2026-05-19"

tls_security:
  decision: "TLS = 외부화 (mesh sidecar 종단). NF 내 TLS stack 미구현."
  status: decided
  mode: external
  externalized_to: "service-mesh sidecar (mTLS 종단·peer authN at mesh)"
  rationale: "R requirement trace. 33.501 manifest exclude — SBA TLS·token 정책 운영/배포 정책으로 외부화, 구현 깊이 0. arch 'config 외부화 권고'. 비교 생략(외부 고정)."
  consequence: "NF 는 h2c. TLS version/cipher 는 mesh 정책. NF 바이너리 TLS 의존 0. NF 내장 TLS 전환 시 본 행 재결정 + tls_provider/tls_dependency 추가."
  source: "design/nssf/_manifest.yaml manual_overrides.exclude 33.501; design/nssf/architecture/decisions/ADR-0001-architecture-baseline.md ## Open choices."
  ratified_by: "jjinri"
  date: "2026-05-19"

oauth2_token_validation:
  decision: "inbound OAuth2 token validation = NF 미구현 (mesh/gateway 외부화)"
  status: decided
  enabled: false
  rationale: "NSSF 자체 OAuth2 scope 정의 없음(manifest 33.501 exclude). inbound access-token validation 은 mesh/gateway 위임(외부화). outbound OAuth2 client credentials(NSSF→NRF token endpoint, NotificationDispatcher POST token 부착)는 별개 — arch configuration-strategy.md oauth2.enabled=false default, 구현은 sbi_client_stack/configuration 영역. 본 slot 은 inbound validation 만."
  consequence: "NF 는 access token 검증 안 함 — mesh/gateway 가 보장. outbound client-credentials 는 configuration_management 의 oauth2.* 옵션, default off — enable 은 별도 결정·구현 범위(token endpoint 호출·secret 주입)로 본 frozen 결정 밖. inbound 검증 NF 내재화 전환 시 enabled=true + token_validation_strategy + lib (본 행 재결정)."
  source: "design/nssf/_manifest.yaml manual_overrides.exclude 33.501; design/nssf/architecture/configuration-strategy.md."
  ratified_by: "jjinri"
  date: "2026-05-19"

persistence:
  decision: "persistence = PostgreSQL(RDBMS) 단일 backend, libpq 드라이버"
  status: decided
  backend: rdbms
  tables: "nssf_subscriptions, nssf_availability, nssf_notification_retry_queue"
  columns: "nssf_subscriptions(subscription_id PK, callback_uri, event, filter_json JSONB, tai_filter_json JSONB, amf_id, amf_set_id, expiry timestamptz, supported_features, etag_version int, created_at, updated_at); nssf_availability(tai_mcc, tai_mnc, tai_tac, tai_nid, amf_set_id, availability_json JSONB, supported_snssai_json JSONB, etag_version int, updated_at); nssf_notification_retry_queue(id PK, subscription_id FK, callback_uri, payload_json JSONB, correlation_id, status, attempt_count int, next_attempt_at timestamptz, locked_by, locked_until timestamptz, last_error, created_at, updated_at)"
  primary_key: "nssf_subscriptions: subscription_id; nssf_availability: (tai_mcc,tai_mnc,tai_tac,tai_nid); nssf_notification_retry_queue: id"
  indexes: "nssf_notification_retry_queue(status,next_attempt_at); nssf_subscriptions(expiry); nssf_availability(amf_set_id); JSONB GIN on filter_json/availability_json (query 필요 시)"
  constraints: "FK nssf_notification_retry_queue.subscription_id→nssf_subscriptions.subscription_id ON DELETE CASCADE; CHECK status IN ('pending','inflight','done','failed'); CHECK attempt_count>=0; expiry NULL = non-expiring 정책"
  migration_tool: "repo-managed versioned SQL migration scripts"
  migration_policy: "forward-only versioned SQL; 기동 시 schema_version 검사·불일치 시 기동 거부"
  driver_or_orm:
    dependency_source: third_party
    version_policy: externalized
    package: libpq
  rationale: "선택가능, 단일 backend 로 닫음(Pane 2 교차검토 합의). 후보=[rdbms, kv, file, in_memory]. persistent 상태 3종(subscription/availability/retry-queue) 모두 lifecycle·query·트랜잭션 필요 → rdbms. 같은 backend 로 subscription 변경+retry enqueue 1 트랜잭션(arch Open Q 해소). rejected — in_memory(restart 손실), file(scale-out 불가), kv(복잡 query·트랜잭션 약). token cache·conn pool 은 runtime cache(persistence backend 아님)."
  consequence: "PostgreSQL 운영 의존. libpq system/container externalized. core key column화 + spec payload/filter JSONB 병행(JSONB-only 금지, Pane 2 보정). retry queue = table-as-queue (row-lock dequeue via FOR UPDATE SKIP LOCKED)."
  source: "design/nssf/architecture/state-persistence.md; design/nssf/contract/data-model/{NssfEventSubscriptionCreateData,NssfEventSubscriptionCreatedData,NssaiAvailabilityInfo,Tai,Snssai,PlmnId}."
  ratified_by: "jjinri"
  date: "2026-05-19"

telemetry:
  decision: "logging=stdlib structured, metrics=prometheus-client-c, tracing=W3C header 전파(stdlib); exporter/sink 외부화"
  status: decided
  logging:
    dependency_source: stdlib
    version_policy: runtime_managed
  metrics:
    dependency_source: third_party
    version_policy: externalized
    package: prometheus-client-c
  tracing:
    dependency_source: stdlib
    version_policy: runtime_managed
  rationale: "선택가능. arch observability.md 가 log/metric/trace signal·correlation 정의, exporter 구체는 dev. C 최소의존 — logging stdlib(syslog/stderr JSON), metrics prometheus-client-c(/metrics 노출, scrape externalized), tracing 은 traceparent + 3gpp-Sbi-Correlation-Info 헤더 전파만(stdlib), 분산 trace SDK/exporter 외부화. rejected — 무거운 OTel C SDK(의존 과대)."
  consequence: "metrics prometheus-client-c externalized 의존. 분산 tracing 은 헤더 전파만 — full span export 는 외부 collector. correlation_id 전 signal 부착."
  source: "design/nssf/architecture/observability.md"
  ratified_by: "jjinri"
  date: "2026-05-19"

configuration_management:
  decision: "config = inih(INI) 파서 + env override; secret 외부화(ref-only)"
  status: decided
  secret_strategy: "secret 미저장 — oauth2.client_secret_ref 등 ref 만 보유, 실제 secret 은 배포(env/vault) 외부 주입"
  override_order: "compiled defaults < config file < environment variables (env 최우선)"
  config:
    dependency_source: third_party
    version_policy: pinned
    package: inih
  rationale: "선택가능. C config 후보=[inih vendored, libconfig, stdlib getenv-only]. inih=초경량 vendoring·INI 단순. override 순서 defaults<file<env. secret 은 ref-only(arch: 형식 강제 안 함). rejected — libconfig(과대), getenv-only(파일 config 부재)."
  consequence: "inih pinned vendored. secret 평문 미보유 — 배포가 env/vault 주입. config 누락 시 compiled default."
  source: "design/nssf/architecture/configuration-strategy.md"
  ratified_by: "jjinri"
  date: "2026-05-19"

test_build_tooling:
  decision: "build=CMake, test=Unity, lint=clang-tidy, static-analysis=clang-analyzer; 의존 container-pinned/vendored"
  status: decided
  lockfile_policy: "C native lockfile 없음 — 의존은 container image 에 pinned(system pkg 버전 고정), 소형 vendored lib(inih/Unity) 는 소스 commit"
  test_runner:
    dependency_source: third_party
    version_policy: pinned
    package: Unity
  build_tool:
    dependency_source: third_party
    version_policy: externalized
    package: cmake
  lint_tool:
    dependency_source: third_party
    version_policy: externalized
    package: clang-tidy
  typecheck_tool:
    dependency_source: third_party
    version_policy: externalized
    package: clang-analyzer
  rationale: "선택가능, language=C 종속. test 후보=[Unity, Criterion, CMocka] → Unity(경량 vendoring). build=CMake(표준). lint=clang-tidy. C 별도 typecheck 없음 — compiler -Werror + clang static analyzer. lockfile=container pin(C 무 native lockfile)."
  consequence: "CMake/clang toolchain container externalized. Unity 소스 commit. CI 가 build+test+lint+analyze gate."
  source: "language=C 결정; repo CI 패턴."
  ratified_by: "jjinri"
  date: "2026-05-19"

구현_언어_런타임:
  decision: "구현 언어/런타임 = C(C11) / native POSIX — core slot language·runtime 결정 참조"
  status: decided
  rationale: "연기 레지스터 #1 해소. language=C·runtime=native 결정과 동일 (slot 결정이 진실 source; 본 register 행은 참조)."
  consequence: "language slot consequence 와 동일 — 수동 메모리·errno·CMake·sanitizer·externalized dep."
  source: "본 문서 ## Decisions language·runtime slot."
  ratified_by: "jjinri"
  date: "2026-05-19"

persistence_backend_subscription_availability:
  decision: "persistence backend = rdbms(PostgreSQL) — core slot persistence 결정 참조"
  status: decided
  rationale: "연기 레지스터 #2 해소. persistence slot backend=rdbms 와 동일."
  consequence: "PostgreSQL/libpq 의존. 3 table schema 확정(persistence slot 참조)."
  source: "본 문서 persistence slot; design/nssf/architecture/state-persistence.md."
  ratified_by: "jjinri"
  date: "2026-05-19"

http_2_tls_oauth2_client_library:
  decision: "HTTP/2=nghttp2, TLS=외부화(mesh), OAuth2 client=옵션(default off) — slot sbi_client_stack·tls_security·oauth2_token_validation 참조"
  status: decided
  rationale: "연기 레지스터 #3 해소. sbi_client_stack(nghttp2 client)·tls_security(external)·oauth2_token_validation(inbound false; outbound client-cred 는 config 옵션) 종합."
  consequence: "nghttp2 client/server 단일 stack. TLS mesh. outbound OAuth2 default off, config 로 enable."
  source: "본 문서 sbi_client_stack/tls_security/oauth2_token_validation slot; design/nssf/architecture/configuration-strategy.md."
  ratified_by: "jjinri"
  date: "2026-05-19"

배포_토폴로지_service_mesh_nf_내장_tls:
  decision: "배포 = service-mesh sidecar (NF 내장 TLS 미채택) — core slot deployment_topology·tls_security 참조"
  status: decided
  rationale: "연기 레지스터 #4 해소. deployment_topology=mesh-sidecar standalone, NF 내장 TLS=rejected(tls_security=external 정합)."
  consequence: "h2c NF + mesh TLS. NF 바이너리 TLS 의존 0."
  source: "본 문서 deployment_topology/tls_security slot."
  ratified_by: "jjinri"
  date: "2026-05-19"

amf_reallocation_via_ran_지원_ngap_reroute_nas_request:
  decision: "AMF reallocation via RAN (NGAP REROUTE NAS REQUEST §8.6.5) 지원 = 범위 외 (미구현 default)"
  status: explicitly_out_of_scope
  rationale: "38.413 manifest exclude. NSSF↔NG-RAN NGAP 경로는 본 NF SBI 구현 범위 밖 운영 정책. 미지원이 default — 자율 코드 생성이 본 경로 코드를 생성하면 안 됨."
  consequence: "NSSF 는 SBI 측만(AuthorizedNetworkSliceInfo 의 candidateAmf/targetAmfSet 응답) 다루고 RAN NGAP REROUTE 자체 미구현. 전환 시 specs/38.413 cp + manual_overrides.exclude 제거 + arch hooking + 본 행 재결정."
  source: "design/nssf/_manifest.yaml manual_overrides.exclude 38.413; design/nssf/architecture/decisions/ADR-0001-architecture-baseline.md ## Open choices."
  ratified_by: "jjinri"
  date: "2026-05-19"

sba_security_profile_깊이_tls_version_cipher:
  decision: "SBA security profile 깊이(TLS version, cipher) = 범위 외 (config 외부화)"
  status: explicitly_out_of_scope
  rationale: "33.501 manifest exclude. TLS version/cipher 는 mesh/배포 정책 — NF 구현이 cipher suite 를 고정하면 안 됨(외부화). arch 'config 외부화 권고' 와 동일."
  consequence: "NF 는 TLS 미내장(tls_security=external). version/cipher 결정은 mesh 운영. 자율 코드 생성이 TLS/cipher 코드 생성 금지."
  source: "design/nssf/_manifest.yaml manual_overrides.exclude 33.501; 본 문서 tls_security slot; design/nssf/architecture/decisions/ADR-0001-architecture-baseline.md ## Open choices."
  ratified_by: "jjinri"
  date: "2026-05-19"

subscription_store_backend_default_권고:
  decision: "Subscription store backend default = rdbms(PostgreSQL) — core slot persistence 단일 backend 결정 참조"
  status: decided
  rationale: "연기 레지스터 #7 해소. arch 가 default 권고를 연기했으나 본 단계서 persistence=rdbms 단일로 닫음(in-memory default 권고 폐기 — eng_frozen 은 단일 backend 요구). #2 와 다른 register 행이나 같은 slot 결정 참조(중복 아님)."
  consequence: "subscription store = nssf_subscriptions table(PostgreSQL). dev 추상 repo 는 코드 관심사, frozen 결정은 단일 rdbms."
  source: "본 문서 persistence slot; design/nssf/architecture/state-persistence.md ## Open Questions."
  ratified_by: "jjinri"
  date: "2026-05-19"
```

## Out of scope

- **AMF reallocation via RAN (NGAP REROUTE NAS REQUEST §8.6.5)** — `explicitly_out_of_scope` (register `amf_reallocation_via_ran_지원_ngap_reroute_nas_request`). 38.413 manifest exclude. 자율 코드 생성이 본 경로 코드 생성 금지.
- **SBA security profile 깊이 (TLS version/cipher)** — `explicitly_out_of_scope` (register `sba_security_profile_깊이_tls_version_cipher`). 33.501 manifest exclude, mesh 외부화.
- **자율 코드 생성** — eng_frozen PASS 후 별 단계(파이프라인 밖). 본 문서는 결정 freeze 까지.

## Open Questions

없음

## References

- `docs/adr/ADR-0002-engineering-design-freeze.md` — Engineering Design Freeze 단계·eng_frozen gate.
- `docs/adr/ADR-0003-engineering-dependency-closure.md` — profile v2 dependency closure.
- `design/schemas/engineering-core-slots.yaml` — 13 core slot typed shape (inventory core).
- `design/nssf/architecture/decisions/ADR-0001-architecture-baseline.md` `## Open choices` — 연기 레지스터 7행(inventory NF-specific).
- `design/nssf/architecture/{module-boundaries,state-persistence,observability,configuration-strategy,runtime-model}.md` — 결정 근거.
- `handoff/nssf/contract.yaml` + `design/nssf/contract/data-model/` — DB schema 도출 근거.
- `design/nssf/_manifest.yaml` `manual_overrides.exclude` — 33.501·38.413 out-of-scope 근거.
- `docs/plans/2026-05-19-nf-eng-design-nssf-firstrun-plan.md` — 본 사이클 plan(Pane 2 second-opinion ×2 반영).
