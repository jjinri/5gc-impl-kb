# nssf_server — nghttp2/libuv SBI server + router (WI-nsselection-handler).
#
# engineering-design.md sbi_server_stack = nghttp2 + TLS app_library (OpenSSL),
# single-process event loop. libuv drives accept/IO; OpenSSL terminates TLS;
# nghttp2 runs the HTTP/2 session; the router dispatches to the handlers. The
# router and the event-loop server share one translation unit (router.c) because
# the per-connection state and route dispatch share the same data path. nghttp2
# = libnghttp2 pkg-config IMPORTED_TARGET; libuv pkg-config module name = libuv.

find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(libnghttp2 REQUIRED IMPORTED_TARGET libnghttp2>=1.43)
pkg_check_modules(libuv REQUIRED IMPORTED_TARGET libuv>=1.43)

add_library(nssf_server STATIC
    router.c)

target_include_directories(nssf_server PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(nssf_server PRIVATE c_std_11)

target_link_libraries(nssf_server
    PUBLIC
        nssf_handlers
        nssf_runtime
        nssf_generated
    PRIVATE
        PkgConfig::libnghttp2
        PkgConfig::libuv
        OpenSSL::SSL
        OpenSSL::Crypto
        nssf_sanitizers
        nssf_warnings)
