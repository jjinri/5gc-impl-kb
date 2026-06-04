/*
 * main.c — NSSF process bootstrap (WI-nsselection-handler).
 *
 * Wiring order (runtime-model.md §lifecycle): TLS context → JWKS cache →
 * AvailabilityRepository → SelectionEngine → SubscriptionStore → retry store +
 * outbound OAuth2 + NotificationDispatcher → AvailabilityEngine (with the live
 * change-event publish seam) → router → nghttp2/libuv server. The dispatcher /
 * stores are built before the AvailabilityEngine so its fan-out publish ctx is
 * live. Graceful shutdown on SIGINT/SIGTERM: stop the listener, drain in-flight
 * streams, then free the engine before its cascade collaborators, then close TLS
 * context / token cache / DB pool in order.
 *
 * This is the NSSF's FIRST real outbound egress: a committed availability mutation
 * publishes a change event → SubscriptionStore fan-out match → retry_queue enqueue
 * → ONE call-driven dispatch (no loop / worker thread — G-08 addendum posture).
 *
 * Every secret-bearing input comes from the environment (operator config) — no
 * hardcoded cert paths, conninfo, JWKS URL, or outbound OAuth2 token_url/cert
 * (ADR-0004 M2/M7). Outbound OAuth2 is M5-toggled via NSSF_OAUTH2_OUTBOUND_ENABLED;
 * when off the dispatcher runs the dev no-attach path (no bearer).
 */

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "availability_engine.h"
#include "availability_repository.h"
#include "notification_dispatcher.h"
#include "oauth2_jwks.h"
#include "oauth2_outbound.h"
#include "router.h"
#include "selection_engine.h"
#include "subscription_store.h"
#include "tls_context.h"

/* The running server, exposed to the signal handler for graceful stop. */
static nssf_server_t *g_server = NULL;

static void on_signal(int signo)
{
    (void)signo;
    /* nssf_server_stop wakes the loop via an async-signal-safe libuv async. */
    if (g_server != NULL) {
        nssf_server_stop(g_server);
    }
}

/* Required env var → value; logs and returns NULL when unset/empty. */
static const char *env_required(const char *name)
{
    const char *v = getenv(name);
    if (v == NULL || v[0] == '\0') {
        fprintf(stderr, "nssf: required config %s is not set\n", name);
        return NULL;
    }
    return v;
}

static uint16_t env_port(const char *name, uint16_t fallback)
{
    const char *v = getenv(name);
    if (v == NULL || v[0] == '\0') {
        return fallback;
    }
    long p = strtol(v, NULL, 10);
    if (p <= 0 || p > 65535) {
        return fallback;
    }
    return (uint16_t)p;
}

/* env flag → bool; "1"/"true"/"yes" (case-insensitive) enable, else disabled. */
static bool env_flag(const char *name)
{
    const char *v = getenv(name);
    if (v == NULL || v[0] == '\0') {
        return false;
    }
    return v[0] == '1' || v[0] == 't' || v[0] == 'T' || v[0] == 'y' || v[0] == 'Y';
}

/*
 * Fan-out cascade context — the live collaborators the AvailabilityEngine change
 * event needs to reach an outbound notification. Borrowed (owned by main, freed
 * after the engine). Installed as nssf_availability_engine_seams_t.ctx.
 */
typedef struct {
    nssf_subscription_store_t *sub_store;
    nssf_retry_store_t *retry_store;
    nssf_notification_dispatcher_t *dispatcher;
} nssf_fanout_ctx_t;

/*
 * Change-event publish trampoline — matches nssf_availability_change_publish_fn.
 * On each committed availability mutation it (1) resolves the event to the
 * affected live subscriptions and enqueues ONE retry_queue row per match
 * (SubscriptionStore fan-out — store-layer match, no outbound here), then (2)
 * triggers EXACTLY ONE call-driven dispatch_pending. Strictly single-shot: NO
 * loop, NO worker thread — the dispatcher dispatches at most one row per call and
 * the caller (this trampoline) fires once per change event. WHY single-shot: a
 * loop here would promote the G-08 fail-closed hot-loop gap to a blocker
 * (open-gaps-and-assumptions G-08 addendum). The outbound POST itself stays
 * entirely inside the dispatcher (ADR-0004 Layer A: URL gate before bearer attach
 * + POST, fail-closed on OAuth2 ABORT).
 */
static void fanout_publish(void *ctx, const nssf_availability_change_event_t *ev)
{
    nssf_fanout_ctx_t *fc = (nssf_fanout_ctx_t *)ctx;
    if (fc == NULL || ev == NULL) {
        return;
    }
    (void)nssf_subscription_store_fanout_change(fc->sub_store, fc->retry_store, ev);
    /* ONE dispatch — call-driven, at most one row. No re-dispatch loop. */
    if (fc->dispatcher != NULL) {
        (void)nssf_notification_dispatcher_dispatch_pending(fc->dispatcher, NULL);
    }
}

int main(void)
{
    int exit_code = EXIT_FAILURE;
    char errbuf[256] = {0};

    nssf_tls_context_t *tls_ctx = NULL;
    nssf_jwks_cache_t *jwks = NULL;
    nssf_availability_repo_t *repo = NULL;
    nssf_selection_engine_t *engine = NULL;
    nssf_availability_engine_t *avail_engine = NULL;
    nssf_subscription_store_t *sub_store = NULL;
    nssf_retry_store_t *retry_store = NULL;
    nssf_oauth2_outbound_t *oauth = NULL;
    nssf_notification_dispatcher_t *dispatcher = NULL;
    nssf_router_t *router = NULL;
    nssf_server_t *server = NULL;
    nssf_fanout_ctx_t fanout_ctx = {0};

    /* ── operator config from environment ── */
    const char *cert = env_required("NSSF_TLS_SERVER_CERT");
    const char *key = env_required("NSSF_TLS_SERVER_KEY");
    const char *ca = env_required("NSSF_TLS_CLIENT_CA");
    const char *jwks_url = env_required("NSSF_OAUTH2_JWKS_URL");
    const char *conninfo = env_required("NSSF_DB_CONNINFO");
    if (cert == NULL || key == NULL || ca == NULL || jwks_url == NULL ||
        conninfo == NULL) {
        return EXIT_FAILURE;
    }
    const char *bind_addr = getenv("NSSF_BIND_ADDRESS");
    uint16_t port = env_port("NSSF_BIND_PORT", 8443);

    /* 1. TLS context (mTLS, peer verification mandatory inside the wrapper). */
    nssf_tls_config_t tls_cfg = {
        .enabled = true,
        .server_cert_path = cert,
        .server_key_path = key,
        .client_ca_path = ca,
        .min_version = getenv("NSSF_TLS_MIN_VERSION"),     /* NULL → TLSv1.3. */
        .tls13_ciphersuites = getenv("NSSF_TLS13_CIPHERS"),
        .tls12_cipher_list = getenv("NSSF_TLS12_CIPHERS"),
    };
    tls_ctx = nssf_tls_context_create(&tls_cfg, errbuf, sizeof(errbuf));
    if (tls_ctx == NULL) {
        fprintf(stderr, "nssf: TLS context init failed: %s\n", errbuf);
        goto cleanup;
    }

    /* 2. JWKS cache for inbound OAuth2 bearer validation (M3). */
    jwks = nssf_jwks_cache_create(jwks_url, 0);
    if (jwks == NULL) {
        fprintf(stderr, "nssf: JWKS cache init failed (jwks_url must be https)\n");
        goto cleanup;
    }

    /* 3. AvailabilityRepository — libpq backend, conninfo from operator config. */
    repo = nssf_availability_repo_new_pg(conninfo, errbuf, sizeof(errbuf));
    if (repo == NULL) {
        fprintf(stderr, "nssf: availability repository init failed: %s\n", errbuf);
        goto cleanup;
    }

    /*
     * 4. SelectionEngine over the repository with zero-initialized seams.
     * NSSelectionGet is read-only and M4-independent: the UDM/NRF collaborators
     * are injected seams (NULL here → body-sourced subscribed set, no outbound
     * AMF resolution). An operator that wants AMF-set resolution supplies a
     * config-backed resolver — never a network call in this slice.
     */
    nssf_selection_seams_t seams = {0};
    engine = nssf_selection_engine_new(repo, &seams);
    if (engine == NULL) {
        fprintf(stderr, "nssf: selection engine init failed\n");
        goto cleanup;
    }

    /*
     * 4b. SubscriptionStore — libpq backend over the same operator conninfo. The
     * Subscription routes (Post/Unsubscribe/SubModifyPatch) resolve through this
     * store, and the fan-out cascade (4d) reads the live subscriptions from it.
     * Built before the AvailabilityEngine so the change-event publish ctx is live.
     */
    sub_store = nssf_subscription_store_new_pg(conninfo, errbuf, sizeof(errbuf));
    if (sub_store == NULL) {
        fprintf(stderr, "nssf: subscription store init failed: %s\n", errbuf);
        goto cleanup;
    }

    /*
     * 4c. Outbound notification path — retry queue + OAuth2 + dispatcher. This is
     * the NSSF's FIRST real outbound egress.
     *
     * retry_store: libpq backend over the same operator conninfo (the producer
     * enqueues into it; dispatch_pending dequeues with FOR UPDATE SKIP LOCKED).
     *
     * oauth: outbound OAuth2 (ADR-0004 M4) constructed from operator config — no
     * hardcoded url/cert/secret (M2/M7). M5 enabled toggle: when
     * NSSF_OAUTH2_OUTBOUND_ENABLED is not set the handle is NULL and the dispatcher
     * runs the dev no-attach path (no bearer). PRODUCTION POSTURE: an operator that
     * needs authenticated outbound MUST set NSSF_OAUTH2_OUTBOUND_ENABLED=1 with the
     * mTLS client material below; the production ctor enforces https-only token_url
     * and fails closed on acquire failure. A NULL oauth here is a dev/local posture.
     */
    retry_store = nssf_retry_store_new_pg(conninfo, errbuf, sizeof(errbuf));
    if (retry_store == NULL) {
        fprintf(stderr, "nssf: retry store init failed: %s\n", errbuf);
        goto cleanup;
    }

    if (env_flag("NSSF_OAUTH2_OUTBOUND_ENABLED")) {
        const char *token_url = env_required("NSSF_OAUTH2_OUTBOUND_TOKEN_URL");
        const char *nf_id = env_required("NSSF_NF_INSTANCE_ID");
        const char *oauth_cert = env_required("NSSF_OAUTH2_OUTBOUND_CLIENT_CERT");
        const char *oauth_key = env_required("NSSF_OAUTH2_OUTBOUND_CLIENT_KEY");
        const char *oauth_ca = env_required("NSSF_OAUTH2_OUTBOUND_CA");
        if (token_url == NULL || nf_id == NULL || oauth_cert == NULL ||
            oauth_key == NULL || oauth_ca == NULL) {
            goto cleanup;
        }
        nssf_oauth2_outbound_config_t oauth_cfg = {
            .enabled = true,
            .token_url = token_url,             /* https-only enforced by the ctor. */
            .nf_instance_id = nf_id,
            .auth_method = NSSF_OAUTH2_AUTH_MTLS, /* production default — no secret. */
            .client_cert_path = oauth_cert,
            .client_key_path = oauth_key,
            .ca_path = oauth_ca,
            .default_scope = getenv("NSSF_OAUTH2_OUTBOUND_SCOPE"),
        };
        oauth = nssf_oauth2_outbound_create(&oauth_cfg);
        if (oauth == NULL) {
            fprintf(stderr, "nssf: outbound OAuth2 init failed (token_url must be "
                            "https; mTLS client cert/key required)\n");
            goto cleanup;
        }
    }

    /* PRODUCTION dispatcher — https-only callback gate, never _new_insecure. oauth
     * may be NULL (dev no-attach path); the dispatcher then attaches no bearer. */
    dispatcher = nssf_notification_dispatcher_new(retry_store, oauth);
    if (dispatcher == NULL) {
        fprintf(stderr, "nssf: notification dispatcher init failed\n");
        goto cleanup;
    }

    /*
     * 4d. AvailabilityEngine with the LIVE change-event publish seam. A committed
     * NSSAIAvailability mutation now publishes a change event into fanout_publish,
     * which resolves the affected live subscriptions, enqueues one retry_queue row
     * per match, and triggers ONE call-driven dispatch — the engine→store→
     * dispatcher cascade (B3 close). The engine is constructed AFTER the dispatcher
     * + store so the publish ctx is live. Single-shot only: no loop / worker thread.
     */
    fanout_ctx.sub_store = sub_store;
    fanout_ctx.retry_store = retry_store;
    fanout_ctx.dispatcher = dispatcher;
    nssf_availability_engine_seams_t avail_seams = {
        .publish = fanout_publish,
        .ctx = &fanout_ctx,
    };
    avail_engine = nssf_availability_engine_new(repo, &avail_seams);
    if (avail_engine == NULL) {
        fprintf(stderr, "nssf: availability engine init failed\n");
        goto cleanup;
    }

    /* Install the SubscriptionStore initial-snapshot seam now that the
     * collaborators exist: create() then performs the ONE call-driven initial-
     * snapshot dispatch for a newly created subscription (activates the already-
     * built create() path that main.c previously left uninstalled). */
    nssf_subscription_store_set_snapshot_seam(sub_store, repo, retry_store,
                                              dispatcher);

    /* 5. Router over the combined handler dependencies. */
    nssf_router_deps_t deps = {
        .jwks_cache = jwks,
        .selection_engine = engine,
        .availability_engine = avail_engine,
        .subscription_store = sub_store,
    };
    router = nssf_router_create(&deps);
    if (router == NULL) {
        fprintf(stderr, "nssf: router init failed\n");
        goto cleanup;
    }

    /* 6. nghttp2/libuv server. */
    nssf_server_config_t srv_cfg = {
        .bind_address = bind_addr,
        .port = port,
    };
    server = nssf_server_create(&srv_cfg, tls_ctx, router, errbuf, sizeof(errbuf));
    if (server == NULL) {
        fprintf(stderr, "nssf: server init failed: %s\n", errbuf);
        goto cleanup;
    }

    g_server = server;
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
    signal(SIGPIPE, SIG_IGN);

    fprintf(stderr,
            "nssf: serving NSSelection + NSSAIAvailability + Subscription routes "
            "on %s:%u (outbound notification %s)\n",
            bind_addr ? bind_addr : "0.0.0.0", (unsigned)port,
            oauth != NULL ? "with OAuth2 bearer"
                          : "no-attach (dev: OAuth2 disabled)");

    if (nssf_server_run(server) == 0) {
        exit_code = EXIT_SUCCESS;
    }

cleanup:
    /* Graceful shutdown order — listener already stopped by nssf_server_stop;
     * tear down transport, then the outbound cascade, then security/token, then DB.
     * The AvailabilityEngine holds the fan-out ctx (→ dispatcher / stores), so it
     * is freed BEFORE those collaborators; the dispatcher borrows the retry_store
     * and oauth, so it is freed before them. */
    g_server = NULL;
    nssf_server_free(server);
    nssf_router_free(router);
    nssf_availability_engine_free(avail_engine);
    nssf_notification_dispatcher_free(dispatcher);
    nssf_oauth2_outbound_free(oauth);
    nssf_retry_store_free(retry_store);
    nssf_subscription_store_free(sub_store);
    nssf_selection_engine_free(engine);
    nssf_availability_repo_free(repo);
    nssf_jwks_cache_free(jwks);
    nssf_tls_context_free(tls_ctx);
    return exit_code;
}
