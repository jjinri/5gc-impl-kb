# NSSF runtime — ADR-0004 Layer A security baseline (mTLS + inbound OAuth2).
# All crypto is OpenSSL (TLS/X.509) + libjwt v3 (JWT/JWS/JWKS) — no hand-rolled
# primitives (M6). zlog is the platform logger; libcurl is libjwt's JWKS transport.

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(libjwt REQUIRED IMPORTED_TARGET libjwt>=3.3.3)
pkg_check_modules(zlog REQUIRED IMPORTED_TARGET zlog>=1.2.16)
pkg_check_modules(libcurl REQUIRED IMPORTED_TARGET libcurl>=7.81)

add_library(nssf_runtime STATIC
    tls_context.c
    oauth2_jwks.c
    oauth2_outbound.c
    oauth2_scope_check.c
    secret_ref_resolver.c)

target_include_directories(nssf_runtime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(nssf_runtime PRIVATE c_std_11)

target_link_libraries(nssf_runtime
    PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
        Threads::Threads
        PkgConfig::libjwt
        PkgConfig::libcurl
        PkgConfig::zlog
    PRIVATE
        nssf_sanitizers
        nssf_warnings)
