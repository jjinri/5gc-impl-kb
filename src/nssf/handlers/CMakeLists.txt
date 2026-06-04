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
# deferred retry-queue/subscription-cascade enqueue (phase2 no-op).
#
# The three subscription_* handlers (Post/Unsubscribe/SubModifyPatch) drive the
# SubscriptionStore (nssf_store); the Post handler validates a request callbackUri
# with the NotificationDispatcher's shared URL-policy gate (nssf_dispatcher),
# reusing the one outbound policy rather than duplicating it. Route registration
# into the nghttp2 router is a server-wiring step in nssf_server.

add_library(nssf_handlers STATIC
    nsselection_get_handler.c
    nssaiavailability_put_handler.c
    nssaiavailability_patch_handler.c
    nssaiavailability_delete_handler.c
    nssaiavailability_options_handler.c
    subscription_post_handler.c
    subscription_unsubscribe_handler.c
    subscription_patch_handler.c)

target_include_directories(nssf_handlers PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(nssf_handlers PRIVATE c_std_11)

target_link_libraries(nssf_handlers
    PUBLIC
        nssf_engine
        nssf_runtime
        nssf_wrappers
        nssf_generated
        nssf_store
        nssf_dispatcher
    PRIVATE
        nssf_sanitizers
        nssf_warnings)
