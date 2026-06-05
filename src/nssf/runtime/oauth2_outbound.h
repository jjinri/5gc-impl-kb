/*
 * oauth2_outbound.h — outbound OAuth2 client_credentials token acquire + bearer
 * attach for NSSF (ADR-0004 M4).
 *
 * NSSF outbound SBI calls (NRF discovery, AMF NSSAIAvailability callback) carry
 * an OAuth2 access token acquired from the NRF token endpoint via the
 * client_credentials grant. This module owns the token cache (lazy acquire +
 * TTL-bounded refresh) and the "attach Authorization: Bearer" entry point.
 *
 * Security baseline (ADR-0004 M4 / M2 / M7):
 *   - token_url is https-only ENFORCED in the production ctor (mirrors the
 *     oauth2_jwks.c scheme policy). A separate test-only loopback ctor exists
 *     for http://127.0.0.1 mock NRF token endpoints.
 *   - TLS peer + hostname verification is MANDATORY on the token POST
 *     (CURLOPT_SSL_VERIFYPEER=1, CURLOPT_SSL_VERIFYHOST=2). Never disabled.
 *   - Redirects are NEVER followed (CURLOPT_FOLLOWLOCATION=0 always); a 3xx
 *     Location on the token endpoint fails closed. The redirect-policy config is
 *     VALIDATED (guardrails) but follow behavior is NOT implemented in this slice
 *     — no-follow is the guaranteed transport behavior.
 *   - fail-closed: when enabled, a token-acquire failure FORBIDS the outbound
 *     call. No unauthenticated outbound is ever emitted.
 *   - Secret hygiene: the config carries a secret_ref (an indirection handle),
 *     never the plaintext secret value, and the ref is the only secret-adjacent
 *     field — nothing here is logged.
 *
 * All crypto is delegated (OpenSSL via libcurl for TLS, libjwt for any JWS exp
 * parse). No primitives are hand-rolled (M6).
 *
 * Resilience (M4 decision #11 — bounded retry + dead-letter):
 *   token acquire 는 single-shot 가 아니라 retriable failure 에 한해 bounded
 *   exponential backoff 로 재시도한다. retriable = transport error (network /
 *   connect / TLS / timeout) | HTTP 5xx | HTTP 429. non-retriable (dead-letter)
 *   = HTTP 400/401/403 및 그 외 4xx, OAuth2 error body invalid_client /
 *   invalid_scope. retry 는 max_attempts 로 bounded — 절대 무한이 아니다.
 *   재시도는 acquire 호출 안에서 SYNCHRONOUS 하게 일어난다 (call-driven). 별도
 *   worker thread / polling loop / timer 는 없다. backoff sleep 은 acquire 경로
 *   안에서 inline 으로 수행된다. retry 소진 또는 dead-letter 결과는 모두
 *   fail-closed — enabled 일 때 ABORT 를 반환하고 unauthenticated outbound 를
 *   절대 내보내지 않으며 partial/stale token 을 캐시하지 않는다.
 */

#ifndef NSSF_RUNTIME_OAUTH2_OUTBOUND_H
#define NSSF_RUNTIME_OAUTH2_OUTBOUND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <curl/curl.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Auth method for the token request. Production default is MTLS — the NSSF
 * authenticates to the NRF token endpoint with its client certificate, and no
 * client_secret is transmitted. SECRET_* methods are dev/operator overrides and
 * are REFUSED when a production flag forbids secret auth (decision 3).
 *
 * SECRET_BASIC = HTTP Basic (preferred when secret auth is explicitly enabled).
 * SECRET_POST  = client_secret in the form body (test/explicit only).
 */
typedef enum nssf_oauth2_auth_method {
    NSSF_OAUTH2_AUTH_MTLS = 0,     /* production default. */
    NSSF_OAUTH2_AUTH_SECRET_BASIC, /* client_secret_basic — explicit opt-in. */
    NSSF_OAUTH2_AUTH_SECRET_POST,  /* client_secret_post — test/explicit only. */
} nssf_oauth2_auth_method_t;

/*
 * Redirect policy (RESERVED config — guardrails only). The transport NEVER
 * follows redirects (CURLOPT_FOLLOWLOCATION=0 always); a 3xx Location on the
 * token endpoint fails closed. follow_redirect and the fields below are
 * VALIDATED at construction (forbidden combinations are rejected) but the
 * follow behavior is NOT implemented in this slice. These fields are reserved
 * for a future redirect-follow capability; setting follow_redirect does NOT
 * cause any redirect to be followed today.
 */
typedef struct nssf_oauth2_redirect_policy {
    bool follow_redirect;                  /* reserved; no-follow is always enforced. */
    uint32_t max_hops;                      /* validated ceiling 1; reserved. */
    bool allow_host_change;                 /* host change requires allowlist; reserved. */
    const char *const *host_change_allowlist; /* permitted new hosts; caller-owned; reserved. */
    size_t host_change_allowlist_len;
} nssf_oauth2_redirect_policy_t;

/*
 * Operator-supplied outbound OAuth2 profile. All string/array lifetimes are
 * owned by the caller (config loader); this module copies only what it caches.
 *
 * No field carries a plaintext secret. secret_ref is an indirection handle the
 * deployment resolves out-of-band (env/vault/file); it is never logged and its
 * resolution is the caller's responsibility via the secret resolver seam.
 *
 * secret_ref semantics (A1): secret_ref is intended as a REFERENCE (e.g.
 * "env:NRF_CLIENT_SECRET" or "file:/run/secrets/nrf"), resolved to the real
 * secret by a resolver seam the deployment supplies BEFORE constructing this
 * config. TODO(secret-resolver-seam): a dedicated resolver is the intended
 * mechanism and is out of scope for this slice — not built here. Until that seam
 * lands, any literal value placed here is DEV-ONLY; mTLS (auth_method default)
 * is the production path and carries no secret, so this is mitigated. Whatever
 * the form, the value is never logged.
 */
typedef struct nssf_oauth2_outbound_config {
    bool enabled;                  /* M5 toggle. When false, dev no-attach path. */
    const char *token_url;         /* required. https:// in production ctor. */
    const char *nf_instance_id;    /* canonical identity (decision 2). required. */

    nssf_oauth2_auth_method_t auth_method; /* default MTLS. */
    const char *secret_ref;        /* reference handle (env:/file:), NOT the value; dev-only if literal. */
    bool allow_secret_auth_in_production; /* false → secret auth REFUSED (decision 3). */

    /* mTLS client material for the token POST — config-sourced paths only. */
    const char *client_cert_path;  /* PEM client cert (auth_method==MTLS). */
    const char *client_key_path;   /* PEM client key (auth_method==MTLS). */
    const char *ca_path;           /* PEM CA bundle for token endpoint peer verify. */

    const char *default_scope;     /* phase2 default "nnrf-disc nnrf-nfm". */

    uint32_t refresh_buffer_seconds; /* default 60 — refresh within N of expiry. */
    uint32_t max_ttl_seconds;        /* default 3600 — cap server-provided TTL. */
    uint32_t connect_timeout_seconds; /* default 2. */
    uint32_t request_timeout_seconds; /* default 5. */

    /*
     * Bounded-retry policy for token acquire (M4 decision #11). All three take
     * decision-mandated defaults when zeroed (self-documenting names). Retry is
     * call-driven and synchronous — there is NO worker thread or polling loop.
     *
     *   max_attempts    — TOTAL token-POST tries per acquire (default 3). The
     *                     first try plus up to (max_attempts-1) retries. A value
     *                     of 1 disables retry (single-shot). Bounded — never
     *                     infinite. Only RETRIABLE outcomes consume an attempt's
     *                     retry budget; a DEAD_LETTER outcome stops immediately.
     *   backoff_base_ms — base delay before the FIRST retry (default 200ms). The
     *                     delay doubles per subsequent retry (exponential).
     *   backoff_max_ms  — per-retry delay ceiling (default 2000ms). The doubled
     *                     delay is capped at this value so backoff stays bounded.
     */
    uint32_t max_attempts;    /* total tries per acquire; default 3 (>=1). */
    uint32_t backoff_base_ms; /* base backoff before first retry; default 200. */
    uint32_t backoff_max_ms;  /* per-retry backoff ceiling; default 2000. */

    nssf_oauth2_redirect_policy_t redirect; /* default no-follow. */
} nssf_oauth2_outbound_config_t;

typedef struct nssf_oauth2_outbound nssf_oauth2_outbound_t;

/*
 * Result of an acquire/attach. ABORT means the caller MUST NOT emit the outbound
 * request (fail-closed). NO_ATTACH means enabled==false and the caller may emit
 * the request without a bearer (dev path only).
 */
typedef enum nssf_oauth2_outbound_status {
    NSSF_OAUTH2_OK = 0,        /* a valid bearer is available/attached. */
    NSSF_OAUTH2_ABORT,         /* fail-closed: do NOT emit the outbound. */
    NSSF_OAUTH2_NO_ATTACH,     /* disabled: emit without bearer (dev only). */
} nssf_oauth2_outbound_status_t;

/*
 * Transport seam. The default transport POSTs to the token endpoint over libcurl
 * with TLS peer+host verification and no redirect-follow. Tests inject a mock
 * NRF token endpoint by replacing this with nssf_oauth2_outbound_set_transport.
 *
 * On success the callback writes the raw token-response JSON body into a malloc'd
 * buffer (*out_body, caller frees) and the HTTP status into *out_http_status, and
 * returns 0. Any transport/connect/TLS failure returns nonzero (fail-closed).
 *
 * form_body is the application/x-www-form-urlencoded request body
 * (grant_type=client_credentials&scope=...). basic_auth, when non-NULL, is the
 * "user:pass" for HTTP Basic (client_secret_basic); it is never logged.
 */
typedef int (*nssf_oauth2_token_transport_fn)(
    const nssf_oauth2_outbound_t *ob,
    const char *token_url,
    const char *form_body,
    const char *basic_auth,
    char **out_body,
    long *out_http_status,
    void *transport_ctx);

/*
 * Create a production token-cache handle from cfg. token_url MUST be https://;
 * a userinfo ('@') or fragment ('#') in the URL is rejected. Zeroed numeric
 * fields take their decision-mandated defaults (refresh 60s, max_ttl 3600s,
 * connect 2s, request 5s). Returns NULL on invalid/ inconsistent config:
 *   - mtls + secret material simultaneously → REJECT (decision 12),
 *   - mtls auth_method without client_cert_path AND client_key_path → REJECT
 *     (fail-fast — a production MTLS handle must be able to present a cert),
 *   - secret auth requested while allow_secret_auth_in_production==false → REJECT,
 *   - follow_redirect set with a forbidden policy → REJECT (config guardrail
 *     only; redirects are never actually followed).
 */
nssf_oauth2_outbound_t *nssf_oauth2_outbound_create(
    const nssf_oauth2_outbound_config_t *cfg);

/*
 * TEST/DEV ONLY. Mirrors nssf_jwks_cache_create_insecure: bypasses the https://
 * enforcement so integration tests can point token_url at a 127.0.0.1 loopback
 * mock NRF token endpoint over http. The host must be a loopback host (rejects
 * '@' userinfo); any other host → NULL. MUST NOT be used in production.
 */
nssf_oauth2_outbound_t *nssf_oauth2_outbound_create_insecure(
    const nssf_oauth2_outbound_config_t *cfg);

/*
 * Install a custom token transport (mock NRF endpoint) and its context. Intended
 * for tests; production uses the built-in libcurl transport. transport_fn==NULL
 * restores the default. ctx lifetime is the caller's.
 */
void nssf_oauth2_outbound_set_transport(nssf_oauth2_outbound_t *ob,
                                        nssf_oauth2_token_transport_fn transport_fn,
                                        void *transport_ctx);

/*
 * Backoff-sleep seam (TEST/DEV — additive, optional). The retry loop calls this
 * to sleep `delay_ms` milliseconds between a RETRIABLE attempt and the next try.
 * The production default sleeps for real (clock_nanosleep). Tests stub it with a
 * no-op so they can assert the attempt count and the computed delay sequence
 * WITHOUT real wall-clock sleeping. The seam is invoked once per retry with the
 * already-computed (exponential, capped) delay for that retry. It carries no
 * secret. sleep_fn==NULL restores the default. ctx lifetime is the caller's.
 */
typedef void (*nssf_oauth2_backoff_sleep_fn)(uint32_t delay_ms, void *ctx);

void nssf_oauth2_outbound_set_backoff_sleep(nssf_oauth2_outbound_t *ob,
                                            nssf_oauth2_backoff_sleep_fn sleep_fn,
                                            void *ctx);

/*
 * Dead-letter observability seam (M4 decision #11). When a token acquire fails
 * non-retriably (HTTP 400/401/403 or other 4xx, or an OAuth2 error body of
 * invalid_client / invalid_scope), the acquire records a dead-letter event: it
 * bumps a per-handle counter and, if set, invokes the operator callback with a
 * MINIMAL non-secret reason — an HTTP status (e.g. 401) and/or a short OAuth2
 * error code string (e.g. "invalid_client"). NEVER are tokens, secrets, the
 * secret_ref, request bodies, or basic-auth material passed to the callback or
 * logged. http_status==0 means "no HTTP status" (the reason is the oauth error
 * code); oauth_error==NULL means "no error code parsed". The callback must NOT
 * retain the oauth_error pointer past the call (it is borrowed).
 *
 * There is NO dead-letter queue and NO worker — this is a call-driven signal
 * only. A dead-letter outcome ALWAYS fail-closes the acquire (ABORT). cb==NULL
 * disables the callback (the counter still increments). ctx lifetime is the
 * caller's.
 */
typedef void (*nssf_oauth2_dead_letter_cb)(long http_status,
                                           const char *oauth_error,
                                           void *ctx);

void nssf_oauth2_outbound_set_dead_letter_cb(nssf_oauth2_outbound_t *ob,
                                             nssf_oauth2_dead_letter_cb cb,
                                             void *ctx);

/*
 * Observability accessors (TEST/DEV/operator). dead_letter_count returns the
 * cumulative number of dead-letter (non-retriable failure) outcomes since the
 * handle was created. retry_count returns the cumulative number of RETRIABLE
 * retries actually performed (i.e. the number of backoff-sleep invocations) —
 * tests assert this to confirm backoff happened. Both return 0 for a NULL
 * handle. Reads are mutex-guarded.
 */
uint32_t nssf_oauth2_outbound_dead_letter_count(const nssf_oauth2_outbound_t *ob);
uint32_t nssf_oauth2_outbound_retry_count(const nssf_oauth2_outbound_t *ob);

/*
 * Acquire (or return cached) a bearer token via the client_credentials grant.
 * scope_override, when non-NULL, overrides cfg->default_scope for this call
 * (per-target scope seam — decision 4); NULL uses the default.
 *
 * On NSSF_OAUTH2_OK, *out_bearer is set to an OWNED, malloc'd copy of the bearer
 * string — the CALLER MUST free() it. WHY owned (not borrowed): the cache is
 * mutex-guarded and a later refresh on this handle frees/replaces the cached
 * buffer, so returning a borrowed pointer across the lock would be a concurrent
 * use-after-free. out_bearer MUST be non-NULL; it is set to NULL on any
 * non-OK return.
 *
 * Returns NSSF_OAUTH2_ABORT on any acquire/validation failure while enabled
 * (fail-closed: no unauthenticated outbound), or NSSF_OAUTH2_NO_ATTACH when the
 * handle is disabled.
 */
nssf_oauth2_outbound_status_t nssf_oauth2_outbound_acquire(
    nssf_oauth2_outbound_t *ob,
    const char *scope_override,
    char **out_bearer);

/*
 * Outbound bearer attach entry. Acquires (lazy/cached) a token and, on success,
 * appends "Authorization: Bearer <token>" to *headers (a curl_slist the caller
 * owns; *headers may start NULL and is updated on append). scope_override as in
 * acquire. The header line is built under the cache lock so no token pointer
 * escapes the critical section (the curl_slist owns its own copy) — this is the
 * UAF-safe path and needs no caller free of the bearer.
 *
 * Returns NSSF_OAUTH2_OK on attach, NSSF_OAUTH2_ABORT to signal the caller to
 * ABORT the outbound (fail-closed), or NSSF_OAUTH2_NO_ATTACH when disabled (the
 * caller may proceed without a bearer — dev path only). *headers is left
 * unmodified on ABORT/NO_ATTACH.
 */
nssf_oauth2_outbound_status_t nssf_oauth2_outbound_attach_bearer(
    nssf_oauth2_outbound_t *ob,
    const char *scope_override,
    struct curl_slist **headers);

void nssf_oauth2_outbound_free(nssf_oauth2_outbound_t *ob);

#ifdef __cplusplus
}
#endif

#endif /* NSSF_RUNTIME_OAUTH2_OUTBOUND_H */
