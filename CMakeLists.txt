cmake_minimum_required(VERSION 3.22)
project(nssf C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(NSSF_ENABLE_SANITIZERS "Enable ASan/UBSan for Debug builds" ON)

add_library(nssf_sanitizers INTERFACE)
if(NSSF_ENABLE_SANITIZERS)
    target_compile_options(nssf_sanitizers INTERFACE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        -g
        -O1)
    target_link_options(nssf_sanitizers INTERFACE
        -fsanitize=address,undefined)
endif()

add_library(nssf_warnings INTERFACE)
target_compile_options(nssf_warnings INTERFACE
    -Wall -Wextra -Werror)

enable_testing()

add_subdirectory(src/nssf/generated)
add_subdirectory(src/nssf)
add_subdirectory(tests/nssf)
