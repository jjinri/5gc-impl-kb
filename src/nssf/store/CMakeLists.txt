# nssf_store — SubscriptionStore (WI-subscription-store).
#
# The NSSF's inbound subscription lifecycle persistence (SubscriptionStore.md):
# Subscribe (POST) / SubModifyPatch (PATCH) / Unsubscribe (DELETE) over the
# `subscription` table (infra/nssf/schema.sql), plus initial-snapshot-on-create
# dispatch and the on_availability_deleted() cascade-tombstone API.
#
# Mirrors nssf_repository / nssf_dispatcher: a libpq backend (production,
# PQexecParams positional params) + an in-memory test seam behind one vtable.
# create() reads the current availability snapshot (nssf_repository) and enqueues
# ONE retry_queue row through the dispatcher's store seam, then triggers ONE
# call-driven dispatch — NO worker thread / polling loop (G-08 addendum). cJSON +
# the nssf_event_notification model come from nssf_generated; libpq from the
# PostgreSQL::PostgreSQL imported target.

find_package(PostgreSQL REQUIRED)

add_library(nssf_store STATIC
    subscription_store.c)

target_include_directories(nssf_store PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(nssf_store PRIVATE c_std_11)

target_link_libraries(nssf_store
    PUBLIC
        nssf_dispatcher
        nssf_repository
        nssf_generated
        PostgreSQL::PostgreSQL
    PRIVATE
        nssf_sanitizers
        nssf_warnings)
