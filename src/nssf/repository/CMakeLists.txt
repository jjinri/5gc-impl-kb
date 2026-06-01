# nssf_repository — AvailabilityRepository (WI-availability-repo).
#
# libpq/PostgreSQL backend (production) + in-memory backend (test seam) behind
# one abstract interface. Targets the `availability` table from
# infra/nssf/schema.sql. cJSON comes from nssf_generated; libpq from the
# PostgreSQL::PostgreSQL imported target (local 14.23 / CI libpq-dev).

find_package(PostgreSQL REQUIRED)

add_library(nssf_repository STATIC
    availability_repository.c)

target_include_directories(nssf_repository PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(nssf_repository PRIVATE c_std_11)

target_link_libraries(nssf_repository
    PUBLIC
        PostgreSQL::PostgreSQL
        nssf_generated
    PRIVATE
        nssf_sanitizers
        nssf_warnings)
