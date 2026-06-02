# nssf_handlers — inbound HTTP handlers (WI-nsselection-handler,
# WI-availability-handlers).
#
# Transport-agnostic request handlers. nsselection_get_handler runs the
# security gate (inbound OAuth2 bearer + scope nnssf-nsselection via
# nssf_runtime) BEFORE dispatching to the SelectionEngine (nssf_engine), then
# maps the result onto an AuthorizedNetworkSliceInfo (nssf_generated) or an RFC
# 7807 ProblemDetails (nssf_wrappers). The four nssaiavailability_* handlers do
# the same with scope nnssf-nssaiavailability over the AvailabilityEngine
# (replace/patch/delete/options); the engine's injected change-event seam is the
# deferred retry-queue/subscription-cascade enqueue (phase2 no-op). Route
# registration into the nghttp2 router is a later server-wiring step — these are
# standalone in-process entries. No outbound network — M4-independent.

add_library(nssf_handlers STATIC
    nsselection_get_handler.c
    nssaiavailability_put_handler.c
    nssaiavailability_patch_handler.c
    nssaiavailability_delete_handler.c
    nssaiavailability_options_handler.c)

target_include_directories(nssf_handlers PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(nssf_handlers PRIVATE c_std_11)

target_link_libraries(nssf_handlers
    PUBLIC
        nssf_engine
        nssf_runtime
        nssf_wrappers
        nssf_generated
    PRIVATE
        nssf_sanitizers
        nssf_warnings)
