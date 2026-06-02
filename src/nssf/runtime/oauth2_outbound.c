/*
 * oauth2_outbound.c — outbound OAuth2 client_credentials token acquire + bearer
 * attach for NSSF (ADR-0004 M4).
 *
 * NSSF_GENERATED_BOUNDARY_STUB: none — handwritten runtime module.
 *
 * The token POST uses libcurl with TLS peer + hostname verification always on
 * (M2) and redirect-follow always off (decision 5 — no-follow guaranteed; the
 * redirect-policy config is validated only, follow is not implemented).
 * OpenSSL (via libcurl) owns TLS and
 * libjwt-adjacent crypto is not needed here — the only token introspection is a
 * non-crypto base64url+JSON read of the JWS "exp" to avoid caching an already
 * expired token (decision 10). No primitive is hand-rolled (M6).
 *
 * token_url / identity / cert paths / scope / TTLs come from operator config
 * (M7). No plaintext secret is stored or logged — only secret_ref (decision 8).
 */

#define _POSIX_C_SOURCE 200809L

#include "oauth2_outbound.h"

#include <ctype.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> /* strcasecmp */
#include <time.h>

#include <curl/curl.h>

/* Decision-mandated defaults (applied when the config field is zero). */
#define NSSF_OAUTH2_DEFAULT_REFRESH_BUFFER_S 60u
#define NSSF_OAUTH2_DEFAULT_MAX_TTL_S 3600u
#define NSSF_OAUTH2_DEFAULT_CONNECT_TIMEOUT_S 2u
#define NSSF_OAUTH2_DEFAULT_REQUEST_TIMEOUT_S 5u

/* JWS exp validation leeway (decision 10 clock_skew_leeway). */
#define NSSF_OAUTH2_CLOCK_SKEW_LEEWAY_S 60

/*
 * Owned, validated copy of the operator config. Strings are duplicated so the
 * cache outlives the caller's config buffer. NOTE: secret_ref is an indirection
 * handle (not the secret value) and is intentionally the only secret-adjacent
 * field; it is never written to any log.
 */
typedef struct {
    bool enabled;
    char *token_url;
    char *nf_instance_id;
    nssf_oauth2_auth_method_t auth_method;
    char *secret_ref;
    bool allow_secret_auth_in_production;
    char *client_cert_path;
    char *client_key_path;
    char *ca_path;
    char *default_scope;
    uint32_t refresh_buffer_seconds;
    uint32_t max_ttl_seconds;
    uint32_t connect_timeout_seconds;
    uint32_t request_timeout_seconds;
    bool follow_redirect;
    bool allow_insecure_loopback; /* test-only ctor flag. */
} owned_config_t;

struct nssf_oauth2_outbound {
    owned_config_t cfg;

    /* Token cache (decision 6). */
    char *cached_token;     /* malloc'd bearer string; NULL when empty. */
    time_t cached_expiry;   /* epoch at which cached_token expires (capped). */

    nssf_oauth2_token_transport_fn transport;
    void *transport_ctx;

    pthread_mutex_t lock;
};

/* ---- URL scheme policy (mirrors oauth2_jwks.c) ---- */

static bool has_prefix_ci(const char *url, const char *prefix)
{
    size_t i = 0;
    for (; prefix[i] != '\0'; i++) {
        if (url[i] == '\0' ||
            tolower((unsigned char)url[i]) != tolower((unsigned char)prefix[i])) {
            return false;
        }
    }
    return true;
}

/*
 * Reject userinfo('@') and fragment('#') anywhere in the URL (decision 9). A
 * fragment is never sent to the server and an '@' shifts the real host, so both
 * are config errors on a token endpoint.
 */
static bool url_has_userinfo_or_fragment(const char *url)
{
    for (const char *p = url; *p != '\0'; p++) {
        if (*p == '@' || *p == '#') {
            return true;
        }
    }
    return false;
}

/*
 * Loopback gate for the test-only ctor. Isolates the authority (between "://"
 * and the first '/', '?', '#') and rejects any '@' before trusting the host —
 * identical reasoning to oauth2_jwks.c host_is_loopback.
 */
static bool host_is_loopback(const char *url)
{
    static const char scheme[] = "http://";
    if (!has_prefix_ci(url, scheme)) {
        return false;
    }
    const char *authority = url + (sizeof(scheme) - 1);
    for (const char *p = authority; *p != '\0'; p++) {
        if (*p == '/' || *p == '?' || *p == '#') {
            break;
        }
        if (*p == '@') {
            return false;
        }
    }
    static const char *const prefixes[] = {
        "http://127.0.0.1", "http://[::1]", "http://localhost",
    };
    for (size_t i = 0; i < sizeof(prefixes) / sizeof(prefixes[0]); i++) {
        if (has_prefix_ci(url, prefixes[i])) {
            char next = url[strlen(prefixes[i])];
            if (next == '\0' || next == ':' || next == '/') {
                return true;
            }
        }
    }
    return false;
}

/* ---- small helpers ---- */

static char *dup_or_null(const char *s)
{
    return s != NULL ? strdup(s) : NULL;
}

static uint32_t nz(uint32_t v, uint32_t dflt)
{
    return v != 0 ? v : dflt;
}

static bool method_uses_secret(nssf_oauth2_auth_method_t m)
{
    return m == NSSF_OAUTH2_AUTH_SECRET_BASIC || m == NSSF_OAUTH2_AUTH_SECRET_POST;
}

/* ---- token-response JSON read (non-crypto field extraction) ---- */

/*
 * Extract a string value for a top-level JSON key from a token response body.
 * Minimal scanner sufficient for the flat {"access_token":...,"token_type":...,
 * "expires_in":...} the NRF token endpoint returns. Returns a malloc'd copy or
 * NULL. WHY hand-written: we deliberately avoid pulling a JSON dependency into
 * the runtime for one flat object; no crypto is involved.
 */
static char *json_get_string(const char *body, const char *key)
{
    size_t keylen = strlen(key);
    const char *p = body;
    while ((p = strchr(p, '"')) != NULL) {
        if (strncmp(p + 1, key, keylen) == 0 && p[1 + keylen] == '"') {
            const char *q = p + 1 + keylen + 1;
            while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r') {
                q++;
            }
            if (*q != ':') {
                p++;
                continue;
            }
            q++;
            while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r') {
                q++;
            }
            if (*q != '"') {
                return NULL; /* not a string value. */
            }
            q++;
            const char *start = q;
            while (*q != '\0' && *q != '"') {
                if (*q == '\\' && q[1] != '\0') {
                    q++; /* skip escaped char. */
                }
                q++;
            }
            if (*q != '"') {
                return NULL;
            }
            size_t len = (size_t)(q - start);
            char *out = malloc(len + 1);
            if (out == NULL) {
                return NULL;
            }
            memcpy(out, start, len);
            out[len] = '\0';
            return out;
        }
        p++;
    }
    return NULL;
}

/*
 * Extract a numeric value for a top-level JSON key. Returns true and sets *out
 * on success; false if absent/non-numeric.
 */
static bool json_get_number(const char *body, const char *key, long long *out)
{
    size_t keylen = strlen(key);
    const char *p = body;
    while ((p = strchr(p, '"')) != NULL) {
        if (strncmp(p + 1, key, keylen) == 0 && p[1 + keylen] == '"') {
            const char *q = p + 1 + keylen + 1;
            while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r') {
                q++;
            }
            if (*q != ':') {
                p++;
                continue;
            }
            q++;
            while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r') {
                q++;
            }
            char *endp = NULL;
            long long v = strtoll(q, &endp, 10);
            if (endp == q) {
                return false;
            }
            *out = v;
            return true;
        }
        p++;
    }
    return false;
}

/* ---- JWS exp parse (non-crypto base64url payload read) ---- */

static int b64url_decode_byte(char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '-') return 62;
    if (c == '_') return 63;
    return -1;
}

static char *b64url_decode(const char *in, size_t len)
{
    char *out = malloc(len + 4);
    if (out == NULL) {
        return NULL;
    }
    size_t o = 0;
    uint32_t acc = 0;
    int bits = 0;
    for (size_t i = 0; i < len; i++) {
        int v = b64url_decode_byte(in[i]);
        if (v < 0) {
            free(out);
            return NULL;
        }
        acc = (acc << 6) | (uint32_t)v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out[o++] = (char)((acc >> bits) & 0xff);
        }
    }
    out[o] = '\0';
    return out;
}

/*
 * Best-effort: if access_token is a JWS, parse its payload "exp" (seconds since
 * epoch) without verifying the signature — this is the NSSF's own outbound token
 * so signature trust is the NRF's; we only avoid caching a token already past
 * exp (decision 10). Returns true and sets *out_exp when an exp is found.
 */
static bool jws_exp(const char *token, long long *out_exp)
{
    const char *dot1 = strchr(token, '.');
    if (dot1 == NULL) {
        return false;
    }
    const char *dot2 = strchr(dot1 + 1, '.');
    if (dot2 == NULL) {
        return false; /* not a 3-part JWS. */
    }
    size_t payload_len = (size_t)(dot2 - (dot1 + 1));
    char *payload = b64url_decode(dot1 + 1, payload_len);
    if (payload == NULL) {
        return false;
    }
    bool found = json_get_number(payload, "exp", out_exp);
    free(payload);
    return found;
}

/* ---- TTL resolution (decision 6 + 10) ---- */

/*
 * Compute the cache expiry epoch from the token response. Caps any server TTL at
 * max_ttl, and if the token is a JWS with an exp, takes the earlier of the two.
 * Returns false (fail-closed) when no TTL/exp can be determined.
 */
static bool resolve_expiry(const owned_config_t *cfg, const char *body,
                           const char *access_token, time_t now, time_t *out_expiry)
{
    bool have = false;
    long long ttl = 0;

    long long expires_in = 0;
    if (json_get_number(body, "expires_in", &expires_in) && expires_in > 0) {
        ttl = expires_in;
        have = true;
    }

    long long capped_ttl = ttl;
    if (have && capped_ttl > (long long)cfg->max_ttl_seconds) {
        capped_ttl = (long long)cfg->max_ttl_seconds; /* cap server TTL. */
    }

    time_t expiry_from_ttl = 0;
    bool have_ttl = have;
    if (have_ttl) {
        expiry_from_ttl = now + (time_t)capped_ttl;
    }

    long long exp = 0;
    bool have_exp = jws_exp(access_token, &exp);
    if (have_exp) {
        /* Reject a token already past exp (with leeway) — never cache it. */
        if (exp <= (long long)now - NSSF_OAUTH2_CLOCK_SKEW_LEEWAY_S) {
            return false;
        }
        /* Also bound the JWS exp by max_ttl from now. */
        time_t exp_capped = (time_t)exp;
        time_t max_from_now = now + (time_t)cfg->max_ttl_seconds;
        if (exp_capped > max_from_now) {
            exp_capped = max_from_now;
        }
        if (!have_ttl || exp_capped < expiry_from_ttl) {
            expiry_from_ttl = exp_capped;
        }
        have_ttl = true;
    }

    if (!have_ttl) {
        return false; /* fail-closed: no TTL and no parseable exp. */
    }
    *out_expiry = expiry_from_ttl;
    return true;
}

/* ---- form body + basic auth (no secret value logged) ---- */

/*
 * Build the application/x-www-form-urlencoded body. Decision 1/2:
 * grant_type=client_credentials&scope=...&nfInstanceId=<uuid>; for SECRET_POST
 * the secret_ref is conveyed as the form client_secret (test/explicit). The
 * scope is the only caller-variable token; identity/secret are not logged by
 * this module.
 *
 * WHY nfInstanceId (not client_id): per 3GPP 29.510 Nnrf_AccessToken
 * AccessTokenReq the canonical NF identity field in the token-request body is
 * `nfInstanceId`. The HTTP Basic *username* mapping for client_secret_basic is a
 * separate auth-policy concern (see refresh_locked) and does not change the
 * 29.510 body field name.
 */
static char *build_form_body(const owned_config_t *cfg, const char *scope)
{
    /* curl_easy_escape requires a handle for escaping; do a minimal allocator. */
    CURL *tmp = curl_easy_init();
    if (tmp == NULL) {
        return NULL;
    }
    char *esc_scope = scope != NULL ? curl_easy_escape(tmp, scope, 0) : NULL;
    char *esc_id = curl_easy_escape(tmp, cfg->nf_instance_id, 0);
    char *esc_secret = NULL;
    if (cfg->auth_method == NSSF_OAUTH2_AUTH_SECRET_POST && cfg->secret_ref != NULL) {
        esc_secret = curl_easy_escape(tmp, cfg->secret_ref, 0);
    }

    size_t cap = 64;
    cap += esc_scope ? strlen(esc_scope) : 0;
    cap += esc_id ? strlen(esc_id) : 0;
    cap += esc_secret ? strlen(esc_secret) : 0;
    cap += 64;
    char *body = malloc(cap);
    if (body != NULL) {
        int n = snprintf(body, cap, "grant_type=client_credentials");
        if (esc_scope != NULL) {
            n += snprintf(body + n, cap - (size_t)n, "&scope=%s", esc_scope);
        }
        /* 29.510 AccessTokenReq canonical identity field is nfInstanceId. */
        if (esc_id != NULL) {
            n += snprintf(body + n, cap - (size_t)n, "&nfInstanceId=%s", esc_id);
        }
        if (esc_secret != NULL) {
            n += snprintf(body + n, cap - (size_t)n, "&client_secret=%s", esc_secret);
        }
        (void)n;
    }

    if (esc_scope != NULL) curl_free(esc_scope);
    if (esc_id != NULL) curl_free(esc_id);
    if (esc_secret != NULL) curl_free(esc_secret);
    curl_easy_cleanup(tmp);
    return body;
}

/* ---- libcurl response sink ---- */

typedef struct {
    char *buf;
    size_t len;
} body_sink_t;

static size_t sink_write(char *ptr, size_t size, size_t nmemb, void *userp)
{
    body_sink_t *sink = (body_sink_t *)userp;
    size_t add = size * nmemb;
    char *grown = realloc(sink->buf, sink->len + add + 1);
    if (grown == NULL) {
        return 0; /* signals curl to abort. */
    }
    sink->buf = grown;
    memcpy(sink->buf + sink->len, ptr, add);
    sink->len += add;
    sink->buf[sink->len] = '\0';
    return add;
}

/* ---- default libcurl transport (production token POST) ---- */

/*
 * The production token transport. TLS peer + hostname verification are MANDATORY
 * and never disabled; redirects are NEVER followed (CURLOPT_FOLLOWLOCATION=0
 * always — decision 5/9, ADR-0004 M2; the redirect-policy config is reserved and
 * not acted on here). basic_auth (user:pass for client_secret_basic) is set via
 * CURLOPT_USERPWD and is never logged.
 */
static int libcurl_transport(const nssf_oauth2_outbound_t *ob,
                             const char *token_url,
                             const char *form_body,
                             const char *basic_auth,
                             char **out_body,
                             long *out_http_status,
                             void *transport_ctx)
{
    (void)transport_ctx;
    const owned_config_t *cfg = &ob->cfg;

    CURL *curl = curl_easy_init();
    if (curl == NULL) {
        return -1;
    }

    body_sink_t sink = {.buf = NULL, .len = 0};
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(
        headers, "Content-Type: application/x-www-form-urlencoded");
    headers = curl_slist_append(headers, "Accept: application/json");
    if (headers == NULL) {
        curl_easy_cleanup(curl);
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, token_url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, form_body);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, sink_write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sink);

    /* TLS verification MANDATORY (ADR-0004 M2) — never 0. */
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_SSLVERSION,
                     (long)CURL_SSLVERSION_TLSv1_2); /* TLS min ≥ 1.2 (M1). */
    if (cfg->ca_path != NULL) {
        curl_easy_setopt(curl, CURLOPT_CAINFO, cfg->ca_path); /* config-sourced. */
    }

    /* mTLS client material for the token POST (config-sourced paths). */
    if (cfg->auth_method == NSSF_OAUTH2_AUTH_MTLS) {
        if (cfg->client_cert_path != NULL) {
            curl_easy_setopt(curl, CURLOPT_SSLCERT, cfg->client_cert_path);
        }
        if (cfg->client_key_path != NULL) {
            curl_easy_setopt(curl, CURLOPT_SSLKEY, cfg->client_key_path);
        }
    }

    /* client_secret_basic → HTTP Basic; carried via USERPWD, never logged. */
    if (basic_auth != NULL) {
        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long)CURLAUTH_BASIC);
        curl_easy_setopt(curl, CURLOPT_USERPWD, basic_auth);
    }

    /* Redirects NEVER followed — no-follow guaranteed (decision 5). */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);

    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT,
                     (long)cfg->connect_timeout_seconds);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)cfg->request_timeout_seconds);

    CURLcode rc = curl_easy_perform(curl);
    long status = 0;
    if (rc == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK) {
        free(sink.buf);
        return -1;
    }
    *out_body = sink.buf;
    *out_http_status = status;
    return 0;
}

/* ---- acquire core ---- */

/* Caller holds ob->lock. Returns true if cached_token is still usable. */
static bool cached_token_fresh_locked(const nssf_oauth2_outbound_t *ob, time_t now)
{
    if (ob->cached_token == NULL) {
        return false;
    }
    /* Refresh when within refresh_buffer of expiry (decision 6). */
    return now + (time_t)ob->cfg.refresh_buffer_seconds < ob->cached_expiry;
}

/*
 * Caller holds ob->lock. Performs the token POST and, on a valid 200 response
 * with a Bearer token and resolvable TTL, replaces the cache. Returns 0 on a
 * usable cached token, nonzero on any failure (fail-closed — never caches a
 * partial/expired token).
 */
static int refresh_locked(nssf_oauth2_outbound_t *ob, const char *scope)
{
    char *form = build_form_body(&ob->cfg, scope);
    if (form == NULL) {
        return -1;
    }

    /* client_secret_basic → "client_id:secret_ref" for HTTP Basic. */
    char *basic_auth = NULL;
    if (ob->cfg.auth_method == NSSF_OAUTH2_AUTH_SECRET_BASIC &&
        ob->cfg.secret_ref != NULL) {
        size_t n = strlen(ob->cfg.nf_instance_id) + 1 +
                   strlen(ob->cfg.secret_ref) + 1;
        basic_auth = malloc(n);
        if (basic_auth != NULL) {
            snprintf(basic_auth, n, "%s:%s", ob->cfg.nf_instance_id,
                     ob->cfg.secret_ref);
        }
    }

    char *body = NULL;
    long http_status = 0;
    int trc = ob->transport(ob, ob->cfg.token_url, form, basic_auth, &body,
                            &http_status, ob->transport_ctx);
    free(form);
    if (basic_auth != NULL) {
        /* Scrub the basic-auth buffer before free — never lingers in memory. */
        memset(basic_auth, 0, strlen(basic_auth));
        free(basic_auth);
    }
    if (trc != 0) {
        free(body);
        return -1; /* network/TLS/timeout failure → fail-closed. */
    }

    /* Decision 11: only 200 is success. Others → caller fail-closed (no cache). */
    if (http_status != 200 || body == NULL) {
        free(body);
        return -1;
    }

    /* Decision 10: access_token required, token_type must be Bearer (ci). */
    char *access_token = json_get_string(body, "access_token");
    char *token_type = json_get_string(body, "token_type");
    int rc = -1;
    if (access_token != NULL && access_token[0] != '\0' && token_type != NULL &&
        strcasecmp(token_type, "Bearer") == 0) {
        time_t now = time(NULL);
        time_t expiry = 0;
        if (resolve_expiry(&ob->cfg, body, access_token, now, &expiry)) {
            free(ob->cached_token);
            ob->cached_token = access_token;
            ob->cached_expiry = expiry;
            access_token = NULL; /* ownership moved into cache. */
            rc = 0;
        }
    }

    free(access_token);
    free(token_type);
    free(body);
    return rc;
}

/*
 * Caller holds ob->lock. Ensures a fresh cached token (lazy refresh) and, on
 * success, returns an OWNED malloc'd copy of the bearer in *out_copy (the caller
 * frees). WHY a copy under the lock: refresh_locked may free/replace
 * ob->cached_token on a later call, so a pointer into the cache must never
 * escape the lock — copying inside the critical section is the UAF-safe contract
 * for the thread-safe API. Returns 0 on success, nonzero (fail-closed) on
 * refresh failure or OOM.
 */
static int acquire_owned_copy_locked(nssf_oauth2_outbound_t *ob,
                                     const char *scope, char **out_copy)
{
    *out_copy = NULL;
    time_t now = time(NULL);
    if (!cached_token_fresh_locked(ob, now)) {
        if (refresh_locked(ob, scope) != 0) {
            return -1; /* fail-closed: no unauth outbound. */
        }
    }
    if (ob->cached_token == NULL) {
        return -1;
    }
    char *copy = strdup(ob->cached_token);
    if (copy == NULL) {
        return -1;
    }
    *out_copy = copy;
    return 0;
}

nssf_oauth2_outbound_status_t nssf_oauth2_outbound_acquire(
    nssf_oauth2_outbound_t *ob,
    const char *scope_override,
    char **out_bearer)
{
    if (out_bearer != NULL) {
        *out_bearer = NULL;
    }
    if (ob == NULL || out_bearer == NULL) {
        return NSSF_OAUTH2_ABORT;
    }
    if (!ob->cfg.enabled) {
        return NSSF_OAUTH2_NO_ATTACH; /* dev no-attach path (decision 7). */
    }

    const char *scope =
        scope_override != NULL ? scope_override : ob->cfg.default_scope;

    pthread_mutex_lock(&ob->lock);
    char *owned = NULL;
    int rc = acquire_owned_copy_locked(ob, scope, &owned);
    pthread_mutex_unlock(&ob->lock);

    if (rc != 0 || owned == NULL) {
        return NSSF_OAUTH2_ABORT;
    }
    *out_bearer = owned; /* OWNED — caller must free(). */
    return NSSF_OAUTH2_OK;
}

nssf_oauth2_outbound_status_t nssf_oauth2_outbound_attach_bearer(
    nssf_oauth2_outbound_t *ob,
    const char *scope_override,
    struct curl_slist **headers)
{
    if (ob == NULL || headers == NULL) {
        return NSSF_OAUTH2_ABORT;
    }
    if (!ob->cfg.enabled) {
        return NSSF_OAUTH2_NO_ATTACH; /* dev no-attach path (decision 7). */
    }

    const char *scope =
        scope_override != NULL ? scope_override : ob->cfg.default_scope;

    /*
     * Build the full "Authorization: Bearer <token>" line INSIDE the lock so the
     * cached-token pointer never escapes the critical section (UAF-safe). The
     * curl_slist_append below copies the line, so once appended under lock the
     * header is independent of the cache buffer.
     */
    pthread_mutex_lock(&ob->lock);
    char *owned = NULL;
    int rc = acquire_owned_copy_locked(ob, scope, &owned);
    struct curl_slist *appended = NULL;
    if (rc == 0 && owned != NULL) {
        size_t n = sizeof("Authorization: Bearer ") + strlen(owned);
        char *line = malloc(n);
        if (line != NULL) {
            snprintf(line, n, "Authorization: Bearer %s", owned);
            appended = curl_slist_append(*headers, line);
            free(line);
        }
    }
    free(owned);
    pthread_mutex_unlock(&ob->lock);

    if (rc != 0) {
        return NSSF_OAUTH2_ABORT; /* fail-closed (refresh failed). */
    }
    if (appended == NULL) {
        return NSSF_OAUTH2_ABORT; /* OOM on the header line. */
    }
    *headers = appended;
    return NSSF_OAUTH2_OK;
}

/* ---- config validation + construction ---- */

/*
 * Validate the consistency mandates that have no defaults:
 *   - mtls + secret material simultaneously → reject (decision 12),
 *   - mtls in production WITHOUT cert+key → reject (fail-fast, see below),
 *   - secret auth requested while production forbids it → reject (decision 3),
 *   - follow_redirect opted in with a forbidden policy → reject (decision 5).
 *
 * require_mtls_material is true for the production ctor and false for the
 * test-only loopback ctor. WHY the split: a production MTLS handle that silently
 * lacks a client cert/key would build successfully and then fail to present a
 * certificate at TLS time — a silent security downgrade. The production ctor
 * therefore fails-fast. The test-only loopback ctor relaxes this so unit tests
 * can drive the cache against a mock NRF without real cert files on disk.
 *
 * Returns true when the config is acceptable.
 */
static bool config_is_consistent(const nssf_oauth2_outbound_config_t *cfg,
                                 bool require_mtls_material)
{
    if (cfg->token_url == NULL || cfg->token_url[0] == '\0') {
        return false;
    }
    if (cfg->nf_instance_id == NULL || cfg->nf_instance_id[0] == '\0') {
        return false;
    }

    bool has_secret = cfg->secret_ref != NULL && cfg->secret_ref[0] != '\0';
    bool has_cert =
        cfg->client_cert_path != NULL && cfg->client_cert_path[0] != '\0';
    bool has_key =
        cfg->client_key_path != NULL && cfg->client_key_path[0] != '\0';
    bool has_mtls_material = has_cert || has_key;

    /* Decision 12: mTLS and client_secret are mutually exclusive. */
    if (cfg->auth_method == NSSF_OAUTH2_AUTH_MTLS && has_secret) {
        return false;
    }
    if (method_uses_secret(cfg->auth_method) && has_mtls_material) {
        return false;
    }

    /*
     * Fail-fast: a production MTLS handle MUST carry both a client cert and key,
     * else it cannot present a client certificate at the TLS handshake. The
     * test-only loopback ctor passes require_mtls_material=false to allow a
     * mock/loopback override without cert files.
     */
    if (cfg->auth_method == NSSF_OAUTH2_AUTH_MTLS && require_mtls_material &&
        !(has_cert && has_key)) {
        return false;
    }

    /* Decision 3: secret auth refused unless explicitly allowed. */
    if (method_uses_secret(cfg->auth_method) &&
        !cfg->allow_secret_auth_in_production) {
        return false;
    }
    if (method_uses_secret(cfg->auth_method) && !has_secret) {
        return false; /* secret method without a secret_ref is incoherent. */
    }

    /*
     * Decision 5: redirect-policy guardrails (config validation only — redirects
     * are never actually followed; these checks reject incoherent reserved
     * config, they do not enable follow behavior).
     */
    if (cfg->redirect.follow_redirect) {
        if (cfg->redirect.max_hops > 1) {
            return false; /* ceiling is one hop. */
        }
        if (cfg->redirect.allow_host_change &&
            (cfg->redirect.host_change_allowlist == NULL ||
             cfg->redirect.host_change_allowlist_len == 0)) {
            return false; /* host change requires a non-empty allowlist. */
        }
    }

    return true;
}

static void owned_config_free(owned_config_t *c)
{
    free(c->token_url);
    free(c->nf_instance_id);
    free(c->secret_ref);
    free(c->client_cert_path);
    free(c->client_key_path);
    free(c->ca_path);
    free(c->default_scope);
}

static nssf_oauth2_outbound_t *create_with_scheme_policy(
    const nssf_oauth2_outbound_config_t *cfg, bool allow_insecure_loopback)
{
    if (cfg == NULL || cfg->token_url == NULL || cfg->token_url[0] == '\0') {
        return NULL;
    }
    if (url_has_userinfo_or_fragment(cfg->token_url)) {
        return NULL; /* decision 9. */
    }
    if (allow_insecure_loopback) {
        if (!host_is_loopback(cfg->token_url)) {
            return NULL;
        }
    } else if (!has_prefix_ci(cfg->token_url, "https://")) {
        return NULL; /* production: https-only ENFORCED (decision 9). */
    }
    /*
     * Production (allow_insecure_loopback==false) requires real mTLS cert+key
     * for an MTLS handle; the test-only loopback ctor relaxes that requirement.
     */
    if (!config_is_consistent(cfg, /*require_mtls_material=*/!allow_insecure_loopback)) {
        return NULL;
    }

    nssf_oauth2_outbound_t *ob = calloc(1, sizeof(*ob));
    if (ob == NULL) {
        return NULL;
    }
    if (pthread_mutex_init(&ob->lock, NULL) != 0) {
        free(ob);
        return NULL;
    }

    ob->cfg.enabled = cfg->enabled;
    ob->cfg.token_url = dup_or_null(cfg->token_url);
    ob->cfg.nf_instance_id = dup_or_null(cfg->nf_instance_id);
    ob->cfg.auth_method = cfg->auth_method;
    ob->cfg.secret_ref = dup_or_null(cfg->secret_ref);
    ob->cfg.allow_secret_auth_in_production = cfg->allow_secret_auth_in_production;
    ob->cfg.client_cert_path = dup_or_null(cfg->client_cert_path);
    ob->cfg.client_key_path = dup_or_null(cfg->client_key_path);
    ob->cfg.ca_path = dup_or_null(cfg->ca_path);
    ob->cfg.default_scope =
        dup_or_null(cfg->default_scope != NULL ? cfg->default_scope
                                               : "nnrf-disc nnrf-nfm");
    ob->cfg.refresh_buffer_seconds =
        nz(cfg->refresh_buffer_seconds, NSSF_OAUTH2_DEFAULT_REFRESH_BUFFER_S);
    ob->cfg.max_ttl_seconds =
        nz(cfg->max_ttl_seconds, NSSF_OAUTH2_DEFAULT_MAX_TTL_S);
    ob->cfg.connect_timeout_seconds = nz(cfg->connect_timeout_seconds,
                                         NSSF_OAUTH2_DEFAULT_CONNECT_TIMEOUT_S);
    ob->cfg.request_timeout_seconds = nz(cfg->request_timeout_seconds,
                                         NSSF_OAUTH2_DEFAULT_REQUEST_TIMEOUT_S);
    ob->cfg.follow_redirect = cfg->redirect.follow_redirect;
    ob->cfg.allow_insecure_loopback = allow_insecure_loopback;

    if (ob->cfg.token_url == NULL || ob->cfg.nf_instance_id == NULL ||
        ob->cfg.default_scope == NULL) {
        owned_config_free(&ob->cfg);
        pthread_mutex_destroy(&ob->lock);
        free(ob);
        return NULL;
    }

    ob->cached_token = NULL;
    ob->cached_expiry = 0;
    ob->transport = libcurl_transport;
    ob->transport_ctx = NULL;
    return ob;
}

nssf_oauth2_outbound_t *nssf_oauth2_outbound_create(
    const nssf_oauth2_outbound_config_t *cfg)
{
    return create_with_scheme_policy(cfg, false);
}

nssf_oauth2_outbound_t *nssf_oauth2_outbound_create_insecure(
    const nssf_oauth2_outbound_config_t *cfg)
{
    return create_with_scheme_policy(cfg, true);
}

void nssf_oauth2_outbound_set_transport(
    nssf_oauth2_outbound_t *ob,
    nssf_oauth2_token_transport_fn transport_fn,
    void *transport_ctx)
{
    if (ob == NULL) {
        return;
    }
    pthread_mutex_lock(&ob->lock);
    ob->transport = transport_fn != NULL ? transport_fn : libcurl_transport;
    ob->transport_ctx = transport_ctx;
    pthread_mutex_unlock(&ob->lock);
}

void nssf_oauth2_outbound_free(nssf_oauth2_outbound_t *ob)
{
    if (ob == NULL) {
        return;
    }
    owned_config_free(&ob->cfg);
    free(ob->cached_token);
    pthread_mutex_destroy(&ob->lock);
    free(ob);
}
