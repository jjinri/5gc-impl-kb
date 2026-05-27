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
