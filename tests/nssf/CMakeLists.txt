add_library(unity STATIC vendor/Unity/unity.c)
target_include_directories(unity PUBLIC vendor/Unity)
target_compile_options(unity PRIVATE -w)

add_executable(test_nsselection_handler
    unit/test_nsselection_handler.c)

target_link_libraries(test_nsselection_handler
    PRIVATE
        unity
        nssf_transport
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_nsselection_handler PRIVATE c_std_11)

add_test(NAME nsselection_handler_smoke
         COMMAND test_nsselection_handler)

# Phase 1 wave 1 WI-codegen-bootstrap smoke test.
# nssf_generated 가 link 가능 + snssai/plmn_id 의 alloc/serialize/parse/free
# round-trip 증명. -w 적용 (generated header 의 doxygen-escape 등 warning
# 정책 책임 외).
add_executable(test_generated_smoke
    unit/test_generated_smoke.c)

target_link_libraries(test_generated_smoke
    PRIVATE
        unity
        nssf_generated
        nssf_sanitizers)

target_compile_features(test_generated_smoke PRIVATE c_std_11)
target_compile_options(test_generated_smoke PRIVATE -Wall -Wextra)

add_test(NAME generated_smoke
         COMMAND test_generated_smoke)

# Phase 1 wave 1 WI-problem-details-wrapper — RFC 7807 factory + mandatory
# header metadata. dev/nssf/error-cause-catalog.yaml 의 18 production cause
# 와 1:1 매핑.
add_executable(test_problem_details_factory
    unit/test_problem_details_factory.c)

target_link_libraries(test_problem_details_factory
    PRIVATE
        unity
        nssf_wrappers
        nssf_sanitizers)

target_compile_features(test_problem_details_factory PRIVATE c_std_11)
target_compile_options(test_problem_details_factory PRIVATE -Wall -Wextra)

add_test(NAME nssf_problem_details_factory
         COMMAND test_problem_details_factory)

add_executable(test_problem_details_headers
    unit/test_problem_details_headers.c)

target_link_libraries(test_problem_details_headers
    PRIVATE
        unity
        nssf_wrappers
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_problem_details_headers PRIVATE c_std_11)

add_test(NAME nssf_problem_details_headers
         COMMAND test_problem_details_headers)

# Phase 1 wave 1 WI-nftype-wrapper — NFType anyOf passthrough policy (G-09).
# design/nssf/contract/data-model/NFType.json 의 67 known enum + unknown
# passthrough wrapper.
add_executable(test_nftype_wrapper
    unit/test_nftype_wrapper.c)

target_link_libraries(test_nftype_wrapper
    PRIVATE
        unity
        nssf_wrappers
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_nftype_wrapper PRIVATE c_std_11)

add_test(NAME nssf_nftype_wrapper
         COMMAND test_nftype_wrapper)

# Phase 1 wave 1 WI-tls-bootstrap — runtime mTLS + inbound OAuth2 (ADR-0004
# Layer A). 두 unit test 가 nssf_runtime 의 TLS context (in-test PKI +
# BIO-pair 핸드셰이크) 와 OAuth2 scope-check (whole-token 의미론) 를 검증.
# ctest -R nssf_tls_ / nssf_oauth2_ 로 매칭.
#
# OpenSSL imported target 은 이 directory scope 에서 별도로 노출해야 한다
# (runtime 의 find_package 는 그 scope 한정). 동일 OpenSSL 패키지라 1회 재선언.
find_package(OpenSSL REQUIRED)

add_executable(test_tls_context_handshake
    unit/test_tls_context_handshake.c)

target_link_libraries(test_tls_context_handshake
    PRIVATE
        unity
        nssf_runtime
        OpenSSL::SSL
        OpenSSL::Crypto
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_tls_context_handshake PRIVATE c_std_11)

add_test(NAME nssf_tls_context_handshake
         COMMAND test_tls_context_handshake)

add_executable(test_oauth2_scope_check
    unit/test_oauth2_scope_check.c)

target_link_libraries(test_oauth2_scope_check
    PRIVATE
        unity
        nssf_runtime
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_oauth2_scope_check PRIVATE c_std_11)

add_test(NAME nssf_oauth2_scope_check
         COMMAND test_oauth2_scope_check)

# Phase 2 WI-outbound-oauth2 — outbound OAuth2 client_credentials token acquire
# + bearer attach (ADR-0004 M4). Driven entirely through the mock token-transport
# seam (nssf_oauth2_outbound_set_transport) — no network, no DB, no live NRF, so
# the 12 ratified decisions (acquire/attach/TTL-cache/refresh-buffer/cap/
# fail-closed/scope-override/https-only/mTLS-vs-secret/secret-prod-gate) run
# UNCONDITIONALLY in CI. curl_slist + curl/curl.h come transitively from
# nssf_runtime (PUBLIC libcurl). ctest -R nssf_oauth2_outbound 매칭
# (nssf_oauth2_outbound_attach).
add_executable(test_oauth2_outbound_attach
    unit/test_oauth2_outbound_attach.c)

target_link_libraries(test_oauth2_outbound_attach
    PRIVATE
        unity
        nssf_runtime
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_oauth2_outbound_attach PRIVATE c_std_11)

add_test(NAME nssf_oauth2_outbound_attach
         COMMAND test_oauth2_outbound_attach)

# Phase 1 wave 1 WI-availability-repo — AvailabilityRepository in-memory test
# seam round-trip. DB 의존 없음 (nssf_availability_repo_new_inmemory). cJSON 은
# nssf_repository 의 PUBLIC nssf_generated 의존으로 전이. ctest -R
# nssf_availability_repo_ 매칭 (nssf_availability_repo_inmemory).
add_executable(test_availability_repo_inmemory
    unit/test_availability_repo_inmemory.c)

target_link_libraries(test_availability_repo_inmemory
    PRIVATE
        unity
        nssf_repository
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_availability_repo_inmemory PRIVATE c_std_11)

add_test(NAME nssf_availability_repo_inmemory
         COMMAND test_availability_repo_inmemory)

# Phase 1 wave 1 WI-selection-engine — SelectionEngine NSSelectionGet decision
#   allowed = requested ∩ subscribed ∩ plmn_policy ∩ availability[tai].
# 네 unit test 가 registration / pdu / ueconfig / redirect(G-07)+seam path 를
# 검증한다. availability 는 nssf_availability_repo_new_inmemory() seed (DB/network
# 없음) — nssf_engine 이 nssf_repository(availability header)+nssf_generated(cJSON)
# 를 transitively 노출한다. ctest -R nssf_selection_engine_ 로 4개 매칭.
add_executable(test_selection_engine_registration
    unit/test_selection_engine_registration.c)

target_link_libraries(test_selection_engine_registration
    PRIVATE
        unity
        nssf_engine
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_selection_engine_registration PRIVATE c_std_11)

add_test(NAME nssf_selection_engine_registration
         COMMAND test_selection_engine_registration)

add_executable(test_selection_engine_pdu
    unit/test_selection_engine_pdu.c)

target_link_libraries(test_selection_engine_pdu
    PRIVATE
        unity
        nssf_engine
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_selection_engine_pdu PRIVATE c_std_11)

add_test(NAME nssf_selection_engine_pdu
         COMMAND test_selection_engine_pdu)

add_executable(test_selection_engine_ueconfig
    unit/test_selection_engine_ueconfig.c)

target_link_libraries(test_selection_engine_ueconfig
    PRIVATE
        unity
        nssf_engine
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_selection_engine_ueconfig PRIVATE c_std_11)

add_test(NAME nssf_selection_engine_ueconfig
         COMMAND test_selection_engine_ueconfig)

add_executable(test_selection_engine_redirect
    unit/test_selection_engine_redirect.c)

target_link_libraries(test_selection_engine_redirect
    PRIVATE
        unity
        nssf_engine
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_selection_engine_redirect PRIVATE c_std_11)

add_test(NAME nssf_selection_engine_redirect
         COMMAND test_selection_engine_redirect)

# Phase 2 WI-availability-engine — AvailabilityEngine NSSAIAvailability CRUD
#   (replace / patch / delete + cascade-event seam).
# 세 unit test 가 replace(dedup/204/400/G-11 default) / patch(RFC 6902 ops·415·
# atomicity·404·PATCHED event) / delete(NO_CONTENT + DELETED cascade event,
# after=NULL · NOT_FOUND) 를 검증한다. availability 는
# nssf_availability_repo_new_inmemory() seed + stub change-event seam — DB/network
# 없이 CI 에서 무조건 실행. nssf_engine 이 availability_engine + availability_repo
# (cJSON 전이) 를 노출한다. ctest -R nssf_availability_engine_ 로 3개 매칭.
add_executable(test_availability_engine_replace
    unit/test_availability_engine_replace.c)

target_link_libraries(test_availability_engine_replace
    PRIVATE
        unity
        nssf_engine
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_availability_engine_replace PRIVATE c_std_11)

add_test(NAME nssf_availability_engine_replace
         COMMAND test_availability_engine_replace)

add_executable(test_availability_engine_patch
    unit/test_availability_engine_patch.c)

target_link_libraries(test_availability_engine_patch
    PRIVATE
        unity
        nssf_engine
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_availability_engine_patch PRIVATE c_std_11)

add_test(NAME nssf_availability_engine_patch
         COMMAND test_availability_engine_patch)

add_executable(test_availability_engine_delete_cascade
    unit/test_availability_engine_delete_cascade.c)

target_link_libraries(test_availability_engine_delete_cascade
    PRIVATE
        unity
        nssf_engine
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_availability_engine_delete_cascade PRIVATE c_std_11)

add_test(NAME nssf_availability_engine_delete_cascade
         COMMAND test_availability_engine_delete_cascade)

# Phase 2 WI-notification-dispatcher — NotificationDispatcher retry-queue STORE +
# PRODUCER seam unit suite. Driven ENTIRELY through the in-memory store backend
# (nssf_retry_store_new_inmemory) — no DB, no libpq, no network — so the
# enqueue-on-change acceptance (Put/Patch committed mutation → retry_queue row) +
# dequeue ordering / SKIP-LOCKED claim / complete-vs-requeue run UNCONDITIONALLY in
# CI. nssf_dispatcher PUBLIC-links nssf_runtime/nssf_engine/nssf_generated + libpq
# + libcurl, so cJSON + curl/curl.h + the availability change-event struct come
# transitively. ctest -R nssf_notification_dispatcher_ 매칭
# (nssf_notification_dispatcher_queue).
add_executable(test_notification_dispatcher_queue
    unit/test_notification_dispatcher_queue.c)

target_link_libraries(test_notification_dispatcher_queue
    PRIVATE
        unity
        nssf_dispatcher
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_notification_dispatcher_queue PRIVATE c_std_11)

add_test(NAME nssf_notification_dispatcher_queue
         COMMAND test_notification_dispatcher_queue)

# Phase 3 WI-subscription-store — SubscriptionStore lifecycle unit suite
# (Subscribe/SubModifyPatch/Unsubscribe + initial-snapshot-on-create dispatch +
# on_availability_deleted cascade-tombstone). Driven ENTIRELY through the
# in-memory test seams — nssf_subscription_store_new_inmemory for the store,
# nssf_retry_store_new_inmemory + nssf_availability_repo_new_inmemory + a mock
# dispatcher transport for the snapshot seam — so NO libpq, NO PostgreSQL, NO
# network: the create/patch/cascade acceptance runs UNCONDITIONALLY in CI.
# nssf_store PUBLIC-links nssf_dispatcher / nssf_repository / nssf_generated, so
# notification_dispatcher.h + availability_repository.h + cJSON + curl/curl.h
# come transitively.
#   - store CRUD names start nssf_subscription_store_ (WI verify: ctest -R
#     nssf_subscription_store_),
#   - the required initial-snapshot check is its own target
#     nssf_subscription_initial_snapshot (proves EXACTLY-ONE dispatch on create).
add_executable(test_subscription_store_create
    unit/test_subscription_store_create.c)

target_link_libraries(test_subscription_store_create
    PRIVATE
        unity
        nssf_store
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_subscription_store_create PRIVATE c_std_11)

add_test(NAME nssf_subscription_store_create
         COMMAND test_subscription_store_create)

add_executable(test_subscription_store_patch
    unit/test_subscription_store_patch.c)

target_link_libraries(test_subscription_store_patch
    PRIVATE
        unity
        nssf_store
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_subscription_store_patch PRIVATE c_std_11)

add_test(NAME nssf_subscription_store_patch
         COMMAND test_subscription_store_patch)

add_executable(test_subscription_store_cascade
    unit/test_subscription_store_cascade.c)

target_link_libraries(test_subscription_store_cascade
    PRIVATE
        unity
        nssf_store
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_subscription_store_cascade PRIVATE c_std_11)

add_test(NAME nssf_subscription_store_cascade
         COMMAND test_subscription_store_cascade)

# REQUIRED CHECK — ctest -R nssf_subscription_initial_snapshot must PASS:
# 신규 subscription 진입 시 initial snapshot 을 EXACTLY ONCE dispatch.
add_executable(test_subscription_initial_snapshot
    unit/test_subscription_initial_snapshot.c)

target_link_libraries(test_subscription_initial_snapshot
    PRIVATE
        unity
        nssf_store
        nssf_sanitizers
        nssf_warnings)

target_compile_features(test_subscription_initial_snapshot PRIVATE c_std_11)

add_test(NAME nssf_subscription_initial_snapshot
         COMMAND test_subscription_initial_snapshot)

# Phase 1 wave 1 WI-schema-bootstrap — PostgreSQL bootstrap integration test.
# libpq fixture, ctest -R nssf_schema_bootstrap 매칭. libpq 부재 또는
# POSTGRES_* env 미주입 시 SKIP PASS 로 회피.
add_subdirectory(integration)

# Phase 1 wave 1 WI-nsselection-handler — NSSelectionGet contract test
# (response-schema conformance). 별도 contract/ subdirectory. unity 타깃은 본
# scope 에서 정의되어 하위에서 가시.
add_subdirectory(contract)
