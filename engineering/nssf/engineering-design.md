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

아래 `inventory_id: {row}` 매핑이 기계 계약(validator 파싱 대상). 본 사이클의 *security re-ratify* (2026-05-21) — `tls_security` / `oauth2_token_validation` / `deployment_topology` / `sbi_server_stack` / `sbi_client_stack` / `configuration_management` 6 core slot + per-NF 연기 레지스터 (8행, architecture/ADR-0001 ## Open choices 와 정합) 가 `docs/adr/ADR-0004-project-security-baseline.md` baseline (TLS / mTLS / OAuth2 production-capable code path 의무) 을 반영해 갱신됐다. 변경 행은 `ratified_by: jjinri / date: 2026-05-21`. 미변경 행은 `2026-05-19` 유지. `nf-eng-status` `gates.eng_frozen` PASS 가 자율 코드 생성 GO.

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
  decision: "배포 = standalone container pod with NF internal TLS production-capable; mesh sidecar 는 선택적 layer (NF 의존 아님)"
  status: decided
  form: "standalone container pod; single native binary with internal TLS stack (tls_security=enabled, app_library). service-mesh sidecar 는 추가 보안·관찰 layer 로 *선택적* 배치 가능 — NF 는 mesh 부재에도 production-capable (ADR-0004 의무 1·5). mesh 배치 시 sidecar 와의 통신은 mTLS or h2c (운영 정책)."
  rationale: "ADR-0004 baseline 의무 1·5 반영. 이전 'mesh 외부화 단일값 close' 결정 폐기 — NF 가 mesh 의존 단일값으로 close 되면 mesh 없는 환경 (dev, edge, peer-to-peer) 에서 production-capable 아님. considered=[NF-internal-TLS standalone (채택), mesh-sidecar standalone (rejected: 단일값 close 위반), NF-internal-TLS + optional mesh (채택과 동일)]. NF 내장 TLS 가 default — mesh 는 추가 layer."
  consequence: "NF 는 TLS 내장 (sbi_server_stack 에 nghttp2 + OpenSSL backend). mesh 부재에서도 inbound HTTPS/h2 + mTLS + OAuth2 bearer validation 가능. mesh 배치 시 sidecar 가 추가 외부 layer (NF 의존 아님). 33.310/33.210 세부는 operator-provided cert/config + library compliance (ADR-0004 의무 7)."
  source: "docs/adr/ADR-0004-project-security-baseline.md 의무 1·5; design/nssf/architecture/module-boundaries.md; design/nssf/architecture/overview.md."
  ratified_by: "jjinri"
  date: "2026-05-21"

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
  decision: "SBI server = nghttp2 기반 h2 + h2c configurable, epoll 단일 프로세스. TLS 는 tls_security app_library backend (OpenSSL) 와 통합."
  status: decided
  server_runtime: "single-process event loop (epoll), C native"
  http2_mode: "h2 (HTTP/2 over TLS) when tls.enabled=true; h2c (prior-knowledge cleartext) fallback when tls.enabled=false (dev profile). production-capable code path 항상 존재 (ADR-0004 의무 1·5)."
  framework:
    dependency_source: third_party
    version_policy: externalized
    package: nghttp2
  rationale: "선택가능. C HTTP/2 server 후보=[nghttp2, libh2o, custom]. nghttp2=성숙·3GPP SBI 적합·TLS backend (OpenSSL/BoringSSL/mbedTLS) 와 통합 지원. rejected — libh2o(통합 복잡, TLS backend 선택 자유 적음), custom(재발명 위험, ADR-0004 의무 6 위반)."
  consequence: "nghttp2 system/container package(externalized) 의존. TLS 적용 시 nghttp2 가 tls_security 의 app_library (OpenSSL) 와 통합. dev 에서 h2c 가능하나 production code path 는 항상 보유."
  source: "design/nssf/architecture/runtime-model.md; design/nssf/architecture/request-flow.md; docs/adr/ADR-0004-project-security-baseline.md 의무 1·5."
  ratified_by: "jjinri"
  date: "2026-05-21"

sbi_client_stack:
  decision: "outbound SBI client 필요 (NotificationDispatcher→AMF callback, NRF discovery + token endpoint). nghttp2 client + tls_security app_library (OpenSSL) backend + outbound OAuth2 token attach production-capable."
  status: decided
  required: true
  source_arch_ref: "design/nssf/architecture/module-boundaries.md#boundaries"
  targets:
    - "AMF — NSSAIAvailability 변경 통지 subscription callback URI (outbound POST)"
    - "NRF — NF discovery + OAuth2 token endpoint (client_credentials grant)"
  http2_mode: "h2 (HTTPS/HTTP/2) to peer with NF-internal TLS (tls_security app_library); h2c fallback when tls.enabled=false (dev profile). outbound mTLS production-capable (ADR-0004 의무 2). outbound OAuth2 token acquire/attach production-capable (ADR-0004 의무 4)."
  timeout_retry_policy: "connect 2s / request 5s; notification retry exponential backoff base 1s cap 60s max 10 attempts, persisted in nssf_notification_retry_queue (status+next_attempt_at, row-lock dequeue via FOR UPDATE SKIP LOCKED). outbound TLS handshake fail / OAuth2 token endpoint 5xx → retry; client_secret invalid → 즉시 dead-letter + alert."
  client:
    dependency_source: third_party
    version_policy: externalized
    package: nghttp2
  rationale: "required=true 는 R/F-derived requirement trace — arch NotificationDispatcher 가 availability 변경을 subscription callback URI 로 outbound POST + NRF discovery. client lib 후보=[libcurl HTTP/2+TLS, nghttp2 direct + OpenSSL, nghttp2+libuv + OpenSSL]. criteria=server stack 재사용·TLS backend 일관·dependency 폭. nghttp2 client 선택 — sbi_server (nghttp2 server) + tls_security (OpenSSL) 와 동일 stack 재사용. rejected — libcurl (HTTP client 편의 크나 nghttp2 stack 과 lib 중복, dependency 폭 증가), nghttp2+libuv (event loop 추가 필요 — current epoll 단순성과 충돌)."
  consequence: "retry queue 가 persistence(rdbms) 와 동일 backend → subscription 변경+enqueue 1 트랜잭션. outbound TLS = NF 내부 OpenSSL backend (tls_security app_library). outbound OAuth2 token cache + endpoint 호출 NF 내부 — production-capable code path 항상 존재, config 가 enable/disable."
  source: "design/nssf/architecture/module-boundaries.md#boundaries (NotificationDispatcher row); design/nssf/architecture/request-flow.md (outbound 시퀀스 5); docs/adr/ADR-0004-project-security-baseline.md 의무 2·4."
  ratified_by: "jjinri"
  date: "2026-05-21"

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
  decision: "TLS = NF internal (app_library backend OpenSSL). production-capable code path 항상 존재 (ADR-0004 의무 1). mTLS production-capable (ADR-0004 의무 2). config 가 enable/disable + cert/key/CA/peer policy 만 결정."
  status: decided
  mode: enabled
  min_version: "TLSv1.2 (default; operator config 로 TLSv1.3 강제 가능)"
  cipher_policy: "library default (OpenSSL secure defaults); operator-provided `tls.cipher_suites` config 로 명시 시 그에 따름. NF 가 cipher suite 를 hard-code 하지 않음 (ADR-0004 의무 7)."
  mutual_auth: "configurable (server-side: `mtls.enabled` + `mtls.client_cert_required`; client-side: `mtls.client_cert_path` + `mtls.client_key_path`). peer identity verify = X.509 SAN/CN 매칭 via library default + operator CA bundle."
  tls_provider: "app_library"
  tls_dependency:
    dependency_source: third_party
    version_policy: externalized
    package: openssl
  rationale: "ADR-0004 baseline 의무 1·2·6 반영. 이전 'TLS external (mesh sidecar)' 결정 폐기 — NF 가 mesh 의존 단일값으로 close 되면 mesh 없는 환경 production-capable 아님 (ADR-0004 의무 5). TLS lib 후보=[openssl, boringssl, mbedtls]. criteria=성숙도·TLS1.3 지원·NF cert/PKI 사용처·nghttp2 통합·widespread audit. openssl 선택 — 가장 광범위·nghttp2 통합 표준·system package externalized 가능. rejected — boringssl (Google fork; system package 부재·build 복잡), mbedtls (소형 임베디드 적합하나 nghttp2 통합 less standard, RSA/ECDSA 외 curve set 적음). TLS/X.509 primitive 직접 구현 금지 (ADR-0004 의무 6)."
  consequence: "NF 바이너리에 OpenSSL externalized 의존. nghttp2 server/client 가 TLS context 사용. cert/key/CA load = configuration_management 의 cert/key/CA path config. TLS version/cipher 세부는 operator/library default — NF hard-code 금지. mTLS server reject / client cert 실패는 error-propagation.md 의 transport 단계 처리 (응답 없음, log/metric 만)."
  source: "docs/adr/ADR-0004-project-security-baseline.md 의무 1·2·6·7; design/nssf/architecture/configuration-strategy.md (TLS / mTLS row); design/nssf/architecture/runtime-model.md (start-up TLS context init)."
  ratified_by: "jjinri"
  date: "2026-05-21"

oauth2_token_validation:
  decision: "inbound OAuth2 bearer token validation = NF internal production-capable (ADR-0004 의무 3). config 가 enable/disable. token signature 는 third-party JWT library (libjwt + OpenSSL backend)."
  status: decided
  enabled: true
  token_validation_strategy: "jwks (JSON Web Key Set 기반). issuer/jwks_uri 는 configuration_management 의 `oauth2_inbound.*` config 로 외부 주입. JWKS background refresh (default TTL 5 min). signature/expiry/audience/scope 검증 NF 내부 수행."
  lib:
    dependency_source: third_party
    version_policy: externalized
    package: libjwt
  rationale: "ADR-0004 baseline 의무 3·6 반영. 이전 'enabled=false (mesh/gateway 외부화)' 결정 폐기 — NF 가 mesh 의존 단일값으로 close 되면 mesh 없는 환경 production-capable 아님 (ADR-0004 의무 5). JWT library 후보=[libjwt, cjose, pico_jose]. criteria=OpenSSL backend 호환·JWKS 지원·active maintenance·C native API. libjwt 선택 — OpenSSL backend 표준·JWKS 지원·system package externalized 가능·simple C API. rejected — cjose (less active, JWKS 지원 약), pico_jose (소형이나 production audit 부족). JWT primitive 직접 구현 금지 (ADR-0004 의무 6). outbound OAuth2 client credentials 는 sbi_client_stack 의 책임 (별 slot)."
  consequence: "NF 바이너리에 libjwt externalized 의존. inbound bearer validation = transport 단계 (request-flow 1-b). dev profile 에서 `oauth2_inbound.enabled=false` 가능하나 production code path 항상 보유 (ADR-0004 의무 5). JWKS load 실패는 fail-fast (start-up). 실패 case = 401 INVALID_TOKEN / 403 INSUFFICIENT_SCOPE (error-propagation.md)."
  source: "docs/adr/ADR-0004-project-security-baseline.md 의무 3·5·6; design/nssf/architecture/configuration-strategy.md (OAuth2 inbound row); design/nssf/architecture/request-flow.md (inbound 1-b)."
  ratified_by: "jjinri"
  date: "2026-05-21"

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
  decision: "config = inih(INI) 파서 + env override; secret 외부화(ref-only). security capability keys (TLS / mTLS / inbound OAuth2 / outbound OAuth2) 가 config 로 enable/disable + 외부 자원 주입."
  status: decided
  secret_strategy: "secret 미저장 — `oauth2_outbound.client_secret_ref` / `mtls.client_key_path` 등 ref 만 보유, 실제 secret/key 는 배포(env/vault/file) 외부 주입. cert/key load 는 NF 가 file path 받아 library 에 전달."
  override_order: "compiled defaults < config file < environment variables (env 최우선)"
  config:
    dependency_source: third_party
    version_policy: pinned
    package: inih
  rationale: "선택가능. C config 후보=[inih vendored, libconfig, stdlib getenv-only]. inih=초경량 vendoring·INI 단순. override 순서 defaults<file<env. secret 은 ref-only(arch: 형식 강제 안 함). rejected — libconfig(과대), getenv-only(파일 config 부재). ADR-0004 baseline 의무 5 (dev disable + production-capable path) 가 *config-driven enable/disable* 로 표현됨 — 본 slot 이 security capability 의 단일 control point."
  consequence: "inih pinned vendored. secret 평문 미보유 — 배포가 env/vault 주입. config 누락 시 compiled default. security config key 그룹 (configuration-strategy.md 참조): `tls.*` (enabled/cert_path/key_path/ca_bundle/min_version/cipher_suites), `mtls.*` (enabled/client_cert_required/client_cert_path/client_key_path/peer_verify), `oauth2_inbound.*` (enabled/issuer/jwks_uri/expected_audience/required_scopes), `oauth2_outbound.*` (enabled/token_url/client_id/client_secret_ref/scope/token_cache_ttl). dev disable 가능, production-capable code path 항상 보유."
  source: "design/nssf/architecture/configuration-strategy.md; docs/adr/ADR-0004-project-security-baseline.md 의무 5."
  ratified_by: "jjinri"
  date: "2026-05-21"

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

http_2_server_client_library:
  decision: "HTTP/2 server + client library = nghttp2 (단일 stack 재사용) — core slot sbi_server_stack·sbi_client_stack 참조"
  status: decided
  rationale: "연기 레지스터 (architecture/ADR-0001 ## Open choices row 'HTTP/2 server / client library') 해소. sbi_server_stack (nghttp2 server) + sbi_client_stack (nghttp2 client) 동일 stack 으로 닫음. PR2 architecture/ADR-0001 splited TLS / OAuth2 lib 를 별 행으로 — 본 행은 HTTP/2 lib 만."
  consequence: "nghttp2 client/server 단일 externalized 의존. epoll 단일 프로세스. TLS backend 통합은 tls_library slot 의 책임."
  source: "본 문서 sbi_server_stack/sbi_client_stack slot; design/nssf/architecture/decisions/ADR-0001-architecture-baseline.md ## Open choices."
  ratified_by: "jjinri"
  date: "2026-05-21"

tls_library:
  decision: "TLS library = OpenSSL (NF internal app_library backend) — core slot tls_security 참조"
  status: decided
  rationale: "연기 레지스터 (architecture/ADR-0001 ## Open choices row 'TLS library') 해소. PR2 architecture 가 TLS lib 결정을 engineering 단계로 위임. tls_security slot 이 후보 비교 (openssl/boringssl/mbedtls) 후 openssl 채택 — 본 행은 그 결정 참조. ADR-0004 의무 1·2·6 (third-party library 의무)."
  consequence: "OpenSSL externalized 의존. nghttp2 (server+client) 와 통합. mTLS production-capable. cipher/TLS version 은 operator-provided config + OpenSSL default (ADR-0004 의무 7)."
  source: "본 문서 tls_security slot; design/nssf/architecture/decisions/ADR-0001-architecture-baseline.md ## Open choices; docs/adr/ADR-0004-project-security-baseline.md."
  ratified_by: "jjinri"
  date: "2026-05-21"

oauth2_jwt_library:
  decision: "OAuth2 / JWT library = libjwt (inbound bearer validation; OpenSSL backend) — core slot oauth2_token_validation 참조"
  status: decided
  rationale: "연기 레지스터 (architecture/ADR-0001 ## Open choices row 'OAuth2 / JWT library') 해소. PR2 architecture 가 JWT lib 결정을 engineering 단계로 위임. oauth2_token_validation slot 이 후보 비교 (libjwt/cjose/pico_jose) 후 libjwt 채택 — 본 행은 그 결정 참조. ADR-0004 의무 3·6."
  consequence: "libjwt externalized 의존 (OpenSSL backend 공유). inbound bearer = JWKS 기반 validation. outbound OAuth2 client_credentials 는 sbi_client_stack 의 책임 (token endpoint 호출 + Bearer attach)."
  source: "본 문서 oauth2_token_validation slot; design/nssf/architecture/decisions/ADR-0001-architecture-baseline.md ## Open choices; docs/adr/ADR-0004-project-security-baseline.md."
  ratified_by: "jjinri"
  date: "2026-05-21"

배포_토폴로지_service_mesh_nf_내장_tls:
  decision: "배포 = standalone container pod with NF 내장 TLS production-capable (mesh sidecar optional) — core slot deployment_topology·tls_security 참조"
  status: decided
  rationale: "연기 레지스터 (architecture/ADR-0001 ## Open choices row '배포 토폴로지') 해소. 이전 'mesh-sidecar 단일' 결정 폐기 (ADR-0004 의무 1·5 충돌). NF 내장 TLS 가 default — mesh 는 추가 layer 로 선택적. deployment_topology=enabled+app_library(OpenSSL) 정합."
  consequence: "NF 바이너리 TLS stack 보유. mesh 부재 production-capable. mesh 배치 시 추가 layer (NF 의존 아님)."
  source: "본 문서 deployment_topology/tls_security slot; design/nssf/architecture/overview.md; docs/adr/ADR-0004-project-security-baseline.md 의무 1·5."
  ratified_by: "jjinri"
  date: "2026-05-21"

amf_reallocation_via_ran_지원_ngap_reroute_nas_request:
  decision: "AMF reallocation via RAN (NGAP REROUTE NAS REQUEST §8.6.5) 지원 = 범위 외 (미구현 default)"
  status: explicitly_out_of_scope
  rationale: "38.413 manifest exclude. NSSF↔NG-RAN NGAP 경로는 본 NF SBI 구현 범위 밖 운영 정책. 미지원이 default — 자율 코드 생성이 본 경로 코드를 생성하면 안 됨."
  consequence: "NSSF 는 SBI 측만(AuthorizedNetworkSliceInfo 의 candidateAmf/targetAmfSet 응답) 다루고 RAN NGAP REROUTE 자체 미구현. 전환 시 specs/38.413 cp + manual_overrides.exclude 제거 + arch hooking + 본 행 재결정."
  source: "design/nssf/_manifest.yaml manual_overrides.exclude 38.413; design/nssf/architecture/decisions/ADR-0001-architecture-baseline.md ## Open choices."
  ratified_by: "jjinri"
  date: "2026-05-19"

tls_version_cipher_세부:
  decision: "TLS version / cipher 세부 = 외부 처리 (operator-provided config + library compliance assumption)"
  status: explicitly_out_of_scope
  rationale: "연기 레지스터 (architecture/ADR-0001 ## Open choices row 'TLS version / cipher 세부') — ADR-0004 의무 7 흡수. 33.310 / 33.210 certificate / cipher profile 세부는 NF 가 직접 결정하지 않는다 (operator/library default). tls_security 의 `cipher_policy` 가 library default + operator config 로 표현."
  consequence: "NF 바이너리에 cipher suite / TLS version hard-code 금지. operator 가 `tls.min_version` / `tls.cipher_suites` config 로 명시 가능, 미명시 시 OpenSSL secure defaults. 자율 코드 생성이 cipher/version 고정 코드 생성 금지."
  source: "docs/adr/ADR-0004-project-security-baseline.md 의무 7; 본 문서 tls_security slot; design/nssf/architecture/decisions/ADR-0001-architecture-baseline.md ## Open choices."
  ratified_by: "jjinri"
  date: "2026-05-21"
```

## Out of scope

- **AMF reallocation via RAN (NGAP REROUTE NAS REQUEST §8.6.5)** — `explicitly_out_of_scope` (register `amf_reallocation_via_ran_지원_ngap_reroute_nas_request`). 38.413 manifest exclude. 자율 코드 생성이 본 경로 코드 생성 금지.
- **TLS version / cipher 세부 정책** — `explicitly_out_of_scope` (register `tls_version_cipher_세부`). ADR-0004 의무 7 — operator-provided config + OpenSSL library compliance assumption. NF 바이너리에 cipher suite/version hard-code 금지.
- **자율 코드 생성** — eng_frozen PASS 후 별 단계(파이프라인 밖). 본 문서는 결정 freeze 까지.

## Open Questions

없음

## References

- `docs/adr/ADR-0002-engineering-design-freeze.md` — Engineering Design Freeze 단계·eng_frozen gate.
- `docs/adr/ADR-0003-engineering-dependency-closure.md` — profile v2 dependency closure.
- `docs/adr/ADR-0004-project-security-baseline.md` — TLS / mTLS / OAuth2 production-capable code path 의무 source (본 사이클 security re-ratify 입력).
- `design/schemas/engineering-core-slots.yaml` — 13 core slot typed shape (inventory core).
- `design/nssf/architecture/decisions/ADR-0001-architecture-baseline.md` `## Open choices` — 연기 레지스터 8행(inventory NF-specific, PR2 에서 TLS/OAuth lib split 반영).
- `design/nssf/architecture/{overview,module-boundaries,state-persistence,observability,configuration-strategy,runtime-model,request-flow,error-propagation,test-strategy}.md` — 결정 근거.
- `handoff/nssf/contract.yaml` + `design/nssf/contract/data-model/` — DB schema 도출 근거.
- `design/nssf/_manifest.yaml` `manual_overrides.exclude` — 33.501 (ADR-0004 흡수) · 38.413 (운영 보류) 사유.
- `docs/plans/2026-05-19-nf-eng-design-nssf-firstrun-plan.md` — 첫 ratify 사이클 plan(Pane 2 second-opinion ×2 반영).
