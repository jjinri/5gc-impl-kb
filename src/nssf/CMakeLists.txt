add_subdirectory(wrappers)
add_subdirectory(runtime)
add_subdirectory(repository)
add_subdirectory(engine)
add_subdirectory(handlers)
add_subdirectory(server)

add_library(nssf_transport STATIC
    transport/nsselection_handler.c)

target_include_directories(nssf_transport PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/transport
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/api
    ${CMAKE_CURRENT_SOURCE_DIR}/common)

target_compile_features(nssf_transport PRIVATE c_std_11)
target_link_libraries(nssf_transport PRIVATE nssf_sanitizers nssf_warnings)

# ─── nssf process executable (WI-nsselection-handler) ──────────────────
# Bootstraps TLS context + JWKS cache + AvailabilityRepository + SelectionEngine
# seams, registers the NSSelectionGet route, and runs the nghttp2/libuv server
# until graceful shutdown. conninfo / cert paths / JWKS URL all come from the
# environment (operator config) — never hardcoded.
add_executable(nssf
    main.c)

target_compile_features(nssf PRIVATE c_std_11)

target_link_libraries(nssf
    PRIVATE
        nssf_server
        nssf_handlers
        nssf_runtime
        nssf_repository
        nssf_engine
        nssf_generated
        nssf_sanitizers
        nssf_warnings)
