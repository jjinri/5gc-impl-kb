/*
 * main.c — NSSF process bootstrap (WI-nsselection-handler).
 *
 * Wiring order (runtime-model.md §lifecycle): TLS context → JWKS cache →
 * AvailabilityRepository → SelectionEngine + AvailabilityEngine → router →
 * nghttp2/libuv server. Graceful shutdown on SIGINT/SIGTERM: stop the listener,
 * drain in-flight streams, then close TLS context / token cache / DB pool in
 * order.
 *
 * Every secret-bearing input comes from the environment (operator config) — no
 * hardcoded cert paths, conninfo, or JWKS URL (ADR-0004 M2/M7).
 */

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "availability_engine.h"
#include "availability_repository.h"
#include "oauth2_jwks.h"
#include "router.h"
#include "selection_engine.h"
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

int main(void)
{
    int exit_code = EXIT_FAILURE;
    char errbuf[256] = {0};

    nssf_tls_context_t *tls_ctx = NULL;
    nssf_jwks_cache_t *jwks = NULL;
    nssf_availability_repo_t *repo = NULL;
    nssf_selection_engine_t *engine = NULL;
    nssf_availability_engine_t *avail_engine = NULL;
    nssf_router_t *router = NULL;
    nssf_server_t *server = NULL;

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
     * 4b. AvailabilityEngine over the same repository with zero-initialized seams.
     * The NSSAIAvailability CRUD routes are M4-independent: the change-event
     * publish seam (the deferred SubscriptionStore/NotificationDispatcher cascade)
     * is NULL here, so a committed mutation publishes nothing — no consumer is
     * registered in phase2 and there is no outbound notification egress.
     */
    nssf_availability_engine_seams_t avail_seams = {0};
    avail_engine = nssf_availability_engine_new(repo, &avail_seams);
    if (avail_engine == NULL) {
        fprintf(stderr, "nssf: availability engine init failed\n");
        goto cleanup;
    }

    /* 5. Router over the combined handler dependencies. */
    nssf_router_deps_t deps = {
        .jwks_cache = jwks,
        .selection_engine = engine,
        .availability_engine = avail_engine,
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
            "nssf: serving NSSelection + NSSAIAvailability routes on %s:%u\n",
            bind_addr ? bind_addr : "0.0.0.0", (unsigned)port);

    if (nssf_server_run(server) == 0) {
        exit_code = EXIT_SUCCESS;
    }

cleanup:
    /* Graceful shutdown order — listener already stopped by nssf_server_stop;
     * tear down transport, then security/token, then DB. */
    g_server = NULL;
    nssf_server_free(server);
    nssf_router_free(router);
    nssf_availability_engine_free(avail_engine);
    nssf_selection_engine_free(engine);
    nssf_availability_repo_free(repo);
    nssf_jwks_cache_free(jwks);
    nssf_tls_context_free(tls_ctx);
    return exit_code;
}
