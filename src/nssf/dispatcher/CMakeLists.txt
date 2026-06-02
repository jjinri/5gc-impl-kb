# nssf_dispatcher — NotificationDispatcher (WI-notification-dispatcher).
#
# The NSSF's single outbound module (NotificationDispatcher.md). Phase 1~3
# minimum (open-gaps-and-assumptions G-08): synchronous dispatch + ONE retry —
# no exponential backoff / dead-letter table (Phase 4 hardening).
#
# The retry_queue store has a libpq backend (production, FOR UPDATE SKIP LOCKED
# dequeue) + an in-memory test seam, mirroring nssf_repository. The outbound POST
# uses libcurl with TLS peer+host verification + no redirect-follow (ADR-0004
# Layer A), and attaches the OAuth2 bearer via nssf_runtime's oauth2_outbound (M4,
# fail-closed). cJSON comes from nssf_generated.

find_package(PostgreSQL REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(libcurl REQUIRED IMPORTED_TARGET libcurl>=7.81)

add_library(nssf_dispatcher STATIC
    notification_dispatcher.c)

target_include_directories(nssf_dispatcher PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(nssf_dispatcher PRIVATE c_std_11)

target_link_libraries(nssf_dispatcher
    PUBLIC
        nssf_runtime
        nssf_engine
        nssf_generated
        PostgreSQL::PostgreSQL
        PkgConfig::libcurl
    PRIVATE
        nssf_sanitizers
        nssf_warnings)
