add_subdirectory(wrappers)
add_subdirectory(runtime)
add_subdirectory(repository)

add_library(nssf_transport STATIC
    transport/nsselection_handler.c)

target_include_directories(nssf_transport PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/transport
    ${CMAKE_CURRENT_SOURCE_DIR}/generated/api
    ${CMAKE_CURRENT_SOURCE_DIR}/common)

target_compile_features(nssf_transport PRIVATE c_std_11)
target_link_libraries(nssf_transport PRIVATE nssf_sanitizers nssf_warnings)
